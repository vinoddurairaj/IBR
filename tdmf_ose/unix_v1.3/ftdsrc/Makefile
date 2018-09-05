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
# These files assume the normal Solaris Make facilities.  gnumake may also
# work as well.
#
# The "all" target builds all the software.
# The "clean" target removes all built software.
# The "install" target will install the software into a subdir of pkg.install
#
# see the end of this file for details on gold mastering process
#
# See warner if you have any questions on the build system.
#
# $Id: Makefile,v 1.3 2001/01/17 17:58:55 hutch Exp $
#

SUBDIRS = lib daemons bin libexec driver init gui doc pkg.install
ifeq (imp,$(USER))
SUBDIRS += qa
endif
EXTRA_CLEANS=version.o
TOP_INSTALL=README
TOP=.

all::	version.o

include ${TOP}/mk/subdir.gmk

floppy::	all install
	@(cd pkg.install; $(MAKE) floppy)

cdrom::	all install
	@(cd pkg.install; $(MAKE) cdrom)

sys-install::	cdrom
	pkgadd -d ./pkg.install

build::
	make all
	make install
	@(cd pkg.install; $(MAKE) floppy

tag:
	@cvs tag ftd-$(CVS_REV)

gold-master: tag cdrom
	@echo "Gold master process complete"

branch:
	@echo XXX BRANCH IS EXPERIMENTAL, CHECK THE RESULTS XXX
	@cvs tag -b FTD-$(CVS_MAJREV) ftd-$(CVS_REV)
	@echo XXX BRANCH IS EXPERIMENTAL, CHECK THE RESULTS XXX

#
# we have three special targets that we add.  The first one is cdrom.
# this will build a cdrom image, tar it up.  The second is floppy.  This will
# produce a single file that has all of the files in it suitable for feeding
# to pkgadd.  Finally, build is used to make the nightly builds.  It just
# builds things with today's date encoded into the VERSION string we build
# into all the binaries.  VERSION is defined in Makefile.inc, so if you
# want to change it, change it there.
#
# There are three other targets that we add as well.  branch, tag and 
# gold-master.  these are generally not used in the day to day operations, 
# but instead are used by the release engineer to build a gold-master and 
# to tag the cvs tree that results.  make tag will make the current files
# that you have checked out as whatever the VERSION string is set to, subject
# to our release rules.  make branch will branch off the current versions
# so that we can have two lines of development (for things like bug fixes
# while we ready the next release).
#
# So you want to make a gold master, but Warner is out of the country?
#
# Follow the following steps, on sophie, and you should have a gold
# master.  Please make sure that you label the release.
#
# % cd /some/where/interesting
# % co -d /home/Cvs co SafeData
# % cd SafeData
# % make gold-master
#
# At this point, you should have a file called qdspkg-x.y.z.tar in
# /some/where/interesting/SafeData.  Burn the cdrom with this image.
#
# The nightly builds are handled by a script in ~imp/bin called nightly-build.
# If you desire to create a build, then you can use this script to do that.
# Alternatively, if you want to create a build from a tree you are working
# on and have checked out, you can do that too.  Just do make build, and it
# will do the right thing.  You may need to rm SafeData/version.o before
# doing this to get the right version number into all the binaries, if that
# is important to you.
#
