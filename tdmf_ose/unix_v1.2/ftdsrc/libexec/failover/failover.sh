#! /bin/sh
########################################################################
#
#  %Q%failover
#
# LICENSED MATERIALS / PROPERTY OF IBM
# %PRODUCTNAME% version %VERSION%
# (c) Copyright %COMPANYNAME%  %COPYRIGHTYEAR 2001%.  All Rights Reserved.
# The source code for this program is not published or otherwise divested of
# its trade secrets, irrespective of what has been deposited with the U.S.
# Copyright Office.
# US Government Users Restricted Rights - Use, duplication or disclosure
# restricted by GSA ADP Schedule Contract with IBM Corp.
#
#
########################################################################

#-------------------------------------------------------------------------------
##### FUNCTION TO SHOW THE COMMAND USAGE ####
show_usage() {

    echo "----------------------------------------------------------------------------------------------------------------------------"
    echo "USAGE:"
    echo "  %Q%failover -g <group number> [-q] [-p] [-m <max wait time on I/Os>] [-n <min time without I/Os>]"
    echo "              [-e <max time BAB not empty>] [-z <min time BAB no group entries>] [-h] [-r] [-b] [-s]"
    echo "----------------------------------------------------------------------------------------------------------------------------"
    echo "  This script performs the sequence of steps to failover from a source server to its designated target server."
    echo "  -g: specifies the group number;"
    echo "  -q: quiet mode; just log messages and results; do not print them on server console;"
    echo "  -p: no prompt; do not prompt the user for any confirmation;"
    echo "  -m: maximum wait time (seconds) on I/Os to quiesce for the specified group on the source server (default: 60);"
    echo "  -n: minimum time (seconds) without I/Os detected when waiting for I/Os to quiesce (default: 10);"
    echo "  -e: maximum wait time (seconds) for BAB to have no entries for the specified group on the source server (default: 60);"
    echo "  -z: minimum time (seconds) with BAB having zero entries for the specified group (default: 10);"
    echo "  -h: display this usage paragraph;"
    echo "  Option provided by the DMC-Agent or target dtc master daemon:"
    echo "  -r: run only the Secondary (target) server part of the script (for RMD side of a 1-to-1 configuration);"
    echo "  Options specific to the case of Linux or AIX boot drive migration:"
    echo "  -b: Linux or AIX boot drive migration for boot drive failover or replacement on target server;"
    echo "  -k: in AIX boot drive migration and failover, keep the target server running at the end of the sequence (no automatic reboot of the target);"
    echo "  -s: reboot (shutdown -r) the source server after source failover part in Linux boot drive migration only (not AIX)."
    echo "------------------------------------------------------------------------------------------------------------------------------"
}


#-------------------------------------------------------------------------------
##### FUNCTION TO LOG A MESSAGE AND DISPLAY IT IF VERBOSE IS SET ####
# Arguments:
# $1: message string
log_message() {

    datestr=`/bin/date "+%Y/%m/%d %T"`;
    errhdr="[${datestr}] %Q%failover: ";
    errmsg=`/bin/echo "${errhdr}" "$1"`
    /bin/echo ${errmsg} 1>> ${logf} 2>&1

    if [ $Verbose -gt 0 ]
    then
	    /bin/echo ${errmsg}
    fi
}

#-------------------------------------------------------------------------------
##### FUNCTION TO SET PLATFORM SPECIFIC VARIABLES ####
set_platform_specifics() {

    IsLinux=0
    IsAIX=0

    Platform=`/bin/uname`
    if [ $Platform = "HP-UX" ]; then
        logf=/var/opt/SFTK%Q%/%Q%error.log;
        DtcLibPath=/etc/opt/SFTK%Q%;
        DtcBinPath=/opt/SFTK%Q%/bin;
        SyncCmd=/bin/sync;
	    COMMAND_USR=`/usr/bin/whoami`

    elif [ $Platform = "SunOS" ]; then
        logf=/var/opt/SFTK%Q%/%Q%error.log;
        DtcLibPath=/etc/opt/SFTK%Q%;
        DtcBinPath=/opt/SFTK%Q%/bin;
        SyncCmd=/bin/sync;
	    if [ -x /usr/ucb/whoami ]; then COMMAND_USR=`/usr/ucb/whoami`; else COMMAND_USR=`/usr/bin/whoami`; fi

    elif [ $Platform = "AIX" ]; then
        logf=/var/%Q%/%Q%error.log;
        DtcLibPath=/etc/%Q%/lib;
        DtcBinPath=/usr/%Q%/bin;
        SyncCmd=/usr/sbin/sync;
	    COMMAND_USR=`/bin/whoami`
        IsAIX=1

    elif [ $Platform = "Linux" ]; then
        IsLinux=1
        logf=/var/opt/SFTK%Q%/%Q%error.log;
        DtcLibPath=/etc/opt/SFTK%Q%;
        DtcBinPath=/opt/SFTK%Q%/bin;
        SyncCmd=/bin/sync;
	    COMMAND_USR=`/usr/bin/whoami`;
        KernelVersion=`/bin/uname -r`;
        # Determine if we are on Redhat or SuSE
        release_file="/etc/redhat-release"
        /bin/ls $release_file 1> /dev/null 2>&1
        file_found=$?
        if [ $file_found -eq 0 ]
        then
            IsRedHat=1
        else
            release_file="/etc/SuSE-release"
            /bin/ls $release_file 1> /dev/null 2>&1
            file_found=$?
            if [ $file_found -eq 0 ]
            then
                IsRedHat=0
            else
                log_message "Did not find /etc/redhat-release nor /etc/SuSE-release to determine if we are on RedHat or SuSE; exiting." 
                exit 1
            fi 
        fi 
    fi
}

#-----------------------------------------------------------------------------------------
##### FUNCTION TO VERIFY THAT A TIME ARGUMENT IN SECONDS DOES NOT EXCEED A FEW HOURS #####
# Argument: $1 = numeric value to check; $2 = description of value
verify_reasonable_time_value() {

    numeric_value=$1
    value_description="$2"
    string_length=`/bin/echo ${#numeric_value} | /bin/awk '{print $1}'`
    # Accept no more than 5 digits for a few hours
    if [ $string_length -gt 5 ]
    then
        log_message "The $value_description exceeds the maximum number of digits to represent a few hours (5 digits)."
        show_usage
        exit 1
    fi

}
#-------------------------------------------------------------------------------
##### FUNCTION TO PARSE THE ARGUMENTS PROVIDED TO THIS SCRIPT #####
parse_command_arguments() {

    ShowUsage=0
    Verbose=1
    GroupNumber=1000
    SetOneToOneRMD=0
    LinuxBootDriveMigration=0
    AIXBootDriveMigration=0
    BootDriveMigration=0
    MaxWaitOnIOs=60
    MinWaitNoIOs=10
    MaxWaitBABnotEmpty=60
    MinWaitBABempty=10
    NoPrompt=0
    error=0
    mflag=0
    nflag=0
    eflag=0
    zflag=0
    KeepRunning=1
    KeepAIXTargetRunning=0

    while getopts "g:bqrm:n:e:z:pskh" opt
    do
            case $opt in
            g)
                    GroupNumber=$OPTARG;;
            h)
                    ShowUsage=1;;
            q)
                    Verbose=0;;
            r)
                    SetOneToOneRMD=1;;
            b)
                    if [ $IsLinux -eq 1 ]
                    then
                        LinuxBootDriveMigration=1
                        BootDriveMigration=1
                    else
                        if [ $IsAIX -eq 1 ]
                        then
                            AIXBootDriveMigration=1
                            BootDriveMigration=1
                        fi
                    fi;;
            s)
                    KeepRunning=0;;
            m)
                    mflag=1
                    MaxWaitOnIOs=$OPTARG
                    verify_reasonable_time_value $MaxWaitOnIOs "max wait time on IOs";;
            n)
                    nflag=1
                    MinWaitNoIOs=$OPTARG
                    verify_reasonable_time_value $MinWaitNoIOs "min wait time no IOs";;
            e)
                    eflag=1
                    MaxWaitBABnotEmpty=$OPTARG
                    verify_reasonable_time_value $MaxWaitBABnotEmpty "max wait time BAB not empty";;
            z)
                    zflag=1
                    MinWaitBABempty=$OPTARG
                    verify_reasonable_time_value $MinWaitBABempty "min wait time with BAB empty";;
            p)
                    NoPrompt=1;;
            k)
                    KeepAIXTargetRunning=1;;
            \?)
                    log_message "Invalid option or missing argument." 
                    error=1;;
            \:)
                    log_message "Option -$OPTARG requires an argument."
                    error=1;;
            esac
    done

    if [ $ShowUsage -gt 0 ]
    then
        show_usage
        exit 0
    fi

    if [ $error -gt 0 ]
    then
        show_usage
        exit 1
    fi

    # Check that the group identifier argument is a valid number
    if /bin/echo $GroupNumber | /bin/grep "[^0-9"] 2>&1 > /dev/null
    then
      log_message "The group identifier is not a valid number."
      show_usage
      exit 1
    fi

    # Check that a valid group number has been provided
    if [ $GroupNumber -gt 999 ]
    then
      log_message "The group number is mandatory and the maximum value is 999 (option -g)."
      show_usage
      exit 1
    fi

    if /bin/echo $MaxWaitOnIOs | /bin/grep "[^0-9"] 2>&1 > /dev/null
    then
      log_message "The maximum wait time on I/Os to quiesce is not a valid number."
      show_usage
      exit 1
    fi

    if /bin/echo $MinWaitNoIOs | /bin/grep "[^0-9"] 2>&1 > /dev/null
    then
      log_message "The minimum time without I/Os detected is not a valid number."
      show_usage
      exit 1
    fi

    if /bin/echo $MaxWaitBABnotEmpty | /bin/grep "[^0-9"] 2>&1 > /dev/null
    then
      log_message "The maximum wait time for BAB to empty is not a valid number."
      show_usage
      exit 1
    fi

    if /bin/echo $MinWaitBABempty | /bin/grep "[^0-9"] 2>&1 > /dev/null
    then
      log_message "The minimum time without BAB entries for this group is not a valid number."
      show_usage
      exit 1
    fi

    if [ $BootDriveMigration -eq 1 ]
    then
        if [ $IsLinux -eq 0 -a $IsAIX -eq 0 ]
        then
            log_message "Option -b is for Linux or AIX only. Exiting."
            exit 1
        fi
        if [ $mflag -eq 1 -o $nflag -eq 1 -o $eflag -eq 1 -o $zflag -eq 1 ]
        then
            log_message "Options -m, -n, -e and -z are not applicable with option -b. Exiting."
            exit 1
        fi
    fi

    if [ $LinuxBootDriveMigration -eq 0 -a $KeepRunning -eq 0 ]
    then
        log_message "Options -s is supported only in Linux boot drive failover. Exiting."
        exit 1
    fi

}


