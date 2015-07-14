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

# <cmd> <args>
run_csv_foreign(){
	local cmd=$1
	shift 1
	OUTPUT=$(/usr/bin/time -f "%e-%M" $cmd $@ 2>&1  > out.dat)
	ELAPSED=$(echo "$OUTPUT" | sed -r 's/\s*([0-9a-z.]+)-([0-9a-]+)\s*/\1/')
	KB=$(echo "$OUTPUT" | sed -r 's/\s*([0-9a-z.]+)-([0-9a-z]+)\s*/\2/')
	echo -n "$ELAPSED,$KB,"
}

# output hdr RANGE 
produce_krajca_csv(){
	local output="$1"
	local hrd="$2"
	local range="$3"
	(
		echo "$hdr"
		for f in $range ; do
			echo -n "$f,"
			run_csv_foreign ./fcbo $f
			run_csv_foreign ./pcbo -P$THREADS -L2
			echo
		done
	) > $output
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
	for n in $(seq 0 ${SAMPLES_LAST}) ; do
		if [ "$FINAL" == "serial" ] ; then  # produce only first time
			produce_krajca_csv "$CSVDIR/real-krajca-$n.csv" "$hdr" "$RANGE" 
		fi
		produce_csv_series "$CSVDIR/real-$intent-$extent-$n.csv" "File,fcbo,---,pcbo,---" "$RANGE" \
			produce_line "$file" $extent $intent
	done
	./merge-csv $CSVDIR/real-krajca-*.csv > final/${FINAL}-real-krajca.csv
	./merge-csv $CSVDIR/real-$intent-$extent-*.csv > final/${FINAL}-real-$intent-$extent.csv	
}

do_all(){
	rm -rf "$CSVDIR"
	mkdir -p final
	mkdir -p "$CSVDIR"
	for_intents_extents process_real_sets 
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
