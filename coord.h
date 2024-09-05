#ifndef EMP_OHE_OHE_OLD_H
#define EMP_OHE_OHE_OLD_H

#include "emp-ot/emp-ot.h"
#include <iostream>
#include <unordered_map>

using namespace std ;

template <typename T>
class Coordinator {
public :
  unordered_map<int, T*> ots ;
  vector<pair<int,int>> robin ;
  int party, parties, length, loglength ;

  Coordinator(int party, int parties, int length, int loglength, vector<pair<int,int>> &robin, unordered_map<int, T*> &ots) {
  this->party = party ;
	this->parties = parties ;
	this->length = length ;
	this->loglength = loglength ;
	this->robin = robin ;
	this->ots = ots ;
  }

  int dummy_ots() ;

  int dummy_ots_threaded() ;
} ;

#endif