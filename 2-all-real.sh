#!/bin/bash
source script-base
SAMPLES=3 # in real datasets it's just number of runs over the same input
let SAMPLES_LAST=$SAMPLES-1
CSVDIR=out-real-csv
THREADS=`nproc`

# $1 - param value,  $2 - file format string, $3 - extent, $4 -intent,
produce_line(){
	local param="$1"
	local extent="$3"
	local intent="$4"
	local file=`printf $2 $1`
	# echo $file >&2
	run_to_csv "$extent" "$intent" table malloc "$ALGOS" "-t$THREADS -L1" "$file"
}

# iterate 2-D range of (extent-type x intent-type )
for_intents_extents(){
	local cmd="$1"
	shift 1
	# linearize combinatorics - fix extent to the "standard" - bitset
	for intent in $INTENTS ; do
		echo "bitset" "$intent" >&2
		for n in `seq 0 ${SAMPLES_LAST}` ; do
			"$cmd" bitset $intent $@
		done
	done
	# linearize combinatorics - fix intent to the "standard" - bitset
	for extent in $EXTENTS ; do
		echo "$extent" "bitset" >&2
		for n in `seq 0 ${SAMPLES_LAST}` ; do
			"$cmd" $extent bitset $@
		done
	done
}

# $1 - extent, $2 - intent,
process_real_sets(){
	local extent=$1
	local intent=$2
	local file="data/%s.dat"
	local hdr=$(echo -n "File," && 	echo "$ALGOS" | sed 's/ /,---,/g')
	produce_csv_series "$CSVDIR/real-$intent-$extent-$n.csv" "$hdr" "$RANGE" \
		produce_line "$file" $extent $intent
	./merge-csv $CSVDIR/real-$intent-$extent-*.csv > final/${FINAL}-real-$intent-$extent.csv
}

do_all(){
	mkdir -p final
	mkdir -p "$CSVDIR"
	for_intents_extents process_real_sets 
	rm -rf $CSVDIR # cleanup
}

# entry point 
echo "Using $THREADS threads for parallel execution." >&2

ALGOS="$PARALLEL"
FINAL="par"
RANGE="$REALDATA"
echo "Parallel versions." >&2
do_all

ALGOS="$SERIAL"
FINAL="serial"
RANGE="$REALDATA"
echo "Serial versions." >&2
do_all
