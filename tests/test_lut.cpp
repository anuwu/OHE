#include "lut.h"
#include <time.h>
#include <iostream>

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
      << " 2 <n> <m> <lut_name> \n";
    exit(EXIT_FAILURE);
  } ;
  if (argc != 5)
    abort() ;

  // Read arguments
  int n = atoi(argv[2]) ;
  int m = atoi(argv[3]) ;
  string lut_name = argv[4] ;

  // Display contents of the file
  LUT lut = input_lut(n, m, "../../luts/" + lut_name + ".lut") ;

  // Diaplying the LUT
  cout << "The LUT after reading -\n" ;
  cout << lut << "\n" ;
}

// Test rotate
void test3(int argc, char **argv) {
  // Abort message
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " 3 <n>\n";
    exit(EXIT_FAILURE);
  } ;
  if (argc != 3)
    abort() ;

  // Read argument
  int n = atoi(argv[2]) ;

  // Initialize variables
  uint64_t N = 1ULL << n ;
  random_device rd ;
  mt19937 rng(rd()) ;
  uniform_int_distribution<int> uid(0, N-1) ;
  int hot_index = uid(rng) ;
  uint64_t rotation = uid(rng) ;
  int num_blocks = get_ohe_blocks(n) ;
  block *hot = new block[num_blocks] ;
  initialize_blocks(hot, num_blocks) ;
  SET_BIT(hot, hot_index) ;

  // Obtain rotated vector
  block *hot_rotated = rotate(n, hot, rotation) ;

  // Verify
  cout << "hot_index = " << hot_index << "\n" ;
  cout << "rotation = " << rotation << "\n" ;
  cout << "rotated_index = " << (hot_index^rotation) << "\n" ;
  for (uint64_t i = 0 ; i < N ; i++) {
    bool rot_bool = TEST_BIT(hot_rotated, i) ;
    bool plain_bool = TEST_BIT(hot, i^rotation) ;
    if (rot_bool != plain_bool) {
      cout << "\033[1;31m" << "Failed " << "\033[0m\n" ;
      exit(EXIT_FAILURE) ;
    }
  
    if (rot_bool)
      cout << "Found rotated index at --> " << i << "\n" ;
  }

  delete[] hot ;
}

// Evaluate LUT
void test4(int argc, char **argv) {
  // Abort message
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " 4 <n>\n";
    exit(EXIT_FAILURE);
  } ;
  if (argc != 3)
    abort() ;

  // Read argument
  int n = atoi(argv[2]) ;

  // Initialize variables
  uint64_t N = 1ULL << n ;
  random_device rd ;
  mt19937 rng(rd()) ;
  uniform_int_distribution<int> uid(0, N-1) ;
  int hot_index = uid(rng) ;
  int num_blocks = get_ohe_blocks(n) ;
  block *hot = new block[num_blocks] ;
  initialize_blocks(hot, num_blocks) ;
  SET_BIT(hot, hot_index) ;

  LUT lut = identity(n) ;
  block *res = eval_lut(n, lut, hot) ;
  uint64_t recovered_index = 0 ;
  for (int i = 0 ; i < n ; i++)
    if (TEST_BIT(res, i))
      recovered_index += 1ULL << i ;

  cout << "hot_index = " << hot_index << "\n" ;
  cout << "recovered_index = " << recovered_index << "\n" ;

  delete[] hot ;
  delete[] res ;
}

// Single OHE evaluation
void test5(int argc, char **argv) {
  // f(identity) * H(a) = a
  // Compute x+a
  // Send and receive (x+a)
  // Rotate H(a) by (x+a) to get H(x)
  // f(T) * H(x) = f(t)
  // Send and receive f(t)

  // Abort message
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " 5 <n>\n";
    exit(EXIT_FAILURE);
  } ;
  if (argc != 3)
    abort() ;
}

// Batched OHE evaluation
void test6() {
  // f(identity) * H(a) = a
  // Compute x+a
  // Send and receive (x+a) => Need batching
  // Rotate H(a) by (x+a) to get H(x)
  // f(T) * H(x) = f(t)
  // Send and receive f(t) => Need batching
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
  
  // test rotation
  case 3 :
    test3(argc, argv) ;
    break ;

  // test evaluation
  case 4 :
    test4(argc, argv) ;
    break ;

  default :
    cerr << "Invalid test case\n" ;
    exit(EXIT_FAILURE) ;
  }

  return 0 ;
}
