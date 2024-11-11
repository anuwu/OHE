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
./../build/bin/bench_ferret_budget_time $1 $2 $3 2 ohe 162382

printf "n = 3 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 3 ohe 40914

printf "n = 4 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 4 ohe 14903

printf "n = 5 ---\n"
./../build/bin/bench_ferret_budget_time $1 $2 $3 5 ohe 6314

printf "n = 6 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 6 ohe 2878

printf "n = 7 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 7 ohe 1369

printf "n = 8 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 8 ohe 666

printf "n = 9 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 9 ohe 332

printf "n = 10 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 10 ohe 168

printf "n = 11 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 11 ohe 83

printf "n = 12 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 12 ohe 42