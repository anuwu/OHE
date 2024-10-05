#ifndef EMP_OHE_OHE_H
#define EMP_OHE_OHE_H

#include "emp-ot/emp-ot.h"

using namespace std ;
using namespace emp ;

void get_ohe_from_plain(block *inp, block *ohe) ;

void random_ohe(int party, int n, COT<NetIO> *ot1, COT<NetIO> *ot2, block *alpha, block *ohe, bool measure=false) ;

void random_gmt(int party, int n, COT<NetIO> *ot1, COT<NetIO> *ot2, block *alpha, block *ohe, bool measure=false) ;

void batched_random_ohe(int party, int n, int batch_size, COT<NetIO> *ot1, COT<NetIO> *ot2, block **alphas, block **ohes, bool measure=false) ;

void batched_random_gmt(int party, int n, int batch_size, COT<NetIO> *ot1, COT<NetIO> *ot2, block **alphas, block **ohes, bool measure=false) ;

#endif