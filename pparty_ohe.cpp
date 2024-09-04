#include "emp-tool/emp-tool.h"
#include "emp-ot/emp-ot.h"
#include "test_util.h"
#include <unordered_map>
#include <vector>
#include <iostream>

using namespace emp ;
using namespace std ;

using pairID = int ;

const static int threads = 1;

inline void parse_options(const char *const *arg, int *party, int *parties, int *start_port, int *length, string &ot_type, int *tool) {
	*party = atoi (arg[1]) ;
    *parties = atoi (arg[2]) ;
    *start_port = atoi(arg[3]) ;
    *length = 1 << atoi(arg[4]) ;
    ot_type = arg[5] ;
    *tool = atoi(arg[6]) ;
}

// gpid stands for get pairID
pairID gpoffset(pair<int,int> pp, int parties) {
    int p1, p2 ;
    p1 = pp.first ; p2 = pp.second ;
    return (p1-1)*parties + (p2 - 1) ;
}

unordered_map<int, NetIO*> get_pairwise_channels(int party, int parties, int start_port, vector<pair<int,int>> robin) {
    unordered_map<int, NetIO*> ios ;
    NetIO *io ;
    for (auto it = robin.begin() ; it != robin.end() ; it++) {
        int p1 = it->first, p2 = it->second ;
        int port_no = start_port + gpoffset(*it, parties) ;
        // cout << "Going to conect (" << p1 << ", " << p2 << ") at port " << port_no << "\n" ;
        io = new NetIO(party == p1 ? nullptr : "127.0.0.1", port_no) ;
        ios[p1 == party ? p2 : p1] = io ;
    }

    return ios ;
}

unordered_map<int, NetIO*> get_pairwise_channels_threaded(int party, int parties, int start_port) {
    unordered_map<int, NetIO*> ios ;
    ThreadPool pool(parties - 1) ;

    for (int i = 1 ; i <= parties ; i++) {
        if (i == party)
            continue ;

        int p1 = party < i ? party : i, p2 = party < i ? i : party ;
        int port_no = start_port + gpoffset(make_pair(p1, p2), parties) ;
        pool.enqueue([party, i, port_no, &ios] {
            // cout << "Going to connect (" << p1 << ", " << p2 << ") at port " << port_no << "\n" ;
            ios[i] = new NetIO(party < i ? nullptr : "127.0.0.1", port_no) ;
        }) ;        
    }

    return ios ;
}

template <typename T>
class Coordinator {
public :
    unordered_map<int, T*> ots ;
    vector<pair<int,int>> robin ;
    int party, parties, length, loglength ;

    Coordinator(int party, int parties, int length, int loglength, vector<pair<int,int>> &robin, unordered_map<int, T*> &ots) {
        this->party = party ;
        this->parties = parties ;
        this->length = length ;
        this->loglength = loglength ;
        this->robin = robin ;
        this->ots = ots ;
    }

    void dummy_ots() {
        block *b0 = new block[this->length] ;
        block *b1 = new block[this->length] ;
        unordered_map<int, block*> rcvs ;
        bool *b = new bool[this->length] ;

        PRG prg ;
        prg.random_block(b0, this->length) ;
        prg.random_block(b1, this->length) ;
        prg.random_bool(b, this->length) ;
        
        for (int i = 1 ; i <= this->parties ; i++) {
            if (i == this->party)
                continue ;
            rcvs[i] = new block[this->length] ;
        }

        for (auto it = robin.begin() ; it != robin.end() ; it++) {
            int p1 = it->first, p2 = it->second ;
            if (party == p1) {
                // cout << "SR (" << p1 << ", " << p2 << ")\n" ;
                this->ots[p2]->send(b0, b1, this->length) ; this->ots[p2]->io->flush() ;
                this->ots[p2]->recv(rcvs[p2], b, this->length) ; 
            } else {
                // cout << "RS (" << p1 << ", " << p2 << ")\n" ;
                this->ots[p1]->recv(rcvs[p1], b, this->length) ;
                this->ots[p1]->send(b0, b1, this->length) ; this->ots[p1]->io->flush() ;
            }
        }
    }

