#!/usr//bin/ksh
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
# $Id: omp_admin.ksh,v 1.7 2010/01/13 19:37:51 naat0 Exp $
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
readonly OMPDIR_HOME=/etc/opt/SFTKomp
readonly OMPDIR_VARHOME=/var/opt/SFTKomp
readonly OMPDIR_LOG=${OMPDIR_HOME}/log
readonly OMPDIR_CFG=${OMPDIR_HOME}/cfg
readonly OMPDIR_DONE=${OMPDIR_HOME}/done
readonly OMPDIR_JOBS=${OMPDIR_HOME}/jobs
readonly OMPDIR_PREP=${OMPDIR_JOBS}/staging
readonly OMPDIR_SAN=${OMPDIR_JOBS}/san

readonly TunablesFile="omp_tunables"
readonly ScheduleGroupFile="schedule_groups"
readonly PstoreDeviceFile=pstore_devices
readonly SchedulerDisableFile="scheduler.disabled"
readonly SchedulerLockFile="scheduler.lock"
readonly LogFile="${PROGNAME}.log"
readonly SchedulerLogFile="omp_sched.log"

readonly TunablesPath="${OMPDIR_HOME}/${TunablesFile}"
readonly CfgScheduleGroupPath="${OMPDIR_CFG}/${ScheduleGroupFile}"
readonly PrepScheduleGroupPath="${OMPDIR_PREP}/${ScheduleGroupFile}"
readonly PstoreDevicePath=${OMPDIR_PREP}/${PstoreDeviceFile}
readonly SchedulerDisablePath="${OMPDIR_CFG}/${SchedulerDisableFile}"
readonly SchedulerLockPath="${OMPDIR_CFG}/${SchedulerLockFile}"
readonly LogPath="${OMPDIR_LOG}/${LogFile}"
readonly SchedulerLogPath="${OMPDIR_LOG}/${SchedulerLogFile}"
readonly CrontabPath="/etc/crontab"

