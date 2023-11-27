#!/bin/bash
gcc create_fs.c -o create_fs
gcc ex2.c -o ex2

fs=$(head -n 1 input.txt)

./create_fs "$fs"
./ex2 input.txt