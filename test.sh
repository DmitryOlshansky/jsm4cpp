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

NAMES="bcbo fcbo inclose2 inclose3"
# parallel names
PNAMES=$(echo $NAMES | sed -r 's/[a-z0-9]+/p-\0/g')
FPNAMES=$(echo $NAMES | sed -r 's/[a-z0-9]+/fp-\0/g')
TPNAMES=$(echo $NAMES | sed -r 's/[a-z0-9]+/tp-\0/g')
# 
ALL="cbo $NAMES $PNAMES $FPNAMES $TPNAMES"
NOTCBO="$NAMES $PNAMES $FPNAMES $TPNAMES"

for alg in $ALL
do
    echo $alg
    # TODO: -m2 - min support tests
    # Piping to sort here *does* have nagative effect on 
    # performance due to I/O happening in batches
    time $CMD "-a$alg" -v1 -L0 -t4 $1  > "$OUT/$alg-$1"
done
time ./fcbo < $1 > "$OUT/orig-fcbo-$1"
time ./pcbo -P4 -L1 < $1 > "$OUT/orig-pcbo-$1"
#first sort numbers in each line;  remove last blank line then sort lines
./datanorm < "$OUT/orig-fcbo-$1" | sed '/^$/d' | sort > "$OUT/orig-fcbo-$1-sorted"
./datanorm < "$OUT/orig-pcbo-$1" | sed '/^$/d' | sort > "$OUT/orig-pcbo-$1-sorted"
for alg in $ALL
do
    ./datanorm < "$OUT/$alg-$1"  | sort > "$OUT/$alg-$1-sorted"
    diff --brief "$OUT/orig-fcbo-$1-sorted" "$OUT/$alg-$1-sorted"
done