readonly tab=$(print "\t")
readonly space=" "

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
	AlreadyDisabled)
	    severity="WARNING"
	    message[0]="The OMP scheduler has already been disabled."
	    ;;
	AlreadyEnabled)
	    severity="WARNING"
	    message[0]="The OMP scheduler has already been enabled."
	    ;;
	AlreadyDeactivated)
	    severity="WARNING"
	    message[0]="The OMP scheduler has already been deactivated."
	    ;;
	AlreadyActivated)
	    severity="WARNING"
	    message[0]="The OMP scheduler has already been activated."
	    ;;
	InvalidRFXLicense)
	    severity="ERROR"
	    message[0]="The %PRODUCTNAME% license is either missing or invalid."
	    ;;
	ScheduleTypeAlreadySet)
	    severity="ERROR"
	    message[0]="The schedule type has already been set."
	    ;;
	NotActive)
	    severity="WARNING"
	    message[0]="The OMP scheduler is not active."
	    ;;
	MissingExec)
	    severity="FATAL"
	    message[0]="Cannot locate the $1 executable."
	    ;;
	MissingCsvFiles)
	    severity="ERROR"
	    message[0]="No source or target csv files were found in directory \"$1\"."
	    ;;
	MatchMissing)
	    severity="ERROR"
	    message[0]="No matchup csv file was found in directory \"$1\"."
	    ;;
	CopyFile)
	    severity="WARNING"
	    message[0]="Only file \"$1\" was found and will be copied."
	    ;;
	NoRestoreGroups)
	    severity="WARNING"
	    message[0]="Scheduing group \"$1\" has no migration configuration files to restore."
	    ;;
	NoRestoreFiles)
	    severity="WARNING"
	    message[0]="There are no migration groups to restore."
	    ;;
	DisableFailed)
	    severity="FATAL"
	    message[0]="Cannot create disable lock file \"$1\"."
	    ;;
	RecoverRepGrp)
	    severity="INFO"
	    message[0]="Recovered configuration file \"$1\" for schedule group \"$2\"."
	    ;;
	BadEditParms)
	    severity="FATAL"
	    message[0]="The edit schedule groups parameter can only be active or prepared."
	    ;;
	InvalidOption) # number value
	    severity="FATAL"
	    message[0]="Invalid option -$1 specified."
	    ;;
	InvalidParameter) # number value
	    severity="FATAL"
	    message[0]="Invalid parameter $1 with value \"$2\"."
	    ;;
	NoScheduleGroups) # option
	    severity="ERROR"
	    message[0]="Did not find a schedule_groups file in directory \"$1\"."
	    ;;
	MissingCfgSchedGroup) # schedule_group,  schedule_groups file
	    severity="ERROR"
	    message[0]="Schedule group \"$1\" not found in configuration files."
	    ;;
	MissingSchedGroup) # schedule_group,  schedule_groups file
	    severity="ERROR"
	    message[0]="Schedule group \"$1\" not found in file \"$2\"."
	    ;;
	MissingArg) # option
	    severity="FATAL"
	    message[0]="Option $1 is missing its argument."
	    ;;
	QuestionGroup) # number
	    severity="WARNING"
	    message[0]="A schedule group name of \"prepared\" or \"active\" was found."
	    ;;
	CfgFilesExist) # directory
	    severity="ERROR"
	    message[0]="Migration group files still exist in directory \"$1\"."
	    message[1]="Either an migration is still in progress or please run the cleanup operation."
	    ;;
	NoCfgFiles) # directory
	    severity="WARNING"
	    message[0]="No Migration group files found in directory \"$1\"."
	    ;;
	ErrorFilesExist) # directory
	    severity="ERROR"
	    message[0]="There are still at least one migration group marked in error state."
	    message[1]="Either user recover command to retry the failed migration group or use"
	    message[2]="the -f option on the start command to force the removal of the failed "
	    message[3]="migration groups."
	    ;;
	NoSchedFile) # directory
	    severity="WARNING"
	    message[0]="No schedule groups file was found in directory \"$1\"."
	    ;;
	IntervalSet) # number
	    severity="INFO"
	    if [[ $1 -eq 1 ]] 
	    then
		message[0]="Cron will execute the scheduler every minute."
	    else
		message[0]="Cron will execute the scheduler every $1 minutes."
	    fi
	    ;;
	FailedRecover)
	    severity="INFO"
	    message[0]="Failed to recover configuration file \"$1\" for schedule group \"$2\"."
	    ;;
	NotInCrontab)
	    severity="WARNING"
	    message[0]="The OMP Scheduler is not available for cron to execute."
	    ;;
	NoPstoreDeviceFile)
	    severity="WARNING"
	    message[0]="The $1 file does not exist."
	    ;;
	SchedulerRunning)
	    severity="INFO"
	    message[0]="The OMP Scheduler is currently running with pid $1."
	    ;;
	StaleLockFile)
	    severity="INFO"
	    message[0]="The OMP Scheduler is currently not running, but the scheduler lock file exists."
	    ;;
	MissingPid)
	    severity="WARNING"
	    message[0]="The OMP Scheduler lock file found without a process identifier."
	    ;;
	SchedulerNotRunning)
	    severity="INFO"
	    message[0]="The OMP Scheduler is currently not running."
	    ;;
	Version)
	    no_log_output
	    severity="USAGE"
	    message[0]="\n%COMPANYNAME2% %PRODUCTNAME% OMP - Offline Migration Package"
	    message[1]="Version %VERSION%%VERSIONBUILD%"
	    message[2]="Copyright IBM Corporation 2007-2008.  All rights reserved"
	    message[3]="\nIBM, Softek, and %PRODUCTNAME% are registered or common law trademarks of"
	    message[4]="IBM Corporation in the United States, other countries, or both."
	    ;;
	CmdEcho)
	    severity="INFO"
	    for parm in "${@-}"
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
	    for parm in "${@-}"
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
	Usage)
	    no_log_output
	    severity="USAGE"
	    (( i=0 ))
	    [[ $1 = @(activate|all) ]] && {
		message[i++]="${PROGNAME} activate [<minutes>]"
		message[i++]="          Installs scheduler into /etc/crontab"
	    }
	    [[ $1 = @(deactivate|all) ]] && {
		message[i++]="${PROGNAME} deactivate"
		message[i++]="          Removes scheduler from /etc/crontab"
	    }
	    [[ $1 = @(disable|all) ]] && {
		message[i++]="${PROGNAME} disable"
		message[i++]="          Stop the scheduler temporarily"
	    }
	    [[ $1 = @(enable|all) ]] && {
		message[i++]="${PROGNAME} enable"
		message[i++]="          Restarts the scheduler after being disabled."
	    }
	    [[ $1 = @(start|all) ]] && {
		message[i++]="${PROGNAME} start [-f]"
		message[i++]="          Starts a migration"
	    }
	    [[ $1 = @(stop|all) ]] && {
		message[i++]="${PROGNAME} stop"
		message[i++]="          Kills an activate scheduler"
	    }
	    [[ $1 = @(editsched|all) ]] && {
		message[i++]="${PROGNAME} editsched|visched [prepared|active]"
		message[i++]="          edit the schedule groups file"
	    }
	    [[ $1 = @(showsched|all) ]] && {
		message[i++]="${PROGNAME} showsched [prepared|active] [-s] [-g] [-d] [-r] [-f <schedule_group>]"
		message[i++]="          display luns by schedule group"
	    }
	    [[ $1 = @(copycsv|all) ]] && {
		message[i++]="${PROGNAME} copycsv [<directory>]"
		message[i++]="          Copy source and target csv files to omp"
	    }
	    [[ $1 = @(copymatchup|all) ]] && {
		message[i++]="${PROGNAME} matchup [<directory>]"
		message[i++]="          Copy matchup csv file to omp"
	    }
	    [[ $1 = @(recover|all) ]] && {
		message[i++]="${PROGNAME} recover [<schedule_group>]..."
		message[i++]="          Rerun migrations marked into error state"
	    }
	    [[ $1 = @(taillog|all) ]] && {
		message[i++]="${PROGNAME} taillog [<tail_options>]..."
		message[i++]="          Perform a tail on the scheduler log"
	    }
	    [[ $1 = @(status|all) ]] && {
		message[i++]="${PROGNAME} status"
		message[i++]="          Perform ls on OMP directories and gives scheduler status"
	    }
	    [[ $1 = @(uninstall|all) ]] && {
		message[i++]="${PROGNAME} uninstall [all]"
		message[i++]="          Display Version and Copyright Information"
	    }
	    [[ $1 = @(version|all) ]] && {
		message[i++]="${PROGNAME} version"
		message[i++]="          Display Version and Copyright Information"
	    }
	    ;;
	*)
	    severity="ERROR"
	    message[0]="Message key \"${msgtype}\" is not defined."
	    msgtype="MissingMsg"
	    (( i=1 ))
	    for parm in "${@-}"
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

