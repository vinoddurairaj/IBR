#
# Copyright (c) 1997, 1998 FullTime Software, Inc.  All rights reserved.
#
# $Id: Makefile,v 1.5 2001/08/17 18:40:31 bouchard Exp $
#

TOP=..

CDEFINES= -D_KERNEL -DKERNEL  

EXTRA_CLEANS=${Q}.conf *.lst ${Q}.?map ${Q}.map ${Q}.exp version.o

include ${TOP}/Makefile.inc

# ifeq (SunOS,$(SYSTYPE))
# ifeq ($(SYSVERS),570)
# SUBDIRS=sparcv8 sparcv9
# else
# SUBDIRS=sparcv8
# endif
# endif

ifeq (SunOS,$(SYSTYPE))
SUBDIRS=sparcv8
ifeq ($(SYSVERS),570)
SUBDIRS+=sparcv9
endif
ifeq ($(SYSVERS),580)
SUBDIRS+=sparcv9
endif
endif

# DTurrin - Reversed the "if" statement
#           to check for version 10.20
#           first instead of 11.00
#           (August 16th, 2001)
ifeq (HP-UX,$(SYSTYPE))
ifeq ($(SYSVERS),1020)
SUBDIRS=Pa1020
else
SUBDIRS=Pa1100_32 Pa1100_64
endif
endif

