#!/bin/sh

# script depending on xxdiff tool
#
# input 2 directories

DIR1=`pwd`/$1
DIR2=`pwd`/$2

cd $DIR1

# compare all *.[ch] files  other than the agent code
#
#for f in `find . -name \*.[ch] | cut -b 3- | grep -v '/agent/' `; do
#    echo ----- diff $f in $DIR1 and $DIR2
#    xxdiff $f $DIR2/$f
#done

# compare all *.[ch] files

for f in `find . -name \*.[ch] | cut -b 3- `; do
    echo ----- xxdiff $DIR1/$f and $DIR2/$f
    xxdiff $f $DIR2/$f
done
