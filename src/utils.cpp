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

void get_ohe_from_plain(block *inp, block *ohe) {
  uint64_t *data = (uint64_t*)inp ;
  SET_BIT(ohe, data[0]) ;
}