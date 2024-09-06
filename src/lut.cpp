#include "lut.h"

using namespace std ;
using namespace emp ;

ostream& operator<<(ostream &os, const TT &tt) {
    os << "Truth table from " << tt.n << " -> " << tt.m << "\n" ;
    for (uint64_t index = 0 ; index < tt.tt_size ; index++) {
      // os << hex << index << " | " ;
      printf("0x%08llx | ", index) ;
      for (int i = tt.m_blocks-1 ; i >= 0 ; i--)
        os << tt.table[index][i] ;
      os << "\n" ;
    }
    return os ;
  }

TT identity (int n) {
  // Error
  if (n > 20)
    cerr << "Unable to generate identity truth table for n > 20\n" ;

  // Create TT object
  TT tt = TT (n, n) ;
  
  // Create truth table
  uint64_t index = 0ULL ;
  while (index < tt.tt_size) {
    tt.table[index][0] = makeBlock(0ULL, index) ;
    index++ ;
  }

  return tt ;
}

TT read_tt() {
  // Read from file
  return identity(1) ;
}
