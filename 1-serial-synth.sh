#!/bin/bash
source script-base
ALGOS="$ALL"
# $1 extent,  $2 - dataset file
produce_line(){
	local extent="$1"
	local file="$2"
	echo -n "$file," | sed -r 's/(.*)\/[a-z]+//g' | sed 's/\.dat//g'  # kill alphabetic prefixes
	run_to_csv "$extent" bitset table malloc "$ALGOS" "$file"
	echo
}

# $1 - name, $2 - input files
produce_file(){
	local name="$1"
	shift 1
	local inps="$@"
	for ext in bitset linear ; do
		(
		echo -n "#$name,"
		echo "$ALGOS" | sed 's/ /,---,/g'
		for f in $inps; do
			produce_line "$ext" "$f"
		done 
		) > "4-$ext-bitset-$name.csv"
	done
}

produce_file attrs exps/a*
produce_file objects exps/o*
produce_file density exps/d*


