#!/bin/bash
gcc monitor.c -o monitor
gcc ex1.c -o ex1

mkdir bebra
./monitor /home/paranid5/PROGRAMMING/C/OS_HW/week10/bebra &
./ex1 /home/paranid5/PROGRAMMING/C/OS_HW/week10/bebra
./ex1_test.sh
rm -r bebra