    void dummy_ots_threaded() {
        block *b0 = new block[this->length] ;
        block *b1 = new block[this->length] ;
        unordered_map<int, block*> rcvs ;
        bool *b = new bool[this->length] ;

        PRG prg ;
        prg.random_block(b0, this->length) ;
        prg.random_block(b1, this->length) ;
        prg.random_bool(b, this->length) ;
        
        for (int i = 1 ; i <= this->parties ; i++) {
            if (i == this->party)
                continue ;

            rcvs[i] = new block[this->length] ;
        }

        ThreadPool pool(this->parties - 1) ;
        // cout << "Created threadpool\n" ;

        for (int i = 1 ; i <= this->parties ; i++) {
            if (i == this->party)
                continue ;

            block *rcv = rcvs[i] ;
            pool.enqueue([i, b0, b1, b, this, rcv] {
                if (this->party < i) {
                    this->ots[i]->send(b0, b1, this->length) ; this->ots[i]->io->flush() ;
                    this->ots[i]->recv(rcv, b, this->length) ; 
                } else {
                    this->ots[i]->recv(rcv, b, this->length) ; this->ots[i]->io->flush() ;
                    this->ots[i]->send(b0, b1, this->length) ;
                }
                this->ots[i]->io->flush() ;
            }) ;
        }
    }

    int64_t test_gmt() {
        unordered_map<int, vector<vector<int>>> mp ;
        unordered_map<int, int> singleton ;
        unordered_map<int, int> remaining ;

        int n = this->loglength ;

        mp = get_subsets(n) ;
        for (int i = 2 ; i < n+1 ; i++) {
            for (int j = 0 ; j < (int)mp[i].size() ; j++) {
                vector<int> vec = mp[i][j] ;
                int totrnk = total_rank(vec, n) ;
                singleton[totrnk] = vec[0] ; 
                vector<int> cop = vec ; cop.erase(cop.begin()) ; remaining[totrnk] = total_rank(cop, n) ;
            }
        }

        PRG prg ;
        block *hot = new block[this->length] ; 
        prg.random_block(hot+1, n) ;

        block *a, *b_blk, *andd, *loc, *loc_tmp, *fin, *fin_tmp ;
        unordered_map<int, block*> r, rcv, a_masked ;
        bool *b ;

        // b_stretched = new block[1] ; 
        andd = new block[1]; 
        loc = new block[1] ; loc_tmp = new block[1] ;
        fin = new block[1] ; fin_tmp = new block[1] ;
        b = new bool[128] ;

        for (auto it = this->robin.begin() ; it != this->robin.end() ; it++) {
            int p1 = it->first, p2 = it->second ;

            r[p1 == party ? p2 : p1] = new block[1] ;
            rcv[p1 == party ? p2 : p1] = new block[1] ;
            a_masked[p1 == party ? p2 : p1] = new block[1] ;
        }

        for (int i = n+1 ; i < this->length ; i++) {
            a = hot + singleton[i] ;

            b_blk = hot + remaining[i] ;
            *b = get_bools_from_block(b_blk) ;
            // stretch_bool(b_stretched, *b, 1) ;
            // copyBools_arr(b_stretched, b_blk, 1) ;

            ThreadPool pool(this->parties - 1) ;

            for (auto it = this->robin.begin() ; it != this->robin.end() ; it++) {
                int p1 = it->first, p2 = it->second ;
                int wh = p1 == party ? p2 : p1 ;
                prg.random_block(r[wh], 1) ;
                xorBlocks_arr(a_masked[wh], a, r[wh], 1) ;

                // cout << "Targetting party " << wh << "\n" ;
                if (p1 == party) {
                    // cout << i << "--> SR : " << this->party << "-" << p << "\n" ; 
                    this->ots[wh]->send(r[wh], a_masked[wh], 1) ; this->ots[wh]->io->flush() ;
                    this->ots[wh]->recv(rcv[wh], b, 128) ;
                } else {
                    // cout << i << "--> RS : " << p << "-" << this->party << "\n" ;
                    this->ots[wh]->recv(rcv[wh], b, 128) ; this->ots[wh]->io->flush() ;
                    this->ots[wh]->send(r[wh], a_masked[wh], 1) ; 
                }
                this->ots[wh]->io->flush() ;
            }

            // Computing final share
            andBlocks_arr(andd, a, b_blk, 1) ;
            copyBlocks_arr(loc_tmp, andd, 1) ;
            for (int p = 1 ; p <= this->parties ; p++) {
                if (p == this->party)
                    continue ;

                xorBlocks_arr(loc, loc_tmp, rcv[p], 1) ;
                copyBlocks_arr(loc_tmp, loc, 1) ;
            }
            copyBlocks_arr(fin_tmp, loc, 1) ;
            for (int p = 1 ; p <= this->parties ; p++) {
                if (p == this->party)
                    continue ;

                xorBlocks_arr(fin, fin_tmp, r[p], 1) ;
                copyBlocks_arr(fin_tmp, fin, 1) ;
            }

            copyBlocks_arr(hot+i, fin, 1) ;
        }

        int64_t total_comms = 0 ;
        for (int p = 1 ; p <= this->parties ; p++) {
            if (p == this->party)
                continue ;

            total_comms += this->ots[p]->io->counter ;
        }
            
        return total_comms ;
    }

