#!/bin/bash
source script-base

build(){
	scons --extent=$1 --intent=$2 --writer=$3 --alloc=$4 --release
}


for WRT in ${WRITERS[@]}; do
	build bitset bitset $WRT malloc
done

for A in ${ALLOCS[@]}; do
	for EXT in ${EXTENTS[@]} ; do
		build $EXT bitset table $A
	done
done

for EXT in ${EXTENTS[@]} ; do
	for INT in ${INTENTS[@]}; do
		build $EXT $INT table malloc
	done
done

cp build/*/gen-* .
