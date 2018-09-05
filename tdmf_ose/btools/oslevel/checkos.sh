#!/bin/sh

if [ `uname` = AIX ]
then
	OSlevel=`uname -sv`
	echo IF  :  $OSlevel
else
	OSlevel=`uname -rs`
	echo ELSE  :  $OSlevel
fi

echo $OSlevel

