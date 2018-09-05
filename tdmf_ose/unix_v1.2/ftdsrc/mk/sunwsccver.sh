#!/bin/sh

# what version of SUNWS cc?
sunwsccver=`cc -V 2>&1 | egrep '(^cc)' | awk '{printf "%s", $NF}'`;
echo ${sunwsccver};
exit 0;