function disable_scheduler
{
    [[ -f ${SchedulerDisablePath} ]] && {
	display AlreadyDisabled disable_scheduler ${LINENO}
	return ${EC_WARN}
    }
    touch ${SchedulerDisablePath} || {
	display DisableFailed disable_scheduler ${LINENO} ${SchedulerDisablePath}
	return ${EC_FATAL}
    }
    return ${EC_OK}
}

function enable_scheduler
{
    [[ ! -f ${SchedulerDisablePath} ]] && {
	display AlreadyEnabled enable_scheduler ${LINENO}
	return ${EC_WARN}
    }
    rm -f ${SchedulerDisablePath}
    return ${EC_OK}
}

function select_scheduler
{
    if [[ -x "${PWD}/omp_sched.ksh" ]]
    then
	print "${PWD}/omp_sched.ksh" 
    elif [[ -x "${PWD}/omp_sched" ]]
    then
	print "${PWD}/omp_sched" 
    elif [[ -x "/etc/opt/SFTKomp/bin/omp_sched.ksh" ]]
    then
	display InstallWrong  select_scheduler ${LINENO} omp_sched 
	print "/etc/opt/SFTKomp/bin/omp_sched.ksh"
    elif [[ -x "/etc/opt/SFTKomp/bin/omp_sched" ]]
    then
	print "/etc/opt/SFTKomp/bin/omp_sched.ksh"
    else
	display MissingExec select_scheduler ${LINENO} omp_sched 
    fi
}

