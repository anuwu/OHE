#include "emp-ot/emp-ot.h"
#include <iostream>

int party;
int port;

NetIO* channel;
COT<NetIO>* ot_channel;

constexpr int threads = 1;

void experiment(const std::vector<std::size_t>& ot_sizes, int32_t div) {
  const auto start_time = clock_start();
  const auto start_comm = channel->counter;

  const auto num_ots = ot_sizes.size();
  std::size_t num_bits = 0;
  for (auto s : ot_sizes) { num_bits += s; }
  const auto num_bytes = (num_bits + 7)/8;

  emp::block* recvs = new emp::block[num_ots];
  emp::block* send_lefts = new emp::block[num_ots];
  emp::block* send_rights = new emp::block[num_ots];
  bool* choices = new bool[num_ots];
  char* messages = new char[num_bytes];

  std::cout << num_ots << '\t' << num_bytes << '\n';

  if (party == ALICE) {
    std::cout << "Sending rot\n" ;
    ot_channel->send_rot(send_lefts, send_rights, num_ots);
    channel->send_data(messages, num_bytes);
    ot_channel->recv_rot(recvs, choices, num_ots);
    channel->recv_data(messages, num_bytes);

    std::cout << "All the choices are -\n" ;
    for (int i = 0 ; i < num_ots ; i++)
      std::cout << choices[i] << ", " ;
    std::cout << "\n" ;
  } else {
    std::cout << "Receiving rot\n" ;
    ot_channel->recv_rot(recvs, choices, num_ots);
    channel->recv_data(messages, num_bytes);
    ot_channel->send_rot(send_lefts, send_rights, num_ots);
    channel->send_data(messages, num_bytes);

    std::cout << "All the choices are -\n" ;
    for (int i = 0 ; i < num_ots ; i++)
      std::cout << choices[i] << ", " ;
    std::cout << "\n" ;
  }
  channel->flush();

  const auto t = time_from(start_time);    
  std::cout << std::fixed << std::setprecision(2);
  std::cout << "Time taken : " << double(t)/1e3 << " ms\n" ;
  std::cout << "Comm : " << (channel->counter - start_comm) << " bytes\n" ;

  delete[] recvs;
  delete[] send_lefts;
  delete[] send_rights;
  delete[] choices;
  delete[] messages;
}

void flute(int reps, int n) {
  int N = 1 << n;
  std::vector<std::size_t> sizes;
  for (std::size_t i = 0; i < (N - n - 1) * reps; ++i) {
    sizes.push_back(1);
  }
  experiment(sizes, reps);
}

void ohe(int reps, int n) {
  std::vector<std::size_t> sizes;
  for (std::size_t j = 0; j < reps; ++j) {
    for (std::size_t i = 1; i < n; ++i) {
      sizes.push_back(1 << i);
    }
  }
  experiment(sizes, reps);
}

int main(int argc, char** argv) {
  const auto abort = [&] {
    std::cerr
      << "usage: "
      << argv[0]
      << " <party> <port> <n> <repetitions> <iknp/ferret> <flute/ohe>\n";
    std::exit(EXIT_FAILURE);
  };

  if (argc < 7) { abort(); }

  party = atoi(argv[1]) ;
  port = atoi(argv[2]) ;
  int n = atoi(argv[3]) ;
  int reps = atoi(argv[4]) ;
  std::string ot_type = argv[5] ;
  std::string protocol = argv[6] ; 

  // std::cout << "going to set channel\n" ;
  channel = new NetIO(party == 1 ? nullptr : "127.0.0.1", port);
  // std::cout << "have set the channel\n" ;
  if (ot_type == "iknp"  ) { ot_channel = new IKNP<NetIO>(channel); }
  else if (ot_type == "ferret") { ot_channel = new FerretCOT<NetIO>(party, threads, &channel); }
  else { abort(); }

  if (protocol == "ohe") { ohe(1, 1) ; ohe(reps, n); }
  else if (protocol == "flute") { flute(1, 1) ; flute(reps, n); }
  else { abort(); }

}