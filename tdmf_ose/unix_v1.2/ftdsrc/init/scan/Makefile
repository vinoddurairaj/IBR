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
#
#
TOP=../..

PLATFORMTYPE:=$(shell uname -s)

ifeq (Linux,$(PLATFORMTYPE))
INIT_INSTALL=${PKGNAME}-scan ${PKGNAME}-start.service ${PKGNAME}-stop.service
PLAT_SH_PROGRAM=${PKGNAME}-scan ${PKGNAME}-start.service ${PKGNAME}-stop.service
PLATSHSRC=scan.sh.${SYSTYPE} ${PKGNAME}-start.service.brand ${PKGNAME}-stop.service.brand
else
INIT_INSTALL=${PKGNAME}-scan
PLAT_SH_PROGRAM=${PKGNAME}-scan
PLATSHSRC=scan.sh.${SYSTYPE}
endif

include ${TOP}/mk/prog.gmk
