#include "ohe.h"
#include "lut.h"
#include "utils.h"
#include <time.h>
#include <iostream>

using namespace std ;
using namespace emp ;

// Test identity
void test1(int argc, char **argv) {
  // Abort message
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " 1 <n> \n";
    exit(EXIT_FAILURE);
  } ;
  if (argc != 3)
    abort() ;

  // Read arguments
  int n = atoi(argv[2]) ;

  // Create identity LUT
  LUT lut = identity(n) ;
  cout << lut ;
}

// Read LUT
void test2(int argc, char **argv) {
  // Abort message
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " 2 <n> <m> <lut_name> \n";
    exit(EXIT_FAILURE);
  } ;
  if (argc != 5)
    abort() ;

  // Read arguments
  int n = atoi(argv[2]) ;
  int m = atoi(argv[3]) ;
  string lut_name = argv[4] ;

  // Display contents of the file
  LUT lut = input_lut(n, m, "../../luts/" + lut_name + ".lut") ;

  // Diaplying the LUT
  cout << "The LUT after reading -\n" ;
  cout << lut << "\n" ;
}

// Test rotate
void test3(int argc, char **argv) {
  // Abort message
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " 3 <n>\n";
    exit(EXIT_FAILURE);
  } ;
  if (argc != 3)
    abort() ;

  // Read argument
  int n = atoi(argv[2]) ;

  // Initialize variables
  uint64_t N = 1ULL << n ;
  random_device rd ;
  mt19937 rng(rd()) ;
  uniform_int_distribution<int> uid(0, N-1) ;
  int hot_index = uid(rng) ;
  uint64_t rotation = uid(rng) ;
  int num_blocks = get_ohe_blocks(n) ;
  block *hot = new block[num_blocks] ;
  initialize_blocks(hot, num_blocks) ;
  SET_BIT(hot, hot_index) ;

  // Obtain rotated vector
  block *hot_rotated = new block[num_blocks] ; initialize_blocks(hot_rotated, num_blocks) ;
  rotate(n, hot, rotation, hot_rotated) ;

  // Verify
  cout << "hot_index = " << hot_index << "\n" ;
  cout << "rotation = " << rotation << "\n" ;
  cout << "rotated_index = " << (hot_index^rotation) << "\n" ;
  for (uint64_t i = 0 ; i < N ; i++) {
    bool rot_bool = TEST_BIT(hot_rotated, i) ;
    bool plain_bool = TEST_BIT(hot, i^rotation) ;
    if (rot_bool != plain_bool) {
      cout << "\033[1;31m" << "Failed " << "\033[0m\n" ;
      exit(EXIT_FAILURE) ;
    }
  
    if (rot_bool)
      cout << "Found rotated index at --> " << i << "\n" ;
  }

  delete[] hot ;
}

// Evaluate LUT
void test4(int argc, char **argv) {
  // Abort message
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " 4 <n>\n";
    exit(EXIT_FAILURE);
  } ;
  if (argc != 3)
    abort() ;

  // Read argument
  int n = atoi(argv[2]) ;

  // Initialize variables
  uint64_t N = 1ULL << n ;
  random_device rd ;
  mt19937 rng(rd()) ;
  uniform_int_distribution<int> uid(0, N-1) ;
  int hot_index = uid(rng) ;
  int num_blocks = get_ohe_blocks(n) ;
  block *hot = new block[num_blocks] ;
  initialize_blocks(hot, num_blocks) ;
  SET_BIT(hot, hot_index) ;

  LUT lut = identity(n) ;
  block *res = new block[1] ; initialize_blocks(res, 1) ;
  eval_lut(n, lut, hot, res) ;
  uint64_t recovered_index = 0 ;
  for (int i = 0 ; i < n ; i++)
    if (TEST_BIT(res, i))
      recovered_index += 1ULL << i ;

  cout << "hot_index = " << hot_index << "\n" ;
  cout << "recovered_index = " << recovered_index << "\n" ;

  delete[] hot ;
  delete[] res ;
}

