#!/bin/sh

# return an OS version number (in the hundreds)
export unamevers;

unamesys=`uname`
if [ "$unamesys" = "AIX" ]
then
	unamev=`uname -v`
	unamer=`uname -r`
	unamevers=`expr $unamev \* 10 + $unamer`
elif [ "$unamesys" = "Linux" ]; then
	unamevers=`uname -r | sed -e '\@\.@ s@@@g' | tr -d '[A-Z]' | sed -e 's/-.*//'`
else
	unamevers=`uname -r | sed -e '\@\.@ s@@@g' | tr -d '[A-Z]'`
fi

while /bin/true
do
	# change for Solaris 10 porting
	if [ "$unamesys" = "SunOS" ]
	then
        # make 5.10 as 5100 rather than 510
		if [ $unamevers -lt 511 ]
		then
			unamevers=`expr $unamevers \* 10`;
		else
			break;
		fi
	else
		if [ $unamevers -lt 100 ]
		then
			unamevers=`expr $unamevers \* 10`;
		else
			break;
		fi
	fi
done

echo ${unamevers};
exit 0
