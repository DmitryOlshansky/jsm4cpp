#!/bin/bash
source script-base
ALGOS="$ALL"

# $1 alloc,  $2 - dataset file
produce_line(){
	local alloc="$1"
	local file="$2"
	run_to_csv bitset bitset table $alloc "$ALGOS" -b$buf "$file"
}

# <input> <output> 
produce_file(){
	local inp="$1"
	local output="$2"
	local hdr=$(echo -n "Alloc," && echo "$ALGOS"  | sed 's/ /,---,/g')
	local range="$ALLOCS"
	produce_csv_series "$output" "$hdr" "$range" produce_line "$inp"
}

rm -rf out-mem-csv
mkdir -p out-mem-csv
mkdir -p final
for name in $REALDATA ; do
	for n in `seq 0 ${SAMPLES_LAST}` ; do
		produce_file data/$name.dat out-mem-csv/mem-$name-$n.csv
	done
done
