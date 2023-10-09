#!/bin/bash
print_date_sleep_3s() {
	date
	sleep 3
}

print_date_sleep_3s
mkdir ex3_root

print_date_sleep_3s
touch ex3_root/root.txt

print_date_sleep_3s
mkdir ex3_home

print_date_sleep_3s
touch ex3_home/home.txt

ls / > ex3_root/root.txt
ls ~ > ex3_home/home.txt

