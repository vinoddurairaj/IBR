#
#	Copyright (c) 1997, 1998 FullTime Software, Inc.  All rights reserved.
#

EXTRA_CLEANS=prototype.sun \
	pkginfo.${PKGNAME} ${Q}.psf hpux.checkremove hpux.preremove \
	hpux.docs.desc hpux.programs.desc hpux.product.desc hpux.postinstall \
	hpux.checkinstall hpux.postremove hpux.unconfigure hpux.vendor.desc \
	postinstall preremove postremove request hpux.docs.postremove copyright \
	validated_AIX_platforms.txt SFTKdtc_AIX_failover_boot_network_reconfig.sh

SUBDIRS=
TOP=..

include ${TOP}/mk/Makefile.inc

ifeq (AIX,$(SYSTYPE))
TEXTS=copyright validated_AIX_platforms.txt
ETC_INSTALL=readme.txt validated_AIX_platforms.txt ${Q}Tracking_Resolutions.txt \
    dtc_pre_failover_pxxx.sh dtc_post_failover_pxxx.sh dtc_post_failover_sxxx.sh SFTKdtc_AIX_failover_boot_network_reconfig.sh DTC.lic.perm
else
ifeq (Linux,$(SYSTYPE))
TEXTS=copyright
ETC_INSTALL=readme.txt ${Q}Tracking_Resolutions.txt \
    dtc_pre_failover_pxxx.sh dtc_post_failover_pxxx.sh dtc_post_failover_sxxx.sh \
    dtc_Linux_bootdrive_post_failover.sh DTC.lic.perm
else
TEXTS=copyright
ETC_INSTALL=readme.txt ${Q}Tracking_Resolutions.txt \
    dtc_pre_failover_pxxx.sh dtc_post_failover_pxxx.sh dtc_post_failover_sxxx.sh DTC.lic.perm
endif
endif

CDROMDIR=${TOP}/CDROMDIR
ifeq (Linux,$(SYSTYPE))
BUILDROOTDIR=${CURDIR}/${TOP}/${PKGROOTDIR}
BUILDROOT=${BUILDROOTDIR}/${BUILDNUM}
endif

both:: common cdrom

ifeq (HP-UX,$(SYSTYPE))
COMMONDEPS=     ${Q}.psf hpux.checkremove hpux.preremove hpux.docs.desc \
                hpux.programs.desc hpux.product.desc copyright   \
                hpux.postinstall hpux.postremove hpux.unconfigure \
                hpux.checkinstall hpux.vendor.desc hpux.docs.postremove
else
ifeq (Linux,$(SYSTYPE))
ifneq (,$(findstring ${ARCH},i686 x86_64))
COMMONDEPS=	copyright \
		${PKGNAME}-$(SYSTYPE)-${VERSION}-${BUILDNUM}.spec \
		${PKGNAME}-BAD-$(SYSTYPE)-${VERSION}-${BUILDNUM}.spec \
		OMP-$(SYSTYPE)-${VERSION}-${BUILDNUM}.spec
else
COMMONDEPS=	copyright \
		${PKGNAME}-$(SYSTYPE)-${VERSION}-${BUILDNUM}.spec \
		${PKGNAME}-BAD-$(SYSTYPE)-${VERSION}-${BUILDNUM}.spec
endif
RPMSDIR=	${BUILDROOTDIR}/RPMS/${ARCH}
MODULES_EXCLUDED:=$(strip $(shell make --no-print-directory -C ../driver/Linux TARGET_ISA=`uname -m` list_excluded_modules))
else
ifeq (AIX,$(SYSTYPE))
COMMONDEPS=	copyright validated_AIX_platforms.txt SFTKdtc_AIX_failover_boot_network_reconfig.sh
else
COMMONDEPS=	pkginfo.${PKGNAME} copyright \
		postinstall preremove postremove request \
                prototype.sun
endif 
endif
endif

# @todo buildroot is the same logical space as the pkg directory.
# @todo We should look to see if we can collapse the double space usage

common:: ${COMMONDEPS}
	@echo "Time: Before package" `date`
ifeq (SunOS,$(SYSTYPE))
	@mkdir -p ${PKGNAME}
	@echo "Making package"
	@cp pkginfo.${PKGNAME} pkginfo
	@pkgmk -o -l 200000 -r `pwd`/pkg -d `pwd` -f prototype.sun
	@pkgtrans -s `pwd`/ SFTKdtc.${VERSION}.pkg all
	@touch common
endif
ifeq (Linux,$(SYSTYPE))
	@rm -rf ${BUILDROOT}
