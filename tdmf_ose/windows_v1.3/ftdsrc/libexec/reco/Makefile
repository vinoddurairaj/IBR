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

PROGRAMS=${Q}reco
OBJS=ftd_reco.o 

LIBS=${LIBFTD} ${LIBUTIL} ${LIBSOCK} ${LIBLST} ${LIBCOMP} \
${LIBPROC} ${LIBERR} ${LIBLIC} ${SOCKET_LIBS}

CINCLUDES=${INCDEVICE} ${INCUTIL} ${INCFTD} ${INCLST} ${INCPROC} \
${INCCOMP} ${INCSOCK} ${INCERR} ${INCLIC}

CDEFINES+=-DHAVE_UNISTD_H

include ${TOP}/mk/prog.gmk

${Q}reco:	${OBJS} \
${DEPLIBERR} ${DEPLIBFTD} ${DEPLIBSOCK} ${DEPLIBLST} ${DEPLIBMD5} \
${DEPLIBUTIL} ${DEPLIBLIC} ${DEPLIBCOMP} ${DEPLIBPROC}
	${CC} -o $@ ${CFLAGS} ${INCLUDES} ${OBJS} ${LIBS}

