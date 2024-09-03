#include "emp-ot/emp-ot.h"
#include "test_util.h"
#include <iostream>
#include <time.h>

using namespace std ;
using namespace emp ;

constexpr int threads = 1 ;

block* reconst_ohe(int party, int n, COT<NetIO> *ot1, COT<NetIO> *ot2, block *ohe, bool print=false) {
    // Declare and initialize
    int num_blocks = n_to_blocks(n) ;
    int comm_bytes = n_to_bytes(n) ;
    block *rcv = new block[num_blocks] ;
    block *res = new block[num_blocks] ;
    block *zero = new block[1] ; 
    initialize_blocks(rcv, num_blocks) ;
    initialize_blocks(res, num_blocks) ;
    initialize_blocks(zero, 1) ;    

    // Send and Recv
    if (party == ALICE) {
      ot1->io->send_data(ohe, comm_bytes) ; 
      ot2->io->recv_data(rcv, comm_bytes) ; 
    } else {
      ot1->io->recv_data(rcv, comm_bytes) ; 
      ot2->io->send_data(ohe, comm_bytes) ; 
    }
    ot1->io->flush() ;
    ot2->io->flush() ;

    // Reconstruct and print
    xorBlocks_arr(res, rcv, ohe, num_blocks) ;
    if (print) {
      cout << "Received -\n" ;
      for (int i = 0 ; i < num_blocks ; i++)
        cout << rcv[i] << "\n" ;

      cout << "Result -\n" ;
      for (int i = 0 ; i < num_blocks ; i++) {
        if(cmpBlock(res + i, zero, 1))
          cout << res[i] << "\n" ;
        else
          cout << "\033[1;31m" << res[i] << "\033[0m\n" ;
      }
    }

    // Delete
    delete[] rcv ; delete[] zero ;
    return res ;
}

block* random_ohe(int party, int n, COT<NetIO> *ot1, COT<NetIO> *ot2, bool print_comm=false) {
  /************************* Base Case *************************/

  // Declare
  PRG prg ;
  int num_blocks = n_to_blocks(n) ;
  bool *b = new bool[n] ; 
  block *ohe = new block[num_blocks] ;

  // Initialize
  prg.random_bool(b, n) ;
  for (int i = 0 ; i < num_blocks ; i++) 
    ohe[i] = zero_block ;
  if (party == ALICE) {
      ohe[0] = set_bit(ohe[0], b[0] ? 1 : 0) ;    
    } else {
      if (b[0]) {
        ohe[0] = set_bit(ohe[0], 1) ;
        ohe[0] = set_bit(ohe[0], 0) ;
      }
    }

  // Handle base case
  if (n == 1) {
    if (print_comm) cout << "ROT comms : 0 bytes\n" ;
    if (print_comm) cout << "Corr comms : 0 bytes\n" ;
    delete[] b ;
    return ohe ;
  }

  /************************* Declare *************************/

  uint64_t rot_comms = ot1->io->counter ;
  block *r0 = new block[n-1] ;
  block *r1 = new block[n-1] ;
  block *rcv_ot = new block[n-1] ;

  /************************* Perform Random OTs *************************/

  // Send and Recv
  if (party == ALICE) {
    ot1->send_rot(r0, r1, n-1) ;
    ot2->recv_rot(rcv_ot, b+1, n-1) ; 
  } else {
    ot1->recv_rot(rcv_ot, b+1, n-1) ;
    ot2->send_rot(r0, r1, n-1) ;
  }
  ot1->io->flush() ;
  ot2->io->flush() ;

  // Print stuff
  rot_comms = ot1->io->counter - rot_comms ;
  if (print_comm) cout << "ROT comms : " << rot_comms << " bytes\n" ;
  int64_t corr_comms = ot1->io->counter ;

  /************************* Correct Random OTs *************************/

  // Declare intermediate variables
  int largest_block = n_to_blocks(n-1) ;
  block *msg = new block[largest_block] ;
  block *rcv_msg = new block[largest_block] ;
  block *cross_share = new block[largest_block] ;
  block *r0_actual = new block[largest_block] ;
  block *r1_actual = new block[largest_block] ;
  block *rcv_ot_actual = new block[largest_block] ;

  // Iterate over rounds
  for (int r = 2 ; r <= n ; r++) {
    // Initialize
    int comm_bytes = n_to_bytes(r-1) ;
    int small_blocks = n_to_blocks(r-1) ;
    initialize_blocks(rcv_msg, small_blocks) ; 

    // Setup r0, r1 and received OTs
    if (r < 8) {
      block mask = zero_block ;
      for (int pos = 0 ; pos < 1 << (r-1) ; pos++)
        mask = set_bit(mask, pos) ;

      andBlocks_arr(r0_actual, r0+r-2, &mask, 1) ; 
      andBlocks_arr(r1_actual, r1+r-2, &mask, 1) ; 
      andBlocks_arr(rcv_ot_actual, rcv_ot+r-2, &mask, 1) ;
    } else if (r == 8) {
      r0_actual[0] = r0[r-2] ;
      r1_actual[0] = r1[r-2] ;
      rcv_ot_actual[0] = rcv_ot[r-2] ;
    } else {
      PRG prg0(r0+r-2) ; PRG prg1(r1+r-2) ; PRG prg_rcv(rcv_ot+r-2) ;
      prg0.random_block(r0_actual, small_blocks) ;
      prg1.random_block(r1_actual, small_blocks) ;
      prg_rcv.random_block(rcv_ot_actual, small_blocks) ;
    }

    // Random OT correction
    xorBlocks_arr(msg, r0_actual, r1_actual, small_blocks) ; 
    xorBlocks_arr(msg, ohe, small_blocks) ;
    if (party == ALICE) {
      ot1->io->send_data(msg, comm_bytes) ; 
      ot2->io->recv_data(rcv_msg, comm_bytes) ; 
    } else {
      ot1->io->recv_data(rcv_msg, comm_bytes) ;
      ot2->io->send_data(msg, comm_bytes) ; 
    }
    ot1->io->flush() ;
    ot2->io->flush() ;

    // Cross share
    copyBlocks_arr(cross_share, rcv_ot_actual, small_blocks) ;
    if (b[r-1])
      xorBlocks_arr(cross_share, rcv_msg, small_blocks) ;
    if (b[r-1])
      xorBlocks_arr(cross_share, ohe, small_blocks) ;
    xorBlocks_arr(cross_share, r0_actual, small_blocks) ;

    // Manipulate cross share
    if (r < 7) {
      block tmp = cross_share[0] ;
      cross_share[0] = cross_share[0] << (1 << (r-1)) ;
      xorBlocks_arr(cross_share, &tmp, 1) ;
    }
    else if (r==7) {
      uint64_t* data = (uint64_t*)cross_share ;
      data[1] = data[0] ;
    } else {
      copyBlocks_arr(ohe + small_blocks, cross_share, small_blocks) ;
    }
    
    // OHE for next iteration
    xorBlocks_arr(ohe, cross_share, small_blocks) ;
  }

  // Delete intermediate variables
  delete[] msg ; delete[] rcv_msg ; delete[] cross_share ;
  delete[] r0_actual ; delete[] r1_actual ; delete[] rcv_ot_actual ;

  // Print stuff
  corr_comms = ot1->io->counter - corr_comms ;
  if (print_comm) cout << "Corr comms : " << corr_comms << " bytes\n" ;

  /************************* Delete and Return *************************/

  delete[] b ;
  delete[] r0 ; delete[] r1 ; delete[] rcv_ot ;
  return ohe ;
}

