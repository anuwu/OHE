#include "emp-tool/emp-tool.h"
#include "emp-ot/emp-ot.h"
#include "test_util.h"
#include <unordered_map>
#include <vector>
#include <iostream>

using namespace emp ;
using namespace std ;

const static int threads = 1;

inline void parse_options(const char *const *arg, int *party, int *port, int *length, string &ot_type, int *tool) {
	*party = atoi (arg[1]) ;
	*port = atoi (arg[2]) ;
    *length = 1 << atoi(arg[3]) ;
    ot_type = arg[4] ;
    *tool = atoi(arg[5]) ;
}

template <typename T>
void test_ohe(T *ot, int party, int64_t length) {
    block ones = makeBlock(~0ULL, ~0ULL) ;

    PRG prg ;
    block *Hbeta, *Hbeta_tmp, *mask, *Hmasked, *rcv, *alpha_stretched, *localAnd, *localShare_intermediate, *localShare ;
    bool *alpha ;

    // Hot vector H(beta)
    Hbeta = new block[2] ;
    prg.random_block(Hbeta, 1) ;
    xorBlocks_arr(&Hbeta[1], Hbeta, ones, 1) ;

    for (int64_t l = 2 ; l < length ; l *= 2) {
        // Mask
        mask = new block[l] ;
        prg.random_block(mask, l) ;

        // H(beta) + mask
        Hmasked = new block[l] ;
        xorBlocks_arr(Hmasked, Hbeta, mask, l) ;

        // Choice bit alpha
        alpha = new bool[l] ;
        prg.random_bool(alpha, 1) ;
        copyBools_arr(alpha + 1, *alpha, l-1) ;

        // Sending and receiving OT
        rcv = new block[l] ;
        if (party == ALICE) {
            ot->send(mask, Hmasked, l) ; ot->io->flush() ;
            ot->recv(rcv, alpha, l) ; ot->io->flush() ;
        } else {
            ot->recv(rcv, alpha, l) ; ot->io->flush();
            ot->send(mask, Hmasked, l) ; ot->io->flush();
        }

        // Computing local AND
        alpha_stretched = new block[l] ;
        stretch_bool(alpha_stretched, alpha[l], l) ;
        localAnd = new block[l] ;
        andBlocks_arr(localAnd, alpha_stretched, Hbeta, l) ;

        // Computing local share
        localShare_intermediate = new block[l] ;
        localShare = new block[l] ;
        xorBlocks_arr(localShare_intermediate, localAnd, mask, l) ;
        xorBlocks_arr(localShare, localShare_intermediate, rcv, l) ;

        // Copy Hbeta0 to Hbeta0_tmp
        Hbeta_tmp = new block[l] ;
        copyBlocks_arr(Hbeta_tmp, Hbeta, l) ;

        // Delete temporary variables
        delete[] Hbeta ;
        delete[] mask ;
        delete[] Hmasked ;
        delete[] alpha ;
        delete[] rcv ;
        delete[] alpha_stretched ;
        delete[] localAnd ;
        delete[] localShare_intermediate ;
        
        // Stack old OHE to new OHE
        Hbeta = new block[2*l] ;
        xorBlocks_arr(Hbeta, Hbeta_tmp, localShare, l) ;
        copyBlocks_arr(Hbeta + l, Hbeta_tmp, l) ;

        // Clear for next iteration
        delete[] localShare ;
        delete[] Hbeta_tmp ;
    }
}

template<typename T>
void test_gmt(T *ot, int party, int64_t length, int64_t n) {
    unordered_map<int, vector<vector<int>>> mp ;
    unordered_map<int, int> singleton ;
    unordered_map<int, int> remaining ;

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
    block *hot = new block[length] ; 
    prg.random_block(hot+1, n) ;

    block *a, *a_masked, *r, *b_blk, *b_stretched, *rcv, *andd, *loc ;
    bool *b ;

    a_masked = new block[1]; 
    r = new block[1]; 
    b_stretched = new block[1] ; 
    rcv = new block[1]; 
    andd = new block[1]; 
    loc = new block[1] ;
    b = new bool[1] ;

    for (int i = n+1 ; i < length ; i++) {
        a = hot + singleton[i] ;
        prg.random_block(r, 1) ;

        b_blk = hot + remaining[i] ;
        *b = get_bool_from_block(b_blk) ;
        xorBlocks_arr(a_masked, a, r, 1) ;

        if (party == ALICE) {
            ot->send(r, a_masked, 1) ; ot->io->flush() ;
            ot->recv(rcv, b, 1) ;
        } else {
            ot->recv(rcv, b, 1) ; ot->io->flush() ;
            ot->send(r, a_masked, 1) ;
        }
        ot->io->flush() ;

        // Computing final share
        andBlocks_arr(andd, a, b_stretched, 1) ;
        xorBlocks_arr(loc, andd, rcv, 1) ;
        xorBlocks_arr(hot+i, loc, r, 1) ;
    }
}


int main(int argc, char** argv) {
	int length, port, party, tool; // make sure all functions work for non-power-of-two lengths
    uint64_t comm ;
    string ot_type ;

	parse_options(argv, &party, &port, &length, ot_type, &tool) ;
	NetIO *io = new NetIO(party==ALICE ? nullptr:"127.0.0.1", port) ;

    auto start = clock_start();
    if (ot_type == "otnp") {
        OTNP<NetIO> *ot = new OTNP<NetIO>(io) ;
        if (tool)
            test_ohe<OTNP<NetIO>>(ot, party, length) ;
        else
            test_gmt<OTNP<NetIO>>(ot, party, length, atoi(argv[3])) ;
        comm = ot->io->counter ;
    }
    else if (ot_type == "iknp") {
        IKNP<NetIO> *ot = new IKNP<NetIO>(io) ;
        if (tool)
            test_ohe<IKNP<NetIO>>(ot, party, length) ;
        else
            test_gmt<IKNP<NetIO>>(ot, party, length, atoi(argv[3])) ;
        comm = ot->io->counter ;
    }
    else if (ot_type == "simple") {
        OTCO<NetIO> *ot = new OTCO<NetIO>(io) ;
        if (tool)
            test_ohe<OTCO<NetIO>>(ot, party, length) ;
        else
            test_gmt<OTCO<NetIO>>(ot, party, length, atoi(argv[3])) ;
        comm = ot->io->counter ;
    }
    else if (ot_type == "ferret") {
        FerretCOT<NetIO> *ot = new FerretCOT<NetIO>(party, threads, &io, false) ;
        if (tool)
            test_ohe<FerretCOT<NetIO>>(ot, party, length) ;
        else
            test_gmt<FerretCOT<NetIO>>(ot, party, length, atoi(argv[3])) ;
        comm = ot->io->counter ;
    } else {
        exit(1) ;
    }

    std::setprecision(2) ;
    long long t = time_from(start);    
    std::cout << std::fixed << std::setprecision(2) << "Time taken : " << double(t)/1e3 << " ms\n" ;
    std::cout << std::fixed << std::setprecision(2) << "Comm : " << comm << " bytes\n" ;

    return 0 ;
}