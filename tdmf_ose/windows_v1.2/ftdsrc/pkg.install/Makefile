#
#	Copyright (c) 1997, 1998 FullTime Software, Inc.  All rights reserved.
#

TEXTS=copyright
EXTRA_CLEANS=prototype.sun prototype.dsun \
	pkginfo.${PKGNAME} pkginfo.${DOCPKGNAME} ${Q}.psf hpux.preremove \
	hpux.docs.desc hpux.programs.desc hpux.product.desc hpux.postinstall \
	hpux.postremove hpux.unconfigure hpux.vendor.desc postinstall\
	preremove postremove request copyright
	
SUBDIRS=
TOP=..

include ${TOP}/Makefile.inc

CLEAN+=copyright

copyright:
	@ echo "Making copyright from ${CAPQ}copyright.txt"
	cp ${CAPQ}copyright.txt copyright



CDROMDIR=/tmp/cdrom

both:: common floppy cdrom

common:: pkginfo.${PKGNAME} pkginfo.${DOCPKGNAME} prototype.sun prototype.dsun ${Q}.psf hpux.preremove hpux.docs.desc hpux.programs.desc hpux.product.desc hpux.postinstall hpux.postremove hpux.unconfigure hpux.vendor.desc postinstall preremove postremove request
	@if [ "`uname -s`" = "SunOS" ]; then \
		mkdir -p ${PKGNAME}; \
		echo "Making packages"; \
		cp pkginfo.${PKGNAME} pkginfo; \
		pkgmk -o -r `pwd`/pkg -d `pwd` -f prototype.sun; \
		cp pkginfo.${DOCPKGNAME} pkginfo; \
		pkgmk -o -r `pwd`/pkg -d `pwd` -f prototype.dsun; \
		touch common; \
	fi

floppy:: common
	@if [ "`uname -s`" = "SunOS" ]; then \
		pkgtrans -s `pwd` `pwd`/${PKGNAME}.${VERSION} ${PKGNAME} ${DOCPKGNAME}; \
		mv -f $(PKGNAME).${VERSION} ${TOP}; \
		touch floppy; \
	fi
	@if [ "`uname -s`" = "HP-UX" ]; then \
		swpackage -v -s ${Q}.psf -x create_target_acls=false -d /tmp/floppy; \
	fi

cdrom:: common
	@if [ "`uname -s`" = "SunOS" ]; then \
		tar cf ${PKGNAME}.${VERSION}.tar ${PKGNAME} ${DOCPKGNAME}; \
		mv -f ${PKGNAME}.${VERSION}.tar ${TOP}; \
		touch cdrom; \
	fi
	@if [ "`uname -s`" = "HP-UX" ]; then \
		swpackage -v -s ${Q}.psf -x create_target_acls=false -d ${CDROMDIR}; \
	fi

clean::
	@rm -rf pkg ${PKGNAME} ${DOCPKGNAME} pkginfo common floppy cdrom 

pkginfo.${PKGNAME}: pkginfo.core.src
	$(PKGIFY)
pkginfo.${DOCPKGNAME}: pkginfo.doc.src
	$(PKGIFY)
prototype.sun: prototype.src
	$(PKGIFY)
prototype.dsun: prototype.doc
	$(PKGIFY)
#
# prototype doc file has brandisms
#
prototype.doc: ${CAPQ}prototype.doc
	echo "Making prototype.doc from ${CAPQ}prototype.doc"
	rm -f prototype.doc
	cp ${CAPQ}prototype.doc prototype.doc
${Q}.psf: source.psf 
	$(PKGIFY)
postinstall: sun.postinstall.src
	$(PKGIFY)
postremove: sun.postremove.src
	$(PKGIFY)
preremove: sun.preremove.src
	$(PKGIFY)
request: sun.request.src
	$(PKGIFY)
hpux.postinstall: hpux.postinstall.src
	$(PKGIFY)
hpux.preremove: hpux.preremove.src
	$(PKGIFY)
hpux.postremove: hpux.postremove.src
	$(PKGIFY)
hpux.unconfigure: hpux.unconfigure.src
	$(PKGIFY)
hpux.product.desc: hpux.product.desc.src
	$(PKGIFY)
hpux.programs.desc: hpux.programs.desc.src
	$(PKGIFY)
hpux.docs.desc: hpux.docs.desc.src
	$(PKGIFY)
hpux.vendor.desc: hpux.vendor.desc.src 
	$(PKGIFY)
