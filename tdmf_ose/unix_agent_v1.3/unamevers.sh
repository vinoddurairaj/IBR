#!/bin/sh

# return an OS version number (in the hundreds)
export unamevers;

unamesys=`uname`
if [ "$unamesys" = "AIX" ]
then
	unamev=`uname -v`
	unamer=`uname -r`
	unamevers=`expr $unamev \* 10 + $unamer`
else
unamevers=`uname -r | sed -e '\@\.@ s@@@g' | tr -d '[A-Z]' | sed -e 's/-.*//'`
fi
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
