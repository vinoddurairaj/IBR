#!/bin/ksh

#
# this one's simple, just blow away pkg.install/prototype.doc
#
if [ "`uname -s`" = "HP-UX" ]; then \
  exit 0; 
fi

rm -f ../pkg.install/prototype.doc
cd ../pkg.install
make prototype.doc
