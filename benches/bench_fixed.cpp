#include "utils.h"
#include <iostream>
#include <time.h>

using namespace std ;
using namespace emp ;

void get_cost(int party, int port, string ot_type, uint64_t num_ots, long long &time, uint64_t &comm, bool silent=false) {
  // Declare variables
  PRG prg ;
  NetIO *io ;
  COT<NetIO> *ot1, *ot2 ; 

  // Setup OT stuff
  io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port, true) ;
  uint64_t running_comm = io->counter ;
  uint64_t pre_comm ;
  auto total_start = clock_start() ;
  if (ot_type == "iknp") {
    ot1 = new IKNP<NetIO>(io) ;
    ot2 = new IKNP<NetIO>(io) ;
  } 
  else if (ot_type == "ferret") {
    ot1 = new FerretCOT<NetIO>(party, 1, &io) ;
    ot2 = new FerretCOT<NetIO>(party == 1 ? 2 : 1, 1, &io) ;
  }
  else {
    cerr << "Incorrect OT type\n" ;
    exit(EXIT_FAILURE) ;
  }

  // Object initialization cost
  pre_comm = io->counter - running_comm ;
  if (!silent)
    cout << "Pre-comm : " << pre_comm << " bytes\n" ;
  running_comm = io->counter ;

  // Allocate for OT
  block *r0 = new block[num_ots] ;
  block *r1 = new block[num_ots] ;
  block *rcv_ot = new block[num_ots] ;
  bool *b = new bool[num_ots] ;
  prg.random_bool(b, num_ots) ;

  // Perform OT
  if (party == ALICE) {
    ot1->send_rot(r0, r1, num_ots) ;
    ot2->recv_rot(rcv_ot, b, num_ots) ;
  } else {
    ot1->recv_rot(rcv_ot, b, num_ots) ;
    ot2->send_rot(r0, r1, num_ots) ;
  }
  ot1->io->flush() ;
  ot2->io->flush() ;

  // End measurements
  time = time_from(total_start) ;
  comm = (io->counter - running_comm) + pre_comm ;

  // Delete and return
  delete[] r0 ;
  delete[] r1 ;
  delete[] rcv_ot ;
  delete[] b ;
}

int main(int argc, char** argv) {
  /************************* Parse Input *************************/
  
  // Abort message
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " <party-id> <port> <n> <iknp/ferret> \n";
    exit(EXIT_FAILURE);
   };

  // Declare variables
  int party, port, n ;
  string ot_type ;

  // Check command line arguments
  if (argc != 5)
    abort() ;
  
  // Parse arguments
  party = atoi(argv[1]) ;
  port = atoi(argv[2]) ;
  n = atoi(argv[3]) ;
  ot_type = argv[4] ;

  /************************* Experiment *************************/

  long long time ;
  uint64_t comm ;
  if (ot_type == "iknp")
    get_cost(party, port, ot_type, 128ULL*n, time, comm) ;
  else if (ot_type == "ferret")
    get_cost(party, port, ot_type, 1ULL<<n, time, comm) ;
  cout << fixed << setprecision(MEASUREMENT_PRECISION) << "Fixed time : " << time/1e3 << " ms\n" ;
  cout << "Fixed comm : " << comm << " bytes\n" ;

  return 0 ;
}