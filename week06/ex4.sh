#!/bin/bash
gcc scheduler_rr.c -o scheduler_rr
gcc worker.c -o worker

./scheduler_rr data.txt