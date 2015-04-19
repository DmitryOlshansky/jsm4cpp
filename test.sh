for alg in cbo fcbo inclose2 inclose3 pfcbo pinclose2 pinclose3
do
    echo $alg
    ./gen -a$alg -v0 -L1 -t4 $1 | sort > $alg-$1
#    ./gen2 -a$alg -v0 -L2 -t4 $1 | sort > $alg-$1-2
done

for alg in cbo fcbo inclose2 inclose3 pfcbo pinclose2 pinclose3
do
    diff -q cbo-$1 $alg-$1
#    diff -q cbo-$1 $alg-$1-2
done

