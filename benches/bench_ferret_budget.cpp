#include "utils.h"
#include "ohe.h"
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
      << " <party-id> <port> <n> <ohe/gmt> <batch_size> <budget_mb> \n" ;
    exit(EXIT_FAILURE);
   };

  // Declare variables
  int party, port, n, num_blocks ;
  int batch_size ;
  int budget_mb ;
  string prot_type ;
  uint64_t budget_bytes ;

  // Check command line arguments
  if (argc != 7)
    abort() ;
  
  // Parse arguments
  party = atoi(argv[1]) ;
  port = atoi(argv[2]) ;
  n = atoi(argv[3]) ;
  prot_type = argv[4] ;
  batch_size = atoi(argv[5]) ;
  budget_mb = atoi(argv[6]) ;
  budget_bytes = budget_mb*1024*1024 ;
  num_blocks = get_ohe_blocks(n) ;

  // Declare OT stuff
  NetIO *io ;
  COT<NetIO> *ot1, *ot2 ;
  io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port, true) ;
  ot1 = new FerretCOT<NetIO>(party, 1, &io) ;
  ot2 = new FerretCOT<NetIO>(party == 1 ? 2 : 1, 1, &io) ;

  /************************* Experiment *************************/

  // Initialize stuff
  block **alphas, **ohes ;
  alphas = new block*[batch_size] ;
  ohes = new block*[batch_size] ;
  for (int b = 0 ; b < batch_size ; b++) {
    alphas[b] = new block[1] ;
    ohes[b] = new block[num_blocks] ;
    initialize_blocks(alphas[b], 1) ;
    initialize_blocks(ohes[b], num_blocks) ;
  } 

  // Perform loop
  int count = 0 ;
  do {
    // Get OHEs
    if (prot_type == "ohe")
      batched_random_ohe(party, n, batch_size, ot1, ot2, alphas, ohes, false) ;
    else if (prot_type == "gmt")
      batched_random_gmt(party, n, batch_size, ot1, ot2, alphas, ohes, false) ;
    else {
      cerr << "Incorrect OT type\n" ;
      exit(EXIT_FAILURE) ;
    }

    // Increase count and zero-out variables
    count++ ;
    for (int b = 0 ; b < batch_size ; b++) {
      initialize_blocks(alphas[b], 1) ;
      initialize_blocks(ohes[b], num_blocks) ;
    }
  } while (io->counter <= budget_bytes) ;

  // Delete OHE variables
  for (int b = 0 ; b < batch_size ; b++) {
    delete[] ohes[b] ;
    delete[] alphas[b] ;
  }    
  delete[] ohes ;
  delete[] alphas ;

  /*
  Calculation for the count proportional to the actual budget. This is needed when the true cost overshoots the budget.
    counter     count
    budget      x

    counter*x = count*budget
    x = count*budget/counter
  */

  cout << "Budget bytes : " << budget_bytes << "\n" ;
  cout << "Counter : " << io->counter << "\n" ;
  cout << "Batch count : " << count << "\n" ;
  cout << "OT count : " << floor(((double)count*budget_bytes)/io->counter*batch_size) << "\n" ;

  // Delete OT stuff
  delete io ;
  delete ot1 ;
  delete ot2 ;
}