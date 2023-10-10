#!/bin/bash
gcc subscriber.c -o subscriber
gcc publisher.c -o publisher && ./publisher 3

for i in 0 1 2
do
  xterm -hold -e ./subscriber $i &
done