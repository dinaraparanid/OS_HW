#!/bin/bash
gcc ex3.c -o ex3 && ./ex3 3 &

for _ in {1..3}
do
  pstree
  sleep 5
done

gcc ex3.c -o ex3 && ./ex3 5 &

for _ in {1..5}
do
  pstree
  sleep 5
done