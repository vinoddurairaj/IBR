#
# Copyright (c) 1997, 1998 FullTime Software, Inc.  All rights reserved.
#
# $Id: Makefile,v 1.3 2001/01/17 17:58:56 hutch Exp $
#

TOP=../..

CDEFINES= -D_KERNEL -DKERNEL  

include ${TOP}/Makefile.inc

whattodo:
	@echo "Making 64-bit sparcv9 driver ${Q}"

COPTFLAGS+=-xarch=v9

all:: whattodo ${Q} ${Q}.conf

#
# solaris drv bld macros
#

# version 8 src and bits

VERSSRC=$(TOP)/version.c

V9HDRS=	../ftd_all.h ../ftd_bab.h ../ftd_bits.h \
	../ftd_buf.h ../ftd_ddi.h ../ftd_def.h \
	../ftd_disk.h ../ftd_kern_cproto.h ../ftd_kern_ctypes.h \
	../ftd_klog.h ../ftd_timers.h ../ftd_var.h ../ftdio.h 

V9SRCS=	../ftd_sun.c ../ftd_buf.c ../ftd_bab.c ../ftd_bits.c \
	../ftd_ioctl.c ../ftd_all.c ../ftd_klog.c ../memset.c \
	$(VERSSRC)

V9OBJS=	ftd_sun.o ftd_buf.o ftd_bab.o ftd_bits.o \
	ftd_ioctl.o ftd_all.o ftd_klog.o memset.o \
	version.o

# SunOSism...
EXTRA_CLEANS+=	${V9OBJS} ${Q} ${Q}.conf master.d-${Q}

#
# installation rules
#
install:: master.d-${Q}
	@mkdir -p $(DESTDIR)/usr/kernel/drv
	@mkdir -p $(DESTDIR)/usr/kernel/drv/sparcv9
	@mkdir -p $(DESTDIR)/etc/init.d
	@mkdir -p $(DESTDIR)/etc/rc3.d
	@${INSTALL} -m 555 ${Q} $(DESTDIR)/usr/kernel/drv/sparcv9
	@${INSTALL} -m 444 ${Q}.conf $(DESTDIR)/usr/kernel/drv
	@${INSTALL} -m 444 ${Q}.conf ${SAMPLEDIR}

#
# obj dep's
#
# $(V9OBJS): $(V9HDRS) $(V9SRCS)

# load v9 driver
${Q}: $(V9OBJS) version.o 
	@echo "Loading 64-bit driver: ${Q}"
	@ld -r -o ${Q}	$(V9OBJS) 

# production rules
ftd_sun.o: ../ftd_sun.c
	${CC} ${CFLAGS} -c $<

ftd_buf.o: ../ftd_buf.c
	${CC} ${CFLAGS} -c $<

ftd_bab.o: ../ftd_bab.c
	${CC} ${CFLAGS} -c $<

ftd_bits.o:  ../ftd_bits.c
	${CC} ${CFLAGS} -c $<

ftd_ioctl.o: ../ftd_ioctl.c
	${CC} ${CFLAGS} -c $<

ftd_all.o: ../ftd_all.c
	${CC} ${CFLAGS} -c $<

ftd_klog.o: ../ftd_klog.c
	${CC} ${CFLAGS} -c $<

memset.o: ../memset.c
	${CC} ${CFLAGS} -c $<

version.o: ${VERSSRC}
	${CC} ${CFLAGS} -c $<

#
# misc targets
#

# driver config file
${Q}.conf: ../driver.conf
	$(PKGIFY)

# hmmm...
master.d-${Q}: ../master.d.src
	$(PKGIFY)

