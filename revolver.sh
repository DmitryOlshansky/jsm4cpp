#!/bin/bash -e

read_batch(){
	local X=""
	while read -t 1 line; do
		local X="$line$X"
	done
	echo "$X"
}

scons $@ 2>&1 | head -40
cp build/*/gen-* .
echo "[REVOLVER] Intial build is done."
inotifywait -m -r src SConstruct -e modify | while true; do
	BUF=$(read_batch)
	if [ "x$BUF" != "x" ] ; then
		scons $@ 2>&1 | head -40
		cp build/*/gen-* .
		echo "[REVOLVER] Done. Waiting for new events."
	fi
done
