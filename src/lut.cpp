#include "lut.h"

using namespace std ;
using namespace emp ;

ostream& operator<<(ostream &os, const LUT &lut) {
    os << "Truth table from " << lut.n << " -> " << lut.m << "\n" ;
    for (uint64_t index = 0 ; index < lut.lut_size ; index++) {
      // os << hex << index << " | " ;
      printf("0x%08llx | ", index) ;
      for (int i = lut.m_blocks-1 ; i >= 0 ; i--)
        os << lut.table[index][i] ;
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

LUT read_lut_from_file(int n, int m, ifstream &handler) {
  // Read from file
  return identity(1) ;
}
