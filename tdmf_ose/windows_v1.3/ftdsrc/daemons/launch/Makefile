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

PROGRAMS=${Q}launchpmd ${Q}launchrefresh ${Q}launchbackfresh

OBJS=ftd_launch_pmd.o ftd_launch_refresh.o \
ftd_launch_backfresh.o 

LIBS=${LIBFTD} ${LIBUTIL} ${LIBSOCK} ${LIBLST} ${LIBCOMP} \
${LIBPROC} ${LIBERR} ${LIBLIC} ${SOCKET_LIBS}

CINCLUDES=${INCDEVICE} ${INCUTIL} ${INCFTD} ${INCLST} ${INCPROC} \
${INCCOMP} ${INCSOCK} ${INCERR} ${INCLIC}

CDEFINES+=-DHAVE_UNISTD_H

include ${TOP}/mk/prog.gmk

${Q}launchpmd:	${DEPLIBFTD} ftd_launch_pmd.o
	${CC} -o $@ ${CFLAGS} ${INCLUDES} ftd_launch_pmd.o ${LIBS}

${Q}launchrefresh:	${DEPLIBFTD} ftd_launch_refresh.o
	${CC} -o $@ ${CFLAGS} ${INCLUDES} ftd_launch_refresh.o ${LIBS}

${Q}launchbackfresh:	${DEPLIBFTD} ftd_launch_backfresh.o
	${CC} -o $@ ${CFLAGS} ${INCLUDES} ftd_launch_backfresh.o ${LIBS}

ftd_launch_pmd.o:	ftd_launch.c
	${CC} ${CFLAGS} -DFTD_LAUNCH_NORMAL -c $? -o $@
 
ftd_launch_refresh.o:	ftd_launch.c
	${CC} ${CFLAGS} -DFTD_LAUNCH_REFRESH -c $? -o $@
 
ftd_launch_backfresh.o:	ftd_launch.c
	${CC} ${CFLAGS} -DFTD_LAUNCH_BACKFRESH -c $? -o $@

