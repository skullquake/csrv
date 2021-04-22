#!/bin/bash
DOSBOX=/mnt/c/opt/dosbox/megabuild/6/dosbox.exe
DOSBOX=/mnt/c/opt/dosbox/daum/20150125/dosbox.exe
DOSROOT=/mnt/c/tmp/dosdrive
DEPLOYDIR=$DOSROOT/tmp/dxe
rm -rf $DEPLOYDIR
mkdir -p $DEPLOYDIR
cp -r ./build/i586-pc-msdosdjgpp/* $DEPLOYDIR
$DOSBOX\
	-noconsole\
	-nogui\
	-nomenu\
	-showcycles\
	-c "mount c c:/tmp/dosdrive"\
	-c "c:"\
	-c "call AUTOEXEC.BAT"\
	-c "clear"\
	-c "cd c:\\tmp\\dxe"\
	-c ".\\run.bat"

