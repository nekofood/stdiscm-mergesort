#!/bin/bash

threads=(1 2 4 8 16 32 64 128 256 512 1024)

echo "================RESULTS================" > result.txt

for n in ${threads[@]}; do
	echo "Doing thread count $n"
	echo "================THREAD COUNT $n================" >> result.txt
	echo -e "8388608\n$n" | ./conc >> result.txt
	echo -e "8388608\n$n" | ./conc >> result.txt
	echo -e "8388608\n$n" | ./conc >> result.txt
	echo -e "8388608\n$n" | ./conc >> result.txt
	echo -e "8388608\n$n" | ./conc >> result.txt
	echo "================================================" >> result.txt
done
