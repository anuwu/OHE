#!/bin/zsh

if [[ $# == 1 ]] then
  if [ "$1" = "--help" ] ; then
    printf "$0 <party> <port> <ip> <ohe/gmt>\n"
    exit ;
  else
    printf "Run '$0 --help' for menu\n"
    exit ;
  fi
  exit ;
fi

if [[ $# != 4 ]] then
  printf "Correct usage -\n"
  printf "$0 <party> <port> <ip> <ohe/gmt>\n"
  exit
fi

printf "n = 2 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 2 $4 162382

printf "n = 3 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 3 $4 65591

printf "n = 4 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 4 $4 30932

printf "n = 5 ---\n"
./../build/bin/bench_ferret_budget_time $1 $2 $3 5 $4 16496

printf "n = 6 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 6 $4 8375

printf "n = 7 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 7 $4 4252

printf "n = 8 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 8 $4 2154

printf "n = 9 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 9 $4 1084

printf "n = 10 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 10 $4 548

printf "n = 11 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 11 $4 275

printf "n = 12 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 12 $4 139