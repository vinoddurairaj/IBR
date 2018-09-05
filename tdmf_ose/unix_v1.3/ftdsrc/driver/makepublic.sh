#!/bin/ksh

export isksym

# these are evidently declared local
echo Undefined Symbols Satisfied by /dev/ksyms: 

nm *.o | grep FUNC | grep UNDEF |\
awk -F'|' '{ printf "%s\n", $8}'|\
sort | uniq  1> ksyms.undefined

rm ksyms.defined
nm *.o | grep FUNC | grep UNDEF |\
awk -F'|' '{ printf "%s\n", $8}' |\
sort | uniq |\
while read sym
do
	isksym=0;
	# look it up as a kern sym
	nm /dev/ksyms | grep $sym | grep ABS | grep GLOB |\
	awk -F'|' '{ printf "%s\n", $8}' |\
	while read ksym
	do
		if [ $ksym = $sym ]
		then
			echo $sym 
			echo $sym  1>> ksyms.defined
			break; 
		fi
	done
done

echo Symbols that need to be declared public in thier modules:
egrep -v -f ksyms.defined ksyms.undefined


