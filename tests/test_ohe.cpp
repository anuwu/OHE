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
  int party, port, n ;
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
  
  /************************* Create OT *************************/

  io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port, true) ;
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
    int batch_size = 128 ;
    int num_blocks = n_to_blocks(n) ;
    block **ohes ;
    block *rec ;
    
    // Get OHEs
    if (prot_type == "ohe")
      ohes = batched_random_ohe(party, n, batch_size, ot1, ot2) ;
    else
      ohes = batched_random_gmt(party, n, batch_size, ot1, ot2) ;

    // Verify OHEs in the batch
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

    // Delete
    for (int b = 0 ; b < batch_size ; b++)
      delete[] ohes[b] ;
    delete[] ohes ;
  }
  else {
    // Declare and initialize
    block *ohe, *rec ;
    int correct = 0 ;
    int num_blocks = n_to_blocks(n) ;

    // Iterate over all test cases
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
  }
  return 0 ;
}