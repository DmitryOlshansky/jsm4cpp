#!/bin/bash
source script-base
let SAMPLES_LAST=$SAMPLES-1
CSVDIR=out-real-csv

# $1 - param value,  $2 - file format string, $3 - extent, $4 -intent,
produce_line(){
	local param="$1"
	local extent="$3"
	local intent="$4"
	local file=`printf $2 $1`
	# echo $file >&2
	run_to_csv "$extent" "$intent" table malloc "$ALGOS" "-t$THREADS -b100 -L1" "$file"
}

# <cmd> <args>
run_csv_foreign(){
	local cmd=$1
	shift 1
	# echo $cmd $@ >&2
	OUTPUT=$(/usr/bin/time -f "%e-%M" $cmd $@ 2>&1  > out.dat)
	ELAPSED=$(echo "$OUTPUT" | sed -r 's/\s*([0-9a-z.]+)-([0-9a-]+)\s*/\1/')
	KB=$(echo "$OUTPUT" | sed -r 's/\s*([0-9a-z.]+)-([0-9a-z]+)\s*/\2/')
	echo -n "$ELAPSED,$KB,"
}

# output hdr RANGE 
produce_krajca_csv(){
	local output="$1"
	local hdr="$2"
	local range="$3"
	(
		echo "$hdr"		
		for f in $range ; do
			local file=`printf data/%s.dat $f`
			echo -n "$f,"
			run_csv_foreign ./fcbo $file
			run_csv_foreign ./pcbo -P$THREADS -L2 $file
			echo
		done
	) > $output
}

# $1 - extent, $2 - intent, $3 - sample count
process_real_sets(){
	local extent=$1
	local intent=$2
	local n=$3
	local file="data/%s.dat"
	local hdr=$(echo -n "File," && 	echo "$ALGOS" | sed 's/ /,---,/g')
	if [ "$FINAL" == "par" ] ; then  # produce only once
		produce_krajca_csv "$CSVDIR/real-krajca-$n.csv" "File,fcbo,---,pcbo,---" "$RANGE" 
		./merge-csv $CSVDIR/real-krajca-*.csv > final/${FINAL}-real-krajca.csv
	fi
	produce_csv_series "$CSVDIR/real-$extent-$intent-$n.csv" "$hdr" "$RANGE" \
		produce_line "$file" $extent $intent
	./merge-csv $CSVDIR/real-$extent-$intent-*.csv > final/${FINAL}-real-$extent-$intent.csv
}

do_all(){
	rm -rf "$CSVDIR"
	mkdir -p final
	mkdir -p "$CSVDIR"
	if [ "$FINAL" == "serial" ] ; then
		for_intents_extents_samples process_real_sets fork_scale
	else
		for_intents_extents_samples process_real_sets
	fi
}

# entry point 
echo "Using $THREADS threads for parallel execution." >&2

ALGOS="$SERIAL"
FINAL="serial"
RANGE="$REALDATA"
echo "Serial versions." >&2
do_all

ALGOS="$PARALLEL"
FINAL="par"
RANGE="$REALDATA"
echo "Parallel versions." >&2
do_all