block* random_gmt(int party, int n, COT<NetIO> *ot1, COT<NetIO> *ot2, bool print_comm=false) {
  /************************* Base Case *************************/

  // Declare
  PRG prg ;
  int num_blocks = n_to_blocks(n) ;
  bool *single_bools = new bool[n] ;
  block *ohe = new block[num_blocks] ;

  // Initialize
  prg.random_bool(single_bools, n) ;
  for (int i = 0 ; i < num_blocks ; i++) 
    ohe[i] = zero_block ;

  // Handle base case
  if (n == 1) {
    if (party == ALICE) {
      ohe[0] = set_bit(ohe[0], single_bools[0] ? 1 : 0) ;    
    } else {
      if (single_bools[0]) {
        ohe[0] = set_bit(ohe[0], 1) ;
        ohe[0] = set_bit(ohe[0], 0) ;
      }
    }

    if (print_comm) cout << "ROT comms : 0 bytes\n" ;
    if (print_comm) cout << "Corr comms : 0 bytes\n" ;
    delete[] single_bools ;
    return ohe ;
  }

  /************************* Declare and Init *************************/

  // Declare
  int64_t rot_comms = ot1->io->counter ;
  int ohe_size = 1 << n ; int num_ots = ohe_size - n - 1 ;
  bool *ohe_bool = new bool[ohe_size] ;
  block *hot = new block[ohe_size] ; 

  // Initialize
  for (int i = 0 ; i < ohe_size ; i++)
    ohe_bool[i] = false ;
  // First entry in hot[] is a share of 1
  for (int i = 0 ; i < ohe_size ; i++)
    hot[i] = zero_block ;
  if (party == ALICE)
    hot[0] = set_bit(hot[0], 0) ; 

  /************************* Preparing metadata for GMT to OHE conversion  *************************/

  // Declare variables
  block *r0 = new block[num_ots] ;
  block *r1 = new block[num_ots] ;
  block *rcv_ot = new block[num_ots] ;
  bool *b = new bool[num_ots] ; prg.random_bool(b, num_ots) ;
  unordered_map<int, vector<vector<int>>> mp ; 		// length of subset -> all subset of that length
	unordered_map<int, int> singleton_gmt ;         // hot index        -> first element of the subset
  unordered_map<int, int> remaining_gmt ;         // hot index        -> hot index of tail
  vector<int> conv_left ;
  vector<int> conv_right1 ;
  vector<vector<int>> conv_right2 ;

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

  /************************* Perform Random OTs *************************/

  if (party == ALICE) {
    ot1->send_rot(r0, r1, num_ots) ;
    ot2->recv_rot(rcv_ot, b, num_ots) ; 
  } else {
    ot1->recv_rot(rcv_ot, b, num_ots) ;
    ot2->send_rot(r0, r1, num_ots) ;
  }
  ot1->io->flush() ;
  ot2->io->flush() ;

  rot_comms = ot1->io->counter - rot_comms ; 
  if (print_comm) cout << "ROT comms : " << rot_comms << " bytes\n" ;
  int64_t corr_comms = ot1->io->counter ;

  /************************* Correct Random OTs *************************/

  // Mask out everything except the least significant bit
  block mask = zero_block ;
  mask = set_bit(mask, 0) ;
  andBlocks_arr(r0, mask, num_ots) ; 
  andBlocks_arr(r1, mask, num_ots) ; 
  andBlocks_arr(rcv_ot, mask, num_ots) ;

  // Set the singleton boolean terms
  for (int i = 0 ; i < n ; i++)
    if (single_bools[i])
      hot[i+1] = set_bit(hot[i+1], 0) ;

  // Declare intermediate variables
  int largest_flat_bits = comb(n, n/2) ;
  int largest_flat_blocks = (largest_flat_bits+127)/128 ;
  block *msg = new block[1] ; block *rcv_msg = new block[1] ;
  block *msgs = new block[largest_flat_blocks] ; 
  block *rcv_msgs = new block[largest_flat_blocks] ;
  bool *msg_bits = new bool[largest_flat_bits] ;
  bool *rcv_bits = new bool[largest_flat_bits] ;

  // Iterate over product terms
  int index = n+1 ;
  for (int r = 2 ; r <= n ; r++) {
    int small_flat_bits = comb(n, r) ;
    int small_flat_bytes = (small_flat_bits+7)/8 ;
    int small_flat_blocks = (small_flat_bits+127)/128 ;
    initialize_blocks(msgs, small_flat_blocks) ; initialize_blocks(rcv_msgs, small_flat_blocks) ;
    initialize_bools(msg_bits, small_flat_bits) ; initialize_bools(rcv_bits, small_flat_bits) ;

    // Flattening
    int index_restore = index ;
    for (int i = 0 ; i < small_flat_bits ; i++, index++) {
      xorBlocks_arr(msg, r0+index-n-1, r1+index-n-1, 1) ; 
      xorBlocks_arr(msg, hot+remaining_gmt[index], 1) ;
      copyBits(msgs+i/128, i%128, msg, 0, 1) ;
      msg_bits[i] = single_bools[singleton_gmt[index]] ^ b[index-n-1] ;
    }

    // Send and Recv correction messages
    if (party == ALICE) {
      ot1->io->send_data(msgs, small_flat_bytes) ;
      ot2->io->recv_data(rcv_msgs, small_flat_bytes) ;
    } else {
      ot1->io->recv_data(rcv_msgs, small_flat_bytes) ;
      ot2->io->send_data(msgs, small_flat_bytes) ;
    }
    ot1->io->flush() ;
    ot2->io->flush() ;

    // Send and Recv correction bits
    if (party == ALICE) {
      ot1->io->send_bool(msg_bits, small_flat_bits) ;
      ot2->io->recv_bool(rcv_bits, small_flat_bits) ;
    } else {
      ot1->io->recv_bool(rcv_bits, small_flat_bits) ;
      ot2->io->send_bool(msg_bits, small_flat_bits) ;
    }
    ot1->io->flush() ;
    ot2->io->flush() ;

    // Unflattening
    index = index_restore ;
    for (int i = 0 ; i < small_flat_bits ; i++, index++) {
      copyBits(rcv_msg, 0, rcv_msgs+i/128, i%128, 1) ;
      copyBlocks_arr(hot+index, rcv_ot+index-n-1, 1) ;
      if (single_bools[singleton_gmt[index]])
        xorBlocks_arr(hot+index, rcv_msg, 1) ;
      if (single_bools[singleton_gmt[index]])
        xorBlocks_arr(hot+index, hot+remaining_gmt[index], 1) ;
      
      if (rcv_bits[i])
        xorBlocks_arr(hot+index, r1+index-n-1, 1) ;
      else
        xorBlocks_arr(hot+index, r0+index-n-1, 1) ;
    }
  }

  // Delete intermediate variables
  delete[] msgs ; delete[] rcv_msgs ; delete[] msg ; delete[] rcv_msg ;
  delete[] msg_bits ; delete[] rcv_bits ; 

  // Print stuff
  corr_comms = ot1->io->counter - corr_comms ;
  if (print_comm) cout << "Corr comms : " << corr_comms << " bytes\n" ;

  /************************* Convert GMT to OHE *************************/
  
  ohe_bool[ohe_size-1] = test_bit(hot[ohe_size-1], 0) ;
  for (int i = 0 ; i < conv_left.size() ; i++) {
    bool the_bool = ohe_bool[conv_right1[i]] ;
    for (int j = 0 ; j < conv_right2[i].size() ; j++)
      the_bool = the_bool ^ test_bit(hot[conv_right2[i][j]], 0) ;
    ohe_bool[conv_left[i]] = the_bool ;
  }
  for (int i = 0 ; i < ohe_size ; i++)
    if (ohe_bool[i]) 
      ohe[i/128] = set_bit(ohe[i/128], i%128) ; 

  /************************* Delete and Return *************************/

  delete[] single_bools ; delete[] b ;
  delete[] r0 ; delete[] r1 ; delete[] rcv_ot ;
  delete[] hot ; delete[] ohe_bool ;
  return ohe ;
}

