#include <iostream>
#include "utils.h"
#include "ohe.h"

using namespace std ;
using namespace emp ;

void get_ohe_from_plain(block *inp, block *ohe) {
  SET_BIT(ohe, ((uint64_t*)inp)[0]) ;
}

void random_ohe(int party, int n, COT<NetIO> *ot1, COT<NetIO> *ot2, block *alpha, block *ohe, bool measure) {
  // Start measurement counters
  long long running_time ;
  uint64_t running_comms = ot1->io->counter ;

  /************************* Base Case *************************/

  // Declare
  PRG prg ;
  int num_blocks = get_ohe_blocks(n) ;
  bool *b = new bool[n] ; 

  // Initialize
  prg.random_bool(b, n) ;
  for (int i = 0 ; i < n ; i++)
    if (b[i])
      SET_BIT(alpha, i) ;
  for (int i = 0 ; i < num_blocks ; i++) 
    ohe[i] = zero_block ;
  if (party == ALICE) {
      SET_BIT(ohe, b[0] ? 1 : 0) ;    
    } else {
      if (b[0]) {
        SET_BIT(ohe, 0) ;
        SET_BIT(ohe, 1) ;
      }
    }

  // Handle base case
  if (n == 1) {
    delete[] b ;
    if (measure) {
      cout << "ROT time : 0 ms\n" ;
      cout << "ROT comms : 0 bytes\n" ;
      cout << "Corr time : 0 ms\n" ;
      cout << "Corr comms : 0 bytes\n" ;
    }

    return ;
  }

  /************************* Declare *************************/

  block *r0 = new block[n-1] ;
  block *r1 = new block[n-1] ;
  block *rcv_ot = new block[n-1] ;

  /************************* Perform Random OTs *************************/

  // Send and Recv
  auto start_exp = clock_start() ;
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
  running_time = time_from(start_exp) ;
  if (measure) {
    cout << fixed << setprecision(MEASUREMENT_PRECISION) << "ROT time : " << running_time/1e3 << " ms\n" ;
    cout << "ROT comms : " << (ot1->io->counter - running_comms) << " bytes\n" ;
  } 
  running_comms = ot1->io->counter ;
  start_exp = clock_start() ;

  /************************* Correct Random OTs *************************/

  // Declare intermediate variables
  int largest_block = get_ohe_blocks(n-1) ;
  block *msg = new block[largest_block] ;
  block *rcv_msg = new block[largest_block] ;
  block *cross_share = new block[largest_block] ;
  block *r0_actual = new block[largest_block] ;
  block *r1_actual = new block[largest_block] ;
  block *rcv_ot_actual = new block[largest_block] ;

  // Iterate over rounds
  for (int r = 2 ; r <= n ; r++) {
    // Initialize
    int comm_bytes = get_ohe_bytes(r-1) ;
    int small_blocks = get_ohe_blocks(r-1) ;
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

  // End measurement for corr
  running_time = time_from(start_exp) ;

  // Delete intermediate variables
  delete[] msg ; delete[] rcv_msg ; delete[] cross_share ;
  delete[] r0_actual ; delete[] r1_actual ; delete[] rcv_ot_actual ;

  /************************* Exit *************************/

  // Delete
  delete[] b ;
  delete[] r0 ; delete[] r1 ; delete[] rcv_ot ;
  
  // Print stuff
  if (measure) {
    cout << fixed << setprecision(MEASUREMENT_PRECISION) << "Corr time : " << running_time/1e3 << " ms\n" ;
    cout << "Corr comms : " << (ot1->io->counter - running_comms) << " bytes\n" ;
  } 
}

void random_gmt(int party, int n, COT<NetIO> *ot1, COT<NetIO> *ot2, block *alpha, block *ohe, bool measure) {
  // Start measurement counters
  long long running_time ;
  uint64_t running_comms = ot1->io->counter ;

  /************************* Base Case *************************/

  // Declare
  PRG prg ;
  int num_blocks = get_ohe_blocks(n) ;
  bool *single_bools = new bool[n] ;

  // Initialize
  prg.random_bool(single_bools, n) ;
  for (int i = 0 ; i < n ; i++)
    if (single_bools[i])
      SET_BIT(alpha, i) ;

  // Handle base case
  if (n == 1) {
    if (party == ALICE)
      SET_BIT(ohe, single_bools[0] ? 1 : 0) ;
    else {
      if (single_bools[0]) {
        SET_BIT(ohe, 0) ;
        SET_BIT(ohe, 1) ;
      }
    }
    
    delete[] single_bools ;
    if (measure) {
      cout << "ROT time : 0 ms\n" ;
      cout << "ROT comms : 0 bytes\n" ;
      cout << "Corr time : 0 ms\n" ;
      cout << "Corr comms : 0 bytes\n" ;
    }

    return ;
  }

  /************************* Declare and Init *************************/

  // Declare
  int ohe_size = 1 << n ; int num_ots = ohe_size - n - 1 ;
  block *hot = new block[num_blocks] ;
  block *r0 = new block[num_ots] ;
  block *r1 = new block[num_ots] ;
  block *rcv_ot = new block[num_ots] ;
  bool *b = new bool[num_ots] ; prg.random_bool(b, num_ots) ;

  // Initialize
  initialize_blocks(hot, num_blocks) ;
  if (party == ALICE)
    SET_BIT(hot, 0) ;

  /************************* Preparing metadata for GMT to OHE conversion  *************************/

  // Declare variables
  unordered_map<int, vector<vector<int>>> mp ; 		// length of subset -> all subset of that length
	unordered_map<int, int> singleton_gmt ;         // hot index        -> first element of the subset
  unordered_map<int, int> remaining_gmt ;         // hot index        -> hot index of tail
  vector<int> conv_left ;
  vector<int> conv_right1 ;
  vector<vector<int>> conv_right2 ;

  get_conv_metadata(n, mp, singleton_gmt, remaining_gmt, conv_left, conv_right1, conv_right2) ;

  /************************* Perform Random OTs *************************/

  // Send and recv
  auto start_exp = clock_start() ;
  if (party == ALICE) {
    ot1->send_rot(r0, r1, num_ots) ;
    ot2->recv_rot(rcv_ot, b, num_ots) ; 
  } else {
    ot1->recv_rot(rcv_ot, b, num_ots) ;
    ot2->send_rot(r0, r1, num_ots) ;
  }
  ot1->io->flush() ;
  ot2->io->flush() ;

  // Print stuff
  running_time = time_from(start_exp) ;
  if (measure) {
    cout << fixed << setprecision(MEASUREMENT_PRECISION) << "ROT time : " << running_time/1e3 << " ms\n" ;
    cout << "ROT comms : " << (ot1->io->counter - running_comms) << " bytes\n" ;
  } 
  running_comms = ot1->io->counter ;
  start_exp = clock_start() ;

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
      SET_BIT(hot, i+1) ;

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
      bool the_bool = test_bit(r0[index-n-1], 0) ^ test_bit(r1[index-n-1], 0) ;
      the_bool = the_bool ^ TEST_BIT(hot, remaining_gmt[index]) ;
      if (the_bool)
        SET_BIT(msgs, i) ;
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
      bool the_bool = test_bit(rcv_ot[index-n-1], 0) ;
      if (single_bools[singleton_gmt[index]]) {
        the_bool = the_bool ^ TEST_BIT(rcv_msgs, i) ;
        the_bool = the_bool ^ TEST_BIT(hot, remaining_gmt[index]) ;
      }
      if (rcv_bits[i])
        the_bool = the_bool ^ test_bit(r1[index-n-1], 0) ;
      else
        the_bool = the_bool ^ test_bit(r0[index-n-1], 0) ;
      if (the_bool)
        SET_BIT(hot, index) ;
    }
  }

  // End measurement for corr
  running_time = time_from(start_exp) ;

  // Delete intermediate variables
  delete[] msgs ; delete[] rcv_msgs ; delete[] msg ; delete[] rcv_msg ;
  delete[] msg_bits ; delete[] rcv_bits ; 

  /************************* Convert GMT to OHE *************************/
  
  copyBits(ohe+(ohe_size-1)/128, (ohe_size-1)%128, hot+(ohe_size-1)/128, (ohe_size-1)%128, 1) ;
  for (int i = 0 ; i < conv_left.size() ; i++) {
    bool the_bool = TEST_BIT(ohe, conv_right1[i]) ;
    for (int j = 0 ; j < conv_right2[i].size() ; j++)
      the_bool = the_bool ^ TEST_BIT(hot, conv_right2[i][j]) ;
    if (the_bool)
      SET_BIT(ohe, conv_left[i]) ;
  }

  /************************* Exit *************************/

  // Delete
  delete[] single_bools ; delete[] b ;
  delete[] r0 ; delete[] r1 ; delete[] rcv_ot ;
  delete[] hot ;

  // Print stuff
  if (measure) {
    cout << fixed << setprecision(MEASUREMENT_PRECISION) << "Corr time : " << running_time/1e3 << " ms\n" ;
    cout << "Corr comms : " << (ot1->io->counter - running_comms) << " bytes\n" ;
  }
}

