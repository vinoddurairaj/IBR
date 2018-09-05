THIS FILE IS NO LONGER THE MASTER FILE; THE MASTER FILE UP TO DATE IS IN THE DataMobilityConsole repository
#! /bin/sh
########################################################################
#
#  UNIX_TDMFIP_remote_install.sh
#
# LICENSED MATERIALS / PROPERTY OF IBM
# TDMF IP UNIX FOR Open Systems version 2.8.0
# (c) Copyright IBM 2014 2001.  All Rights Reserved.
# The source code for this program is not published or otherwise divested of
# its trade secrets, irrespective of what has been deposited with the U.S.
# Copyright Office.
# US Government Users Restricted Rights - Use, duplication or disclosure
# restricted by GSA ADP Schedule Contract with IBM Corp.
#
#
########################################################################


#-------------------------------------------------------------------------------
##### FUNCTION TO CREATE THE INSTALLATION LOG FILE ####
create_log_file() {

    TimeStamp=`/bin/date "+%Y-%m-%d-%Hh%Mm%S"`;
    logf=/var/UNIX_TDMFIP_Remote_Install.$Host_name.$TimeStamp.log
    datestr=`/bin/date "+%Y/%m/%d %T"`; # To log the first message with the same time format as in log_message
    errmsg="[${datestr}] UNIX_TDMFIP_remote_install: launching UNIX TDMF IP remote installation...";
    /bin/echo ${errmsg} 1>> ${logf} 2>&1

    if [ $Verbose -gt 0 ]
    then
	    /bin/echo ${errmsg}
    fi
}

#-------------------------------------------------------------------------------
##### FUNCTION TO LOG A MESSAGE AND DISPLAY IT IF VERBOSE IS SET ####
# Arguments:
# $1: message string
log_message() {

    datestr=`/bin/date "+%Y/%m/%d %T"`;
    errhdr="[${datestr}] UNIX_TDMFIP_remote_install: ";
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
        DtcBinPath=/opt/SFTKdtc/bin;
        SyncCmd=/bin/sync;
        Host_name=`/usr/bin/hostname`;
        Product_logf=/var/opt/SFTKdtc/dtcerror.log;
	    COMMAND_USR=`/usr/bin/whoami`

    elif [ $Platform = "SunOS" ]; then
        DtcBinPath=/opt/SFTKdtc/bin;
        SyncCmd=/bin/sync;
        Host_name=`/usr/bin/hostname`;
        Product_logf=/var/opt/SFTKdtc/dtcerror.log;
	    if [ -x /usr/ucb/whoami ]; then COMMAND_USR=`/usr/ucb/whoami`; else COMMAND_USR=`/usr/bin/whoami`; fi

    elif [ $Platform = "AIX" ]; then
        IsAIX=1;
        DtcBinPath=/usr/dtc/bin;
        SyncCmd=/usr/sbin/sync;
        Host_name=`/usr/bin/hostname`;
        Product_logf=/var/dtc/dtcerror.log;
        GrepCmd=/usr/bin/grep;
        EchoCmd=/usr/bin/echo;
        MvCmd=/usr/bin/mv;
	    COMMAND_USR=`/bin/whoami`

    elif [ $Platform = "Linux" ]; then
        IsLinux=1;
        DtcBinPath=/opt/SFTKdtc/bin;
        SyncCmd=/bin/sync;
        Host_name=`/bin/hostname --long | /usr/bin/tr -d ' '`;  # tr command is for trimming any trailing whitespace
        Product_logf=/var/opt/SFTKdtc/dtcerror.log;
        Processor=`/bin/uname -p`;
        GrepCmd=/bin/grep;
        EchoCmd=/bin/echo;
        MvCmd=/bin/mv;
        COMMAND_USR=`/usr/bin/whoami`
    fi
}


#---------------------------------------------------------------------------------------------------
##### FUNCTION TO DETERMINE IF A PARAMETER PARAGRAPH BELONGS TO THIS SERVER, BASED ON HOST NAME ####
#####            If so, the parameter paragraph is appended to this server's parameter file     ####
# Arguments:
# $1: temporary_paragraph_file which contains the information to check
# $2: target file where to append the information if a host name match is found
# return: 1 if paragraph info matches this host's name
check_paragraph_and_save_if_good() {

    temporary_paragraph_file=$1
    Server_install_parm_file=$2

    found_this_server_paragraph=0

    stored_hostname=`$GrepCmd "SERVER=" $temporary_paragraph_file | $GrepCmd -v "#" | /usr/bin/cut -d '=' -f2 | /bin/awk '{print $1}'`
    if [ $stored_hostname == ${Host_name} ]
    then
        found_this_server_paragraph=1
    else
        # Host name not equal; check if our variable does not contain the fully qualified host name
        # If so, accept a grep match with the stored host name
        $EchoCmd ${Host_name} | $GrepCmd "\." 1> /dev/null 2>&1
        result=$?
        if [ $result -ne 0 ]
        then
            # Our variable Host_name is not fully qualified; use grep
            $GrepCmd "SERVER=" $temporary_paragraph_file | $GrepCmd -v "#" | $GrepCmd ${Host_name} 1> /dev/null 2>&1
            result=$?
            if [ $result -eq 0 ]
            then
                found_this_server_paragraph=1
            fi
        fi
    fi
    if [ $found_this_server_paragraph -eq 1 ]
    then
        # This paragraph is the good one
        # Make it the active one for the installation (concatenate to the active one)
        /bin/cat $temporary_paragraph_file >> $Server_install_parm_file
    fi

    return $found_this_server_paragraph
}

