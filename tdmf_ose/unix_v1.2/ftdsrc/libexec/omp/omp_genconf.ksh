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
# LICENSED MATERIALS / PROPERTY OF IBM
#
# $Id: omp_genconf.ksh,v 1.3 2008/12/28 10:12:16 naat0 Exp $
#
# Function: Frontend to Execute Configuration Validation
#
###############################################################################
set -o nounset
umask 0

PROGNAME=${0##*/}

#
# Return codes
#

readonly EC_TRAP=8
readonly EC_FATAL=4
readonly EC_ERROR=2
readonly EC_WARN=1
readonly EC_OK=0

readonly OMPDIR_HOME="/etc/opt/SFTKomp"
readonly OMPDIR_LOG="${OMPDIR_HOME}/log"
readonly OMPDIR_BIN="${OMPDIR_HOME}/bin"
readonly OMPDIR_JOBS=${OMPDIR_HOME}/jobs
readonly OMPDIR_PREP=${OMPDIR_JOBS}/staging
readonly LogPath="${OMPDIR_LOG}/${PROGNAME}.log"

typeset userid
typeset -l answer

###############################################################################
#
# display - Displays a message
#
###############################################################################

typeset discard_output=""

function no_std_output
{
    if [[ -z "${discard_output}" ]]
    then
	discard_output="no_std"
    else
	discard_output="${discard_output} no_std"
    fi
}

function no_log_output
{
    if [[ -z "${discard_output}" ]]
    then
	discard_output="no_log"
    else
	discard_output="${discard_output} no_log"
    fi
}

function display
{
    typeset message
    typeset -i i
    typeset msgtype func linenum parm

    [[ $# < 3 ]] && {
	set -- TooFewArgs display ${LINENO} "$@"
    }

    msgtype=$1; shift
    func=$1; shift
    linenum=$1; shift

    case ${msgtype} in
	NotRootUser)
	    severity="ERROR"
	     message[0]="You must be super user to run this program."
	    ;;
	InstallWrong)
	    severity="WARNING"
	    message[0]="The file $1 was installed with its file type suffix."
	    ;;
	MissingExec)
	    severity="FATAL"
	    message[0]="Cannot locate the $1 executable."
	    ;;
	NoRestoreFiles)
	    severity="WARNING"
	    message[0]="Scheduing group \"$1\" has no migration configuration files to restore."
	    ;;
	DisableFailed)
	    severity="FATAL"
	    message[0]="Cannot create disable lock file \"$1\"."
	    ;;
	RecoverRepGrp)
	    severity="INFO"
	    message[0]="Recovered configuration file \"$1\" for schedule group \"$2\"."
	    ;;
	FailedRecover)
	    severity="INFO"
	    message[0]="Failed to recover configuration file \"$1\" for schedule group \"$2\"."
	    ;;
	AnswerAlreadySet)
	    severity="ERROR"
	    message[0]="Specify -y or -n parameters but not both."
	    ;;
	BadParam)
	    severity="ERROR"
	     message[0]="The opiton \"-$1\" is not valid parameter."
	    ;;
	MissingParam)
	    severity="ERROR"
	     message[0]="The opiton \"-$1\" needs a parameter."
	    ;;
	Usage)
	    severity="USAGE"
	    no_log_output
	    message[0]="${PROGNAME} [-y] [-v]"
	    message[1]="            -y   start migration now without prompting"
	    message[1]="            -n   do not start a  migration without prompting"
	    message[2]="            -g   Display migration group details"
	    message[3]="            -v   verify only"
	    ;;
	CmdEcho)
	    severity="INFO"
	    for parm in "$@"
	    do
		if [[ -z "${message[0]-}" ]]
		then
		    message[0]="${parm}"
		else
		    message[0]="${message[0]} ${parm}"
		fi
	    done
	    ;;
	TooFewArgs)
	    severity="ERROR"
	    message[0]="Too few arguments passed to function \"${func}\"." 
	    (( i=1 ))
	    for parm in "$@"
	    do
		if (( i == 1 ))
		then
		    message[1]="parm${i}=\"${parm}\""
		else
		    message[1]="${message[1]} parm${i}=\"${parm}\""
		fi

		(( i++ ))
	    done
	    ;;
	*)
	    severity="ERROR"
	    message[0]="Message key \"${msgtype}\" is not defined."
	    msgtype="MissingMsg"
	    (( i=1 ))
	    for parm in "$@"
	    do
		if (( i == 1 ))
		then
		    message[1]="parm${i}=\"${parm}\""
		else
		    message[1]="${message[1]} parm${i}=\"${parm}\""
		fi
		(( i++ ))
	    done
	    ;;
    esac
    if (( ${#message[*]} > 0 ))
    then
        (( i = 0 ))
        while (( i < ${#message[*]} ))
        do
	    [[ "${discard_output}" != *no_std* ]] && {
		if [[ "${severity}" = USAGE ]]
		then
		    print "${message[i]}"
		elif [[ "${severity}" = ?(INFO|WARNING) ]]
		then
		    print "${severity}:" ${message[i]} "[${msgtype}]"
		else
		    print "${severity}:" ${message[i]} "[${msgtype} ${func} ${linenum}]"
		fi
	    }
	    [[ "${discard_output}" != *no_log* && -w ${OMPDIR_LOG} ]] && {
		print $(date '+[%Y.%m.%d--%T]') "${PROGNAME}[$$]" "${severity}:" ${message[i]} "[${msgtype} ${func} ${linenum}]" >>${LogPath}
	    }
            (( i += 1 ))
        done
    fi
    discard_output=""
}
#
################# Define all functions above this line #######################
#

[[ -o xtrace || -n "${OMP_DEBUG-}" ]] && {
    typeset -ft $(typeset +f)
}

userid=$(id | cut -d '(' -f 1 | cut -d '=' -f 2)
if [ $? -ne 0 ] || [ ${userid} -ne 0 ]
then
    display NotRootUser main ${LINENO} 
    exit $(( EC_ERROR ))
fi

answer=
verify=
detail=
while getopts ":ynvg" arg
do
    case ${arg} in
	y)
	    [[ -n "${answer}" ]] && {
		display AnswerAlreadySet main ${LINENO} ${answer}
		exit $(( EC_ERROR ))
	    }
	    answer="yes"
	;;
	n)
	    [[ -n "${answer}" ]] && {
		display AnswerAlreadySet main ${LINENO} ${answer}
		exit $(( EC_ERROR ))
	    }
	    answer="no"
	;;
	v)
	    verify="-v ValidateOnly=true"
	;;

	g)
	    detail="-v DetailDisplay=true"
	;;
	*)
	    if [[ ${arg} = ':' ]]
	    then
		display MissingParam main ${LINENO} ${OPTARG}
	    elif [[ ${arg} != ${OPTARG} ]]
	    then
		display BadParam main ${LINENO} ${OPTARG}
	    fi
	    display Usage main ${LINENO}
	    exit $(( EC_ERROR ))
	;;
    esac
done

[[ -z "${verify-}" ]] && rm -f ${OMPDIR_PREP}/p???.cfg
if [[ -x "./omp_genconf.awk" ]]
then
    ./omp_genconf.awk ${detail-} ${verify-} || exit $?
elif [[ -x "${OMPDIR_BIN}/omp_genconf.awk" ]]
then
    ${OMPDIR_BIN}/omp_genconf.awk ${detail-} ${verify-} || exit $?
else
    display MissingExec main ${LINENO} omp_genconf 
    exit ${EC_FATAL}
fi

[[ -n "${verify-}" ]] && exit ${EC_OK}

until [[ "${answer-}" = @(y|ye|yes) || "${answer-}" = @(n|no) ]] 
do
    read answer?"Do you want to start the migration? (y[es] or n[o])" || { print; exit 1; }
done

if [[ "${answer}" = @(y|ye|yes) ]]
then
    omp_admin start
fi
