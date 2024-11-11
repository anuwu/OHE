#!/bin/zsh

if [[ $# == 1 ]] then
  if [ "$1" = "--help" ] ; then
    printf "$0 <party> <port> <ip>\n"
    exit ;
  else
    printf "Run '$0 --help' for menu\n"
    exit ;
  fi
  exit ;
fi

if [[ $# != 3 ]] then
  printf "Correct usage -\n"
  printf "$0 <party> <port> <ip>\n"
  exit
fi

# printf "n = 2 ---\n" 
# ./../build/bin/bench_ferret_budget_time $1 $2 $3 2 ohe 162382

# printf "n = 3 ---\n" 
# ./../build/bin/bench_ferret_budget_time $1 $2 $3 3 ohe 65591

# printf "n = 4 ---\n" 
# ./../build/bin/bench_ferret_budget_time $1 $2 $3 4 ohe 30932

# printf "n = 5 ---\n"
# ./../build/bin/bench_ferret_budget_time $1 $2 $3 5 ohe 16496

printf "n = 6 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 6 ohe 8375

printf "n = 7 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 7 ohe 4252

printf "n = 8 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 8 ohe 2154

printf "n = 9 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 9 ohe 1084

printf "n = 10 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 10 ohe 548

printf "n = 11 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 11 ohe 275

printf "n = 12 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 12 ohe 139