#---------------------------------------------------------------------------------------------
##### FUNCTION TO EXTRACT THIS SERVER'S INSTALLATION PARAMETERS FROM THE GLOBAL PARM FILE ####
#
# NOTE: the parameter file is expected to be located in the directory from which the script was launched
#       (i.e. the working directory).
extract_installation_parms_from_global_file() {

    # Convert DOS end of lines to Unix end of lines in the global parameter file
    $MvCmd ./TDMFIP_Remote_Install.parms ./TDMFIP_Remote_Install.parms.original
    /usr/bin/tr -d '\r' < ./TDMFIP_Remote_Install.parms.original > ./TDMFIP_Remote_Install.parms
    result=$?
    if [ $result -ne 0 ]
    then
        log_message "WARNING: error while trying to convert Windows end of lines to Unix end of lines. Trying to proceed anyway."
        # Restore the original file
        /bin/rm -f ./TDMFIP_Remote_Install.parms
        $MvCmd ./TDMFIP_Remote_Install.parms.original ./TDMFIP_Remote_Install.parms
    else
        /bin/rm -f ./TDMFIP_Remote_Install.parms.original
    fi

    Global_parm_file=./TDMFIP_Remote_Install.parms
    Server_install_parm_file=./${Host_name}_TDMFIP_remote_install.parms
    temporary_paragraph_file=./tmp_parm_paragraph_file

    if [ ! -f $Global_parm_file ]
    then
        log_message "Did not find the global installation parameter file: $Global_parm_file. Exiting."
        exit 1
    fi

    # Check if we are doing just a package uninstall (no installation); in this case, we will just push
    # the UNINSTALL_ONLY parameter to the server's parm file.
    # NOTE: this is a global parameter, valid for all the servers in the parameter file.
    Package_Uninstall_Only=0
    while read line
    do
        $EchoCmd $line | $GrepCmd "UNINSTALL_ONLY" | $GrepCmd -v "#" 1> /dev/null 2>&1
        result=$?
        if [ $result -eq 0 ]
        then
            # Found the UNINSTALL_ONLY parameter; if it is set to 1, save it to this server's parm paragraph file and return
            Package_Uninstall_Only=`$EchoCmd $line | /usr/bin/cut -d '=' -f2 | /bin/awk '{print $1}'`
            break;
        fi
    done < $Global_parm_file
    if [ $Package_Uninstall_Only -eq 1 ]
    then
        $EchoCmd $line > $Server_install_parm_file
        # Delete the global parameter file
        if [ $Debug_remote_install -eq 0 ]
        then
            /bin/rm -f $Global_parm_file
        fi
        return # ---> return; no need of other parameters
    fi

    # Find the Collector IP address first, which is global and appears only once in the global file
    found_collector_IP=0
    while read line
    do
        $EchoCmd $line | $GrepCmd "COLLECTOR_IP" | $GrepCmd -v "#" 1> /dev/null 2>&1
        result=$?
        if [ $result -eq 0 ]
        then
            # Found the collector IP address; save it to this server's parm paragraph file
            $EchoCmd $line > $Server_install_parm_file
            found_collector_IP=1
            break
        fi
    done < $Global_parm_file
    if [ $found_collector_IP -eq 0 ]
    then
        log_message "Collector IP address not found in global parameter file. Exiting."
        if [ $Debug_remote_install -eq 0 ]
        then
            /bin/rm -f $temporary_paragraph_file $Global_parm_file
        fi
        exit 1
    fi

    # Now locate and extract this server's parameter paragraph
    found_start_target_server_paragraph=0
    found_this_server_paragraph=0

    while read line
    do
        if [ $found_start_target_server_paragraph -eq 0 ]
        then
            $EchoCmd $line | $GrepCmd "START_TARGET_SERVER_PARAGRAPH" 1> /dev/null 2>&1
            result=$?
            if [ $result -eq 0 ]
            then
                found_start_target_server_paragraph=1
                $EchoCmd $line > $temporary_paragraph_file # Overwrites the old content of the temporary file
            fi
        else
            # Extract this target server parameter paragraph to a temporary file unless it is the start of another server's paragraph
            # Check for the end of this paragraph (by detecting the start of the next one)
            $EchoCmd $line | $GrepCmd "START_TARGET_SERVER_PARAGRAPH" 1> /dev/null 2>&1
            result=$?
            if [ $result -eq 0 ]
            then
                # This is the start of another server's paragraph; check if the previously saved one is the good one by checking the host name in it
                check_paragraph_and_save_if_good $temporary_paragraph_file $Server_install_parm_file
                found_this_server_paragraph=$?

                if [ $found_this_server_paragraph -eq 1 ]
                then
                    # The last extracted paragraph is the good one
                    # We're done
                    break
                else
                    $EchoCmd $line > $temporary_paragraph_file # Overwrites the old content of the temporary file
                fi
            else
                # Still inside paragraph; save the line
                $EchoCmd $line >> $temporary_paragraph_file
            fi
        fi
    done < $Global_parm_file

    # Now check the last saved paragraph if not found yet
    if [ $found_this_server_paragraph -eq 0 ]
    then
        check_paragraph_and_save_if_good $temporary_paragraph_file $Server_install_parm_file
        found_this_server_paragraph=$?
    fi

    if [ $found_this_server_paragraph -eq 0 ]
    then
        log_message "Could not locate this server (${Host_name}) parameter paragraph in global file $Global_parm_file. Exiting."
        if [ $Debug_remote_install -eq 0 ]
        then
            /bin/rm -f $temporary_paragraph_file $Global_parm_file
        fi
        exit 1
    else
        log_message "This server's parameters have been saved in file $Server_install_parm_file"
    fi

    # Delete the global and temporary files
    if [ $Debug_remote_install -eq 0 ]
    then
        /bin/rm -f $temporary_paragraph_file $Global_parm_file
    fi
}

