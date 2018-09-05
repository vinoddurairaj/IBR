#!/bin/sh
#
# /%OPTDIR%/%PKGNM%/bin/%Q%usersguide
#
# This script will bring up the FullTime Data User's Guide in the netscape
# browser (if both are found on the system.)
#
# Copyright (c) 1997 FullTime Software, Inc.
#
a="-"
b=`echo $PATH | tr -s ':' ' '`
c=" /%OPTDIR%/netscape /usr/local/netscape /usr/local/bin /usr/bin/netscape"
paths="${b} ${c}"
for i in $paths
do
	if [ -f $i/netscape ]; then
		a="$i/netscape"
		break
	fi
done
if [ $a != "-" ]; then
	if [ -f "/%OPTDIR%/%PKGNM%/doc/html/frames.htm" ]; then
		echo "%Q%usersguide:  launching netscape..."
		(cd $HOME; $a /%OPTDIR%/%PKGNM%/doc/html/frames.htm) &
	else
		echo "%Q%usersguide:  %PRODUCTNAME% User\'s Guide not installed"
	fi
else
	echo "%Q%usersguide:  could not find netscape in PATH"
fi
exit 0


