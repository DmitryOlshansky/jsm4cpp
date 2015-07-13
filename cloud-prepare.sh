#!/bin/sh
# A script to get fresh cloud VM up and running
sudo apt-get update && sudo apt-get upgrade -y
sudo apt-get install -y build-essential  g++ gcc libboost-dev scons gdc git

git clone https://blackwhale@bitbucket.org/blackwhale/rnd.git

cd rnd/jsm-generate2
./build-all.sh
