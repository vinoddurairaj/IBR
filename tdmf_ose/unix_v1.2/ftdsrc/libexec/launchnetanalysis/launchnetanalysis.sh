#!/bin/sh
#
# /%OPTDIR%/%PKGNM%/bin/launchnetanalysis
#
# LICENSED MATERIALS / PROPERTY OF IBM
# %PRODUCTNAME% version %VERSION%
# (c) Copyright %COMPANYNAME%  %COPYRIGHTYEAR 2001%.  All Rights Reserved.
# The source code for this program is not published or otherwise
# divested of its trade secrets, irrespective of what has been
# deposited with the U.S. Copyright Office.
# US Government Users Restricted Rights - Use, duplication or
# disclosure restricted by GSA ADP Schedule Contract with %COMPANYNAME%
#
#
# This script launches the %Q%netanalysis binary, which will send a command to the master daemon in.dtc to put
# PMD/RMD pairs in a data transfer mode to collect network bandwidth statistics.
# NOTE: at this point, this can only be launched in loopback mode OR between 2 servers (not one source to several target servers).
#
##### FUNCTION TO SHOW THE COMMAND USAGE ####
show_usage() {

    echo "----------------------------------------------------------------------------------------------------------------------------"
    echo "USAGE:"
    echo "  launchnetanalysis -t <number of seconds> -p <Primary server hostname or IP> -s <Secondary server hostname or IP>"
    echo "                   [-n <number of PMD-RMD pairs>] [-c chunksize in bytes>] [-d <chunkdelay]> [-m <netmaxkbps>]"
    echo "                   [-i <statistics sampling interval>] [-q] [-h]"
    echo "----------------------------------------------------------------------------------------------------------------------------"
    echo "  This script launches the %Q%netanalysis binary, which will send a command to the master daemon in.dtc to put
    echo "  PMD/RMD pairs in a data transfer mode to collect network bandwidth statistics.
    echo "  Options: "
    echo "  -t: time duration in seconds for the PMD-RMD pairs to run (mandatory argument);"
    echo "  -p: Primary server hostname or IP address (mandatory argument);"
    echo "  -s: Secondary server hostname or IP address (mandatory argument);"
    echo "  -n: number of PMD-RMD pairs to launch (default is 1, maximum is 100);"
    echo "  -c: size of data chunks to transfer in KBytes (chunksize, default is 2048 Kbytes);"
    echo "  -d: delay between chunk transfers in milliseconds (chunkdelay, default is 0);"
    echo "  -m: maximimum KBytes per second to transfer over the network (netmaxkbps, default is no maximum);"
    echo "  -i: statistics sampling interval (default is 10 seconds);"
    echo "  -q: quiet mode; just log messages and results; do not print them on server console;"
    echo "  -h: display this usage paragraph;"
    echo "------------------------------------------------------------------------------------------------------------------------------"
}

#-------------------------------------------------------------------------------
##### FUNCTION TO LOG A MESSAGE AND DISPLAY IT IF VERBOSE IS SET ####
# Arguments:
# $1: message string
log_message() {

    message_string=$1
    datestr=`date "+%Y/%m/%d %T"`;
    errhdr="[${datestr}] launchnetanalysis: ";
    errmsg=`echo "${errhdr}" "$message_string"`
    echo ${errmsg} 1>> ${logf} 2>&1

    if [ $Verbose -gt 0 ]
    then
	    echo ${errmsg}
    fi
}