#-------------------------------------------------------------------------------
##### FUNCTION TO GET THE CURRENT TIME IN SECONDS #####
# Arguments:
# $1: platform on which we run (Solaris does not support the date +%s command)
# Return: $CurrentTimeSeconds
GetCurrentTimeSeconds() {

    platform=$1

    if [ $platform = "SunOS" ]
    then
        which perl 1>/dev/null 2>&1
        perl_status=$?
        if [ $perl_status -eq 0 ]
        then
            perl_path=`which perl | /bin/awk '{print $1}'`
            CurrentTimeSeconds=`$perl_path -e "print time" | /bin/awk '{print $1}'`
        else
            which truss 1>/dev/null 2>&1
            truss_status=$?
            if [ $truss_status -eq 0 ]
            then
                truss_path=`which truss | /bin/awk '{print $1}'`
                CurrentTimeSeconds=`$truss_path /usr/bin/date 2>&1 | /bin/grep ^time | /bin/awk -F"= " '{print $2}'`
            else
                log_message "Cannot find either perl nor truss to manipulate time info on Solaris."
            fi
        fi
    else
        CurrentTimeSeconds=`/bin/date "+%s" 2> /dev/null | /bin/awk '{print $1}'`
    fi

}


#-------------------------------------------------------------------------------
##### FUNCTION TO CHECK IF PRE / POST FAILOVER SCRIPTS ARE READY FOR THIS GROUP #####
# Arguments:
# $1: group number 3-digit substring (ex.: "001")
VerifyPrePostFailoverScripts() {

    groupstring=$1

    SourcePreFailoverReady=0
    SourcePostFailoverReady=0
    TargetPostFailoverReady=0

    # Check Source pre-failover script
    ScriptName="$DtcLibPath/dtc_pre_failover_p$groupstring.sh"
    /bin/ls $ScriptName 1> /dev/null 2>&1
    result=$?
    if [ $result -eq 0 ]
    then
        SourcePreFailoverReady=1
    fi

    # Check Source post-failover script
    ScriptName="$DtcLibPath/dtc_post_failover_p$groupstring.sh"
    /bin/ls $ScriptName 1> /dev/null 2>&1
    result=$?
    if [ $result -eq 0 ]
    then
        SourcePostFailoverReady=1
    fi

    # Check Target post-failover script
    ScriptName="$DtcLibPath/dtc_post_failover_s$groupstring.sh"
    /bin/ls $ScriptName 1> /dev/null 2>&1
    result=$?
    if [ $result -eq 0 ]
    then
        TargetPostFailoverReady=1
    fi
}


#-------------------------------------------------------------------------------
##### FUNCTION TO VERIFY THAT THE GROUP IS IN A STATE ACCEPTED FOR FAILOVER #####
CheckIfGroupStateOk() {
# Arguments:
# $1: group number

    group=$1
    GroupState=`$DtcBinPath/dtcinfo -g$group -q  2>/dev/null | /bin/grep "Mode" | /bin/awk '{print $4}'`
    if [ "$GroupState" != "Normal" ]
    then
        if [ "$GroupState" = "Full" ]
        then
            GroupState="Full Refresh"
        fi
        log_message "Group $group state = $GroupState, not accepted for failover; must be in Normal mode; exiting failover sequence."
        exit 1
    else
        # Group state is Normal; verify if Checkpoint mode is ON
        checkpoint_state=`$DtcBinPath/dtcinfo -g$group -q  2>/dev/null | /bin/grep -i "Checkpoint" | /bin/awk '{print $3}'`
        if [ "$checkpoint_state" = "on" -o "$checkpoint_state" = "On" -o "$checkpoint_state" = "ON" ]
        then
            log_message "Group $group state = Normal but Checkpoint state is ON, not accepted for failover; exiting failover sequence."
            exit 1
        fi
    fi
    log_message "Group $group state = $GroupState."
}


#-------------------------------------------------------------------------------
##### FUNCTION TO EXECUTE THE SOURCE SERVER PRE-FAILOVER SCRIPT #####
# Arguments:
# $1: group number
# Return: no return if failure, otherwise it means status OK
# <<< TODO: test a script that must perform operations from a user ID other than root
RunSourcePreFailoverScript() {

    group=$1
    FormatGroupNumberSubstring $group

    ScriptName="$DtcLibPath/dtc_pre_failover_p$GroupNumberSubstring.sh"
    log_message "Launching source server pre-failover script $ScriptName"
    $ScriptName 1> /dev/null 2>&1
    result=$?
    if [ $result -ne 0 ]
    then
        log_message "Pre-failover script $ScriptName returned error code $result; exiting failover sequence."
        /bin/ls -l $ScriptName | /bin/awk '{print $1}' | /bin/grep "x" 1> /dev/null 2>&1
        if [ $? -ne 0 ]
        then
            log_message "NOTE: your script $ScriptName does not seem to have the Executable permission."
        fi
        exit 1
    else
        log_message "Pre-failover script $ScriptName returned success status (0); continuing...."
    fi

}


#-------------------------------------------------------------------------------
##### FUNCTION TO EXECUTE THE SOURCE SERVER POST-FAILOVER SCRIPT #####
# Arguments:
# $1: group number
# Return: no return if failure, otherwise it means status OK
# <<< TODO: test a script that must perform operations from a user ID other than root
RunSourcePostFailoverScript() {

    group=$1
    FormatGroupNumberSubstring $group

    ScriptName="$DtcLibPath/dtc_post_failover_p$GroupNumberSubstring.sh"
    log_message "Launching source server post-failover script $ScriptName"
    $ScriptName 1> /dev/null 2>&1
    result=$?
    if [ $result -ne 0 ]
    then
        log_message "Post-failover script $ScriptName returned error code $result; exiting failover sequence."
        /bin/ls -l $ScriptName | /bin/awk '{print $1}' | /bin/grep "x" 1> /dev/null 2>&1
        if [ $? -ne 0 ]
        then
            log_message "NOTE: your script $ScriptName does not seem to have the Executable permission."
        fi
        if [ $AIXBootDriveMigration -eq 1 ]
        then
            log_message "Deleting the $DtcLibPath/SFTKdtc_services_disabled flag file so that the PMD can be launched again."
            /usr/bin/rm $DtcLibPath/SFTKdtc_services_disabled
            log_message "IMPORTANT: after fixing the error situation with the script $ScriptName, you must rerun dtc-target-netinfo"
            log_message "and launchpmds for this group to go back to Normal mode before relaunching dtcfailover."
        fi
        exit 1
    fi
}


