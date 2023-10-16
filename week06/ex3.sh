#!/bin/bash
gcc scheduler_sjf.c -o scheduler_sjf
gcc worker.c -o worker

./scheduler_sjf data.txt