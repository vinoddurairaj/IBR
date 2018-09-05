#
# Copyright (c) 1999 Legato Systems, Inc. All Rights Reserved.
#
TOP=../..

OBJS= ftd_cfgsys.o ftd_config.o ftd_dev.o ftd_devio.o ftd_fsm.o \
ftd_ioctl.o ftd_journal.o ftd_lg.o ftd_proc.o ftd_ps.o ftd_rsync.o \
ftd_sock.o ftd_error.o ftd_stat.o ftd_platform.o ftd_throt.o \
ftd_lic.o ftd_cpoint.o

CINCLUDES=${INCLST} ${INCSOCK} ${INCMD5} ${INCPROC} ${INCUTIL} \
${INCDEVICE} ${INCCOMP} ${INCERR} ${INCLIC}

CDEFINES+=-DHAVE_UNISTD_H 

LIBRARY=ftd
CLEANS+=ftd_errmsg.h errors.msg

include ${TOP}/mk/lib.gmk

ifdef GCC_CHECKER
ftd_errmsg.h: errors.msg
	awk -F'	' 'BEGIN { printf "/* Generated from errors.msg */\n"; } !/^#/ { printf "#define M_%s %c%s%c\n", $$1, 34, $$2, 34 }' < errors.msg > errmsg.h
else
ftd_errmsg.h: errors.msg
	awk 'BEGIN { printf "/* Generated from errors.msg */\n"; } !/^#/ { printf "#define M_%s %c%s%c\n", $$1, 34, $$1, 34 }' < errors.msg > ftd_errmsg.h
endif

errors.msg: errors.MSG 
	$(PKGIFY) 
errsweep:
	@for i in `awk '!/^#/ { printf "M_%s\n", $$1;}' < errors.msg`; do \
		echo -n '.' ; \
		find ${TOP} -name \*.c | xargs egrep $i > /tmp/xxx; \
		if [ -s /tmp/xxx ]; then \
			echo $$i ; \
		fi; \
		rm -f /tmp/xxx ; \
	done

*.c: ftd_errmsg.h
