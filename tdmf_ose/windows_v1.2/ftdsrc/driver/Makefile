#
# Copyright (c) 1997, 1998 FullTime Software, Inc.  All rights reserved.
#
# $Id: Makefile,v 1.1.1.1 2001/09/14 13:47:47 dturrin Exp $
#

TOP=..

#CDEFINES= -D_KERNEL -DKERNEL  -DFTD_MAX_BUF=16
CDEFINES= -D_KERNEL -DKERNEL  

OBJS=	ftd_sun.o ftd_buf.o ftd_bab.o ftd_bits.o \
	ftd_ioctl.o ftd_all.o ftd_klog.o memset.o  \
	${V}

PROGRAMS=${Q}

EXTRA_CLEANS=${Q}.conf *.lst ${Q}.?map ${Q}.map ${Q}.exp

include ${TOP}/Makefile.inc

#
# HPUX drv bld rules
#
ifeq (HP-UX,$(SYSTYPE))
    IDENT=-DNEW_MFCTL_W -DINET_COSE -DIVT_INTERCEPT -DLOCAL_SWITCH -DMULTIPLE_LOGICAL_HOSTS -DRSVP_ISI -DMROUTING -DMULTICAST -DAPPLETALK -DFDDI_VM -DSPARSE_PDIR -DAUDIT -DACLS -DPGPROF -DKI -DMP -DLWSYSCALL -DICA_ON -DNEW_RDB -DRDB -D_LVMROOT -D_HPUX -D_LVM -DCONVERGED_IO

    IDENT_INT=-D__HIGHC__ -D__STDC_EXT__ -D_KERNEL_BUILD -D_XPG4_EXTENDED -D_HPUX_SOURCE -D_UNSUPPORTED -D__hp9000s800 -Dhp9000s800

    IDENT_KERN=-D_KERNEL -DKERNEL

    UNDEF=-U__hp9000s700

    COPTS=${IDENT} ${OPTIONS} ${UNDEF} ${IDENT_INT} ${IDENT_KERN} ${VERSIONDEFINES}

    OFLAGS=+XixdU 

    CCOPTS=-Wp,-H300000 $(OFLAGS) +Hx0 +R25 -Wl,-a,archive \
  +DAportable +DS899 +ESsfc

#    CFLAGS= -DDRV_DDEBUG -w -I. -I/usr/conf/io ${COPTS} ${CCOPTS} -Ae -DHPUX 
    CFLAGS= -DDRV_DDEBUG -I. -I/usr/conf/io ${COPTS} ${CCOPTS} -Ae -DHPUX 

	SRCS=	ftd_hpux.c ftd_bab.c ftd_bits.c ftd_ioctl.c \
		ftd_all.c ftd_ddi.c bab.c ftd_klog.c memset.c
		
	OBJS=	ftd_hpux.o ftd_bab.o ftd_bits.o ftd_ioctl.o \
		ftd_all.o ftd_ddi.o bab.o ftd_klog.o memset.o \
		${V}

	EXTRA_CLEANS = lib${Q}.a ftd_hpux.c

ftd_hpux.c: ftd_hpux.C
	$(PKGIFY)

endif

#
# _AIX drv bld rules
#
ifeq (AIX,$(SYSTYPE))

	SUBDIRS+=aixmethods

	SRCS=	ftd_aix.c ftd_aixtimers.c ftd_buf.c ftd_bab.c \
	        ftd_bits.c ftd_ioctl.c ftd_all.c ftd_ddi.c bab.c \
	        ftd_klog.c memset.c 

	OBJS=	ftd_aix.o ftd_aixtimers.o ftd_buf.o ftd_bab.o \
	        ftd_bits.o ftd_ioctl.o ftd_all.o ftd_ddi.o bab.o \
	        ftd_klog.o memset.o ${V}

	CDEFINES+=-DFTD_DEBUG -qlist -qsource 
	AIXLDOPTS+=-bkeepfile:${V}

endif

#
# installation rules
#
install:: master.d-${Q}

ifeq (SunOS,$(SYSTYPE))
	@mkdir -p $(DESTDIR)/usr/kernel/drv
	@mkdir -p $(DESTDIR)/etc/init.d
	@mkdir -p $(DESTDIR)/etc/rc3.d
	@${INSTALL} -m 555 ${Q} $(DESTDIR)/usr/kernel/drv
	@${INSTALL} -m 444 ${Q}.conf $(DESTDIR)/usr/kernel/drv
	@${INSTALL} -m 444 ${Q}.conf ${SAMPLEDIR}
endif

ifeq (AIX,$(SYSTYPE))
	@echo "WARNING: see Makefile in installp/mklpp/aixbin..."
endif

ifeq (HP-UX,$(SYSTYPE))
	@mkdir -p $(DESTDIR)/usr/conf/lib
	@mkdir -p $(DESTDIR)/usr/conf/master.d
	@mkdir -p $(DESTDIR)/sbin/init.d
	@mkdir -p $(DESTDIR)/sbin/rc3.d
	@mkdir -p $(DESTDIR)/etc/opt/${PKGNAME}
	@${INSTALL} -m 444 ${Q}.conf ${SAMPLEDIR}
	@${INSTALL} -m 444 ${Q}.conf $(DESTDIR)/etc/opt/${PKGNAME}
	@${INSTALL} -m 664 lib${Q}.a $(DESTDIR)/usr/conf/lib
	@${INSTALL} -m 444 master.d-${Q} ${DESTDIR}/usr/conf/master.d/${Q}
endif

wl:
	$(MAKE) VERSION=$(REV).`date +%y%m%d.%H%M` ftd COPTFLAGS="-g -Xa"

#
# obj dep's
#
$(OBJS):ftd_buf.h ftd_bab.h ftd_bits.h ftd_ddi.h ftd_def.h ftd_disk.h \
	ftd_var.h ftdio.h ftd_all.h ftd_klog.h

#
# drv target rules
#

${Q}: $(OBJS)

#
# _AIX targets
#
ifeq (AIX,$(SYSTYPE))
	./aixmethods/mkexpsyms.sh $(OBJS) > ./${Q}.exp
	ld -o $@ $(OBJS) -eftd_config -bl:${Q}.ld -H512 -D512 \
		${AIXLDOPTS} \
		-bmap:./${Q}.map -bE:./${Q}.exp \
		-bR:./${Q}.Rmap -bC:./${Q}.Cmap -bX:./${Q}.Xmap \
		-bI:/usr/lib/kernex.exp -bI:/usr/lib/syscalls.exp -lsys -lcsys
endif 

#
# HPUX targets
#
ifeq (HP-UX,$(SYSTYPE))
	@/bin/ar r lib${Q}.a $(OBJS)
endif

#
# SunOS targets
#
ifeq (SunOS,$(SYSTYPE))
	@ld -r -o $(PROGRAMS) $(OBJS)
	@nm -u ${Q} | tail +5 | diff -c sparc/undef.expected -
endif 

#
# misc targets
#
all:: ${Q}.conf

${Q}.conf: driver.conf
	$(PKGIFY)

master.d-${Q}: master.d.src
	$(PKGIFY)
