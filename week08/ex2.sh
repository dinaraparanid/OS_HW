#!/bin/bash
sudo gcc pager.c -o pager
sudo gcc mmu.c -o mmu

sudo ./pager 4 2 &
sleep 1
PAGER_PID=$(cat pager_pid.txt)

sudo ./mmu 4 R0 R1 W1 R0 R2 W2 R0 R3 W2 "$PAGER_PID"
sudo rm pager_pid.txt