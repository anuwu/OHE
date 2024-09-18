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

  LUT(int n, int m) {
    // Fill parameters
    this->n = n ;
    this->m = m ;
    this->m_blocks = (m+127)/128 ;
    this->lut_size = 1ULL << n ;

    // Allocate table
    this->table = new block*[lut_size] ;
    for (uint64_t i = 0 ; i < this->lut_size ; i++) {
      this->table[i] = new block[m_blocks] ;
      for (int j = 0 ; j < m_blocks ; j++)
        this->table[i][j] = zero_block ;
    }
  }

  friend ostream& operator<<(ostream &os, const LUT &tt) ;

  ~LUT() {
    for (uint64_t i = 0 ; i < this->lut_size ; i++)
      delete[] this->table[i] ;
    delete[] this->table ;
  }
} ;

// Generate identity truth table
LUT identity(int n) ; 

// Read truth table
LUT input_lut(int n, int m, string lut_path) ;

// Rotate H(a) by (x+a)
block* rotate(int n, block *hot, uint64_t rot) ;

// Multiply with truth table
block* eval_lut(int n, LUT &lut, block *hot) ;

// Send and receive shares of output => Need batching

#endif