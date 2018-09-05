#
# Copyright (c) 1997, 1998 FullTime Software, Inc.  All rights reserved.
#
# $Id: Makefile,v 1.24 2010/11/09 14:16:40 proulxm Exp $
#

TOP=$(shell cd ..; pwd)

CDEFINES= -D_KERNEL -DKERNEL  

EXTRA_CLEANS=${Q}.conf ${Q}.?map ${Q}.map ${Q}.exp version.o

include ${TOP}/mk/Makefile.inc

# Solaris 5.6  sparcv8 (32 bit) (not supported)
# Solaris 5.7  sparcv8 sparcv9 (64 bit)
# Solaris 5.8  sparcv8 sparcv9
# Solaris 5.9  sparcv8 sparcv9
# Solaris 5.10 sparcv8 sparcv9
ifeq (SunOS,$(SYSTYPE))
SUBDIRS=sparcv8 sparcv9
endif

ifeq (HP-UX,$(SYSTYPE))
# We Used to support building drivers for 10.20, 11.00 and 11.11 as well.
ifeq ($(SYSVERS),1123)
SUBDIRS=hpux1123_64
endif
ifeq ($(SYSVERS),1131)
SUBDIRS=hpux1131_64
endif
endif

ifeq (AIX,$(SYSTYPE))
# All drivers for 5.1 and after are built in the same manner. There used to be a different method for the 4.3 drivers.
SUBDIRS=aix51
endif

ifeq (Linux,$(SYSTYPE))
SUBDIRS=Linux
# The Makefile link (used to be a copy) is a reminescence of when we had another linux 2.4 specific makefile.
# If we ever move from cvs to something than can keep the history through a file rename, we should just rename Makefile26 to Makefile then.
$(shell cd Linux && ln -s Makefile26 Makefile)
# The following syntax allows to only create the link if needed, but doesn't work in gmake versions 3.80 and before (Suse 10)
# because of the semicolons.
#$(shell cd Linux ; if ! test -h Makefile; then ln -s Makefile26 Makefile;  fi)
endif