void batched_random_ohe(int party, int n, int batch_size, COT<NetIO> *ot1, COT<NetIO> *ot2, block **alphas, block **ohes, bool measure) {
  // Start measurement counters
  long long running_time ;
  uint64_t running_comms = ot1->io->counter ;

  /************************* Base Case *************************/

  // Declare
  PRG prg ;
  bool *first_bools = new bool[batch_size] ;

  // Initialize
  prg.random_bool(first_bools, batch_size) ;
  for (int b = 0 ; b < batch_size ; b++)
    if(first_bools[b])
      SET_BIT(alphas[b], 0) ;
  for (int b = 0 ; b < batch_size ; b++) {
    if (party == ALICE)
      SET_BIT(ohes[b], first_bools[b] ? 1 : 0) ;
    else {
      if (first_bools[b]) {
        SET_BIT(ohes[b], 0) ;
        SET_BIT(ohes[b], 1) ;
      }
    }
  }

  // Return for base case
  if (n == 1) {
    delete[] first_bools ;
    if (measure) {
      cout << "ROT time : 0 ms\n" ;
      cout << "ROT comms : 0 bytes\n" ;
      cout << "Corr time : 0 ms\n" ;
      cout << "Corr comms : 0 bytes\n" ;
    }

    return ;
  }

  /************************* Declare and Init *************************/

  block *r0s = new block[batch_size*(n-1)] ;
  block *r1s = new block[batch_size*(n-1)] ;
  block *rcv_ots = new block[batch_size*(n-1)] ;
  bool *bs = new bool[batch_size*(n-1)] ;
  prg.random_bool(bs, batch_size*(n-1)) ;
  for (int b = 0 ; b < batch_size ; b++)
    for (int i = 0 ; i < n-1 ; i++)
      if (bs[b*(n-1)+i])
        SET_BIT(alphas[b], i+1) ;

  /************************* Perform Random OTs *************************/

  // Send and Recv
  auto start_exp = clock_start() ;
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
  running_time = time_from(start_exp) ;
  if (measure) {
    cout << fixed << setprecision(MEASUREMENT_PRECISION) << "ROT time : " << running_time/1e3 << " ms\n" ;
    cout << "ROT comms : " << (ot1->io->counter - running_comms) << " bytes\n" ;
  } 
  running_comms = ot1->io->counter ;
  start_exp = clock_start() ;

  /************************* Correct Random OTs *************************/

  // Declare intermediate variables
  int largest_block = get_ohe_blocks(n-1) ;
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
    int small_blocks = get_ohe_blocks(r-1) ;
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

  // End measurement for corr
  running_time = time_from(start_exp) ;

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
  
  /************************* Exit *************************/

  // Delete
  delete[] first_bools ; delete[] bs ;
  delete[] r0s ; delete[] r1s ; delete[] rcv_ots ;

  // Print stuff
  if (measure) {
    cout << fixed << setprecision(MEASUREMENT_PRECISION) << "Corr time : " << running_time/1e3 << " ms\n" ;
    cout << "Corr comms : " << (ot1->io->counter - running_comms) << " bytes\n" ;
  } 
}

