#
# Copyright (c) 1998 FullTime Software, Inc. All Rights Reserved.
#
TOP=../..

BUILDBRAND=$(shell ${TOP}/mk/getosarch.sh)
ifeq (${BUILDBRAND},)
$(error Unable to collect the operating system architecture information)
endif

OBJS=common.o config.o errors.o iputil.o network.o platform.o throttle.o \
	procinfo.o ipc.o process.o disksize.o ftdio.o rsync.o \
	devcheck.o qio.o stat_intr.o ps_intr.o cfg_intr.o err.o ftdif.o \
	stop_group.o hash.o

# We currently compile the extra libzfs file only under solaris 10.
# This should really be decided through some autoconf like mechanism.
SYSTYPE:= $(shell uname -s)
ifeq (SunOS,$(SYSTYPE))
OBJS += dynamic_libzfs.o
endif

CINCLUDES=$(INCDEVICE) $(INCLIC) $(INCMD5) $(INCGEN)

LIBRARY=gen
ETC_INSTALL=errors.msg

PKGHDRS=pathnames.h

ifeq ($(wildcard ${TOP}/mk/vars_tdmf_${BUILDBRAND}.gmk),)
include ${TOP}/mk/lib.gmk
else
include ${TOP}/mk/lib_tdmf.gmk
endif

ifndef COPTFLAGS
COPTFLAGS=$(UCFLAGS)
endif

ifdef DEBUG
CDEFINES += $(UDEBUG)
COPTFLAGS += -g
endif

CLEANS+=errmsg.h errors.msg pathnames.h

mt:
	$(MAKE) all CDEFINES+="-DMT -DTESTTHROTTLEPARSE"

ifdef GCC_CHECKER
errmsg.h: errors.msg
	awk -F'	' 'BEGIN { printf "/* Generated from errors.msg */\n"; } !/^#|^[ \t]*$$/ { printf "#define M_%s \"%s\"\n", $$1, $$2 }' < errors.msg > errmsg.h
else
errmsg.h: errors.msg
	awk 'BEGIN { printf "/* Generated from errors.msg */\n"; } !/^#|^[ \t]*$$/ { printf "#define M_%s \"%s\"\n", $$1, $$1 }' < errors.msg > errmsg.h
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
	${CC} -g -o devcheck devcheck.o disksize.o -L . -lgen -L ../libmd5 -lmd5

common.c: errmsg.h pathnames.h