#-------------------------------------------------------------------------------
##### FUNCTION TO EXECUTE THE TARGET SERVER POST-FAILOVER SCRIPT #####
# Arguments:
# $1: group number
# Return: no return if failure, otherwise it means status OK
# <<< TODO: test a script that must perform operations from a user ID other than root
RunTargetPostFailoverScript() {

    group=$1
    FormatGroupNumberSubstring $group

    ScriptName="$DtcLibPath/dtc_post_failover_s$GroupNumberSubstring.sh"
    log_message "Launching target server post-failover script $ScriptName"
    $ScriptName 1> /dev/null 2>&1
    result=$?
    if [ $result -ne 0 ]
    then
        log_message "Post-failover script $ScriptName returned error code $result; exiting failover sequence."
        /bin/ls -l $ScriptName | /bin/awk '{print $1}' | /bin/grep "x" 1> /dev/null 2>&1
        if [ $? -ne 0 ]
        then
            log_message "NOTE: your script $ScriptName does not seem to have the Executable permission."
        fi
        if [ $AIXBootDriveMigration -eq 1 ]
        then
            log_message "Deleting the $DtcLibPath/s$GroupNumberSubstring.off flag file so that the PMD-RMD of this group can be launched again."
            /usr/bin/rm $DtcLibPath/s$GroupNumberSubstring.off
            log_message "IMPORTANT: after fixing the error situation with the script $ScriptName, you must manually delete the $DtcLibPath/SFTKdtc_services_disabled flag file on the source server."
            log_message "Then you must rerun dtc-target-netinfo and launchpmds for this group to go back to Normal mode before relaunching dtcfailover."
        fi

        exit 1
        # <<< TODO: must return failure status to source or DMC?
    fi

}



#-------------------------------------------------------------------------------
##### FUNCTION TO PROMPT THE USER FOR CONFIRMATION THAT APPLICATIONS ARE STOPPED #####
#####                     AND SOURCE DEVICES UNMOUNTED                           #####
ConfirmApplicationsAreStoppedAndDevicesUnmounted() {

    # If Verbose is on and NoPrompt is false, prompt the user to confirm
    # that the applications have been stopped and source devices unmounted.
    # Else, we assume that the command comes from the DMC and that the DMC has prompted the user,
    # or we are running in a mode where answering a prompt is not possible (such as automated
    # tests).
    if [ $Verbose -ne 0 ]
    then
        if [ $NoPrompt -eq 0 ]
        then
            answer="n"
            /bin/echo " "
            /bin/echo "    Have the applications using this group's devices been stopped and the devices unmounted [y/n]? "
            read answer
            if [ $answer = "y" ]
            then
                /bin/echo "Continuing..."
            else
                log_message "Applications using this group's devices must be stopped and the devices unmounted prior to failover. Exiting."
                exit 1
            fi
        fi
    fi

}


#-------------------------------------------------------------------------------
##### FUNCTION TO DETERMINE THE NUMBER OF SOURCE DEVICES IN THE SPECIFIED GROUP #####
GetNumberOfDevicesInGroup() {
# Arguments:
# $1: group number

    group=$1
    NumberOfDevices=`$DtcBinPath/dtcinfo -g$group -q 2>/dev/null | /bin/grep "Local disk name" | /usr/bin/wc -l | /bin/awk '{print $1}'`

}

#-------------------------------------------------------------------------------
##### FUNCTION TO WAIT FOR I/O TO QUIESCE ON DEVICES OF THE SPECIFIED GROUP #####
# Arguments:
# $1: group number
# $2: maximum wait time before timeout
# $3: minimum time without any I/O change to declare success
WaitForIOtoQuiesce() {

    group=$1
    maxwaitseconds=$2
    minIOnochangeseconds=$3

    waited=0
    noIOchangeseconds=0

    device=0

    # Get the initial I/O counters for all devices of the group
    for IOcount in `$DtcBinPath/dtcinfo -g$group -q 2> /dev/null | /bin/grep "Write I/O count" | /bin/awk '{ print $4 }'`
    do
        eval PreviousNumberOfIOs$device=$IOcount
        device=`expr $device + 1`
    done

    log_message "Waiting for I/O to quiesce on source devices..."

    next_message_time=5
    /bin/sleep 1
    while [ $waited -lt $maxwaitseconds ]
    do
        IOchanged=0
        device=0
        do_sleep=1
        for IOcount in `$DtcBinPath/dtcinfo -g$group -q 2> /dev/null | /bin/grep "Write I/O count" | /bin/awk '{ print $4 }'`
        do
            if eval [ \$PreviousNumberOfIOs$device -ne $IOcount ]
            then
                IOchanged=1
                noIOchangeseconds=0
            fi
            eval PreviousNumberOfIOs$device=$IOcount
            device=`expr $device + 1`
        done
        if [ $IOchanged -eq 0 ]
        then
            # No IO count change for any device since 1 second; increment counter of time without I/O change
            noIOchangeseconds=`expr $noIOchangeseconds + 1`
            if [ $noIOchangeseconds -eq $minIOnochangeseconds ]
            then
                # No IO count change for the minimum wait time; stop looping
                waited=$maxwaitseconds
                log_message "No I/O occured on this group's devices for the specified time of $minIOnochangeseconds seconds. Proceeding..."
                do_sleep=0
            else
                # We have seen cases (such as on Linux) where buffering of IOs may cause that we see no IO
                # for a long time because they are buffered; call sync in case we would have that situation
                # WR PROD10529
                GetCurrentTimeSeconds $Platform
                time_before_sync_in_seconds=$CurrentTimeSeconds
                $SyncCmd
                GetCurrentTimeSeconds $Platform
                time_after_sync_in_seconds=$CurrentTimeSeconds
                # On HP-UX we have seen a problem where a non-numeric character is sometimes returned in the time value (defect 70040)
                # and this causes the time check below to fail. On HP, just sleep 1 second after sync and increment time_waited by 1.
                if [ $Platform = "HP-UX" ]
                then
                    do_sleep=1
                else
                    sync_time=`expr $time_after_sync_in_seconds - $time_before_sync_in_seconds`
                    if [ $sync_time -gt 0 ]
                    then
                        waited=`expr $waited + $sync_time`
                        do_sleep=0
                    fi
                fi
            fi
        else
            # At least one device still has I/Os; reset the no-change counter to 0
            noIOchangeseconds=0
            fivesecondsmodulo=`expr $waited % 5`
            if [ $waited -ge $next_message_time ]
            then
                log_message "I/Os still being detected after $waited seconds... Still waiting (max wait is $maxwaitseconds seconds)..."
                next_message_time=`expr $next_message_time + 5`
            fi
            do_sleep=1
        fi
        if [ $do_sleep -eq 1 ]
        then
            waited=`expr $waited + 1`
            /bin/sleep 1
        fi
    done
    if [ $IOchanged -ne 0 -o $noIOchangeseconds -lt $minIOnochangeseconds ]
    then
        log_message "Maximum wait time ($maxwaitseconds seconds) for source device I/Os to quiesce expired. Exiting failover sequence."
        exit 1
    fi

}


#-------------------------------------------------------------------------------
##### FUNCTION TO WAIT FOR BAB TO EMPTY FOR THE SPECIFIED GROUP #####
# Arguments:
# $1: group number
# $2: maximum wait time before timeout on BAB not emptying
# $3: minimum time without any BAB entry for the group to declare success
WaitForBABtoEmpty() {

    group=$1
    maxwaitseconds=$2
    minBABemptyseconds=$3

    waited=0
    BABemptyseconds=0

    log_message "Waiting for BAB to empty for group $group..."

    /bin/sleep 1
    while [ $waited -lt $maxwaitseconds ]
    do
        BABhasEntries=0
        for BABentriesCount in `$DtcBinPath/dtcinfo -g$group -q 2> /dev/null | /bin/grep "Entries in the BAB" | /bin/awk '{ print $5 }'`
        do
            if [ $BABentriesCount -ne 0 ]
            then
                BABhasEntries=1
                BABemptyseconds=0
            fi
        done
        if [ $BABhasEntries -eq 0 ]
        then
            # No BAB entry for the group since 1 second; increment counter of time without BAB entry
            BABemptyseconds=`expr $BABemptyseconds + 1`
            if [ $BABemptyseconds -eq $minBABemptyseconds ]
            then
                # No BAB entry for the minimum wait time; stop looping
                waited=$maxwaitseconds
                log_message "No BAB entry for this group for the specified time of $minBABemptyseconds seconds. Proceeding..."
            else
                waited=`expr $waited + 1`
            fi
        else
            # Still BAB entries for this group; reset the BAB-empty counter to 0
            BABemptyseconds=0
            waited=`expr $waited + 1`
            fivesecondsmodulo=`expr $waited % 5`
            if [ $fivesecondsmodulo -eq 0 ]
            then
                log_message "There are still BAB entries for group $group after $waited seconds... Still waiting (max wait is $maxwaitseconds seconds)..."
            fi
        fi
        /bin/sleep 1
    done
    if [ $BABhasEntries -ne 0 ]
    then
        log_message "Maximum wait time ($maxwaitseconds seconds) for BAB to empty for this group expired. Exiting failover sequence."
        exit 1
    fi

}


