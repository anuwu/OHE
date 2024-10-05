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
  if (batched) {
    batch_size = atoi(argv[7]) ;
    if (batch_size % 128 > 0) {
      cerr << "Batch size must be a multiple of 128\n" ;
      exit(EXIT_FAILURE) ;
    }
  }
  int num_blocks = get_ohe_blocks(n) ;
  
  /************************* Create OT *************************/

  io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port, true) ;
  uint64_t running_comm = io->counter ;
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

  /************************* Experiment *************************/

  if (batched) {
    // Declare and initialize
    block **alphas, **ohes ;
    alphas = new block*[batch_size] ;
    ohes = new block*[batch_size] ;
    for (int b = 0 ; b < batch_size ; b++) {
      alphas[b] = new block[1] ;
      ohes[b] = new block[num_blocks] ;
      initialize_blocks(alphas[b], 1) ;
      initialize_blocks(ohes[b], num_blocks) ;
    } 
      
    // Get OHEs
    auto total_start = clock_start() ;
    if (prot_type == "ohe")
      batched_random_ohe(party, n, batch_size, ot1, ot2, alphas, ohes, true) ;
    else if (prot_type == "gmt")
      batched_random_gmt(party, n, batch_size, ot1, ot2, alphas, ohes, true) ;
    else {
      cerr << "Incorrect OT type\n" ;
      exit(EXIT_FAILURE) ;
    }
    long long total_time = time_from(total_start) ;
    cout << fixed << setprecision(MEASUREMENT_PRECISION) << "Total time : " << total_time/1e3 << " ms\n" ;
    cout << "Total comms : " << (io->counter - running_comm) << " bytes\n" ;

    // Delete
    for (int b = 0 ; b < batch_size ; b++) {
      delete[] ohes[b] ;
      delete[] alphas[b] ;
    }    
    delete[] ohes ;
    delete[] alphas ;
  }
  else {
    block *ohe, *alpha ;
    alpha = new block[1] ;
    ohe = new block[num_blocks] ;
    initialize_blocks(alpha, 1) ;
    initialize_blocks(ohe, num_blocks) ;

    /************************* Fixed cost measurer *************************/

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

    // Get OHE
    auto total_start = clock_start() ;
    if (prot_type == "ohe")
      random_ohe(party, n, ot1, ot2, alpha, ohe, true) ;
    else if (prot_type == "gmt")
      random_gmt(party, n, ot1, ot2, alpha, ohe, true) ;
    else {
      cerr << "Incorrect protocol type\n" ;
      exit(EXIT_FAILURE) ;
    }
    long long total_time = time_from(total_start) ;
    cout << fixed << setprecision(MEASUREMENT_PRECISION) << "Total time : " << total_time/1e3 << " ms\n" ;
    cout << "Total comms : " << (io->counter - running_comm) << " bytes\n" ;

    // Delete
    delete[] ohe ; 
    delete[] alpha ;
  }

  // Delete OT stuff
  delete ot1 ;
  delete ot2 ;
  delete io ;
  return 0 ;
}