#
# Copyright (c) 1997, 1998 FullTime Software, Inc.  All rights reserved.
#
# $Id: Makefile,v 1.4 2001/09/17 18:52:07 bouchard Exp $
#

TOP=../..

CDEFINES= -D_KERNEL -DKERNEL  

include ${TOP}/Makefile.inc

all:: mod64.o master.d-${Q} space.h-${Q} system.${Q}

# HPUX 11.0-64 src and bits

VERSSRC=${TOP}/version.c

HDRS=	../ftd_all.h ../ftd_bab.h ../ftd_bits.h \
	../ftd_buf.h ../ftd_ddi.h ../ftd_def.h \
	../ftd_disk.h ../ftd_kern_cproto.h ../ftd_kern_ctypes.h \
	../ftd_klog.h ../ftd_timers.h ../ftd_var.h ../ftdio.h

SRCS=	../ftd_ddi.c ../ftd_buf.c ../ftd_hpux.c ../ftd_bab.c ../ftd_bits.c \
	../ftd_ioctl.c ../ftd_all.c ../ftd_klog.c ../memset.c \
	../ftd_timers.c $(VERSSRC)

OBJS=	ftd_ddi.o ftd_buf.o ftd_hpux.o ftd_bab.o ftd_bits.o \
	ftd_ioctl.o ftd_all.o ftd_klog.o memset.o \
	ftd_timers.o version.o

EXTRA_CLEANS = mod64.o master.d-${Q} system.${Q} space.h-${Q}

#
# HPUX 11.0-64 drv bld rules
#

IDENT=-DLP64_TMP_NODBG -DSPP_RUNWAY_ERR_ENABLED -DSPP_OBP_BOOT -D__ROSEVILLE__ \
	-DKGDB_ON -DSTCP -DHPONCPLUS -D__ROSE__ -DNEW_MFCTL_W -DINET_COSE \
	-DIVT_INTERCEPT -DLOCAL_SWITCH -DMULTIPLE_LOGICAL_HOSTS -DRSVP_ISI \
	-DMROUTING -DMULTICAST -DAPPLETALK -DFDDI_VM -DSPARSE_PDIR -DAUDIT \
	-DACLS -DPGPROF -DKI -DMP -DLWSYSCALL -DICA_ON -DNEW_RDB -DRDB \
	-D_LVMROOT -D_HPUX -D_LVM -DCONVERGED_IO 

IDENT_TMP64=D__TEMP64__

IDENT_INT=-D__HIGHC__ -D__STDC_EXT__ -D_KERNEL_BUILD -D_XPG4_EXTENDED \
	-D_HPUX_SOURCE -D_UNSUPPORTED -D__hp9000s800 -Dhp9000s800

IDENT_KERN=	-D_KERNEL -DKERNEL

UNDEF=	-U__hp9000s700

PAARCH=64

DAFLAG=+DA2.0W
DSFLAG=+DS2.0

COPTS=	${IDENT} ${IDENT_TMP64} ${OPTIONS} ${UNDEF} ${IDENT_INT} ${IDENT_KERN} \
	${VERSIONDEFINES}

OFLAGS=	+XixdU 

CCOPTS=	-Wp,-H300000 $(OFLAGS) +Hx0 +R500 -Wl,-a,archive \
	${DAFLAG} ${DSFLAG} +ESsfc +ESssf 

CFLAGS= -DDRV_DDEBUG -I. -I/usr/conf/io ${COPTS} ${CCOPTS} \
	-w -Ae -DHPUX 

CFLAG2= +ES1.Xindirect_calls 

AFLAGS= -ew 


#
# installation rules
#
install:: mod64.o master.d-${Q} system.${Q} space.h-${Q} ${Q}.conf
	@mkdir -p $(DESTDIR)/usr/conf/${Q}
	@mkdir -p $(DESTDIR)/sbin/init.d
	@mkdir -p $(DESTDIR)/sbin/rc3.d
	@mkdir -p $(DESTDIR)/etc/opt/${PKGNAME}
	@${INSTALL} -m 444 ${Q}.conf ${SAMPLEDIR}
	@${INSTALL} -m 444 ${Q}.conf $(DESTDIR)/etc/opt/${PKGNAME}
	@${INSTALL} -m 444 mod64.o ${DESTDIR}/usr/conf/${Q}
	@${INSTALL} -m 444 master.d-${Q} ${DESTDIR}/usr/conf/${Q}/master
	@${INSTALL} -m 444 system.${Q} ${DESTDIR}/usr/conf/${Q}/system
	@${INSTALL} -m 444 space.h-${Q} ${DESTDIR}/usr/conf/${Q}/space.h

wl:
	$(MAKE) VERSION=$(REV).`date +%y%m%d.%H%M` ftd COPTFLAGS="-g -Xa"

#
# obj dep's
#
mod64.o: ${OBJS}

# drv target rules
mod64.o: $(OBJS) version.o
	@echo "Making 64-bit HPUX 11.0 driver ${Q}"
	@ld -r -o mod64.o $(OBJS) version.o /usr/lib/pa20_64/milli.a

# production rules
#Pbouchard
#../ftd_hpux.c: ../ftd_hpux.C 
#	$(PKGIFY)

ftd_hpux.o: ../ftd_hpux.c
	${CC} ${CFLAGS} ${CFLAG2} -c $<

ftd_ddi.o: ../ftd_ddi.c ${HDRS}
	${CC} ${CFLAGS} ${CFLAG2} -c $<

ftd_buf.o: ../ftd_buf.c ${HDRS}
	${CC} ${CFLAGS} ${CFLAG2} -c $<

ftd_bab.o: ../ftd_bab.c
	${CC} ${CFLAGS} ${CFLAG2} -c $<

ftd_bits.o:  ../ftd_bits.c
	${CC} ${CFLAGS} ${CFLAG2} -c $<

ftd_ioctl.o: ../ftd_ioctl.c
	${CC} ${CFLAGS} ${CFLAG2} -c $<

ftd_all.o: ../ftd_all.c
	${CC} ${CFLAGS} ${CFLAG2} -c $<

ftd_klog.o: ../ftd_klog.c
	${CC} ${CFLAGS} ${CFLAG2} -c $<

ftd_timers.o: ../ftd_timers.c
	${CC} ${CFLAGS} ${CFLAG2} -c $<

memset.o: ../memset.c
	${CC} ${CFLAGS} ${CFLAG2} -c $<

version.o: ${VERSSRC}
	${CC} ${CFLAGS} ${CFLAG2} -c $<


#
# misc targets
#

${Q}.conf: ../driver.conf
	${PKGIFY}

master.d-${Q}: master.d.src
	$(PKGIFY)

space.h-${Q}: space.h.src
	$(PKGIFY)

system.${Q}: system.src
	$(PKGIFY)

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