#-------------------------------------------------------------------------------
##### FUNCTION TO SET PLATFORM SPECIFIC VARIABLES ####
set_platform_specifics() {

    Platform=`/bin/uname`
    if [ $Platform = "HP-UX" ]; then
        logf=/var/opt/SFTK%Q%/%Q%error.log;
        DtcLibPath=/etc/opt/SFTK%Q%;
        DtcBinPath=/opt/SFTK%Q%/bin;
	    COMMAND_USR=`/usr/bin/whoami`

    elif [ $Platform = "SunOS" ]; then
        logf=/var/opt/SFTK%Q%/%Q%error.log;
        DtcLibPath=/etc/opt/SFTK%Q%;
        DtcBinPath=/opt/SFTK%Q%/bin;
	    if [ -x /usr/ucb/whoami ]; then COMMAND_USR=`/usr/ucb/whoami`; else COMMAND_USR=`/usr/bin/whoami`; fi

    elif [ $Platform = "AIX" ]; then
        logf=/var/%Q%/%Q%error.log;
        DtcLibPath=/etc/%Q%/lib;
        DtcBinPath=/usr/%Q%/bin;
	    COMMAND_USR=`/bin/whoami`

    elif [ $Platform = "Linux" ]; then
        logf=/var/opt/SFTK%Q%/%Q%error.log;
        DtcLibPath=/etc/opt/SFTK%Q%;
        DtcBinPath=/opt/SFTK%Q%/bin;
	    COMMAND_USR=`/usr/bin/whoami`
    fi
}

#-------------------------------------------------------------------------------
##### FUNCTION TO TO VERIFY THE PRIMARY SERVER SPECIFICATION #####
# Arguments:
# $1: primary server hostname or IP address
VerifyPrimaryServer() {

    server_id="$1"
     
    # Verify if the server definition exists in the local /etc/hosts file
    /bin/grep "$server_id" /etc/hosts | /bin/grep -v "#"  1>/dev/null 2>&1
    server_found=$?
    if [ $server_found -ne 0 ]
    then
        # Server not found in /etc/hosts; so it must not be an IP address that was given, or it is wrong;
        # or it may be because the fully qualified host name was given
        # but only the short name is in the hosts file; check that the /bin/hostname output is part
        # of the server specification.
        hostname_output=`/bin/hostname`
        echo "$server_id" | /bin/grep $hostname_output 1>/dev/null 2>&1
        server_found=$?
        if [ $server_found -ne 0 ]
        then
            log_message "The primary server specification ($server_id) does not seem to be present in /etc/hosts nor \
                         to match the output of the /bin/hostname command ($hostname_output)."
            log_message "Please check the source server specification, along with /etc/hosts and the output of /bin/hostname."
            exit 1
        fi
    fi
}    

#------------------------------------------------------------------------------------
##### FUNCTION TO VERIFY THAT A NUMERIC ARGUMENT DOES NOT EXCEED A 32-BIT VALUE #####
# Argument: $1 = numeric value to check; $2 = description of value
verify_32bit_value() {

    numeric_value=$1
    value_description="$2"
    platform_type=`/bin/uname`
    if [ $platform_type = "SunOS" ]
    then
        string_length=`/bin/echo "$numeric_value" | tr -d "\n" | wc -c`
    else
        string_length=`/bin/echo ${#numeric_value} | /bin/awk '{print $1}'`
    fi
    if [ $string_length -gt 10 ]
    then
        log_message "The $value_description exceeds the maximum number of digits for a 32-bit integer (10 digits)."
        show_usage
        exit 1
    fi

}



