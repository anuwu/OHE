#include "emp-ot/emp-ot.h"
#include "utils.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iostream>

using namespace emp ;
using namespace std ;

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

int get_ohe_blocks(int n) {
  // return n < 8 ? 1 : 1 << (n-7) ;
  return 1 << max(n-7, 0) ;
}

int get_ohe_bytes(int n) {
  // return n < 4 ? 1 : 1 << (n-3) ;
  return 1 << max(n-3, 0) ;
}

bool test_bit(const block& blk, int i) {
  uint64_t *data = (uint64_t*)&blk ;
  if (i < 64)
    return data[0] & (1ULL << i) ;
  else
    return data[1] & (1ULL << (i-64)) ;
}

void copy_schedule(int pos_to, int pos_from, int bits, vector<int> &offset1s, vector<int> &offset2s, vector<int> &pos1s, vector<int> &pos2s, vector<int> &copy_lengths) {
  int pos1, pos2, offset1, offset2 ;

  pos1 = pos_to ;
  pos2 = pos_from ;
  offset1 = 0 ;
  offset2 = 0 ;

  int copy_length = 0 ;
  do {
    // Find how many bits need to be copied
    copy_length = min(bits, 128 - (pos1 < pos2 ? pos2 : pos1)) ;

    // Storing schedule
    bits -= copy_length ;
    offset1s.push_back(offset1) ;
    offset2s.push_back(offset2) ;
    pos1s.push_back(pos1) ;
    pos2s.push_back(pos2) ;
    copy_lengths.push_back(copy_length) ;

    // Increment position
    pos1 += copy_length ;
    pos1 %= 128 ;
    pos2 += copy_length ;
    pos2 %= 128 ;
    if (!pos1)
      offset1++ ;
    if (!pos2)
      offset2++ ;
  } while (bits > 0) ;
}

// bug when 0, 62, 3
inline void copyBits_single(block *to, int pos1, block *from, int pos2, int bits) {
  block mask1 = zero_block ;
  for (int ind1 = 0 ; ind1 < 128 ; ind1++)
    if (ind1 < pos1 || ind1 > pos1 + bits - 1)
      mask1 = set_bit(mask1, ind1) ;
  andBlocks_arr(to, mask1, 1) ;

  // cout << "\nThis is mask1 --> " << mask1 << "\n" ;
  block mask2 = zero_block ;
  int ind2 = pos2 ;
  while (ind2 < pos2 + bits) {
    mask2 = set_bit(mask2, ind2) ;
    ind2++ ;
  }
  // cout << "This is mask2 --> " << mask2 << "\n" ;
  andBlocks_arr(&mask2, from, 1) ;
  // cout << "This is anded mask2 --> " << mask2 << "\n" ;
  if (pos1 > pos2)
    mask2 = left_shift(mask2, pos1 - pos2) ;
  else
    mask2 = right_shift(mask2, pos2 - pos1) ;
  // cout << "\nThis is shifted mask2 --> " << mask2 << "\n" ;

  xorBlocks_arr(to, &mask2, 1) ;
}

