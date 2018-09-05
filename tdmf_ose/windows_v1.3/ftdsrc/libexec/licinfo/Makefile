#
# Copyright (c) 1999 Legato Systems, Inc.  All Rights Reserved.
#
# RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
# Government is subject to restrictions as set forth in
# subparagraph (c)(1)(ii) of the Rights in Technical Data and
# Computer Software clause at DFARS 52.227-7013 and in similar
# clauses in the FAR and NASA FAR Supplement.
#
#
TOP=../..

PROGRAM=licinfo
OBJS=ftd_licinfo.o 

LIBS=${LIBLST} ${LIBFTD} ${LIBSOCK} ${LIBUTIL} ${LIBCOMP} \
${LIBPROC} ${LIBLIC} ${LIBERR} ${SOCKET_LIBS}

CINCLUDES=${INCERR} ${INCDEVICE} ${INCUTIL} ${INCFTD} ${INCLST} \
${INCPROC} ${INCCOMP} ${INCSOCK} ${INCLIC}

CDEFINES+=-DHAVE_UNISTD_H

include ${TOP}/mk/prog.gmk