void batched_random_gmt(int party, int n, int batch_size, COT<NetIO> *ot1, COT<NetIO> *ot2, block **alphas, block **ohes, bool measure) {
  // Start measurement counters
  long long running_time ;
  uint64_t running_comms = ot1->io->counter ;

  /************************* Base Case *************************/

  // Declare
  PRG prg ;
  int num_blocks = get_ohe_blocks(n) ;
  bool **single_bools = new bool*[batch_size] ;

  // Initialize
  for (int b = 0 ; b < batch_size ; b++) {
    single_bools[b] = new bool[n] ;
    prg.random_bool(single_bools[b], n) ;
    for (int i = 0 ; i < n ; i++)
      if (single_bools[b][i])
        SET_BIT(alphas[b], i) ;
  }

  // Handle Base Case
  if (n == 1) {
    for (int b = 0 ; b < batch_size ; b++) {
      if (party == ALICE)
        SET_BIT(ohes[b], single_bools[b][0] ? 1 : 0) ;
      else {
        if (single_bools[b][0]) {
          SET_BIT(ohes[b], 0) ;
          SET_BIT(ohes[b], 1) ;
        }
      }
    }

    delete[] single_bools ;
    if (measure) {
      cout << "ROT time : 0 ms\n" ;
      cout << "ROT comms : 0 bytes\n" ;
      cout << "Corr time : 0 ms\n" ;
      cout << "Corr comms : 0 bytes\n" ;
    }

    return ;
  }

  /************************* Declare and Init *************************/

  // Declare
  int ohe_size = 1 << n ; 
  int num_ots = ohe_size - n - 1 ;
  block **hots = new block*[batch_size] ;
  block *r0s = new block[batch_size*num_ots] ;
  block *r1s = new block[batch_size*num_ots] ;
  block *rcv_ots = new block[batch_size*num_ots] ;
  bool *bs = new bool[batch_size*num_ots] ;

  // Initialize
  for (int b = 0 ; b < batch_size ; b++) {
    hots[b] = new block[num_blocks] ;
    initialize_blocks(hots[b], num_blocks) ;
    if (party == ALICE)
      SET_BIT(hots[b], 0) ;
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

  get_conv_metadata(n, mp, singleton_gmt, remaining_gmt, conv_left, conv_right1, conv_right2) ;

  /************************* Perform Random OTs  *************************/

  // Send and receive
  auto start_exp = clock_start() ;
  if (party == ALICE) {
    ot1->send_rot(r0s, r1s, batch_size*num_ots) ;
    ot2->recv_rot(rcv_ots, bs, batch_size*num_ots) ; 
  } else {
    ot1->recv_rot(rcv_ots, bs, batch_size*num_ots) ;
    ot2->send_rot(r0s, r1s, batch_size*num_ots) ;
  }
  ot1->io->flush() ;
  ot2->io->flush() ;

  // Print stuff
  running_time = time_from(start_exp) ;
  if (measure) {
    cout << fixed << setprecision(MEASUREMENT_PRECISION) << "ROT time : " << running_time/1e3 << " ms\n" ;
    cout << "ROT comms : " << (ot1->io->counter - running_comms) << " bytes\n" ;
  } 
  running_comms = ot1->io->counter ;
  start_exp = clock_start() ;

  /************************* Correct Random OTs *************************/

  block mask = zero_block ;
  mask = set_bit(mask, 0) ;
  andBlocks_arr(r0s, mask, batch_size*num_ots) ; 
  andBlocks_arr(r1s, mask, batch_size*num_ots) ; 
  andBlocks_arr(rcv_ots, mask, batch_size*num_ots) ;

  // Set the singleton boolean terms
  for (int b = 0 ; b < batch_size ; b++)
    for (int i = 0 ; i < n ; i++)
      if (single_bools[b][i])
        SET_BIT(hots[b], i+1) ;

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
        bool the_bool = test_bit(r0s[b*num_ots+index-n-1], 0) ^ test_bit(r1s[b*num_ots+index-n-1], 0) ;
        the_bool = the_bool ^ TEST_BIT(hots[b], remaining_gmt[index]) ;
        if (the_bool)
          SET_BIT(msgs, b*small_flat_bits + i) ;
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
        bool the_bool = test_bit(rcv_ots[b*num_ots+index-n-1], 0) ;
        if (single_bools[b][singleton_gmt[index]]) {
          the_bool = the_bool ^ TEST_BIT(rcv_msgs, b*small_flat_bits+i) ;
          the_bool = the_bool ^ TEST_BIT(hots[b], remaining_gmt[index]) ;
        }
        if (rcv_bits[b*small_flat_bits+i])
          the_bool = the_bool ^ test_bit(r1s[b*num_ots+index-n-1], 0) ;
        else
          the_bool = the_bool ^ test_bit(r0s[b*num_ots+index-n-1], 0) ;
        if (the_bool)
          SET_BIT(hots[b], index) ;
      }
    }
  }

  // End measurement for corr
  running_time = time_from(start_exp) ;

  // Delete intermediate variables
  delete[] msgs ; delete[] rcv_msgs ; delete[] msg ; delete[] rcv_msg ;
  delete[] msg_bits ; delete[] rcv_bits ; 

  /************************* Convert GMT to OHE *************************/

  for (int b = 0 ; b < batch_size ; b++) {
    copyBits(ohes[b]+(ohe_size-1)/128, (ohe_size-1)%128, hots[b]+(ohe_size-1)/128, (ohe_size-1)%128, 1) ;
    for (int i = 0 ; i < conv_left.size() ; i++) {
      bool the_bool = TEST_BIT(ohes[b], conv_right1[i]) ;
      for (int j = 0 ; j < conv_right2[i].size() ; j++)
        the_bool = the_bool ^ TEST_BIT(hots[b], conv_right2[i][j]) ;
      if (the_bool)
        SET_BIT(ohes[b], conv_left[i]) ;
    }
  }

  /************************* Exit *************************/

  // Delete stuff
  for (int b = 0 ; b < batch_size ; b++)
    delete[] single_bools[b] ;
  delete[] single_bools ;
  for (int b = 0 ; b < batch_size ; b++)
    delete[] hots[b] ;
  delete[] hots ;
  delete[] bs ;
  delete[] r0s ; delete[] r1s ; delete[] rcv_ots ;

  // Print stuff
  if (measure) {
    cout << fixed << setprecision(MEASUREMENT_PRECISION) << "Corr time : " << running_time/1e3 << " ms\n" ;
    cout << "Corr comms : " << (ot1->io->counter - running_comms) << " bytes\n" ;
  }
}