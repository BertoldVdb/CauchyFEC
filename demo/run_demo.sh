#!/bin/sh

g++ -std=c++1y -O3 -Wall -Werror demo.cpp -o demo -L .. -lerasure -I ../src &&
LD_LIBRARY_PATH=.. ./demo
