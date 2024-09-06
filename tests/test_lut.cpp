#include "lut.h"
#include <iostream>

using namespace std ;
using namespace emp ;

// Test identity
void test1(int argc, char **argv) {
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " 1 <n> \n";
    exit(EXIT_FAILURE);
  } ;

  if (argc != 3)
    abort() ;

  int n = atoi(argv[2]) ;
  TT tt = identity(n) ;
  cout << tt ;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    cerr << "To view help : " << argv[0] << " <test_no> \n" ;
    exit(EXIT_FAILURE) ;
  }

  int test_no = atoi(argv[1]) ;

  switch(test_no) {
  // test identity
  case 1 :
    test1(argc, argv) ;
    break ;

  default :
    cerr << "Invalid test case\n" ;
    exit(EXIT_FAILURE) ;
  }

  return 0 ;
}
