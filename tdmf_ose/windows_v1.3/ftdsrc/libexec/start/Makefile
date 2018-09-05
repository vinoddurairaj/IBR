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

PROGRAM=start

OBJS=ftd_start.o 

LIBS=${LIBFTD} ${LIBLST} ${LIBCOMP} ${LIBSOCK} ${LIBLST} ${LIBUTIL} \
${LIBERR} ${LIBPROC} ${SOCKET_LIBS} ${LVM_LIBS}

BIN_INSTALL=${Q}start

SYSTYPE:= $(shell uname -s)
ifeq (HP-UX,$(SYSTYPE))
    SBIN_INSTALL=${Q}start
    LINKOPTS=-Wl,-a,archive
else
    USRBIN_INSTALL=${Q}start
endif

USRSBIN_INSTALL=${Q}start

CINCLUDES=${INCFTD} ${INCDEVICE} ${INCSOCK} ${INCLST} ${INCCOMP} \
${INCPROC} ${INCERR}

CDEFINES+=-DHAVE_UNISTD_H

include ${TOP}/mk/prog.gmk

