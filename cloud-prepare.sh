#!/bin/sh
# A script to get fresh cloud VM up and running
sudo apt-get update && sudo apt-get upgrade -y
sudo apt-get install -y make build-essential g++ libboost-dev scons git libboost-system-dev htop sysstat

# install recent DMD D compiler
wget http://downloads.dlang.org/releases/2.x/2.067.1/dmd_2.067.1-0_amd64.deb
sudo dpkg -i dmd_2.067.1-0_amd64.deb
sudo apt-get install -fy

if git clone https://blackwhale@bitbucket.org/blackwhale/rnd.git ; then
	cd rnd/jsm-generate2
	dmd -O -release datagen.d &
	dmd -O -release merge-csv.d &
	cd foreign/fcbo-ins
	make
	cp fcbo ../..
	cd ../pcbo-amai
	make
	cp pcbo ../..
	cd ../..
	./build-all.sh
	wait
fi
