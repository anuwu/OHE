#include "emp-ot/emp-ot.h"
#include "test_util.h"
#include <iostream>
#include <time.h>

using namespace std ;
using namespace emp ;

constexpr int threads = 1 ;

int main(int argc, char** argv) {
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " <party-id> <port> <n> <iknp/ferret> \n";
    exit(EXIT_FAILURE);
   };

  int party, port, n ;
  string ot_type ;
  NetIO *io ;
  COT<NetIO> *ot1, *ot2 ;

  if (argc != 5)
    abort();
  
  party = atoi(argv[1]) ;
  port = atoi(argv[2]) ;
  n = atoi(argv[3]) ;
  ot_type = argv[4] ;

  io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port) ;

  if (ot_type == "iknp") {
    ot1 = new IKNP<NetIO>(io) ;
    ot2 = new IKNP<NetIO>(io) ;
  } 
  else if (ot_type == "ferret") {
    ot1 = new FerretCOT<NetIO>(party, threads, &io) ;
    ot2 = new FerretCOT<NetIO>(party == 1 ? 2 : 1, threads, &io) ;
  }
  
  block *msg = new block[1] ;
  block *rcv_msg = new block[1] ;
  int counter = 0 ;

  do {
    msg[0] = makeBlock(0L, 0xabcd) ;
    rcv_msg[0] = zero_block ;

    if (party == ALICE) {
    ot1->io->send_data(msg, 32) ;
    ot2->io->recv_data(rcv_msg, 32) ;
    } else {
      ot1->io->recv_data(rcv_msg, 32) ;
      ot2->io->send_data(msg, 32) ;
    }
    ot1->io->flush() ; 
    ot2->io->flush() ;

    counter++ ;
    cout << counter << "\n" ;
  } while (check_equal(msg, rcv_msg)) ;

  cout << "The message - " << msg[0] << "\n" ;
  if (check_equal(msg, rcv_msg)) {
    cout << "They are equal!" << "\n" ;
  } else {
    cout << "They are not equal!" << "\n" ;
  }

  delete[] msg ;
  delete[] rcv_msg ;
}