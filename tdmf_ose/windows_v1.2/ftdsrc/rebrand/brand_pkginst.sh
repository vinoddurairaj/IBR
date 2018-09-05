#!/bin/ksh

#
# this one's simple, just blow away pkg.install/prototype.doc
#

rm -f ../pkg.install/prototype.doc
cd ../pkg.install
gmake prototype.doc
