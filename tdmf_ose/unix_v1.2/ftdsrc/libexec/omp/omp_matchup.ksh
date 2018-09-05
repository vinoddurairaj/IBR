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
# $Id: omp_matchup.ksh,v 1.2 2008/12/28 10:12:16 naat0 Exp $
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
readonly LogPath="${OMPDIR_LOG}/${PROGNAME}.log"

typeset userid

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
	MissingExec)
	    severity="FATAL"
	    message[0]="Cannot locate the $1 executable."
	    ;;
	Usage)
	    severity="USAGE"
	    no_log_output
	    message[0]="${PROGNAME} - Creates source and target csv files using the matchup csv file."
	    ;;
	InvalidArgument) # number value
	    severity="ERROR"
	    message[0]="Invalid option $1 specified."
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
		    print "${severity}:" "${message[i]}" "[${msgtype}]"
		else
		    print "${severity}:" "${message[i]}" "[${msgtype} ${func} ${linenum}]"
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

if (( $# > 0 ))
then
    if [[ ${1-} = @(-\?|-h) ]]
    then
	display Usage main ${LINENO}
	exit ${EC_OK}
    fi
    display InvalidArgument main ${LINENO} $1 
    exit ${EC_ERROR}
fi

[[ -x "./omp_matchup.awk" ]] && {
    ./omp_matchup.awk
    exit $?
}
[[ -x "${OMPDIR_BIN}/omp_matchup.awk" ]] && {
    ${OMPDIR_BIN}/omp_matchup.awk
    exit $?
}

display MissingExec main ${LINENO} omp_matchup 
exit ${EC_FATAL}
