#include "utils.h"
#include "ohe.h"
#include <iostream>
#include <time.h>

using namespace std ;
using namespace emp ;

#define batch_size 128

int main(int argc, char** argv) {
  /************************* Parse Input *************************/
  
  // Abort message
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " <party-id> <port> <ip> <n> <ohe/gmt> <batches>\n" ;
    exit(EXIT_FAILURE);
   };

  // Declare variables
  int party, port, n, num_blocks ;
  char *ip ;
  int batches ;
  string prot_type ;

  // Check command line arguments
  if (argc != 7)
    abort() ;
  
  // Parse arguments
  party = atoi(argv[1]) ;
  port = atoi(argv[2]) ;
  ip = argv[3] ;
  n = atoi(argv[4]) ;
  prot_type = argv[5] ;
  batches = atoi(argv[6]) ;
  num_blocks = get_ohe_blocks(n) ;

  // Declare OT stuff
  NetIO *io ;
  COT<NetIO> *ot1, *ot2 ;
  io = new NetIO(party == ALICE ? nullptr : ip, port, true) ;
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
  int count = 1 ;
  auto total_start = clock_start() ;
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
  } while (count < batches) ;

  long long total_time = time_from(total_start) ;
  cout << fixed << setprecision(MEASUREMENT_PRECISION) << "Total time : " << total_time/1e3 << " ms\n" ;

  // Delete OHE variables
  for (int b = 0 ; b < batch_size ; b++) {
    delete[] ohes[b] ;
    delete[] alphas[b] ;
  }    
  delete[] ohes ;
  delete[] alphas ;

  // Delete OT stuff
  delete io ;
  delete ot1 ;
  delete ot2 ;
}