#ifndef EMP_OHE_UTILS_H
#define EMP_OHE_UTILS_H

#include "emp-ot/emp-ot.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iostream>

#define SET_BIT(A, B) (A[(B)/128] = set_bit(A[(B)/128], (B)%128))
#define TEST_BIT(A, B) test_bit(A[(B)/128], (B)%128)
#define MEASUREMENT_PRECISION 3

using namespace emp ;
using namespace std ;

/************************* Block manipulation *************************/

/**** AND ****/
// 2 var, copy
inline void andBlocks_arr(block* res, block* x, block* y, int nblocks) {
	const block *dest = nblocks + x ;
	for (; x != dest;) {
		*(res++) = *(x++) & *(y++);
	}
}

// 1 var, accum
inline void andBlocks_arr(block* res, block* y, int nblocks) {
	const block *dest = nblocks + res ;
	for (; res != dest;) {
    *res = *res & *y ;
    res++ ; y++ ;
	}
}

// 2 var, copy
inline void andBlocks_arr(block* res, block* x, const block y, int nblocks) {
	const block *dest = nblocks + x ;
	for (; x != dest;) 
		*(res++) =  *(x++) & y;
}

// 1 var, accum
inline void andBlocks_arr(block* res, const block y, int nblocks) {
	const block *dest = nblocks + res ;
	for (; res != dest;) {
    *res = *res & y ;
    res++ ;
  }
}

/**** XOR ****/
// 2 var, copy
inline void xorBlocks_arr(block* res, block* x, block* y, int nblocks) {
	const block *dest = nblocks + x ;
	for (; x != dest;)
		*(res++) = *(x++) ^ *(y++);
}

// 1 var, accum
inline void xorBlocks_arr(block* res, block* y, int nblocks) {
	const block *dest = nblocks + res ;
	for (; res != dest;) {
    *res = *res ^ *y ;
    res++ ; y++ ;
	}
}

// 2 var, copy
inline void xorBlocks_arr(block* res, block* x, const block y, int nblocks) {
	const block *dest = nblocks + x ;
	for (; x != dest;) 
		*(res++) =  *(x++) ^ y;
}

// 1 var, accum
inline void xorBlocks_arr(block* res, const block y, int nblocks) {
	const block *dest = nblocks + res ;
	for (; res != dest;) {
    *res = *res ^ y ;
    res++ ;
  }
}

/**** Copy ****/
inline void copyBlocks_arr(block* to, block* from, int nblocks) {
	const block *dest = nblocks + from ;
	for (; from != dest;) {
		*(to++) = *(from++) ;
	}
}

inline void copyBlocks_arr(block *to, const block from, int nblocks) {
	const block *dest = nblocks + to ; 
	for (; to != dest;) 
		*(to++) = from ;
}

inline void copyBools_arr(bool* to, bool* from, int nblocks) {
	const bool *dest = nblocks + from ;
	for (; from != dest;) {
		*(to++) = *(from++) ;
	}
}

inline void copyBools_arr(bool *to, const bool from, int nblocks) {
	const bool *dest = nblocks + to ; 
	for (; to != dest;) 
		*(to++) = from ;
}

inline void stretch_bool(block* bs, bool b, int len) {
    uint64_t stretch = 0ULL ;
    if (b) stretch = ~stretch ; 
    bs[0] = makeBlock(stretch, stretch) ;
    copyBlocks_arr(bs + 1, *bs, len-1) ;
}

/**** Misc ****/
inline void initialize_blocks(block *blk, int num_blocks, const block init=zero_block) {
  for (int i = 0 ; i < num_blocks ; i++)
    blk[i] = init ;
}

inline void initialize_bools(bool *bs, int num_bools, const bool init=false) {
  for (int i = 0 ; i < num_bools ; i++)
    bs[i] = init ;
}

bool get_bool_from_block(const block *blk) ;

bool get_bools_from_block(const block *blk) ;

bool check_equal(block *b1, block *b2) ;

bool xorr(bool b1, bool b2) ;

int get_ohe_blocks(int n) ;

int get_ohe_bytes(int n) ;

bool test_bit(const block& blk, int i) ;

inline block left_shift(const block &blk, int shift) {
  uint64_t* data = (uint64_t*)&blk ;

  if (shift > 127)
    return zero_block ;
  else if (shift > 63) {
    uint64_t dat1, dat0 ;
    dat0 = 0ULL ;
    dat1 = data[0] << (shift - 64) ;
    return makeBlock(dat1, dat0) ;
  } else {
    uint64_t dat1, dat0 ;
    uint64_t mask = (1ULL << shift) - 1 ;

    // Copying most significant bits of data[0] that will get erased
    mask = mask << (64 - shift) ;
    mask = mask & data[0] ;
    mask = mask >> (64 - shift) ;

    // Shifting data[1] and pasting from data[0]
    dat1 = data[1] << shift ;
    dat1 = dat1 ^ mask ;

    // Shifting data[0] and return
    dat0 = data[0] << shift ;
    return makeBlock(dat1, dat0) ;
  }
}

inline block right_shift(const block &blk, int shift) {
  uint64_t* data = (uint64_t*)&blk ;

  if (shift > 127)
    return zero_block ;
  else if (shift > 63) {
    uint64_t dat1, dat0 ;
    dat1 = 0ULL ;
    dat0 = data[1] >> (shift - 64) ;
    return makeBlock(dat1, dat0) ;
  } else {
    uint64_t dat1, dat0 ;
    uint64_t mask = (1ULL << shift) - 1 ;

    // Copying least significant bits of data[1] that will get erased
    mask = mask & data[1] ;

    // Shifting data[0] and pasting from data[1]
    // cout << "This is data[0] = " << hex << setw(16) << setfill('0') << data[0] << "\n" ;
    dat0 = data[0] >> shift ;
    mask = mask << (64 - shift) ;
    dat0 = dat0 ^ mask ;

    // Shifting data[1] and return
    dat1 = data[1] >> shift ;
    return makeBlock(dat1, dat0) ;
  }
}

void copyBits(block *to, int pos1, block *from, int pos2, int bits) ;

/************************* GMT to OHE Conversion *************************/

uint64_t comb(uint64_t n, uint64_t k) ;

int rankk(vector<int> &subset, int n) ;

int total_rank(vector<int> &subset, int n) ;

unordered_map<int, vector<vector<int>>> get_subsets(int n) ;

vector<vector<int>> counts_to_subsets(vector<int> &counts, int curr) ;

vector<int> get_ohe_ranks (vector<int> &cop, int n, int singleton) ;


/************************* Old OHE stuff *************************/

vector<pair<int,int>> get_round_robin_scheme(int party, int parties) ;

unordered_map<int, NetIO*> get_pairwise_channels(int party, int parties, int start_port, vector<pair<int,int>> robin) ; 

unordered_map<int, NetIO*> get_pairwise_channels_threaded(int party, int parties, int start_port) ;

/************************* Other utilities *************************/

void reconst_flat(int party, COT<NetIO> *ot1, COT<NetIO> *ot2, int bits, int batch_size, block **shares, block **rec) ;

void reconst(int party, COT<NetIO> *ot1, COT<NetIO> *ot2, int bits, block *share, block *rec) ;

void rotate(int n, block *vec, uint64_t rot, block *vec_rotated) ;

void get_conv_metadata(
  int n, 
  unordered_map<int, 
  vector<vector<int>>> &mp, 
  unordered_map<int, int> &singleton_gmt, 
  unordered_map<int, int> &remaining_gmt, 
  vector<int> &conv_left, vector<int> &conv_right1, 
  vector<vector<int>> &conv_right2) ;

#endif