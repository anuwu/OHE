#!/bin/zsh

if [[ $# == 1 ]] then
  if [ "$1" = "--help" ] ; then
    printf "$0 <party> <port> <iknp/ferret> <ohe/gmt> <batched?> <start> <end>\n"
    exit ;
  else
    printf "Run '$0 --help' for menu\n"
    exit ;
  fi
  exit ;
fi

if [[ $# != 7 ]] then
  printf "Correct usage -\n"
  printf "$0 <party> <port> <iknp/ferret> <ohe/gmt> <batched?> <start> <end>\n"
  exit
fi

for n in $(seq $6 $7);
do
  printf "n = $n\n" 
  ./../build/tests/test_ohe $1 $2 $n $3 $4 $5
done
