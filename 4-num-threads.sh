#!/bin/bash
source script-base
SAMPLES=3 # in real datasets it's just number of runs over the same input
let SAMPLES_LAST=$SAMPLES-1
CSVDIR=out-threads-csv
THREADS=`nproc`

# $1 - param value,  $2 - file format string, $3 - extent, $4 -intent, $5 - value of L, $6 - alloc
produce_line(){
	local param="$1"
	local extent="$3"
	local intent="$4"
	local L="$5"
	local alloc="$6"
	local file="$2"
	echo "***" "-t$param -L$L" >&2
	run_to_csv "$extent" "$intent" table "$alloc" "$ALGOS" "-t$param -L$L" "$file"
}

# $1 - extent, $2 - intent, $3 - sample #, $4 - dataset, $5 - L, $6 - alloc
process_real_sets(){
	local extent=$1
	local intent=$2
	local n=$3
	local dataset=$4
	local L=$5
	local alloc=$6
	local file="data/$dataset.dat"
	local hdr=$(echo -n "L," && echo "$ALGOS" | sed 's/ /,---,/g')
	echo "$extent" "$intent" "$file" >&2
	produce_csv_series "$CSVDIR/threads-$L-$extent-$intent-$alloc-$dataset-$n.csv" "$hdr" "$RANGE" \
		produce_line "$file" $extent $intent $L $alloc
	./merge-csv $CSVDIR/threads-$L-$extent-$intent-$alloc-$dataset-*.csv > final/threads-L$L-$extent-$intent-$alloc-$dataset.csv
}

do_all(){
	rmdir -rf "$CSVDIR"
	mkdir -p final
	mkdir -p "$CSVDIR"
	for dataset in $DATA ; do 
	for L in 2 ; do # just use 2 for L
	for alloc in malloc shared-pool tls-pool ; do
	for s in $(seq 0 ${SAMPLES_LAST}) ; do
			process_real_sets bitset bitset $s $dataset $L $alloc
			process_real_sets linear bitset $s $dataset $L $alloc
	done
	done
	done
	done
}

# entry point 
echo "Using up to $THREADS threads for parallel execution." >&2

ALGOS="$PARALLEL"
FINAL="threads"
RANGE=$(seq 1 $THREADS)
DATA="$REALDATA"
echo "Parallel versions." >&2
do_all
