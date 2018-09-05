# %PRODUCTNAME% Linux spec File
#/bin/uname -r|grep "2.6" 1 > /dev/null 2>&1
Summary: Offline Migration Package
Name: OMP
Version: %VERSION% 
Release: %BUILDNUM%
License: %COPYRIGHTYEAR 2001%, %COMPANYNAME%. All Rights Reserved.
Vendor: %COMPANYNAME%.
Group: system
#Source: 


%description
IBM Softek TDMF Offline Migration Package is a toolkit to assist migrations
for SAN environments.  The migrations are performed in an offline fashion to
support platforms that are not directly supported by the TDMF product line.

This version of the OMP is meant to be used in conjunction with TDMF UNIX (IP).

%pre

check_distribution() {

    [ ${OSARCH} = ${PKGARCH} ] || return 1

    if [ -f ${REDHAT_DIST} ]
    then
	case "$1" in
	    2.6.9-*)
		RELEASE_OUTPUT=`/bin/grep "${RELEASE_STRING}" ${REDHAT_DIST}`
		;;
	    2.6.18-*)
		RELEASE_OUTPUT=`/bin/grep "${RELEASE_STRING}" ${REDHAT_DIST}`
		;;
	    2.6.32-*)
		RELEASE_OUTPUT=`/bin/grep "${RELEASE_STRING}" ${REDHAT_DIST}`
		;;		
	    *)
		RELEASE_OUTPUT="FOUND"
		/bin/false
		;;
	    esac
    elif [ -f ${SUSE_DIST} ]
    then
	case "$1" in
	    2.6.5-*)
		RELEASE_OUTPUT=`/bin/grep "${RELEASE_STRING}" ${SUSE_DIST}`
		;;
	    2.6.16.*)
		RELEASE_OUTPUT=`/bin/grep "${RELEASE_STRING}" ${SUSE_DIST}`
		;;
	    *)
		RELEASE_OUTPUT="FOUND"
		/bin/false
		;;
	    esac
    else
    	echo ""
%ifdistribution == suse
    	echo "ERROR: Could not find the ${SUSE_DIST} file"
%endif
%ifdistribution == redhat
    	echo "ERROR: Could not find the ${REDHAT_DIST} file"
%endif
	/bin/false
    fi
    return $?
}

RELEASE_OUTPUT="NOTFOUND"
REDHAT_DIST=/etc/redhat-release
RELEASE_STRING="Unknown Linux Release"
OSARCH=`uname -m`
PKGARCH="%ARCH%"

%ifdistribution == redhat
RELEASE_STRING="Redhat release is not set"
%ifdistrelease == 4
RELEASE_STRING="Red Hat Enterprise Linux [EAW]S release 4"
%endif
%ifdistrelease == 5
RELEASE_STRING="Red Hat Enterprise Linux Server release 5"
%endif
%ifdistrelease == 6
RELEASE_STRING="Red Hat Enterprise Linux Server release 6"
%endif
%ifdistrelease == 7
RELEASE_STRING="Red Hat Enterprise Linux Server release 7"
%endif
%endif

SUSE_DIST=/etc/SuSE-release
%ifdistribution == suse
RELEASE_STRING="SuSE release is not set"
%ifdistrelease == 9
RELEASE_STRING="SUSE LINUX Enterprise Server 9"
%endif
%ifdistrelease == 10
RELEASE_STRING="SUSE Linux Enterprise Server 10"
%endif
%endif

PCRELEASE=`/bin/uname -r`
if ! check_distribution "${PCRELEASE}" ; then
    echo "ERROR: The package selected for installation differs from the expected"
    echo "ERROR: target system.  Please select the correct package for this system."
    if [ "${RELEASE_OUTPUT}" = "FOUND" ] ; then
    	echo "ERROR: Kernel Release level ${PCRELEASE} ${OSARCH} not supported."
    fi
    echo "ERROR: Package expects '${RELEASE_STRING}' ${PKGARCH}" 
    [ -f "${REDHAT_DIST}" ] && {
	echo "ERROR: Contents of ${REDHAT_DIST} ${OSARCH} on this system"
	while read line
	do
	    echo "ERROR: $line"
	done <${REDHAT_DIST}
    }
    [ -f "${SUSE_DIST}" ] && {
	echo "ERROR: Contents of ${SUSE_DIST} ${OSARCH} on this system"
	while read line
	do
	    echo "ERROR: $line"
	done < ${SUSE_DIST}
    }
    exit 1
fi

