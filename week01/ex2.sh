#!/bin/bash
mkdir ex2_test
cd ex2_test
touch tree.txt
touch test.txt
tree | sort -r | head -n 5 > tree.txt
history | sort | tail -n 5 > test.txt
cd ..
rm -r ex2_test
history | tail -n 9 > ex2.txt
