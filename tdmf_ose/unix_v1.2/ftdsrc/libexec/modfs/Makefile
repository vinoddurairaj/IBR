#Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
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

SYSTYPE:= $(shell uname -s)

BIN_INSTALL=${Q}modfs
SH_PROGRAM=${Q}modfs

SHSRC=dtcmodfs.sh

ifeq (Linux,$(SYSTYPE))
  # For Redhat 6, ksh is not installed on the platform, so we need to use the sh shell 
  # instead for Linux platform (i.e. Redhat and suse). 
  # (Requires some options to be enabled, see the script, or find 'print' equivalent).
 
  SHSRC=dtcmodfs.Linux.sh

endif

include ${TOP}/mk/prog.gmk
