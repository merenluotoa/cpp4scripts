#!/bin/bash
# release version:
# g++ -o builder-lnx builder.cpp -Wall -Wno-unused-result -pthread -fexceptions -fno-rtti -fuse-cxa-atexit -O2 -std=c++14
g++ -o builder-lnx builder.cpp -Wall -Wno-unused-result -pthread -fexceptions -fno-rtti -fuse-cxa-atexit -O2
# debug version:
# g++ -o builder-lnx builder.cpp -Wall -Wno-unused-result -pthread -fexceptions -fno-rtti -fuse-cxa-atexit -ggdb