    int64_t test_gmt_threaded() {
        unordered_map<int, vector<vector<int>>> mp ;
        unordered_map<int, int> singleton ;
        unordered_map<int, int> remaining ;

        int n = this->loglength ;

        mp = get_subsets(n) ;
        for (int i = 2 ; i < n+1 ; i++) {
            for (int j = 0 ; j < (int)mp[i].size() ; j++) {
                vector<int> vec = mp[i][j] ;
                int totrnk = total_rank(vec, n) ;
                singleton[totrnk] = vec[0] ; 
                vector<int> cop = vec ; cop.erase(cop.begin()) ; remaining[totrnk] = total_rank(cop, n) ;
            }
        }

        PRG prg ;
        block *hot = new block[this->length] ; 
        prg.random_block(hot+1, n) ;

        block *a, *b_blk, *andd, *loc, *loc_tmp, *fin, *fin_tmp ;
        unordered_map<int, block*> r, rcv, a_masked ;
        bool *b ;

        // b_stretched = new block[1] ; 
        andd = new block[1]; 
        loc = new block[1] ; loc_tmp = new block[1] ;
        fin = new block[1] ; fin_tmp = new block[1] ;
        b = new bool[128] ;

        for (int p = 1 ; p <= this->parties ; p++) {
            if (p == this->party)
                continue ;

            r[p] = new block[1] ;
            rcv[p] = new block[1] ;
            a_masked[p] = new block[1] ;
        }

        for (int i = n+1 ; i < this->length ; i++) {
            a = hot + singleton[i] ;

            b_blk = hot + remaining[i] ;
            *b = get_bools_from_block(b_blk) ;
            // stretch_bool(b_stretched, *b, 1) ;
            // copyBools_arr(b_stretched, b_blk, 1) ;

            ThreadPool pool(this->parties - 1) ;

            for (int p = 1 ; p <= this->parties ; p++) {
                if (p == this->party)
                    continue ;

                prg.random_block(r[p], 1) ;
                xorBlocks_arr(a_masked[p], a, r[p], 1) ;

                // cout << "Targetting party " << p << "\n" ;
                pool.enqueue([p, this, &r, &a_masked, b, &rcv] {
                    this->ots[p]->io->sync() ;
                    if (this->party < p) {
                        // cout << i << "--> SR : " << this->party << "-" << p << "\n" ; 
                        this->ots[p]->send(r[p], a_masked[p], 1) ; this->ots[p]->io->flush() ;
                        this->ots[p]->recv(rcv[p], b, 128) ;
                    } else {
                        // cout << i << "--> RS : " << p << "-" << this->party << "\n" ;
                        this->ots[p]->recv(rcv[p], b, 128) ; this->ots[p]->io->flush() ;
                        this->ots[p]->send(r[p], a_masked[p], 1) ;    
                    }
                    this->ots[p]->io->flush() ;
                }) ;
            }

            // Computing final share
            andBlocks_arr(andd, a, b_blk, 1) ;
            copyBlocks_arr(loc_tmp, andd, 1) ;
            for (int p = 1 ; p <= this->parties ; p++) {
                if (p == this->party)
                    continue ;

                xorBlocks_arr(loc, loc_tmp, rcv[p], 1) ;
                copyBlocks_arr(loc_tmp, loc, 1) ;
            }
            copyBlocks_arr(fin_tmp, loc, 1) ;
            for (int p = 1 ; p <= this->parties ; p++) {
                if (p == this->party)
                    continue ;

                xorBlocks_arr(fin, fin_tmp, r[p], 1) ;
                copyBlocks_arr(fin_tmp, fin, 1) ;
            }

            copyBlocks_arr(hot+i, fin, 1) ;
        }

        int64_t total_comms = 0 ;
        for (int p = 1 ; p <= this->parties ; p++) {
            if (p == this->party)
                continue ;

            total_comms += this->ots[p]->io->counter ;
        }
            
        return total_comms ;
    }
} ;

vector<pair<int,int>> remove_dummy(vector<pair<int,int>> robin, int parties) {
    pair<int,int> pp ;
    for (auto it = robin.begin() ; it != robin.end() ; it++) {
        pp = *it ;
        if (pp.first == parties || pp.second == parties) {
            robin.erase(it) ;
            break ;
        }
    }

    return robin ;
}

