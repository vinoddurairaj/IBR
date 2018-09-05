#
#	Copyright (c) 1997, 1998 FullTime Software, Inc.  All rights reserved.
#

SYSTYPE:= $(shell uname -s)

ifeq (SunOS,$(SYSTYPE))
    SUBDIRS = solaris 
else
    SUBDIRS = hpux 
endif

TOP=../..

include ${TOP}/mk/subdir.gmk

