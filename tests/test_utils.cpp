#include "emp-ot/emp-ot.h"
#include "utils.h"
#include <iostream>
#include <unordered_set>
#include <time.h>

using namespace std ;
using namespace emp ;

constexpr int threads = 1 ;


int main(int argc, char** argv) {
  /************************* GMT to OHE converter *************************/
  {
    // const auto abort = [&] {
    //   cerr
    //     << "usage: "
    //     << argv[0]
    //     << " <n> \n";
    //   exit(EXIT_FAILURE);
    // } ;

    // if (argc != 2)
    //   abort();

    // int n ;
    // n = atoi(argv[1]) ;

    // unordered_map<int, vector<vector<int>>> mp ; 		    // length of subset -> all subset of that length
    // unordered_map<int, int> singleton_gmt ;             // total rank of subset -> first element of the subset
    // unordered_map<int, int> remaining_gmt ;             // total rank of subset -> total rank of tail of subset

    // // unordered_map<int, pair<int, int>> converter_ind1 ;
    // vector<int> conv_left ;
    // vector<int> conv_right1 ;
    // vector<vector<int>> conv_right2 ;

    // mp = get_subsets(n) ;
    // cout << "OHE_INDEX(RANK) = OHE_INDEX(RANK) + SINGLETON\n" ;
    // for (int i = 1 ; i < n+1 ; i++) {
    //   cout << "All subsets of length " << i << "\n" ;
    //   for (int j = 0 ; j < (int)mp[i].size() ; j++) {
    //     vector<int> vec = mp[i][j] ;
    //     for (auto it = vec.begin() ; it != vec.end() ; it++)
    //       cout << *it << ", " ;
        
    //     int totrnk = total_rank(vec, n) ;
    //     singleton_gmt[totrnk] = vec[0] ; 
    //     vector<int> cop = vec ; cop.erase(cop.begin()) ; 
    // 		int remrnk = total_rank(cop, n) ;
    // 		remaining_gmt[totrnk] = remrnk ;
    // 		int rem_ohe_index = 0, ohe_index ;
    // 		for (auto it1 = cop.begin() ; it1 != cop.end() ; it1++)
    // 			rem_ohe_index += 1 << *it1 ;
    // 		ohe_index = rem_ohe_index + (1 << vec[0]) ;

    //     // converted[ohe_index] = make_pair(rem_ohe_index, vec[0]) 

    // 		ohe_index = (1 << n) - ohe_index - 1 ;
    // 		rem_ohe_index = (1 << n) - rem_ohe_index - 1 ;

    //     // cop_count = reverse(cop_count.begin(), cop_count.end()) ;
    //     // vector<vector<int>> cop_subsets = counts_to_subsets(cop_count, n-1) ;
    //     // vector<int> subsets_to_rank = subsets_to_rank(cop_subsets) ;
    //     vector<int> ranks = get_ohe_ranks(cop, n, vec[0]) ;

    // 		// cout << " -->\t" << totrnk << ", " << singleton[totrnk] << ", " << remaining[totrnk] << " | " << ohe_index << ", " << rem_ohe_index << "\n" ;
    // 		cout << " -->\t" << ohe_index << "(" << totrnk << ")" << " = " << rem_ohe_index << "(" << remrnk << ")" << " + " << vec[0] << " | " ;
    //     // for (auto it = cop_count.begin() ; it != cop_count.end() ; it++)
    //     //   cout << *it << "," ;
    //     // cout << " --> " ;
    //     // for (auto it1 = cop_subsets.begin() ; it1 != cop_subsets.end() ; it1++) {
    //     //   vector<int> sub = *it1 ;
    //     //   for (auto it2 = sub.begin() ; it2 != sub.end() ; it2++) {
    //     //     cout << *it2 << "," ;
    //     //   }
    //     //   if (sub.size() == 0) cout << "ONE" ;
    //     //   if (it1 + 1 != cop_subsets.end()) cout << " + " ;
    //     // }

    //     for (auto it = ranks.begin() ; it != ranks.end() ; it++) {
    //       cout << *it ;
    //       if (it + 1 != ranks.end()) cout << " + " ;
    //     }
    //     cout << "\n" ;

    //     conv_left.push_back(ohe_index) ;
    //     conv_right1.push_back(rem_ohe_index) ;
    //     conv_right2.push_back(ranks) ;
    // 	}
    //   cout << "\n" ;
    // }  

    // cout << "\nFrom the converter -\n" ;
    // for (int i = 0 ; i < (int)conv_left.size() ; i++) {
    //   cout << conv_left[i] << " = " << conv_right1[i] << " + (" ;
    //   for (int j = 0 ; j < (int)conv_right2[i].size() ; j++) {
    //     cout << conv_right2[i][j] ;
    //     if (j+1 != (int)conv_right2[i].size())
    //       cout << " + " ;
    //   }
        
    //   cout << ")\n" ;
    // } 

    // return 0 ;
  }


  /************************* copyBits test *************************/

  {
    // const auto abort = [&] {
    //   cerr << "usage: " << argv[0] << " <pos1> <pos2> <bits>";
    //   exit(EXIT_FAILURE);
    // } ;

    // if (argc != 4)
    //   abort();

    // PRG prg ;
    // block *blk1 = new block[1] ;
    // block *blk2 = new block[1] ;
    // int pos1 = atoi(argv[1]) ;
    // int pos2 = atoi(argv[2]) ;
    // int bits = atoi(argv[3]) ;

    // prg.random_block(blk1, 1) ;
    // prg.random_block(blk2, 1) ;

    // cout << "Before Copy - \n" ;
    // cout << *blk1 << "\n" ;
    // cout << *blk2 << "\n" ;

    // copyBits(blk1, pos1, blk2, pos2, bits) ;

    // cout << "\nAfter Copy - \n" ;
    // cout << *blk1 << "\n" ;
    // cout << *blk2 << "\n" ;

    // return 0 ;
  }

  /************************* setBit test *************************/

  {
    // const auto abort = [&] {
    //   cerr << "usage: " << argv[0] << " <pos>\n" ;
    //   exit(EXIT_FAILURE);
    // } ;

    // if (argc != 2)
    //   abort() ;

    // int pos = atoi(argv[1]) ;

    // PRG prg ;
    // block *blk = new block[1] ;
    // prg.random_block(blk, 1) ;

    // cout << "Before -->\t" << blk[0] << "\n" ;
    // blk[0] = set_bit(blk[0], pos) ;
    // cout << "After -->\t" << blk[0] << "\n" ;

    // delete[] blk ;
    // return 0 ;
  }

  /************************* testBit test *************************/

  {
    // const auto abort = [&] {
    //   cerr << "usage: " << argv[0] << " <pos>\n" ;
    //   exit(EXIT_FAILURE);
    // } ;

    // if (argc != 2)
    //   abort() ;

    // int pos = atoi(argv[1]) ;

    // PRG prg ;
    // block *blk = new block[1] ;
    // prg.random_block(blk, 1) ;

    // cout << "blk --> " << blk[0] << "\n" ;
    // bool the_bool = test_bit(blk[0], pos) ;
    // cout << "bool --> " << the_bool << "\n" ;
    // cout << "blk --> " << blk[0] << "\n" ;

    // delete[] blk ;
    // return 0 ;
  }


  /************************* zero wrap-over *************************/

  {

    const auto abort = [&] {
      cerr << "usage: " << argv[0] << "\n" ;
      exit(EXIT_FAILURE);
    } ;

    if (argc != 1)
      abort() ;

    uint64_t index = 0ULL ;
    cout << "This is zero --> " << index << "\n" ;
    index = ~index ;
    cout << "This is all 1s --> " << index << "\n" ;
    index += 1 ;
    cout << "This is wrap-over --> " << index << "\n" ;

    return 0 ;
  }

}