#!/bin/bash
for ((i=1; i <= $#; i++)); do
	echo "size: ${!i} - FLOP - TIME"
	rm Double_*_${!i}.matrix output.txt -f
	./generateMatrices ${!i} > /dev/null
	./cudaMxM ${!i} Double_A_8192.matrix Double_B_8192.matrix Double_GOLD_${!i}.matrix 1 > output.txt 2>&1
	time= cat output.txt | grep "time: [0-9]\{1,\}.[0-9]\{1,\}" -o | cut -c7-
	nvprof -m flop_count_dp ./cudaMxM ${!i} Double_A_8192.matrix Double_B_8192.matrix Double_GOLD_${!i}.matrix 1 > output.txt 2>&1
	flop= cat output.txt | grep "Double Precisi  [0-9e+\.]\{1,\}" -o | cut -c17-
done
