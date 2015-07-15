#!/bin/bash
# Run as ./0-run-all.sh [stage# to start with] [last stage# to run]
run_stage(){
	echo "Stage $1" >&2
	sudo time ./$1-*.sh
}

runner(){
	for n in `seq $START $END` ; do
		run_stage $n
	done
}

if [ "x$1" == "x" ]; then
	START=1
else
	START=$1
fi

if [ "x$2" == "x" ];  then
	END=6
else
	END=$2
fi


# need root level permissions to use nice -n -20
runner 2> error-log.log
