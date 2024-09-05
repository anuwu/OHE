#include "coord.h"
#include "utils.h"
#include <unordered_map>
#include <vector>
#include <iostream>

using namespace emp ;
using namespace std ;

template <typename T>
int Coordinator<T>::dummy_ots() {
  // Declare
  PRG prg ;
	block *b0 = new block[this->length] ;
	block *b1 = new block[this->length] ;
	unordered_map<int, block*> rcvs ;
	bool *b = new bool[this->length] ;

	// Initialize
	prg.random_block(b0, this->length) ;
	prg.random_block(b1, this->length) ;
	prg.random_bool(b, this->length) ;
	for (int i = 1 ; i <= this->parties ; i++) {
		if (i == this->party)
			continue ;
		rcvs[i] = new block[this->length] ;
	}

  // Perform OTs
	for (auto it = robin.begin() ; it != robin.end() ; it++) {
		int p1 = it->first, p2 = it->second ;
		if (party == p1) {
			this->ots[p2]->send(b0, b1, this->length) ; this->ots[p2]->io->flush() ;
			this->ots[p2]->recv(rcvs[p2], b, this->length) ; 
		} else {
			this->ots[p1]->recv(rcvs[p1], b, this->length) ;
			this->ots[p1]->send(b0, b1, this->length) ; this->ots[p1]->io->flush() ;
		}
	}

  int total_comms = 0 ;
  for (int p = 1 ; p <= this->parties ; p++) {
    if (p == this->party)
      continue ;
    total_comms += this->ots[p]->io->counter ;
  }
  return total_comms ;
}
template int Coordinator<IKNP<NetIO>>::dummy_ots() ;
template int Coordinator<OTCO<NetIO>>::dummy_ots() ;
template int Coordinator<OTNP<NetIO>>::dummy_ots() ;
template int Coordinator<FerretCOT<NetIO>>::dummy_ots() ;


template <typename T>
int Coordinator<T>::dummy_ots_threaded() {
  // Declare
  PRG prg ;
	block *b0 = new block[this->length] ;
	block *b1 = new block[this->length] ;
	unordered_map<int, block*> rcvs ;
	bool *b = new bool[this->length] ;

  // Initialize
	prg.random_block(b0, this->length) ;
	prg.random_block(b1, this->length) ;
	prg.random_bool(b, this->length) ;
	for (int i = 1 ; i <= this->parties ; i++) {
		if (i == this->party)
			continue ;

		rcvs[i] = new block[this->length] ;
	}

  // Perform OTs
	ThreadPool pool(this->parties - 1) ;
	for (int i = 1 ; i <= this->parties ; i++) {
		if (i == this->party)
			continue ;

		block *rcv = rcvs[i] ;
		pool.enqueue([i, b0, b1, b, this, rcv] {
			if (this->party < i) {
				this->ots[i]->send(b0, b1, this->length) ; this->ots[i]->io->flush() ;
				this->ots[i]->recv(rcv, b, this->length) ; 
			} else {
				this->ots[i]->recv(rcv, b, this->length) ; this->ots[i]->io->flush() ;
				this->ots[i]->send(b0, b1, this->length) ;
			}
			this->ots[i]->io->flush() ;
		}) ;
	}

  int total_comms = 0 ;
	for (int p = 1 ; p <= this->parties ; p++) {
    if (p == this->party)
      continue ;
    total_comms += this->ots[p]->io->counter ;
	}
	return total_comms ;
}
template int Coordinator<IKNP<NetIO>>::dummy_ots_threaded() ;
template int Coordinator<OTCO<NetIO>>::dummy_ots_threaded() ;
template int Coordinator<OTNP<NetIO>>::dummy_ots_threaded() ;
template int Coordinator<FerretCOT<NetIO>>::dummy_ots_threaded() ;