#
# Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
#
# RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
# Government is subject to restrictions as set forth in
# subparagraph (c)(1)(ii) of the Rights in Technical Data and
# Computer Software clause at DFARS 52.227-7013 and in similar
# clauses in the FAR and NASA FAR Supplement.
#
#
#
#
#
TOP=../..

PROGRAM=syncfs
OBJS=ftd_syncfs.o 
BIN_INSTALL=${Q}syncfs
SYSTYPE:= $(shell uname -s)

include ${TOP}/mk/prog.gmk

ifeq (AIX,$(SYSTYPE))
  HAS_SYNCVFS:= $(shell test -f /usr/include/sys/syncvfs.h; echo $$?)
  ifeq (0,$(HAS_SYNCVFS))
    CFLAGS += -DHAS_SYNCVFS
  endif
  CFLAGS += -brtl -blazy 
endif
