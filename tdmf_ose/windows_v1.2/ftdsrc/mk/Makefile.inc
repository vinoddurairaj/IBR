#
# Copyright (c) 1997, 1998 FullTime Software, Inc.  All Rights Reserved
#
# Include this last after you've made your definitions in your makefile.
#
# $Id: Makefile.inc,v 1.1.1.1 2001/09/14 13:48:44 dturrin Exp $
#
#
include ${TOP}/mk/vars.gmk

_MAKE_SUBDIRS=	\
	@for i in ${SUBDIRS} x; do \
		if [ $$i != x ]; then \
			echo Making $@ in $$i; \
			(cd $$i ; $(MAKE) VERSION=$(VERSION) $@); \
		fi; \
	done

all::	_FORCE $(PROGRAMS) $(TEXTS)
	$(_MAKE_SUBDIRS)
clean::	_FORCE
	$(_MAKE_SUBDIRS)
	@for i in ${CLEANS} x; do \
		if [ $$i != x ]; then \
			$(RM) $$i; \
		fi ; \
	done
install:: _FORCE
	$(_MAKE_SUBDIRS)
	@mkdir -p ${BINDIR} ${LIBDIR} ${ETCDIR} ${MANDIR} ${TCLDIR} \
		${SAMPLEDIR} ${PKGUSRSBINDIR} ${INITDIR} ${PKGDIR} ${CONFIGDIR} \
		${HTMLDIR} ${IMGDIR} ${PSDIR} ${LIBEXECDIR} ${PDFDIR}\
		${HTMLIMGDIR};\
	for i in ${BIN_INSTALL} x; do \
		if [ $$i != x ]; then \
			${INSTALL} -m 555 $$i ${BINDIR}; \
		fi; \
	done ; \
	for i in ${LIBEXEC_INSTALL} x; do \
		if [ $$i != x ]; then \
			${INSTALL} -m 555 $$i ${LIBEXECDIR}; \
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

include ${TOP}/mk/depend.gmk