block** batched_random_ohe(int party, int n, int batch_size, COT<NetIO> *ot1, COT<NetIO> *ot2, bool print_comm=false) {
  /************************* Base Case *************************/

  // Declare
  PRG prg ;
  int num_blocks = n_to_blocks(n) ;
  bool *first_bools = new bool[batch_size] ;
  block **ohes = new block*[batch_size] ;

  // Initialize
  prg.random_bool(first_bools, batch_size) ;
  for (int b = 0 ; b < batch_size ; b++) {
    ohes[b] = new block[num_blocks] ;
    for (int i = 0 ; i < num_blocks ; i++) 
      ohes[b][i] = zero_block ;

    if (party == ALICE) {
        ohes[b][0] = set_bit(ohes[b][0], first_bools[b] ? 1 : 0) ;    
    } else {
      if (first_bools[b]) {
        ohes[b][0] = set_bit(ohes[b][0], 1) ;
        ohes[b][0] = set_bit(ohes[b][0], 0) ;
      }
    }
  }

  // Return for base case
  if (n == 1) {
    if (print_comm) cout << "ROT comms : 0 bytes\n" ;
    if (print_comm) cout << "Corr comms : 0 bytes\n" ;
    delete[] first_bools ;
    return ohes ;
  }

  /************************* Declare and Init *************************/

  uint64_t rot_comms = ot1->io->counter ;
  block *r0s = new block[batch_size*(n-1)] ;
  block *r1s = new block[batch_size*(n-1)] ;
  block *rcv_ots = new block[batch_size*(n-1)] ;
  bool *bs = new bool[batch_size*(n-1)] ;
  prg.random_bool(bs, batch_size*(n-1)) ;

  /************************* Perform Random OTs *************************/

  // Send and Recv
  if (party == ALICE) {
    ot1->send_rot(r0s, r1s, batch_size*(n-1)) ; 
    ot2->recv_rot(rcv_ots, bs, batch_size*(n-1)) ; 
  } else {
    ot1->recv_rot(rcv_ots, bs, batch_size*(n-1)) ;
    ot2->send_rot(r0s, r1s, batch_size*(n-1)) ;
  }
  ot1->io->flush() ;
  ot2->io->flush() ;

  // Print stuff
  rot_comms = ot1->io->counter - rot_comms ;
  if (print_comm) cout << "ROT comms : " << rot_comms << " bytes\n" ;
  int64_t corr_comms = ot1->io->counter ;

  /************************* Correct Random OTs *************************/

  // Declare intermediate variables
  int largest_block = n_to_blocks(n-1) ;
  int largest_flat_blocks = ((1 << (n-1)) * batch_size)/128 ;
  block *corr_flat_send = new block[largest_flat_blocks] ; 
  block *corr_flat_rcv = new block[largest_flat_blocks] ; 
  block **cross_shares = new block*[batch_size] ;
  block **r0_actuals = new block*[batch_size] ;
  block **r1_actuals = new block*[batch_size] ;
  block **rcv_ot_actuals = new block*[batch_size] ;
  block **msgs = new block*[batch_size] ;
  block **rcv_msgs = new block*[batch_size] ;
  for (int b = 0 ; b < batch_size ; b++) {
    cross_shares[b] = new block[largest_block] ;
    r0_actuals[b] = new block[largest_block] ;
    r1_actuals[b] = new block[largest_block] ;
    rcv_ot_actuals[b] = new block[largest_block] ;
    msgs[b] = new block[largest_block] ;
    rcv_msgs[b] = new block[largest_block] ;
  }

  // Iterate over rounds
  for (int r = 2 ; r <= n ; r++) {
    // cout << "Round " << r << " - \n" ;
    // Initialize
    int comm_bits = 1 << (r-1) ;
    int small_blocks = n_to_blocks(r-1) ;
    int flat_bytes = (comm_bits*batch_size)/8 ;      
    int flat_blocks = flat_bytes/16 ;
    initialize_blocks(corr_flat_send, flat_blocks) ;
    initialize_blocks(corr_flat_rcv, flat_blocks) ;

    // Setup r0, r1 and received OTs
    for (int b = 0 ; b < batch_size ; b++) {
      if (r < 8) {
        block mask = zero_block ;
        for (int pos = 0 ; pos < 1 << (r-1) ; pos++)
          mask = set_bit(mask, pos) ;
        andBlocks_arr(r0_actuals[b], r0s+b*(n-1)+r-2, &mask, 1) ; 
        andBlocks_arr(r1_actuals[b], r1s+b*(n-1)+r-2, &mask, 1) ; 
        andBlocks_arr(rcv_ot_actuals[b], rcv_ots+b*(n-1)+r-2, &mask, 1) ;
      } else if (r == 8) {
        r0_actuals[b][0] = *(r0s+b*(n-1)+r-2) ;
        r1_actuals[b][0] = *(r1s+b*(n-1)+r-2) ;
        rcv_ot_actuals[b][0] = *(rcv_ots+b*(n-1)+r-2) ;
      } else {
        PRG prg0(r0s+b*(n-1)+r-2) ; PRG prg1(r1s+b*(n-1)+r-2) ; PRG prg_rcv(rcv_ots+b*(n-1)+r-2) ;
        prg0.random_block(r0_actuals[b], small_blocks) ;
        prg1.random_block(r1_actuals[b], small_blocks) ;
        prg_rcv.random_block(rcv_ot_actuals[b], small_blocks) ;
      }
    }

    /**** Send correction bits ****/

    // Flatten
    for (int b = 0 ; b < batch_size ; b++) {
      xorBlocks_arr(msgs[b], r0_actuals[b], r1_actuals[b], small_blocks) ;
      xorBlocks_arr(msgs[b], ohes[b], small_blocks) ;
      if (r < 8)
        copyBits(corr_flat_send + (b*comm_bits)/128, (b*comm_bits) % 128, msgs[b], 0, comm_bits) ;
      else
        copyBlocks_arr(corr_flat_send + small_blocks*b, msgs[b], small_blocks) ;
    }

    // Send and Recv
    if (party == ALICE) {
      ot1->io->send_data(corr_flat_send, flat_bytes) ;
      ot2->io->recv_data(corr_flat_rcv, flat_bytes) ;
    } else {
      ot1->io->recv_data(corr_flat_rcv, flat_bytes) ;
      ot2->io->send_data(corr_flat_send, flat_bytes) ;
    }
    ot1->io->flush() ;
    ot2->io->flush() ;

    // Unflatten
    for (int b = 0 ; b < batch_size ; b++) {
      if (r < 8)
        copyBits(rcv_msgs[b], 0, corr_flat_rcv + (b*comm_bits)/128, (b*comm_bits % 128), comm_bits) ;
      else
        copyBlocks_arr(rcv_msgs[b], corr_flat_rcv + small_blocks*b, small_blocks) ;
    }

    /******************************/

    for (int b = 0 ; b < batch_size ; b++) {
      // Cross share
      copyBlocks_arr(cross_shares[b], rcv_ot_actuals[b], small_blocks) ;
      if (bs[b*(n-1)+r-2])
        xorBlocks_arr(cross_shares[b], rcv_msgs[b], small_blocks) ;
      if (bs[b*(n-1)+r-2])
        xorBlocks_arr(cross_shares[b], ohes[b], small_blocks) ;
      xorBlocks_arr(cross_shares[b], r0_actuals[b], small_blocks) ;

      // Manipulate cross share
      if (r < 7) {
        block tmp = cross_shares[b][0] ;
        cross_shares[b][0] = cross_shares[b][0] << (1 << (r-1)) ;
        xorBlocks_arr(cross_shares[b], &tmp, 1) ;
      } else if (r == 7) {
        uint64_t* data = (uint64_t*)cross_shares[b] ;
        data[1] = data[0] ;
      } else {
        copyBlocks_arr(ohes[b] + small_blocks, cross_shares[b], small_blocks) ;
      }

      // OHE for next iteration
      xorBlocks_arr(ohes[b], cross_shares[b], small_blocks) ;
    }
  }

  // Delete intermediate variables
  delete[] corr_flat_send ;
  delete[] corr_flat_rcv ;
  for (int b = 0 ; b < batch_size ; b++) {
    delete[] cross_shares[b] ;
    delete[] r0_actuals[b] ;
    delete[] r1_actuals[b] ;
    delete[] rcv_ot_actuals[b] ;
    delete[] msgs[b] ;
    delete[] rcv_msgs[b] ;
  }
  delete[] cross_shares ;
  delete[] r0_actuals ; delete[] r1_actuals ; delete[] rcv_ot_actuals ;
  delete[] msgs ; delete[] rcv_msgs ;

  // Print stuff
  corr_comms = ot1->io->counter - corr_comms ;
  if (print_comm) cout << "Corr comms : " << corr_comms << " bytes\n" ;
  
  /************************* Delete and Return *************************/

  delete[] first_bools ; delete[] bs ;
  delete[] r0s ; delete[] r1s ; delete[] rcv_ots ;
  return ohes ;
}

