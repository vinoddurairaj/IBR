#!/bin/sh

# return an OS version number (in the hundreds)

unamevers=`uname -r | sed -e '\@\.@ s@@@g' | tr -d '[A-Z]'`
while /bin/true
do
if [ $unamevers -lt 100 ]
then
	unamevers=`expr $unamevers \* 10`;
else
	break;
fi
done
echo ${unamevers};
exit 0
