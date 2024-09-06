#ifndef EMP_OHE_LUT_H
#define EMP_OHE_LUT_H

#include "emp-ot/emp-ot.h"
#include "utils.h"

using namespace std ;
using namespace emp ;

class TT {
public :
  int n ;
  uint64_t tt_size ;
  int m, m_blocks ;
  block **table ;

  TT(int n, int m) {
    this->n = n ;
    this->m = m ;
    this->m_blocks = (m+127)/128 ;
    this->tt_size = 1ULL << n ;

    this->table = new block*[tt_size] ;
    for (uint64_t i = 0 ; i < this->tt_size ; i++) {
      this->table[i] = new block[m_blocks] ;
      for (int j = 0 ; j < m_blocks ; j++)
        this->table[i][m] = zero_block ;
    }
  }

  friend ostream& operator<<(ostream &os, const TT &tt) ;

  ~TT() {
    for (uint64_t i = 0 ; i < this->tt_size ; i++)
      delete[] this->table[i] ;
    delete[] this->table ;
  }
} ;

// Generate identity truth table
TT identity(int n) ; 

// Read truth table
TT read_tt() ;

// Retrieve a from H(a)
block* unhot(block *hot) ;

// Send and receive (x+a)

// Rotate H(a) by (x+a)
block* rotate(block *hot, int rot) ;

// Multiply with truth table
block* eval_tt(TT table, block *hot) ;

// Send and receive shares of output

#endif