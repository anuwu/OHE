#include "emp-ot/emp-ot.h"
#include "test_util.h"
#include <iostream>
#include <time.h>

using namespace std ;
using namespace emp ;

constexpr int threads = 1 ;

block* reconst_ohe(int party, int n, COT<NetIO> *ot1, COT<NetIO> *ot2, block *ohe, bool print=false) {
    int num_blocks ;
    if (n < 8)
      num_blocks = 1 ;
    else
      num_blocks = 1 << (n - 6) ;

    block *rcv = new block[num_blocks] ;
    block *res = new block[num_blocks] ;
    if (party == ALICE) {
      ot1->io->send_data(ohe, n < 4 ? 1 : (1 << (n-3))) ; 
      ot2->io->recv_data(rcv, n < 4 ? 1 : (1 << (n-3))) ; 

      // ot1->io->send_data(ohe, 32) ; 
      // ot2->io->recv_data(rcv, 32) ; 
    } else {
      ot1->io->recv_data(rcv, n<4 ? 1 : (1 << (n-3))) ; 
      ot2->io->send_data(ohe, n<4 ? 1 : (1 << (n-3))) ; 

      // ot1->io->recv_data(rcv, 32) ;
      // ot2->io->send_data(ohe, 32) ; 
    }
    ot1->io->flush() ;
    ot2->io->flush() ;
    xorBlocks_arr(res, rcv, ohe, num_blocks) ;
    if (print) cout << "Received -->\t" << rcv[0] << "\n" ;
    if (print) cout << "Result -->\t" << res[0] << "\n" ;
    delete[] rcv ; 

    return res ;
}

block* random_ohe(int party, int n, COT<NetIO> *ot1, COT<NetIO> *ot2) {
  int num_blocks ;
  if (n < 8)
    num_blocks = 1 ;
  else
    num_blocks = 1 << (n - 6) ;

  PRG prg ;
  block *ohe = new block[num_blocks] ;
  for (int i = 0 ; i < num_blocks ; i++)  // Initializing OHE
    ohe[i] = zero_block ;

  bool *b = new bool[n] ;
  prg.random_bool(b, n) ;
  block *r0 = new block[n-1] ;
  block *r1 = new block[n-1] ;
  block *rcv_ot = new block[n-1] ;
  
  // Setting initial one-hot vector
  if (party == ALICE) {
    ohe[0] = set_bit(ohe[0], b[0] ? 1 : 0) ;    
  } else if (party == BOB) {
    if (b[0]) {
      ohe[0] = set_bit(ohe[0], 1) ;
      ohe[0] = set_bit(ohe[0], 0) ;
    }
  }
  // // Printing for sanity
  // cout << "\nb[0] -->\t" << b[0] << "\n" ;
  // cout << "ohe[0] -->\t" << ohe[0] << "\n" ;

  // Sending and receiving OTs

  // cout << "r0[0] -->\t" << r0[0] << "\n" ;
  // cout << "r1[0] -->\t" << r1[0] << "\n" ;
  // cout << "b[1] -->\t" << b[1] << "\n" ;
  // cout << "rcv_ot[0] -->\t" << rcv_ot[0] << "\n" ;

  if (party == ALICE) {
    ot1->send_rot(r0, r1, n-1) ;
    ot2->recv_rot(rcv_ot, b+1, n-1) ; 
  } else {
    ot1->recv_rot(rcv_ot, b+1, n-1) ;
    ot2->send_rot(r0, r1, n-1) ;
  }
  ot1->io->flush() ;
  ot2->io->flush() ;

  // cout << "\nr0[0] -->\t" << r0[0] << "\n" ;
  // cout << "r1[0] -->\t" << r1[0] << "\n" ;
  // cout << "b[1] -->\t" << b[1] << "\n" ;
  // cout << "rcv_ot[0] -->\t" << rcv_ot[0] << "\n" ;

  for (int r = 2 ; r <= n ; r++) {
    if (r < 8) {
      block *msg = new block[1] ;
      block *rcv_msg = new block[1] ;
      block *cross_share = new block[1] ;

      block mask = zero_block ;
      for (int pos = 0 ; pos < 1 << (r-1) ; pos++)
        mask = set_bit(mask, pos) ;

      // Masking blocks for ease of debugging
      andBlocks_arr(r0+r-2, &mask, 1) ; andBlocks_arr(r1+r-2, &mask, 1) ; andBlocks_arr(rcv_ot+r-2, &mask, 1) ;

      xorBlocks_arr(msg, r0+r-2, r1+r-2, 1) ; 
      xorBlocks_arr(msg, ohe, 1) ;

      int comm_bytes = r < 5 ? 1 : (1 << (r-4)) ;
      if (party == ALICE) {
        ot1->io->send_data(msg, comm_bytes) ; 
        ot2->io->recv_data(rcv_msg, comm_bytes) ; 
      } else {
        ot1->io->recv_data(rcv_msg, comm_bytes) ;
        ot2->io->send_data(msg, comm_bytes) ; 
      }
      ot1->io->flush() ;
      ot2->io->flush() ;

      copyBlocks_arr(cross_share, rcv_ot+r-2, 1) ;
      if (b[r-1])
        xorBlocks_arr(cross_share, rcv_msg, 1) ;

      if (b[r-1])
        xorBlocks_arr(cross_share, ohe, 1) ;

      xorBlocks_arr(cross_share, r0+r-2, 1) ;

      // First half
      xorBlocks_arr(ohe, cross_share, 1) ;

      // Second half
      if (r < 7)
        cross_share[0] = cross_share[0] << (1 << (r-1)) ;
      else {
        uint64_t* data = (uint64_t*)cross_share ;
        data[1] = data[0] ;
        data[0] = 0L ;
      }
      for (int pos = 0 ; pos < 1 << (r-1) ; pos++)
        cross_share[0] = set_bit(cross_share[0], pos) ;
      for (int pos = 1 << (r-1) ; pos < 1 << r ; pos++)
        ohe[0] = set_bit(ohe[0], pos) ;
      andBlocks_arr(ohe, cross_share, 1) ;
      
    } else {
      ;
    }

    delete[] msg ; delete[] rcv_msg ; delete[] cross_share ;
  }

  delete[] b ;
  delete[] r0 ; delete[] r1 ; delete[] rcv_ot ;
  return ohe ;
}


int main(int argc, char** argv) {
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " <party-id> <port> <n> <iknp/ferret> \n";
    exit(EXIT_FAILURE);
   };

  int party, port, n ;
  string ot_type ;
  NetIO *io ;
  COT<NetIO> *ot1, *ot2 ;

  if (argc != 5)
    abort();
  
  party = atoi(argv[1]) ;
  port = atoi(argv[2]) ;
  n = atoi(argv[3]) ;
  ot_type = argv[4] ;

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


  block *ohe, *rec ;
  int counter = 0 ;
  do {
    ohe = random_ohe(party, n, ot1, ot2) ;
    rec = reconst_ohe(party, n, ot1, ot2, ohe, false) ;

    int no_set = 0 ;
    uint64_t *dat = (uint64_t*)rec ;
    for (int pos = 0 ; pos < 64 ; pos++) {
      if (dat[0] & (1L << pos))
        no_set++ ;
      if (dat[1] & (1L << pos))
        no_set++ ;
    }

    if (no_set != 1)
      break ;
      
    delete[] ohe ; delete[] rec ;
    counter++ ;
  } while (counter < 10 * (1 << n)) ;

  if (counter == 10 * (1 << n))
    cout << "Passed\n" ;
  else
    cout << "Failed\n" ;
}