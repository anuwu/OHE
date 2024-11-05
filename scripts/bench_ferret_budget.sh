#!/bin/zsh

if [[ $# == 1 ]] then
  if [ "$1" = "--help" ] ; then
    printf "$0 <party> <port> <ohe/gmt> <batch_size> <budget> <start> <end> \n"
    exit ;
  else
    printf "Run '$0 --help' for menu\n"
    exit ;
  fi
  exit ;
fi

if [[ $# != 7 ]] then
  printf "Correct usage -\n"
  printf "$0 <party> <port> <ohe/gmt> <batch_size> <budget> <start> <end>\n"
  exit
fi

for n in $(seq $6 $7);
do
  printf "n = $n\n" 
  ./../build/bin/bench_ferret_budget $1 $2 $n $3 $4 $5
done