#-------------------------------------------------------------------------------
##### FUNCTION TO READ AND VALIDTE THE INSTALLATION PARAMETERS ####
#
# Parameter file format:
#
# PACKAGE_NAME=<Name of the package file without directory path>
# 
# COLLECTOR_IP=<Collector IP address for Agent communications with the DMC>;
#     NOTE: COLLECTOR_IP must be provided if REUSE_EXISTING_CONFIG is set to false (0)
#
# The following parameters are optional and have default values if not specified:
# <<<<<<<<<<<<<<<< TO BE UPDATED
# TARGET_PATH=<target path where the package, parameters and install script are on this target server>; {default = /var}
# FORCE_INSTALL=1 | 0; {default = 1, i.e. yes}
# REUSE_EXISTING_CONFIG=1 | 0; {default = 0, i.e. no}
# BAB_SIZE=<BAB size in Mbs>; {optional; default = 128 MB}
# TRANSMIT_INTERVAL=<agent transmit interval in seconds>; {optional} 
# MIGRATION_TARGET_ONLY=1 | 0; {default = 0, i.e. no, this is a migration source: set the BAB size to load the driver}
# 
source_and_validate_installation_parms() {

    # Set variables to non-valid (or default) values to detect if some of them are missing in the parm file

    # Mandatory parameters
    PACKAGE_NAME="Undefined"

    # COLLECTOR_IP must be provided if REUSE_EXISTING_CONFIG is set to false (0)
    COLLECTOR_IP="Undefined"

    # Optional parameters 
    TARGET_PATH=/var
    UNINSTALL_ONLY=0
    FORCE_INSTALL=-1
    REUSE_EXISTING_CONFIG=-1
    BAB_SIZE=-1
    TRANSMIT_INTERVAL=-1
    MIGRATION_TARGET_ONLY=-1

    Override_old_BAB_size=0

    Install_parm_file=./${Host_name}_TDMFIP_remote_install.parms

    if [ -f $Install_parm_file ]
    then
      . $Install_parm_file
    else
        log_message "Did not find the Installation parameter file: ${Install_parm_file}. Exiting."
        exit 1
    fi

    # Check if we just uninstall the currently installed package; if so, no other parsing to do.
    if [ $UNINSTALL_ONLY -eq 1 ]
    then
        log_message "the UNINSTALL_ONLY parameter is set; we will just uninstall the currently installed package."
        return
    fi

    # We are installing a new package

    # Check mandatory parameter
    if [ $PACKAGE_NAME == "Undefined" ]
    then
        log_message "the package name is undefined in installation parameter file ${Install_parm_file}. Exiting."
        exit 1
    fi

    if [ $REUSE_EXISTING_CONFIG -lt 0 ]
    then
        log_message "Setting default value for REUSE_EXISTING_CONFIG parameter: 0 (no)"
        REUSE_EXISTING_CONFIG=0
    fi

    # COLLECTOR_IP must be provided if REUSE_EXISTING_CONFIG is set to false (0)
    if [ $COLLECTOR_IP == "Undefined" -a $REUSE_EXISTING_CONFIG -eq 0 ] 
    then
        log_message "Error: Collector IP address undefined in installation parameter file ${Install_parm_file} and REUSE_EXISTING_CONFIG is false."
        log_message "If we are not reusing the existing configurations, the Collector IP address must be provided. Exiting."
        exit 1
    fi

    # Now check the optional parameters; set default values if applicable
    if [ $FORCE_INSTALL -lt 0 ]
    then
        log_message "Setting default value for FORCE_INSTALL parameter: 1"
        FORCE_INSTALL=1
    fi

    if [ $MIGRATION_TARGET_ONLY -lt 0 ]
    then
        log_message "Setting default value for MIGRATION_TARGET_ONLY parameter: 0"
        MIGRATION_TARGET_ONLY=0
    fi

    if [ $BAB_SIZE -lt 0 ]
    then
        if [ $REUSE_EXISTING_CONFIG -eq 0 -a $MIGRATION_TARGET_ONLY -eq 0 ]
        then
            log_message "Setting default value for BAB_SIZE parameter: 128"
        fi
        # We set the variable in all cases whether we will reuse the existing config or not
        BAB_SIZE=128
    else
        # If the BAB size is provided and we reuse the existing configuration, this new BAB size will
        # override the old one
        Override_old_BAB_size=1
    fi

    if [ $TRANSMIT_INTERVAL -lt 0 ]
    then
        if [ $REUSE_EXISTING_CONFIG -eq 0 ]
        then
            log_message "Setting default value for TRANSMIT_INTERVAL parameter: 4"
        fi
        # We set the variable in all cases whether we will reuse the existing config or not
        TRANSMIT_INTERVAL=4
    fi

    log_message "Installation parameters read from $Install_parm_file (and possibly default values set):"

    log_message "PACKAGE_NAME: ${PACKAGE_NAME}"    
    log_message "TARGET_PATH: ${TARGET_PATH}"    
    log_message "FORCE_INSTALL: ${FORCE_INSTALL}"    
    log_message "REUSE_EXISTING_CONFIG: ${REUSE_EXISTING_CONFIG}"    
    if [ $REUSE_EXISTING_CONFIG -eq 0 ]
    then
        log_message "COLLECTOR_IP: ${COLLECTOR_IP}"    
        log_message "TRANSMIT_INTERVAL: ${TRANSMIT_INTERVAL}"
        if [ $MIGRATION_TARGET_ONLY -eq 0 ]
        then
            log_message "BAB_SIZE: ${BAB_SIZE}"
        fi    
    else
        if [ $COLLECTOR_IP != "Undefined" ]
        then
            log_message "COLLECTOR_IP: ${COLLECTOR_IP}"
        fi
        if [ $Override_old_BAB_size -eq 1 ]
        then
            log_message "BAB_SIZE: ${BAB_SIZE}"
        fi    
    fi    
    log_message "MIGRATION_TARGET_ONLY: ${MIGRATION_TARGET_ONLY}"    
}

