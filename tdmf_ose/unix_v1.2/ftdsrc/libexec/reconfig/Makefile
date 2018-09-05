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

PROGRAM=reconfig
OBJS=ftd_reconfig.o 
LIBS=${LIBGEN} ${SOCKET_LIBS} ${LVM_LIBS}
BIN_INSTALL=${Q}reconfig
SYSTYPE:= $(shell uname -s)
ifeq (HP-UX,$(SYSTYPE))
    SBIN_INSTALL=${Q}reconfig
    LINKOPTS=-Wl,-a,archive
else
    USRSBIN_INSTALL=${Q}reconfig
endif
CINCLUDES=${INCGEN} ${INCDEVICE}

include ${TOP}/mk/prog.gmk
