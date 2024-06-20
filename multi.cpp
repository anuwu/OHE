#include "emp-tool/emp-tool.h"
#include "emp-ot/emp-ot.h"
#include <unordered_map>
#include <vector>
#include <iostream>

using namespace emp ;
using namespace std ;

const static int threads = 1;

inline void parse_options(const char *const *arg, int *party, int *port12, int *port23, int *port13, int *length) {
	*party = atoi (arg[1]) ;
	*port12 = atoi (arg[2]) ;
    *port23 = atoi (arg[3]) ;
    *port13 = atoi (arg[4]) ;
    *length = 1 << atoi(arg[5]) ;
}

template <typename T>
void test_ot(T *ot12, T *ot23, T *ot13, int party, int64_t length) {
    if (party == 1) {
        PRG prg1 ;
        block *b0 = new block[length], *b1 = new block[length] ;
        prg1.random_block(b0, length) ;
        prg1.random_block(b1, length) ;

        cout << "The blocks are -\n" ;
        for (int32_t i = 0 ; i < length ; i++)
            cout << b0[i] << " | " << b1[i] << "\n" ;
        cout << "\n" ;
        ot12->send(b0, b1, length) ; ot12->io->flush() ; cout << "Party 1 send to 2\n" ;
        ot13->send(b0, b1, length) ; ot13->io->flush() ; cout << "Party 1 send to 3\n" ;

        delete[] b0 ;
        delete[] b1 ;
    } else if (party == 2) {
        PRG prg2 ;
        block *rcv2 = new block[length] ;
        bool *b2 = new bool[length] ;
        prg2.random_bool(b2, length) ;

        cout << "The bool is " << b2[0] << "\n" ;
        ot12->recv(rcv2, b2, length) ; ot12->io->flush() ; cout << "Party 2 received from 1\n\n" ;
        cout << "Received -\n" ;
        for (int32_t i = 0 ; i < length ; i++)
            cout << rcv2[i] << "\n" ;

        delete[] rcv2 ;
        delete[] b2 ;
    } else if (party == 3) {
        PRG prg3 ;
        block *rcv3 = new block[length] ;
        bool *b3 = new bool[length] ;
        prg3.random_bool(b3, length) ;

        cout << "The bool is " << b3[0] << "\n" ;
        ot13->recv(rcv3, b3, length) ; ot13->io->flush() ; cout << "Party 3 received from 1\n\n" ;
        cout << "Received -\n" ;
        for (int32_t i = 0 ; i < length ; i++)
            cout << rcv3[i] << "\n" ;

        delete[] rcv3 ;
        delete[] b3 ;
    }
}

int main(int argc, char** argv) {
	int length, port12, port23, port13, party ;
    // uint64_t comm ;

	parse_options(argv, &party, &port12, &port23, &port13, &length) ;

    NetIO *io12, *io23, *io13 ;
    IKNP<NetIO> *ot12, *ot23, *ot13 ;
    

    if (party == 1) {
        io12 = new NetIO(nullptr, port12) ;
        io13 = new NetIO(nullptr, port13) ;

        ot12 = new IKNP<NetIO>(io12) ;
        ot13 = new IKNP<NetIO>(io13) ;
    } else if (party == 2) {
        io12 = new NetIO("127.0.0.1", port12) ;
        io23 = new NetIO(nullptr, port23) ;

        ot12 = new IKNP<NetIO>(io12) ;
        ot23 = new IKNP<NetIO>(io23) ;
    } else if (party == 3) {
        io13 = new NetIO("127.0.0.1", port13) ;
        io23 = new NetIO("127.0.0.1", port23) ;

        ot13 = new IKNP<NetIO>(io13) ;
        ot23 = new IKNP<NetIO>(io23) ;
    }

    test_ot<IKNP<NetIO>>(ot12, ot23, ot13, party, length) ;

    return 0 ;
}