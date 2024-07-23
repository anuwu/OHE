  #include "emp-ot/emp-ot.h"
#include "test_util.h"
#include <iostream>
#include <time.h>

using namespace std ;
using namespace emp ;

constexpr int threads = 1 ;

block* reconst_ohe(int party, int n, COT<NetIO> *ot1, COT<NetIO> *ot2, block *ohe, bool print=false) {
    int num_blocks = n_to_blocks(n) ;
    block *rcv = new block[num_blocks] ;
    block *res = new block[num_blocks] ;
    int comm_bytes = n_to_bytes(n) ;
    if (party == ALICE) {
      ot1->io->send_data(ohe, comm_bytes) ; 
      ot2->io->recv_data(rcv, comm_bytes) ; 
    } else {
      ot1->io->recv_data(rcv, comm_bytes) ; 
      ot2->io->send_data(ohe, comm_bytes) ; 
    }
    ot1->io->flush() ;
    ot2->io->flush() ;
    xorBlocks_arr(res, rcv, ohe, num_blocks) ;
    if (print) {
      cout << "Received -\n" ;
      for (int i = 0 ; i < num_blocks ; i++)
        cout << rcv[i] << "\n" ;

      cout << "Result -\n" ;
      for (int i = 0 ; i < num_blocks ; i++)
        cout << res[i] << "\n" ;
    }
    delete[] rcv ; 
    return res ;
}

block* random_ohe(int party, int n, COT<NetIO> *ot1, COT<NetIO> *ot2) {
  /************************* Declare and Init *************************/

  int num_blocks = n_to_blocks(n) ;
  PRG prg ;
  block *r0 = new block[n-1] ;
  block *r1 = new block[n-1] ;
  block *rcv_ot = new block[n-1] ;
  bool *b = new bool[n] ; prg.random_bool(b, n) ;
  block *ohe = new block[num_blocks] ;
  for (int i = 0 ; i < num_blocks ; i++) 
    ohe[i] = zero_block ;
  
  /************************* First Bit *************************/

  if (party == ALICE) {
    ohe[0] = set_bit(ohe[0], b[0] ? 1 : 0) ;    
  } else {
    if (b[0]) {
      ohe[0] = set_bit(ohe[0], 1) ;
      ohe[0] = set_bit(ohe[0], 0) ;
    }
  }

  /************************* Perform Random OTs *************************/

  if (party == ALICE) {
    ot1->send_rot(r0, r1, n-1) ;
    ot2->recv_rot(rcv_ot, b+1, n-1) ; 
  } else {
    ot1->recv_rot(rcv_ot, b+1, n-1) ;
    ot2->send_rot(r0, r1, n-1) ;
  }
  ot1->io->flush() ;
  ot2->io->flush() ;

  for (int r = 2 ; r <= n ; r++) {
    // Declare and initialize
    int comm_bytes = n_to_bytes(r-1) ;
    int small_blocks = n_to_blocks(r-1) ;
    block *msg = new block[small_blocks] ;
    block *rcv_msg = new block[small_blocks] ;
    block *cross_share = new block[small_blocks] ;
    block *r0_actual = new block[small_blocks] ;
    block *r1_actual = new block[small_blocks] ;
    block *rcv_ot_actual = new block[small_blocks] ;

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

    delete[] msg ; delete[] rcv_msg ; delete[] cross_share ;
    delete[] r0_actual ; delete[] r1_actual ; delete[] rcv_ot_actual ;
  }

  delete[] b ;
  delete[] r0 ; delete[] r1 ; delete[] rcv_ot ;
  return ohe ;
}


block* random_gmt(int party, int n, COT<NetIO> *ot1, COT<NetIO> *ot2) {
  return NULL ;
}

int main(int argc, char** argv) {
  /************************* Parse Input *************************/
  
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " <party-id> <port> <n> <iknp/ferret> <verify> \n";
    exit(EXIT_FAILURE);
   };

  int party, port, n ;
  string ot_type ;
  bool verify ;
  NetIO *io ;
  COT<NetIO> *ot1, *ot2 ;

  if (argc != 6)
    abort();
  
  party = atoi(argv[1]) ;
  port = atoi(argv[2]) ;
  n = atoi(argv[3]) ;
  ot_type = argv[4] ;
  verify = atoi(argv[5]) ;
  
  /************************* Create OT *************************/

  io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port) ;
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

  block *ohe, *rec ;
  int num_blocks = n_to_blocks(n) ;
  int no_tests = min(1000, 10 * (1 << n)) ;
  if (verify) {
    int counter = 0 ;
    do {
      ohe = random_ohe(party, n, ot1, ot2) ;
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
      if (no_set != 1)
        break ;
        
      delete[] ohe ; delete[] rec ;
      counter++ ;
    } while (counter < no_tests) ;

    if (counter == no_tests)
      cout << "\033[1;32m" << "Passed" << "\033[0m\n" ;
    else {
      cout << "\033[1;31m" << "Failed" << "\033[0m\n" ;
      return 1 ;
    }
  } else {
    ohe = random_ohe(party, n, ot1, ot2) ;
    rec = reconst_ohe(party, n, ot1, ot2, ohe, false) ;
    cout << "Reconstructed OHE - \n" ;
    for (int i = 0 ; i < num_blocks ; i++)
      cout << rec[i] << "\n" ;
    delete[] ohe ; delete[] rec ;
  }

  return 0 ;
}