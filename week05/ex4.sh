#!/bin/bash
gcc -pthread ex4.c -o ex4

for i in 1 2 4 10 100
do
  time ./ex4 -n 10000000 -m $i
done