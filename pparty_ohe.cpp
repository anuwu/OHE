#include "emp-tool/emp-tool.h"
#include "emp-ot/emp-ot.h"
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
        if (p1 == party || p2 == party) {
            int port_no = start_port + gpoffset(*it, parties) ;
            cout << "Going to conect (" << p1 << ", " << p2 << ")\n" ;
            io = new NetIO(party == p1 ? nullptr : "127.0.0.1", port_no) ;
            ios[p1 == party ? p2 : p1] = io ;
        }
    }

    return ios ;
}

unordered_map<int, NetIO*> get_pairwise_channels_threaded(int party, int parties, int start_port) {
    unordered_map<int, NetIO*> ios ;
    NetIO *io ;

    ThreadPool pool(parties - 1) ;

    for (int i = 1 ; i <= parties ; i++) {
        if (i == party)
            continue ;

        int p1 = party < i ? party : i, p2 = party < i ? i : party ;
        int port_no = start_port + gpoffset(make_pair(p1, p2), parties) ;
        pool.enqueue([party, p1, p2, port_no, &io] {
            cout << "Going to connect (" << p1 << ", " << p2 << ")\n\n" ;
            io = new NetIO(party == p1 ? nullptr : "127.0.0.1", port_no) ;
        }) ;
        ios[i] = io ;
    }

    return ios ;
}


template <typename T>
class Coordinator {
public :
    unordered_map<int, T*> ots ;
    int party, parties, length ;

    Coordinator(int party, int parties, int length, unordered_map<int, T*> &ots) {
        this->party = party ;
        this->parties = parties ;
        this->ots = ots ;
        this->length = length ;
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

        ThreadPool pool(this->parties - 1) ;
        cout << "Created threadpool\n" ;

        for (int i = 1 ; i <= this->parties ; i++) {
            if (i == this->party)
                continue ;

            block *rcv = rcvs[i] ;
            pool.enqueue([i, b0, b1, b, this, &rcv] {
                if (this->party < i) {
                    this->ots[i]->send(b0, b1, this->length) ; this->ots[i]->io->flush() ;
                    this->ots[i]->recv(rcv, b, this->length) ; 
                } else {
                    this->ots[i]->recv(rcv, b, this->length) ; 
                    this->ots[i]->send(b0, b1, this->length) ; this->ots[i]->io->flush() ;
                }
                this->ots[i]->io->flush() ;
            }) ;
        }
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

    vector<pair<int,int>> robin ;
    robin = get_round_robin_scheme(party, parties) ;

    unordered_map<int, NetIO*> ios = get_pairwise_channels(party, parties, start_port, robin) ;
    // unordered_map<int, NetIO*> ios = get_pairwise_channels_threaded(party, parties, start_port) ;


    if (ot_type == "otnp") {
        unordered_map<int, OTNP<NetIO>*> ots ;
        for (auto &it : ios)
            ots[it.first] = new OTNP<NetIO>(it.second) ;
        Coordinator<OTNP<NetIO>> cood(party, parties, length, ots) ;
        // cood.dummy_ots() ;
    }
    else if (ot_type == "iknp") {
        unordered_map<int, IKNP<NetIO>*> ots ;
        for (auto &it : ios)
            ots[it.first] = new IKNP<NetIO>(it.second) ;
        Coordinator<IKNP<NetIO>> cood(party, parties, length, ots) ;
        // cood.dummy_ots() ;
    }
    else if (ot_type == "simple") {
        unordered_map<int, OTCO<NetIO>*> ots ;
        for (auto &it : ios)
            ots[it.first] = new OTCO<NetIO>(it.second) ;
        Coordinator<OTCO<NetIO>> cood(party, parties, length, ots) ;
        // cood.dummy_ots() ;
    }
    else if (ot_type == "ferret") {
        unordered_map<int, FerretCOT<NetIO>*> ots ;
        for (auto &it : ios)
            ots[it.first] = new FerretCOT<NetIO>(party, threads, &it.second, false) ;
        Coordinator<FerretCOT<NetIO>> cood(party, parties, length, ots) ;
        // cood.dummy_ots() ;
    } else {
        exit(1) ;
    }

    cout << "Done with dummy OTs\n" ;

    return 0 ;
}