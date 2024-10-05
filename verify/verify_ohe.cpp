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
      << " <party> <port> <n> <iknp/ferret> <ohe/gmt> <batched> <opt:batch_size> \n";
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
  else if (argc == 7) {
    if (atoi(argv[6]))
      abort() ;
  }
  else if (argc == 8) {
    if (!atoi(argv[6]))
      abort() ;
  }
  else
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
    cerr << "Incorrect OT type\n" ;
    exit(EXIT_FAILURE) ;
  }

  /************************* Experiment *************************/

  int num_blocks = get_ohe_blocks(n) ;

  if (batched) {
    // Declare and initialize
    uint64_t ohe_size = 1ULL << n ;
    block **ohes, **alphas ;
    block *rec ;
    rec = new block[num_blocks] ;
    alphas = new block*[batch_size] ;
    ohes = new block*[batch_size] ;
    for (int b = 0 ; b < batch_size ; b++) {
      ohes[b] = new block[num_blocks] ;
      alphas[b] = new block[1] ;
      initialize_blocks(ohes[b], num_blocks) ;
      initialize_blocks(alphas[b], 1) ;
    }
      
    // Get OHEs
    if (prot_type == "ohe")
      batched_random_ohe(party, n, batch_size, ot1, ot2, alphas, ohes) ;
    else if (prot_type == "gmt")
      batched_random_gmt(party, n, batch_size, ot1, ot2, alphas, ohes) ;
    else {
      cerr << "Incorrect protocol type\n" ;
      exit(EXIT_FAILURE) ;
    }

    // Verify OHEs in the batch
    int correct = 0 ;    
    for (int b = 0 ; b < batch_size ; b++) {
      // rec = reconst_ohe(party, n, ot1, ot2, ohes[b], false) ;
      initialize_blocks(rec, num_blocks) ;
      reconst(party, ot1, ot2, ohe_size, ohes[b], rec) ;
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
    }

    // Print things
    if (correct == batch_size)
      cout << "\033[1;32m" << "Passed" << "\033[0m\n" ;
    else 
      cout << "\033[1;31m" << "Failed " << " --> " << correct << "\033[0m\n" ;

    // Delete
    for (int b = 0 ; b < batch_size ; b++) {
      delete[] ohes[b] ;
      delete[] alphas[b] ;
    }
    delete[] ohes ;
    delete[] rec ;
    delete[] alphas ;
  }
  else {
    // Declare and initialize
    block *alpha, *ohe, *rec ;
    int correct = 0 ;
    alpha = new block[1] ;
    ohe = new block[num_blocks] ;
    rec = new block[num_blocks] ;

    // Iterate over all test cases
    uint64_t ohe_size = 1ULL << n ;
    int no_tests = min((uint64_t)100, 10*ohe_size) ;
    for (int counter = 0 ; counter < no_tests ; counter++) {
      initialize_blocks(alpha, 1) ;
      initialize_blocks(ohe, num_blocks) ;
      initialize_blocks(rec, num_blocks) ;

      if (prot_type == "ohe")
        random_ohe(party, n, ot1, ot2, alpha, ohe) ;
      else if (prot_type == "gmt")
        random_gmt(party, n, ot1, ot2, alpha, ohe) ;
      else {
        cerr << "Incorrect OT type\n" ;
        exit(EXIT_FAILURE) ;
      }

      reconst(party, ot1, ot2, ohe_size, ohe, rec) ;
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
    }

    // Delete stuff
    delete[] alpha ;
    delete[] ohe ; 
    delete[] rec ;
    
    // Print things
    if (correct == no_tests)
      cout << "\033[1;32m" << "Passed" << "\033[0m\n" ;
    else {
      cout << "\033[1;31m" << "Failed --> " << correct << "\033[0m\n" ;
      return 1 ;
    }
  }

  // Delete OT stuff
  delete ot1 ;
  delete ot2 ;
  delete io ;
  return 0 ;
}