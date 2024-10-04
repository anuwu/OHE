#include "utils.h"
#include "ohe.h"
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

void eval_lut(int n, LUT &lut, block *ohe, block *output) {
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
  block *ohe_t = new block[m_blocks] ;
  initialize_blocks(tmp, m_blocks) ;
  for (uint64_t i = 0 ; i < N ; i++) {
    // Replicate OHE at index i, m times
    block rep_block, rest_rep_block ;
    if (TEST_BIT(ohe, i)) {
      rep_block = all_one_block ;
      rest_rep_block = one_rest ;
    } else {
      rep_block = zero_block ;
      rest_rep_block = zero_block ;
    }
    initialize_blocks(ohe_t, m_blocks-1, rep_block) ;
    ohe_t[m_blocks-1] = rest_rep_block ;

    // XOR accumulate into res
    andBlocks_arr(tmp, ohe_t, lut.table[i], m_blocks) ;
    xorBlocks_arr(output, tmp, m_blocks) ;
  }

  // Delete and return
  delete[] tmp ;
  delete[] ohe_t ;
}

void eval_lut_with_rot(int n, LUT &lut, block *ohe, uint64_t rot, block *output) {
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
  block *ohe_t = new block[m_blocks] ;
  initialize_blocks(tmp, m_blocks) ;
  for (uint64_t i = 0 ; i < N ; i++) {
    // Replicate OHE at index i, m times
    block rep_block, rest_rep_block ;
    if (TEST_BIT(ohe, i^rot)) {
      rep_block = all_one_block ;
      rest_rep_block = one_rest ;
    } else {
      rep_block = zero_block ;
      rest_rep_block = zero_block ;
    }
    initialize_blocks(ohe_t, m_blocks-1, rep_block) ;
    ohe_t[m_blocks-1] = rest_rep_block ;

    // XOR accumulate into res
    andBlocks_arr(tmp, ohe_t, lut.table[i], m_blocks) ;
    xorBlocks_arr(output, tmp, m_blocks) ;
  }

  // Delete and return
  delete[] tmp ;
  delete[] ohe_t ;
}

void secure_eval(int party, int n, COT<NetIO> *ot1, COT<NetIO> *ot2, LUT &lut, block *inp, block *ohe, block *alpha, block *output) {
  // Error message
  if (n != lut.n) {
    cerr << "Incompatible size of OHE (" << n << ") and LUT input size (" << lut.n << ")\n" ;
    exit(EXIT_FAILURE) ;
  }

  // Initialize variables
  int m = lut.m ;
  int m_blocks = (m+127)/128 ;
 
  // Compute x+a
  block *masked_inp = new block[1] ;
  xorBlocks_arr(masked_inp, inp, alpha, 1) ;

  // Send and receive shares of (x+a)
  block *reconst_masked_inp = new block[1] ; initialize_blocks(reconst_masked_inp, 1) ;
  reconst(party, ot1, ot2, n, masked_inp, reconst_masked_inp) ;

  // f(T) * H(x) = f(t) with rotation
  block *otp_share = new block[m_blocks] ; initialize_blocks(otp_share, m_blocks) ;
  eval_lut_with_rot(n, lut, ohe, *((uint64_t*)reconst_masked_inp), otp_share) ;

  // Send and receive f(t)
  reconst(party, ot1, ot2, lut.m, otp_share, output) ;

  // Delete stuff
  // delete[] alpha ;
  delete[] masked_inp ;
  delete[] reconst_masked_inp ;
  delete[] otp_share ;
}

void batched_secure_eval(int party, int n, int batch_size, COT<NetIO> *ot1, COT<NetIO> *ot2, LUT &lut, block *inp, block **ohes, block **alphas, block **outputs) {
  // Error message
  if (n != lut.n) {
    cerr << "Incompatible size of OHE (" << n << ") and LUT input size (" << lut.n << ")\n" ;
    exit(EXIT_FAILURE) ;
  }

  // Compute x+a
  block *masked_inp = new block[batch_size] ; initialize_blocks(masked_inp, batch_size) ;
  for (int b = 0 ; b < batch_size ; b++)
    xorBlocks_arr(masked_inp+b, inp+b, alphas[b], 1) ;

  // Send and receive shares of (x+a)
  int msg_flat_blocks = (n*batch_size+127)/128 ;
  block *msg_flat = new block[msg_flat_blocks] ; initialize_blocks(msg_flat, msg_flat_blocks) ;
  block *reconst_msg_flat = new block[msg_flat_blocks] ; initialize_blocks(reconst_msg_flat, msg_flat_blocks) ;
  block *reconst_masked_inp = new block[batch_size] ; initialize_blocks(reconst_masked_inp, batch_size) ;
  for (int b = 0 ; b < batch_size ; b++)
    copyBits(msg_flat+(b*n)/128, (b*n)%128, masked_inp+b, 0, n) ;
  reconst(party, ot1, ot2, n*batch_size, msg_flat, reconst_msg_flat) ;
  for (int b = 0 ; b < batch_size ; b++)
    copyBits(reconst_masked_inp+b, 0, reconst_msg_flat+(b*n)/128, (b*n)%128, n) ;

  // f(T) * H(x) = f(t) with rotation
  block *otp_share = new block[batch_size] ; initialize_blocks(otp_share, batch_size) ;
  for (int b = 0 ; b < batch_size ; b++)
    eval_lut_with_rot(n, lut, ohes[b], *((uint64_t*)(reconst_masked_inp+b)), otp_share+b) ;
    
  // Send and receive f(t)
  for (int b = 0 ; b < batch_size ; b++)
    reconst(party, ot1, ot2, lut.m, otp_share+b, outputs[b]) ;

  // Delete stuff
  // delete[] alpha ;
  delete[] masked_inp ;
  delete[] msg_flat ;
  delete[] reconst_msg_flat ;
  delete[] reconst_masked_inp ;
  delete[] otp_share ;
}