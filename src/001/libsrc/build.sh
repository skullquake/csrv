#!/bin/bash
#CC=x86_64-w64-mingw32
CC=/usr/bin/gcc
CXX=/usr/bin/g++
ls -d */|while read DIR;do
	CC=$CC CXX=$CXX make -C $DIR
done