/************************* OHE evaluation *************************/

// Single OHE evaluation
void test5(int argc, char **argv) {
  // Abort message
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " 5 <party> <port> <n> <m> <iknp/ferret> <ohe/gmt> <lut_name>\n";
    exit(EXIT_FAILURE);
  } ;
  if (argc != 9)
    abort() ;

  // Read arguments
  int party = atoi(argv[2]) ;
  int port = atoi(argv[3]) ;
  int n = atoi(argv[4]) ;
  int m = atoi(argv[5]) ;
  string ot_type = argv[6] ;
  string prot_type = argv[7] ;
  string lut_name = argv[8] ;

  // Initializing network stuff
  NetIO *io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port, true) ;
  COT<NetIO> *ot1, *ot2 ;
  if (ot_type == "iknp") {
    ot1 = new IKNP<NetIO>(io) ;
    ot2 = new IKNP<NetIO>(io) ;
  } 
  else if (ot_type == "ferret") {
    ot1 = new FerretCOT<NetIO>(party, 1, &io) ;
    ot2 = new FerretCOT<NetIO>(party == 1 ? 2 : 1, 1, &io) ;
  }
  else {
    cout << "Incorrect OT type\n" ;
    exit(EXIT_FAILURE) ;
  }

  // Initializing stuff
  PRG prg ;
  int num_blocks = get_ohe_blocks(n) ;
  int m_blocks = (m+127)/128 ;
  block *inp_share = new block[1] ;
  prg.random_block(inp_share, 1) ; 
  block input_mask = zero_block ;
  for (int i = 0 ; i < n ; i++)
    input_mask = set_bit(input_mask, i) ;
  andBlocks_arr(inp_share, input_mask, 1) ;
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

  // Get OHE
  block *ohe ;
  if (prot_type == "ohe")
    ohe = random_ohe(party, n, ot1, ot2) ;
  else if (prot_type == "gmt")
    ohe = random_gmt(party, n, ot1, ot2) ;
  else {
    cerr << "Incorrect protocol type\n" ;
    exit(EXIT_FAILURE) ;
  }

  // Do secure evaluation
  block *reconst_otp = new block[m_blocks] ; initialize_blocks(reconst_otp, m_blocks) ;
  secure_eval(party, n, ot1, ot2, func, inp_share, ohe, reconst_otp) ;

  // Check validity
  block *reconst_inp = new block[1] ; initialize_blocks(reconst_inp, 1) ;
  reconst(party, ot1, ot2, n, inp_share, reconst_inp) ;
  block *reconst_inp_ohe = new block[num_blocks] ; initialize_blocks(reconst_inp_ohe, num_blocks) ;
  get_ohe_from_plain(reconst_inp, reconst_inp_ohe) ;
  block *clear_otp = new block[m_blocks] ; initialize_blocks(clear_otp, m_blocks) ;
  eval_lut(n, func, reconst_inp_ohe, clear_otp) ;
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
  delete[] reconst_otp ;
  delete[] reconst_inp ;
  delete[] reconst_inp_ohe ;
  delete[] clear_otp ; 
}

