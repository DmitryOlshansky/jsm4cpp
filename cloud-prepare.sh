#!/bin/sh
# A script to get fresh cloud VM up and running
sudo apt-get update && sudo apt-get upgrade -y
sudo apt-get install -y build-essential g++ libboost-dev scons git libboost-system-dev

# install recent DMD D compiler
wget http://downloads.dlang.org/releases/2.x/2.067.1/dmd_2.067.1-0_amd64.deb
sudo dpkg -i dmd_2.067.1-0_amd64.deb

git clone https://blackwhale@bitbucket.org/blackwhale/rnd.git

cd rnd/jsm-generate2
dmd -O -release datagen.d &
dmd -O -release merge-csv.d &
./build-all.sh
wait
