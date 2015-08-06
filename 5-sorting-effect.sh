#!/bin/bash
source script-base
let SAMPLES_LAST=$SAMPLES-1
CSVDIR=out-sorting-csv

# $1 - param value,  $2 - file format string, $3 - extent, $4 -intent, $5 - sorting flag
produce_line(){
	local param="$1"
	local extent="$3"
	local intent="$4"
	local flag="$5"
	local file=`printf $2 $1`
	echo "***" "$file $flag" >&2
	run_to_csv "$extent" "$intent" table malloc "$ALGOS" "-t$THREADS -b100 -L1" "$file"
}

# $1 - extent, $2 - intent, $3 - sample #, $4 - sort flag
process_real_sets(){
	local extent=$1
	local intent=$2
	local n=$3
	local sorted=$4
	local file="data/%s.dat"
	local hdr=$(echo -n "L," && echo "$ALGOS" | sed 's/ /,---,/g')
	echo "$extent" "$intent"  >&2
	if echo "x$flag" | grep -q sorted ; then
		value="yes"
	else
		value="no"
	fi
	produce_csv_series "$CSVDIR/sorting-$value-$intent-$extent-$dataset-$n.csv" "$hdr" "$RANGE" \
		produce_line "$file" $extent $intent $sorted
	./merge-csv $CSVDIR/sorting-$value-$intent-$extent-$dataset-*.csv > final/$FINAL-sorting-$value-$intent-$extent-$dataset.csv
}

do_all(){
	rm -rf "$CSVDIR"
	mkdir -p final
	mkdir -p "$CSVDIR"
	for flag in "-sort" " " ; do 
		for s in $(seq 0 ${SAMPLES_LAST}) ; do
			if [ "$FINAL" == "serial" ] ; then # ad-hoc parallelism
				process_real_sets bitset bitset $s $flag &
				process_real_sets linear bitset $s $flag &
			else
				process_real_sets bitset bitset $s $flag 
				process_real_sets linear bitset $s $flag 
			fi
		done
		wait # 2 * sample count forks at most
	done
	rm -rf $CSVDIR # cleanup
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