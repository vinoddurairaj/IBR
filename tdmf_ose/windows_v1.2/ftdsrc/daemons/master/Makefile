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

PROGRAMS=in.${Q}
OBJS=ftd_master.o 

LIBS=${LIBFTD} ${LIBUTIL} ${LIBSOCK} ${LIBLST} ${LIBCOMP} \
${LIBPROC} ${LIBERR} ${SOCKET_LIBS}

CINCLUDES=${INCDEVICE} ${INCUTIL} ${INCFTD} ${INCLST} ${INCPROC} \
${INCCOMP} ${INCSOCK} ${INCERR}

CDEFINES+=-DHAVE_UNISTD_H

include ${TOP}/mk/prog.gmk

in.${Q}:	${OBJS} \
${DEPLIBERR} ${DEPLIBFTD} ${DEPLIBSOCK} ${DEPLIBLST} ${DEPLIBMD5} \
${DEPLIBUTIL} ${DEPLIBLIC} ${DEPLIBCOMP} ${DEPLIBPROC}
	${CC} -o $@ ${CFLAGS} ${INCLUDES} ${OBJS} ${LIBS}