block** batched_random_gmt(int party, int n, int batch_size, COT<NetIO> *ot1, COT<NetIO> *ot2, bool print_comm=false) {
  /************************* Base Case *************************/

  // Declare
  PRG prg ;
  int num_blocks = n_to_blocks(n) ;
  block **ohes = new block*[batch_size] ;
  bool **single_bools = new bool*[batch_size] ;

  // Initialize
  for (int b = 0 ; b < batch_size ; b++) {
    ohes[b] = new block[num_blocks] ;
    for (int i = 0 ; i < num_blocks ; i++)
      ohes[b][i] = zero_block ;
    single_bools[b] = new bool[n] ;
    prg.random_bool(single_bools[b], n) ;
  }

  // Handle Base Case
  if (n == 1) {
    for (int b = 0 ; b < batch_size ; b++) {
      if (party == ALICE) {
        ohes[b][0] = set_bit(ohes[b][0], single_bools[b][0] ? 1 : 0) ;    
      } else {
        if (single_bools[0]) {
          ohes[b][0] = set_bit(ohes[b][0], 1) ;
          ohes[b][0] = set_bit(ohes[b][0], 0) ;
        }
      }
    }

    if (print_comm) cout << "ROT comms : 0 bytes\n" ;
    if (print_comm) cout << "Corr comms : 0 bytes\n" ;
    delete[] single_bools ;
    return ohes ;
  }

  /************************* Declare and Init *************************/

  // Declare
  uint64_t rot_comms = ot1->io->counter ;
  int ohe_size = 1 << n ; 
  int num_ots = ohe_size - n - 1 ;
  bool *ohe_bool = new bool[ohe_size] ;    // This is a temporary storage variable and does not need to replicated per batch
  block **hots = new block*[batch_size] ;
  block *r0s = new block[batch_size*num_ots] ;
  block *r1s = new block[batch_size*num_ots] ;
  block *rcv_ots = new block[batch_size*num_ots] ;
  bool *bs = new bool[batch_size*num_ots] ;

  // Initialize
  initialize_bools(ohe_bool, ohe_size) ;
  for (int b = 0 ; b < batch_size ; b++) {
    hots[b] = new block[ohe_size] ;
    for (int i = 0 ; i < ohe_size ; i++)
      hots[b][i] = zero_block ;
    if (party == ALICE)
      hots[b][0] = set_bit(hots[b][0], 0) ;
  }    
  prg.random_bool(bs, batch_size*num_ots) ;

  /************************* Prepare metadata for GMT to OHE conversion  *************************/

  // Declare metadata variables
  unordered_map<int, vector<vector<int>>> mp ;    // length of subset -> all subset of that length
  unordered_map<int, int> singleton_gmt ;         // hot index        -> first element of the subset
  unordered_map<int, int> remaining_gmt ;         // hot index        -> hot index of tail
  vector<int> conv_left ;
  vector<int> conv_right1 ;
  vector<vector<int>> conv_right2 ;

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

  /************************* Perform Random OTs  *************************/

  if (party == ALICE) {
    ot1->send_rot(r0s, r1s, batch_size*num_ots) ;
    ot2->recv_rot(rcv_ots, bs, batch_size*num_ots) ; 
  } else {
    ot1->recv_rot(rcv_ots, bs, batch_size*num_ots) ;
    ot2->send_rot(r0s, r1s, batch_size*num_ots) ;
  }
  ot1->io->flush() ;
  ot2->io->flush() ;

  rot_comms = ot1->io->counter - rot_comms ;
  if (print_comm) cout << "ROT comms : " << rot_comms << " bytes\n" ;
  int64_t corr_comms = ot1->io->counter ;

  /************************* Correct Random OTs *************************/

  block mask = zero_block ;
  mask = set_bit(mask, 0) ;
  andBlocks_arr(r0s, mask, batch_size*num_ots) ; 
  andBlocks_arr(r1s, mask, batch_size*num_ots) ; 
  andBlocks_arr(rcv_ots, mask, batch_size*num_ots) ;

  // Set the singleton boolean terms
  for (int b = 0 ; b < batch_size ; b++) {
    for (int i = 0 ; i < n ; i++) {
      if (single_bools[b][i])
        hots[b][i+1] = set_bit(hots[b][i+1], 0) ;
    }
  }

  // Iterate over all product terms
  int largest_flat_bits = comb(n, n/2) ;
  int largest_flat_blocks = (batch_size*largest_flat_bits+127)/128 ;
  block *msg = new block[1] ; block *rcv_msg = new block[1] ;
  block *msgs = new block[largest_flat_blocks] ; 
  block *rcv_msgs = new block[largest_flat_blocks] ;
  bool *msg_bits = new bool[batch_size*largest_flat_bits] ;
  bool *rcv_bits = new bool[batch_size*largest_flat_bits] ;

  // Iterate over product terms
  int index = n+1 ;
  for (int r = 2 ; r <= n ; r++) {
    int small_flat_bits = comb(n, r) ;
    int small_flat_bytes = (batch_size*small_flat_bits+7)/8 ;
    int small_flat_blocks = (batch_size*small_flat_bits+127)/128 ;
    initialize_blocks(msgs, small_flat_blocks) ; initialize_blocks(rcv_msgs, small_flat_blocks) ;
    initialize_bools(msg_bits, batch_size*small_flat_bits) ; initialize_bools(rcv_bits, batch_size*small_flat_bits) ;

    // Flattening
    int index_restore = index ;
    for (int b = 0 ; b < batch_size ; b++) {
      for (int i = 0 ; i < small_flat_bits ; i++, index++) {
        xorBlocks_arr(msg, r0s+b*num_ots+index-n-1, r1s+b*num_ots+index-n-1, 1) ; 
        xorBlocks_arr(msg, hots[b]+remaining_gmt[index], 1) ;
        copyBits(msgs+(b*small_flat_bits+i)/128, (b*small_flat_bits+i)%128, msg, 0, 1) ;
        msg_bits[b*small_flat_bits+i] = single_bools[b][singleton_gmt[index]] ^ bs[b*num_ots+index-n-1] ;
      } 
      index = index_restore ;
    }

    // Send and Recv correction messages
    if (party == ALICE) {
      ot1->io->send_data(msgs, small_flat_bytes) ;
      ot2->io->recv_data(rcv_msgs, small_flat_bytes) ;
    } else {
      ot1->io->recv_data(rcv_msgs, small_flat_bytes) ;
      ot2->io->send_data(msgs, small_flat_bytes) ;
    }
    ot1->io->flush() ;
    ot2->io->flush() ;

    // Send and Recv correction bits
    if (party == ALICE) {
      ot1->io->send_bool(msg_bits, batch_size*small_flat_bits) ;
      ot2->io->recv_bool(rcv_bits, batch_size*small_flat_bits) ;
    } else {
      ot1->io->recv_bool(rcv_bits, batch_size*small_flat_bits) ;
      ot2->io->send_bool(msg_bits, batch_size*small_flat_bits) ;
    }
    ot1->io->flush() ;
    ot2->io->flush() ;

    // Unflattening
    for (int b = 0 ; b < batch_size ; b++) {
      index = index_restore ;
      for (int i = 0 ; i < small_flat_bits ; i++, index++) {
        copyBits(rcv_msg, 0, rcv_msgs+(b*small_flat_bits+i)/128, (b*small_flat_bits+i)%128, 1) ;
        copyBlocks_arr(hots[b]+index, rcv_ots+b*num_ots+index-n-1, 1) ;
        if (single_bools[b][singleton_gmt[index]])
          xorBlocks_arr(hots[b]+index, rcv_msg, 1) ;
        if (single_bools[b][singleton_gmt[index]])
          xorBlocks_arr(hots[b]+index, hots[b]+remaining_gmt[index], 1) ;
        
        if (rcv_bits[b*small_flat_bits+i])
          xorBlocks_arr(hots[b]+index, r1s+b*num_ots+index-n-1, 1) ;
        else
          xorBlocks_arr(hots[b]+index, r0s+b*num_ots+index-n-1, 1) ;
      }
    }
  }


  // Delete intermediate variables
  delete[] msgs ; delete[] rcv_msgs ; delete[] msg ; delete[] rcv_msg ;
  delete[] msg_bits ; delete[] rcv_bits ; 

  // Print stuff
  corr_comms = ot1->io->counter - corr_comms ;
  if (print_comm) cout << "Corr comms : " << corr_comms << " bytes\n" ;

  /************************* Convert GMT to OHE *************************/

  for (int b = 0 ; b < batch_size ; b++) {
    ohe_bool[ohe_size-1] = test_bit(hots[b][ohe_size-1], 0) ;
    for (int i = 0 ; i < conv_left.size() ; i++) {
      bool the_bool = ohe_bool[conv_right1[i]] ;
      for (int j = 0 ; j < conv_right2[i].size() ; j++)
        the_bool = the_bool ^ test_bit(hots[b][conv_right2[i][j]], 0) ;
      ohe_bool[conv_left[i]] = the_bool ;
    }

    for (int i = 0 ; i < ohe_size ; i++)
      if (ohe_bool[i]) 
        ohes[b][i/128] = set_bit(ohes[b][i/128], i%128) ; 
    initialize_bools(ohe_bool, ohe_size) ;
  }

  /************************* Delete and Return *************************/

  delete[] ohe_bool ;
  for (int b = 0 ; b < batch_size ; b++) {
    delete[] hots[b] ;
    delete[] single_bools[b] ;
  }
  delete[] hots ;
  delete[] single_bools ; delete[] bs ;
  delete[] r0s ; delete[] r1s ; delete[] rcv_ots ;
  
  return ohes ;
}

