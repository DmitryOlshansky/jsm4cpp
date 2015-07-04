#!/bin/bash
source script-base
ALGOS="$ALL"

# $1 alloc,  $2 - dataset file
produce_line(){
	local alloc="$1"
	local file="$2"
	echo -n "$alloc,"
	run_to_csv bitset bitset table $alloc "$ALGOS" -b$buf "$file"
	echo
}

(
echo -n "Alloc,"
echo "$ALGOS" | sed 's/ /,,/g'
for ALOC in ${ALLOCS[@]} ; do
	produce_line "$ALOC" data1.dat
done
)  # > 1-io-tab.csv

