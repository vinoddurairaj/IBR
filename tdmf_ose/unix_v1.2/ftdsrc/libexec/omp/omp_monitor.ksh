#!/bin/ksh
########################################################################
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
# $Id: omp_monitor.ksh,v 1.3 2010/01/13 19:37:51 naat0 Exp $
#
# Function: Execute Administrative Commands
#
########################################################################
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

readonly REPDIR_CFG=/etc/opt/SFTKdtc
readonly REPDIR_PRF=/var/opt/SFTKdtc
readonly OMPDIR_HOME=/etc/opt/SFTKomp
readonly OMPDIR_BIN=${OMPDIR_HOME}/bin
readonly OMPDIR_LOG=${OMPDIR_HOME}/log
readonly OMPDIR_CFG=${OMPDIR_HOME}/cfg
readonly OMPDIR_DONE=${OMPDIR_HOME}/done
readonly OMPDIR_JOBS=${OMPDIR_HOME}/jobs
readonly OMPDIR_PREP=${OMPDIR_JOBS}/staging
readonly OMPDIR_SAN=${OMPDIR_JOBS}/san
readonly tab=$(print "\t")

readonly ScheduleGroupFile="schedule_groups"
readonly SchedulerDisableFile="scheduler.disabled"
readonly SchedulerLockFile="scheduler.lock"
readonly LogFile="${PROGNAME}.log"
readonly SchedulerLogFile="omp_sched.log"
readonly AwkMonitorFile="${PROGNAME%%.ksh}.awk"

readonly CfgScheduleGroupPath="${OMPDIR_CFG}/${ScheduleGroupFile}"
readonly PrepScheduleGroupPath="${OMPDIR_PREP}/${ScheduleGroupFile}"
readonly SchedulerDisablePath="${OMPDIR_CFG}/${SchedulerDisableFile}"
readonly SchedulerLockPath="${OMPDIR_CFG}/${SchedulerLockFile}"
readonly LogPath="${OMPDIR_LOG}/${LogFile}"
readonly SchedulerLogPath="${OMPDIR_LOG}/${SchedulerLogFile}"
readonly CrontabPath="/etc/crontab"

readonly MinInterval=10
readonly MaxInterval=600
integer  DefaultInterval=20
integer  Interval=-1

readonly MinLogLines=1
readonly MaxLogLines=200
integer  DefaultLogLines=4
integer  LogLines=-1

integer  DisplayType=0
integer  PreserveStatus=0
integer  PrintSchedulerLog=0
integer  ContinuousPrinting=0

integer  exitcode=${EC_OK}

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
	LogAlreadySet)
	    severity="ERROR"
	     message[0]="The log file parameter is already set."
	    ;;
	BadLogPath)
	    severity="ERROR"
	     message[0]="Cannnot create log file using the log file argument \"$1\"."
	    ;;
	IntervalAlreadySet)
	    severity="ERROR"
	     message[0]="The interval parameter is already set."
	    ;;
	InvalidInterval)
	    severity="ERROR"
	     message[0]="The interval argument \"$1\" contains non-numeric charaters."
	    ;;
	IntervalBounds)
	    severity="ERROR"
	     message[0]="The interval argument \"$1\" is not between ${MinInterval} and ${MaxInterval}."

	    ;;
	LogLinesAlreadySet)
	    severity="ERROR"
	     message[0]="The display scheduler log lines parameter is already set."
	    ;;
	InvalidLogLines)
	    severity="ERROR"
	     message[0]="The display scheduler log lines argument \"$1\" contains non-numeric charaters."
	    ;;
	LogLinesBounds)
	    severity="ERROR"
	     message[0]="The display scheduler log lines argument \"$1\" is not between $2 and $3."

	    ;;
	BadParam)
	    severity="ERROR"
	     message[0]="The opiton \"-$1\" is not valid parameter."
	    ;;
	MissingParam)
	    severity="ERROR"
	     message[0]="The opiton \"-$1\" needs a parameter."
	    ;;
	MissingExec)
	    severity="FATAL"
	    message[0]="Cannot locate the $1 executable."
	    ;;
	NotInCrontab)
	    severity="WARNING"
	    message[0]="The OMP Scheduler is not available for cron to execute."
	    ;;
	Usage)
	    severity="USAGE"
	    message[0]="${PROGNAME} [-s] [-g] [-d] [-p] [-c <lines>] [-l <logfile>] [-i 0|<interval>]"
	    message[1]="          -s            display schedule groups summary report" 
	    message[2]="          -g            display migration groups report" 
	    message[3]="          -d            display migration groups report with device names" 
	    message[4]="          -p            display the last few lines of the scheduler log file" 
	    message[5]="          -u            do not clear the screen between display intervals" 
	    message[6]="          -c lines      sets the number of lines to display from the scheduler log"
	    message[7]="          -l logfile    output to a file instead of the screen"
	    message[8]="          -i 0|interval redisplay status at the given interval"
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

