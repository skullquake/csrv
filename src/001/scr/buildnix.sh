#!/bin/bash
CC=/usr/bin/gcc
CXX=/usr/bin/g++
find ./libsrc|grep -i makefile|while read PTH;do
	CC=$CC CXX=$CXX make -C `dirname $PTH`
done
CC=$CC CXX=$CXX make -C ./makefile
