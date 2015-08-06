#!/bin/bash
source script-base
let SAMPLES_LAST=$SAMPLES-1
CSVDIR=out-L-csv

# $1 - param value,  $2 - file format string, $3 - extent, $4 -intent,
produce_line(){
	local param="$1"
	local extent="$3"
	local intent="$4"
	local file="$2"
	echo "***" "-t$THREADS -L$param" >&2
	run_to_csv "$extent" "$intent" table malloc "$ALGOS" "-t$THREADS -b100 -L$param" "$file"
}

# $1 - extent, $2 - intent, $3 - sample #, $4 - dataset
process_real_sets(){
	local extent=$1
	local intent=$2
	local n=$3
	local dataset=$4
	local file="data/$dataset.dat"
	local hdr=$(echo -n "L," && echo "$ALGOS" | sed 's/ /,---,/g')
	echo "$extent" "$intent" "$file" >&2
	produce_csv_series "$CSVDIR/L-$extent-$intent-$dataset-$n.csv" "$hdr" "$RANGE" \
		produce_line "$file" $extent $intent
	./merge-csv $CSVDIR/L-$extent-$intent-$dataset-*.csv > final/L-$extent-$intent-$dataset.csv
}

do_all(){
	rmdir -rf "$CSVDIR"
	mkdir -p final
	mkdir -p "$CSVDIR"
	for dataset in $DATA ; do 
	for s in $(seq 0 ${SAMPLES_LAST}) ; do
		process_real_sets bitset bitset $s $dataset
		process_real_sets linear bitset $s $dataset
	done
	done
}

# entry point 
echo "Using $THREADS threads for parallel execution." >&2

ALGOS="$PARALLEL"
RANGE="0 1 2 3 4 5" # values of L
DATA="$REALDATA"
echo "Parallel part." >&2
do_all
