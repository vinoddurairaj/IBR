#
# Copyright (c) 1998 FullTime Software, Inc. All Rights Reserved.
#
TOP=../..

OBJS=md5c.o
CDEFINES = -DMD=5 $(UDEFINES)
CINCLUDES += $(UINCLUDES)
ifndef COPTFLAGS
COPTFLAGS = $(UCFLAGS)
endif
LIBRARY=md5

BUILDBRAND=$(shell ${TOP}/mk/getosarch.sh)
ifeq (${BUILDBRAND},)
$(error Unable to collect the operating system architecture information)
endif

ifeq ($(wildcard ${TOP}/mk/vars_tdmf_${BUILDBRAND}.gmk),)
include ${TOP}/mk/lib.gmk
else
include ${TOP}/mk/lib_tdmf.gmk
endif
