#!/bin/zsh

for n in $(seq 14 16);
do
 printf "n = $n\n" 
 ./build/test_ohe $1 10000 $n $2 $3 $4 $5
done