#-------------------------------------------------------------------------------
##### FUNCTION TO CONCATENATE THE INSTALLATION LOG FILE TO THE PRODUCT LOG ####
enter_log_file_in_product_log() {

    /bin/cat ${logf} >> $Product_logf
}



################################# AIX SPECIFIC FUNCTIONS ###############################

#-------------------------------------------------------------------------------
##### FUNCTION TO UNINSTALL THE CURRENT TDMFIP PACKAGE ON AIX ####
# Arguments: none
uninstall_AIX_TDMFIP() {

    uninstall_log_file=/var/TDMFIP_uninstall_$Host_name.$TimeStamp.log
    log_message "Uninstalling the current TDMF IP package; uninstall log file is $uninstall_log_file"
    /usr/sbin/installp -u dtc.rte 1> $uninstall_log_file 2>&1
    result=$?
    /usr/bin/rm -f ./dtc.rte ./.toc
    /usr/bin/rm -fr ./Softek

    if [ $result -ne 0 ]
    then
        log_message "TDMF IP package uninstall reported a non-zero status. Please verify by manually logging to the server."
    else
        log_message "TDMF IP package uninstall reported a success status."
    fi
}

#-------------------------------------------------------------------------------
##### FUNCTION TO INSTALL THE TDMFIP PACKAGE ON AIX ####
# Arguments:
# $1: file name of the package to install
# $2: flag that says if we must uninstall any existing package if applicable
# NOTE: the package to install is expected to be in the current working directory
install_AIX_TDMFIP_package() {

    Package=$1
    Uninstall_old_package=$2

    # Check that the specified package exists
    if [ ! -f $Package ]
    then
	    log_message "$Package not found. Please check the PACKAGE_NAME specification in ${Host_name}_TDMFIP_remote_install.parms."
	    log_message "NOTE: the package to install is expected to be in the same directory as this install script. Exiting."
	    exit 1
    fi

    # Check if a previous package is already installed
    $DtcBinPath/dtcinfo -v 1> /dev/null 2>&1
    result=$?
    if [ $result -eq 0 ]
    then
        # Detected package already installed, check if we can uninstall it
        if [ $Uninstall_old_package -eq 1 ]
        then
            log_message "Detected previous product packcage already installed; uninstalling this package..."
            uninstall_log_file=/var/TDMFIP_uninstall_$Host_name.$TimeStamp.log
            log_message "uninstall log file is $uninstall_log_file"
            /usr/sbin/installp -u dtc.rte 1> $uninstall_log_file 2>&1
            if [ $Package != "dtc.rte" ]
            then
                /usr/bin/rm -f dtc.rte
            fi
        else
            log_message "Detected previous product packcage already installed and FORCE_INSTALL parameter is false; exiting."
            exit 1
        fi
    fi

    # Set the name of the AIX installation log file
    installp_logfile=/var/TDMFIP_installp_$Host_name.$TimeStamp.log

    # Check if the package has the .gz extension (gzipped); if so, gunzip it
    file_extension=`/usr/bin/echo $Package | awk -F . '{print $NF}'`
    if [ $file_extension == "gz" ]
    then
        log_message "gunzipping the package to install with the command /usr/bin/gunzip -f $Package 1> $installp_logfile 2>&1"
        /usr/bin/gunzip -f $Package 1> $installp_logfile 2>&1
        result=$?
        if [ $result -ne 0 ]
        then
            log_message "gunzipping the package reported a non-zero status. Please check the package format and/or the parameter file ${Host_name}_TDMFIP_remote_install.parms."
            log_message "You can also look at the log file $installp_logfile for possible traces of what happened. Exiting."
            exit 1
        fi
        # Strip the .gz extension
        Package=`/usr/bin/basename $Package .gz`
    fi

    # Check if the package has the .Z extension (compressed); if so, uncompress it
    file_extension=`/usr/bin/echo $Package | awk -F . '{print $NF}'`
    if [ $file_extension == "Z" ]
    then
        if [ -f /usr/bin/uncompress ]
        then
            uncompress_command=/usr/bin/uncompress
        else
            uncompress_command=/bin/gunzip
        fi
        log_message "uncompressing with the command $uncompress_command -f $Package 1> $installp_logfile 2>&1"
        $uncompress_command -f $Package 1> $installp_logfile 2>&1
        result=$?
        if [ $result -ne 0 ]
        then
            log_message "Uncompressing the package reported a non-zero status. Please check the package format and/or the parameter file ${Host_name}_TDMFIP_remote_install.parms."
            log_message "You can also look at the log file $installp_logfile for possible traces of what happened. Exiting."
            exit 1
        fi
        # Strip the .Z extension
        Package=`/usr/bin/basename $Package .Z`
    fi

    # Check if the package has the .tar extension; if so, untar it
    file_extension=`/usr/bin/echo $Package | awk -F . '{print $NF}'`
    if [ $file_extension == "tar" ]
    then
        # Determine the location where will be untarred the package, i.e. full path and file name
        Untarred_package=`/usr/bin/tar -tf $Package 2> /dev/null`
        log_message "$Package will be extracted to $Untarred_package"

        log_message "Untarring with the command /usr/bin/tar -xf $Package 1> $installp_logfile 2>&1"
        /usr/bin/tar -xf $Package 1> $installp_logfile 2>&1
        result=$?
        if [ $result -ne 0 ]
        then
            log_message "Untarring the package reported a non-zero status. Please check the package format, disk space and/or the parameter file ${Host_name}_TDMFIP_remote_install.parms."
            log_message "You can also look at the log file $installp_logfile for possible traces of what happened. Exiting."
            exit 1
        fi

    else
        Untarred_package=$Package
    fi

    # Move the package to the current working directory if applicable, i.e. if there was an extraction path specified in the tar file
    /usr/bin/echo $Untarred_package | /usr/bin/grep "/"  1> /dev/null 2>&1
    result=$?
    if [ $result -eq 0 ]
    then
        if [ $Untarred_package != "./dtc.rte" ]
        then
            log_message "Moving $Untarred_package to current working directory..."
            /usr/bin/mv -f $Untarred_package .
        fi
    fi

    # Remove previous .toc (table of contents)
    /usr/bin/rm -f .toc 2>&1 > /dev/null

    # Install the package
    log_message "Installing the package with the command /usr/sbin/installp -a -V 4 -e $installp_logfile -d . dtc.rte"
    /usr/sbin/installp -a -V 4 -e $installp_logfile -d . dtc.rte 1> /dev/null 2>&1
    result=$?
    if [ $result -ne 0 ]
    then
        log_message "Status returned from installp command not 0. The installp installation log $installp_logfile must be checked. Exiting."
        exit 1
    else
        log_message "Package installation reported success status."
    fi

}


