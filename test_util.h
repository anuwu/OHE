#include "emp-tool/emp-tool.h"
#include "emp-ot/emp-ot.h"
#include <unordered_map>
#include <vector>
#include <iostream>

using namespace emp ;
using namespace std ;

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

inline void andBlocks_arr(block* res, const block* x, const block* y, int nblocks) {
	const block *dest = nblocks + x ;
	for (; x != dest;) {
		*(res++) = *(x++) ^ *(y++);
	}
}

inline void andBlocks_arr(block* res, const block* x, block y, int nblocks) {
	const block *dest = nblocks + x ;
	for (; x != dest;) 
		*(res++) =  *(x++) ^ y;
}

inline void copyBlocks_arr(block* to, const block* from, int nblocks) {
	const block *dest = nblocks + from ;
	for (; from != dest;) {
		*(to++) = *(from++) ;
	}
}

inline void copyBlocks_arr(block *to, block from, int nblocks) {
	const block *dest = nblocks + to ; 
	for (; to != dest;) 
		*(to++) = from ;
}

inline void copyBools_arr(bool* to, const bool* from, int nblocks) {
	const bool *dest = nblocks + from ;
	for (; from != dest;) {
		*(to++) = *(from++) ;
	}
}

inline void copyBools_arr(bool *to, bool from, int nblocks) {
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