#-------------------------------------------------------------------------------
##### FUNCTION TO FORMAT THE GROUP NUMBER 3-DIGIT SUFFIX OF DAEMON NAME #####
# Arguments:
# $1: group number
FormatGroupNumberSubstring() {

    group=$1
    if [ $group -gt 99 ]
    then
        GroupNumberSubstring="$group"
    else
        if [ $group -gt 9 ]
        then
            GroupNumberSubstring="0$group"
        else
            GroupNumberSubstring="00$group"
        fi
    fi

}


#-------------------------------------------------------------------------------
##### FUNCTION TO ENSURE JOURNAL APPLICATION FOR THE SPECIFIED GROUP #####
# Arguments:
# $1: group number
# Return: no return if failure; otherwise: success.
EnsureJournalApplication() {

    group=$1

    # Call dtcrmdreco to make sure all journals are applied.
    # NOTE: this part is done whether Journals are ON or OFF. If they are OFF, the RMDA launched
    # by dtcrmdreco should not find any journal to apply. If it ever happened that, while in Tracking
    # when there were journals not applied and the user has changed the Journals to be OFF before
    # restarting the PMD/RMD, the RMD refuses going into Journal-less mode and applies the remaining
    # journals (see the error message: CANNOT_JLESS  Cannot activate Journal-Less mode until journals
    # are done being applied. Retry later).

    log_message "Calling $DtcBinPath/%Q%rmdreco to apply all Journals for group $group..."

    $DtcBinPath/%Q%rmdreco -g$group 1>/dev/null 2>&1
    result=$?
    if [ $result -ne 0 ]
    then
        log_message "$DtcBinPath/%Q%rmdreco reported error status $result; exiting failover sequence."
        exit 1
    fi

}



#-------------------------------------------------------------------------------
##### FUNCTION TO DETERMINE THE TYPE OF CONFIGURATION ON THIS SERVER #####
# Arguments:
# $1: group number
GetConfigurationType() {

    # Determine if we are on a loopback config or a 1-to-1 config;
    # if 1-to-1, determine if we are the PMD or the RMD side.

    group=$1
    LoopbackConfig=0
    OneToOnePMDside=0
    OneToOneRMDside=0
    FoundConfiguration=0

    # Format the 3-digit group number for PMD_ and RMD_ strings and s___.cfg string.
    FormatGroupNumberSubstring $GroupNumber

    # If we are an RMD in a 1-to-1 configuration, the option -r must have been
    # provided with the failover command, either by the DMC or the target 
    # dtc Master Daemon, because we cannot rely on the RMD being running or not.
    # But we must check if the command with -r has been given manually on CLI on
    # the PMD side of a 1-to-1 configuration. Check if the RMD config file is absent.
    if [ $SetOneToOneRMD -eq 1 ]
    then
        /bin/ls -l $DtcLibPath/s$GroupNumberSubstring.cfg 1> /dev/null 2>&1
        if [ $? -ne 0 ]
        then
            log_message "-r option has been given (valid on RMD side of configuration) but config file $DtcLibPath/s$GroupNumberSubstring.cfg does not exist. Exiting."
            exit 1    
        fi
        OneToOneRMDside=1
        FoundConfiguration=1
        log_message "Group $group is in a 1-to-1 configuration on this server, RMD side."
        return 0
    fi

    # Getting here means that we are on the Primary server; we can then be
    # in a loopback configuration or 1-to-1 PMD side. Since we are on the Primary server
    # it implies that we are starting the failover sequence, for which a prerequisite
    # is that the specified group be in Normal mode: therefore we can grep the process
    # names for PMD/RMD presence.

    # Check if PMD is there
    /bin/ps -ef | /bin/grep PMD_$GroupNumberSubstring | /bin/grep -v grep 1> /dev/null 2>&1
    result1=$?
    # Check if RMD is there
    /bin/ps -ef | /bin/grep RMD_$GroupNumberSubstring | /bin/grep -v grep 1> /dev/null 2>&1
    result2=$?

    if [ $result1 -eq 0 ]
    then
        if [ $result2 -eq 0 ]
        then
            # Found both the PMD and RMD: loopback config
            LoopbackConfig=1
            FoundConfiguration=1
            log_message "Group $group is in a loopback configuration on this server."
        else
            # Found only the PMD
            OneToOnePMDside=1
            FoundConfiguration=1
            log_message "Group $group is in a 1-to-1 configuration on this server, PMD side."
        fi
    else
        if [ $result2 -eq 0 ]
        then
            # Found only the RMD
            OneToOneRMDside=1
            FoundConfiguration=1
            log_message "Group $group is in a 1-to-1 configuration on this server, RMD side."
        fi
    fi

}


#------------------------------------------------------------------------------------------
##### FUNCTION TO CHECK THAT THE SOURCE DRIVE IS A LUN FOR LINUX BOOT DRIVE MIGRATION #####
# Arguments:
# $1: group number 3-digit substring
CheckMigratedDeviceDefinition() {

    group_substring=$1

    if [ $IsLinux -eq 1 ]
    then
        for device in `/bin/grep DATA-DISK $DtcLibPath/p$group_substring.cfg | /bin/grep -v "#" | /bin/awk '{print $2}'`
        do
            /bin/echo $device | /bin/grep "/dev/sd" 1> /dev/null 2>&1
            probable_SCSI_LUN=$?
            /bin/echo $device | /bin/grep "/dev/hd" 1> /dev/null 2>&1
            probable_IDE_LUN=$?
            if [ $probable_SCSI_LUN -eq 1 -a $probable_IDE_LUN -eq 1 ]
            then
                log_message "Source device $device is not accepted for Linux boot drive migration; accepted devices are LUNs /dev/hd[a to h] for IDE disks, /dev/sd[a to z] for SCSI disks."
                exit 1
            else
                found=0
                if [ $probable_SCSI_LUN -eq 0 ]
                then
                    device_letter=`/bin/echo $device | /bin/sed s,/dev/sd,,g`
                    for x in a b c d e f g h i j k l m n o p q r s t u v w x y z
                    do
                        if [ $device_letter = $x ]
                        then
                            found=1
                        fi
                    done
                fi
                if [ $probable_IDE_LUN -eq 0 ]
                then
                    device_letter=`/bin/echo $device | /bin/sed s,/dev/hd,,g`
                    for x in a b c d e f g h
                    do
                        if [ $device_letter = $x ]
                        then
                            found=1
                        fi
                    done
                fi
                if [ $found -eq 0 ]
                then
                    log_message "Source device $device is not accepted for Linux boot drive migration; accepted devices are LUNs /dev/hd[a to h] for IDE disks, /dev/sd[a to z] for SCSI disks."
                    exit 1
                fi
            fi
        done
    fi

    if [ $IsAIX -eq 1 ]
    then
        for device in `/bin/grep DATA-DISK $DtcLibPath/p$group_substring.cfg | /bin/grep -v "#" | /bin/awk '{print $2}'`
        do
            /bin/echo $device | /bin/grep "/dev/rhdisk" 1> /dev/null 2>&1
            RawHdiskDevice=$?
            /bin/echo $device | /bin/grep "/dev/hdisk" 1> /dev/null 2>&1
            HdiskDevice=$?
            if [ $RawHdiskDevice -eq 1 -a $HdiskDevice -eq 1 ]
            then
                log_message "Source device $device is not accepted for AIX boot drive migration; accepted devices are LUNs (hdiskx)."
                exit 1
            fi
        done
    fi
}