#-------------------------------------------------------------------------------
##### FUNCTION TO CONFIGURE AND LAUNCH THE DMC AGENT ON AIX ####
# Arguments:
# global parameters set in source_and_validate_installation_parms():
#    COLLECTOR_IP    
#    REUSE_EXISTING_CONFIG    
#    BAB_SIZE    
#    TRANSMIT_INTERVAL    
launch_AIX_DMC_Agent() {
 
    BAB_size_not_defined=0

    if [ $REUSE_EXISTING_CONFIG -eq 1 ]
    then
        $DtcBinPath/dtcagentset -r 1 1> /dev/null 2>&1
        result=$?
        if [ $result -eq 0 -a $COLLECTOR_IP != "Undefined" ]
        then
            # After recovering the existing configuration, if a Collector IP address has been specified, override the previous Collector IP
            $DtcBinPath/dtcagentset -r 0 -e $COLLECTOR_IP 1> /dev/null 2>&1
            result=$?
        fi
        # Check if the BAB size was defined in the configuration that we have restored. If not, and if this is not a migration target,
        # log a message saying the driver might not be loaded by the Agent at the end of the sequence because the BAB size needs to be defined.
        if [ $result -eq 0 ]
        then
            $DtcBinPath/dtcagentset | /usr/bin/grep "BAB size not set" 1> /dev/null 2>&1
            result2=$?
            if [ $result2 -eq 0 -a $MIGRATION_TARGET_ONLY -eq 0 ]
            then
                BAB_size_not_defined=1
                log_message "The BAB size was not defined in the recovered Agent configuration. The TDMFIP driver might not be loaded at the end of the installation."
                log_message "You will need to manually complete the procedure in order to load the driver."
            fi
        fi
        if [ $Override_old_BAB_size -eq 1 ]
        then
            $DtcBinPath/dtcagentset -r 0 -b $BAB_SIZE 1> /dev/null 2>&1
            result=$?
            if [ $result -eq 0 ]
            then
                log_message "The new BAB size has been set to the specified value: $BAB_SIZE MB"
            else
                log_message "WARNING: an error occurred while setting the new BAB size to the specified value: $BAB_SIZE MB"
                log_message "Please login to this server to verify the BAB size."
            fi
        fi
    else
    # Not reusing the previous configurations
        if [ $Verbose -eq 0 ]
        then
            # If parameters indicate that we must load the driver, configure the BAB size; then launchagent will load the driver
            if [ $MIGRATION_TARGET_ONLY -eq 0 ]
            then
                $DtcBinPath/dtcagentset -r0 -e $COLLECTOR_IP -b $BAB_SIZE -t $TRANSMIT_INTERVAL 1> /dev/null 2>&1
            else
                $DtcBinPath/dtcagentset -r0 -e $COLLECTOR_IP -t $TRANSMIT_INTERVAL 1> /dev/null 2>&1
            fi
        else
            if [ $MIGRATION_TARGET_ONLY -eq 0 ]
            then
                $DtcBinPath/dtcagentset -r0 -e $COLLECTOR_IP -b $BAB_SIZE -t $TRANSMIT_INTERVAL
            else
                $DtcBinPath/dtcagentset -r0 -e $COLLECTOR_IP -t $TRANSMIT_INTERVAL
            fi
        fi
        result=$?
    fi
    if [ $result -eq 0 ]
    then
        log_message "Successfully configured the Agent."
        # Display the Agent configuration in the log file; -r0 is for avoiding any prompt from dtcagentset
        $DtcBinPath/dtcagentset -r0 >> ${logf}
        # Display the Agent configuration also on screen if Verbose is set
        if [ $Verbose -eq 1 ]
        then
            $DtcBinPath/dtcagentset -r0
        fi
    else
        log_message "dtcagentset reported a non-zero status; error trying to configure the Agent. Exiting."
        exit 1
    fi
    if [ $MIGRATION_TARGET_ONLY -eq 0 -a $BAB_size_not_defined -eq 0 ]
    then
        log_message "Launching the Agent... This server should now be visible on the DMC and the TDMF IP driver should be loaded."
    else
        log_message "Launching the Agent... This server should now be visible on the DMC."
    fi
    $DtcBinPath/launchagent 1> /dev/null 2>&1
}



