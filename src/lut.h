#ifndef EMP_OHE_LUT_H
#define EMP_OHE_LUT_H

#include "emp-ot/emp-ot.h"
#include "utils.h"

using namespace std ;
using namespace emp ;

#define MAX_LUT_SIZE 20

class LUT {
public :
  int n ;
  uint64_t lut_size ;
  int m, m_blocks ;
  block **table ;

  // Empty LUT
  LUT() { 
    table = NULL ;
  }

  // LUT with empty table
  LUT(int n, int m) {
    // Fill parameters
    this->n = n ;
    this->m = m ;
    this->m_blocks = (m+127)/128 ;
    this->lut_size = 1ULL << n ;

    // Allocate table
    this->table = new block*[lut_size] ;
    for (uint64_t i = 0 ; i < this->lut_size ; i++) {
      this->table[i] = new block[this->m_blocks] ;
      for (int j = 0 ; j < this->m_blocks ; j++)
        this->table[i][j] = zero_block ;
    }
  }

  // Copy constructor
  LUT(const LUT &other) {
    this->n = other.n ;
    this->lut_size = other.lut_size ;
    this->m = other.m ;
    this->m_blocks = other.m_blocks ;
    this->table = new block*[lut_size] ;
    for (uint64_t i = 0 ; i < this->lut_size ; i++) {
      this->table[i] = new block[this->m_blocks] ;
      copyBlocks_arr(this->table[i], other.table[i], this->m_blocks) ;
    }
  }

  // Assignment operator
  LUT& operator=(const LUT &other) {
    this->n = other.n ;
    this->lut_size = other.lut_size ;
    this->m = other.m ;
    this->m_blocks = other.m_blocks ;
    this->table = new block*[lut_size] ;
    for (uint64_t i = 0 ; i < this->lut_size ; i++) {
      this->table[i] = new block[this->m_blocks] ;
      copyBlocks_arr(this->table[i], other.table[i], this->m_blocks) ;
    }
    return *this ;
  }

  // Printing
  friend ostream& operator<<(ostream &os, const LUT &tt) ;

  // Destructor
  ~LUT() {
    for (uint64_t i = 0 ; i < this->lut_size ; i++)
      delete[] this->table[i] ;
    delete[] this->table ;
  }

  // Multiply with truth table
  void eval_lut(int n, block *ohe, block *output) ;

  // Multiply with truth table with in-place rotation
  void eval_lut_with_rot(int n, block *ohe, uint64_t rot, block *output) ;

  // Secure eval
  void secure_eval(int party, int n, COT<NetIO> *ot1, COT<NetIO> *ot2, block *inp, block *ohe, block *alpha, block *output, bool measure=false) ;

  // Batched secure eval
  void batched_secure_eval(int party, int n, int batch_size, COT<NetIO> *ot1, COT<NetIO> *ot2, block **inps, block **ohes, block **alphas, block **outputs, bool measure=false) ;
} ;

// Generate identity truth table
LUT identity(int n) ; 

// Read truth table
LUT input_lut(int n, int m, string lut_path) ;

#endif