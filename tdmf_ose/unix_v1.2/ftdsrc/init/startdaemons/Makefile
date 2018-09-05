#
# Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
#
# RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
# Government is subject to restrictions as set forth in
# subparagraph (c)(1)(ii) of the Rights in Technical Data and
# Computer Software clause at DFARS 52.227-7013 and in similar
# clauses in the FAR and NASA FAR Supplement.
#
#
#
# $Id: Makefile,v 1.6 2017/01/11 20:48:42 paulclou Exp $
#

TOP=../..

PLATFORMTYPE:=$(shell uname -s)

ifeq (Linux,$(PLATFORMTYPE))
INIT_INSTALL=${PKGNAME}-startdaemons ${PKGNAME}-startdaemons.service ${PKGNAME}-startmaster.service ${PKGNAME}-startpmds.service
PLAT_SH_PROGRAM=${PKGNAME}-startdaemons ${PKGNAME}-startdaemons.service ${PKGNAME}-startmaster.service ${PKGNAME}-startpmds.service
PLATSHSRC=startdaemons.sh.${SYSTYPE} ${PKGNAME}-startdaemons.service.brand ${PKGNAME}-startmaster.service.brand ${PKGNAME}-startpmds.service.brand
else
INIT_INSTALL=${PKGNAME}-startdaemons
PLAT_SH_PROGRAM=${PKGNAME}-startdaemons
PLATSHSRC=startdaemons.sh.${SYSTYPE}
endif

include ${TOP}/mk/prog.gmk