#-------------------------------------------------------------------------------
##### FUNCTION TO CALL THE LINUX BOOT DRIVE MIGRATION POST FAILOVER SCRIPT #####
# Arguments:
# $1: group number
RunLinuxBootDriveMigPostFailover() {

    group=$1

    log_message "Invoking Linux boot drive migration post failover script..."
    if [ $Verbose -eq 1 ]
    then
        /etc/opt/SFTKdtc/dtc_Linux_bootdrive_post_failover.sh -g$group -n
    else
        /etc/opt/SFTKdtc/dtc_Linux_bootdrive_post_failover.sh -g$group -n -q
    fi
    result=$?
    if [ $result -ne 0 ]
    then
        log_message "dtc_Linux_bootdrive_post_failover.sh reported error status $result."
        log_message "Please consult the message logs. Exiting..."
        exit 1
    fi
}

#-------------------------------------------------------------------------------
#####  FUNCTION TO CHECK PARTITIONS FOR SUSE LINUX BOOT DRIVE MIGRATION    #####
# Arguments: none
CheckSwapAndRootDefinitions() {

    # FIRST: check /etc/fstab
    # The format is the following (example with links to real partitions using by-id dynamic links):
    # /dev/disk/by-id/scsi-20010b9fc080ca25f-part1 swap                 swap       defaults              0 0
    # /dev/disk/by-id/scsi-20010b9fc080ca25f-part2 /                    ext3       acl,user_xattr        1 1

    # 1) Check if the /etc/fstab swap partition definition is a soft link
    check_dev_prefix=0
    swap_partition=`/bin/grep swap /etc/fstab | /bin/grep -v "#" | /bin/awk '{print $1}'`
    /bin/ls -l $swap_partition | /bin/grep ">" 1> /dev/null 2>&1
    result=$?
    if [ $result -eq 0 ]
    then
        # It is a soft link; if a dynamic device node, it might not get migrated.
        # Set the info in a file that will get migrated, for the target to check.
        real_swap_partition=`/bin/ls -l $swap_partition | /usr/bin/cut -d '>' -f2 | /bin/awk '{print $1}'`
        # Eliminate any "../" in the partition path
        /bin/echo $real_swap_partition | /bin/grep "../" 1> /dev/null 2>&1
        backward_path=$?
        while [ $backward_path -eq 0 ]
        do
            check_dev_prefix=1
            real_swap_partition=`/bin/echo $real_swap_partition | /bin/sed s,../,,g | /bin/awk '{print $1}'`
            /bin/echo $real_swap_partition | /bin/grep "../" 1> /dev/null 2>&1
            backward_path=$?
        done
        if [ $check_dev_prefix -eq 1 ]
        then
            # We had a relative path; verify that "/dev/" is in the final path
            /bin/echo $real_swap_partition | /bin/grep "/dev/" 1> /dev/null 2>&1
            if [ $? -ne 0 ]
            then
                real_swap_partition="/dev/"$real_swap_partition
            fi
        fi
        /bin/echo "swap $swap_partition $real_swap_partition" > /boot/SFTKdtc_check_fstab_swap_root_definitions
        $SyncCmd
    fi

    # 2) Verify for the fstab root partition definition now
    check_dev_prefix=0
    root_partition=`/bin/grep " / " /etc/fstab | /bin/grep -v "#" | /bin/awk '{print $1}'`
    /bin/ls -l $root_partition | /bin/grep ">" 1> /dev/null 2>&1
    result=$?
    if [ $result -eq 0 ]
    then
        # It is a soft link.
        real_root_partition=`/bin/ls -l $root_partition | /usr/bin/cut -d '>' -f2 | /bin/awk '{print $1}'`
        # Eliminate any "../" in the partition path
        /bin/echo $real_root_partition | /bin/grep "../" 1> /dev/null 2>&1
        backward_path=$?
        while [ $backward_path -eq 0 ]
        do
            check_dev_prefix=1
            real_root_partition=`/bin/echo $real_root_partition | /bin/sed s,../,,g | /bin/awk '{print $1}'`
            /bin/echo $real_root_partition | /bin/grep "../" 1> /dev/null 2>&1
            backward_path=$?
        done
        if [ $check_dev_prefix -eq 1 ]
        then
            # We had a relative path; verify that "/dev/" is in the final path
            /bin/echo $real_root_partition | /bin/grep "/dev/" 1> /dev/null 2>&1
            if [ $? -ne 0 ]
            then
                real_root_partition="/dev/"$real_root_partition
            fi
        fi
        /bin/echo "root $root_partition $real_root_partition" >> /boot/SFTKdtc_check_fstab_swap_root_definitions
    fi

    # SECOND: check the boot loader config file: menu.lst
    # The format is the following (example with links to real partitions using by-id dynamic links); note: this is a single line:
    # kernel /boot/vmlinuz-2.6.32.12-0.7-default root=/dev/disk/by-id/scsi-20010b9fc080ca25f-part2 console=ttyS0
    # resume=/dev/disk/by-id/scsi-20010b9fc080ca25f-part1 splash=silent crashkernel= showopts
    # 1) Swap definition ("resume" keyword)
    # Find the first occurence of the swap "resume" keyword, using cut to separate fields delimited by "="
    /bin/grep " resume=" /boot/grub/menu.lst | /bin/grep -v "#" 1> /dev/null 2>&1
    result=$?
    if [ $result -ne 0 ]
    then
        log_message "WARNING: cannot identify the swap specification in the bootloader cfg file menu.lst."
    else
        field_number=1
        field_found=0
        while [ $field_found -ne 1 ]
        do
            /bin/grep -m 1 " resume=" /boot/grub/menu.lst | /bin/grep -v "#" | /usr/bin/cut -d '=' -f $field_number | /bin/grep "resume" 1> /dev/null 2>&1
            result=$?
            if [ $result -ne 0 ]
            then
                field_number=`expr $field_number + 1`
            else
                field_found=1
            fi
        done
    fi
    check_dev_prefix=0
    # We found the field number at which the keyword appears; the value associated to it is the next field
    field_number=`expr $field_number + 1`
    swap_partition=`/bin/grep -m 1 " resume=" /boot/grub/menu.lst | /bin/grep -v "#"| /usr/bin/cut -d '=' -f $field_number | /bin/awk '{print $1}'`
    /bin/ls -l $swap_partition | /bin/grep ">" 1> /dev/null 2>&1
    result=$?
    if [ $result -eq 0 ]
    then
        # It is a soft link; set the info in a file that will get migrated, for the target to check.
        real_swap_partition=`/bin/ls -l $swap_partition | /usr/bin/cut -d '>' -f2 | /bin/awk '{print $1}'`
        # Eliminate any "../" in the partition path
        /bin/echo $real_swap_partition | /bin/grep "../" 1> /dev/null 2>&1
        backward_path=$?
        while [ $backward_path -eq 0 ]
        do
            check_dev_prefix=1
            real_swap_partition=`/bin/echo $real_swap_partition | /bin/sed s,../,,g | /bin/awk '{print $1}'`
            /bin/echo $real_swap_partition | /bin/grep "../" 1> /dev/null 2>&1
            backward_path=$?
        done
        if [ $check_dev_prefix -eq 1 ]
        then
            # We had a relative path; verify that "/dev/" is in the final path
            /bin/echo $real_swap_partition | /bin/grep "/dev/" 1> /dev/null 2>&1
            if [ $? -ne 0 ]
            then
                real_swap_partition="/dev/"$real_swap_partition
            fi
        fi
        /bin/echo "swap $swap_partition $real_swap_partition" > /boot/SFTKdtc_check_menu_lst_swap_root_definitions
    fi
    # 2) Root definition ("root" keyword)
    # Find the first occurence of the "root" keyword, using cut to separate fields delimited by "="
    /bin/grep -m 1 " root=" /boot/grub/menu.lst | /bin/grep -v "#" 1> /dev/null 2>&1
    result=$?
    if [ $result -ne 0 ]
    then
        log_message "WARNING: cannot identify the root specification in the bootloader cfg file menu.lst."
    else
        field_number=1
        field_found=0
        while [ $field_found -ne 1 ]
        do
            /bin/grep -m 1 " root=" /boot/grub/menu.lst | /bin/grep -v "#" | /usr/bin/cut -d '=' -f $field_number | /bin/grep "root" 1> /dev/null 2>&1
            result=$?
            if [ $result -ne 0 ]
            then
                field_number=`expr $field_number + 1`
            else
                field_found=1
            fi
        done
    fi
    # We found the field number at which the keyword appears; the value associated to it is the next field
    field_number=`expr $field_number + 1`
    check_dev_prefix=0
    root_partition=`/bin/grep -m 1 " root=" /boot/grub/menu.lst | /bin/grep -v "#" | /usr/bin/cut -d '=' -f $field_number | /bin/awk '{print $1}'`
    /bin/ls -l $root_partition | /bin/grep ">" 1> /dev/null 2>&1
    result=$?
    if [ $result -eq 0 ]
    then
        # It is a soft link; set the info in a file that will get migrated, for the target to check.
        real_root_partition=`/bin/ls -l $root_partition | /usr/bin/cut -d '>' -f2 | /bin/awk '{print $1}'`
        # Eliminate any "../" in the partition path
        /bin/echo $real_root_partition | /bin/grep "../" 1> /dev/null 2>&1
        backward_path=$?
        while [ $backward_path -eq 0 ]
        do
            check_dev_prefix=1
            real_root_partition=`/bin/echo $real_root_partition | /bin/sed s,../,,g | /bin/awk '{print $1}'`
            /bin/echo $real_root_partition | /bin/grep "../" 1> /dev/null 2>&1
            backward_path=$?
        done
        if [ $check_dev_prefix -eq 1 ]
        then
            # We had a relative path; verify that "/dev/" is in the final path
            /bin/echo $real_root_partition | /bin/grep "/dev/" 1> /dev/null 2>&1
            if [ $? -ne 0 ]
            then
                real_root_partition="/dev/"$real_root_partition
            fi
        fi
        /bin/echo "root $root_partition $real_root_partition" >> /boot/SFTKdtc_check_menu_lst_swap_root_definitions
    fi
}


