#!/bin/sh
#
# /%OPTDIR%/%PKGNM%/bin/%Q%usersguide
#
# This script will launch the %PRODUCTNAME% users' guide in Adobe Acrobat      
# Reader if found.
#
#
a="-"
b=`echo $PATH | tr -s ':' ' '`
c=" /%OPTDIR%/Acrobat4/bin /usr/local/Acrobat4 /usr/local/bin /usr/bin/Acrobat4"
paths="${b} ${c}"
for i in $paths
do
	if [ -f $i/acroread ]; then
		a="$i/acroread"
		break
	fi
done
if [ $a != "-" ]; then
	if [ -d "/%OPTDIR%/%PKGNM%/doc/pdf" ]; then
		echo "%Q%usersguide:  launching Acrobat Reader..."
		(cd $HOME; $a /%OPTDIR%/%PKGNM%/doc/pdf/*.pdf) &
	else
		echo "%Q%usersguide:  %PRODUCTNAME% User's Guide not installed"
	fi
else
	echo "%Q%usersguide:  could not find acroread in PATH"
fi
exit 0


