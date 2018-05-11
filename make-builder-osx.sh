#!/bin/bash
g++ -o builder builder.cpp -Wall -pthread -fexceptions -fno-rtti -fuse-cxa-atexit -O2 -std=c++14
#g++ -ggdb -o builder-osx builder.cpp -Wall -pthread -fexceptions -fno-rtti -fuse-cxa-atexit -O0