(/bin/rpm -q  TDMFIP || /bin/rpm -q Replicator) >/dev/null 2>&1 || {
    TDMFinst=0
    echo ""
    echo "WARNING: IBM Softek TDMF UNIX (IP) is not installed.  Please install"
    echo "WARNING: this product before activating the Offline Migration Package."
    echo ""
} 
/bin/rpm -q sg3_utils >/dev/null 2>&1 || {
    echo ""
    echo "WARNING: The required sg3_utils package is not installed.  Please install"
    echo "WARNING: this package before activating the Offline Migration Package."
    echo ""
}
exit 0

%post
/etc/opt/SFTKomp/bin/omp_setup
exit 0

%preun
# Pre-removal script for linux 

/bin/ls /etc/opt/%OEM%omp/cfg/p[0-9][0-9][0-9].cfg >/dev/null 2>&1
if [ $? -eq 0 ]
then
	echo
	echo "ERROR : A migration is in progress.  Stop your migration and run"
	echo "ERROR : omp_cleanup before trying to uninstall."
	echo
	exit 1
fi

/bin/ls /etc/opt/%OEM%%Q%/p[0-9][0-9][0-9].cfg >/dev/null 2>&1
if [ $? -eq 0 ]
then
	echo
	echo "ERROR : A migration is  in progress.  Stop your migration and run"
	echo "ERROR : omp_cleanup before trying to uninstall."
	echo
	exit 1
fi

if [ -x /etc/opt/SFTKomp/bin/omp_admin ]
then
    /etc/opt/SFTKomp/bin/omp_admin deactivate
fi
if [ -d /etc/opt/SFTKomp/log ]
then
    rm -rf /etc/opt/SFTKomp/log/* 2>/dev/null
fi
if [ -d /etc/opt/SFTKomp/jobs/san ]
then
    rm -rf /etc/opt/SFTKomp/jobs/san/*
fi
if [ -d /etc/opt/SFTKomp/jobs/staging ]
then
    rm -rf /etc/opt/SFTKomp/jobs/staging/*
fi
exit 0

%ifarch == i386 i686 x86_64
%files
%attr(0755,root,root) /etc/profile.d/omp.csh
%attr(0755,root,root) /etc/profile.d/omp.sh
%dir %attr(0755,root,root) /etc/opt
%dir %attr(0755,root,root) /etc/opt/%OEM%omp
%dir %attr(0755,root,root) /etc/opt/%OEM%omp/bin
%attr(0755,root,root) /etc/opt/%OEM%omp/bin/omp_setup
%attr(0755,root,root) /etc/opt/%OEM%omp/bin/omp_cleanup
%attr(0755,root,root) /etc/opt/%OEM%omp/bin/omp_sched
%attr(0755,root,root) /etc/opt/%OEM%omp/bin/omp_getscsidsk
%attr(0755,root,root) /etc/opt/%OEM%omp/bin/omp_selectctlr
%attr(0755,root,root) /etc/opt/%OEM%omp/bin/omp_monitor
%attr(0755,root,root) /etc/opt/%OEM%omp/bin/omp_monitor.awk
%attr(0755,root,root) /etc/opt/%OEM%omp/bin/omp_admin
%attr(0755,root,root) /etc/opt/%OEM%omp/bin/omp_showsched.awk
%attr(0755,root,root) /etc/opt/%OEM%omp/bin/omp_help
%attr(0755,root,root) /etc/opt/%OEM%omp/bin/omp_genconf.awk
%attr(0755,root,root) /etc/opt/%OEM%omp/bin/omp_genconf
%attr(0755,root,root) /etc/opt/%OEM%omp/bin/omp_matchup.awk
%attr(0755,root,root) /etc/opt/%OEM%omp/bin/omp_matchup
%attr(0755,root,root) /etc/opt/%OEM%omp/bin/omp_verifysched
%dir %attr(0755,root,root) /etc/opt/%OEM%omp/cfg
%dir %attr(0755,root,root) /etc/opt/%OEM%omp/docs
%attr(0666,root,root) /etc/opt/%OEM%omp/docs/omp_help.topic
%attr(0666,root,root) /etc/opt/%OEM%omp/docs/omp_help.text
%dir %attr(0755,root,root) /etc/opt/%OEM%omp/done
%dir %attr(0755,root,root) /etc/opt/%OEM%omp/jobs
%dir %attr(0755,root,root) /etc/opt/%OEM%omp/jobs/staging
%dir %attr(0755,root,root) /etc/opt/%OEM%omp/jobs/san
%dir %attr(0777,root,root) /etc/opt/%OEM%omp/log
%dir %attr(0755,root,root) /etc/opt/%OEM%omp/tmp
%dir %attr(0755,root,root) /var/opt
%dir %attr(0755,root,root) /var/opt/%OEM%omp
%endif

%changelog
