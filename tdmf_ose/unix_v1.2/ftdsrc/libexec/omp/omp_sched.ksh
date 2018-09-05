#!/usr/bin/ksh
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
# $Id: omp_sched.ksh,v 1.4 2010/01/13 19:37:51 naat0 Exp $
#
# Function: Schedule and Execute Migrations
#
########################################################################
set -o nounset
umask 0
#
# 
# Script designed to be run by cron or manually
#

PROGNAME=${0##*/}

#
# Return codes
#

readonly EC_TRAP=8
readonly EC_FATAL=4
readonly EC_ERROR=2
readonly EC_WARN=1
readonly EC_OK=0

readonly REPDIR_CFG="/etc/opt/SFTKdtc"
readonly OMPDIR_HOME="/etc/opt/SFTKomp"
readonly OMPDIR_LOG="${OMPDIR_HOME}/log"
readonly OMPDIR_CFG="${OMPDIR_HOME}/cfg"
readonly OMPDIR_DONE="${OMPDIR_HOME}/done"
typeset  OMPDIR_TMP

readonly TunablesFile="omp_tunables"
readonly ScheduleGroupFile="schedule_groups"
readonly SchedulerDisableFile="scheduler.disabled"
readonly SchedulerLockFile="scheduler.lock"
readonly LogFile="${PROGNAME}.log"

readonly TunablesPath="${OMPDIR_HOME}/${TunablesFile}"
readonly ScheduleGroupPath="${OMPDIR_CFG}/${ScheduleGroupFile}"
readonly SchedulerDisablePath="${OMPDIR_CFG}/${SchedulerDisableFile}"
readonly SchedulerLockPath="${OMPDIR_CFG}/${SchedulerLockFile}"
readonly LogPath="${OMPDIR_LOG}/${LogFile}"
readonly tab=$(print \\t)

# 
# Create rooted aliases for all external executables
#

alias dtcinfo=/opt/SFTKdtc/bin/dtcinfo
alias dtcinit=/opt/SFTKdtc/bin/dtcinit
alias dtckillpmd=/opt/SFTKdtc/bin/dtckillpmd
alias dtckillrefresh=/opt/SFTKdtc/bin/dtckillrefresh
alias dtcset=/opt/SFTKdtc/bin/dtcset
alias dtcrefresh=/opt/SFTKdtc/bin/dtcrefresh
alias dtcstart=/opt/SFTKdtc/bin/dtcstart
alias dtcstop=/opt/SFTKdtc/bin/dtcstop
alias rm=/bin/rm
alias mv=/bin/mv
alias cp=/bin/cp
alias grep=/bin/grep
alias gawk=/bin/gawk
alias date=/bin/date
alias mktemp=/bin/mktemp

PATH=/bin:/usr/bin

set -A GroupSched
set -A GroupNum
set -A GroupState
set -A GroupPstore
set -A GroupDevices

typeset MaxRepGroups=512
typeset MaxRepDevices=130
typeset MaxRepGroupsActive=32
typeset DelayAfterNumStops=0
typeset SleepAfterNumStops=4
typeset DelayAfterNumStarts=0
typeset SleepAfterNumStarts=4

typeset ScheduleGroupList
typeset ScheduleGroup
typeset RepGrpNum
typeset RepSchedGrp
typeset RepPstore
typeset RepDevices
typeset ActivePstores
typeset -u RepState

typeset -i RepGroupsActive=0
typeset -i RepDevicesActive=0
typeset -i RepGroupsStopped=0
typeset -i PrevRepGroupsWaiting=0
typeset -i RepGroupsWaiting=0
typeset -i RepDevicesWaiting=0
typeset -i DisplayStatus=0
typeset -i PreserveStatus=0
typeset -i exitcode=${EC_OK}

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
    typeset severity

    [[ $# < 3 ]] && {
	set -- TooFewArgs display $LINENO "${@-}"
    }

    msgtype=$1; shift
    func=$1; shift
    linenum=$1; shift

    case "${msgtype}" in
	NotRootUser)
	    severity="FATAL"
	    message[0]="You must be super user to run this program."
	    ;;
	MissingParameter) # number
	    severity="FATAL"
	    message[0]="Function called without parameter $1. "
	    ;;
	MissingConfiguration) # number
	    severity="ERROR"
	    message[0]="Missing configuration file \"$1\"."
	    ;;
	InvalidParameter) # number value
	    severity="FATAL"
	    message[0]="Invalid parameter $1 with value \"$2\"."
	    ;;
	BadParam)
	    severity="ERROR"
	     message[0]="The opiton \"-$1\" is not valid parameter."
	    ;;
	MissingParam)
	    severity="ERROR"
	     message[0]="The opiton \"-$1\" needs a parameter."
	    ;;
	UnexpectedState) # grpnum state
	    severity="FATAL"
	    message[0]="Unexpected State \"$2 \" for migration group $1 in schedule group \"$3\"."
	    ;;
	MissingPstore) # pstore
	    severity="FATAL"
	    message[0]="Missing persistent store device in file \"$1\"."
	    ;;
	PstoreFailure) # pstore function
	    severity="FATAL"
	    message[0]="Persistent store device \"$1\" failed to initialized."
	    ;;
	TempDirFailure)
	    severity="FATAL"
	    message[0]="Creating temporary directory in \"$1\" failed."
	    ;;
	TempFileFailure)
	    severity="FATAL"
	    message[0]="Creating temporary file \"$1\" failed."
	    ;;
	MoveFailure)
	    severity="FATAL"
	    message[0]="Moving file \"$1\" to file \"$2\" in directory \"$3\" failed."
	    ;;
	CopyFailure)
	    severity="FATAL"
	    message[0]="Copying file \"$1\" to file \"$2\" in directory \"$3\" failed."
	    ;;
	InvalidDevice)
	    severity="FATAL"
	    message[0]="Expected a block or character device for device \"$1\"."
	    ;;
	CmdOutput)
	    severity="INFO"
	    message[0]="${1-}"
	    ;;
	InitializedPstore)
	    severity="INFO"
	    message[0]="Persistent store device \"$1\" initialized."
	    ;;
	NoStopGroup)
	    severity="WARNING"
	    message[0]="Stopping group \"$1\", but group was not started."
	    ;;
	StopFailed)
	    severity="ERROR"
	    message[0]="Unable to stop migration group $1.  Manual intervention"
	    message[1]="is required if the problem persists."
	    ;;
	MissingScheduleActive) # pstore
	    severity="FATAL"
	    message[0]="No schedule groups file found but there are active migrations."
	    ;;
	MissingSchedule) # pstore
	    severity="ERROR"
	    message[0]="No scheduling groups found in file \"$1\"."
	    ;;
	MigrationDone)
	    severity="INFO"
	    message[0]="The scheduled migration task has completed."
	    ;;
	MigGroupActive)
	    severity="ERROR"
	    message[0]="Cannot remove migration group $1 because it is still active."
	    message[1]="Manual intervention will be required if the problem persists."
	    ;;
	MigGroupDone)
	    severity="INFO"
	    message[0]="Migration group $1 for schedule group \"$2\" finished."
	    ;;
	MigGroupStarted)
	    severity="INFO"
	    message[0]="Migration group $1 for schedule group \"$2\" started."
	    ;;
	SchedulerDisabled)
	    severity="INFO"
	    message[0]="The OMP Scheduler has been disabled."
	    ;;
	SchedulerRunning) # pid
	    severity="WARNING"
	    message[0]="Another OMP Scheduler with pid $1 is already running."
	    ;;
	SchedulerExit) # number
	    if [[ -n "$1" && "$1" -eq 0 ]]
	    then
		severity="INFO"
	    else
		severity="WARNING"
	    fi
	    message[0]="The OMP Scheduler exiting with status $1."
	    ;;
	TunableSyntax) # path
	    severity="WARNING"
	    message[0]="Errors detected while processing \"$1\" file."
	    ;;
	Usage)
	    severity="USAGE"
	    no_log_output
	    message[0]="${PROGNAME}"
	    ;;
	CmdEcho)
	    severity="INFO"
	    [[ $# -gt 0 ]] && {
		for parm in "$@"
		do
		    if [[ -z "${message[0]-}" ]]
		    then
			message[0]="${parm}"
		    else
			message[0]="${message[0]} ${parm}"
		    fi
		done
	    }
	    ;;
	TooFewArgs)
	    severity="ERROR"
	    message[0]="Too few arguments passed to function \"${func}\"." 
	    (( i=1 ))
	    [[ $# -gt 0 ]] && {
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
	    }
	    ;;
	*)
	    severity="ERROR"
	    message[0]="Message key \"${msgtype}\" is not defined."
	    msgtype="MissingMsg"
	    (( i=1 ))
	    [[ $# -gt 0 ]] && {
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
	    }
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
		if [[ -w ${LogPath%/*} ]]
		then
		    print $(date '+[%Y.%m.%d--%T]') "${PROGNAME}[$$]" "${severity}:" ${message[i]} "[${msgtype} ${func} ${linenum}]" >>${LogPath}
		else
		    print log file write error
		fi
	    }
            (( i += 1 ))
        done
    fi
    discard_output=""
}

function logit
{
    typeset line
    typeset OMPRC
    while read line
    do
	[[ "${line}" = OMPRC=* ]] && {
	    break
	}
	display CmdOutput logit ${LINENO} "${line}" 
    done
    eval $line
    return $OMPRC
}

#
# 
#

function scheduler_lock
{
    typeset mylockfile=${OMPDIR_CFG}/scheduler.$$
    typeset ret
    typeset pid

    # Make my lock file 

    print $$ > ${mylockfile} 2>/dev/null || {
	return 0
    }

    # Attempt to link my lock file to the scheduler lock file

    ln ${mylockfile} ${SchedulerLockPath} >/dev/null 2>&1 && {
	rm -f ${mylockfile}
	return 1
    }

    # Check to see if the lock file is left over

    read pid < ${SchedulerLockPath}
    [[ $? -ne 0 || -z "${pid-}" ]] && {
	rm -f ${mylockfile}
	return 0
    }

    kill -0 ${pid} >/dev/null 2>&1 && {
	rm -f ${mylockfile}
	return 0
    }

    # lock is stale so clean up and try again

    rm -f ${SchedulerLockPath}
    ln ${mylockfile} ${SchedulerLockPath} >/dev/null 2>&1
    (( ret=$?==0 ))
    rm -f ${mylockfile}
    return ${ret}
}

function cleanup
{
    typeset pid
    trap - EXIT HUP INT QUIT USR1 USR2 TERM
    (( DisplayStatus > 0 )) && display SchedulerExit cleanup ${LINENO} $1
    if (( PreserveStatus == 0 ))
    then
	[[ -n "${OMPDIR_TMP-}" && -d ${OMPDIR_TMP} ]] && rm -rf ${OMPDIR_TMP}
    fi
    [[ -s ${SchedulerLockPath} ]] && {
	read pid < ${SchedulerLockPath} >/dev/null 2>&1
	(( "${pid}" == $$ )) && rm -f ${SchedulerLockPath}
    }

    (( $1 == EC_TRAP )) && exit $1
}

# Initialize pstore if necessary
# Input: migration group number, [pstore]

function init_pstore
{
    [[ -z "${1-}" ]] && {
	display  MissingParameter init_pstore ${LINENO} 1
	return 1
    }

    [[ "$1" != @([0-9]|[0-9][0-9]|[0-9][0-9][0-9]) ]] && {
	display  InvalidParameter init_pstore ${LINENO} 1 "$1" 
	return 1
    }

    typeset -Z3 grpnum="$1"
    typeset repgrp="${REPDIR_CFG}/p${grpnum}.cfg"
    typeset pstore rest1 rest2

    [[ ! ( -f "${repgrp}" && -s "${repgrp}" ) ]] && {
	display  MissingConfiguration init_pstore ${LINENO} "${repgrp}"
	return 1
    }

    # Second parameter is optional

    (( $# == 1 )) && {
	set -- $(grep  "PSTORE:" ${repgrp})
    }
    pstore="$2"

    # Note: Do not use function parameters beyond this point

    [[ -z "${pstore}" || $# != 2 ]] && {
	display  MissingPstore init_pstore ${LINENO} "${repgrp}"
	return 1
    }
    [[ ! ( -b "${pstore}" || -c "${pstore}"  ) ]] && {
	display  InvalidDevice init_pstore ${LINENO} "${pstore}"
	return 1
    }
    no_std_output
    display CmdEcho init_pstore ${LINENO} dtcinit -p ${pstore}
    { dtcinit -p "${pstore}" 2>&1; print OMPRC=$?; } | logit || {
	display PstoreFailure init_pstore ${LINENO} "${pstore}"
	return 1
    }
    display InitializedPstore init_pstore ${LINENO} "${pstore}"
    return 0
}

#
# Start a migration group
#
# Input: migration group number
# Returns: status of dtcstart command
#

function start_group
{
    [[ -z "${1-}" ]] && {
	display  MissingParameter start_group ${LINENO} 1
	return 1
    }

    [[ "$1" != @([0-9]|[0-9][0-9]|[0-9][0-9][0-9]) ]] && {
	display  InvalidParameter 1 start_group ${LINENO} "$1"
	return 1
    }

    no_std_output
    display CmdEcho start_group ${LINENO} dtcstart -g "$1"
    { dtcstart -g "$1" 2>&1; print OMPRC=$?; } | logit
}

#
# Starts a full refresh or restartable full refresh based upon state
#
# Input: migration group number, state
# Returns: status of dtcrefresh command
#

function start_refresh
{
    typeset option
    [[ -z "${1-}" ]] && {
	display  MissingParameter start_refresh ${LINENO} 1
	return 1
    }

    [[ "$1" != @([0-9]|[0-9][0-9]|[0-9][0-9][0-9]) ]] && {
	display  InvalidParameter 1 start_refresh ${LINENO} "$1"
	return 1
    }

    option=
    case "${2-}" in
	PASSTHRU)
	    option="-f"
	    ;;
	TRACKING)
	    option="-r"
	    ;;
	*)
	    display UnexpectedState start_refresh ${LINENO} "$1" "$2" ${GroupSched[$1]}
	    return 1
	    ;;
    esac

    no_std_output
    display CmdEcho start_refresh ${LINENO} dtcset -g "$1" LRT=off JOURNAL=off
    { dtcset -g "$1" LRT=off JOURNAL=off 2>&1; print OMPRC=$?; } | logit

    no_std_output
    display CmdEcho start_refresh ${LINENO} dtcrefresh ${option} -g "$1"
    { dtcrefresh ${option} -g "$1" 2>&1; print OMPRC=$?; } | logit
}

#
# Stops a refresh operation from running
#
# Input: migration group number
# Returns: status of dtckillrefresh command
#

function stop_refresh 
{
    [[ -z "${1-}" ]] && {
	display  MissingParameter stop_refresh ${LINENO} 1
	return 1
    }

    [[ "$1" != @([0-9]|[0-9][0-9]|[0-9][0-9][0-9]) ]] && {
	display  InvalidParameter stop_refresh ${LINENO} 1 "$1"
	return 1
    }

    no_std_output
    display CmdEcho start_group ${LINENO} dtckillrefresh -g "$1"
    { dtckillrefresh -g "$1" 2>&1; print OMPRC=$?; } | logit
}

#
# Stops a replication from running
#
# Input: migration group number
# Returns: status of dtcstop command
#

function stop_group 
{
    [[ -z "${1-}" ]] && {
	display  MissingParameter stop_group ${LINENO} 1
	return 1
    }

    [[ "$1" != @([0-9]|[0-9][0-9]|[0-9][0-9][0-9]) ]] && {
	display  InvalidParameter stop_group ${LINENO} 1 "$1"
	return 1
    }

    # Ignore errors from killpmd but return dtcstop execution status

    no_std_output
    display CmdEcho stop_group ${LINENO} dtckillpmd -g "$1"
    { dtckillpmd -g "$1" 2>&1; print OMPRC=$?; } | logit

    typeset -Z3 grpnum="$1"
    typeset ccfgfile="p${grpnum}.cur"

    if [[ -f "${REPDIR_CFG}/${ccfgfile}" ]]
    then
	no_std_output
	display CmdEcho stop_group ${LINENO} dtcstop -g "$1"
	{ dtcstop -g "$1" 2>&1; print OMPRC=$?; } | logit
    else
	display NoStopGroup stop_group ${LINENO} "$1"
	true;
    fi
}

#

# move configuration files between directories
#
# Input: migration group number, status
#
# Status	Description
# done		Configuration files are moved to the done directory
#		with the "done" suffix
# error		Configuration files are moved to the done directory
#		with the "error" suffix
# activate	Configuration files are moved to the TDMF UNIX (IP) directory
#
# Returns: mv status

function move_migration_group
{
    [[ -z "${1-}" ]] && {
	display  MissingParameter move_migration_group ${LINENO} 1
	return 1
    }

    [[ "$1" != @([0-9]|[0-9][0-9]|[0-9][0-9][0-9]) ]] && {
	display  InvalidParameter move_migration_group ${LINENO} 1 "$1"
	return 1
    }

    [[ -z "${2-}" ]] && {
	display  MissingParameter move_migration_group ${LINENO} 2
	return 1
    }
    [[ "$2" != @(done|error|activate) ]] && {
	display  InvalidParameter move_migration_group ${LINENO} 2 "$2"
	return 1
    }

    typeset -Z3 grpnum="$1"
    typeset pcfgfile="p${grpnum}.cfg"
    typeset scfgfile="s${grpnum}.cfg"
    typeset dcfgfile="p${grpnum}.$2"
    typeset ccfgfile="p${grpnum}.cur"

    [[ "$2" != @(done|error) && -f "${REPDIR_CFG}/${ccfgfile}" ]] && {
	display  MigGroupActive move_migration_group ${LINENO} 2 "$1"
	return 1
    }


    rm -f ${REPDIR_CFG}/${scfgfile}
    case $2 in
	done|error)
	    dcfgfile="p${grpnum}.$2"
	    mv ${REPDIR_CFG}/${pcfgfile} ${OMPDIR_DONE}/${dcfgfile} || {
		display  MoveFailure move_migration_group ${LINENO} 2 ${pcfgfile} ${dcfgfile} ${OMPDIR_DONE}
		false
	    }
	    ;;
	activate)
	    cp ${OMPDIR_CFG}/${pcfgfile} ${REPDIR_CFG}/${scfgfile} || {
		display  CopyFailure move_migration_group ${LINENO} 2 ${pcfgfile} ${scfgfile} ${REPDIR_CFG}
		false
	    }
	    mv ${OMPDIR_CFG}/${pcfgfile} ${REPDIR_CFG}/${pcfgfile} || {
		display  MoveFailure move_migration_group ${LINENO} 2 ${pcfgfile} ${dcfgfile} ${REPDIR_CFG}
		false
	    }
	    ;;
    esac
}

#
# Collect potential migration groups that could be started 
# from the TDMF UNIX (IP) configuration files
#

function collect_waiting_groups
{
    typeset grpnum pstore schedgrplist schedgrp cfgfile
    typeset field1 field2 field3
    typeset -i repdevices
    typeset cfgfilepattern="${OMPDIR_CFG}/p[0-9][0-9][0-9].cfg"

    schedgrplist=' '"$*"' '
    for cfgfile in ${cfgfilepattern}
    do
	[[ "${cfgfilepattern}" = "${cfgfile}" ]] && break

	grpnum=""
	schedgrp=""
	pstore=""
	(( repdevices=0 ))

	while read field1 field2 field3
	do
	    [[ ${field1} = "NOTES:" ]] && {
		[[ "${schedgrplist}" != *\ ${field2}\ * ]] && {
		    continue 2
		}
		schedgrp=${field2}
		grpnum=${cfgfile##*/p}
		grpnum=${grpnum%%.cfg}
		pstore=""
		(( repdevices=0 ))
	    }
	    [[ ${field1} = "PSTORE:" ]] && {
		pstore=${field2}
	    }
	    [[ ${field1} = "DTC-DEVICE:" ]] && {
		(( repdevices++ ))
	    }
	done < ${cfgfile}
	print -u3 ${grpnum} ${schedgrp} "WAITING" ${pstore} ${repdevices}
    done
}

#
# Collect information about the active migration groups
#

function collect_active_groups
{
    typeset option state pstore
    typeset -Z3 grpnum
    typeset field1 field2 field3 field4 rest
    typeset -i devicecnt

    if (( $# == 0 ))
    then
	option="-a"
    elif [[ "$1" = @([0-9]|[0-9][0-9]|[0-9][0-9][0-9]) ]]
    then
	option="-g $1"
    else
	display  InvalidParameter collect_active_groups ${LINENO} 1 "$1"
	    return 1
    fi

    dtcinfo ${option} 2>/dev/null >${DtcInfoPath}
    while read field1 field2 field3 field4 rest
    do

	[[ ( "${field1}" = "Replication" || "${field1}" = "Migration" ) && "${field2}" = "Group" ]] && {
	    [[ -n ${grpnum-} ]] && {
		print -u3 ${grpnum} ${schedgrp} ${state} ${pstore} ${devicecnt}
	    }
	    grpnum="${field3}"
	    schedgrp=$(gawk '/NOTES:/{print $2; exit(0);}' ${REPDIR_CFG}/p${grpnum}.cfg)
	    pstore=""
	    state=""
	    (( devicecnt=0 ))
	    continue
	}
	[[ "${field1}" = "Device" ]] && {
	    (( devicecnt++ ))
	    continue
	}
	field2=${field2%%.*}
	[[ "${field1}" = "Persistent" && "${field2}" = "Store" ]] && {
	    pstore="${field3}"
	    continue
	}
	field3=${field3%%.*}
	[[ "${field1}" = "Mode" && "${field3}" = "operations" ]] && {
	    state="${field4}"
	    continue
	}
    done <${DtcInfoPath}
    [[ -n ${grpnum-} ]] && {
	print -u3 ${grpnum} ${schedgrp} ${state} ${pstore} ${devicecnt}
    }
}

#
# Return the length of dynamically built variables
#

function has_replication_groups
{
    eval \[\[ -n \${${1}-} \]\]  
}

#
# Removes the first value from a dynamic variable and returns its vailue
# Input: dynamic variable name
# Returns: 1) The variable RepGrpNum contains the extracted value  
#          2) The dynamic variable is rebuilt
#

function extract_replication_group
{
    typeset var=$1
    set -- $(eval print \$${var})
    RepGrpNum=$1
    shift
    if (( $# <= 0 ))
    then
	eval $var=\"\"
    else
	eval $var=\""$@"\"
    fi
}

function unique_list
{
    typeset value list
    [[ $# -gt 0 ]] && {
	for value in $*
	do
	    [[ " ${list-} " != *\ ${value}\ * ]] && {
		list="${list-} ${value}"
	    }
	done
    }
    print ${list-}
}

#
################# Define all functions above this line #######################
#

if [[ -r  ${TunablesPath} ]]
then
    if (set -e; . ${TunablesPath}) >/dev/null 2>&1
    then
	. ${TunablesPath}
    else
	Display TunableSyntax main ${LINENO} ${TunablesPath}
    fi
    # TODO Validate value
    [[ -n "${DebugSched:-}" ]] && OMP_DEBUG=${DebugSched}
    [[ -n "${MaxMigrationGroups:-}" ]] && MaxRepGroups=${MaxMigrationGroups}
    [[ -n "${MaxMigrationDevices:-}" ]] && MaxRepDevices=${MaxMigrationDevices}
    [[ -n "${MaxActiveMigrationGroups:-}" ]] && MaxRepGroupsActive=${MaxActiveMigrationGroups}
    [[ -n "${SchedDelayAfterNumStarts:-}" ]] && DelayAfterNumStarts=${SchedDelayAfterNumStarts}
    [[ -n "${SchedSleepAfterNumStarts:-}" ]] && SleepAfterNumStarts=${SchedSleepAfterNumStarts}
    [[ -n "${SchedDelayAfterNumStops:-}" ]] && DelayAfterNumStops=${SchedDelayAfterNumStops}
    [[ -n "${SchedSleepAfterNumStops:-}" ]] && SleepAfterNumStops=${SchedSleepAfterNumStops}
fi    

[[ -o xtrace || -n "${OMP_DEBUG-}" ]] && {
    typeset -ft $(typeset +f)
}

trap "trap '' EXIT HUP INT QUIT USR1 USR2 TERM; cleanup \${exitcode}" EXIT
trap "trap '' EXIT HUP INT QUIT USR1 USR2 TERM; cleanup ${EC_TRAP}" INT HUP QUIT TERM USR1 USR2

while getopts :z arg
do
    case ${arg} in
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
	    exit $(( EC_ERROR ))
	;;
    esac
done

# Checking if the user is root

userid=$(id | cut -d '(' -f 1 | cut -d '=' -f 2)
if [ $? -ne 0 ] || [ ${userid} -ne 0 ]
then
    display NotRootUser main ${LINENO}
    (( DisplayStatus=1 ))
    exit $(( exitcode=EC_ERROR ))
fi

scheduler_lock && {
    typeset pid
    read pid < ${SchedulerLockPath}
    display SchedulerRunning main ${LINENO} ${pid}
    exit $(( exitcode=EC_OK ))
}

# If scheduler has been temporarily disabled, report once and always exit

[[ -f "${SchedulerDisablePath}" ]] && {
    [[ ! -s "${SchedulerDisablePath}" ]] && {
	date >>${SchedulerDisablePath}
	display SchedulerDisabled main ${LINENO}
    }
    exit $(( exitcode=EC_OK ))
}

[[ -f "${ScheduleGroupPath}" ]] || {
    ls ${REPDIR_CFG}/p???.cfg >/dev/null 2>&1 && {
	display MissingScheduleActive main ${LINENO}
	(( DisplayStatus=1 ))
	exit $(( exitcode=EC_FATAL ))
    }
    exit $(( exitcode=EC_OK ))
}

# Make sure a scheduling group file exists and there are groups to process

[[ -s "${ScheduleGroupPath}" ]] || {
    display MissingSchedule main ${LINENO} ${ScheduleGroupPath}
    exit $(( exitcode=EC_OK ))
}
ScheduleGroupList=$(grep -v "#" ${ScheduleGroupPath})

[[ -z "${ScheduleGroupList}" ]] && {
    display MissingSchedule main ${LINENO} ${ScheduleGroupPath}
    exit $(( exitcode=EC_OK ))
}

# Make a temporary directory for our temporary files

OMPDIR_TMP=$(mktemp -dq -p /usr/tmp omp_XXXXXXXXXX)  || {
    display TempDirFailure main ${LINENO} /usr/tmp
    exit $(( exitcode=EC_ERROR ))
}

readonly WaitingGroupPath="${OMPDIR_TMP}/waitinggroups"
readonly ActiveGroupPath="${OMPDIR_TMP}/activegroups"
readonly DtcInfoPath="${OMPDIR_TMP}/dtcinfo"

#
# Kludge around pdksh and bash failure to handle pipelined builtins
# Also || in pdksh on exec does not work either
#

exec 3>${ActiveGroupPath}
[[ "$?"  -ne 0 ]] && {
    display TempFileFailure main ${LINENO} ${ActiveGroupPath}
}

collect_active_groups
exec 3>&-

# stop completed and unexpected state migration groups 
# and build data structures for active migration groups

while read RepGrpNum RepSchedGrp RepState RepPstore RepDevices
do
    # print ${RepGrpNum} ${RepSchedGrp} ${RepState} ${RepPstore} ${RepDevices}
    if [[ "${RepState}" = ?(NORMAL|TRACKING|BACKFRESH) ]]
    then
	(( DisplayStatus=1 ))
	stop_group $RepGrpNum || {
	    display StopFailed main ${LINENO} ${RepGrpNum}
	    (( RepDevicesActive+=${RepDevices} ))
	    (( RepGroupsActive+=${RepDevices} ))
	    continue
	}
	if [[  "${RepState}" = "NORMAL" ]]
	then
	    move_migration_group ${RepGrpNum} done || {
		(( RepDevicesActive+=${RepDevices} ))
		(( RepGroupsActive+=${RepDevices} ))
	    }
	    display MigGroupDone main ${LINENO} ${RepGrpNum} ${RepSchedGrp} 
	else
	    display UnexpectedState main ${LINENO} ${RepGrpNum} ${RepState} ${RepSchedGrp}
	    move_migration_group ${RepGrpNum} error || {
		(( RepDevicesActive+=${RepDevices} ))
		(( RepGroupsActive+=${RepDevices} ))
	    }
	fi
	(( RepGroupsStopped++ ))
	(( DelayAfterNumStops != 0 && RepGroupsStopped % DelayAfterNumStop == 0 )) && {
	    sleep ${SleepAfterNumStops}
	}
	continue
    fi

    GroupSched[10#${RepGrpNum}]=${RepSchedGrp}
    GroupNum[10#${RepGrpNum}]=${RepGrpNum}
    GroupState[10#${RepGrpNum}]="${RepState}"
    GroupPstore[10#${RepGrpNum}]="${RepPstore}"
    GroupDevices[10#${RepGrpNum}]="${RepDevices}"
    (( RepDevicesActive+=${RepDevices} ))
    (( RepGroupsActive++ ))
done < ${ActiveGroupPath}

ActivePstores=$(unique_list ${GroupPstore[@]-}) 

exec 3>${WaitingGroupPath}
[[ "$?"  -ne 0 ]] && {
    display TempFileFailure main ${LINENO} ${WaitingGroupPath}
}
collect_waiting_groups ${ScheduleGroupList}
exec 3>&-

while read RepGrpNum RepSchedGrp RepState RepPstore RepDevices
do
    # print ${RepGrpNum} ${RepSchedGrp} ${RepState} ${RepPstore} ${RepDevices}

    GroupSched[10#${RepGrpNum}]=${RepSchedGrp}
    GroupNum[10#${RepGrpNum}]=${RepGrpNum}
    GroupState[10#${RepGrpNum}]="${RepState}"
    GroupPstore[10#${RepGrpNum}]="${RepPstore}"
    GroupDevices[10#${RepGrpNum}]="${RepDevices}"
    (( RepDevicesWaiting+=${RepDevices} ))
    (( RepGroupsWaiting++ ))

    #
    # Build list of waiting migration groups belonging to a schedule group.
    # We are usng dynamic variables where schedule group is part of the name
    #

    eval "WMG_${RepSchedGrp}=\"\${WMG_${RepSchedGrp}-} ${RepGrpNum}\""
done <${WaitingGroupPath}


print RepGroupsActive ${RepGroupsActive} RepGroupsWaiting ${RepGroupsWaiting}
print MaxRepGroupsActive ${MaxRepGroupsActive} 
print MaxRepDevices ${MaxRepDevices}
print RepDevicesActive ${RepDevicesActive} RepDevicesWaiting ${RepDevicesWaiting}

if (( RepGroupsActive < MaxRepGroupsActive &&
      RepDevicesActive < MaxRepDevices && RepGroupsWaiting > 0 ))
then
    if (( MaxRepGroupsActive - RepGroupsActive >= RepGroupsWaiting &&
	  MaxRepDevices - RepDevicesActive >= RepDevicesWaiting ))
    then
	print  Start all remaining replication groups
    else
	print Start some of the remaining replication groups
    fi
    while true
    do
	(( PrevRepGroupsWaiting=RepGroupsWaiting ))

	for ScheduleGroup in ${ScheduleGroupList}
	do
	    if has_replication_groups WMG_${ScheduleGroup}
	    then
		extract_replication_group WMG_${ScheduleGroup}
		(( RepGroupsWaiting-- ))
		(( RepDevicesWaiting-=${GroupDevices[10#${RepGrpNum}]} ))

		(( RepGroupsActive++ ))
		(( RepDevicesActive+=${GroupDevices[10#${RepGrpNum}]} ))

	        (( RepGroupsActive >  MaxRepGroupsActive ||
	           RepDevicesActive > MaxRepDevices )) && break
		(( DisplayStatus=1 ))

		(( DelayAfterNumStarts != 0 && RepGroupsActive % DelayAfterNumStarts == 0 )) && {
		    sleep ${SleepAfterNumStarts}
		}

		move_migration_group ${RepGrpNum} activate || {
			move_migration_group ${RepGrpNum} error
			(( RepGroupsWaiting++ ))
			(( RepDevicesWaiting+=${GroupDevices[10#${RepGrpNum}]} ))
			(( RepGroupsActive-- ))
			(( RepDevicesActive-=${GroupDevices[10#${RepGrpNum}]} ))
			continue
		}

		[[ " ${ActivePstores} " != *\ ${GroupPstore[10#${RepGrpNum}]}\ * ]] && { 
		    init_pstore ${RepGrpNum} ${GroupPstore[10#${RepGrpNum}]} || {
			move_migration_group ${RepGrpNum} error
			(( RepGroupsWaiting++ ))
			(( RepDevicesWaiting+=${GroupDevices[10#${RepGrpNum}]} ))
			(( RepGroupsActive-- ))
			(( RepDevicesActive-=${GroupDevices[10#${RepGrpNum}]} ))
			continue
		    }
		    ActivePstores="${ActivePstores} ${GroupPstore[10#${RepGrpNum}]}"
		}

		start_group ${RepGrpNum} || {
		    stop_group ${RepGrpNum}
		    move_migration_group ${RepGrpNum} error
		    (( RepGroupsWaiting++ ))
		    (( RepDevicesWaiting+=${GroupDevices[10#${RepGrpNum}]} ))
		    (( RepGroupsActive-- ))
		    (( RepDevicesActive-=${GroupDevices[10#${RepGrpNum}]} ))
		    continue
		}

		GroupState[10#${RepGrpNum}]="PASSTHRU"
		start_refresh ${RepGrpNum} ${GroupState[10#${RepGrpNum}]} || {
		    stop_group ${RepGrpNum}
		    move_migration_group ${RepGrpNum} error
		    (( RepGroupsWaiting++ ))
		    (( RepDevicesWaiting+=${GroupDevices[10#${RepGrpNum}]} ))
		    (( RepGroupsActive-- ))
		    (( RepDevicesActive-=${GroupDevices[10#${RepGrpNum}]} ))
		    continue
		}
		display MigGroupStarted main ${LINENO} $RepGrpNum ${ScheduleGroup}
	    fi
	done

	(( PrevRepGroupsWaiting == RepGroupsWaiting ||
	   RepGroupsWaiting <= 0 ||
	   RepGroupsActive >=  MaxRepGroupsActive ||
	   RepDevicesActive >= MaxRepDevices )) && break
    done
fi
(( RepGroupsStopped > 0 && RepGroupsActive == 0 && RepGroupsWaiting == 0 )) && {
    display MigrationDone main ${LINENO}
}
print Final RepGroupsActive ${RepGroupsActive} RepGroupsWaiting ${RepGroupsWaiting}
print Final RepDevicesActive ${RepDevicesActive} RepDevicesWaiting ${RepDevicesWaiting}
