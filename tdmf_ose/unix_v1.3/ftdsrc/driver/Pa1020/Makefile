#
# Copyright (c) 1997, 1998 FullTime Software, Inc.  All rights reserved.
#
# $Id: Makefile,v 1.4 2001/09/17 18:52:02 bouchard Exp $
#

TOP=../..

CDEFINES= -D_KERNEL -DKERNEL  

PROGRAMS=${Q}

include ${TOP}/Makefile.inc

#
# HPUX drv bld rules
#
IDENT=		-DNEW_MFCTL_W -DINET_COSE -DIVT_INTERCEPT \
		-DLOCAL_SWITCH -DMULTIPLE_LOGICAL_HOSTS \
		-DRSVP_ISI -DMROUTING -DMULTICAST -DAPPLETALK \
		-DFDDI_VM -DSPARSE_PDIR -DAUDIT -DACLS -DPGPROF \
		-DKI -DMP -DLWSYSCALL -DICA_ON -DNEW_RDB -DRDB \
		-D_LVMROOT -D_HPUX -D_LVM -DCONVERGED_IO

IDENT_INT=	-D__HIGHC__ -D__STDC_EXT__ -D_KERNEL_BUILD \
		-D_XPG4_EXTENDED -D_HPUX_SOURCE -D_UNSUPPORTED \
		-D__hp9000s800 -Dhp9000s800

IDENT_KERN=	-D_KERNEL -DKERNEL

UNDEF=	-U__hp9000s700

COPTS=${IDENT} ${OPTIONS} ${UNDEF} ${IDENT_INT} ${IDENT_KERN} ${VERSIONDEFINES}

OFLAGS=	+XixdU 

CCOPTS=	-Wp,-H300000 $(OFLAGS) +Hx0 +R25 -Wl,-a,archive \
	+DAportable +DS899 +ESsfc

CFLAGS= -DDRV_DDEBUG -I. -I/usr/conf/io ${COPTS} ${CCOPTS} -Ae -DHPUX \
	 -DSTATIC_BAB_MALLOC

SRCS=	../ftd_hpux.c ../ftd_bab.c ../ftd_bits.c ../ftd_ioctl.c \
	../ftd_buf.c ../ftd_all.c ../ftd_ddi.c ../bab.c ../ftd_klog.c \
	../ftd_timers.c ../memset.c

VERSSRC=../../version.c
		
OBJS=	ftd_hpux.o ftd_bab.o ftd_bits.o ftd_ioctl.o \
	ftd_buf.o ftd_all.o ftd_ddi.o bab.o ftd_klog.o \
	ftd_timers.o memset.o \
	version.o

EXTRA_CLEANS = lib${Q}.a ${Q}.conf


#
# installation rules
#
install:: master.d-${Q}
	@mkdir -p $(DESTDIR)/usr/conf/lib
	@mkdir -p $(DESTDIR)/usr/conf/master.d
	@mkdir -p $(DESTDIR)/sbin/init.d
	@mkdir -p $(DESTDIR)/sbin/rc3.d
	@mkdir -p $(DESTDIR)/etc/opt/${PKGNAME}
	@${INSTALL} -m 444 ${Q}.conf ${SAMPLEDIR}
	@${INSTALL} -m 444 ${Q}.conf $(DESTDIR)/etc/opt/${PKGNAME}
	@${INSTALL} -m 664 lib${Q}.a $(DESTDIR)/usr/conf/lib
	@${INSTALL} -m 444 master.d-${Q} ${DESTDIR}/usr/conf/master.d/${Q}
#
# drv target rules
#
${Q}: $(OBJS)
	@/bin/ar r lib${Q}.a $(OBJS)

ftd_hpux.o: ../ftd_hpux.c
	${CC} ${CFLAGS} -c $<

ftd_ddi.o: ../ftd_ddi.c ${HDRS}
	${CC} ${CFLAGS} -c $<

ftd_buf.o: ../ftd_buf.c ${HDRS}
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

ftd_timers.o: ../ftd_timers.c
	${CC} ${CFLAGS} -c $<

bab.o: ../bab.c
	${CC} ${CFLAGS} -c $<

memset.o: ../memset.c
	${CC} ${CFLAGS} -c $<

version.o: ${VERSSRC}
	${CC} ${CFLAGS} -c $<


#
# misc targets
#
all:: ${Q}.conf

${Q}.conf: ../driver.conf
	$(PKGIFY)

master.d-${Q}: master.d.src
	$(PKGIFY)

# makedep's
ftd_hpux.o: ../ftd_hpux.c
ftd_hpux.o: ../ftd_ddi.h 
ftd_hpux.o: ../ftd_def.h
ftd_hpux.o: ../ftd_kern_ctypes.h 
ftd_hpux.o: ../ftd_kern_cproto.h
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
ftd_bits.o: ../ftd_bits.c
ftd_bits.o: ../ftd_kern_ctypes.h
ftd_bits.o: ../ftd_bab.h
ftd_bits.o: ../ftd_def.h
ftd_bits.o: ../ftd_bab.h
ftd_bits.o: ../ftd_buf.h
ftd_ddi.o:  ../ftd_ddi.c
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

