#!/bin/bash
source script-base
THREADS=`nproc`

# $1 - buf_size, $2 writer,  $3 - dataset file
produce_line(){
	local buf="$1"
	local wrt="$2"
	local file="$3"
	# "" - no extra flags
	run_to_csv bitset bitset $wrt malloc "$ALGOS" -L1 -b$buf "-t$THREADS" "$file"
}

# <input> <output> <writer>
produce_file(){
	local inp="$1"
	local output="$2"
	local writer="$3"
	local hdr=$(echo -n "Size," && echo "$ALGOS"  | sed 's/ /,---,/g')
	local range="10 20 40 80 100 1000 2000 4000 8000 16000 32000"
	produce_csv_series "$output" "$hdr" "$range" produce_line "$writer" "$inp"
}

echo "Using $THREADS threads for parallel execution." >&2
# entry point 
rm -rf "$CSVDIR"
mkdir -p final
mkdir -p out-io
for tripple in "10000 200 0.05" ; do
	name=`dataset_name $tripple`
	make_random_datasets $SAMPLES $tripple "out-io"
	if true ; then 
		ALGOS="$SERIAL"
		# use fork-based parallelism for serial versions
		echo "Running serial part."
		apply_to_datasets $tripple out-io out-io-csv/io-serial-sim-$name-%s.csv produce_file simple &
		apply_to_datasets $tripple out-io out-io-csv/io-serial-tab-$name-%s.csv produce_file table &
		wait
	fi
	if true ; then 
		ALGOS="$PARALLEL"
		echo "Running parallel part."
		apply_to_datasets $tripple out-io out-io-csv/io-par-sim-$name-%s.csv produce_file simple
		apply_to_datasets $tripple out-io out-io-csv/io-par-tab-$name-%s.csv produce_file table
		mkdir -p final
		./merge-csv out-io-csv/io-par-sim-$name-*.csv > final/io-sim-par-$name.csv
		./merge-csv out-io-csv/io-par-tab-$name-*.csv > final/io-tab-par-$name.csv
		./merge-csv out-io-csv/io-serial-sim-$name-*.csv > final/io-sim-serial-$name.csv
		./merge-csv out-io-csv/io-serial-tab-$name-*.csv > final/io-tab-serial-$name.csv
	fi
done
