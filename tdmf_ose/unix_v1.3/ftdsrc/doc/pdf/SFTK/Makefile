#
# Copyright (c) 2000 FullTime Software, Inc.  All rights reserved.
#

SYSTYPE:= $(shell uname -s)

ifeq (SunOS,$(SYSTYPE))
    SUBDIRS = HP_SUN 
else
    SUBDIRS = HP_SUN
endif

PDF_INSTALL=`echo *.pdf`
TOP=../../..

include ${TOP}/mk/subdir.gmk

