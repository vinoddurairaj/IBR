#
# Copyright (c) 1996, 1998 FullTime Software, Inc. All Rights Reserved.
#
TOP=../..

OBJS= license.o	licplat.o
LIBRARY=lic
CINCLUDES=${INCGEN}

BUILDBRAND=$(shell ${TOP}/mk/getosarch.sh)
ifeq (${BUILDBRAND},)
$(error Unable to collect the operating system architecture information)
endif

ifeq ($(wildcard ${TOP}/mk/vars_tdmf_${BUILDBRAND}.gmk),)
include ${TOP}/mk/lib.gmk
else
include ${TOP}/mk/lib_tdmf.gmk
endif
#
# WRs PROD5602 PROD5610, PROD5628
# For HP-UX >= 11.23, enable long host names API (for agent uname() calls)
# Note: we no longer support HP releases < 11.11, so we check just this level
#
ifeq (HP-UX,$(SYSTYPE))
ifneq ($(SYSVERS),1111)
CDEFINES += -D_HPUX_API_LEVEL=20040821
endif
endif

EXTRA_CLEANS+=licprod.c
license.c: licprod.c
licprod.c: licprod.src.c
	$(PKGIFY)