##### FUNCTION TO CHECK THE NUMBER OF DEVICE PAIRS IN THE CONFIGURATION #####
# Arguments:
# $1: group number
CheckNumberOfDevicePairs() {

    group=$1
    # Format the 3-digit group number for PMD config file name
    FormatGroupNumberSubstring $group

    PMDfilename="$DtcLibPath/p$GroupNumberSubstring.cfg"
    # If there is more than one pair of devices, log a warning that first pair is expected to be the bootable one
    numofpairs=`/bin/grep DATA-DISK $PMDfilename | /bin/grep -v "#" | /usr/bin/wc -l | /bin/awk '{print $1}'`
    if [ $numofpairs -gt 1 ]
    then
        log_message "WARNING: the configuration group has more than 1 pair of devices; it is expected that pair 1 contains the boot partition and OS. Proceeding..."
    fi

}


##### FUNCTION TO CHANGE THE AIX BOOTLIST, TO FAILOVER TO REPLICATED ROOT DRIVE #####
# Arguments:
# $1: group number
ChangeBootList()
{
    group=$1
    # Format the 3-digit group number for the RMD config file name
    FormatGroupNumberSubstring $group

    RMDfilename="$DtcLibPath/s$GroupNumberSubstring.cfg"
    # Find the first mirror hdisk definition in the RMD config file
    found_device=0
    for device in `/bin/grep MIRROR-DISK $RMDfilename | /bin/grep -v "#" | /bin/awk '{print $2}'`
    do
        /bin/echo $device | /bin/grep "/dev/rhdisk" 1> /dev/null 2>&1
        RawHdiskDevice=$?
        if [ $RawHdiskDevice -eq 0 ]
        then
            device_number=`/bin/echo $device | /bin/sed s,/dev/rhdisk,,g`
            found_device=1
            break # ---> exit the for loop
        fi
        /bin/echo $device | /bin/grep "/dev/hdisk" 1> /dev/null 2>&1
        HdiskDevice=$?
        if [ $HdiskDevice -eq 0 ]
        then
            device_number=`/bin/echo $device | /bin/sed s,/dev/hdisk,,g`
            found_device=1
            break # ---> exit the for loop
        fi
    done
    if [ $found_device -eq 0 ]
    then
        log_message "ERROR: did not find the hdisk device to failover to. Please verify the group configuration."
        log_message "Deleting the $DtcLibPath/s$GroupNumberSubstring.off flag file so that the PMD-RMD of this group can be launched again."
        /usr/bin/rm $DtcLibPath/s$GroupNumberSubstring.off
        log_message "IMPORTANT: after fixing the error situation, you must manually delete the $DtcLibPath/SFTKdtc_services_disabled flag file on the source server."
        log_message "Then you must rerun dtc-target-netinfo and launchpmds for this group to go back to Normal mode before relaunching dtcfailover."
        return 1
    fi
    NewBootDrive="hdisk$device_number"

    # Take note of the current boot drive, in case an error would occur and we must restore
    OriginalBootDrive=`/usr/bin/bootlist -m normal -o | /usr/bin/awk '{print $1}'`
    log_message "Original boot drive: $OriginalBootDrive. New boot drive: $NewBootDrive"

    # Change the bootlist for failover to the migrated drive
    /usr/bin/bootlist -m normal "$NewBootDrive"

    # Verify that there is indeed a boot logical volume on this drive
    /usr/bin/bootlist -m normal -o | /bin/grep "blv=" 1> /dev/null 2>&1
    FoundBootLogicalVol=$?
    if [ $FoundBootLogicalVol -ne 0 ]
    then
        log_message "ERROR: the replicated boot drive $NewBootDrive does not seem to contain a boot logical volume."
        log_message "       Please verify PMD-RMD configurations, bootlist, and error logs in /var/dtc/dtcerror.log."
        # Restore the original bootlist
        log_message "Restoring the original bootlist."
        /usr/bin/bootlist -m normal "$OriginalBootDrive"
        log_message "Deleting the $DtcLibPath/s$GroupNumberSubstring.off flag file so that the PMD-RMD of this group can be launched again."
        /usr/bin/rm $DtcLibPath/s$GroupNumberSubstring.off
        log_message "IMPORTANT: after fixing the error situation, you must manually delete the $DtcLibPath/SFTKdtc_services_disabled flag file on the source server."
        log_message "Then you must rerun dtc-target-netinfo and launchpmds for this group to go back to Normal mode before relaunching dtcfailover."
        return 1
    else
        log_message "The bootlist has been changed for booting from drive $NewBootDrive."
        return 0
    fi
}


#----------------------------------
copy_file_no_exit()
{
    /usr/bin/cp -pf "$1" "$2"
	cp_result=$?
	if [ cp_result -ne 0 ]
    then
     log_message "Copying $1 to $2 failed.";
	  log_message "This error is most likely caused by directory permissions"
	  log_message "or being out of space on the filesystem assoicated with the"
	  log_message "/etc directory which is normally the root filesystem."
	  sync
	  return 1
    fi

    if ! cmp -s "$1" "$2"
    then
	  log_message "Comparing $1 to $2 after copying failed."
	  log_message "This error is most likely caused by being out of space on"
	  log_message "the filesystem the /etc directory is associated with which"
	  log_message "is normally the root filesystem."
	  sync
	  return 1
    fi

    return 0
}


#--------------------------------------------
# rc.tcpip flag file cleanup and file restore
CleanupRcTcpipFlagAndRestoreFile()
{

    log_message "Deleting the SFTKdtc_call_mktcpip_at_boot flag file and restoring original rc.tcpip."
    /usr/bin/rm -f /etc/SFTKdtc_call_mktcpip_at_boot

    # Confirm that the current rc.tcpip has been modified by us (in case of some manual intervention that would have changed it)
    /usr/bin/grep SFTKdtc_AIX_failover_boot_network_reconfig /etc/rc.tcpip 1> /dev/null 2>&1
    result=$?
    if [ $result -eq 0 ]
    then
      # The current rc.tcpip is the one we have modified; we must remove our changes.
      # If an error occurs while restoring, we keep the modified file, since it will not do any harm
      # now that the flag /etc/SFTKdtc_call_mktcpip_at_boot has been removed.
      # NOTE: we do not simply copy rc.tcpip.preSFTKdtc over the file in case the active rc.tcpip has been
      #       modified for something else.
      copy_file_no_exit "/etc/rc.tcpip" "/etc/rc.tcpip.postSFTKdtc"
      result=$?
      if [ $result -eq 0 ]
      then
          # Copy successful, remove current rc.tcpip that we have just backed up
          /usr/bin/rm -f /etc/rc.tcpip
          # Remove the lines we have inserted in the file for failover boot script call and save into new rc.tcpip  
          /usr/bin/sed -e '/@@@ CALL TO SFTKdtc FAILOVER NETWORK RECONFIGURATION SCRIPT + @@@/,/@@@ CALL TO SFTKdtc FAILOVER NETWORK RECONFIGURATION SCRIPT - @@@/d' \
          /etc/rc.tcpip.postSFTKdtc > /etc/rc.tcpip;

          # Check the resulting rc.tcpip file
          numrctcpiplines=`/usr/bin/wc -l /etc/rc.tcpip | /usr/bin/awk {'print $1'}`
          if [ $numrctcpiplines -eq 0 ] 
          then
            # The rc.tcpip file has not been properly saved. Restore the modified file.
            /usr/bin/rm -f /etc/rc.tcpip 1> /dev/null 2>&1
	        /usr/bin/mv /etc/rc.tcpip.postSFTKdtc /etc/rc.tcpip
            log_message "Undoing /etc/rc.tcpip changes failed (filesystem full?); keeping the current /etc/rc.tcpip file."
            log_message "Please check if the filesystem is full and manually edit the file to remove the paragraph delimited by CALL TO SFTKdtc FAILOVER NETWORK RECONFIGURATION SCRIPT lines."
          else
            # Success
            log_message "The call to SFKTdtc failover network reconfiguration script was successfully removed from rc.tcpip";
            # Cleanup backup file
            /usr/bin/rm -f /etc/rc.tcpip.postSFTKdtc
	      fi
      else
          log_message "Keeping the modified rc.tcpip but this will not cause problems since the flag file SFTKdtc_call_mktcpip_at_boot has been removed."
      fi

    fi
    # Whatever manipulation has been done on rc.tcpip, make sure the final script is executable
    /usr/bin/chmod 774 /etc/rc.tcpip
}

