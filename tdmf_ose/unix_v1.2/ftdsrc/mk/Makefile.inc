#
# Copyright (c) 1997, 1998 FullTime Software, Inc.  All Rights Reserved
#
# Include this last after you've made your definitions in your makefile.
#
# $Id: Makefile.inc,v 1.15 2008/12/05 08:11:48 naat0 Exp $
#
#
ifneq (,$(PREDIR))
BUILDBRAND=$(shell ${PREDIR}/${TOP}/mk/getosarch.sh)
else
BUILDBRAND=$(shell ${TOP}/mk/getosarch.sh)
endif

ifeq (${BUILDBRAND},)
$(error Unable to collect the operating system architecture information)
endif

ifneq (,$(findstring 2.6,$(shell uname -r)))
ifneq (,$(PREDIR))
include $(PREDIR)/${TOP}/mk/vars.gmk
else
include ${TOP}/mk/vars.gmk
endif
else
ifeq ($(wildcard ${TOP}/mk/vars_tdmf_${BUILDBRAND}.gmk),)
include ${TOP}/mk/vars.gmk
else
include ${TOP}/mk/vars_tdmf_${BUILDBRAND}.gmk
endif
endif

_MAKE_SUBDIRS=	\
	@for i in ${SUBDIRS} x; do \
		if [ $$i != x ]; then \
			echo Making $@ in $$i; \
			(cd $$i ; $(MAKE) VERSION=$(VERSION) $@) || exit $$?; \
		fi; \
	done

all::	_FORCE $(PROGRAMS) 
	$(_MAKE_SUBDIRS)
clean::	_FORCE
	$(_MAKE_SUBDIRS)
	@for i in ${CLEANS} x; do \
		if [ $$i != x ]; then \
			$(RM) -r $$i; \
		fi ; \
	done
install:: _FORCE
	$(_MAKE_SUBDIRS)
	@mkdir -p ${BINDIR} ${LIBDIR} ${ETCDIR} ${MANDIR} ${TCLDIR} \
		${SAMPLEDIR} ${SBINDIR} ${PKGUSRSBINDIR} ${INITDIR} \
		${PKGDIR} ${CONFIGDIR} \
		${HTMLDIR} ${IMGDIR} ${PSDIR} ${LIBEXECDIR} ${PDFDIR}\
		${RUNDIR} ${HTMLIMGDIR} ${HACMPDIR};\
	for i in ${BIN_INSTALL} x; do \
		if [ $$i != x ]; then \
			${INSTALL} $$i ${BINDIR}; \
		fi; \
	done ; \
	for i in ${HACMP_INSTALL} x; do \
		if [ $$i != x ]; then \
			${INSTALL} $$i ${HACMPDIR}; \
		fi; \
	done ; \
	for i in ${LIBEXEC_INSTALL} x; do \
		if [ $$i != x ]; then \
			${INSTALL} $$i ${LIBEXECDIR}; \
		fi; \
	done ; \
	for i in ${HTML_INSTALL} x; do \
		if [ $$i != x ]; then \
			${INSTALL} -m 444 $$i ${HTMLDIR}; \
		fi; \
	done ; \
	for i in ${IMG_INSTALL} x; do \
		if [ $$i != x ]; then \
			${INSTALL} -m 444 $$i ${IMGDIR}; \
		fi; \
	done ; \
        for i in ${HTMLIMG_INSTALL} x; do \
                if [ $$i != x ]; then \
                        ${INSTALL} -m 444 $$i ${HTMLIMGDIR}; \
                fi; \
        done ; \
        for i in ${PDF_INSTALL} x; do \
                if [ $$i != x ]; then \
                        ${INSTALL} -m 444 $$i ${PDFDIR}; \
                fi; \
        done ; \
        for i in ${HTMLIMG_INSTALL} x; do \
                if [ $$i != x ]; then \
                        ${INSTALL} -m 444 $$i ${HTMLIMGDIR}; \
                fi; \
        done ; \
	for i in ${PS_INSTALL} x; do \
		if [ $$i != x ]; then \
			${INSTALL} -m 444 $$i ${PSDIR}; \
		fi; \
	done ; \
	for i in ${TOP_INSTALL} x; do \
		if [ $$i != x ]; then \
			${INSTALL} -m 444 $$i ${TOPDIR}; \
		fi; \
	done ; \
	for i in ${USRSBIN_INSTALL} x; do \
		if [ $$i != x ]; then \
			${INSTALL} -m 444 $$i ${PKGUSRSBINDIR}; \
		fi; \
	done ; \
	for i in ${SBIN_INSTALL} x; do \
		if [ $$i != x ]; then \
			${INSTALL} -m 444 $$i ${SBINDIR}; \
		fi; \
	done ; \
	for i in ${INIT_INSTALL} x; do \
		if [ $$i != x ]; then \
			${INSTALL} -m 444 $$i ${INITDIR}; \
		fi; \
	done ; \
	for i in ${CONFIG_INSTALL} x; do \
		if [ $$i != x ]; then \
			${INSTALL} -m 444 $$i ${CONFIGDIR}; \
		fi; \
	done ; \
	for i in ${ETC_INSTALL} x; do \
		if [ $$i != x ]; then \
			${INSTALL} -m 444 $$i ${ETCDIR}; \
		fi; \
	done ; \
	for i in ${PKG_INSTALL} x; do \
		if [ $$i != x ]; then \
			${INSTALL} -m 444 $$i ${PKGDIR}; \
		fi; \
	done ; \
	for i in ${LIB_INSTALL} x; do \
		if [ $$i != x ]; then \
			${INSTALL} -m 444 $$i ${LIBDIR}; \
		fi; \
	done ; \
	for i in ${MAN_INSTALL} x; do \
		if [ $$i != x ]; then \
			${INSTALL} -m 444 $$i ${MANDIR}; \
		fi; \
	done ; \
	for i in ${TCL_INSTALL} x; do \
		if [ $$i != x ]; then \
			${INSTALL} -m 444 $$i ${TCLDIR}; \
		fi; \
	done ; \
	for i in ${SAMPLE_INSTALL} x; do \
		if [ $$i != x ]; then \
			${INSTALL} -m 444 $$i ${SAMPLEDIR}; \
		fi; \
	done ;

_FORCE:

.c.o:
	${CC} ${CFLAGS} -c $<

ifneq (,$(findstring 2.6,$(shell uname -r)))
ifneq (,$(PREDIR))
include $(PREDIR)/$(TOP)/mk/depend.gmk
else
include ${TOP}/mk/depend.gmk
endif
else
include ${TOP}/mk/depend.gmk
endif
