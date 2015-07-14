#!/bin/bash
source script-base
SAMPLES=3
let SAMPLES_LAST=$SAMPLES-1
DATADIR=out-synth
CSVDIR=out-synth-csv
THREADS=`nproc`

# $1 - param value,  $2 - file format string, $3 - extent, $4 -intent,
produce_line(){
	local param="$1"
	local extent="$3"
	local intent="$4"
	local file=`printf $2 $1`
	run_to_csv "$extent" "$intent" table malloc "$ALGOS" "-t$THREADS -L1" "$file"
}


# iterate over 2-D range of (values x sample count)
# $1 - value range, $2 cmd, $3 ... - extra args
# cmd is run as: $cmd $value $sample $extra
for_range_and_sample(){
	local range="$1"
	local cmd="$2"
	shift 2 
	for p in $range ; do
		for n in `seq 0 ${SAMPLES_LAST}` ; do
			"$cmd" $p $n $@
		done
	done
}

# iterate 3-D range of (extent-type x intent-type x sample count)
for_intents_extents_samples(){
	local cmd="$1"
	shift 1
	for intent in $INTENTS ; do
	for extent in $EXTENTS ; do
		for n in `seq 0 ${SAMPLES_LAST}` ; do
			"$cmd" $intent $extent $n $@
		done
	done
	done
}

gen_attr(){
	./datagen 2500 $1 0.05 > $DATADIR/a-$1-$2.dat
}

gen_objs(){
	./datagen $1 200 0.05 > $DATADIR/o-$1-$2.dat
}

gen_density(){
	./datagen 5000 150 $1 > $DATADIR/d-$1-$2.dat
}

# $1 - extent, $2 - intent, $3 - sample
produce_object_csv_series(){
	local extent=$1
	local intent=$2
	local n=$3
	local file="$DATADIR/o-%s-$n.dat"
	local hdr=$(echo -n "#Objects," && 	echo "$ALGOS" | sed 's/ /,---,/g')
	# echo "$hdr" >&2
	produce_csv_series "$CSVDIR/objects-$intent-$extent-$n.csv" "$hdr" "$OBJRANGE" \
		produce_line "$file" $extent $intent
	./merge-csv $CSVDIR/objects-$intent-$extent-*.csv > final/${FINAL}synth-objects-$intent-$extent.csv
}

# $1 - extent, $2 - intent, $3 - sample
produce_attribute_csv_series(){
	local extent=$1
	local intent=$2
	local n=$3
	local file="$DATADIR/a-%s-$n.dat"
	local hdr=$(echo -n "#Attrs," && 	echo "$ALGOS" | sed 's/ /,---,/g')
	# echo "$hdr" >&2
	produce_csv_series "$CSVDIR/attrs-$intent-$extent-$n.csv" "$hdr" "$ATTRRANGE" \
		produce_line "$file" $extent $intent
	./merge-csv $CSVDIR/attrs-$intent-$extent-*.csv > final/${FINAL}synth-attrs-$intent-$extent.csv
}


# $1 - extent, $2 - intent, $3 - sample
produce_density_csv_series(){
	local extent=$1
	local intent=$2
	local n=$3
	local file="$DATADIR/d-%s-$n.dat"
	local hdr=$(echo -n "#Density," && 	echo "$ALGOS" | sed 's/ /,---,/g')
	# echo "$hdr" >&2
	produce_csv_series "$CSVDIR/density-$intent-$extent-$n.csv" "$hdr" "$DENRANGE" \
		produce_line "$file" $extent $intent
	./merge-csv $CSVDIR/density-$intent-$extent-*.csv > final/${FINAL}synth-density-$intent-$extent.csv
}

do_all(){
	mkdir -p final
	mkdir -p "$DATADIR"
	mkdir -p "$CSVDIR"
	for_range_and_sample "$ATTRRANGE" gen_attr 
	for_range_and_sample "$OBJRANGE" gen_objs
	for_range_and_sample "$DENRANGE" gen_density

	# object series
	for_intents_extents_samples produce_object_csv_series 
	# attribute series
	for_intents_extents_samples produce_attribute_csv_series

	# density series
	for_intents_extents_samples produce_density_csv_series
	
	rm -rf $CSVDIR # cleanup
}
echo "Using $THREADS threads for parallel version." >&2
# entry point 
ALGOS="$SERIAL"
FINAL="serial-"
OBJRANGE="5000 10000 20000 30000 40000 50000 60000"
ATTRRANGE="50 100 150 200 250 300 400 500 600"
DENRANGE="0.01 0.02 0.03 0.04 0.05 0.06 0.07 0.08 0.09 0.10"
do_all

ALGOS="$PARALLEL"
FINAL="par-"
OBJRANGE="10000 20000 30000 40000 50000 60000 70000 80000 90000"
ATTRRANGE="50 100 150 200 250 300 400 500 600 700 800 900"
DENRANGE="0.01 0.02 0.03 0.04 0.05 0.06 0.07 0.08 0.09 0.10 0.11 0.12 0.13"
do_all

