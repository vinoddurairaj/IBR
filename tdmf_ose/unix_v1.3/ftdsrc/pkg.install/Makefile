#
#	Copyright (c) 1997, 1998 FullTime Software, Inc.  All rights reserved.
#

TEXTS=copyright
EXTRA_CLEANS=prototype.sun \
	pkginfo.${PKGNAME} ${Q}.psf hpux.preremove \
	hpux.docs.desc hpux.programs.desc hpux.product.desc hpux.postinstall \
	hpux.checkinstall hpux.postremove hpux.unconfigure hpux.vendor.desc \
	postinstall preremove postremove request # copyright

SUBDIRS=
TOP=..

include ${TOP}/Makefile.inc

CDROMDIR=/tmp/cdrom

both:: common floppy cdrom

ifeq (HP-UX,$(SYSTYPE))
COMMONDEPS=     ${Q}.psf hpux.preremove hpux.docs.desc            \
                hpux.programs.desc hpux.product.desc              \
                hpux.postinstall hpux.postremove hpux.unconfigure \
                hpux.checkinstall hpux.vendor.desc 
else
COMMONDEPS=	pkginfo.${PKGNAME}  \
		postinstall preremove postremove request \
                prototype.sun 
endif

common:: ${COMMONDEPS}
	@if [ "`uname -s`" = "SunOS" ]; then \
		mkdir -p ${PKGNAME}; \
		echo "Making package"; \
		cp pkginfo.${PKGNAME} pkginfo; \
		pkgmk -o -r `pwd`/pkg -d `pwd` -f prototype.sun; \
		touch common; \
	fi

floppy:: common
	@if [ "`uname -s`" = "SunOS" ]; then \
		pkgtrans -s `pwd` `pwd`/${PKGNAME}.${VERSION} ${PKGNAME} ; \
		mv -f $(PKGNAME).${VERSION} ${TOP}; \
		touch floppy; \
	fi
	if [ "`uname -s`" = "HP-UX" ]; then \
		/usr/local/bin/sudo /usr/sbin/swpackage -v -s ${Q}.psf -x write_remote_files=true -x create_target_acls=false -d /tmp/floppy; \
	fi

# For the HPUX mess below:
# 1) swpackage has to run as root. 2) the final tar file may be across the net.
# Since we do not want to write across the net as root, 
#  - Create the package as root (sudo)
#  - tar it up as root
#  - make it readable by all
#  - copy it across the net (or wherever) as non privileged user
#  - remove the evidence
cdrom:: common
	@if [ "`uname -s`" = "SunOS" ]; then \
		tar cf ${PKGNAME}.${VERSION}.tar ${PKGNAME}; \
		mv -f ${PKGNAME}.${VERSION}.tar ${TOP}; \
	fi
	@if [ "`uname -s`" = "HP-UX" ]; then \
		echo /usr/local/bin/sudo rm -rf /var/tmp/buildrep; \
		/usr/local/bin/sudo rm -rf /var/tmp/buildrep; \
		echo /usr/local/bin/sudo /usr/sbin/swpackage -v -s ${Q}.psf -x write_remote_files=true -x create_target_acls=false @ /var/tmp/buildrep; \
		/usr/local/bin/sudo /usr/sbin/swpackage -v -s ${Q}.psf -x write_remote_files=true -x create_target_acls=false @ /var/tmp/buildrep; \
		echo /usr/local/bin/sudo tar cf /var/tmp/buildrep.tar -C /var/tmp/buildrep .; \
		/usr/local/bin/sudo tar cf /var/tmp/buildrep.tar -C /var/tmp/buildrep .; \
		echo /usr/local/bin/sudo chmod 666 /var/tmp/buildrep.tar \
		/usr/local/bin/sudo chmod 666 /var/tmp/buildrep.tar \
		echo mkdir -p ${CDROMDIR}; \
		mkdir -p ${CDROMDIR}; \
		echo cp /var/tmp/buildrep.tar ${CDROMDIR}/sw.tar; \
		cp /var/tmp/buildrep.tar ${CDROMDIR}/sw.tar; \
		echo /usr/local/bin/sudo rm -rf /var/tmp/buildrep /var/tmp/buildrep.tar; \
		/usr/local/bin/sudo rm -rf /var/tmp/buildrep /var/tmp/buildrep.tar; \
	fi

clean::
	@rm -rf pkg ${PKGNAME} pkginfo common floppy cdrom 

pkginfo.${PKGNAME}: pkginfo.core.src
	$(PKGIFY)

# variant package manifests...
ifeq (SunOS,$(SYSTYPE))
ifeq ($(SYSVERS),551)
prototype.sun: prototype_v8.src
	$(PKGIFY)
else
ifeq ($(SYSVERS),560)
prototype.sun: prototype_v8.src
	$(PKGIFY)
else
prototype.sun: prototype_v9.src
	$(PKGIFY)
endif
endif
endif

# DTurrin - Modified the "if" statement
#           to check for exact version
#           of the HP-UX OS
#           (August 17th, 2001)
ifeq (HP-UX,$(SYSTYPE))
ifeq ($(SYSVERS),1020)
${Q}.psf: source_1020.psf
	$(PKGIFY)
endif
ifeq ($(SYSVERS),1100)
${Q}.psf: source_1100.psf
	$(PKGIFY)
endif
ifeq ($(SYSVERS),1111)
${Q}.psf: source_1111.psf
	$(PKGIFY)
endif
endif

postinstall: sun.postinstall.src
	$(PKGIFY)
postremove: sun.postremove.src
	$(PKGIFY)
preremove: sun.preremove.src
	$(PKGIFY)
request: sun.request.src
	$(PKGIFY)
hpux.checkinstall: hpux.checkinstall.src
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
