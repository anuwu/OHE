#include "lut.h"
#include <iostream>
#include <fstream>

using namespace std ;
using namespace emp ;

ostream& operator<<(ostream &os, const LUT &lut) {
    os << "Truth table from " << lut.n << " -> " << lut.m << "\n" ;
    for (uint64_t index = 0 ; index < lut.lut_size ; index++) {
      printf("0x%08llx | ", index) ;
      for (int i = lut.m_blocks-1 ; i >= 0 ; i--)
        os << lut.table[index][i] << " " ;
      os << "\n" ;
    }
    return os ;
  }

LUT identity (int n) {
  // Error
  if (n > MAX_LUT_SIZE)
    cerr << "Unable to generate identity truth table for n > 20\n" ;

  // Create LUT object
  LUT lut = LUT (n, n) ;
  
  // Create truth table
  uint64_t index = 0ULL ;
  while (index < lut.lut_size) {
    lut.table[index][0] = makeBlock(0ULL, index) ;
    index++ ;
  }

  return lut ;
}

LUT input_lut(int n, int m, string lut_path) {
  // Initialize file stream
  ifstream handler(lut_path) ;
  if (handler.fail()) {
    cerr << "Unable to open " << lut_path << "\n" ;
    exit(EXIT_FAILURE) ;
  }

  // Initialize variables
  string contents ;
  int line_count = 0 ;
  uint64_t max_lines = 1ULL << n ;
  LUT lut(n, m) ;

  // Loop over lines
  while (getline(handler, contents)) {
    // Excess inputs in LUT file
    if (line_count > max_lines) {
      cerr << "Found more than " << max_lines << " inputs in " << lut_path << "\n" ;
      exit(EXIT_FAILURE) ;
    }

    // Incorrect output length
    if (contents.length() != m) {
      cerr << "LUT has output length " << m << ", but found " << contents.length() << "\n" ;
      exit(EXIT_FAILURE) ;
    }

    // Enter contents into LUT object
    for (int i = contents.length() ; i >= 0 ; i--)
      if (contents[i-1] == '1')
        SET_BIT(lut.table[line_count], m-i) ;

    // Increment line count
    line_count++ ;
  }

  // Insufficient inputs in LUT file
  if (line_count < max_lines) {
    cerr << "Insufficient inputs in " << lut_path << "\n" ;
    exit(EXIT_FAILURE) ;
  }

  // Close handler and return
  handler.close() ;
  return lut ;
}

void eval_lut(int n, LUT &lut, block *vec, block *output) {
  if (n != lut.n) {
    cerr << "Incompatible size of OHE (" << n << ") and LUT input size (" << lut.n << ")\n" ;
    exit(EXIT_FAILURE) ;
  }

  // Initialize variables
  uint64_t N = 1ULL << n ;
  int m = lut.m ;
  int m_blocks = (m+127)/128 ;
  int m_rem = m % 128 ;
  block one_rest = zero_block ;
  for (int i = 0 ; i < m_rem ; i++)
    one_rest = set_bit(one_rest, i) ;

  // Do the product
  block *tmp = new block[m_blocks] ;
  block *vec_t = new block[m_blocks] ;
  initialize_blocks(tmp, m_blocks) ;
  for (uint64_t i = 0 ; i < N ; i++) {
    // Replicate OHE at index i, m times
    block rep_block, rest_rep_block ;
    if (TEST_BIT(vec, i)) {
      rep_block = all_one_block ;
      rest_rep_block = one_rest ;
    } else {
      rep_block = zero_block ;
      rest_rep_block = zero_block ;
    }
    initialize_blocks(vec_t, m_blocks-1, rep_block) ;
    vec_t[m_blocks-1] = rest_rep_block ;

    // XOR accumulate into res
    andBlocks_arr(tmp, vec_t, lut.table[i], m_blocks) ;
    xorBlocks_arr(output, tmp, m_blocks) ;
  }

  // Delete and return
  delete[] tmp ;
  delete[] vec_t ;
}

void eval_lut_with_rot(int n, LUT &lut, block *vec, uint64_t rot, block *output) {
  ;
}
