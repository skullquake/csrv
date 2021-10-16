#!/bin/bash
CC=gcc
find ./libsrc|grep -i makefile|while read PTH;do
	CC=$CC make -C `dirname $PTH`
done
CC=$CC make
