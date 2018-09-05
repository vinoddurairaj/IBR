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

PROGRAM=checkpoint
OBJS=ftd_cp.o 
LIBS= ${LIBGEN} ${SOCKET_LIBS} ${LIBMD5} ${LVM_LIBS}
BIN_INSTALL=${Q}checkpoint
USRSBIN_INSTALL=${Q}checkpoint
CINCLUDES=${INCGEN} ${INCDEVICE}

include ${TOP}/mk/prog.gmk
