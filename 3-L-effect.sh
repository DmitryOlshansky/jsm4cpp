#!/bin/bash
source script-base
SAMPLES=3 # in real datasets it's just number of runs over the same input
let SAMPLES_LAST=$SAMPLES-1
CSVDIR=out-L-csv
THREADS=`nproc`

# $1 - param value,  $2 - file format string, $3 - extent, $4 -intent,
produce_line(){
	local param="$1"
	local extent="$3"
	local intent="$4"
	local file="$2"
	echo "***" "-t$THREADS -L$param" >&2
	run_to_csv "$extent" "$intent" table malloc "$ALGOS" "-t$THREADS -L$param" "$file"
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
	produce_csv_series "$CSVDIR/L-$intent-$extent-$dataset-$n.csv" "$hdr" "$RANGE" \
		produce_line "$file" $extent $intent
	./merge-csv $CSVDIR/L-$intent-$extent-$dataset-*.csv > final/L-$intent-$extent-$dataset.csv
}

do_all(){
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
RANGE="0 1 2 3 4 5 6" # values of L
DATA="$REALDATA"
echo "Parallel versions." >&2
do_all
