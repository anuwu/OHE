#include "emp-ot/emp-ot.h"
#include "test_util.h"
#include <iostream>
#include <time.h>

using namespace std ;
using namespace emp ;

constexpr int threads = 1 ;

int main(int argc, char** argv) {
  /************************* Parse Input *************************/
  
	const auto abort = [&] {
	cerr
			<< "usage: "
			<< argv[0]
			<< "\n";
	exit(EXIT_FAILURE);
	} ;

	cout << "Hello World\n" ;
  
  return 0 ;
}