int main(int argc, char** argv) {
  /************************* Parse Input *************************/
  
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " <party-id> <port> <n> <iknp/ferret> <prot_type> <batched> <verify> \n";
    exit(EXIT_FAILURE);
   };

  int64_t comm_var = 0 ;
  int party, port, n ;
  string ot_type, prot_type ;
  bool verify, batched ;
  NetIO *io ;
  COT<NetIO> *ot1, *ot2 ;

  if (argc != 8)
    abort();
  
  party = atoi(argv[1]) ;
  port = atoi(argv[2]) ;
  n = atoi(argv[3]) ;
  ot_type = argv[4] ;
  prot_type = argv[5] ;
  batched = atoi(argv[6]) ;
  verify = atoi(argv[7]) ;
  
  /************************* Create OT *************************/

  io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port) ;
  comm_var = io->counter ;
  if (ot_type == "iknp") {
    ot1 = new IKNP<NetIO>(io) ;
    ot2 = new IKNP<NetIO>(io) ;
  } 
  else if (ot_type == "ferret") {
    ot1 = new FerretCOT<NetIO>(party, threads, &io) ;
    ot2 = new FerretCOT<NetIO>(party == 1 ? 2 : 1, threads, &io) ;
  }
  else {
    cout << "Incorrect OT type\n" ;
    exit(EXIT_FAILURE) ;
  }

  /************************* Experiment *************************/

  if (batched) {
    int batch_size = 128 ;
    auto start_exp = clock_start(); 
    // Declare and initialize
    int num_blocks = n_to_blocks(n) ;
    block **ohes ;
    block *rec ;
    
    // Get OHEs
    if (prot_type == "ohe")
      ohes = batched_random_ohe(party, n, batch_size, ot1, ot2, true) ;
    else
      ohes = batched_random_gmt(party, n, batch_size, ot1, ot2, true) ;
    comm_var = io->counter - comm_var ;

    // Verify per batch
    if (verify) {
      int correct = 0 ;    
      for (int b = 0 ; b < batch_size ; b++) {
        rec = reconst_ohe(party, n, ot1, ot2, ohes[b], false) ;
        int no_set = 0 ;
        for (int i = 0 ; i < num_blocks ; i++) {
          uint64_t *dat = (uint64_t*)(rec+i) ;
          for (int pos = 0 ; pos < 64 ; pos++) {
            if (dat[0] & (1L << pos))
              no_set++ ;
            if (dat[1] & (1L << pos))
              no_set++ ;
          }
        }
        if (no_set == 1)
          correct++ ;

        delete[] rec ;
      }

      // Print things
      if (correct == batch_size)
        cout << "\033[1;32m" << "Passed" << "\033[0m\n" ;
      else 
        cout << "\033[1;31m" << "Failed " << " --> " << correct << "\033[0m\n" ;
    }

    // Delete
    for (int b = 0 ; b < batch_size ; b++)
      delete[] ohes[b] ;
    delete[] ohes ;

    setprecision(5) ;
    long long t_exp = time_from(start_exp);    

    // Print things
    cout << fixed << setprecision(5) << "Time taken : " << double(t_exp)/(1e3*batch_size) << " ms\n" ;
    // cout << fixed << setprecision(2) << "OT comms : " << ot_comms/128 << " bytes\n" ;    
  }
  else {
    block *ohe ;
    if (verify) {
      block *rec ;
      int correct = 0 ;
      int num_blocks = n_to_blocks(n) ;
      int no_tests = min(100, 10 * (1 << n)) ;
      for (int counter = 0 ; counter < no_tests ; counter++) {
        if (prot_type == "ohe")
          ohe = random_ohe(party, n, ot1, ot2) ;
        else
          ohe = random_gmt(party, n, ot1, ot2) ;

        rec = reconst_ohe(party, n, ot1, ot2, ohe, false) ;
        int no_set = 0 ;
        for (int i = 0 ; i < num_blocks ; i++) {
          uint64_t *dat = (uint64_t*)(rec+i) ;
          for (int pos = 0 ; pos < 64 ; pos++) {
            if (dat[0] & (1L << pos))
              no_set++ ;
            if (dat[1] & (1L << pos))
              no_set++ ;
          }
        }
        if (no_set == 1)
          correct++ ;

        delete[] ohe ; delete[] rec ;
      }

      // Print things
      if (correct == no_tests)
        cout << "\033[1;32m" << "Passed" << "\033[0m\n" ;
      else {
        cout << "\033[1;31m" << "Failed --> " << correct << "\033[0m\n" ;
        return 1 ;
      }
    } else {
      /************************* Dummy stuff *************************/

      // block *b1 = new block[100] ;
      // block *b2 = new block[100] ;
      // block *recv = new block[100] ;
      // bool *choices = new bool[100] ;

      // // Measure
      // auto start_dummy = clock_start(); 
      // if (party == ALICE) {
      //   ot1->send_rot(b1, b2, 100) ;
      //   ot2->recv_rot(recv, choices, 100) ;
      // } else {
      //   ot1->recv_rot(recv, choices, 100) ;
      //   ot2->send_rot(b1, b2, 100) ;
      // }
      // long long t_dummy = time_from(start_dummy) ;
      // int64_t dummy_comms = ot1->io->counter - comm_var ; comm_var = ot1->io->counter ;

      // // Print things
      // cout << "Dummy comms : " << dummy_comms << " bytes\n" ;
      // cout << "Dummy time : " << double(t_dummy)/1e3 << " ms\n" ;

      /************************* Real stuff *************************/

      // Measure
      auto start_exp = clock_start(); 
      int reps = 1 ;
      for (int i = 0 ; i < reps ; i++) {
        if (prot_type == "ohe")
          ohe = random_ohe(party, n, ot1, ot2, true) ;
        else
          ohe = random_gmt(party, n, ot1, ot2, true) ;
      }
      int64_t ot_comms ; 
      ot_comms = ot1->io->counter - comm_var ; 
      setprecision(5) ;
      long long t_exp = time_from(start_exp);    

      // Print things
      cout << fixed << setprecision(5) << "Time taken : " << double(t_exp)/(1e3*reps) << " ms\n" ;
      cout << fixed << setprecision(5) << "OT comms : " << ot_comms/reps << " bytes\n" ;    

      // Delete
      delete[] ohe ; 
    }
  }

  return 0 ;
}