void copyBits(block *to, int pos1, block *from, int pos2, int bits) {
  vector<int> offset1s ;
  vector<int> offset2s ;
  vector<int> pos1s ;
  vector<int> pos2s ;
  vector<int> copy_lengths ;

  copy_schedule(pos1, pos2, bits, offset1s, offset2s, pos1s, pos2s, copy_lengths) ;
  int sched_len = pos1s.size() ;

  for (int i = 0 ; i < sched_len ; i++)
    copyBits_single(to+offset1s[i], pos1s[i], from+offset2s[i], pos2s[i], copy_lengths[i]) ;
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

/************************* Old OHE stuff *************************/

vector<pair<int,int>> remove_dummy(vector<pair<int,int>> robin, int parties) {
  pair<int,int> pp ;
  for (auto it = robin.begin() ; it != robin.end() ; it++) {
    pp = *it ;
    if (pp.first == parties || pp.second == parties) {
      robin.erase(it) ;
      break ;
    }
  }
  return robin ;
}

vector<pair<int,int>> get_round_robin_scheme(int party, int parties) {
  if (parties % 2) {
    vector<pair<int,int>> robin = get_round_robin_scheme(party, parties + 1) ;
    return remove_dummy(robin, parties + 1) ;
  } else {
    vector<pair<int,int>> robin ;
    vector<pair<int,int>> circle ;

    // Initialize the circle
    for (int i = 1, j = parties ; i < j ; i++, j--)
        circle.push_back(make_pair(i, j)) ;

    // Trivial case
    if (parties == 2)
      return circle ;

      // (parties - 1) rounds in total
    for (int i = 0 ; i < parties - 1 ; i++) {
      // Insert into robin
      for (auto it = circle.begin() ; it != circle.end() ; it++) {
        if (it->first == party || it->second == party) {
          pair<int,int> pp ;
          if (it->first < it->second)
              pp = *it ;
          else
              pp = make_pair(it->second, it->first) ;
          robin.push_back(pp) ;
        }
      }
            
      // Rotate
      /*
      1   2   3   4   5   6   7
      14  13  12  11  10  9   8

      save 14, save 7
      1   14  2   3   4   5   6
      14  13  12  11  10  9   8

      1   14  2   3   4   5   6
      13  12  11  10  9   8   7
      */

      int save = circle[0].second ;
      int tmp ;
      for (auto it = circle.begin() + 1 ; it != circle.end() ; it++) {
        tmp = it->first ;
        it->first = save ;
        save = tmp ;
      }

      for (auto it = circle.rbegin() ; it != circle.rend() ; it++) {
        tmp = it->second ;
        it->second = save ;
        save = tmp ;
      }
    }

    return robin ;
  }
}

int gpoffset(pair<int,int> pp, int parties) {
  int p1, p2 ;
  p1 = pp.first ; p2 = pp.second ;
  return (p1-1)*parties + (p2 - 1) ;
}

unordered_map<int, NetIO*> get_pairwise_channels(int party, int parties, int start_port, vector<pair<int,int>> robin) {
    unordered_map<int, NetIO*> ios ;
    NetIO *io ;
    for (auto it = robin.begin() ; it != robin.end() ; it++) {
      int p1 = it->first, p2 = it->second ;
      int port_no = start_port + gpoffset(*it, parties) ;
      io = new NetIO(party == p1 ? nullptr : "127.0.0.1", port_no) ;
      ios[p1 == party ? p2 : p1] = io ;
    }

    return ios ;
}

unordered_map<int, NetIO*> get_pairwise_channels_threaded(int party, int parties, int start_port) {
  unordered_map<int, NetIO*> ios ;
  ThreadPool pool(parties - 1) ;

    for (int i = 1 ; i <= parties ; i++) {
      if (i == party)
        continue ;

      int p1 = party < i ? party : i, p2 = party < i ? i : party ;
      int port_no = start_port + gpoffset(make_pair(p1, p2), parties) ;
      pool.enqueue([party, i, port_no, &ios] {
        ios[i] = new NetIO(party < i ? nullptr : "127.0.0.1", port_no) ;
      }) ;        
    }

    return ios ;
}

void reconst_flat(int party, COT<NetIO> *ot1, COT<NetIO> *ot2, int bits, int batch_size, block **shares, block **rec) {
  int flat_blocks = (batch_size*bits+127)/128 ;
  block *flat = new block[flat_blocks] ;
  block *reconst_flat = new block[flat_blocks] ;
  initialize_blocks(flat, flat_blocks) ;
  initialize_blocks(reconst_flat, flat_blocks) ;
  for (int b = 0 ; b < batch_size ; b++)
    copyBits(flat+(b*bits)/128, (b*bits)%128, shares[b], 0, bits) ;
  reconst(party, ot1, ot2, batch_size*bits, flat, reconst_flat) ;
  for (int b = 0 ; b < batch_size ; b++)
    copyBits(rec[b], 0, reconst_flat+(b*bits)/128, (b*bits)%128, bits) ;

  delete[] flat ;
  delete[] reconst_flat ;
}

void reconst(int party, COT<NetIO> *ot1, COT<NetIO> *ot2, int bits, block *share, block *rec) {
  int bytes = (bits+7)/8 ;
  int blocks = (bits+127)/128 ;
  block *rcv_share = new block[blocks] ;
  initialize_blocks(rcv_share, blocks) ;
  if (party == ALICE) {
    ot1->io->send_data(share, bytes) ;
    ot2->io->recv_data(rcv_share, bytes) ;
  } else {
    ot1->io->recv_data(rcv_share, bytes) ;
    ot2->io->send_data(share, bytes) ;
  }
  ot1->io->flush() ;
  ot2->io->flush() ;

  xorBlocks_arr(rec, rcv_share, share, blocks) ;
  delete[] rcv_share ;
}

void rotate(int n, block *vec, uint64_t rot, block *vec_rotated) {
  uint64_t N = 1ULL << n ;
  for (uint64_t i = 0 ; i < N ; i++)
    if (TEST_BIT(vec, i))
      SET_BIT(vec_rotated, i^rot) ;
}

void get_conv_metadata(
  int n, 
  unordered_map<int, 
  vector<vector<int>>> &mp, 
  unordered_map<int, int> &singleton_gmt, 
  unordered_map<int, int> &remaining_gmt, 
  vector<int> &conv_left, vector<int> &conv_right1, 
  vector<vector<int>> &conv_right2) {
  // Populate metadata
  mp = get_subsets(n) ;
  for (int i = 1 ; i < n+1 ; i++) {
    for (int j = 0 ; j < (int)mp[i].size() ; j++) {
      vector<int> vec = mp[i][j] ;
      int totrnk = total_rank(vec, n) ;
      singleton_gmt[totrnk] = vec[0] ; 
      vector<int> cop = vec ; cop.erase(cop.begin()) ; 
      int remrnk = total_rank(cop, n) ;
      remaining_gmt[totrnk] = remrnk ;

      int rem_ohe_index = 0, ohe_index ;
      for (auto it1 = cop.begin() ; it1 != cop.end() ; it1++)
        rem_ohe_index += 1 << *it1 ;
      ohe_index = rem_ohe_index + (1 << vec[0]) ;

      ohe_index = (1 << n) - ohe_index - 1 ;
      rem_ohe_index = (1 << n) - rem_ohe_index - 1 ;
      vector<int> ranks = get_ohe_ranks(cop, n, vec[0]) ;
      conv_left.push_back(ohe_index) ;
      conv_right1.push_back(rem_ohe_index) ;
      conv_right2.push_back(ranks) ;
    }
  }
}