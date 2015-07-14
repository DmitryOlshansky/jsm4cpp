#!/bin/bash
source script-base
SAMPLES=3 # in real datasets it's just number of runs over the same input
let SAMPLES_LAST=$SAMPLES-1
DATADIR=out-real
CSVDIR=out-real-csv
THREADS=`nproc`

# $1 - param value,  $2 - file format string, $3 - extent, $4 -intent,
produce_line(){
	local param="$1"
	local extent="$3"
	local intent="$4"
	local file=`printf $2 $1`
	run_to_csv "$extent" "$intent" table malloc "$ALGOS" "-t$THREADS -L1" "$file"
}

# iterate 2-D range of (extent-type x intent-type )
for_intents_extents(){
	local cmd="$1"
	shift 1
	for intent in $INTENTS ; do
	for extent in $EXTENTS ; do
		for n in `seq 0 ${SAMPLES_LAST}` ; do
			"$cmd" $intent $extent $@
		done
	done
	done
}

# $1 - extent, $2 - intent,
process_real_sets(){
	local extent=$1
	local intent=$2
	local file="data/%s.dat"
	local hdr=$(echo -n "#Objects," && 	echo "$ALGOS" | sed 's/ /,---,/g')
	# echo "$hdr" >&2
	produce_csv_series "$CSVDIR/objects-$intent-$extent-$n.csv" "$hdr" "$RANGE" \
		produce_line "$file" $extent $intent
	./merge-csv $CSVDIR/objects-$intent-$extent-*.csv > final/${FINAL}real-$intent-$extent.csv
}

do_all(){
	mkdir -p final
	mkdir -p "$CSVDIR"
	for_intents_extents process_real_sets 
	rm -rf $CSVDIR # cleanup
}

echo "Using $THREADS threads for parallel version." >&2

# entry point 
ALGOS="$SERIAL"
FINAL="serial-"
RANGE="adult mushroom"
do_all

ALGOS="$PARALLEL"
FINAL="par-"
# same RANGE
do_all

