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

printf "n = 2 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 2 gmt 162382

printf "n = 3 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 3 gmt 40914

printf "n = 4 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 4 gmt 14903

printf "n = 5 ---\n"
./../build/bin/bench_ferret_budget_time $1 $2 $3 5 gmt 6314

printf "n = 6 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 6 gmt 2878

printf "n = 7 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 7 gmt 1369

printf "n = 8 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 8 gmt 666

printf "n = 9 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 9 gmt 332

printf "n = 10 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 10 gmt 168

printf "n = 11 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 11 gmt 83

printf "n = 12 ---\n" 
./../build/bin/bench_ferret_budget_time $1 $2 $3 12 gmt 42