#
# Copyright (c) 1997, 1998 FullTime Software, Inc.  All rights reserved.
#
# $Id: Makefile,v 1.13 2010/12/17 16:43:32 proulxm Exp $
#

TOP=../..

CDEFINES= -D_KERNEL -DKERNEL  

include ${TOP}/mk/Makefile.inc
SUBDIRS= aixmethods
CFLAGS+=-qsource -qlist
EXTRA_CLEANS=${Q}.map ${Q}.conf
whattodo:
	@echo "Making aix51 driver ${Q}"

all:: whattodo ${Q} ${Q}.conf

VERSSRC=$(TOP)/version.c

AIXHDRS=	../ftd_all.h ../ftd_bab.h ../ftd_bits.h \
	../ftd_buf.h ../ftd_ddi.h ../ftd_def.h \
	../ftd_disk.h ../ftd_kern_cproto.h ../ftd_kern_ctypes.h \
	../ftd_klog.h ../ftd_timers.h ../ftd_var.h ../ftdio.h 

AIXSRCS=	../ftd_.c ../ftd_buf.c ../ftd_bab.c ../ftd_bits.c \
	../ftd_ioctl.c ../ftd_all.c ../ftd_klog.c ../memset.c \
	../ftd_aix.c ../ftd_ddi.c ftd_timers.c ../ftd_dynamic_activation.c ../ftd_dynamic_activation_aix.c ../ftd_hash_table.c $(VERSSRC)

AIXOBJS= ftd_aix.o ftd_buf.o ftd_bab.o ftd_bits.o \
	ftd_ioctl.o ftd_all.o ftd_klog.o memset.o \
	ftd_ddi.o ftd_timers.o ftd_pending_ios_monitoring.o ftd_dynamic_activation.o ftd_dynamic_activation_aix.o ftd_hash_table.o version.o 

AIXOBJS64= ftd_aix64.o ftd_buf64.o ftd_bab64.o ftd_bits64.o \
	ftd_ioctl64.o ftd_all64.o ftd_klog64.o memset64.o \
	ftd_ddi64.o ftd_timers64.o ftd_pending_ios_monitoring64.o ftd_dynamic_activation64.o ftd_dynamic_activation_aix64.o ftd_hash_table64.o version64.o

EXTRA_CLEANS+=	${AIXOBJS} ${AIXOBJS64} ${AIXOBJS:%.o=%.lst} ${AIXOBJS64:%.o=%.lst} ${Q}32 ${Q}64 ${Q} ${Q}.conf

#
# installation rules
#
install::

#
# obj dep's
#
# $(V9OBJS): $(V9HDRS) $(V9SRCS)

# load v9 driver
ifneq ($(findstring ${SYSVERS},510 520 530),)
${Q}: ${Q}32 ${Q}64
	@echo "Archiving driver: ${Q}"
	@ar -X32_64 -r ${Q} ${Q}32 ${Q}64
else
${Q}: ${Q}64
	@echo "Archiving driver: ${Q}"
	@ar -X64 -r ${Q} ${Q}64
endif

${Q}32: $(AIXOBJS) version.o ${Q}_syms.exp 
	@echo "Linking driver: ${Q}32"
	@ld -b32 -H4096 -eftd_config -bI:/lib/kernex.exp \
	-bI:/lib/syscalls.exp -bloadmap:$@.loadmap -bmap:$@.map -bE:${Q}_syms.exp  \
	-lcsys -o ${Q}32 $(AIXOBJS) 

${Q}64: $(AIXOBJS64)
	@echo "Linking driver: ${Q}64"
	@ld -b64 -H4096 -eftd_config -bI:/lib/kernex.exp \
	-bI:/lib/syscalls.exp -bloadmap:$@.loadmap -bmap:$@.map -bE:${Q}_syms.exp  \
	-lcsys -o ${Q}64 $(AIXOBJS64) 

# production rules
ftd_aix.o: ../ftd_aix.c
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
	
ftd_ddi.o: ../ftd_ddi.c
	${CC} ${CFLAGS} -c $<

