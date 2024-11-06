#include "emp-ot/emp-ot.h"
#include <iostream>
#include <time.h>

using namespace std ;
using namespace emp ;

constexpr int threads = 1 ;

int main(int argc, char** argv) {
  /************************* Parse Input *************************/
  
  const auto abort = [&] {
		cerr << "usage : " << argv[0] << " <party>" << " <port>" << " <ip>\n" ;
		exit(EXIT_FAILURE);
	} ;

	if (argc != 4)
		abort() ;

	int party = atoi(argv[1]) ;
	int port = atoi(argv[2]) ;
	const char *ip = argv[3] ;
	NetIO *io ;
	io = new NetIO(party == ALICE ? nullptr : ip, port) ;

	block *data = new block[10000000] ;
	block *rcv = new block[10000000] ;

	auto start_exp = clock_start() ; 

	if (party == ALICE) {
		io->send_block(data, 10000000) ;
		io->recv_block(rcv, 10000000) ;
	} else {
		io->recv_block(rcv, 10000000) ;
		io->send_block(data, 10000000) ;
	}
	io->flush() ;

	setprecision(2) ;
  long long t_exp = time_from(start_exp) ;   

	cout << fixed << setprecision(2) << "Time taken : " << double(t_exp)/1e3 << " ms\n" ;
	
  delete[] data ;
  return 0 ;
}