ifeq ($(SYSVERS),2632)
	@/usr/bin/rpmbuild -bb --define "CURDIR ${CURDIR}" --define "BUILDROOT ${BUILDROOT}" --define "OPTDIR ${OPTDIR}" --define "ETCOPTDIR ${ETCOPTDIR}" --define "OEM ${OEM}" --define "dtcQ ${Q}" --define "VAROPTDIR ${VAROPTDIR}" --define "BUILDROOTDIR ${BUILDROOTDIR}" --define "RPMSDIR ${RPMSDIR}" --define "PRODUCTNAME_TOKEN ${PRODUCTNAME_TOKEN}-BAD" --define "_topdir ${BUILDROOTDIR}" --buildroot ${BUILDROOT} --target ${ARCH} ${PKGNAME}-BAD-$(SYSTYPE)-${VERSION}-${BUILDNUM}.spec
	@/usr/bin/rpmbuild -bb --define "CURDIR ${CURDIR}" --define "BUILDROOT ${BUILDROOT}" --define "OPTDIR ${OPTDIR}" --define "ETCOPTDIR ${ETCOPTDIR}" --define "OEM ${OEM}" --define "dtcQ ${Q}" --define "VAROPTDIR ${VAROPTDIR}" --define "PRODUCTNAME_TOKEN ${PRODUCTNAME_TOKEN}" --define "_topdir ${BUILDROOTDIR}" --define "MODULES_EXCLUDED ${MODULES_EXCLUDED}" --buildroot ${BUILDROOT} --target ${ARCH} ${PKGNAME}-$(SYSTYPE)-${VERSION}-${BUILDNUM}.spec
else	
	@mkdir -p ${BUILDROOT}
	@mkdir -p ${BUILDROOT}/${OPTDIR}
	@mkdir -p ${BUILDROOT}/${ETCOPTDIR}/${OEM}${Q}
	@mkdir -p ${BUILDROOT}/${VAROPTDIR}/${OEM}${Q}
	@mkdir -p ${BUILDROOT}/${VAROPTDIR}/${OEM}${Q}/Agn_tmp
	@mkdir -p ${BUILDROOT}/etc/init.d
	@mkdir -p ${BUILDROOT}/sbin
	@mkdir -p ${BUILDROOT}/usr/sbin
	@mkdir -p ${BUILDROOT}/var/run/${OEM}${Q}
	@mkdir -p ${BUILDROOTDIR}/BUILD ${RPMSDIR}
	@mkdir -p ${BUILDROOTDIR}/SPEC
	@cp -pR `pwd`/pkg/${OEM}${Q} ${BUILDROOT}/${OPTDIR}
	@cp -p `pwd`/pkg/etc/init.d/${OEM}${Q}-* ${BUILDROOT}/etc/init.d
	@cp -pR `pwd`/pkg/etc/opt/${OEM}${Q} ${BUILDROOT}/${ETCOPTDIR}
	@cp -pR `pwd`/pkg/sbin/ ${BUILDROOT}
	@cp -pR `pwd`/pkg/usr/sbin/ ${BUILDROOT}/usr
	@/usr/bin/rpmbuild -bb --define "PRODUCTNAME_TOKEN ${PRODUCTNAME_TOKEN}-BAD" --define "_topdir ${BUILDROOTDIR}" --buildroot ${BUILDROOT} --target ${ARCH} ${PKGNAME}-BAD-$(SYSTYPE)-${VERSION}-${BUILDNUM}.spec
	for x in ${MODULES_EXCLUDED} ; do rm -f ${BUILDROOT}/${ETCOPTDIR}/${OEM}${Q}/driver/$${x} ; done
	rmdir --ignore-fail-on-non-empty ${BUILDROOT}/${ETCOPTDIR}/${OEM}${Q}/driver/linux-*
	@/usr/bin/rpmbuild -bb --define "PRODUCTNAME_TOKEN ${PRODUCTNAME_TOKEN}" --define "_topdir ${BUILDROOTDIR}" --buildroot ${BUILDROOT} --target ${ARCH} ${PKGNAME}-$(SYSTYPE)-${VERSION}-${BUILDNUM}.spec
endif	
ifneq (,$(findstring ${ARCH},i686 x86_64))
	@rm -rf ${BUILDROOT}
	@mkdir -p ${BUILDROOT}/${ETCOPTDIR}/${OEM}omp/bin
	@mkdir -p ${BUILDROOT}/${ETCOPTDIR}/${OEM}omp/docs
	@mkdir -p ${BUILDROOT}/${ETCOPTDIR}/${OEM}omp/cfg
	@mkdir -p ${BUILDROOT}/${ETCOPTDIR}/${OEM}omp/done
	@mkdir -p ${BUILDROOT}/${ETCOPTDIR}/${OEM}omp/log
	@mkdir -p ${BUILDROOT}/${ETCOPTDIR}/${OEM}omp/tmp
	@mkdir -p ${BUILDROOT}/${ETCOPTDIR}/${OEM}omp/jobs/staging
	@mkdir -p ${BUILDROOT}/${ETCOPTDIR}/${OEM}omp/jobs/san
	@mkdir -p ${BUILDROOT}/${VAROPTDIR}/${OEM}omp
	@mkdir -p ${BUILDROOT}/etc/profile.d
	@cp -p `pwd`/pkg/etc/profile.d/omp.*sh ${BUILDROOT}/etc/profile.d
	@cp -pR `pwd`/pkg/etc/opt/${OEM}omp ${BUILDROOT}/${ETCOPTDIR}
	@/usr/bin/rpmbuild -bb --define "_topdir ${BUILDROOTDIR}" --buildroot ${BUILDROOT} --target ${ARCH} OMP-$(SYSTYPE)-${VERSION}-${BUILDNUM}.spec
	@rm -rf ${BUILDROOT}
