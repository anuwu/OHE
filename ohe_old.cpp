#include "ohe_old.h"
#include "utils.h"
#include <unordered_map>
#include <vector>
#include <iostream>

using namespace emp ;
using namespace std ;

template <typename T>
void Coordinator<T>::dummy_ots() {
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
}

template <typename T>
void Coordinator<T>::dummy_ots_threaded() {
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
}

template <typename T>
int64_t Coordinator<T>::test_gmt() {
  // Declare
	unordered_map<int, vector<vector<int>>> mp ;
	unordered_map<int, int> singleton ;
	unordered_map<int, int> remaining ;
	int n = this->loglength ;

  // Fill metadata
	mp = get_subsets(n) ;
	for (int i = 2 ; i < n+1 ; i++) {
    for (int j = 0 ; j < (int)mp[i].size() ; j++) {
      vector<int> vec = mp[i][j] ;
      int totrnk = total_rank(vec, n) ;
      singleton[totrnk] = vec[0] ; 
      vector<int> cop = vec ; cop.erase(cop.begin()) ; remaining[totrnk] = total_rank(cop, n) ;
    }
	}

  // Intermediate variables
	PRG prg ;
	block *hot = new block[this->length] ; 
	block *a, *b_blk, *andd, *loc, *loc_tmp, *fin, *fin_tmp ;
	unordered_map<int, block*> r, rcv, a_masked ;
	bool *b ;
	andd = new block[1]; 
	loc = new block[1] ; loc_tmp = new block[1] ;
	fin = new block[1] ; fin_tmp = new block[1] ;
	b = new bool[128] ;
  prg.random_block(hot+1, n) ;


	for (auto it = this->robin.begin() ; it != this->robin.end() ; it++) {
    int p1 = it->first, p2 = it->second ;
    r[p1 == party ? p2 : p1] = new block[1] ;
    rcv[p1 == party ? p2 : p1] = new block[1] ;
    a_masked[p1 == party ? p2 : p1] = new block[1] ;
	}

	for (int i = n+1 ; i < this->length ; i++) {
    a = hot + singleton[i] ;
    b_blk = hot + remaining[i] ;
    *b = get_bools_from_block(b_blk) ;

    // Perform OTs
    ThreadPool pool(this->parties - 1) ;
    for (auto it = this->robin.begin() ; it != this->robin.end() ; it++) {
      int p1 = it->first, p2 = it->second ;
      int wh = p1 == party ? p2 : p1 ;
      prg.random_block(r[wh], 1) ;
      xorBlocks_arr(a_masked[wh], a, r[wh], 1) ;
      if (p1 == party) {
        this->ots[wh]->send(r[wh], a_masked[wh], 1) ; this->ots[wh]->io->flush() ;
        this->ots[wh]->recv(rcv[wh], b, 128) ;
      } else {
        this->ots[wh]->recv(rcv[wh], b, 128) ; this->ots[wh]->io->flush() ;
        this->ots[wh]->send(r[wh], a_masked[wh], 1) ; 
      }
      this->ots[wh]->io->flush() ;
    }

    // Computing final share
    andBlocks_arr(andd, a, b_blk, 1) ;
    copyBlocks_arr(loc_tmp, andd, 1) ;
    for (int p = 1 ; p <= this->parties ; p++) {
      if (p == this->party)
        continue ;

      xorBlocks_arr(loc, loc_tmp, rcv[p], 1) ;
      copyBlocks_arr(loc_tmp, loc, 1) ;
    }
    copyBlocks_arr(fin_tmp, loc, 1) ;
    for (int p = 1 ; p <= this->parties ; p++) {
      if (p == this->party)
        continue ;

      xorBlocks_arr(fin, fin_tmp, r[p], 1) ;
      copyBlocks_arr(fin_tmp, fin, 1) ;
    }
    copyBlocks_arr(hot+i, fin, 1) ;
	}

  // Add up communication and return
	int64_t total_comms = 0 ;
	for (int p = 1 ; p <= this->parties ; p++) {
    if (p == this->party)
      continue ;
    total_comms += this->ots[p]->io->counter ;
	}
	return total_comms ;
}
template int64_t Coordinator<IKNP<NetIO>>::test_gmt() ;
template int64_t Coordinator<OTCO<NetIO>>::test_gmt() ;
template int64_t Coordinator<OTNP<NetIO>>::test_gmt() ;
template int64_t Coordinator<FerretCOT<NetIO>>::test_gmt() ;


