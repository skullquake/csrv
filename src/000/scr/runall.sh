#!/bin/bash
LIBS=`find ./lib|cut -d"/" -f3|cut -d"-" -f1|cut -b4-|grep -v "^$"|sort|uniq`
#find ./bin|grep "\.out"|while read PRG;do
find ./bin|while read PRG;do
	echo ./$PRG $LIBS
	./$PRG $LIBS
	echo --------------------------------------------------------------------------------
done
