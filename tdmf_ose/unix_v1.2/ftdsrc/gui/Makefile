#
# Copyright (c) 1997, 1998 FullTime Software, Inc.  All rights reserved.
#
# $Id: Makefile,v 1.8 2006/01/06 22:26:46 naat0 Exp $
#

SUBDIRS =
TOP=..

PROGRAMS=${Q}configtool ${Q}perftool ${Q}monitortool ${Q}migratecfg.tcl \
	 ${Q}confutil.tcl \
	 ${CAPQ}logo47.gif ${CAPQ}logo47r.gif ${CAPQ}logo202.gif

BIN_INSTALL=${Q}configtool ${Q}perftool ${Q}monitortool
TCL_INSTALL=${CAPQ}logo47.gif ${CAPQ}logo47r.gif ${CAPQ}logo202.gif ${Q}confutil.tcl  ${Q}migratecfg.tcl
include ${TOP}/mk/Makefile.inc 

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

${CAPQ}logo47.gif: ${GIFPREFIX}logo47.gif
	cp -p ${GIFPREFIX}logo47.gif ${CAPQ}logo47.gif

${CAPQ}logo47r.gif: ${GIFPREFIX}logo47r.gif
	cp -p ${GIFPREFIX}logo47r.gif ${CAPQ}logo47r.gif

${CAPQ}logo202.gif: ${GIFPREFIX}logo202.gif
	cp -p ${GIFPREFIX}logo202.gif ${CAPQ}logo202.gif

#copyright.txt: 
#	echo "Making copyright.txt from ${CAPQ}copyright.txt"
#	cp ${CAPQ}copyright.txt copyright.txt