#-------------------------------------------------------------------------------
##### FUNCTION TO PARSE THE ARGUMENTS PROVIDED TO THIS SCRIPT #####
parse_command_arguments() {

    ShowUsage=0
    Verbose=1
    NumberOfSeconds=0
    PrimaryServer="Unknown"
    SecondaryServer="Unknown"
    NumberOfPMDRMDpairs=1
    ChunkSize=2048
    ChunkDelay=0
    NetMaxKBPS=-1
    StatInterval=10
    error=0

    toption=0
    poption=0
    soption=0
    noption=0
    coption=0
    doption=0
    moption=0
    ioption=0

    Max32bitPositive=2147483647 # Max positive 32-bit integer, to avoid script validation errors 

    while getopts "t:p:s:n:c:d:m:i:qh" opt
    do
            case $opt in
            t)
                    if [ $toption -eq 1 ]
                    then
                        log_message "Error: option t has been entered more than once."
                        exit 1
                    fi
                    toption=1
                    NumberOfSeconds=$OPTARG
                    verify_32bit_value $NumberOfSeconds "number of seconds";;
            p)
                    if [ $poption -eq 1 ]
                    then
                        log_message "Error: option p has been entered more than once."
                        exit 1
                    fi
                    poption=1
                    PrimaryServer=$OPTARG;;
            s)
                    if [ $soption -eq 1 ]
                    then
                        log_message "Error: option s has been entered more than once."
                        exit 1
                    fi
                    soption=1
                    SecondaryServer=$OPTARG;;
            n)
                    if [ $noption -eq 1 ]
                    then
                        log_message "Error: option n has been entered more than once."
                        exit 1
                    fi
                    noption=1
                    NumberOfPMDRMDpairs=$OPTARG
                    verify_32bit_value $NumberOfPMDRMDpairs "number of PMD-RMD pairs";;
            c)
                    if [ $coption -eq 1 ]
                    then
                        log_message "Error: option c has been entered more than once."
                        exit 1
                    fi
                    coption=1
                    ChunkSize=$OPTARG
                    verify_32bit_value $ChunkSize "chunk size";;
            d)
                    if [ $doption -eq 1 ]
                    then
                        log_message "Error: option d has been entered more than once."
                        exit 1
                    fi
                    doption=1
                    ChunkDelay=$OPTARG
                    verify_32bit_value $ChunkDelay "chunk delay";;
            m)
                    if [ $moption -eq 1 ]
                    then
                        log_message "Error: option m has been entered more than once."
                        exit 1
                    fi
                    moption=1
                    NetMaxKBPS=$OPTARG
                    verify_32bit_value $NetMaxKBPS  "NetMaxKBPS";;
            i)
                    if [ $ioption -eq 1 ]
                    then
                        log_message "Error: option i has been entered more than once."
                        exit 1
                    fi
                    ioption=1
                    StatInterval=$OPTARG
                    verify_32bit_value $StatInterval "StatInterval";;
            q)
                    Verbose=0;;
            h)
                    ShowUsage=1;;
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

    # Validate the arguments
    if echo $NumberOfSeconds | grep "[^0-9"] 2>&1 > /dev/null
    then
      log_message "The number of seconds is not a valid number."
      show_usage
      exit 1
    fi

    if [ $NumberOfSeconds -eq 0 ]
    then
      log_message "The number of seconds (-t) is mandatory."
      show_usage
      exit 1
    fi

    if [ $NumberOfSeconds -gt $Max32bitPositive ]
    then
      log_message "The number of seconds (-t) exceeds the maximum allowed value (positive 32-bit integer $Max32bitPositive)."
      show_usage
      exit 1
    fi

    if [ "$PrimaryServer" = "Unknown" ]
    then
      log_message "The Primary server hostname or IP address (-p) is mandatory."
      show_usage
      exit 1
    fi

    # Verify if the Primary server definition makes sense.
    VerifyPrimaryServer "$PrimaryServer"

    if [ "$SecondaryServer" = "Unknown" ]
    then
      log_message "The Secondary server hostname or IP address (-s) is mandatory."
      show_usage
      exit 1
    fi

    if echo $NumberOfPMDRMDpairs | grep "[^0-9"] 2>&1 > /dev/null
    then
      log_message "The number of PMD-RMD pairs is not a valid number (-n)."
      show_usage
      exit 1
    fi

    if [ $NumberOfPMDRMDpairs -gt 100 ]
    then
      log_message "The number of PMD-RMD pairs exceeds the maximum for Network analysis mode (max is 100)."
      show_usage
      exit 1
    fi

    if [ $NumberOfPMDRMDpairs -lt 1 ]
    then
      log_message "The number of PMD-RMD pairs (-n) must be between 1 and 100."
      show_usage
      exit 1
    fi

    if echo $ChunkSize | grep "[^0-9"] 2>&1 > /dev/null
    then
      log_message "The chunk size specified is not a valid number."
      show_usage
      exit 1
    fi

    if [ $ChunkSize -gt 16384 ]
    then
      log_message "The chunk size specified exceeds the maximum (max is 16384 KB)."
      show_usage
      exit 1
    fi

    if [ $ChunkSize -lt 64 ]
    then
      log_message "The chunk size specified is below the minimum (64 KB)."
      show_usage
      exit 1
    fi

    if echo $ChunkDelay | grep "[^0-9"] 2>&1 > /dev/null
    then
      log_message "The chunk delay specified is not a valid number."
      show_usage
      exit 1
    fi

    if [ $ChunkDelay -gt 999 ]
    then
      log_message "The chunk delay specified exceeds the maximum (max is 999)."
      show_usage
      exit 1
    fi

    if [ $NetMaxKBPS -ne -1 ]
    then
        if echo $NetMaxKBPS | grep "[^0-9"] 2>&1 > /dev/null
        then
          log_message "The chunk delay specified is not a valid number."
          show_usage
          exit 1
        fi
    fi

    if [ $NetMaxKBPS -gt $Max32bitPositive ]
    then
      log_message "The maximimum KBytes per second (-m) exceeds the maximum allowed value (positive 32-bit integer $Max32bitPositive)."
      show_usage
      exit 1
    fi

    if echo $StatInterval | grep "[^0-9"] 2>&1 > /dev/null
    then
      log_message "The statistics sampling interval specified is not a valid number."
      show_usage
      exit 1
    fi

    if [ $StatInterval -gt 86400 ]
    then
      log_message "The maximimum statistics sampling interval (-i) exceeds a reasonnable useful value of 1 day (86400 seconds)."
      show_usage
      exit 1
    fi

}

