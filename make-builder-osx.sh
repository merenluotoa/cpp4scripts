#!/bin/bash
g++ -o builder-osx builder.cpp -Wall -pthread -fexceptions -fno-rtti -fuse-cxa-atexit -O2
#g++ -ggdb -o builder-osx builder.cpp -Wall -pthread -fexceptions -fno-rtti -fuse-cxa-atexit -O0
