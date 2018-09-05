#  
# Makefile for daemons and support utilities for FullTime Data for Unix
#
# Copyright (c) 1997, 1998 FullTime Software, Inc.  All Rights Reserved
#
# $Id: Makefile,v 1.1.1.1 2001/09/14 13:43:53 dturrin Exp $
#

TOP=../..
CDEFINES= 
CINCLUDES=-I. -I${TOP}/driver ${INCLIC} ${INCMD5} ${INCGEN}
LIBS= ${LIBGEN} ${LIBLIC} ${SOCKET_LIBS} ${THREAD_LIB} ${LVM_LIBS}
HDRS	=
OBJS	=
PMD_OBJS = pmd.o
FTD_OBJS = ftdd.o
UTIL_OBJS= startpmd.o startbfd.o startrfd.o killpmd.o killrmd.o \
    killbfd.o killrfd.o \
	updatepmd.o 
RMD_OBJS = rmd.o
THROT_OBJS = throtd.o
RMDRECO_OBJS = rmdreco.o
CTL_OBJS=$(Q)ctld.o

SYSTYPE:= $(shell uname -s)
ifeq (HP-UX,$(SYSTYPE))
CTL_BIN=$(Q)ctld
else
CTL_BIN=
endif

PROGRAMS=in.${Q} in.pmd in.rmd throtd ${Q}rmdreco \
	${Q}pmd ${Q}backfresh ${Q}refresh \
	${Q}killpmd ${Q}killrmd ${Q}killbackfresh \
	${Q}killrefresh ${Q}updatepmd \
	${CTL_BIN}
BIN_INSTALL=in.${Q} in.pmd in.rmd throtd \
	${Q}rmdreco \
	${Q}pmd ${Q}backfresh ${Q}refresh ${Q}killpmd ${Q}killrmd \
	${Q}killbackfresh ${Q}killrefresh \
	${Q}updatepmd \
	${CTL_BIN}

EXTRA_CLEANS=$(PMD_OBJS) $(RMD_OBJS) $(FTD_OBJS) $(RMDRECO_OBJS) \
	$(UTIL_OBJS) $(THROT_OBJS) \
	$(Q)ctld.o $(Q)ctld
SAMPLE_INSTALL=p000.cfg.sample

include ${TOP}/Makefile.inc
include ${TOP}/mk/debug.gmk

mt: 
	$(MAKE) all CDEFINES+="-DMT"  

opt:: clean
	$(MAKE) all COPTFLAGS="-xO2"

in.pmd: ${PMD_OBJS} ${DEPLIBLIC} ${DEPLIBGEN}
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) $(PMD_OBJS) $(LIBS) ${V}

in.rmd: ${RMD_OBJS} ${DEPLIBLIC} ${DEPLIBGEN}
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) $(RMD_OBJS) $(LIBS) ${V}

$(Q)ctld: $(Q)ctld.o ${DEPLIBGEN}
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) $(Q)ctld.o $(LIBS) ${V}

in.$(Q): ${FTD_OBJS} ${DEPLIBLIC}
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) $(FTD_OBJS) $(LIBS) ${V}

throtd: ${THROT_OBJS} ${DEPLIBLIC}
	$(CC) -o $@ $(CFLAGS) $(INCLUDES) $(THROT_OBJS) $(LIBS) ${V}

$(Q)rmdreco: ${RMDRECO_OBJS}
	$(CC) -o $@ ${RMDRECO_OBJS} $(LIBS) ${V}

$(Q)pmd: ${OBJS} startpmd.o
	$(CC) -o $@ startpmd.o ${OBJS} $(LIBS) ${V}

$(Q)backfresh: ${OBJS} startbfd.o
	$(CC) -o $@ startbfd.o ${OBJS} $(LIBS) ${V}

$(Q)refresh: ${OBJS} startrfd.o
	$(CC) -o $@ startrfd.o ${OBJS} $(LIBS) ${V}

$(Q)updatepmd: ${OBJS} updatepmd.o
	$(CC) -o $@ updatepmd.o ${OBJS} $(LIBS) ${V}

$(Q)killpmd: ${OBJS} killpmd.o
	$(CC) -o $@ killpmd.o ${OBJS} $(LIBS) ${V}

$(Q)killrmd: ${OBJS} killrmd.o
	$(CC) -o $@ killrmd.o ${OBJS} $(LIBS) ${V}

$(Q)killbackfresh: ${OBJS} killbfd.o
	$(CC) -o $@ killbfd.o ${OBJS} $(LIBS) ${V}

$(Q)killrefresh: ${OBJS} killrfd.o
	$(CC) -o $@ killrfd.o ${OBJS} $(LIBS) ${V}

${Q}ctld.o: ctld.c ${HDRS}
	$(CC) ${CFLAGS} -c -o $@ ctld.c $(CINCLUDES)
	
ftdd.o: ftdd.c $(HDRS)
pmd.o: pmd.c $(HDRS)
rmd.o: rmd.c $(HDRS)
throtd.o: throtd.c $(HDRS) 
licinfo.o: licinfo.c $(HDRS)
updatepmd.o: updatepmd.c $(HDRS)

startpmd.o: start.c
	${CC} ${CFLAGS} -DSTARTPMD -c $? -o $@
startbfd.o: start.c
	${CC} ${CFLAGS} -DSTARTBFD -c $? -o $@
startrfd.o: start.c
	${CC} ${CFLAGS} -DSTARTRFD -c $? -o $@
killpmd.o: kill.c
	${CC} ${CFLAGS} -DKILLPMD -c $? -o $@
killrmd.o: kill.c
	${CC} ${CFLAGS} -DKILLRMD -c $? -o $@
killbfd.o: kill.c
	${CC} ${CFLAGS} -DKILLBFD -c $? -o $@
killrfd.o: kill.c
	${CC} ${CFLAGS} -DKILLRFD -c $? -o $@