// Batched OHE evaluation
void test6(int argc, char **argv) {
  // Abort message
  const auto abort = [&] {
    cerr
      << "usage: "
      << argv[0]
      << " 6 <party> <port> <n> <m> <batch_size> <iknp/ferret> <ohe/gmt> <lut_name>\n";
    exit(EXIT_FAILURE);
  } ;
  if (argc != 10)
    abort() ;

  // Read arguments
  int party = atoi(argv[2]) ;
  int port = atoi(argv[3]) ;
  int n = atoi(argv[4]) ;
  int m = atoi(argv[5]) ;
  int batch_size = atoi(argv[6]) ;
  string ot_type = argv[7] ;
  string prot_type = argv[8] ;
  string lut_name = argv[9] ;

  // Initializing network stuff
  NetIO *io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port, true) ;
  COT<NetIO> *ot1, *ot2 ;
  if (ot_type == "iknp") {
    ot1 = new IKNP<NetIO>(io) ;
    ot2 = new IKNP<NetIO>(io) ;
  } 
  else if (ot_type == "ferret") {
    ot1 = new FerretCOT<NetIO>(party, 1, &io) ;
    ot2 = new FerretCOT<NetIO>(party == 1 ? 2 : 1, 1, &io) ;
  }
  else {
    cout << "Incorrect OT type\n" ;
    exit(EXIT_FAILURE) ;
  }

  // Initializing data
  int num_blocks = get_ohe_blocks(n) ;
  int m_blocks = (m+127)/128 ;
  PRG prg ;
  block *inp_share = new block[batch_size] ;
  prg.random_block(inp_share, batch_size) ; 
  block input_mask = zero_block ;
  for (int i = 0 ; i < n ; i++)
    input_mask = set_bit(input_mask, i) ;
  andBlocks_arr(inp_share, input_mask, batch_size) ;
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
  
  // Get OHE
  block **ohes ;
  if (prot_type == "ohe")
    ohes = batched_random_ohe(party, n, batch_size, ot1, ot2) ;
  else if (prot_type == "gmt")
    ohes = batched_random_gmt(party, n, batch_size, ot1, ot2) ;
  else {
    cerr << "Incorrect protocol type\n" ;
    exit(EXIT_FAILURE) ;
  }

  // Secure evaluation
  block **reconst_otps = new block*[batch_size] ;
  for (int b = 0 ; b < batch_size ; b++) {
    reconst_otps[b] = new block[m_blocks] ;
    initialize_blocks(reconst_otps[b], m_blocks) ;
  }
  batched_secure_eval(party, n, batch_size, ot1, ot2, func, inp_share, ohes, reconst_otps) ;
    
  // Check validity
  block *reconst_inp = new block[batch_size] ; initialize_blocks(reconst_inp, batch_size) ;
  block **reconst_inp_ohes = new block*[batch_size] ;
  block **clear_otps = new block*[batch_size] ;
  bool all_flag = true ;
  int corr = 0 ;
  for (int b = 0 ; b < batch_size ; b++) {
    reconst(party, ot1, ot2, n, inp_share+b, reconst_inp+b) ;
    reconst_inp_ohes[b] = new block[num_blocks] ; initialize_blocks(reconst_inp_ohes[b], num_blocks) ;
    get_ohe_from_plain(reconst_inp+b, reconst_inp_ohes[b]) ;
    clear_otps[b] = new block[m_blocks] ; initialize_blocks(clear_otps[b], m_blocks) ;
    eval_lut(n, func, reconst_inp_ohes[b], clear_otps[b]) ;

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
    delete[] ohes[b] ;
    delete[] reconst_otps[b] ;
    delete[] reconst_inp_ohes[b] ;
    delete[] clear_otps[b] ;
  }
  delete[] inp_share ;
  delete[] ohes ;
  delete[] reconst_inp ;
  delete[] reconst_otps ;
  delete[] reconst_inp_ohes ;
  delete[] clear_otps ; 
}

int main(int argc, char** argv) {
  if (argc < 2) {
    cerr << "To view help : " << argv[0] << " <test_no> \n" ;
    exit(EXIT_FAILURE) ;
  }

  int test_no = atoi(argv[1]) ;

  switch(test_no) {
  // test identity
  case 1 :
    test1(argc, argv) ;
    break ;
  
  // test reading from file
  case 2 :
    test2(argc, argv) ;
    break ;
  
  // test rotation
  case 3 :
    test3(argc, argv) ;
    break ;

  // test evaluation
  case 4 :
    test4(argc, argv) ;
    break ;
  
  // single ohe evaluation
  case 5 :
    test5(argc, argv) ;
    break ;

  // batched ohe evaluation
  case 6 :
    test6(argc, argv) ;
    break ;

  default :
    cerr << "Invalid test case\n" ;
    exit(EXIT_FAILURE) ;
  }

  return 0 ;
}
