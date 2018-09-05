#
# Copyright (c) 1997, 1998 FullTime Software, Inc.  All rights reserved.
#
# $Id: Makefile,v 1.4 2001/06/21 17:55:19 bouchard Exp $
#

SUBDIRS =
TOP=..

PROGRAMS=${Q}configtool ${Q}perftool ${Q}monitortool ${Q}migratecfg.tcl \
	 ${Q}confutil.tcl

BIN_INSTALL=${Q}configtool ${Q}perftool ${Q}monitortool ${Q}migratecfg.tcl
TCL_INSTALL=${CAPQ}logo47.gif ${CAPQ}logo47r.gif ${CAPQ}logo202.gif ${Q}confutil.tcl  ${Q}migratecfg.tcl
include ${TOP}/Makefile.inc 

${Q}configtool: configtool.tcl
	$(PKGIFY)
	chmod 755 ${Q}configtool

${Q}perftool: perftool.tcl
	$(PKGIFY)
	chmod 755 ${Q}perftool

${Q}monitortool: monitortool.tcl
	$(PKGIFY)
	chmod 755 ${Q}monitortool

${Q}migratecfg.tcl: migratecfg.tcl
	$(PKGIFY)
	chmod 644 ${Q}migratecfg.tcl

${Q}confutil.tcl: confutil.tcl
	$(PKGIFY)
	chmod 644 ${Q}confutil.tcl

#copyright.txt: 
#	echo "Making copyright.txt from ${CAPQ}copyright.txt"
#	cp ${CAPQ}copyright.txt copyright.txt
