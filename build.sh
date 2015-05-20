#!/bin/sh
g++ -O2 -std=c++11 gen.cpp  -pthread -Wl,--no-as-needed -ogen
g++ -O2 -std=c++11 jsm.cpp  -pthread -Wl,--no-as-needed -ojsm
