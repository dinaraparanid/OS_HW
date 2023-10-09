#!/bin/bash
gcc -pthread ex5.c -o ex5

for i in 1 2 4 10 100
do
  time ./ex5 -n 10000000 -m $i
done