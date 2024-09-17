#include "lut.h"
#include <iostream>
#include <fstream>

using namespace std ;
using namespace emp ;

// Test identity
void test1(int argc, char **argv) {
  // Abort message
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " 1 <n> \n";
    exit(EXIT_FAILURE);
  } ;
  if (argc != 3)
    abort() ;

  // Read arguments
  int n = atoi(argv[2]) ;

  // Create identity LUT
  LUT lut = identity(n) ;
  cout << lut ;
}

// Read LUT
void test2(int argc, char **argv) {
  // Abort message
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " 2 n m <lut_name> \n";
    exit(EXIT_FAILURE);
  } ;
  if (argc != 5)
    abort() ;

  // Read arguments
  int n = atoi(argv[2]) ;
  int m = atoi(argv[3]) ;
  string lut_name = argv[4] ;

  // Read file
  ifstream handler("../../luts/" + lut_name + ".lut") ;
  if (handler.fail()) {
    cerr << "Unable to open " << lut_name << ".lut\n" ;
    exit(EXIT_FAILURE) ;
  }

  // Display contents of the file
  cout << "The contents of the file are - \n" ;
  string contents ;
  int line_count = 0 ;
  uint64_t max_lines = 1ULL << n ;

  LUT lut(n, m) ;

  while (getline(handler, contents)) {
    // Excess inputs in LUT file
    if (line_count > max_lines) {
      cerr << "Found more than " << max_lines << " inputs in " << lut_name << ".lut\n" ;
      exit(EXIT_FAILURE) ;
    }

    // Incorrect output length
    if (contents.length() != m) {
      cerr << "LUT has length " << m << ", but found " << contents.length() << "\n" ;
      exit(EXIT_FAILURE) ;
    }

    // Enter contents into LUT object
    cout << contents << "\n" ;
    // for (int i = contents.length() ; i >= 0 ; i--) {
    //   lut.table[line_count][i/128] = set_bit(lut.table[line_count][i/128], i%128) ;
    // }
    line_count++ ;
  }

  // Insufficient inputs in LUT file
  if (line_count < max_lines) {
    cerr << "Insufficient inputs in " << lut_name << ".lut\n" ;
    exit(EXIT_FAILURE) ;
  }
  
  // Close the file
  handler.close() ;
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
  
  // test reading from file
  case 2 :
    test2(argc, argv) ;
    break ;

  default :
    cerr << "Invalid test case\n" ;
    exit(EXIT_FAILURE) ;
  }

  return 0 ;
}
