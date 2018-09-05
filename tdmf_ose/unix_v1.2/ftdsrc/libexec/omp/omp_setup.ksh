#!/usr/bin/ksh
###############################################################################
#
# LICENSED MATERIALS / PROPERTY OF IBM
#
# Offline Migration Package (OMP)   Version %VERSION%
#
# (c) Copyright %COMPANYNAME%  %COPYRIGHTYEAR 2007%.  All Rights Reserved.
#
# The source code for this program is not published or otherwise
# divested of its trade secrets, irrespective of what has been
# deposited with the U.S. Copyright Office.
#
# US Government Users Restricted Rights - Use, duplication or
# disclosure restricted by GSA ADP Schedule Contract with %COMPANYNAME%
#
# $Id: omp_setup.ksh,v 1.5 2010/01/13 19:37:51 naat0 Exp $
#
# Function: Create directories during Product Install
#
###############################################################################
umask 0

readonly PROGNAME=${0##*/}
readonly EC_TRAP=8
readonly EC_FATAL=4
readonly EC_ERROR=2
readonly EC_WARN=1
readonly EC_OK=0

readonly REPDIR_CFG=/etc/opt/SFTKdtc
readonly REPDIR_BIN=/opt/SFTKdtc/bin
readonly OMPDIR_HOME=/etc/opt/SFTKomp
readonly OMPDIR_VARHOME=/var/opt/SFTKomp
readonly OMPDIR_BIN=${OMPDIR_HOME}/bin
readonly OMPDIR_LOG=${OMPDIR_HOME}/log
readonly OMPDIR_CFG=${OMPDIR_HOME}/cfg
readonly OMPDIR_DOCS=${OMPDIR_HOME}/docs
readonly OMPDIR_DONE=${OMPDIR_HOME}/done
readonly OMPDIR_TEMP=${OMPDIR_HOME}/tmp
readonly OMPDIR_JOBS=${OMPDIR_HOME}/jobs
readonly OMPDIR_PREP=${OMPDIR_JOBS}/staging
readonly OMPDIR_SAN=${OMPDIR_JOBS}/san
readonly RPMDIR_HOME=${PWD}/rpm_staging
readonly RPMDIR_ROOT=${RPMDIR_HOME}/root
readonly RPMDIR_BUILD=${RPMDIR_ROOT}/BUILD

readonly PstoreDeviceFile=pstore_devices

readonly PstoreDevicePath=${OMPDIR_PREP}/${PstoreDeviceFile}

typeset exitcode=${EC_OK}
typeset TDMFinst=1
typeset userid
typeset directory
typeset BABSIZE
typeset rtncode

###############################################################################
#
# display - Displays a message
#
###############################################################################

function display
{
    typeset message
    case $1 in
	NotRootUser)
	     message[0]="FATAL: You must be super user to run this program."
	    ;;
	InstallReplicator) # (no args
	     message[0]="WARNING: %COMPANYNAME2% %PRODUCTNAME% is not installed.  Please install this"
	     message[1]="WARNING: product before activating the Offline Migration Package."
	    ;;
	InstallSg3Utils) # (no args
	     message[0]="WARNING: The required sg3_utils package is not installed.  Please install this"
	     message[1]="WARNING: package before activating the Offline Migration Package."
	    ;;
	MkdirFailed) # number function
	     message[0]="ERROR: Cannot create directory \"$2\"."
	    ;;
	BABmissing) # (no args
	     message[0]="ERROR: BAB size was not specified. Please Re-run with BAB size:"
	     message[1]="ERROR: e.g. ${PROGNAME} bab 32"
	    ;;
	InvalidBAB) # (no args
	     message[0]="ERROR: BAB size parameter is invalid.  Please Re-run with"
	     message[1]="ERROR: a BAB size between 1 and 1547."
	    ;;
	KillmstrFailed) # (no args
	     message[0]="ERROR: killdtcmaster Failed.  BAB size was not set"
	    ;;
	RmmodFailed) # (no args
	     message[0]="ERROR: rmmod sftkdtc Failed.  BAB size was not set"
	    ;;
	DtcinitFailed) # (no args
	     message[0]="ERROR: dtcinfo -b ${2} Failed.  BAB size was not set"
	    ;;
	LaunchmstrFailed) # (no args
	     message[0]="ERROR: launchdtcmaster Failed.  BAB size was not set"
	    ;;
	DtcinfoFailed ) # (no args
	     message[0]="ERROR: dtcinfo -a Failed."
	     message[1]="ERROR: Execute: dtcinfo -a  (to verify BAB size)"
	    ;;
	BABnoTDMF) # (no args
	     message[0]="ERROR: BAB can't be set until you install TDMF"
	    ;;
	TDMFLicense) # (no args
	     message[0]="WARNING: You need a valid license for %PRODUCTNAME% before"
	     message[1]="WARNING: running migrations."
	    ;;
	InvalidArgs) # (no args
	     message[0]="Usage: ${PROGNAME} [bab <size>]"
	    ;;
	*)
	    message[0]="ERROR: Message $1 is not defined."
	    ;;
    esac
    if (( ${#message[*]} > 0 ))
    then
        (( i = 0 ))
        while (( i < ${#message[*]} ))
        do
            print ${message[i]}
            (( i += 1 ))
        done
    fi
}

userid=$(id | cut -d '(' -f 1 | cut -d '=' -f 2)
if [ $? -ne 0 ] || [ ${userid} -ne 0 ]
then
    display "NotRootUser"
    exit $(( EC_ERROR ))
fi

(rpm -q  TDMFIP || rpm -q Replicator) >/dev/null 2>&1 || {
    TDMFinst=0
    display InstallReplicator
    (( exitcode=exitcode < EC_WARN ? EC_WARN : exitcode ))
} 
/bin/rpm -q sg3_utils >/dev/null 2>&1 || {
    display InstallSg3Utils
    (( exitcode=exitcode < EC_WARN ? EC_WARN : exitcode ))
}
[[ "${TDMFinst}" != "0" ]] && ${REPDIR_BIN}/dtclicinfo -q >/dev/null 2>&1 || {
    display TDMFLicense
    (( exitcode=exitcode < EC_WARN ? EC_WARN : exitcode ))
} 

if [[ "${1-}" = "rpm" ]]
then
    rm -rf ${RPMDIR_HOME}

    for directory in ${RPMDIR_HOME} ${RPMDIR_ROOT} ${RPMDIR_BUILD} ${RPMDIR_ROOT}/RPMS ${RPMDIR_ROOT}/SOURCES ${RPMDIR_ROOT}/SPECS ${RPMDIR_ROOT}/SRPMS ${RPMDIR_ROOT}/RPMS/noarch ${RPMDIR_ROOT}/RPMS/athlon ${RPMDIR_ROOT}/RPMS/i386 ${RPMDIR_ROOT}/RPMS/i486  ${RPMDIR_ROOT}/RPMS/i586 ${RPMDIR_ROOT}/RPMS/i686 
    do
	mkdir "${directory}" || {
	    display MkdirFailed "${directory}"
	    (( exitcode=exitcode < EC_ERROR ? EC_ERROR : exitcode ))
	}
    done
    for directory in /etc /etc/opt /etc/profile.d "${OMPDIR_HOME}" "${OMPDIR_BIN}" "${OMPDIR_JOBS}" "${OMPDIR_DOCS}" "${OMPDIR_LOG}" "${OMPDIR_CFG}" "${OMPDIR_DONE}" "${OMPDIR_TEMP}" "${OMPDIR_SAN}" "${OMPDIR_PREP}" /var /var/opt "${OMPDIR_VARHOME}"
    do
	mkdir "${RPMDIR_BUILD}/${directory}" || {
	    display MkdirFailed "${directory}"
	    (( exitcode=exitcode < EC_ERROR ? EC_ERROR : exitcode ))
	}
    done
    cp omp.csh omp.sh ${RPMDIR_BUILD}/etc/profile.d
    cp omp_setup.ksh ${RPMDIR_BUILD}${OMPDIR_BIN}/omp_setup
    cp omp_cleanup.ksh ${RPMDIR_BUILD}${OMPDIR_BIN}/omp_cleanup
    cp omp_sched.ksh ${RPMDIR_BUILD}${OMPDIR_BIN}/omp_sched
    cp omp_getscsidsk.ksh ${RPMDIR_BUILD}${OMPDIR_BIN}/omp_getscsidsk
    cp omp_selectctlr.ksh ${RPMDIR_BUILD}${OMPDIR_BIN}/omp_selectctlr
    cp omp_monitor.ksh ${RPMDIR_BUILD}${OMPDIR_BIN}/omp_monitor
    cp omp_monitor.awk ${RPMDIR_BUILD}${OMPDIR_BIN}/omp_monitor.awk
    cp omp_admin.ksh ${RPMDIR_BUILD}${OMPDIR_BIN}/omp_admin
    cp omp_showsched.awk ${RPMDIR_BUILD}${OMPDIR_BIN}/omp_showsched.awk
    cp omp_help.ksh ${RPMDIR_BUILD}${OMPDIR_BIN}/omp_help
    cp omp_help.topic omp_help.text ${RPMDIR_BUILD}${OMPDIR_DOCS}/
    cp omp_genconf.awk ${RPMDIR_BUILD}${OMPDIR_BIN}/omp_genconf.awk
    cp omp_genconf.ksh ${RPMDIR_BUILD}${OMPDIR_BIN}/omp_genconf
    cp omp_matchup.awk ${RPMDIR_BUILD}${OMPDIR_BIN}/omp_matchup.awk
    cp omp_matchup.ksh ${RPMDIR_BUILD}${OMPDIR_BIN}/omp_matchup
    cp omp_verifysched.ksh ${RPMDIR_BUILD}${OMPDIR_BIN}/omp_verifysched

    rpmbuild -bb --buildroot ${RPMDIR_BUILD} omp.spec

    exit ${EX_OK}

elif [[ "${1-}" = @(pkg|clean|rpm) ]]
then
    rm -rf "${OMPDIR_HOME}" /etc/profile.d/omp.csh /etc/profile.d/omp.sh "${OMPDIR_VARHOME}"

    for directory in /etc/opt "${OMPDIR_HOME}" "${OMPDIR_BIN}" "${OMPDIR_JOBS}" "${OMPDIR_DOCS}" "${OMPDIR_LOG}" "${OMPDIR_CFG}" "${OMPDIR_DONE}" "${OMPDIR_TEMP}" "${OMPDIR_SAN}" "${OMPDIR_PREP}" /etc/opt ${OMPDIR_VARHOME}
    do
	[[ -d ${directory} ]] || {
	    mkdir "${directory}" || {
		display MkdirFailed "${directory}"
		(( exitcode=exitcode < EC_ERROR ? EC_ERROR : exitcode ))
	    }
	}
    done

    chmod -R 755 ${OMPDIR_HOME}
    chmod -R 755 ${OMPDIR_VARHOME}
    chmod 777 ${OMPDIR_LOG}
    cp omp.csh omp.sh /etc/profile.d
    cp omp_setup.ksh ${OMPDIR_BIN}/omp_setup
    cp omp_cleanup.ksh ${OMPDIR_BIN}/omp_cleanup
    cp omp_sched.ksh ${OMPDIR_BIN}/omp_sched
    cp omp_getscsidsk.ksh ${OMPDIR_BIN}/omp_getscsidsk
    cp omp_selectctlr.ksh ${OMPDIR_BIN}/omp_selectctlr
    cp omp_monitor.ksh ${OMPDIR_BIN}/omp_monitor
    cp omp_monitor.awk ${OMPDIR_BIN}/omp_monitor.awk
    cp omp_admin.ksh ${OMPDIR_BIN}/omp_admin
    cp omp_showsched.awk ${OMPDIR_BIN}/omp_showsched.awk
    cp omp_help.ksh ${OMPDIR_BIN}/omp_help
    cp omp_help.topic omp_help.text ${OMPDIR_DOCS}/
    cp omp_genconf.awk ${OMPDIR_BIN}/omp_genconf.awk
    cp omp_genconf.ksh ${OMPDIR_BIN}/omp_genconf
    cp omp_matchup.awk ${OMPDIR_BIN}/omp_matchup.awk
    cp omp_matchup.ksh ${OMPDIR_BIN}/omp_matchup
    cp omp_verifysched.ksh ${OMPDIR_BIN}/omp_verifysched
    chown -R root:root ${OMPDIR_HOME}/*
    chmod 755 ${OMPDIR_BIN}/*
    chmod 666 ${OMPDIR_DOCS}/*
    chown -R root:root /etc/profile.d/omp.*
    chmod 755 /etc/profile.d/omp.*
    [[ "${1-}" = pkg ]] && {
	rm -f SFTKomp.tar
	tar cv -P -f SFTKomp.tar /etc/profile.d/omp.* ${OMPDIR_HOME} 
    }
    exit ${EC_OK}
fi

[[ $# -gt 0 && $1 != bab ]] && {
    display InvalidArgs
    exit ${EX_OK}
}

rm -f ${PstoreDevicePath}
for pathname in $(ls -d1 /sys/block/ram+([0-9]) | sort -k1.15n)
do
    diskname=${pathname##*/}
    read disksize < ${pathname}/size
    (( disksize=disksize / 2048 )) # translate 512 byte blocks to megabytes
    print "${diskname},${disksize}" >>${PstoreDevicePath}
done

[[ -x ${OMPDIR_BIN}/omp_admin && -x ${OMPDIR_BIN}/omp_sched ]] && {
    ${OMPDIR_BIN}/omp_admin activate
}
while [[ "${1-}" = "bab" ]]
do
    if [[ $# != 2 ]]
    then
	display BABmissing 
	(( exitcode=exitcode < EC_ERROR ? EC_ERROR : exitcode ))
	break
    fi
    if [[ ${2-} != +([0-9]) || ${2} -lt 1 || ${2} -gt 1547 ]]
    then
	display InvalidBAB
	(( exitcode=exitcode < EC_ERROR ? EC_ERROR : exitcode ))
	break
    fi

    if [[ "${TDMFinst}" = "0" ]]
    then
	display BABnoTDMF
	(( exitcode=exitcode < EC_ERROR ? EC_ERROR : exitcode ))
	break
    fi
    actual=$(${REPDIR_BIN}/dtcinfo -g 999 2>/dev/null| grep "Actual BAB size" | awk '{ print $7;}') 

    [[ ${actual-} = +([0-9]) && ${actual} -gt 0 && ${actual} -eq $2 ]] && break
    
    ${REPDIR_BIN}/killdtcmaster || {
	display KillmstrFailed
	(( exitcode=exitcode < EC_ERROR ? EC_ERROR : exitcode ))
	break
    }
    lsmod | grep sftkdtc >/dev/null 2>&1 && {
	/sbin/rmmod sftkdtc || {
	    if [[ ! -z "${actual}" ]]
	    then
		    display RmmodFailed
		    (( exitcode=exitcode < EC_ERROR ? EC_ERROR : exitcode ))
		    break
	    fi
	}
    }
    ${REPDIR_BIN}/dtcinit -b ${2} || {
	display DtcinitFailed
	(( exitcode=exitcode < EC_ERROR ? EC_ERROR : exitcode ))
	break
    }
    ${REPDIR_BIN}/launchdtcmaster 2>/dev/null || {
	display LaunchmstrFailed
	(( exitcode=exitcode < EC_ERROR ? EC_ERROR : exitcode ))
	break
    }
    ${REPDIR_BIN}/dtcinfo -g 999 2>/dev/null 
    rtncode="$?"

    if [[ "$rtncode" -ne "0" ]] && [[ "$rtncode" -ne "1" ]]
    then
	display DtcinfoFailed
	(( exitcode=exitcode < EC_WARN ? EC_WARN : exitcode ))
    fi
    break
done
exit $(( exitcode ))
