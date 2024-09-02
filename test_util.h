#include "emp-tool/emp-tool.h"
#include "emp-ot/emp-ot.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iostream>

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
inline void initialize_blocks(block *blk, int num_blocks, const block init = zero_block) {
  for (int i = 0 ; i < num_blocks ; i++)
    blk[i] = init ;
}

inline void initialize_bools(bool *bs, int num_bools, const bool init = false) {
  for (int i = 0 ; i < num_bools ; i++)
    bs[i] = init ;
}

bool get_bool_from_block(const block *blk) {
    uint64_t *data = (uint64_t*) blk ;
    return data[1] & 1 ;
}

bool get_bools_from_block(const block *blk) {
    bool *b = new bool[128] ;
    uint64_t *data = (uint64_t*) blk ;

    for (int i = 0 ; i < 64 ; i++) {
        b[63-i] = (data[0] >> i) & 1 ;
        b[127-i] = (data[1] >> i) & 1 ;
    }

    return b ;
}

bool check_equal(block *b1, block *b2) {
  uint64_t *d1 = (uint64_t*)b1 ;
  uint64_t *d2 = (uint64_t*)b2 ;

  return d1[0] == d2[0] && d1[1] == d2[1] ;
}

bool xorr(bool b1, bool b2) {
    return b1 ? (!b2) : b2 ;
}

int n_to_blocks(int n) {
  // return n < 8 ? 1 : 1 << (n-7) ;
  return 1 << max(n-7, 0) ;
}

int n_to_bytes(int n) {
  // return n < 4 ? 1 : 1 << (n-3) ;
  return 1 << max(n-3, 0) ;
}

bool test_bit(const block& blk, int i) {
  uint64_t *data = (uint64_t*)&blk ;
  if (i < 64)
    return data[0] & (1 << i) ;
  else
    return data[1] & (1 << (i-64)) ;
}

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
    dat0 = data[0] >> shift ;
    dat0 = dat0 ^ mask ;

    // Shifting data[1] and return
    dat1 = data[1] >> shift ;
    return makeBlock(dat1, dat0) ;
  }
}

inline void copyBits(block *to, int pos1, block *from, int pos2, int bits) {
  block mask1 = zero_block ;
  for (int ind1 = 0 ; ind1 < 128 ; ind1++)
    if (ind1 < pos1 || ind1 > pos1 + bits - 1)
      mask1 = set_bit(mask1, ind1) ;
  andBlocks_arr(to, mask1, 1) ;

  // cout << "Mask1 - \n" ;
  // cout << *to << "\n" ;

  block mask2 = zero_block ;
  int ind2 = pos2 ;
  while (ind2 < pos2 + bits) {
    mask2 = set_bit(mask2, ind2) ;
    ind2++ ;
  }
  andBlocks_arr(&mask2, from, 1) ;
  // cout << "Mask2 before shift - \n" ;
  // cout << mask2 << "\n" ;
  if (pos1 > pos2)
    mask2 = left_shift(mask2, pos1 - pos2) ;
  else
    mask2 = right_shift(mask2, pos2 - pos1) ;

  // cout << "Mask2 after shift - \n" ;
  // cout << mask2 << "\n" ;
  xorBlocks_arr(to, &mask2, 1) ;
}

/************************* GMT to OHE Conversion *************************/

uint64_t comb(uint64_t n, uint64_t k)
{
    if (k > n) return 0;
    if (k * 2 > n) k = n-k;
    if (k == 0) return 1;

    int result = n;
    for( int i = 2; i <= k; ++i ) {
        result *= (n-i+1);
        result /= i;
    }
    return result;
}

int rankk(vector<int> &subset, int n) {
    int rem_items = n ;
    int l = subset.size() ;
    int acc = 0 ;
    for (int ind = 0 ; ind < l ; ind++) {
        int entry = subset[ind] ;
        int index = 0 ;
        for (int head_done = 0 ; head_done < rem_items + entry - n ; head_done++)
            index += comb(rem_items-head_done-1, l-ind-1) ;

        acc += index ;
        rem_items = n - entry - 1 ;
    }

    return acc ;
}

int total_rank(vector<int> &subset, int n) {
    int r = rankk(subset, n) ;

    for (int i = 0 ; i < (int)subset.size() ; i++)
        r += comb(n, i) ;

    return r ;
}

unordered_map<int, vector<vector<int>>> get_subsets(int n) {
    assert(n > 0) ;

    unordered_map<int, vector<vector<int>>> ret ;
    ret[0] = vector<vector<int>>() ;
    ret[1] = vector<vector<int>>() ;
    
    for (int i = 0 ; i < n ; i++) {
        vector<int> vec ;
        vec.push_back(i) ;
        ret[1].push_back(vec) ;
    }    

    for (int i = 2 ; i < n+1 ; i++) {
        ret[i] = vector<vector<int>>() ;
        for (int j = 0 ; j < (int)ret[i-1].size() ; j++) {
            vector<int> subset = ret[i-1][j] ;
            int l = subset.size() ;
            int last_entry = subset[l-1] ;
            if (last_entry < n-1) {
                for (int k = last_entry + 1 ; k < n ; k++) {
                    vector<int> curr = subset ;
                    curr.push_back(k) ;
                    ret[i].push_back(curr) ;
                }
            }
        }
    }

    return ret ;
}

vector<vector<int>> counts_to_subsets(vector<int> &counts, int curr) {
    if (curr == 0) {
        vector<vector<int>> ans ;
        vector<int> empty ;

        if (counts[0] == 0)
            ans.push_back(empty) ;
        else if (counts[0] == 1) {
            empty.push_back(0) ;
            ans.push_back(empty) ;
        } else {
            ans.push_back(empty) ;
            empty.push_back(0) ;
            ans.push_back(empty) ;
        }

        return ans ;
    }

    vector<vector<int>> sub_ans = counts_to_subsets(counts, curr-1) ;
    if (counts[curr] == 0)
        return sub_ans ;
    else if (counts[curr] == 1) {
        for (auto it1 = sub_ans.begin() ; it1 != sub_ans.end() ; it1++)
            it1->push_back(curr) ;

        return sub_ans ;
    } else {
        int sz = sub_ans.size() ;
        vector<vector<int>> ans = sub_ans ;
        for (int i = 0 ; i < sz ; i++) {
            ans[i].push_back(curr) ;
            ans.push_back(sub_ans[i]) ;
        }

        return ans ;
    }
}

vector<int> get_ohe_ranks (vector<int> &cop, int n, int singleton) {
  unordered_set<int> cop_set ;
  for (auto it = cop.begin() ; it != cop.end() ; it++)
    cop_set.insert(*it) ;
  vector<int> cop_count ;
  for (int k = 0 ; k < n ; k++) {
    if (k == singleton)
      cop_count.push_back(0) ;
    else if (cop_set.find(k) != cop_set.end())
      cop_count.push_back(2) ;
    else
      cop_count.push_back(1); 
  }

  vector<vector<int>> cop_subsets = counts_to_subsets(cop_count, n-1) ;
  vector<int> ret ;

  for (auto it = cop_subsets.begin() ; it != cop_subsets.end() ; it++) {
    int rnk = total_rank(*it, n) ;
    ret.push_back(rnk) ;
  }

  return ret ;
}