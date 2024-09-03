#!/bin/zsh

for n in $(seq 18 23);
do
 printf "n = $n\n" 
 ./build/vrot $1 10000 $n $2 $3 $4 0
done
