#include "lut.h"

using namespace std ;
using namespace emp ;

TT identity (int n) {
  // Error
  if (n > 63)
    cerr << "Unable to generate identity truth table for n > 63" ;

  // Create TT object
  TT tt = TT (n, n) ;
  
  // Create truth table
  uint64_t index = 0ULL ;
  do {
    tt.table[index][0] = makeBlock(0ULL, index) ;
    index++ ;
  } while (index != 0) ;

  return tt ;
}

TT read_tt() {
  // Read from file
  return identity(1) ;
}