#-----------------------------------------------------------------------------------------------------------------------
# Function to check that there are enough group numbers available and the total launched will not exceed the maximum
# Argument: $1 = requested number of new PMD-RMD pairs to launch
check_number_of_netanalysis_groups() {

    needed_PMD_RMD_netanal_pairs=$1
    actual_running_netanal_pairs=`/bin/grep NETWORK-ANALYSIS $DtcLibPath/p*.cfg 2>/dev/null | /bin/grep -i " on" | wc -l | awk '{print $1}'`  
	total_netanal_pairs=`expr ${needed_PMD_RMD_netanal_pairs} + ${actual_running_netanal_pairs}`
    if [ $total_netanal_pairs -gt 100 ]
    then
        log_message "With the already configured groups for network analysis ($actual_running_netanal_pairs), the total number ($total_netanal_pairs) would exceed the maximum of 100."
        exit 1
    fi

}

#-------------------------------------------------------------------------------
#####          FUNCTION TO SET NETWORK ANALYSIS PARAMETER FILE             #####
# Set the network analysis parameter file for this run, including group attributes
# that are normally found in the Pstore.
# Note: some of the parameters are not specified as arguments to this script
# but their usual default values are set in the file as they are normally found
# in the Pstore.
set_network_analysis_parms_file() {

    net_parms_file=$DtcLibPath/SFTKdtc_net_analysis_parms.txt

    echo "NUMSECONDS: $NumberOfSeconds" > $net_parms_file
    echo "NUMPMDRMD: $NumberOfPMDRMDpairs" >> $net_parms_file
    echo "CHUNKSIZE: $ChunkSize" >> $net_parms_file
    echo "CHUNKDELAY: $ChunkDelay" >> $net_parms_file
    echo "SYNCMODE: off" >> $net_parms_file
    echo "SYNCMODEDEPTH: 1" >> $net_parms_file
    echo "SYNCMODETIMEOUT: 30" >> $net_parms_file
    echo "NETMAXKBPS: $NetMaxKBPS" >> $net_parms_file
    echo "STATINTERVAL: $StatInterval" >> $net_parms_file
    echo "MAXSTATFILESIZE: 1024" >> $net_parms_file
    echo "TRACETHROTTLE: off" >> $net_parms_file
    echo "COMPRESSION: off" >> $net_parms_file
    echo "JOURNAL: off" >> $net_parms_file
    echo "_MODE: NETANALYSIS" >> $net_parms_file
    echo "_AUTOSTART: no" >> $net_parms_file
    echo "LRT: off" >> $net_parms_file
}    

