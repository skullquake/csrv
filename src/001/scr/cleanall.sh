#!/bin/bash
CC=gcc
find ./libsrc|grep -i makefile|while read PTH;do
	CC=$CC make -C `dirname $PTH` clean
done
CC=$CC make clean
CC=x86_64-w64-mingw32-gcc
find ./libsrc|grep -i makefile|while read PTH;do
	CC=$CC make -C `dirname $PTH` clean
done
CC=$CC make clean

