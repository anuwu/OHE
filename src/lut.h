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
    this->n = n ;
    this->m = m ;
    this->m_blocks = (m+127)/128 ;
    this->lut_size = 1ULL << n ;

    this->table = new block*[lut_size] ;
    for (uint64_t i = 0 ; i < this->lut_size ; i++) {
      this->table[i] = new block[m_blocks] ;
      for (int j = 0 ; j < m_blocks ; j++)
        this->table[i][m] = zero_block ;
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
LUT read_lut_from_file(int n, int m, ifstream &handler) ;

// Retrieve a from H(a)
block* unhot(block *hot) ;

// Send and receive (x+a)

// Rotate H(a) by (x+a)
block* rotate(block *hot, int rot) ;

// Multiply with truth table
block* eval_lut(LUT table, block *hot) ;

// Send and receive shares of output

#endif