################################# LINUX SPECIFIC FUNCTIONS ###############################
#-------------------------------------------------------------------------------
##### FUNCTION TO UNINSTALL THE CURRENT TDMFIP PACKAGE ON LINUX ####
# Arguments: none
uninstall_Linux_TDMFIP() {

    # Check if a previous package is indeed installed
    $DtcBinPath/dtcinfo -v 1> /dev/null 2>&1
    result=$?
    if [ $result -ne 0 ]
    then
        log_message "No TDMF IP package installed."
        return
    fi

    uninstall_log_file=/var/TDMFIP_rpm_uninstall_$Host_name.$TimeStamp.log

    # Check for Replicator package
    Replicator_package=`/bin/rpm -qa | /bin/grep Replicator 2> /dev/null`
    result=$?
    if [ $result -eq 0 ]
    then
        # Found Replicator package
        log_message "Uninstalling the $Replicator_package package (uninstall log is $uninstall_log_file)..."
        /bin/rpm -e $Replicator_package 1> $uninstall_log_file 2>&1
        result=$?
        if [ $result -eq 0 ]
        then
            log_message "$Replicator_package was successfully uninstalled."
        else
            log_message "Replicator package uninstall reported a non-zero status. Please verify by manually logging to the server."
        fi
    else
        # Check for TDMFIP package
        TDMFIP_package=`/bin/rpm -qa | /bin/grep TDMFIP 2> /dev/null`
        result=$?
        if [ $result -eq 0 ]
        then
            # Found TDMFIP package
            log_message "Uninstalling the $TDMFIP_package package (uninstall log is $uninstall_log_file)..."
            /bin/rpm -e $TDMFIP_package 1> $uninstall_log_file 2>&1
            result=$?
            if [ $result -eq 0 ]
            then
                log_message "$TDMFIP_package was successfully uninstalled."
            else
                log_message "TDMFIP package uninstall reported a non-zero status. Please verify by manually logging to the server."
            fi
        fi
    fi

    /bin/rm -fr ./Softek

}

