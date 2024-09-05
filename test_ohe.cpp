#include "ohe.h"
#include "utils.h"
#include <iostream>
#include <time.h>

using namespace std ;
using namespace emp ;

int main(int argc, char** argv) {
  /************************* Parse Input *************************/
  
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " <party-id> <port> <n> <iknp/ferret> <prot_type> <batched> <verify> \n";
    exit(EXIT_FAILURE);
   };

  int64_t comm_var = 0 ;
  int party, port, n ;
  string ot_type, prot_type ;
  bool verify, batched ;
  NetIO *io ;
  COT<NetIO> *ot1, *ot2 ;

  if (argc != 8)
    abort();
  
  party = atoi(argv[1]) ;
  port = atoi(argv[2]) ;
  n = atoi(argv[3]) ;
  ot_type = argv[4] ;
  prot_type = argv[5] ;
  batched = atoi(argv[6]) ;
  verify = atoi(argv[7]) ;
  
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
    int batch_size = 128 ;
    // Declare and initialize
    int num_blocks = n_to_blocks(n) ;
    block **ohes ;
    block *rec ;
    
    // Get OHEs
    auto start_exp = clock_start(); 
    if (prot_type == "ohe")
      ohes = batched_random_ohe(party, n, batch_size, ot1, ot2, !verify) ;
    else
      ohes = batched_random_gmt(party, n, batch_size, ot1, ot2, !verify) ;
    long long t_exp = time_from(start_exp);  
    comm_var = io->counter - comm_var ;

    // Verify per batch
    if (verify) {
      int correct = 0 ;    
      for (int b = 0 ; b < batch_size ; b++) {
        rec = reconst_ohe(party, n, ot1, ot2, ohes[b], false) ;
        int no_set = 0 ;
        for (int i = 0 ; i < num_blocks ; i++) {
          uint64_t *dat = (uint64_t*)(rec+i) ;
          for (int pos = 0 ; pos < 64 ; pos++) {
            if (dat[0] & (1L << pos))
              no_set++ ;
            if (dat[1] & (1L << pos))
              no_set++ ;
          }
        }
        if (no_set == 1)
          correct++ ;

        delete[] rec ;
      }

      // Print things
      if (correct == batch_size)
        cout << "\033[1;32m" << "Passed" << "\033[0m\n" ;
      else 
        cout << "\033[1;31m" << "Failed " << " --> " << correct << "\033[0m\n" ;
    }

    // Delete
    for (int b = 0 ; b < batch_size ; b++)
      delete[] ohes[b] ;
    delete[] ohes ;

    // Print things
    if (!verify) {
      setprecision(5) ;  
      cout << fixed << setprecision(5) << "Time taken : " << double(t_exp)/(1e3*batch_size) << " ms\n" ;
    }
  }
  else {
    block *ohe ;
    if (verify) {
      block *rec ;
      int correct = 0 ;
      int num_blocks = n_to_blocks(n) ;
      int no_tests = min(100, 10 * (1 << n)) ;
      for (int counter = 0 ; counter < no_tests ; counter++) {
        if (prot_type == "ohe")
          ohe = random_ohe(party, n, ot1, ot2) ;
        else
          ohe = random_gmt(party, n, ot1, ot2) ;

        rec = reconst_ohe(party, n, ot1, ot2, ohe, false) ;
        int no_set = 0 ;
        for (int i = 0 ; i < num_blocks ; i++) {
          uint64_t *dat = (uint64_t*)(rec+i) ;
          for (int pos = 0 ; pos < 64 ; pos++) {
            if (dat[0] & (1L << pos))
              no_set++ ;
            if (dat[1] & (1L << pos))
              no_set++ ;
          }
        }
        if (no_set == 1)
          correct++ ;

        delete[] ohe ; delete[] rec ;
      }

      // Print things
      if (correct == no_tests)
        cout << "\033[1;32m" << "Passed" << "\033[0m\n" ;
      else {
        cout << "\033[1;31m" << "Failed --> " << correct << "\033[0m\n" ;
        return 1 ;
      }
    } else {
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
  }

  return 0 ;
}