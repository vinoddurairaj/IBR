#!/bin/sh

#
# monitor production builds
#

bld=$1

while /bin/true
do
	clear;
	activ=`find ./* -prune -name "*.LCK" -o -name "*.lck"`;
	echo "=========";
	echo "Lock files:";
	echo $activ | \
	awk '{ for ( i = 1 ; i <= NF ; i = i + 1) {printf "%s\n", $i} } ';
	echo "=========";
	echo "Errata files:"
	echo "=========";
	for x in `echo *${bld}*blderrata`
	do
		echo "---------";
		echo $x:;
		echo "---------";
		tail -2 $x | cut -b1-80;
	done
	sleep 10;
	
done