template <typename T>
int64_t Coordinator<T>::test_gmt_threaded() {
  // Declare stuff
	unordered_map<int, vector<vector<int>>> mp ;
	unordered_map<int, int> singleton ;
	unordered_map<int, int> remaining ;
	int n = this->loglength ;

  // Fill metadata
	mp = get_subsets(n) ;
	for (int i = 2 ; i < n+1 ; i++) {
    for (int j = 0 ; j < (int)mp[i].size() ; j++) {
      vector<int> vec = mp[i][j] ;
      int totrnk = total_rank(vec, n) ;
      singleton[totrnk] = vec[0] ; 
      vector<int> cop = vec ; cop.erase(cop.begin()) ; remaining[totrnk] = total_rank(cop, n) ;
    }
	}

  // Intermediate variables
	PRG prg ;
	block *hot = new block[this->length] ; 
	block *a, *b_blk, *andd, *loc, *loc_tmp, *fin, *fin_tmp ;
	unordered_map<int, block*> r, rcv, a_masked ;
	bool *b ;
	andd = new block[1]; 
	loc = new block[1] ; loc_tmp = new block[1] ;
	fin = new block[1] ; fin_tmp = new block[1] ;
	b = new bool[128] ;
  prg.random_block(hot+1, n) ;

	for (int p = 1 ; p <= this->parties ; p++) {
    if (p == this->party)
      continue ;

    r[p] = new block[1] ;
    rcv[p] = new block[1] ;
    a_masked[p] = new block[1] ;
	}

	for (int i = n+1 ; i < this->length ; i++) {
    a = hot + singleton[i] ;
    b_blk = hot + remaining[i] ;
    *b = get_bools_from_block(b_blk) ;

    // Perform OTs
    ThreadPool pool(this->parties - 1) ;
    for (int p = 1 ; p <= this->parties ; p++) {
      if (p == this->party)
        continue ;

      prg.random_block(r[p], 1) ;
      xorBlocks_arr(a_masked[p], a, r[p], 1) ;
      pool.enqueue([p, this, &r, &a_masked, b, &rcv] {
        this->ots[p]->io->sync() ;
        if (this->party < p) {
          this->ots[p]->send(r[p], a_masked[p], 1) ; this->ots[p]->io->flush() ;
          this->ots[p]->recv(rcv[p], b, 128) ;
        } else {
          this->ots[p]->recv(rcv[p], b, 128) ; this->ots[p]->io->flush() ;
          this->ots[p]->send(r[p], a_masked[p], 1) ;  
        }
        this->ots[p]->io->flush() ;
      }) ;
    }

    // Computing final share
    andBlocks_arr(andd, a, b_blk, 1) ;
    copyBlocks_arr(loc_tmp, andd, 1) ;
    for (int p = 1 ; p <= this->parties ; p++) {
      if (p == this->party)
        continue ;

      xorBlocks_arr(loc, loc_tmp, rcv[p], 1) ;
      copyBlocks_arr(loc_tmp, loc, 1) ;
    }
    copyBlocks_arr(fin_tmp, loc, 1) ;
    for (int p = 1 ; p <= this->parties ; p++) {
      if (p == this->party)
        continue ;

      xorBlocks_arr(fin, fin_tmp, r[p], 1) ;
      copyBlocks_arr(fin_tmp, fin, 1) ;
    }
    copyBlocks_arr(hot+i, fin, 1) ;
	}

  // Add up communication and return
	int64_t total_comms = 0 ;
	for (int p = 1 ; p <= this->parties ; p++) {
    if (p == this->party)
      continue ;
    total_comms += this->ots[p]->io->counter ;
	}
	return total_comms ;
}
template int64_t Coordinator<IKNP<NetIO>>::test_gmt_threaded() ;
template int64_t Coordinator<OTCO<NetIO>>::test_gmt_threaded() ;
template int64_t Coordinator<OTNP<NetIO>>::test_gmt_threaded() ;
template int64_t Coordinator<FerretCOT<NetIO>>::test_gmt_threaded() ;