# make deps
ftd_sun.o: ../ftd_sun.c
ftd_sun.o: ../ftd_kern_ctypes.h
ftd_sun.o: ../ftd_ddi.h
ftd_sun.o: ../ftd_def.h
ftd_sun.o: ../ftd_bab.h
ftd_sun.o: ../ftd_def.h
ftd_sun.o: ../ftdio.h
ftd_sun.o: ../ftd_kern_ctypes.h
ftd_sun.o: ../ftd_buf.h
ftd_sun.o: ../ftdio.h
ftd_sun.o: ../ftd_klog.h
ftd_sun.o: ../ftd_bab.h
ftd_sun.o: ../ftd_all.h
ftd_sun.o: ../ftd_klog.h
ftd_sun.o: ../ftd_kern_cproto.h
ftd_buf.o: ../ftd_buf.c
ftd_buf.o: ../ftd_kern_ctypes.h
ftd_buf.o: ../ftd_buf.h
ftd_buf.o: ../ftd_def.h
ftd_buf.o: ../ftd_bab.h
ftd_buf.o: ../ftd_def.h
ftd_buf.o: ../ftdio.h
ftd_buf.o: ../ftd_kern_ctypes.h
ftd_buf.o: ../ftd_buf.h
ftd_buf.o: ../ftdio.h
ftd_buf.o: ../ftd_klog.h
ftd_buf.o: ../ftd_kern_cproto.h
ftd_bab.o: ../ftd_bab.c
ftd_bab.o: ../ftd_kern_ctypes.h
ftd_bab.o: ../ftd_ddi.h
ftd_bab.o: ../ftd_def.h
ftd_bab.o: ../ftd_bab.h
ftd_bab.o: ../ftd_def.h
ftd_bab.o: ../ftdio.h
ftd_bab.o: ../ftd_kern_ctypes.h
ftd_bab.o: ../ftd_buf.h
ftd_bab.o: ../ftdio.h
ftd_bab.o: ../ftd_klog.h
ftd_bab.o: ../ftd_bab.h
ftd_bab.o: ../ftd_klog.h
ftd_bab.o: ../ftd_kern_cproto.h
ftd_bits.o: ../ftd_bits.c
ftd_bits.o: ../ftd_kern_ctypes.h
ftd_bits.o: ../ftd_bab.h
ftd_bits.o: ../ftd_def.h
ftd_bits.o: ../ftd_bab.h
ftd_bits.o: ../ftd_buf.h
ftd_bits.o: ../ftdio.h
ftd_bits.o: ../ftd_kern_ctypes.h
ftd_bits.o: ../ftd_klog.h
ftd_bits.o: ../ftdio.h
ftd_bits.o: ../ftd_bits.h
ftd_bits.o: ../ftd_def.h
ftd_bits.o: ../ftd_def.h
ftd_bits.o: ../ftdio.h
ftd_bits.o: ../ftd_klog.h
ftd_bits.o: ../ftd_kern_cproto.h
ftd_ioctl.o: ../ftd_ioctl.c
ftd_ioctl.o: ../ftd_kern_ctypes.h
ftd_ioctl.o: ../ftd_ddi.h
ftd_ioctl.o: ../ftd_def.h
ftd_ioctl.o: ../ftd_bab.h
ftd_ioctl.o: ../ftd_def.h
ftd_ioctl.o: ../ftdio.h
ftd_ioctl.o: ../ftd_kern_ctypes.h
ftd_ioctl.o: ../ftd_buf.h
ftd_ioctl.o: ../ftdio.h
ftd_ioctl.o: ../ftd_klog.h
ftd_ioctl.o: ../ftd_bab.h
ftd_ioctl.o: ../ftd_bits.h
ftd_ioctl.o: ../ftd_def.h
ftd_ioctl.o: ../ftd_var.h
ftd_ioctl.o: ../ftd_all.h
ftd_ioctl.o: ../ftdio.h
ftd_ioctl.o: ../ftd_klog.h
ftd_ioctl.o: ../ftd_kern_cproto.h
ftd_all.o: ../ftd_all.c
ftd_all.o: ../ftd_kern_ctypes.h
ftd_all.o: ../ftd_ddi.h
ftd_all.o: ../ftd_def.h
ftd_all.o: ../ftd_bab.h
ftd_all.o: ../ftd_def.h
ftd_all.o: ../ftdio.h
ftd_all.o: ../ftd_kern_ctypes.h
ftd_all.o: ../ftd_buf.h
ftd_all.o: ../ftdio.h
ftd_all.o: ../ftd_klog.h
ftd_all.o: ../ftd_bab.h
ftd_all.o: ../ftd_bits.h
ftd_all.o: ../ftd_def.h
ftd_all.o: ../ftd_var.h
ftd_all.o: ../ftd_all.h
ftd_all.o: ../ftd_klog.h
ftd_all.o: ../ftd_kern_cproto.h
ftd_klog.o: ../ftd_klog.c
ftd_klog.o: ../ftd_kern_ctypes.h
ftd_klog.o: ../ftd_klog.h
memset.o: ../memset.c
memset.o: ../ftd_kern_ctypes.h
version.o: ../../version.c
