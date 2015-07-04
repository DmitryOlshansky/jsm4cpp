#!/bin/bash
source script-base
ALGOS="cbo fcbo p-bcbo p-fcbo"

# $1 - buf_size, $2 writer,  $3 - dataset file
produce_line(){
	local buf="$1"
	local wrt="$2"
	local file="$3"
	echo -n "$buf,"
	run_to_csv bitset bitset $wrt malloc "$ALGOS" -b$buf "$file"
	echo
}
(
echo -n "Size,"
echo "$ALGOS"  | sed 's/ /,,/g'
for buf in 10 20 40 80 100 1000 2000 4000 8000 10000 20000 400000 100000 ; do
	produce_line $buf table data1.dat
done
)  > 1-io-tab.csv

for buf in 10 20 40 80 100 1000 2000 4000 8000 10000 20000 400000 100000 ; do
	produce_line $buf table data1.dat
done > 1-io-sim.csv