vector<pair<int,int>> get_round_robin_scheme(int party, int parties) {
    if (parties % 2) {
        vector<pair<int,int>> robin = get_round_robin_scheme(party, parties + 1) ;
        return remove_dummy(robin, parties + 1) ;
    } else {
        vector<pair<int,int>> robin ;
        vector<pair<int,int>> circle ;

        // Initialize the circle
        for (int i = 1, j = parties ; i < j ; i++, j--)
            circle.push_back(make_pair(i, j)) ;

        // Trivial case
        if (parties == 2)
            return circle ;

        // (parties - 1) rounds in total
        for (int i = 0 ; i < parties - 1 ; i++) {
            // Insert into robin
            for (auto it = circle.begin() ; it != circle.end() ; it++) {
                if (it->first == party || it->second == party) {
                    pair<int,int> pp ;
                    if (it->first < it->second)
                        pp = *it ;
                    else
                        pp = make_pair(it->second, it->first) ;
                    robin.push_back(pp) ;
                }
            }
                
            // Rotate
            /*
            1   2   3   4   5   6   7
            14  13  12  11  10  9   8

            save 14, save 7
            1   14  2   3   4   5   6
            14  13  12  11  10  9   8

            1   14  2   3   4   5   6
            13  12  11  10  9   8   7
            */

            int save = circle[0].second ;
            int tmp ;
            for (auto it = circle.begin() + 1 ; it != circle.end() ; it++) {
                tmp = it->first ;
                it->first = save ;
                save = tmp ;
            }

            for (auto it = circle.rbegin() ; it != circle.rend() ; it++) {
                tmp = it->second ;
                it->second = save ;
                save = tmp ;
            }
        }

        return robin ;
    }
}

int main(int argc, char** argv) {
    int party, parties, start_port, length, tool ;
    string ot_type ;
    parse_options(argv, &party, &parties, &start_port, &length, ot_type, &tool) ;
    int loglength = atoi(argv[4]) ;

    vector<pair<int,int>> robin ;
    robin = get_round_robin_scheme(party, parties) ;

    unordered_map<int, NetIO*> ios ;

    if (parties > 6 && ot_type == "ferret")
        ios = get_pairwise_channels(party, parties, start_port, robin) ;
    else 
        ios = get_pairwise_channels_threaded(party, parties, start_port) ;

    auto start = clock_start(); int64_t comms ;
    if (ot_type == "otnp") {
        unordered_map<int, OTNP<NetIO>*> ots ;
        for (auto &it : ios)
            ots[it.first] = new OTNP<NetIO>(it.second) ;
        Coordinator<OTNP<NetIO>> cood(party, parties, length, loglength, robin, ots) ;
        // cout << "Created OTNP coordinator\n" ;
        if (tool == 0)
            comms = cood.test_gmt_threaded() ; // cout << "Total comms : " << comms << "\n" ;
    }
    else if (ot_type == "iknp") {
        unordered_map<int, IKNP<NetIO>*> ots ;
        for (auto &it : ios)
            ots[it.first] = new IKNP<NetIO>(it.second) ;
        Coordinator<IKNP<NetIO>> cood(party, parties, length, loglength, robin, ots) ;
        // cout << "Created INKP coordinator\n" ;
        if (tool == 0)
            comms = cood.test_gmt_threaded() ; // cout << "Total comms : " << comms << "\n" ;
    }
    else if (ot_type == "simple") {
        unordered_map<int, OTCO<NetIO>*> ots ;
        for (auto &it : ios)
            ots[it.first] = new OTCO<NetIO>(it.second) ;
        Coordinator<OTCO<NetIO>> cood(party, parties, length, loglength, robin, ots) ;
        // cout << "Created OTCO coordinator\n" ;
        if (tool == 0)
            comms = cood.test_gmt_threaded() ; // cout << "Total comms : " << comms << "\n" ;
    }
    else if (ot_type == "ferret") {
        unordered_map<int, FerretCOT<NetIO>*> ots ;
        for (auto &it : ios) {
            cout << "Party " << party << " connected to " << it.first ;
            ots[it.first] = new FerretCOT<NetIO>(party < it.first ? ALICE : BOB, threads, &it.second, false) ;
            cout << " --> done\n" ;
        }
            
        Coordinator<FerretCOT<NetIO>> cood(party, parties, length, loglength, robin, ots) ;
        cout << "Created Ferret coordinator\n" ;
        if (tool == 0) {
            if (parties > 6 && ot_type == "ferret")
                comms = cood.test_gmt() ;   
            else
                comms = cood.test_gmt_threaded() ;
        }
            // comms = cood.test_gmt_threaded() ; // cout << "Total comms : " << comms << "\n" ;
    } else {
        exit(1) ;
    }
    
    // cout << "Done with dummy OTs\n" ;
    std::setprecision(2) ;
    long long t = time_from(start);    
    std::cout << std::fixed << std::setprecision(2) << "Time taken : " << double(t)/1e3 << " ms\n" ;
    std::cout << std::fixed << std::setprecision(2) << "Comm : " << comms << " bytes\n" ;

    return 0 ;
}