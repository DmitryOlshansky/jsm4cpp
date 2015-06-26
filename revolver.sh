#!/bin/bash -e

read_batch(){
	local X=""
	while read -t 1 line; do
		local X="$line$X"
	done
	echo "$X"
}

inotifywait -m -r src -e modify | while true; do
	BUF=$(read_batch)
	if [ "x$BUF" != "x" ] ; then
		scons 2>&1 | head -40
		echo "[REVOLVER] Done. Wating for new events."
	fi
done
