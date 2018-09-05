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

PROGRAMS=${Q}killpmd ${Q}killrmd ${Q}killrefresh ${Q}killbackfresh
OBJS=ftd_kill_pmd.o ftd_kill_rmd.o ftd_killrefresh.o \
ftd_kill_backfresh.o 

LIBS=${LIBFTD} ${LIBUTIL} ${LIBSOCK} ${LIBLST} ${LIBCOMP} \
${LIBPROC} ${LIBERR} ${LIBLIC} ${SOCKET_LIBS}

CINCLUDES=${INCDEVICE} ${INCUTIL} ${INCFTD} ${INCLST} ${INCPROC} \
${INCCOMP} ${INCSOCK} ${INCERR} ${INCLIC}

CDEFINES+=-DHAVE_UNISTD_H

include ${TOP}/mk/prog.gmk

${Q}killpmd:	${DEPLIBFTD} ftd_kill_pmd.o
	${CC} -o $@ ${CFLAGS} ${INCLUDES} ftd_kill_pmd.o ${LIBS}

${Q}killrmd:	${DEPLIBFTD} ftd_kill_rmd.o
	${CC} -o $@ ${CFLAGS} ${INCLUDES} ftd_kill_rmd.o ${LIBS}

${Q}killrefresh:	${DEPLIBFTD} ftd_kill_refresh.o
	${CC} -o $@ ${CFLAGS} ${INCLUDES} ftd_kill_refresh.o ${LIBS}

${Q}killbackfresh:	${DEPLIBFTD} ftd_kill_backfresh.o
	${CC} -o $@ ${CFLAGS} ${INCLUDES} ftd_kill_backfresh.o ${LIBS}

ftd_kill_pmd.o:	ftd_kill.c
	${CC} ${CFLAGS} -DFTD_KILL_PMD -c $? -o $@
 
ftd_kill_rmd.o:	ftd_kill.c
	${CC} ${CFLAGS} -DFTD_KILL_RMD -c $? -o $@
 
ftd_kill_refresh.o:	ftd_kill.c
	${CC} ${CFLAGS} -DFTD_KILL_REFRESH -c $? -o $@
 
ftd_kill_backfresh.o:	ftd_kill.c
	${CC} ${CFLAGS} -DFTD_KILL_BACKFRESH -c $? -o $@
 
