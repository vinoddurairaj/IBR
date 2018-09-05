#  
# Makefile for daemons and support utilities for FullTime Data for Unix
#
# Copyright (c) 1997, 1998 FullTime Software, Inc.  All Rights Reserved
#
# $Id: Makefile,v 1.23 2014/09/15 20:25:44 paulclou Exp $
#

TOP=../..
CDEFINES= 
CINCLUDES=-I. $(INCDEVICE) ${INCLIC} ${INCMD5} ${INCGEN}
LIBS= ${LIBGEN} ${LIBLIC} ${SOCKET_LIBS} ${THREAD_LIB} ${LVM_LIBS}
HDRS	=
OBJS	=
PMD_OBJS = pmd.o
FTD_OBJS = ftdd.o
UTIL_OBJS= startpmd.o startrfd.o killpmd.o killrmd.o \
    killrfd.o netanalysis.o stopnetanalysis.o
# Note: backfresh no longer supported as of TDMF IP 2.8.0 (startbfd.o and killbfd.o)
RMD_OBJS = rmd.o
THROT_OBJS = throtd.o
RMDRECO_OBJS = rmdreco.o
CTL_OBJS=$(Q)ctl.o

PROGRAMS=in.${Q} in.pmd in.rmd throtd ${Q}rmdreco \
	${Q}pmd ${Q}refresh \
	${Q}killpmd ${Q}killrmd \
	${Q}killrefresh ${Q}netanalysis ${Q}stopnetanalysis \
	p000.cfg.sample chlink
# Note: ${Q}backfresh and ${Q}killbackfresh removed as of TDMF IP 2.8.0
BIN_INSTALL=in.${Q} in.pmd in.rmd throtd \
	${Q}rmdreco \
	${Q}pmd ${Q}refresh ${Q}killpmd ${Q}killrmd \
	${Q}killrefresh ${Q}netanalysis ${Q}stopnetanalysis
# Note: ${Q}backfresh and ${Q}killbackfresh removed as of TDMF IP 2.8.0

EXTRA_CLEANS=$(PMD_OBJS) $(RMD_OBJS) $(FTD_OBJS) $(RMDRECO_OBJS) \
	$(UTIL_OBJS) $(THROT_OBJS) \
	$(Q)ctl.o $(Q)ctl
SAMPLE_INSTALL=p000.cfg.sample chlink

include ${TOP}/mk/Makefile.inc
include ${TOP}/mk/debug.gmk

ifeq (HP-UX,$(SYSTYPE))

ifeq (ipf,$(ARCH))
   GDBLINK=
   LINKEROPT=
else
   GDBLINK=/opt/langtools/lib/end.o
   LINKEROPT=-Wl,-a,archive
endif

else
GDBLINK=
endif

ifeq (Linux,$(SYSTYPE))
CDEFINES=$(UDEFINES)
COPTFLAGS+=$(UCOPTFLAGS)
CINCLUDES+=$(UINCLUDES)
endif
ifdef DEBUG
CDEFINES += $(UDEBUG)
COPTFLAGS += -g
endif
LDFLAGS = $(UTARGET) ${LINKOPTS}

mt: 
	$(MAKE) all CDEFINES+="-DMT"  

opt:: clean
	$(MAKE) all COPTFLAGS="-xO2"

in.pmd: ${PMD_OBJS} ${DEPLIBLIC} ${DEPLIBGEN}
	$(CC) $(LDFLAGS) -o $@ $(CFLAGS) $(INCLUDES) $(PMD_OBJS) $(LIBS) ${V}

in.rmd: ${RMD_OBJS} ${DEPLIBLIC} ${DEPLIBGEN}
	$(CC) $(LDFLAGS) -o $@ $(CFLAGS) $(INCLUDES) $(RMD_OBJS) $(LIBS) ${V}

$(Q)ctl: $(Q)ctl.o  ${DEPLIBLIC} ${DEPLIBGEN}
	$(CC) $(LDFLAGS) -o $@ $(CFLAGS) $(LINKEROPT) $(INCLUDES) $(Q)ctl.o $(LIBS) ${V}

in.$(Q): ${FTD_OBJS} ${DEPLIBLIC} ${DEPLIBGEN}
	$(CC) $(LDFLAGS) -o $@ $(CFLAGS) $(INCLUDES) $(FTD_OBJS) $(LIBS) ${V}

throtd: ${THROT_OBJS} ${DEPLIBLIC}
	$(CC) $(LDFLAGS) -o $@ $(CFLAGS) $(INCLUDES) $(THROT_OBJS) $(LIBS) ${V}