#-------------------------------------------------------------------------------
##### FUNCTION TO INSTALL THE TDMFIP PACKAGE ON LINUX ####
# Arguments:
# $1: file name of the package to install
# $2: flag that says if we must uninstall any existing package if applicable
# NOTE: the package to install is expected to be in the current working directory
install_Linux_TDMFIP_package() {

    Package=$1
    Uninstall_old_package=$2

    # Check that the specified package exists
    if [ ! -f $Package ]
    then
	    log_message "$Package not found. Please check the PACKAGE_NAME specification in ${Host_name}_TDMFIP_remote_install.parms."
	    log_message "NOTE: the package to install is expected to be in the same directory as this install script. Exiting."
	    exit 1
    fi

    # Check if a previous package is already installed
    $DtcBinPath/dtcinfo -v 1> /dev/null 2>&1
    result=$?
    if [ $result -eq 0 ]
    then
        # Detected package already installed, check if we can uninstall it
        if [ $Uninstall_old_package -eq 1 ]
        then
            log_message "Detected previous product packcage already installed; uninstalling this package..."
            uninstall_Linux_TDMFIP
        else
            log_message "Detected previous product packcage already installed and FORCE_INSTALL parameter is false; exiting."
            exit 1
        fi
    fi

    # Set the installation log file name
    rpm_install_logfile=/var/TDMFIP_rpm_install_$Host_name.$TimeStamp.log

    # Check if the package has the .gz extension (gzipped); if so, gunzip it
    file_extension=`/bin/echo $Package | awk -F . '{print $NF}'`
    if [ $file_extension == "gz" ]
    then
        log_message "gunzipping the package to install with the command /bin/gunzip -f $Package 1> $rpm_install_logfile 2>&1"
        /bin/gunzip -f $Package 1> $rpm_install_logfile 2>&1
        result=$?
        if [ $result -ne 0 ]
        then
            log_message "gunzipping the package reported a non-zero status. Please check the package format and/or the parameter file ${Host_name}_TDMFIP_remote_install.parms."
            log_message "You can also look at the log file $rpm_install_logfile for possible traces of what happened. Exiting."
            exit 1
        fi
        # Strip the .gz extension
        Package=`/bin/basename $Package .gz`
    fi

    # Check if the package has the .Z extension (compressed); if so, uncompress it
    file_extension=`/bin/echo $Package | awk -F . '{print $NF}'`
    if [ $file_extension == "Z" ]
    then
        if [ -f /bin/uncompress ]
        then
            uncompress_command=/bin/uncompress
        else
            uncompress_command=/bin/gunzip
        fi
        log_message "uncompressing with the command $uncompress_command -f $Package 1> $rpm_install_logfile 2>&1"
        $uncompress_command -f $Package 1> $rpm_install_logfile 2>&1
        result=$?
        if [ $result -ne 0 ]
        then
            log_message "Uncompressing the package reported a non-zero status. Please check the package format and/or the parameter file ${Host_name}_TDMFIP_remote_install.parms."
            log_message "You can also look at the log file $rpm_install_logfile for possible traces of what happened. Exiting."
            exit 1
        fi
        # Strip the .Z extension
        Package=`/bin/basename $Package .Z`
    fi

    # Check if the [unzipped | uncompressed] package has the .tar extension; if so, untar it
    file_extension=`/bin/echo $Package | awk -F . '{print $NF}'`
    if [ $file_extension == "tar" ]
    then
        # Determine the location where will be untarred the package, i.e. full path and file name
        Untarred_package=`/bin/tar -tf $Package | /bin/grep $Processor`
        log_message "$Package will be extracted to $Untarred_package"

        log_message "Untarring with the command /bin/tar -xf $Package 1> $rpm_install_logfile 2>&1"
        /bin/tar -xf $Package 1> $rpm_install_logfile 2>&1
        result=$?
        if [ $result -ne 0 ]
        then
            log_message "Untarring the package reported a non-zero status. Please check the package format, disk space and/or the parameter file ${Host_name}_TDMFIP_remote_install.parms."
            log_message "You can also look at the log file $rpm_install_logfile for possible traces of what happened. Exiting."
            exit 1
        fi

    else
        Untarred_package=$Package
    fi

    # Move the package to the current working directory if applicable, i.e. if there was an extraction path specified in the tar file
    /bin/echo $Untarred_package | /bin/grep "/" 1> /dev/null 2>&1
    result=$?
    if [ $result -eq 0 ]
    then
        log_message "Moving $Untarred_package to the current working directory..."
        /bin/mv -f $Untarred_package .
        
        # Extract the package name without path for the rpm command
        package_file_name=`/bin/basename $Untarred_package`
    else
        package_file_name=$Untarred_package
    fi

    # Install the package
    log_message "Installing the package with the command /bin/rpm -ivh $package_file_name 1> $rpm_install_logfile 2>&1"

    /bin/rpm -ivh $package_file_name 1> $rpm_install_logfile 2>&1
    result=$?
    if [ $result -ne 0 ]
    then
        log_message "Status returned from rpm command not 0. The rpm installation log $rpm_install_logfile must be checked. Exiting."
        exit 1
    else
        log_message "Package installation reported success status."
    fi

}


#-------------------------------------------------------------------------------
##### FUNCTION TO CONFIGURE AND LAUNCH THE DMC AGENT ON LINUX ####
# Arguments:
# global parameters set in source_and_validate_installation_parms():
#    COLLECTOR_IP    
#    REUSE_EXISTING_CONFIG    
#    BAB_SIZE    
#    TRANSMIT_INTERVAL    
launch_Linux_DMC_Agent() {
 
    BAB_size_not_defined=0

    if [ $REUSE_EXISTING_CONFIG -eq 1 ]
    then
        $DtcBinPath/dtcagentset -r 1 1> /dev/null 2>&1
        result=$?
        if [ $result -eq 0 -a $COLLECTOR_IP != "Undefined" ]
        then
            # After recovering the existing configuration, if a Collector IP address has been specified, override the previous Collector IP
            $DtcBinPath/dtcagentset -r 0 -e $COLLECTOR_IP 1> /dev/null 2>&1
            result=$?
        fi 
        # Check if the BAB size was defined in the configuration that we have restored. If not, and if this is not a migration target,
        # log a message saying the driver might not be loaded by the Agent at the end of the sequence because the BAB size needs to be defined.
        if [ $result -eq 0 ]
        then
            $DtcBinPath/dtcagentset | /bin/grep "BAB size not set" 1> /dev/null 2>&1
            result2=$?
            if [ $result2 -eq 0 -a $MIGRATION_TARGET_ONLY -eq 0 ]
            then
                BAB_size_not_defined=1
                log_message "The BAB size was not defined in the recovered Agent configuration. The TDMFIP driver might not be loaded at the end of the installation."
                log_message "You will need to manually complete the procedure in order to load the driver."
            fi
        fi
        if [ $Override_old_BAB_size -eq 1 ]
        then
            $DtcBinPath/dtcagentset -r 0 -b $BAB_SIZE 1> /dev/null 2>&1
            result=$?
            if [ $result -eq 0 ]
            then
                log_message "The new BAB size has been set to the specified value: $BAB_SIZE MB"
            else
                log_message "WARNING: an error occurred while setting the new BAB size to the specified value: $BAB_SIZE MB"
                log_message "Please login to this server to verify the BAB size."
            fi
        fi
    else
    # Not reusing the previous configurations
        if [ $Verbose -eq 0 ]
        then
            # If parameters indicate that we must load the driver, configure the BAB size; then launchagent will load the driver
            if [ $MIGRATION_TARGET_ONLY -eq 0 ]
            then
                $DtcBinPath/dtcagentset -r0 -e $COLLECTOR_IP -b $BAB_SIZE -t $TRANSMIT_INTERVAL 1> /dev/null 2>&1
            else
                $DtcBinPath/dtcagentset -r0 -e $COLLECTOR_IP -t $TRANSMIT_INTERVAL 1> /dev/null 2>&1
            fi
        else
            if [ $MIGRATION_TARGET_ONLY -eq 0 ]
            then
                $DtcBinPath/dtcagentset -r0 -e $COLLECTOR_IP -b $BAB_SIZE -t $TRANSMIT_INTERVAL
            else
                $DtcBinPath/dtcagentset -r0 -e $COLLECTOR_IP -t $TRANSMIT_INTERVAL
            fi
        fi
        result=$?
    fi
    if [ $result -eq 0 ]
    then
        log_message "Successfully configured the Agent."
        # Display the Agent configuration in the log file; -r0 is for avoiding any prompt from dtcagentset
        $DtcBinPath/dtcagentset -r0 >> ${logf}
        # Display the Agent configuration also on screen if Verbose is set
        if [ $Verbose -eq 1 ]
        then
            $DtcBinPath/dtcagentset -r0
        fi
    else
        log_message "dtcagentset reported a non-zero status; error trying to configure the Agent. Exiting."
        exit 1
    fi
    if [ $MIGRATION_TARGET_ONLY -eq 0 -a $BAB_size_not_defined -eq 0 ]
    then
        log_message "Launching the Agent... This server should now be visible on the DMC and the TDMF IP driver should be loaded."
    else
        log_message "Launching the Agent... This server should now be visible on the DMC."
    fi
    $DtcBinPath/launchagent 1> /dev/null 2>&1
}


