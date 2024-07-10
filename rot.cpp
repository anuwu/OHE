#include "emp-ot/emp-ot.h"
#include <iostream>

int party;
int port;
int num_ots;
int num_bytes;

NetIO* channel;
COT<NetIO>* ot_channel;

constexpr int threads = 1;

void experiment() {
  if (party == ALICE) {
    char* send_msg = new char[num_bytes];
    emp::block* send_lefts = new emp::block[num_ots];
    emp::block* send_rights = new emp::block[num_ots];

    std::cout << "Sending rot\n" ;
    ot_channel->send_rot(send_lefts, send_rights, num_ots);
    channel->send_data(send_msg, num_bytes);

    delete[] send_lefts;
    delete[] send_rights;
    delete[] send_msg;
  } else {
    char* rcv_msg = new char[num_bytes] ;
    bool* choices = new bool[num_ots];
    emp::block* recvs = new emp::block[num_ots];
    std::cout << "Receiving rot\n" ;

    ot_channel->recv_rot(recvs, choices, num_ots);
    channel->recv_data(rcv_msg, num_bytes);

    delete[] recvs;
    delete[] choices;
    delete[] rcv_msg;
  }
  channel->flush();
}

int main(int argc, char** argv) {
  const auto abort = [&] {
    std::cerr
      << "usage: "
      << argv[0]
      << " <party-id> <port> <n> <repetitions> <iknp/ferret> <flute/ohe>\n";
    std::exit(EXIT_FAILURE);
  };

  if (argc != 5) { abort(); }
  
  party = atoi(argv[1]) ;
  port = atoi(argv[2]) ;
  num_ots = atoi(argv[3]) ;
  num_bytes = atoi(argv[4]) ;

  channel = new NetIO(party == 1 ? nullptr : "127.0.0.1", port);
  ot_channel = new IKNP<NetIO>(channel) ;

  experiment() ;
}
