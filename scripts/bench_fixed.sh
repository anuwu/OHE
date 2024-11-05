#!/bin/zsh

if [[ $# == 1 ]] then
  if [ "$1" = "--help" ] ; then
    printf "$0 <party> <port> <iknp/ferret> <start> <end>\n"
    exit ;
  else
    printf "Run '$0 --help' for menu\n"
    exit ;
  fi
  exit ;
fi

if [[ $# != 5 ]] then
  printf "Correct usage -\n"
  printf "$0 <party> <port> <iknp/ferret> <start> <end>\n"
  exit ;
fi

for n in $(seq $4 $5);
do
  printf "n = $n\n" 
  ./../build/bin/bench_fixed $1 $2 $n $3
done
