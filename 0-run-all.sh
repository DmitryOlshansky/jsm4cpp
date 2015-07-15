#!/bin/bash

run_stage(){
	echo "Stage $1" >&2
	time ./$1-*.sh
}

runner(){
	for n in `seq 1 6` ; do
		run_stage $n
	done
}
# need root level permissions to use nice -n -20
sudo runner 2> error-log.log