function cleanup
{
    typeset pid
    trap - EXIT HUP INT QUIT USR1 USR2 TERM
    if (( PreserveStatus == 0 ))
    then
	[[ -n "${OMPDIR_TMP-}" && -d ${OMPDIR_TMP} ]] && rm -rf ${OMPDIR_TMP}
    fi

    (( $1 == EC_TRAP )) && exit $1
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
    exit $(( exitcode=EC_ERROR ))
fi

trap "trap '' EXIT HUP INT QUIT USR1 USR2 TERM; cleanup \${exitcode}" EXIT
trap "trap '' EXIT HUP INT QUIT USR1 USR2 TERM; cleanup ${EC_TRAP}" INT HUP QUIT TERM USR1 USR2

# Make a temporary directory for our temporary files

OMPDIR_TMP=$(mktemp -dq -p /usr/tmp omp_XXXXXXXXXX)  || {
    display TempDirFailure main ${LINENO} /usr/tmp
    exit $(( exitcode=EC_ERROR ))
}

if [[ -x "./${AwkMonitorFile}" ]]
then
    StatusDisplayPath="./${AwkMonitorFile}"
elif [[ -x "${OMPDIR_BIN}/${AwkMonitorFile}" ]]
then
    StatusDisplayPath="${OMPDIR_BIN}/${AwkMonitorFile}"
else
    display MissingExec main ${LINENO} ${AwkMonitorFile}
    exit $(( exitcode=EC_FATAL ))
fi

StatusLogPath=

while getopts :sgdzpuc:l:i: arg
do
    case ${arg} in
	l)		# log <logfile>
	    if [[ -n "${StatusLogPath}" ]]
	    then
		display LogAlreadySet main ${LINENO} ${OPTARG}
		exit $(( exitcode=EC_ERROR ))
	    fi

	    StatusLogPath=${OPTARG}
	    if [[ -a "${StatusLogPath}" &&  ! -f "${StatusLogPath}" ]]
	    then
		display BadLogPath main ${LINENO}  ${StatusLogPath}
		exit $(( exitcode=EC_ERROR ))

	    elif ! touch  ${StatusLogPath} # >/dev/null 2>&1
	    then
		display BadLogPath main ${LINENO} ${StatusLogPath}
		exit $(( exitcode=EC_ERROR ))
	    fi
	;;
	i)		# interval <seconds> 
	    if (( Interval > 0 ))
	    then
		display IntervalAlreadySet main ${LINENO} ${OPTARG}
		exit $(( exitcode=EC_ERROR ))
	    fi
	    if [[ ${OPTARG} != +([0-9]) ]]
	    then
		display InvalidInterval main ${LINENO} ${OPTARG}
		exit $(( exitcode=EC_ERROR ))
	    fi
	    (( Interval=${OPTARG} ))
	    if (( Interval != 0 && (Interval < MinInterval || Interval > MaxInterval) ))
	    then
		display IntervalBounds main ${LINENO} ${OPTARG} ${MinInterval} ${MaxInterval}
		exit $(( exitcode=EC_ERROR ))
	    fi
	;;
	c)		# display log lines <lines>
	    if (( LogLines > 0 ))
	    then
		display LogLinesAlreadySet main ${LINENO} ${OPTARG}
		exit $(( exitcode=EC_ERROR ))
	    fi
	    if [[ ${OPTARG} != +([0-9]) ]]
	    then
		display InvalidLogLines main ${LINENO} ${OPTARG}
		exit $(( exitcode=EC_ERROR ))
	    fi
	    (( LogLines=${OPTARG} ))
	    if (( LogLines < MinLogLines || LogLines > MaxLogLines ))
	    then
		display LogLinesBounds main ${LINENO} ${OPTARG} ${MinLogLines} ${MaxLogLines}
		exit $(( exitcode=EC_ERROR ))
	    fi
	    (( PrintSchedulerLog=1 ))
	;;
	s)		# schedule group report
	    (( DisplayType |= 1 ))
	;;
	g)		# migration group report
	    (( DisplayType |= 2 ))
	;;
	d)		# migration group with LUNs report
	    (( DisplayType |= 4 ))
	;;
	p)		# print scheduler log
	    (( PrintSchedulerLog=1 ))
	;;
	u)		# print scheduler log
	    (( ContinuousPrinting=1 ))
	;;
	z)		# debug flag preserve the status data
	    (( PreserveStatus=1 ))
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
	    exit $(( exitcode=EC_ERROR ))
	;;
    esac
