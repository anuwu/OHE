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
      << " <party-id> <port> <start> <end> \n";
    exit(EXIT_FAILURE);
   };

  // Declare variables
  int party, port ;
  int n ;
  int start, end ;

  // Check command line arguments
  if (argc != 5)
    abort() ;
  
  // Parse arguments
  party = atoi(argv[1]) ;
  port = atoi(argv[2]) ;
  start = atoi(argv[3]) ;
  end = atoi(argv[4]) ;

  /************************* Experiment *************************/

  for (int n = start ; n <= end ; n++) {
    uint64_t num_ots ;
    long long time ;
    uint64_t comm ;

    num_ots = 1 << n ;
    get_cost(party, port, "ferret", num_ots, time, comm) ;
    cout << num_ots << " - \n" ;
    cout << fixed << setprecision(MEASUREMENT_PRECISION) << "Time  : " << time/1e3 << " ms\n" ;
    cout << "Comms : " << comm << " bytes\n\n" ;
  }
}