#!/bin/zsh

for n in $(seq 1 16);
do
 printf "n = $n\n" 
 ./build/vrot $1 10000 $n $2 1
done
