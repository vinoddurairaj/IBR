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

PROGRAM=stop

OBJS=ftd_stop.o 

LIBS=${LIBFTD} ${LIBLST} ${LIBSOCK} ${LIBPROC} ${LIBERR} ${LIBUTIL} \
${LIBCOMP} ${SOCKET_LIBS} ${LVM_LIBS}

BIN_INSTALL=${Q}stop

USRSBIN_INSTALL=${Q}stop

CINCLUDES=${INCFTD} ${INCERR} ${INCDEVICE} ${INCLST} ${INCSOCK} \
${INCCOMP} ${INCPROC}

CDEFINES+=-DHAVE_UNISTD_H

include ${TOP}/mk/prog.gmk
