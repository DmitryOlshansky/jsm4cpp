if [ $# -ne 1 ]; then
	echo "Usage. /test.sh <file.dat>"
	exit 1
fi
NAMES="bcbo fcbo inclose2 inclose3"
# parallel names
PNAMES=$(echo $NAMES | sed -r 's/[a-z0-9]+/p-\0/g')
FPNAMES=$(echo $NAMES | sed -r 's/[a-z0-9]+/fp-\0/g')
ALL="cbo $NAMES $PNAMES $FPNAMES"
NOTCBO="$NAMES $PNAMES $FPNAMES"
for alg in $ALL
do
    echo $alg
    #TODO: -m2 - min support tests
    time ./gen -a$alg -v1 -L1 -t4 $1 | sort > "$alg-$1"
done

for alg in $NOTCBO
do
    diff --brief "cbo-$1" "$alg-$1"
done