$(Q)rmdreco: ${RMDRECO_OBJS} ${DEPLIBLIC} ${DEPLIBGEN}
	$(CC) $(LDFLAGS) -o $@ ${RMDRECO_OBJS} $(LIBS) ${V}

$(Q)pmd: ${OBJS} startpmd.o ${DEPLIBLIC} ${DEPLIBGEN}
	$(CC) $(LDFLAGS) -o $@ startpmd.o ${OBJS} $(LIBS) ${V} ${GDBLINK}

# backfresh removed as of TDMF IP 2.8.0
# $(Q)backfresh: ${OBJS} startbfd.o ${DEPLIBLIC} ${DEPLIBGEN}
#	$(CC) $(LDFLAGS) -o $@ startbfd.o ${OBJS} $(LIBS) ${V}

$(Q)refresh: ${OBJS} startrfd.o ${DEPLIBLIC} ${DEPLIBGEN}
	$(CC) $(LDFLAGS) -o $@ startrfd.o ${OBJS} $(LIBS) ${V}


$(Q)killpmd: ${OBJS} killpmd.o ${DEPLIBLIC} ${DEPLIBGEN}
	$(CC) $(LDFLAGS) -o $@ killpmd.o ${OBJS} $(LIBS) ${V}

$(Q)killrmd: ${OBJS} killrmd.o ${DEPLIBLIC} ${DEPLIBGEN}
	$(CC) $(LDFLAGS) -o $@ killrmd.o ${OBJS} $(LIBS) ${V}

# killbackfresh removed as of TDMF IP 2.8.0
# $(Q)killbackfresh: ${OBJS} killbfd.o ${DEPLIBLIC} ${DEPLIBGEN}
#	$(CC) $(LDFLAGS) -o $@ killbfd.o ${OBJS} $(LIBS) ${V}

$(Q)killrefresh: ${OBJS} killrfd.o ${DEPLIBLIC} ${DEPLIBGEN}
	$(CC) $(LDFLAGS) -o $@ killrfd.o ${OBJS} $(LIBS) ${V}

$(Q)netanalysis: ${OBJS} netanalysis.o ${DEPLIBLIC} ${DEPLIBGEN}
	$(CC) $(LDFLAGS) -o $@ netanalysis.o ${OBJS} $(LIBS) ${V}

$(Q)stopnetanalysis: ${OBJS} stopnetanalysis.o ${DEPLIBLIC} ${DEPLIBGEN}
	$(CC) $(LDFLAGS) -o $@ stopnetanalysis.o ${OBJS} $(LIBS) ${V}

${Q}ctl.o: ctld.c ${HDRS}
	$(CC) ${CFLAGS} $(LINKEROPT) -c -o $@ ctld.c $(CINCLUDES)

ftdd.o: ftdd.c $(HDRS)
pmd.o: pmd.c $(HDRS)
rmd.o: rmd.c $(HDRS)
throtd.o: throtd.c $(HDRS) 
licinfo.o: licinfo.c $(HDRS)

startpmd.o: start.c
	${CC} ${CFLAGS} -DSTARTPMD -c $? -o $@
# startbfd.o removed as of TDMF IP 2.8.0
# startbfd.o: start.c
# 	${CC} ${CFLAGS} -DSTARTBFD -c $? -o $@
startrfd.o: start.c
	${CC} ${CFLAGS} -DSTARTRFD -c $? -o $@
netanalysis.o: netanalysis.c
	${CC} ${CFLAGS} -c $? -o $@
killpmd.o: kill.c
	${CC} ${CFLAGS} -DKILLPMD -c $? -o $@
killrmd.o: kill.c
	${CC} ${CFLAGS} -DKILLRMD -c $? -o $@
# killbfd.o removed as of TDMF IP 2.8.0
# killbfd.o: kill.c
# 	${CC} ${CFLAGS} -DKILLBFD -c $? -o $@
killrfd.o: kill.c
	${CC} ${CFLAGS} -DKILLRFD -c $? -o $@
stopnetanalysis.o: stopnetanalysis.c
	${CC} ${CFLAGS} -c $? -o $@

chlink:chlink.sh
	$(PKGIFY)
	@chmod 444 $@

p000.cfg.sample:p000.cfg.sample.${SYSTYPE}
	$(PKGIFY)
	@chmod 444 $@

dtc.cscope: ftdd.cscope
	$(RM) $@
	ln ftdd.cscope $@

%.cscope: %.c
	$(CSCOPE) -b -f $@ $(CINCLUDES) $<

CSCOPE_APPS = pmd rmd dtc ftdd

cscope: $(CSCOPE_APPS:%=%.cscope)

cscope_clean:
	$(RM) $(CSCOPE_APPS:%=%.cscope)