#-------------------------------------------------------------------------------
#####         FUNCTION TO CREATE A FICTITIOUS GROUPS CONFIG FILE           #####
create_fictitious_group_config_file() {

    config_file=$DtcLibPath/net_analysis_group.cfg
    echo "#===============================================================" > $config_file
    echo "#  Softek Replicator for UNIX*" >> $config_file
    echo "#  Configuration file to allow launching PMD-RMD pairs" >> $config_file
    echo "#  in network bandwidth analysis mode." >> $config_file
    echo "#===============================================================" >> $config_file
    echo "#" >> $config_file
    echo "# Primary System Definition:" >> $config_file
    echo "#" >> $config_file
    echo "SYSTEM-TAG:          SYSTEM-A                  PRIMARY" >> $config_file
    echo "  HOST:                $PrimaryServer" >> $config_file
    echo "  PSTORE:              /dev/null" >> $config_file
    echo "  AUTOSTART:           on" >> $config_file
    echo "  DYNAMIC-ACTIVATION:  on" >> $config_file
    echo "  NETWORK-ANALYSIS:    on" >> $config_file
    echo "#" >> $config_file
    echo "# Secondary System Definition:" >> $config_file
    echo "#" >> $config_file
    echo "SYSTEM-TAG:          SYSTEM-B                  SECONDARY" >> $config_file
    echo "  HOST:                $SecondaryServer" >> $config_file
    echo "  JOURNAL:             /journal" >> $config_file
    echo "  SECONDARY-PORT:      575" >> $config_file
    echo "  CHAINING:            off" >> $config_file
    echo "#" >> $config_file
    echo "# Device Definitions:" >> $config_file
    echo "#" >> $config_file
    echo "PROFILE:            1" >> $config_file
    echo "  REMARK:  " >> $config_file
    echo "  PRIMARY:          SYSTEM-A" >> $config_file
    echo "  DTC-DEVICE:       /dev/dtc/lg999/dsk/dtc0" >> $config_file # <-- group number will be adjusted on runtime
    # IMPORTANT: do not change the following (/dev/zero); it may be expected by other modules
    echo "  DATA-DISK:         /dev/zero " >> $config_file
    echo "  SECONDARY:        SYSTEM-B" >> $config_file
    echo "  MIRROR-DISK:       /dev/null " >> $config_file
    echo "#" >> $config_file
    echo "#" >> $config_file
    echo "# -- End of Softek Replicator for UNIX* Configuration File for network bandwidth analysis" >> $config_file

}    

############################# MAIN SCRIPT BODY ################################

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

log_message "$CommandArguments"

# Check if we are root
if [ "$COMMAND_USR" != "root" ]; then
	log_message "You must be root to run this process...aborted."
	exit 1
fi

# Check that the license is valid
RESULT=`$DtcBinPath/%Q%licinfo`
if [ $? -ne 0 ]
then
       echo $RESULT
       echo "Please correct this %PRODUCTNAME% license problem and try again."
       exit 1
fi

# Check that there are enough group numbers available and the total launched will not exceed the maximum
check_number_of_netanalysis_groups $NumberOfPMDRMDpairs

# Set the network analysis parameter file for this run
set_network_analysis_parms_file

# Create a fictitious replication group configuration file to allow starting the PMD-RMD
create_fictitious_group_config_file

# Call the network analysis binary to send the command to the master daemon
$DtcBinPath/%Q%netanalysis -n $NumberOfPMDRMDpairs
result=$?
exit $result