function update_crontab
{
    typeset scheduler
    typeset interval=""

    case "${1-}" in
      activate)
	shift
	if [[ $# > 0 ]]
	then
	    if [[ "$1" = +([0-9]) && "$1" < 60 ]]
	    then
		interval=$1
	    else
		display InvalidParameter update_crontab ${LINENO} 1 $1
		interval=1
	    fi
	    display IntervalSet update_crontab ${LINENO} ${interval}
	fi
	if [[ -z "$interval" ]]
	then
	   interval=1
	   grep omp_sched ${CrontabPath} >/dev/null 2>&1 && {
	       display AlreadyActivated update_crontab ${LINENO}
	       return
	   }
	   display IntervalSet update_crontab ${LINENO} ${interval}
	fi
	   scheduler=$(select_scheduler)
		ed - ${CrontabPath} <<-EOF
		g/omp_sched/d
		a
		*/${interval-1} * * * * root /etc/opt/SFTKomp/bin/omp_sched >/dev/null 2>&1
		.
		w
		q
		EOF
	;;
      deactivate)
	grep omp_sched ${CrontabPath} >/dev/null 2>&1 || {
	    display AlreadyDeactivated update_crontab ${LINENO}
	    return
	}
	ed - ${CrontabPath} <<-EOF
	g/omp_sched/d
	w
	q
	EOF
	;;
    esac
}