done

(( Interval < 0 )) && (( Interval=DefaultInterval ))
(( LogLines < 0 )) && (( LogLines=DefaultLogLines ))

(( idx=0 ))
(( DisplayType == 0 )) && (( DisplayType=1 ))

[[ -n "${StatusLogPath}" ]] && {
    exec 1>${StatusLogPath}
    exec 2>&1
}

while true
do
    (( idx++ ))
    eval $(date "+SH=%H SM=%M SS=%S")
    if (( PreserveStatus != 0 ))
    then
	StatusPath="${OMPDIR_TMP}/status${idx}"
    else
	StatusPath="${OMPDIR_TMP}/status"
    fi

    print "Wait:QUEUE" > ${StatusPath}
    (cd ${OMPDIR_CFG}; egrep -H "NOTES:|REMARK:|DTC-DEVICE" p???.cfg 2>/dev/null) >> ${StatusPath}
    print "Active:QUEUE" >> ${StatusPath}
    (cd ${REPDIR_CFG}; 
	for pfile in p???.cfg
	do
	    [[ ${pfile} = "p???.cfg" ]] && break
	    egrep -H "NOTES:|REMARK:|DTC-DEVICE" ${pfile} 2>/dev/null
	    [[ -f "${REPDIR_PRF}/${pfile%%.cfg}.prf" ]] && {
		print "$pfile:PERFORMANCE:"$(tail -1 ${REPDIR_PRF}/${pfile%%.cfg}.prf) 
	    }
	done) >> ${StatusPath}
    print "Done:QUEUE" >> ${StatusPath}
    (cd ${OMPDIR_DONE}; egrep -H "NOTES:|REMARK:|DTC-DEVICE" p???.done 2>/dev/null) >> ${StatusPath}
    print "Error:QUEUE" >> ${StatusPath}
    (cd ${OMPDIR_DONE}; egrep -H "NOTES:|REMARK:|DTC-DEVICE" p???.error 2>/dev/null) >> ${StatusPath}

    [[ ${ContinuousPrinting} -eq 0 && -t 1 ]] && clear
    (( idx == 1 )) && {
	grep omp_sched ${CrontabPath} >/dev/null 2>&1 || {
	    display NotInCrontab main ${LINENO}
	}
    }
    [[ ${PrintSchedulerLog} -gt 0 && -s ${SchedulerLogPath} ]] && {
	tail -${LogLines} ${SchedulerLogPath} | awk '{ $2=" "; sub(/  /,""); print $0}' 
	print ""
    }
    ${StatusDisplayPath} -v DisplayType=${DisplayType} ${StatusPath}
    #
    # Calculate remaining interval time
    #

    (( Interval == 0 )) && break

    eval $(date "+EH=%H EM=%M ES=%S")
    delay=$(bc <<-EOF
	s=(((${SH} * 60) + ${SM}) * 60) + ${SS}
	e=(((${EH} * 60) + ${EM}) * 60) + ${ES}

	if (e < s) e = 86400 + e
	i=${Interval} - (e - s) % ${Interval}
	if (i < 3 && e - s <= ${Interval}) i+=${Interval}
	i
	EOF
	)
    sleep ${delay}
    [[ ! -t 1 ]] && print ""
done
exit $(( exitcode=EC_OK ))
