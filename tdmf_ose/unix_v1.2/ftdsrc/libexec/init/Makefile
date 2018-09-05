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

PROGRAM=init
OBJS=ftd_init.o 
LIBS=${LIBGEN} ${SOCKET_LIBS} ${LVM_LIBS}
BIN_INSTALL=${Q}init
CINCLUDES=${INCGEN} ${INCDEVICE}

include ${TOP}/mk/prog.gmk
