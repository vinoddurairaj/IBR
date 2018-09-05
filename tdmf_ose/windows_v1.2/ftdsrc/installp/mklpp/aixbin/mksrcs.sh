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
		echo $x
	fi
	
done

cd ${here};

exit 0;