#-------------------------------------------------------------------------------
##### FUNCTION TO VERIFY IF THE REQUESTED BAB SIZE HAS BEEN OBTAINED ####
# Arguments:
# $1: requested BAB size in MBs
verify_obtained_BAB_size() {

    requested_BAB_Size=$1
    can_obtain_BAB_size=0
    waited=0
    maxwaitseconds=30

    # Wait until the driver has been loaded
    while [ $waited -lt $maxwaitseconds ]
    do
        $DtcBinPath/dtcinfo -a | $GrepCmd "Actual BAB size" 1> /dev/null 2>&1
        result=$?
        if [ $result -eq 0 ]
        then
            can_obtain_BAB_size=1
            break
        else
            sleep 2
            waited=`expr $waited + 2`
        fi
    done
    if [ $can_obtain_BAB_size -eq 1 ]
    then
        obtained_BAB_size=`$DtcBinPath/dtcinfo -a | grep "Actual BAB size" | /bin/awk '{print $5}' 2> /dev/null`
        if [ $obtained_BAB_size -ne $requested_BAB_Size ]
        then
            log_message "WARNING: please verify the obtained BAB size; requested: $requested_BAB_Size; obtained: $obtained_BAB_size."
        fi
    else
            log_message "WARNING: could not verify the obtained BAB size; you may have to check the size that was obtained."
    fi

}


################################ MAIN SCRIPT BODY ########################################
# Set Verbose to 1 to display the messages on screen and to have them uploaded to the
# WinSCP remote installation initiator.
Verbose=1

# Debugging flag
Debug_remote_install=0

# Set platform dependent variables
set_platform_specifics

# Create the installation log file
create_log_file

# Check if we are root
if [ "$COMMAND_USR" != "root" ]; then
	log_message "Only root can install the product...aborted."
	exit 1
fi

# The parameter file we receive may contain information related to several servers;
# we must extract the information for this server.
extract_installation_parms_from_global_file

# Source the installation parameter file and validate the parameters.
# NOTE: if parameters are not valid, the valid function exits, so it returns here
#       we are OK to proced.
source_and_validate_installation_parms

BAB_size_bytes=`expr $BAB_SIZE \* 1024 \* 1024`

# Proceed with installation (or uninstall if parameter UNINSTALL_ONLY is set to 1) of the package
if [ $IsAIX -eq 1 ]
then
    if [ $UNINSTALL_ONLY -eq 1 ]
    then
        uninstall_AIX_TDMFIP
    else
        install_AIX_TDMFIP_package $PACKAGE_NAME $FORCE_INSTALL
        launch_AIX_DMC_Agent
        sleep 2
        if [ $MIGRATION_TARGET_ONLY -eq 0 -a $BAB_size_not_defined -eq 0 ]
        then
            verify_obtained_BAB_size $BAB_size_bytes
        fi
    fi
fi

if [ $IsLinux -eq 1 ]
then
    if [ $UNINSTALL_ONLY -eq 1 ]
    then
        uninstall_Linux_TDMFIP
    else
        install_Linux_TDMFIP_package $PACKAGE_NAME $FORCE_INSTALL
        launch_Linux_DMC_Agent
        sleep 2
        if [ $MIGRATION_TARGET_ONLY -eq 0 -a $BAB_size_not_defined -eq 0 ]
        then
            verify_obtained_BAB_size $BAB_size_bytes
        fi
    fi
fi

# Give time to the Agent to complete its startup and concatenate the log file
# to the product log file so that the information be uploaded to the DMC Events tab.
sleep 5
enter_log_file_in_product_log

exit 0

