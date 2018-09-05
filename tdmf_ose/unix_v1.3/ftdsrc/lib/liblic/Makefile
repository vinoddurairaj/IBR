#
# Copyright (c) 1996, 1998 FullTime Software, Inc. All Rights Reserved.
#
TOP=../..

OBJS= license.o	licplat.o
LIBRARY=lic
CINCLUDES=${INCGEN}

include ${TOP}/mk/lib.gmk

EXTRA_CLEANS+=licprod.c
license.c: licprod.c
licprod.c: licprod.src.c
	$(PKGIFY)
