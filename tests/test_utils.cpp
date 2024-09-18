#include "emp-ot/emp-ot.h"
#include "utils.h"
#include <iostream>
#include <unordered_set>
#include <time.h>

using namespace std ;
using namespace emp ;

constexpr int threads = 1 ;

// GMT to OHE converter
void test1(int argc, char** argv) {
   // Abort message
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " 1 <n> \n";
    exit(EXIT_FAILURE);
  } ;
  if (argc != 3)
    abort();

  // Read arguments
  int n ;
  n = atoi(argv[2]) ;

  unordered_map<int, vector<vector<int>>> mp ; 		    // length of subset -> all subset of that length
  unordered_map<int, int> singleton_gmt ;             // total rank of subset -> first element of the subset
  unordered_map<int, int> remaining_gmt ;             // total rank of subset -> total rank of tail of subset

  vector<int> conv_left ;
  vector<int> conv_right1 ;
  vector<vector<int>> conv_right2 ;

  mp = get_subsets(n) ;
  cout << "OHE_INDEX(RANK) = OHE_INDEX(RANK) + SINGLETON\n" ;
  for (int i = 1 ; i < n+1 ; i++) {
    cout << "All subsets of length " << i << "\n" ;
    for (int j = 0 ; j < (int)mp[i].size() ; j++) {
      vector<int> vec = mp[i][j] ;
      for (auto it = vec.begin() ; it != vec.end() ; it++)
        cout << *it << ", " ;
      
      int totrnk = total_rank(vec, n) ;
      singleton_gmt[totrnk] = vec[0] ; 
      vector<int> cop = vec ; cop.erase(cop.begin()) ; 
  		int remrnk = total_rank(cop, n) ;
  		remaining_gmt[totrnk] = remrnk ;
  		int rem_ohe_index = 0, ohe_index ;
  		for (auto it1 = cop.begin() ; it1 != cop.end() ; it1++)
  			rem_ohe_index += 1 << *it1 ;
  		ohe_index = rem_ohe_index + (1 << vec[0]) ;
  		ohe_index = (1 << n) - ohe_index - 1 ;
  		rem_ohe_index = (1 << n) - rem_ohe_index - 1 ;
      vector<int> ranks = get_ohe_ranks(cop, n, vec[0]) ;

  		cout << " -->\t" << ohe_index << "(" << totrnk << ")" << " = " << rem_ohe_index << "(" << remrnk << ")" << " + " << vec[0] << " | " ;
      for (auto it = ranks.begin() ; it != ranks.end() ; it++) {
        cout << *it ;
        if (it + 1 != ranks.end())
          cout << " + " ;
      }
      cout << "\n" ;

      conv_left.push_back(ohe_index) ;
      conv_right1.push_back(rem_ohe_index) ;
      conv_right2.push_back(ranks) ;
  	}
    cout << "\n" ;
  }  

  cout << "\nFrom the converter -\n" ;
  for (int i = 0 ; i < (int)conv_left.size() ; i++) {
    cout << conv_left[i] << " = " << conv_right1[i] << " + (" ;
    for (int j = 0 ; j < (int)conv_right2[i].size() ; j++) {
      cout << conv_right2[i][j] ;
      if (j+1 != (int)conv_right2[i].size())
        cout << " + " ;
    }
    cout << ")\n" ;
  } 
}

// copyBits test
void test2(int argc, char** argv) {
  // Abort message
  const auto abort = [&] {
    cerr << "usage: " << argv[0] << " 2 <pos1> <pos2> <bits>\n" ;
    exit(EXIT_FAILURE);
  } ;
  if (argc != 5)
    abort();

  // Read arguments
  int pos1 = atoi(argv[2]) ;
  int pos2 = atoi(argv[3]) ;
  int bits = atoi(argv[4]) ;

  // Initialize blocks
  PRG prg ;
  block *blk1 = new block[1] ;
  block *blk2 = new block[1] ;
  prg.random_block(blk1, 1) ;
  prg.random_block(blk2, 1) ;

  // Print stuff
  cout << "Before Copy - \n" ;
  cout << *blk1 << "\n" ;
  cout << *blk2 << "\n" ;
  copyBits(blk1, pos1, blk2, pos2, bits) ;
  cout << "\nAfter Copy - \n" ;
  cout << *blk1 << "\n" ;
  cout << *blk2 << "\n" ;

  // Delete stuff
  delete[] blk1 ;
  delete[] blk2 ;
}

// set_bit test
void test3(int argc, char** argv) {
  // Abort message
  const auto abort = [&] {
    cerr << "usage: " << argv[0] << " 3 <pos>\n" ;
    exit(EXIT_FAILURE);
  } ;
  if (argc != 3)
    abort() ;

  // Read arguments
  int pos = atoi(argv[2]) ;

  // Declare blocks
  PRG prg ;
  block *blk = new block[1] ;
  prg.random_block(blk, 1) ;

  // Print stuff
  cout << "Before -->\t" << blk[0] << "\n" ;
  blk[0] = set_bit(blk[0], pos) ;
  cout << "After -->\t" << blk[0] << "\n" ;

  // Delete stuff
  delete[] blk ;
}

// testBit test
void test4(int argc, char** argv) {
  // Abort message
  const auto abort = [&] {
    cerr << "usage: " << argv[0] << " 4 <pos>\n" ;
    exit(EXIT_FAILURE);
  } ;
  if (argc != 3)
    abort() ;

  // Read arguments
  int pos = atoi(argv[2]) ;

  // Initialize blocks
  PRG prg ;
  block *blk = new block[1] ;
  prg.random_block(blk, 1) ;

  // Print stuff
  cout << "blk --> " << blk[0] << "\n" ;
  bool the_bool = test_bit(blk[0], pos) ;
  cout << "bool --> " << the_bool << "\n" ;
  cout << "blk --> " << blk[0] << "\n" ;

  // Delete stuff
  delete[] blk ;
}

// zero-wrap over test
void test5(int argc, char** argv) {
  // Abort message
  const auto abort = [&] {
    cerr << "usage: " << argv[0] << " 5\n" ;
    exit(EXIT_FAILURE);
  } ;
  if (argc != 2)
    abort() ;

  // Print stuff
  uint64_t index = 0ULL ;
  cout << "This is zero --> " << index << "\n" ;
  index = ~index ;
  cout << "This is all 1s --> " << index << "\n" ;
  index += 1 ;
  cout << "This is wrap-over --> " << index << "\n" ;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    cerr << "To view help : " << argv[0] << " <test_no> \n" ;
    exit(EXIT_FAILURE) ;
  }

  int test_no = atoi(argv[1]) ;

  switch(test_no) {
  // GMT to OHE converter
  case 1 :
    test1(argc, argv) ;
    break ;

  // copyBits test
  case 2 :
    test2(argc, argv) ;
    break ;

  // set_bit test
  case 3 :
    test3(argc, argv) ;
    break ;

  // testBit test
  case 4 :
    test4(argc, argv) ;
    break ;

  // zero wrap-over test
  case 5 :
    test5(argc, argv) ;
    break ;

  default :
    cerr << "Invalid test case\n" ;
    exit(EXIT_FAILURE) ;
  }

  return 0 ;
}
