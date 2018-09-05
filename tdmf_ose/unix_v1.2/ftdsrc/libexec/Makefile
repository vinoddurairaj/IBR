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
TOP=..

include ${TOP}/mk/vars.gmk

SUBDIRS=checkpoint debugcapture hostinfo hrdb_math info init \
	killpmds killrefresh killrmds killmaster \
	launchpmds launchrefresh launchmaster launchnetanalysis \
	licinfo monitortty override set start stop \
	modfs psmigrate panalyze failover psmigrate272
# Note: killbackfresh and launchbackfresh are deactivated as of TDMF IP 2.8.0
#reconfig
ifeq (AIX,$(SYSTYPE))
SUBDIRS += jfspremount jfspostmount genmklv syncfs updateodm chfs hacmp genlparid target_netinfo
endif
ifeq (Linux,$(SYSTYPE))
ifneq (,$(findstring ${ARCH},i686 x86_64))
SUBDIRS += omp
endif
CDEFINES = $(UDEFINES)
CINCLUDES = $(UINCLUDES)
COPTFLAGS = $(UCFLAGS)
else
SUBDIRS += expand limitsize wish
endif
include ${TOP}/mk/subdir.gmk
