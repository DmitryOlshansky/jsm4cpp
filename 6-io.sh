#!/bin/bash -e
source script-base
ALGOS="$ALL"
SAMPLES=3 # number of random samples per synthetic data set 
# $1 - buf_size, $2 writer,  $3 - dataset file
produce_line(){
	local buf="$1"
	local wrt="$2"
	local file="$3"
	echo -n "$buf,"
	run_to_csv bitset bitset $wrt malloc "$ALGOS" -b$buf "$file"
	echo
}

# <input> <output> <writer>
produce_file(){
	(
	echo -n "Size,"
	echo "$ALGOS"  | sed 's/ /,---,/g'
	for buf in 10 20 40 80 100 1000 2000 4000 8000 10000 20000 400000 ; do
		produce_line "$buf" "$3" "$1"
	done
	)  > $2 
}

# objects attrs density
produce_all_files(){
	mkdir -p out-io-csv
	name="$1-$2-$3"
	for dat in $(pick_datasets $1 $2 $3 out-io) ; do
		N=$(dataset_sample_number $dat)
		produce_file $dat out-io-csv/io-tab-$name-$N.csv table
		produce_file $dat out-io-csv/io-sim-$name-$N.csv simple
	done
}

# entry point 
mkdir -p out-1-io
for tripple in "5000 100 0.05" "5000 150 0.05" "10000 50 0.05" ; do
	make_random_datasets $SAMPLES $tripple "out-io"
	produce_all_files $tripple
done
