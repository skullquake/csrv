#!/bin/bash
#CC=x86_64-w64-mingw32
CC=x86_64-w64-mingw32-gcc
#-gcc-9.3-posix
find ./libsrc|grep -i makefile|while read PTH;do
	echo making $PTH
	CC=$CC make -C `dirname $PTH`
done
CC=$CC make

