#!/bin/bash
LIBS=`find ./lib|cut -d"/" -f3|cut -d"-" -f1|cut -b4-|grep -v "^$"|sort|uniq`
make run $LIBS
