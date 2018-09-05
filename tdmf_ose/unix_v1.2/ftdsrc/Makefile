#
# Copyright (c) 1997, 1998 FullTime Software, Inc.  All Rights Reserved.
#
# Top level makefile for the building of the FullTime Data product.
#
# We assume that we have a number of subdirectories under this top level.
# These are listed in SUBDIRS (which is true for all subdirectories too).
# Furthermore, there is a special subdirectory called pkg.install which
# contains all of the stuff needed to build a package for Solaris.
#
# These makefiles are all in GNU make syntax
#
# The "all" target builds all the software.
# The "clean" target removes all built software.
# The "install" target will install the software into a subdir of pkg.install
#
# $Id: Makefile,v 1.11 2009/02/05 06:34:49 naat0 Exp $
#

SYSTYPE:=$(uname -s)
SUBDIRS := lib daemons bin libexec driver init gui pkg.install agent
ifeq (imp,$(USER))
SUBDIRS += qa
endif
EXTRA_CLEANS=version.o
TOP_INSTALL=README
TOP=.

all::	version.o

include ${TOP}/mk/subdir.gmk

ifeq (AIX,$(SYSTYPE))
  clean : SUBDIRS += installp/mklpp/aixbin
endif

time: _FORCE
	@echo "Time: Before install" `date`

cdrom::	all time install
	@(cd pkg.install; $(MAKE) cdrom)

#
# we have special target called cdrom which creates an installable package.
#
