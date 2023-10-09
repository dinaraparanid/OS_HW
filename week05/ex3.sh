#!/bin/bash
gcc -pthread ex3.c -o ex3

for i in 1 2 4 10 100
do
  time ./ex3 -n 10000000 -m $i
done