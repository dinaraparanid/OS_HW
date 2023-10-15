#!/bin/bash
gcc scheduler.c -o scheduler
gcc worker.c -o worker

./scheduler data.txt