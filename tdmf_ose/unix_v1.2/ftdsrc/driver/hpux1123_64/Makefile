#
# Copyright (C) 2005 Softek Storage Solutions, Inc.
#
# $Id: Makefile,v 1.2 2006/01/29 03:03:01 naat0 Exp $
#
# HP-UX 11.23 Makefile
#
# Target      Purpose
# ==========  ====================================================
#   all       Build the module
#   dev       Create the device file(s)
#   install   Install the module in the module directory
#   load      Start using the module
#   status    Display the status the module
#   unload    Stop using the module
#   unconfig  Remove the module from the module directory
#   clobber   remove device files and build output
# ==========  ====================================================

TOP=../..
include ${TOP}/mk/Makefile.inc
EXTRA_CLEANS=${Q}.conf

##################################################################
# Define the architecture to build for - "ipf" or "pa", or let it
# default to matching the running system.
##################################################################
ARCH=

# Put the module name here.  It must match the name in the module's
# metadata file.

MODULE=dtc

# module preparation script if any ...
# so far no

MODPREP=

# If you want this Makefile to create device files on the local system
# for testing, put the names of the block and character device files here,
# and uncomment the corresponding rules below.

DEV_B=
DEV_C=

#############################################################
# Lines below this point normally don't need to be modified #
#############################################################

# Command locations

KCMODULE= /usr/sbin/kcmodule
LSDEV= /usr/sbin/lsdev

# Main build target

all:: ${MODULE}

${MODULE}:
	if [ -n "$(ARCH)" ]; \
	then \
		$(MAKE) -w -r -f Makefile.bld ARCH=$(ARCH) MODULE=${MODULE}; \
	elif [ "`uname -m`" = "ia64" ]; \
	then \
		$(MAKE) -w -r -f Makefile.bld ARCH=ipf MODULE=${MODULE}; \
	else \
		$(MAKE) -w -r -f Makefile.bld ARCH=pa MODULE=${MODULE}; \
	fi
 
dev: $(DEV_B) $(DEV_C)

# Create the block device file (the sample DLKM doesn't have one)

#$(DEV_B):
#	${LSDEV} -hd $(MODULE) | head -n 1 | awk '\
#		{ \
#			rc=system("mknod $(DEV_B) b " $$1 " 0"); \
#			exit rc; \
#		}'

# Create the character device file (the sample DLKM doesn't have one)

#$(DEV_C):
#	${LSDEV} -hd $(MODULE) | head -n 1 | awk '\
#		{ \
#			rc=system("mknod $(DEV_C) c " $$0 " 0"); \
#			exit rc; \
#		}'

# Copy the module to the module directory

install:: ${MODULE}
#	mkdir -p $(DESTDIR)/usr/conf/${Q}
	mkdir -p $(DESTDIR)/usr/conf/mod
	mkdir -p $(DESTDIR)/sbin/init.d
	mkdir -p $(DESTDIR)/sbin/rc3.d
	mkdir -p $(DESTDIR)/etc/opt/${PKGNAME}
	${INSTALL} -m 444 ${Q}.conf ${SAMPLEDIR}
	${INSTALL} -m 444 ${Q}.conf $(DESTDIR)/etc/opt/${PKGNAME}
	${INSTALL} -m 444 ${Q} ${DESTDIR}/usr/conf/mod/${Q}
#	the following line for local testing
#	cp ${MODULE} /usr/conf/mod/${MODULE}
	if [[ -n "${MODPREP}" ]] ; then \
	    cp ${MODPREP} ${DESTDIR}/usr/conf/mod/${MODULE}.prep ; \
	fi
 
# Start using the module

load: install
	${KCMODULE} -s $(MODULE)=best
 
# Display the status of the module

status:
	${KCMODULE} ${MODULE}

# Stop using the module

unload:
	${KCMODULE} $(MODULE)=unused
 
# Remove the module from the module directory

uninstall:
	-${KCMODULE} $(MODULE)=uninstall
	rm -f /usr/conf/mod/${MODULE} /usr/conf/mod/${MODULE}.prep
 
# Remove intermediate files

clean::
	rm -f *.modmeta.c *.o ${MODULE}.modmeta ${MODULE} ${Q}.conf
 
# Unload the module and remove intermediate files, device files, and the module

clobber:: clean uninstall
	rm -f $(DEV_B) $(DEV_C) ${MODULE}