function recover_migration_groups
{
    typeset schedgrp recoverfiles errorfile cfgfile
    typeset -i exitcode=0

    if (( $# > 0 ))
    then
	for schedgrp in "$@"
	do
	    recoverfiles=$(egrep -l "NOTES:[\ ${tab}]+${schedgrp}" ${OMPDIR_DONE}/p[0-9][0-9][0-9].error 2>/dev/null)
	    [[ -z ${recoverfiles} ]] && {
		display NoRestoreGroups recover_migraiton_groups ${LINENO} ${schedgrp}
		(( exitcode=1 ))
		continue
	    }
	    for errorfile in ${recoverfiles}
	    do
		cfgfile="${errorfile%%.error}.cfg"
		cfgfile="${OMPDIR_CFG}${cfgfile##${OMPDIR_DONE}}"
		if mv ${errorfile} ${cfgfile}
		then
		    display RecoverRepGrp recover_migraiton_groups ${LINENO} ${errorfile##${OMPDIR_DONE}/} ${schedgrp}
		else
		    display FailedRecover recover_migraiton_groups ${LINENO} ${errorfile##${OMPDIR_DONE}/} ${schedgroup}
		    (( exitcode=2 ))
		fi
	    done
	done
    else
	recoverfiles=$(ls ${OMPDIR_DONE}/p[0-9][0-9][0-9].error 2>/dev/null)
	[[ -z "${recoverfiles}" ]] && {
	    display NoRestoreFiles recover_migration_groups ${LINENO}
	    (( exitcode=1 ))
	    return $(( exitcode ))
	}
	
	for errorfile in ${recoverfiles}
	do
	    schedgrp=$(egrep "NOTES:" ${errorfile} | gawk '{print $2;}')
	    cfgfile="${errorfile%%.error}.cfg"
	    cfgfile="${OMPDIR_CFG}${cfgfile##${OMPDIR_DONE}}"
	    if mv ${errorfile} ${cfgfile}
	    then
		display RecoverRepGrp recover_migraiton_groups ${LINENO} ${errorfile##${OMPDIR_DONE}/} ${schedgrp}
	    else
		display FailedRecover recover_migraiton_groups ${LINENO} ${errorfile##${OMPDIR_DONE}/}  ${schedgrp}
		(( exitcode=2 ))
	    fi
	done
	print $?
    fi
    return $(( exitcode ))
}

function start_migration
{
    typeset rc
    disable_scheduler
    rc=$?
    if (( ${rc} >= EC_ERROR )) 
    then
	return ${EC_ERROR}
    fi
    /opt/SFTKdtc/bin/dtclicinfo -q >/dev/null 2>&1 || {
	[[ ${rc} -eq EC_OK ]] && enable_scheduler
	display InvalidRFXLicense start_migration  ${LINENO}
	return ${EC_ERROR}
    }
    ls ${REPDIR_CFG}/*.cfg >/dev/null 2>&1 && {
	[[ ${rc} -eq EC_OK ]] && enable_scheduler
	display CfgFilesExist start_migration  ${LINENO} ${REPDIR_CFG}
	return ${EC_ERROR}
    }
    ls ${OMPDIR_CFG}/*.cfg >/dev/null 2>&1 && {
	[[ ${rc} -eq EC_OK ]] && enable_scheduler
	display CfgFilesExist start_migration  ${LINENO} ${OMPDIR_CFG}
	return ${EC_ERROR}
    }
    ls ${PrepScheduleGroupPath} >/dev/null 2>&1 || {
	[[ ${rc} -eq EC_OK ]] && enable_scheduler
	display NoSchedFile start_migration  ${LINENO} ${OMPDIR_PREP}
	return ${EC_WARN}
    }
    ls ${OMPDIR_PREP}/*.cfg >/dev/null 2>&1 || {
	[[ ${rc} -eq EC_OK ]] && enable_scheduler
	display NoCfgFiles start_migration  ${LINENO} ${OMPDIR_PREP}
	return ${EC_WARN}
    }
    ls ${OMPDIR_DONE}/p???.error >/dev/null 2>&1 && {
	[[ "${1-}" != "-f" ]] && {
	    [[ ${rc} -eq EC_OK ]] && enable_scheduler
	    display ErrorFilesExist start_migration  ${LINENO}
	    return ${EC_WARN}
	}
    }
    rm -f ${OMPDIR_DONE}/p[0-9][0-9][0-9].@(done|error) >/dev/null 2>&1
    rm -f ${CfgScheduleGroupPath} >/dev/null 2>&1
    mv ${OMPDIR_PREP}/p[0-9][0-9][0-9].cfg ${OMPDIR_CFG}
    mv ${PrepScheduleGroupPath} ${OMPDIR_CFG}
    grep omp_sched ${CrontabPath} >/dev/null 2>&1 || {
	display NotActive start_migration ${LINENO}
    }
    [[ ${rc} -eq EC_OK ]] && enable_scheduler
}

function stop_migration
{
    typeset rc pid
    disable_scheduler
    rc=$?
    [[ -f ${SchedulerLockPath}  ]] && {
	read pid < ${SchedulerLockPath} >/dev/null 2>&1
	[[ $? -ne 0 || -z "${pid-}" ]] && {
	    display NotActive stop_migration ${LINENO}
	    [[ ${rc} -eq EC_OK ]] && enable_scheduler
	    return 1
	}
	kill -SIGTERM ${pid}
    }
    [[ ${rc} -eq EC_OK ]] && enable_scheduler
}

function edit_schedule_groups
{
    typeset editor
    typeset -l arg=${1-}
    if [[ -n "${VISUAL-}" ]]
    then
	editor="${VISUAL}"
    elif [[ -n "${EDITOR-}" ]]
    then
	editor="${EDITOR}"
    else
	editor="/bin/vi"
    fi

    if [[ -z "${arg-}" || "prepared" = ${arg-}* ]]
    then
	if [[ -f ${PrepScheduleGroupPath} ]]
	then
	    ${editor} ${PrepScheduleGroupPath}
	else
	    display NoScheduleGroups edit_schedule_groups ${LINENO} ${PrepScheduleGroupPath%/*}
	fi
    elif [[ "active" = ${arg-}* ]]
    then
	if [[ -f ${CfgScheduleGroupPath} ]]
	then
	    ${editor} ${CfgScheduleGroupPath}
	else
	    display NoScheduleGroups edit_schedule_groups ${LINENO} ${CfgScheduleGroupPath%/*}
	 fi
    else
	display BadEditParms edit_schedule_groups ${LINENO} "${1-}"
	return ${EC_ERROR}
    fi
}

function copy_csv_files
{
    typeset source=$1/source.csv
    typeset target=$1/target.csv
    [[ ! -s ${source} && ! -s ${target} ]] && {
	display MissingCsvFiles copy_csv_files ${LINENO} $1
	return ${EC_ERROR}
    }
    [[ ! -s ${target} ]] && {
	display CopyFile copy_csv_files ${LINENO} ${source}
	target=""
    }
    [[ ! -s ${source} ]] && {
	display CopyFile copy_csv_files ${LINENO} ${target}
	source=""
    }
    cp ${source-} ${target-} ${OMPDIR_SAN} || return ${EC_ERROR}
    return ${EC_ERROR}
}
function copy_matchup_file
{
    typeset matchup=$1/matchup.csv

    [[ ! -s ${matchup} ]] && {
	display MatchMissing copy_matchup_file ${LINENO} $1
	return ${EC_ERROR}
    }
    cp ${matchup-} ${OMPDIR_SAN} || return ${EC_ERROR}
    return ${EC_ERROR}
}

function show_schedule_groups
{
    integer DisplayType=0
    typeset CfgFiles=""
    typeset FilterGroups=""
    typeset ScheduleGroup=""
    typeset GroupList=""
    typeset gawk_prog
    integer retcode=0

    function expand_args
    {
	IFS=":,${tab}${space}"
	set -- $@
	IFS=
	print $@
    }

    # In ksh93 Must create this function using () not the function statement 
    # This allows the sharing of variables with function "show_schedule_groups"

    SetShowScheduleArgs()
    {
	integer retcode=0
	# Must reset OPTIND before calling getopts multiple times since OPTIND is incremented
	OPTIND=1
	while getopts :sgdrfh arg
	do
	    case $arg in
		s)
		    (( DisplayType |= 1 ))
		    ;;
		g)
		    (( DisplayType |= 2 ))
		    ;;
		d)
		    (( DisplayType |= 4 ))
		    ;;
		r)
		    (( DisplayType |= 8 ))
		    ;;
		f)
		    (( retcode=1 ))
		    ;;
		h|\?)
		    display Usage SetShowScheduleArgs ${LINENO} showsched
		    (( retcode=2 ))
		    ;;
		*)
		    (( retcode=2 ))
		    display InvalidOption SetShowScheduleArg ${LINENO} ${OPTARG}
		    ;;
	    esac
	done
	return ${retcode}
    }

    if [[ $# -eq 1 && -z "$1" ]]
    then
	(( DisplayType=1 ))
	CfgFiles=$(ls ${OMPDIR_PREP}/p[0-9][0-9][0-9].cfg 2>/dev/null)
    else
	while (( $# > 0 ))
	do
	    case $1 in   
		prepared)
		    if [[ -n "${CfgFiles}" ]]
		    then
			display ScheduleTypeAlreadySet show_schedule_groups ${LINENO}
			return 1
		    else
			CfgFiles=$(ls ${OMPDIR_PREP}/p[0-9][0-9][0-9].cfg 2>/dev/null)
			[[ -z "${CfgFiles}" ]] && {
			    display NoCfgFiles show_schedule_groups ${LINENO} ${OMPDIR_PREP}
			    return 1
			}
			ScheduleGroupPath="${PrepScheduleGroupPath}"
		    fi
		;;
		active)
		    if [[ -n "${CfgFiles}" ]]
		    then
			display ScheduleTypeAlreadySet show_schedule_groups ${LINENO}
			return 1
		    else
			CfgFiles=$(ls ${OMPDIR_CFG}/p[0-9][0-9][0-9].cfg ${REPDIR_CFG}/p[0-9][0-9][0-9].cfg ${OMPDIR_DONE}/p[0-9][0-9][0-9]@(.done|.error) 2>/dev/null)
			[[ -z "${CfgFiles}" ]] && {
			    display NoCfgFiles show_schedule_groups ${LINENO} ${REPDIR_CFG}
			    return 1
			}
			ScheduleGroupPath="${CfgScheduleGroupPath}"
		    fi
		;;
		-*)		
		    SetShowScheduleArgs $1
		    (( retcode=$? ))
		    if (( ${retcode} == 1 ))
		    then
			shift
			if [[ $# -eq 0 || $1 = -* ]]
			then
			    display MissingArg show_schedule_groups ${LINENO}  "-f"
			    return ${EC_ERROR}
			else
			    if [[ $1 = ?(prepared|active) ]]
			    then
				display QuestionGroup show_schedule_groups ${LINENO}
			    fi
			    for ScheduleGroup in $(expand_args $1)
			    do
				if [[ -z ${GroupList-} ]]
				then
				    GroupList="${ScheduleGroup}"
				else
				    GroupList="${GroupList} ${ScheduleGroup}"
				fi
			    done
			fi
		    elif (( ${retcode} == 2 ))
		    then
			return 1
		    fi
		;;
		*)
		    display Usage show_schedule_groups ${LINENO} showsched
		    return 1
		;;
	    esac
	    shift
	done
    fi

    if (( DisplayType == 0 ))
    then
	(( DisplayType=1 ))
    fi

    if [[ -z "${CfgFiles}" ]]
    then
	CfgFiles=$(ls ${OMPDIR_PREP}/p[0-9][0-9][0-9].cfg 2>/dev/null)
	[[ -z "${CfgFiles}" ]] && {
	    display NoCfgFiles show_schedule_groups ${LINENO} ${OMPDIR_PREP}
	    return 1
	}
	ScheduleGroupPath="${PrepScheduleGroupPath}"
    fi

    for ScheduleGroup in ${GroupList}
    do
	if { grep ${ScheduleGroup} ${ScheduleGroupPath} >/dev/null 2>&1; }
	then
	    gawk_prog='
		    BEGIN { exitcode=1; }
		    $2=="'${ScheduleGroup}'" { exitcode=0; }
		    END { exit(exitcode); }'
	    if grep "NOTES:" ${CfgFiles} 2>/dev/null | gawk "${gawk_prog}"
	    then
		if [[ -z ${FilterGroups-} ]]
		then
		    FilterGroups="${ScheduleGroup}"
		else
		    FilterGroups="${FilterGroups}:${ScheduleGroup}"
		fi
	    else
		display  MissingCfgSchedGroup show_schedule_groups ${LINENO} ${ScheduleGroup} ${ScheduleGroupPath}
		return 1
	    fi
	else
	    display  MissingSchedGroup show_schedule_groups ${LINENO} ${ScheduleGroup} ${ScheduleGroupPath}
	    return 1
	fi
    done
    omp_showsched.awk -v DisplayType=${DisplayType} -v FilterGroups=":${FilterGroups}:" ${CfgFiles}
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
    [[ -n "${DebugAdmin:-}" ]] && OMP_DEBUG=${DebugAdmin}
fi    

[[ -o xtrace || -n "${OMP_DEBUG-}" ]] && {
    typeset -ft $(typeset +f)
}

userid=$(id | cut -d '(' -f 1 | cut -d '=' -f 2)
if [ $? -ne 0 ] || [ ${userid} -ne 0 ]
then
    display NotRootUser main ${LINENO} 
    exit $(( EC_ERROR ))
fi

no_std_output
display CmdEcho main ${LINENO} ${PROGNAME} ${@-}

case ${1-} in
    ?(-|--|/)activate)
	shift
	[[ ${1-} = @(-\?|-h) ]] && {
	    display Usage main ${LINENO} activate
	    return 0
	}
	update_crontab activate ${@-}
	;;
    ?(-|--|/)deactivate)
	shift
	[[ ${1-} = @(-\?|-h) ]] && {
	    display Usage main ${LINENO} deactivate
	    return 0
	}
	update_crontab deactivate
	;;
    ?(-|--|/)start)
	shift
	[[ ${1-} = @(-\?|-h) ]] && {
	    display Usage main ${LINENO} start
	    return 0
	}
	start_migration ${@-}
	;;
    ?(-|--|/)stop)
	shift
	[[ ${1-} = @(-\?|-h) ]] && {
	    display Usage main ${LINENO} stop
	    return 0
	}
	stop_migration
	;;
    ?(-|--|/)enable)
	shift
	[[ ${1-} = @(-\?|-h) ]] && {
	    display Usage main ${LINENO} enable
	    return 0
	}
	enable_scheduler
	;;
    ?(-|--|/)disable)
	shift
	[[ ${1-} = @(-\?|-h) ]] && {
	    display Usage main ${LINENO} disable
	    return 0
	}
	disable_scheduler
	;;
    ?(-|--|/)taillog)
	shift
	[[ ${1-} = @(-\?|-h) ]] && {
	    display Usage main ${LINENO} taillog
	    return 0
	}
	tail ${@-} ${SchedulerLogPath}
	;;
    ?(-|--|/)showsched)
	shift
	[[ ${1-} = @(-\?|-h) ]] && {
	    display Usage main ${LINENO} showsched
	    return 0
	}
	show_schedule_groups "${@-}"
	;;
    ?(-|--|/)@(vi|edit)sched)
	shift
	[[ ${1-} = @(-\?|-h) ]] && {
	    display Usage main ${LINENO} editsched
	    return 0
	}
	edit_schedule_groups ${@-}
	;;
    ?(-|--|/)ver?(s|si|sio|sion))
	shift
	[[ ${1-} = @(-\?|-h) ]] && {
	    display Usage main ${LINENO} version
	    return 0
	}
	display Version main ${LINENO}
	;;
    ?(-|--|/)@(copycsv|cpcsv))
	shift
	[[ ${1-} = @(-\?|-h) ]] && {
	    display Usage main ${LINENO} copycsv
	    return 0
	}
	copy_csv_files ${@-.}
	;;
    ?(-|--|/)@(cpmatch|cpmatchup|copymatch|copymatchup))
	shift
	[[ ${1-} = @(-\?|-h) ]] && {
	    display Usage main ${LINENO} copymatchup
	    return 0
	}
	copy_matchup_file ${@-.}
	;;
    ?(-|--|/)status)
	shift
	[[ ${1-} = @(-\?|-h) ]] && {
	    display Usage main ${LINENO} status
	    return 0
	}
	print "\n${REPDIR_CFG}:"; ls ${REPDIR_CFG}
	print "\n${OMPDIR_CFG}:"; ls ${OMPDIR_CFG}
	print "\n${OMPDIR_LOG}:"; ls ${OMPDIR_LOG}
	print "\n${OMPDIR_PREP}:"; ls ${OMPDIR_PREP}
	print "\n${OMPDIR_SAN}:"
	[[ -f ${OMPDIR_SAN}/source.csv ]] && ls -o  ${OMPDIR_SAN}/source.csv
	[[ -f ${OMPDIR_SAN}/target.csv ]] && ls -o  ${OMPDIR_SAN}/target.csv
	ls ${OMPDIR_SAN}
	print "\n${OMPDIR_DONE}:"; ls ${OMPDIR_DONE}
	print "\n"

	[[ -f ${PstoreDevicePath} ]] || {
	    display NoPstoreDeviceFile start_migration ${LINENO} ${PstoreDeviceFile}
	}

	grep omp_sched ${CrontabPath} >/dev/null 2>&1 || {
	    display NotInCrontab start_migration ${LINENO}
	}
	if [[ -f ${SchedulerLockPath} ]]
	then
	    read pid < ${SchedulerLockPath} >/dev/null 2>&1
	    status=$?
	else
	    status=1
	fi
	if [[ ${status} -eq 0 ]]
	then
	    if [[ -n "${pid-}" ]]
	    then
		kill -0 ${pid}
		if [[ $? -eq 0 ]]
		then
		    display SchedulerRunning main ${LINENO} ${pid}
		else
		    display StaleLockFile main ${LINENO} ${pid}
		fi
	    else
		display MissingPid main ${LINENO} ${pid}
	    fi
	else
	    display SchedulerNotRunning main ${LINENO}
	fi
	;;
    ?(-|--|/)recover)
	shift
	[[ ${1-} = @(-\?|-h) ]] && {
	    display Usage main ${LINENO} recover
	    return 0
	}
	recover_migration_groups ${@-}
	;;
    ?(-|--|/)uninstall)
	shift
	[[ ${1-} = @(-\?|-h) ]] && {
	    display Usage main ${LINENO} uninstall
	    return 0
	}
	grep omp_sched ${CrontabPath} >/dev/null 2>&1 && {
	    ed - ${CrontabPath} <<-EOF
		g/omp_sched/d
		w
		q
		EOF
	}
	[[ -d ${OMPDIR_HOME} ]] && stop_migration
	if rpm -q SFTKomp >/dev/null 2>&1
	then
	    rpm -e SFTKomp
	else
	    rm -rf /etc/profile.d/omp.csh /etc/profile.d/omp.sh "${OMPDIR_HOME}"
	    [[ "${1-}" = all ]]  && rm -rf "${OMPDIR_VARHOME}"
	fi
	;;
    *)
	display Usage main ${LINENO} all
	;;
esac
