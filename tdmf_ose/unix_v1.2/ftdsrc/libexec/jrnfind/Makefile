#
# Copyright (c) 2005 Softek Solutions, Inc.  All Rights Reserved.
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

PROGRAM=jrnfind
OBJS=jrnfind.o
SRC=$(OBJS:%.o=%.c)
LIBS=$(LIBGEN) $(SOCKET_LIBS) $(LVM_LIBS)
BIN_INSTALL=$(Q)$(PROGRAM)
USRSBIN_INSTALL=$(Q)$(PROGRAM)
CINCLUDES=$(INCGEN) $(INCDEVICE) $(INCMD5)

include $(TOP)/mk/prog.gmk

ifeq (SunOS,$(SYSTYPE))
GCC = /usr/local/bin/gcc
SPROCC = /opt/SUNWspro/bin/cc
CC = $(shell if [ -f $(SPROCC) ] ; then \
				 echo $(SPROCC) ; \
			 elif [ -f $(GCC) ] ; then \
				 echo $(GCC) ; \
			 else \
				 echo gcc ; \
			 fi)
LARGEFILE_CFLAGS=$(shell getconf LFS_CFLAGS)
LARGEFILE_LDFLAGS=$(shell getconf LFS_LDFLAGS)
LARGEFILE_LIBS=$(shell getconf LFS_LIBS)
endif

ifeq (Linux,$(SYSTYPE))
COPTFLAGS+=$(UCOPTFLAGS)
ifeq (x86_64,$(shell uname -r))
LARGEFILE_CFLAGS=
LARGEFILE_LDFLAGS=
LARGEFILE_LIBS=
else
LARGEFILE_CFLAGS=-D_FILE_OFFSET_BITS=64
LARGEFILE_LDFLAGS=
LARGEFILE_LIBS=
endif
endif

ifeq (HP-UX,$(SYSTYPE))
endif
ifeq (AIX,$(SYSTYPE))
LARGEFILE_CFLAGS=-q32 -D_LARGE_FILES -qlonglong
LARGEFILE_CFLAGS=-q64
LARGEFILE_LDFLAGS=-b32
LARGEFILE_LDFLAGS=-b64
endif

#COPTFLAGS = -g
#CFLAGS	+= -ansi
CFLAGS	+= $(LARGEFILE_CFLAGS)
LDFLAGS += $(LARGEFILE_LDFLAGS)
LIBS	+= $(LARGEFILE_LIBS)