endif
endif
	@touch common

cdrom:: common
ifeq (SunOS,$(SYSTYPE))
	@tar cf ${PKGNAME}.${VERSION}.tar ${PKGNAME}
	@mv -f ${PKGNAME}.${VERSION}.tar ${TOP}
endif
ifeq (HP-UX,$(SYSTYPE))
	@mkdir -p ${CDROMDIR}
	@/usr/sbin/swpackage -s ${Q}.psf -x run_as_superuser=false -x media_type=tape @ ${CDROMDIR}/${SYSTYPE}-${VERSION}-${BUILDNUM}.depot
endif

clean::
	@rm -rf pkg ${PKGNAME} pkginfo common cdrom 
ifeq (Linux,$(SYSTYPE))
	@rm -f  ${PKGNAME}-$(SYSTYPE)-${VERSION}-*.spec
	@rm -f  ${PKGNAME}-BAD-$(SYSTYPE)-${VERSION}-*.spec
	@rm -f  OMP-$(SYSTYPE)-${VERSION}-*.spec
	@rm -rf ${BUILDROOTDIR}
endif

clean-pkg:
ifeq (Linux,$(SYSTYPE))
	@$(RM) ${PKGNAME}-$(SYSTYPE)-${VERSION}-*.spec
	@rm -f  ${PKGNAME}-BAD-$(SYSTYPE)-${VERSION}-*.spec
	@rm -f  OMP-$(SYSTYPE)-${VERSION}-*.spec
	@rm -rf ${BUILDROOTDIR}
endif

pkginfo.${PKGNAME}: pkginfo.core.src
	$(PKGIFY)

# variant package manifests...
ifeq (SunOS,$(SYSTYPE))
prototype.sun: prototype_v9.src
	$(PKGIFY)
endif

ifeq (HP-UX,$(SYSTYPE))
${Q}.psf: source.psf
	$(PKGIFY)
endif

copyright: Softekcopyright.txt
	$(PKGIFY)

%: sun.%.src
	$(PKGIFY)

hpux.%: hpux.%.src
	$(PKGIFY)

ifeq (AIX,$(SYSTYPE))
validated_AIX_platforms.txt: validated_AIX_platforms.txt.src
	$(PKGIFY)
SFTKdtc_AIX_failover_boot_network_reconfig.sh: SFTKdtc_AIX_failover_boot_network_reconfig.sh.src
	$(PKGIFY)
endif

ifeq (Linux,$(SYSTYPE))

tdmf_linux-BAD-$(ARCH).spec: tdmf_linux26.spec
	@(echo r tdmf_linux26.spec ; \
	 echo '/<<driver modules>>/d' ; echo .-1; echo 'a' ; \
	 for x in `make -C ../driver/Linux TARGET_ISA=\`uname -m\` list_modules` ; do \
		echo '%dir %attr(0755,root,root) /etc/opt/%OEM%%Q%/driver'/`dirname $${x}` ; \
		echo '%attr(0555,root,root) /etc/opt/%OEM%%Q%/driver'/$${x} ; \
	 done ; \
	 echo '.' ; echo f $@; echo w ; echo q) | ed -s

tdmf_linux-$(ARCH).spec: tdmf_linux26.spec
	@(echo r tdmf_linux26.spec ; \
	 echo '/<<driver modules>>/d' ; echo .-1; echo 'a' ; \
	 for x in `make -C ../driver/Linux TARGET_ISA=\`uname -m\` list_included_modules` ; do \
		echo '%dir %attr(0755,root,root) /etc/opt/%OEM%%Q%/driver'/`dirname $${x}` ; \
		echo '%attr(0555,root,root) /etc/opt/%OEM%%Q%/driver'/$${x} ; \
	 done ; \
	 echo '.' ; echo f $@; echo w ; echo q) | ed -s


$(PKGNAME)-$(SYSTYPE)-$(VERSION)-$(BUILDNUM).spec: tdmf_linux-$(ARCH).spec
	$(PKGIFY)
	@$(RM) tdmf_linux-$(ARCH).spec

$(PKGNAME)-BAD-$(SYSTYPE)-$(VERSION)-$(BUILDNUM).spec: tdmf_linux-BAD-$(ARCH).spec
	$(PKGIFY)
	@$(RM) tdmf_linux-BAD-$(ARCH).spec

OMP-$(SYSTYPE)-$(VERSION)-$(BUILDNUM).spec: omp_linux.spec
	$(PKGIFY)
	@$(RM) omp_linux-$(ARCH).spec
endif
