#
# Copyright (c) 1998 FullTime Software, Inc. All Rights Reserved.
#
TOP=../..

OBJS=common.o config.o errors.o iputil.o network.o platform.o throttle.o \
	procinfo.o ipc.o process.o disksize.o ftdio.o rsync.o \
	devcheck.o qio.o stat_intr.o ps_intr.o cfg_intr.o err.o ftdif.o \
	stop_group.o
CINCLUDES=-I${TOP}/driver ${INCLIC} ${INCMD5} ${INCGEN}
LIBRARY=gen
ETC_INSTALL=errors.msg

PKGHDRS=pathnames.h

include ${TOP}/mk/lib.gmk
CLEANS+=errmsg.h errors.msg

mt:
	$(MAKE) all CDEFINES+="-DMT -DTESTTHROTTLEPARSE"

ifdef GCC_CHECKER
errmsg.h: errors.msg
	awk -F'	' 'BEGIN { printf "/* Generated from errors.msg */\n"; } !/^#/ { printf "#define M_%s %c%s%c\n", $$1, 34, $$2, 34 }' < errors.msg > errmsg.h
else
errmsg.h: errors.msg
	awk 'BEGIN { printf "/* Generated from errors.msg */\n"; } !/^#/ { printf "#define M_%s %c%s%c\n", $$1, 34, $$1, 34 }' < errors.msg > errmsg.h
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

# after AIX, pathnames.h is platform dependent
# so, generate a platform specific vers...

pathnames.h: pathnames.H
	$(PKGIFY)

devcheck: devcheck.o disksize.o
	${CC} -o devcheck devcheck.o disksize.o

*.c: errmsg.h
