for alg in cbo fcbo inclose2 inclose3 pfcbo pinclose2 pinclose3
do
    echo $alg
    ./gen -a$alg -v1 -L1 -t4 $1 | sort > "$alg-$1"
done

for alg in fcbo inclose2 inclose3 pfcbo pinclose2 pinclose3
do
    diff --brief "cbo-$1" "$alg-$1"
done

