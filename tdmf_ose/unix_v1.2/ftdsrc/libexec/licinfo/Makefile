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

PROGRAM=licinfo
OBJS=licinfo.o
CINCLUDES=${INCGEN} ${INCDEVICE} ${INCLIC}
LIBS=${LIBGEN} ${LIBLIC} ${SOCKET_LIBS} ${LVM_LIBS}
BIN_INSTALL=${Q}licinfo
include ${TOP}/mk/prog.gmk
EXTRA_CLEANS+=licinfo.c
licinfo.c: licinfo.src.c
	$(PKGIFY)
