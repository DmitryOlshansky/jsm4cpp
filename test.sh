for alg in cbo fcbo inclose2 inclose3 pfcbo
do
    echo $alg
    ./gen -a$alg -t8 $1 | sort > $alg-$1
done

for alg in cbo fcbo inclose2 inclose3 pfcbo
do
    diff cbo-$1 $alg-$1
done

