#
# Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
#
# RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
# Government is subject to restrictions as set forth in
# subparagraph (c)(1)(ii) of the Rights in Technical Data and
# Computer Software clause at DFARS 52.227-7013 and in similar
# clauses in the FAR and NASA FAR Supplement.
#
TOP=../..

PROGRAM=start
OBJS=ftd_start.o 
LIBS=${LIBGEN} ${SOCKET_LIBS} ${LVM_LIBS}
BIN_INSTALL=${Q}start
CINCLUDES=${INCGEN} ${INCDEVICE}

include ${TOP}/mk/prog.gmk

ifeq (HP-UX,$(SYSTYPE))
    SBIN_INSTALL=${Q}start
    ifeq (ipf,$(ARCH))
       LINKOPTS=
    else
       LINKOPTS=-Wl,-a,archive
    endif
else
    USRSBIN_INSTALL=${Q}start
endif

ifeq (Linux,$(SYSTYPE))
    SBIN_INSTALL=${Q}start
endif

SRCS = $(OBJS:%.o=%.c)
