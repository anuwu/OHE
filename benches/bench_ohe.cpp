#include "ohe.h"
#include "utils.h"
#include <iostream>
#include <time.h>

using namespace std ;
using namespace emp ;

int main(int argc, char** argv) {
  /************************* Parse Input *************************/
  
  // Abort message
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " <party-id> <port> <n> <iknp/ferret> <ohe/gmt> <batched> <opt:batch_size> \n";
    exit(EXIT_FAILURE);
   };

  // Declare variables
  int64_t comm_var = 0 ;
  int party, port, n, batch_size ;
  string ot_type, prot_type ;
  bool batched ;
  NetIO *io ;
  COT<NetIO> *ot1, *ot2 ;

  // Check command line arguments
  if (argc < 7)
    abort() ;
  if (argc == 7) {
    if (atoi(argv[6]))
      abort() ;
  }
  else if (argc > 8)
    abort() ;
  
  // Parse arguments
  party = atoi(argv[1]) ;
  port = atoi(argv[2]) ;
  n = atoi(argv[3]) ;
  ot_type = argv[4] ;
  prot_type = argv[5] ;
  batched = atoi(argv[6]) ;
  if (batched)
    batch_size = atoi(argv[7]) ;
  
  /************************* Create OT *************************/

  io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port) ;
  comm_var = io->counter ;
  if (ot_type == "iknp") {
    ot1 = new IKNP<NetIO>(io) ;
    ot2 = new IKNP<NetIO>(io) ;
  } 
  else if (ot_type == "ferret") {
    ot1 = new FerretCOT<NetIO>(party, 1, &io) ;
    ot2 = new FerretCOT<NetIO>(party == 1 ? 2 : 1, 1, &io) ;
  }
  else {
    cout << "Incorrect OT type\n" ;
    exit(EXIT_FAILURE) ;
  }

  /************************* Experiment *************************/

  if (batched) {
    // Declare and initialize
    block **ohes ;
    
    // Get OHEs
    auto start_exp = clock_start(); 
    if (prot_type == "ohe")
      ohes = batched_random_ohe(party, n, batch_size, ot1, ot2, true) ;
    else
      ohes = batched_random_gmt(party, n, batch_size, ot1, ot2, true) ;
    long long t_exp = time_from(start_exp);  
    comm_var = io->counter - comm_var ;

    // Delete
    for (int b = 0 ; b < batch_size ; b++)
      delete[] ohes[b] ;
    delete[] ohes ;

    // Print things
    setprecision(5) ;  
    cout << fixed << setprecision(5) << "Time taken : " << double(t_exp)/(1e3*batch_size) << " ms\n" ;
  }
  else {
    block *ohe ;
    /************************* Dummy stuff *************************/

    // block *b1 = new block[100] ;
    // block *b2 = new block[100] ;
    // block *recv = new block[100] ;
    // bool *choices = new bool[100] ;

    // // Measure
    // auto start_dummy = clock_start(); 
    // if (party == ALICE) {
    //   ot1->send_rot(b1, b2, 100) ;
    //   ot2->recv_rot(recv, choices, 100) ;
    // } else {
    //   ot1->recv_rot(recv, choices, 100) ;
    //   ot2->send_rot(b1, b2, 100) ;
    // }
    // long long t_dummy = time_from(start_dummy) ;
    // int64_t dummy_comms = ot1->io->counter - comm_var ; comm_var = ot1->io->counter ;

    // // Print things
    // cout << "Dummy comms : " << dummy_comms << " bytes\n" ;
    // cout << "Dummy time : " << double(t_dummy)/1e3 << " ms\n" ;

    /************************* Real stuff *************************/

    // Measure and print
    auto start_exp = clock_start(); 
    int reps = 1 ;
    for (int i = 0 ; i < reps ; i++) {
      if (prot_type == "ohe")
        ohe = random_ohe(party, n, ot1, ot2, true) ;
      else
        ohe = random_gmt(party, n, ot1, ot2, true) ;
    }
    long long t_exp = time_from(start_exp);    
    setprecision(5) ;
    cout << fixed << setprecision(5) << "Time taken : " << double(t_exp)/(1e3*reps) << " ms\n" ;

    // Delete
    delete[] ohe ; 
  }

  return 0 ;
}