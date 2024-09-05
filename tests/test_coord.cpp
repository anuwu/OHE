#include "coord.h"
#include "utils.h"
#include <iostream>
#include <time.h>

using namespace std ;
using namespace emp ;

void parse_options(const char *const *arg, int *party, int *parties, int *start_port, int *length, string &ot_type, bool *threaded) {
  *party = atoi (arg[1]) ;
  *parties = atoi (arg[2]) ;
  *start_port = atoi(arg[3]) ;
  *length = 1 << atoi(arg[4]) ;
  ot_type = arg[5] ;
  *threaded = atoi(arg[6]) ;
}

int main(int argc, char** argv) {
  if (argc != 7)
    cout << "Usage : " << argv[0] << " <party> <parties> <start_port> <length> <ot_type> <threaded>\n" ;

  // Parse options
  int party, parties, start_port, length ;
  bool threaded ;
  string ot_type ;
  parse_options(argv, &party, &parties, &start_port, &length, ot_type, &threaded) ;

  // Declare
  int loglength = atoi(argv[4]) ;
  vector<pair<int,int>> robin ;
  robin = get_round_robin_scheme(party, parties) ;
  unordered_map<int, NetIO*> ios ;

  // Setup communication channels
  if (parties > 6 && ot_type == "ferret")
    ios = get_pairwise_channels(party, parties, start_port, robin) ;
  else 
    ios = get_pairwise_channels_threaded(party, parties, start_port) ;

  int64_t comms ;
  auto start = clock_start() ;
  if (ot_type == "otnp") {
    unordered_map<int, OTNP<NetIO>*> ots ;
    for (auto &it : ios)
      ots[it.first] = new OTNP<NetIO>(it.second) ;
    Coordinator<OTNP<NetIO>> cood(party, parties, length, loglength, robin, ots) ;
    if (threaded)
      comms = cood.dummy_ots_threaded() ;
    else
      comms = cood.dummy_ots() ;
  }
  else if (ot_type == "iknp") {
    unordered_map<int, IKNP<NetIO>*> ots ;
    for (auto &it : ios)
      ots[it.first] = new IKNP<NetIO>(it.second) ;
    Coordinator<IKNP<NetIO>> cood(party, parties, length, loglength, robin, ots) ;
    if (threaded)
      cood.dummy_ots_threaded() ; 
    else
      cood.dummy_ots() ;
  }
  else if (ot_type == "simple") {
    unordered_map<int, OTCO<NetIO>*> ots ;
    for (auto &it : ios)
      ots[it.first] = new OTCO<NetIO>(it.second) ;
    Coordinator<OTCO<NetIO>> cood(party, parties, length, loglength, robin, ots) ;
    if (threaded)
      comms = cood.dummy_ots_threaded() ;
    else
      comms = cood.dummy_ots() ;
  }
  else if (ot_type == "ferret") {
    unordered_map<int, FerretCOT<NetIO>*> ots ;
    for (auto &it : ios)
      ots[it.first] = new FerretCOT<NetIO>(party < it.first ? ALICE : BOB, 1, &it.second, false) ;
      
    Coordinator<FerretCOT<NetIO>> cood(party, parties, length, loglength, robin, ots) ;
    if (threaded)
      comms = cood.dummy_ots_threaded() ;
    else 
      comms = cood.dummy_ots() ; 
  } else {
    exit(1) ;
  }
  
  std::setprecision(2) ;
  long long t = time_from(start);  
  std::cout << std::fixed << std::setprecision(2) << "Time taken : " << double(t)/1e3 << " ms\n" ;
  std::cout << std::fixed << std::setprecision(2) << "Comm : " << comms << " bytes\n" ;

  return 0 ;
}