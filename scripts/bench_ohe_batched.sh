#!/bin/zsh

if [[ $# == 1 ]] then
  if [ "$1" = "--help" ] ; then
    printf "$0 <party> <port> <ip> <iknp/ferret> <ohe/gmt> <batch_size> <start> <end> \n"
    exit ;
  else
    printf "Run '$0 --help' for menu\n"
    exit ;
  fi
  exit ;
fi

if [[ $# != 8 ]] then
  printf "Correct usage -\n"
  printf "$0 <party> <port> <ip> <iknp/ferret> <ohe/gmt> <batch_size> <start> <end>\n"
  exit
fi

for n in $(seq $7 $8);
do
  printf "n = $n ---\n" 
  ./../build/bin/bench_ohe $1 $2 $3 $n $4 $5 1 $6
done
