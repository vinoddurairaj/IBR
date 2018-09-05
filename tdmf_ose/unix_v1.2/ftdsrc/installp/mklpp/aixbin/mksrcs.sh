#!/bin/sh

top=$1;

here=`pwd`;

cd ${top};

# analyze the source, listing only regular files.
# resolving what may be symlinks in the process.
# tar(1) can be used to good effect for this.

tar cvhf - . 2>&1 1> /dev/null | awk '{print $2}' |\
while read x
do
	if [ -f $x ]
	then
		echo $x|egrep 'tk8.4|tcl8.4|tix8.1|driver/Linux|driver/Pa1|driver/sparc|/CVS/|/qa/|^./buildroot/|^./CDROMDIR/|driver/hpux' 2>&1 1> /dev/null
		if [ $? -ne 0 ]
		then 
		echo $x
		fi
	fi
	
done

cd ${here};

exit 0;
