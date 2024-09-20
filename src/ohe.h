#ifndef EMP_OHE_OHE_H
#define EMP_OHE_OHE_H

#include "emp-ot/emp-ot.h"

using namespace std ;
using namespace emp ;

block* random_ohe(int party, int n, COT<NetIO> *ot1, COT<NetIO> *ot2, bool print_comm=false) ;

block* random_gmt(int party, int n, COT<NetIO> *ot1, COT<NetIO> *ot2, bool print_comm=false) ;

block** batched_random_ohe(int party, int n, int batch_size, COT<NetIO> *ot1, COT<NetIO> *ot2, bool print_comm=false) ;

block** batched_random_gmt(int party, int n, int batch_size, COT<NetIO> *ot1, COT<NetIO> *ot2, bool print_comm=false) ;

#endif