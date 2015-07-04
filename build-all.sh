#!/bin/bash
EXTENTS=(bitset linear tree) # TODO: hashset
INTENTS=(bitset linear tree) # might not be the same as extents
WRITERS=(table simple)
ALLOCS=(malloc shared-pool tls-pool)

# gen-extent-intent-writer-alloc
#      $1      $2     $3    $4
binary_suffix(){
	echo "${1:0:3}-${2:0:3}-${3:0:3}-${4:0:1}"
}

build(){
	scons --extent=$1 --intent=$2 --writer=$3 --alloc=$4 --release
}


for WRT in ${WRITERS[@]}; do
	build bitset bitset $WRT malloc
done

for A in ${ALLOCS[@]}; do
	build bitset bitset table $A
done

for EXT in ${EXTENTS[@]} ; do
	for INT in ${INTENTS[@]}; do
		build $EXT $INT table malloc
	done
done

cp build/*/gen-* .
