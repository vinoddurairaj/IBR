#
#	Copyright (c) 1997, 1998 FullTime Software, Inc.  All rights reserved.
#

SUBDIRS=libgen liblic libmd5
TOP=..

include ${TOP}/mk/subdir.gmk

# XXX This is now bogus, but we'll fix in a little bit
#install::
#@cp -rp blt2.4 tix4.1 tcl8.0 tk8.0 ${LIBDIR}

#
# copy, resolving symlinks. also, skip CVS directories...
#
install::
	tar chf - blt2.4 tix4.1 tcl8.0 tk8.0 | (cd ${LIBDIR}; tar xf -);
	
