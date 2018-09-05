#
# Generated automatically from Makefile.in by configure.
# ------------------------------------------------------------------------
# 	Makefile for library files and directories of BLT library
# ------------------------------------------------------------------------

INSTALL      = //bin/install -c
INSTALL_DATA = ${INSTALL} -m 644
TOPDIR       = /usr/local/blt
LIBDIR       = /usr/local/blt/lib
BLT_LIBRARY  = /usr/local/blt/lib/blt2.1
DDPROTO_DIR  = $(BLT_LIBRARY)/dd_protocols
RM           = rm -f
SHELL        = /bin/sh
FILES        = bltGraph.pro bltGraph.tcl tclIndex
srcdir	     = .

all:

install-dirs:
	@for i in $(TOPDIR) $(LIBDIR) $(BLT_LIBRARY) $(DDPROTO_DIR); do \
	    if test -d $$i ; then : ; else mkdir $$i ; fi ; \
        done

install-dragdrop: install-dirs
	@set -x; for i in $(srcdir)/dd_protocols/*tcl* ; do \
	    $(INSTALL_DATA) $$i $(BLT_LIBRARY)/dd_protocols ; \
	done 

install-files: install-dirs
	@set -x; for i in $(srcdir)/$(FILES) ; do \
	    $(INSTALL_DATA) $$i $(BLT_LIBRARY) ; \
	done 

install: install-dirs install-dragdrop install-files

clean:
	$(RM) *.bak *\~ "#"*