ftd_timers.o: ../ftd_timers.c
	${CC} ${CFLAGS} -c $<
	
ftd_klog.o: ../ftd_klog.c
	${CC} ${CFLAGS} -c $<
	
ftd_pending_ios_monitoring.o: ../ftd_pending_ios_monitoring.c
	${CC} ${CFLAGS} -c $<
	
${Q}_syms.exp:
	./aixmethods/mkexpsyms.sh ./*.o > dtc_syms.exp 2>&1

memset.o: ../memset.c
	${CC} ${CFLAGS} -c $<

ftd_dynamic_activation.o: ../ftd_dynamic_activation.c
	${CC} ${CFLAGS} -c $<

ftd_dynamic_activation_aix.o: ../ftd_dynamic_activation_aix.c
	${CC} ${CFLAGS} -c $<

ftd_hash_table.o: ../ftd_hash_table.c
	${CC} ${CFLAGS} -c $<

version.o: ${VERSSRC}
	${CC} ${CFLAGS} -c $<

# production rules
ftd_aix64.o: ../ftd_aix.c
	${CC} ${CFLAGS} -q64 -o ftd_aix64.o -c $<

ftd_buf64.o: ../ftd_buf.c
	${CC} ${CFLAGS} -q64 -o ftd_buf64.o -c $<

ftd_bab64.o: ../ftd_bab.c
	${CC} ${CFLAGS} -q64 -o ftd_bab64.o -c $<

ftd_bits64.o:  ../ftd_bits.c
	${CC} ${CFLAGS} -q64 -o ftd_bits64.o -c $<

ftd_ioctl64.o: ../ftd_ioctl.c
	${CC} ${CFLAGS} -q64 -o ftd_ioctl64.o -c $<

ftd_all64.o: ../ftd_all.c
	${CC} ${CFLAGS} -q64 -o ftd_all64.o -c $<
	
ftd_ddi64.o: ../ftd_ddi.c
	${CC} ${CFLAGS} -q64 -o ftd_ddi64.o -c $<

ftd_timers64.o: ../ftd_timers.c
	${CC} ${CFLAGS} -q64 -o ftd_timers64.o -c $<
	
ftd_klog64.o: ../ftd_klog.c
	${CC} ${CFLAGS} -q64 -o ftd_klog64.o -c $<
	
ftd_pending_ios_monitoring64.o: ../ftd_pending_ios_monitoring.c
	${CC} ${CFLAGS} -q64 -o ftd_pending_ios_monitoring64.o -c $<
	
memset64.o: ../memset.c
	${CC} ${CFLAGS} -q64 -o memset64.o -c $<

version64.o: ${VERSSRC}
	${CC} ${CFLAGS} -q64 -o version64.o -c $<

ftd_dynamic_activation64.o: ../ftd_dynamic_activation.c
	${CC} ${CFLAGS} -q64 -o ftd_dynamic_activation64.o -c $<

ftd_dynamic_activation_aix64.o: ../ftd_dynamic_activation_aix.c
	${CC} ${CFLAGS} -q64 -o ftd_dynamic_activation_aix64.o -c $<

ftd_hash_table64.o: ../ftd_hash_table.c
	${CC} ${CFLAGS} -q64 -o ftd_hash_table64.o -c $<

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
ftd_ioctl.o: ../ftd_dynamic_activation.h
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
ftd_all.o: ../ftd_dynamic_activation.h
ftd_klog.o: ../ftd_klog.c
ftd_klog.o: ../ftd_kern_ctypes.h
ftd_klog.o: ../ftd_klog.h
memset.o: ../memset.c
memset.o: ../ftd_kern_ctypes.h
ftd_dynamic_activation.o: ../ftd_dynamic_activation.h
ftd_dynamic_activation.o: ../ftd_kern_cproto.h
ftd_dynamic_activation.o: ../ftd_klog.h
ftd_dynamic_activation.o: ../ftd_ddi.h
ftd_dynamic_activation.o: ../ftd_all.h
ftd_dynamic_activation_aix.o: ../ftd_dynamic_activation_os.h
ftd_hash_table.o: ../ftd_def.h
version.o: ../../version.c
