# attributes
mkdir -p exps

for attr in 50 100 200 300 400 500 600 
do
./datagen 2500 $attr 0.05 > exps/a$attr.dat
done

# objects
for obj in 5000 10000 20000 30000 40000 50000 60000 
do
./datagen $obj 200 0.05 > exps/o$obj.dat
done

# density
for density in 0.01 0.02 0.03 0.04 0.05 0.06 0.07 0.08 0.09 0.10
do
./datagen 5000 150 $density > exps/d$density.dat
done

