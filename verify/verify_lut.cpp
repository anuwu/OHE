#include "ohe.h"
#include "lut.h"
#include "utils.h"
#include <time.h>
#include <iostream>

using namespace std ;
using namespace emp ;

int main(int argc, char** argv) {
  // Abort message
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " <party> <port> <ip> <n> <m> <iknp/ferret> <ohe/gmt> <lut_name> <batched> <opt:batch_size>\n";
    exit(EXIT_FAILURE);
  } ;

  // Declare variables
  int party, port, n, m, batch_size ;
  char *ip ;
  string ot_type, prot_type, lut_name ;
  bool batched ;
  NetIO *io ;
  COT<NetIO> *ot1, *ot2 ;

  // Check command line arguments
  if (argc < 10)
    abort() ;
  else if (argc == 10) {
    if (atoi(argv[9]))
      abort() ;
  }
  else if (argc == 11) {
    if (!atoi(argv[9]))
      abort() ;
  }
  else
    abort() ;
    
  // Parse arguments
  party = atoi(argv[1]) ;
  port = atoi(argv[2]) ;
  ip = argv[3] ;
  n = atoi(argv[4]) ;
  m = atoi(argv[5]) ;
  ot_type = argv[6] ;
  prot_type = argv[7] ;
  lut_name = argv[8] ;
  batched = atoi(argv[9]) ;
  if (batched) {
    batch_size = atoi(argv[10]) ;
    if (batch_size % 128 > 0) {
      cerr << "Batch size must be a multiple of 128\n" ;
      exit(EXIT_FAILURE) ;
    }
  }
  
  /************************* Create OT *************************/

  io = new NetIO(party == ALICE ? nullptr : ip, port, true) ;
  if (ot_type == "iknp") {
    ot1 = new IKNP<NetIO>(io) ;
    ot2 = new IKNP<NetIO>(io) ;
  } 
  else if (ot_type == "ferret") {
    ot1 = new FerretCOT<NetIO>(party, 1, &io) ;
    ot2 = new FerretCOT<NetIO>(party == 1 ? 2 : 1, 1, &io) ;
  }
  else {
    cerr << "Incorrect OT type\n" ;
    exit(EXIT_FAILURE) ;
  }

  int num_blocks = get_ohe_blocks(n) ;
  int m_blocks = (m+127)/128 ;
  PRG prg ;
  block input_mask = zero_block ;
  for (int i = 0 ; i < n ; i++)
    input_mask = set_bit(input_mask, i) ;

  LUT func ;
  if (lut_name == "id") {
    if (m != n) {
      cerr << "Identity function needs n = m\n" ;
      exit(EXIT_FAILURE) ;
    }
    func = identity(n) ;
  }
  else
    func = input_lut(n, m, "../../luts/" + lut_name + ".lut") ;

  if (batched) {
    // Initializing data
    block **inp_shares = new block*[batch_size] ;
    for (int b = 0 ; b < batch_size ; b++) {
      inp_shares[b] = new block[1] ;
      prg.random_block(inp_shares[b], 1) ; 
      andBlocks_arr(inp_shares[b], input_mask, 1) ;
    }
    
    // Get OHE
    block **alphas, **ohes ;
    alphas = new block*[batch_size] ;
    ohes = new block*[batch_size] ;
    for (int b = 0 ; b < batch_size ; b++) {
      alphas[b] = new block[1] ;
      ohes[b] = new block[num_blocks] ;
      initialize_blocks(alphas[b], 1) ;
      initialize_blocks(ohes[b], num_blocks) ;
    }
    if (prot_type == "ohe")
      batched_random_ohe(party, n, batch_size, ot1, ot2, alphas, ohes) ;
    else if (prot_type == "gmt")
      batched_random_gmt(party, n, batch_size, ot1, ot2, alphas, ohes) ;
    else {
      cerr << "Incorrect protocol type\n" ;
      exit(EXIT_FAILURE) ;
    }

    // Batched Secure evaluation
    block **reconst_otps = new block*[batch_size] ;
    for (int b = 0 ; b < batch_size ; b++) {
      reconst_otps[b] = new block[m_blocks] ;
      initialize_blocks(reconst_otps[b], m_blocks) ;
    }
    func.batched_secure_eval(party, n, batch_size, ot1, ot2, inp_shares, ohes, alphas, reconst_otps) ;
      
    // Check validity
    block **reconst_inps = new block*[batch_size] ;
    block **reconst_inp_ohes = new block*[batch_size] ;
    block **clear_otps = new block*[batch_size] ;
    bool all_flag = true ;
    int corr = 0 ;
    for (int b = 0 ; b < batch_size ; b++) {
      reconst_inps[b] = new block[1] ; initialize_blocks(reconst_inps[b], 1) ;
      reconst(party, ot1, ot2, n, inp_shares[b], reconst_inps[b]) ;
      reconst_inp_ohes[b] = new block[num_blocks] ; initialize_blocks(reconst_inp_ohes[b], num_blocks) ;
      get_ohe_from_plain(reconst_inps[b], reconst_inp_ohes[b]) ;
      clear_otps[b] = new block[m_blocks] ; initialize_blocks(clear_otps[b], m_blocks) ;
      func.eval_lut(n, reconst_inp_ohes[b], clear_otps[b]) ;

      bool flag_batch = true ; 
      for (int m = 0 ; m < m_blocks ; m++) {
        if (!check_equal(clear_otps[b]+m, reconst_otps[b]+m)) {
          flag_batch = false ;
          break ;
        }
      }

      if (!flag_batch)
        all_flag = false ;
      else
        corr++ ;
    }
    
    if (all_flag) 
      cout << "\033[1;32m" << "Passed" << "\033[0m\n" ;
    else
      cout << "\033[1;31m" << "Failed --> " << corr << "\033[0m\n" ;

    // Delete stuff
    for (int b = 0 ; b < batch_size ; b++) {
      delete[] inp_shares[b] ;
      delete[] ohes[b] ;
      delete[] alphas[b] ;
      delete[] reconst_otps[b] ;
      delete[] reconst_inps[b] ;
      delete[] reconst_inp_ohes[b] ;
      delete[] clear_otps[b] ;
    }
    delete[] inp_shares ;
    delete[] ohes ;
    delete[] alphas ;
    delete[] reconst_otps ;
    delete[] reconst_inps ;
    delete[] reconst_inp_ohes ;
    delete[] clear_otps ; 
  } else {
    // Initializing stuff
    block *inp_share = new block[1] ;
    prg.random_block(inp_share, 1) ; 
    andBlocks_arr(inp_share, input_mask, 1) ;

    // Get OHE
    block *ohe, *alpha ;
    alpha = new block[1] ;
    ohe = new block[num_blocks] ;
    initialize_blocks(alpha, 1) ;
    initialize_blocks(ohe, num_blocks) ;
    if (prot_type == "ohe")
      random_ohe(party, n, ot1, ot2, alpha, ohe) ;
    else if (prot_type == "gmt")
      random_gmt(party, n, ot1, ot2, alpha, ohe) ;
    else {
      cerr << "Incorrect protocol type\n" ;
      exit(EXIT_FAILURE) ;
    }

    // Do secure evaluation
    block *reconst_otp = new block[m_blocks] ; initialize_blocks(reconst_otp, m_blocks) ;
    func.secure_eval(party, n, ot1, ot2, inp_share, ohe, alpha, reconst_otp) ;

    // Check validity
    block *reconst_inp = new block[1] ; initialize_blocks(reconst_inp, 1) ;
    reconst(party, ot1, ot2, n, inp_share, reconst_inp) ;
    block *reconst_inp_ohe = new block[num_blocks] ; initialize_blocks(reconst_inp_ohe, num_blocks) ;
    get_ohe_from_plain(reconst_inp, reconst_inp_ohe) ;
    block *clear_otp = new block[m_blocks] ; initialize_blocks(clear_otp, m_blocks) ;
    func.eval_lut(n, reconst_inp_ohe, clear_otp) ;
    bool flag = true ;
    for (int i = 0 ; i < m_blocks ; i++) {
      if (!check_equal(clear_otp+i, reconst_otp+i)) {
        flag = false ;
        break ;
      }
    }
    if (flag) 
      cout << "\033[1;32m" << "Passed" << "\033[0m\n" ;
    else
      cout << "\033[1;31m" << "Failed" << "\033[0m\n" ;

    // Delete stuff
    delete[] inp_share ;
    delete[] ohe ;
    delete[] alpha ;
    delete[] reconst_otp ;
    delete[] reconst_inp ;
    delete[] reconst_inp_ohe ;
    delete[] clear_otp ; 
  }

  // Delete OT stuff
  delete ot1 ;
  delete ot2 ;
  delete io ;

  return 0 ;
}
