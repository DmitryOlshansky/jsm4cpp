if [ $# -ne 1 ]; then
	echo "Usage. /test.sh <file.dat>"
	exit 1
fi
for alg in cbo bcbo fcbo inclose2 inclose3 pcbo pfcbo pinclose2 pinclose3
do
    echo $alg
    #TODO: -m2 - min support tests
    time ./gen -a$alg -v1 -L1 -t4 $1 | sort > "$alg-$1"
done

for alg in bcbo fcbo inclose2 inclose3 pcbo pfcbo pinclose2 pinclose3
do
    diff --brief "cbo-$1" "$alg-$1"
done