########################################################################################
################################ MAIN SCRIPT BODY ######################################
########################################################################################

########################################################################################
#                   SECTION 1 OF 3 OF THE SCRIPT: INITIALIZATIONS
########################################################################################
LANG=C
export LANG

# Set Verbose to false (no screen output); its real value will be defined in parse_command_arguments;
# we set it to false in case log_message would be called before command arguments are parsed.
Verbose=0

# Set platform dependent variables
set_platform_specifics

# Check if not enough arguments have been provided to the script.
# If so, print usage paragraph.
if [ $# -lt 1 ]
then
    show_usage
    exit 1
fi

# Parse the arguments provided to the script
# This function should be called before anything uses log_message
CommandArguments=$@
parse_command_arguments $@

# Check if we are root
if [ "$COMMAND_USR" != "root" ]; then
	log_message "You must be root to run this process...aborted."
	exit 1
fi

#----- Determine if we are on a loopback config or a 1-to-1 config;
# if 1-to-1, determine if we are the PMD or the RMD side.
GetConfigurationType $GroupNumber
if [ $FoundConfiguration -eq 0 ]
then
    log_message "Neither the PMD nor the RMD of group $GroupNumber is running on this server. Exiting failover sequence."
    exit 1
fi

#----- If this is an AIX root drive failover, it cannot be loopback (not supported)
if [ $LoopbackConfig -eq 1 -a $AIXBootDriveMigration -eq 1 ]
then
    log_message "AIX root drive migration and failover is not supported in a loopback configuration. Exiting."
    exit 1
fi

#----- Format the 3-digit group number for PMD_ and RMD_ pre/post failover script strings. -----
FormatGroupNumberSubstring $GroupNumber

#--------------- Identify pre/post failover scripts ready for this group, if any
VerifyPrePostFailoverScripts $GroupNumberSubstring


########################################################################################
#         SECTION 2 OF 3 OF THE SCRIPT: 1-TO-1 CONFIG TARGET SERVER PART
########################################################################################
# Here we do only the target server part (RMD side on a 1-to-1 configuration) and exit.
if [ $OneToOneRMDside -eq 1 ]
then
    log_message "Target Server part of Failover started for group $GroupNumber; command arguments: $CommandArguments."

    # If this is a Linux boot drive migration, the source server does a shutdown (if -s option is given) so that
    # our scripts be run to terminate all activity. Wait here to give it time to do so.
    if [ $LinuxBootDriveMigration -eq 1 -a $KeepRunning -eq 0 ]
    then
        log_message "Linux root drive failover. Waiting 30 seconds to allow source server shutdown scripts to run..."
        /bin/sleep 30
    fi

    # Ensure journal application completion.
    # Note: if EnsureJournalApplication fails, it exits;
    # if it returns, it means status OK and we can continue.
    EnsureJournalApplication $GroupNumber

    # Run the Target server post-failover script if applicable;
    if [ $TargetPostFailoverReady -ne 0 ]
    then
        # Note: if RunTargetPostFailoverScript fails, it exits;
        # if it returns, it means status OK.
        # <<< TODO: check error status return.
        # <<< TODO: hum... should we undo dtcrmdreco if error???
        RunTargetPostFailoverScript $GroupNumber
    fi

#----- If AIX boot drive failover, change the bootlist to boot from the replicated drive
    if [ $AIXBootDriveMigration -eq 1 ]
    then
        # Failover boot from migrated boot drive unless the option was specified not to reboot automatically
        if [ $KeepAIXTargetRunning -eq 0 ]
        then
            ChangeBootList $GroupNumber
            result=$?
            if [ $result -ne 0 ]
            then
                log_message "An error occured while attempting to change the bootlist. Exiting."
                exit 1
            else
                log_message "Migration for failover completed. Will now reboot from migrated boot drive."
                $SyncCmd
                /usr/bin/sleep 5
                /usr/sbin/shutdown -r now  # <------------ DONE
            fi
        else
            log_message "Migration for failover completed. No automatic reboot of the target server due to specification of -k option (Keep running)."
            log_message "When ready to reboot from the migrated boot drive, you will first have to change the boot list (bootlist -m normal <new boot drive>)."
        fi
        exit 0
    fi

    if [ $LinuxBootDriveMigration -eq 1 ]
    then
        # Note: if RunLinuxBootDriveMigPostFailover fails, it exits;
        # if it returns, it means status OK.
        RunLinuxBootDriveMigPostFailover $GroupNumber
    fi

    log_message "Migration for failover completed."
    exit 0  # <------------ DONE
fi



########################################################################################
#                   SECTION 3 OF 3 OF THE SCRIPT: SOURCE SERVER PART
########################################################################################
# Check if the specified group is started.
$DtcBinPath/dtcinfo -g$GroupNumber -q 1>/dev/null 2>&1
GroupNotStarted=$?
if [ $GroupNotStarted -ne 0 ]
then
	log_message "Group number $GroupNumber is not started. Exiting the failover sequence."
	exit 1
fi

#----- Verify that the group is in an accepted state for failover -----
CheckIfGroupStateOk $GroupNumber

#----- Format the 3-digit group number for PMD_ and RMD_ pre/post failover script strings. -----
FormatGroupNumberSubstring $GroupNumber

#----- For AIX root drive failover, check that dtc-target-netinfo has been run
# dtc-target-netinfo creates a network parameter file that must be replicated to
# the target server before failover; these parameters will allow the target host to
# reconfigure its own network settings upon failover to the replicated root drive.
if [ $AIXBootDriveMigration -eq 1 ]
then
    if ! [ -e /etc/SFTKdtc_rootvg_failover_network_parms.txt  ]
    then
        log_message "The failover network parameter file /etc/SFTKdtc_rootvg_failover_network_parms.txt was not found."
        log_message "You must run dtc-target-netinfo prior to failover, while the Replication group is in Normal mode. Exiting."
        exit 1
    fi
    # Warn the user that the target bootlist must ensure that the service (maintenance) mode boot drive is safe.
    log_message "Please make sure that the target server bootlist ensures that the service (maintenance) mode boot drive is safe"
    log_message "in case some error occured and booting the target in maintenance mode would be needed."
    if [ $Verbose -ne 0 -a $NoPrompt -eq 0 ]
    then
        /bin/echo "    Please type <Enter> to continue, or <Ctrl-C> to escape... \c"
        read answer
    fi
fi

#----- For Linux and AIX boot drive migration, check that we are migrating a LUN (/dev/sda or /dev/hda for instance, for Linux, /dev/rhdiskx for AIX) -----
# If a source device is not accepted, the next function will cause an exit.
if [ $BootDriveMigration -eq 1 ]
then
    CheckMigratedDeviceDefinition $GroupNumberSubstring
fi

log_message "Failover started for group $GroupNumber; command arguments: $CommandArguments."

# Log additional messages if this is a Linux root drive migration
if [ $BootDriveMigration -eq 1 ]
then
    log_message "Boot drive migration. Please make sure to look at the logs on the target server and/or DMC target server Events tab."
    if [ $LinuxBootDriveMigration -eq 1 ]
    then
        if [ $OneToOnePMDside -eq 1 -a $KeepRunning -eq 0 ]
        then
            log_message "WARNING: the source server will reboot at the end of its sequence."
            if [ $Verbose -ne 0 -a $NoPrompt -eq 0 ]
            then
                /bin/echo -ne "    Please type <Enter> to continue, or <Ctrl-C> to escape... \c"
                read answer
            fi
        fi
    fi
    # Log a warning if there are more than 1 device pairs in the group; pair 1 has to be the bootable one
    CheckNumberOfDevicePairs $GroupNumber
fi

#---------------
# Check if the pre-failover script has been prepared to run for this group;
# if so, run it.
if [ $SourcePreFailoverReady -eq 1 ]
then
    # Note: if RunSourcePreFailoverScript fails, it exits;
    # if it returns, it means status OK and we can continue.
    RunSourcePreFailoverScript $GroupNumber
fi

#----- Confirm that applications have been stopped and source devices unmounted, unless this is a boot drive failover -----
if [ $BootDriveMigration -eq 0 ]
then
    ConfirmApplicationsAreStoppedAndDevicesUnmounted
fi

#----- In the case of a SuSE Linux boot drive migration, check /etc/fstab and the boot loader config file -----
# Check if they are using redirections to swap and root partitions
if [ $LinuxBootDriveMigration -eq 1 ]
then
    if [ $IsRedHat -eq 0 ]
    then
        CheckSwapAndRootDefinitions
    fi
fi
#----- In the case of a boot drive migration, disable Replicator services for after failover -----
if [ $BootDriveMigration -eq 1 ]
then
    log_message "Creating marker file $DtcLibPath/SFTKdtc_services_disabled to deactivate SFTKdtc services after failover to new boot drive."
    /bin/touch $DtcLibPath/SFTKdtc_services_disabled
fi

#----- sync data to disk -----
$SyncCmd

#----- Wait for I/O to quiesce -----
if [ $BootDriveMigration -eq 0 ]
then
    WaitForIOtoQuiesce $GroupNumber $MaxWaitOnIOs $MinWaitNoIOs
fi

#------ Wait for BAB to empty -----
if [ $BootDriveMigration -eq 0 ]
then
    WaitForBABtoEmpty $GroupNumber $MaxWaitBABnotEmpty $MinWaitBABempty
fi

#----- Stop PMD and RMD if not Linux boot drive failover -----
# NOTE: we stop the PMDs/RMDs also in the case of an AIX boot drive failover,
#       and we add a message in our log file saying that this is the cut off point
#       in the log and that preceding messages are from the source server, because
#       the target will inherit the source logfile at failover and use it.
if [ $LinuxBootDriveMigration -eq 0 ]
then
    if [ $AIXBootDriveMigration -eq 1 ]
    then
        save_Verbose=$Verbose
        Verbose=0
        source_hostname=`/usr/bin/hostname`
        log_message "NOTE: preceding log entries are from the source server $source_hostname (message intended to target server to which this logfile is replicated)."
        Verbose=$save_Verbose
        $SyncCmd
        $SyncCmd
        sleep 3
    fi

    log_message "Stopping the PMD and RMD of group $GroupNumber..."
    $DtcBinPath/killpmds -g$GroupNumber
    result=$?
    if [ $result -ne 0 ]
    then
        log_message "Error reported when stopping PMD/RMD (status: $result); exiting failover sequence."
        CleanupRcTcpipFlagAndRestoreFile
        exit 1
    fi
fi

#----- If AIX boot drive failover, delete the SFTKdtc_call_mktcpip_at_boot flag file and restore rc.tcpip
if [ $AIXBootDriveMigration -eq 1 ]
then
    CleanupRcTcpipFlagAndRestoreFile
fi

#--------------- FINAL SEQUENCE OF THE FAILOVER ON THE SOURCE SERVER -----------
# If 1-to-1 configuration:
#     run the Source server post-failover script if applicable;
#     transfer the failover command for completion on the Target (RMD) side;
#     exit.
# If loopback configuration (single server):
#     ensure journal application completion;
#     run the Source server post-failover script if applicable;
#     run the Target server post-failover script if applicable;
#     exit.

#-------------- Case 1-to-1 PMD side --------------
if [ $OneToOnePMDside -eq 1 ]
then
    # Save the source server kervel version string in a file for the target to know it when preparing failover
    # in the case of Linux boot volume migration for failover.
    if [ $LinuxBootDriveMigration -eq 1 ]
    then
       /bin/echo $KernelVersion > /boot/SFTKdtc_source_kernel_version.txt
       $SyncCmd
       /bin/sleep 2
    fi

    # Run the Source server post-failover script if applicable.
    if [ $SourcePostFailoverReady -ne 0 ]
    then
        # Note: if RunSourcePostFailoverScript fails, it exits;
        # if it returns, it means status OK and we can continue.
        RunSourcePostFailoverScript $GroupNumber
        log_message "Source post-failover script for group $GroupNumber returned success status; continuing...."
    fi
    log_message "Transferring rest of failover sequence to the target server..."

    # Transfer the failover command for completion on the Target (RMD) side.
    if [ $BootDriveMigration -eq 0 ]
    then
        $DtcBinPath/dtcsendfailover -g$GroupNumber
    else
        if [ $LinuxBootDriveMigration -eq 1 ]
        then
            # In the case of the Linux boot drive migration, if specified with -s, we want the reboot of the source server to occur
            # now so that our shutdown scripts be run before the target server finishes the sequence. This is
            # why we call dtcsendfailover in the background. The target side of the failover sequence will allow some
            # time for the shutdown to complete before it invokes dtcrmdreco to finish journal application and completes
            # the sequence.
            # If the option was specified to shutdown the source server, forward it to dtcsendfailover.
            # NOTE: in the case of the AIX boot drive failover, there is no shutdown of the source server and KeepRunning is forced to 1.
            if [ $KeepRunning -eq 0 ]
            then
                $DtcBinPath/dtcsendfailover -g$GroupNumber -b -s &
            else
                $DtcBinPath/dtcsendfailover -g$GroupNumber -b
            fi
        fi
        if [ $AIXBootDriveMigration -eq 1 ]
        then
            # In the case of the AIX boot drive migration, if specified with -k, we do not want the reboot of the target server to occur
            # automatically.
            # If the option was specified to NOT shutdown the target server, forward it to dtcsendfailover.
            if [ $KeepAIXTargetRunning -eq 1 ]
            then
                $DtcBinPath/dtcsendfailover -g$GroupNumber -b -k
            else
                $DtcBinPath/dtcsendfailover -g$GroupNumber -b
            fi
        fi
    fi
    result=$?
    if [ $result -ne 0 ]
    then
        log_message "Failed transferring rest of failover sequence to the target server. Exiting."
        exit $result
    else
        log_message "Transferred rest of failover sequence to the target server."
        # <<< TODO: error reporting from target to complete <<<
    fi

    if [ $LinuxBootDriveMigration -eq 1 -a $KeepRunning -eq 0 ]
    then
        log_message "Shutting down and rebooting the source server..."
        $SyncCmd
        /bin/sleep 2
        /sbin/shutdown -r now
    fi
    
    exit $result #<------------------------  DONE  -------------------------
fi

#----- Last possible case: loopback configuration (single server) -----
# NOTE: this is not applicable to the AIX boot drive failover.
if [ $LoopbackConfig -eq 1 ]
then
    # Ensure journal application completion.
    # Note: if EnsureJournalApplication fails, it exits;
    # if it returns, it means status OK and we can continue.
    EnsureJournalApplication $GroupNumber

    # Run the Source server post-failover script if applicable;
    if [ $SourcePostFailoverReady -ne 0 ]
    then
        # Note: if RunSourcePostFailoverScript fails, it exits;
        # if it returns, it means status OK and we can continue.
        # <<< TODO: should we undo dtcrmdreco if error???
        RunSourcePostFailoverScript $GroupNumber
        log_message "Source post-failover script for group $GroupNumber returned success status; continuing...."
    fi

    # Run the Target server post-failover script if applicable;
    if [ $TargetPostFailoverReady -ne 0 ]
    then
        # Note: if RunTargetPostFailoverScript fails, it exits;
        # if it returns, it means status OK and we can continue.
        # <<< TODO: check error status return.
        # <<< TODO: should we undo dtcrmdreco if error???
        RunTargetPostFailoverScript $GroupNumber
    fi
    log_message "Migration for failover completed."

    exit 0   #<------------------------  DONE  -------------------------
fi




