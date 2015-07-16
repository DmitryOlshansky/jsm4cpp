#!/bin/bash
source script-base
if [ $# -ne 1 ]; then
	echo "Usage. /test.sh <file.dat>"
	exit 1
fi
if [ "x$CMD" == "x" ] ; then 
	echo "Using default command: ./gen"
	CMD=./gen
fi
if [ "x$OUT" == "x" ] ; then 
	echo "Using default output folder: out"
	OUT=out
fi
mkdir -p $OUT


NAME=`echo "$1" | sed -r 's|.*/(.*)|\1|'`
for alg in $ALL
do
    echo $alg
    # TODO: -m2 - min support tests
    # Piping to sort here *does* have negative effect on
    # performance due to I/O happening in batches
    time $CMD "-a$alg" -v1 -L0 -t4 $1  > "$OUT/$alg-$NAME"
done
time ./fcbo < $1 > "$OUT/orig-fcbo-$NAME"
time ./pcbo -P4 -L1 < $1 > "$OUT/orig-pcbo-$NAME"
#first sort numbers in each line;  remove last blank line then sort lines
./datanorm < "$OUT/orig-fcbo-$NAME" | sed '/^$/d' | sort > "$OUT/orig-fcbo-$NAME-sorted"
./datanorm < "$OUT/orig-pcbo-$NAME" | sed '/^$/d' | sort > "$OUT/orig-pcbo-$NAME-sorted"
for alg in $ALL
do
    ./datanorm < "$OUT/$alg-$NAME"  | sort > "$OUT/$alg-$NAME-sorted"
    diff --brief "$OUT/orig-fcbo-$NAME-sorted" "$OUT/$alg-$NAME-sorted"
done

