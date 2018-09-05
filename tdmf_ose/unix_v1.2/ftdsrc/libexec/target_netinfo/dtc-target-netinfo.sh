#! /bin/sh
########################################################################
#
#  dtc-target-netinfo.sh
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
########################################################################
#
# This command, from the information given by the user, will create the parameter file
# /etc/SFTKdtc_rootvg_failover_network_parms.txt, which will be read by the script
# SFTKdtc_AIX_failover_boot_network_reconfig.sh whose purpose it to reset the target
# network configuration at failover boot (i.e. when switching toe the replicated rootvg)
# on the target. The info that the user must provide is that which is expected in
# smitty Basic Configuration menu, plus the Replicator group number used for replication of the rootvg.
#
########################################################################



##### FUNCTION TO SHOW THE COMMAND USAGE ####
show_usage() {

    echo "------------------------------------------------------------------------------------------------------"
    echo "USAGE:"
    echo "  dtc-target-netinfo"
    echo "  this script is intended for AIX rootvg replication and failover;"
    echo "  it prompts for the Target server network configuration parameters and creates a parameter"
    echo "  file that will be read by the script called at boot failover time on the configured target server."
    echo "-------------------------------------------------------------------------------------------------------"
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
##### FUNCTION TO SET DEFAULT VALUES FOR TESTING #####
SetDefaultValues() {

TargetHostname="sjpwr7ded"
TargetHostIPaddress="9.30.121.29"
Subnetmask="255.255.255.0"
NetworkInterface="en0"
NameServerIPaddress="9.0.128.50"
DomainName="svl.ibm.com"
GatewayIPaddress="9.30.121.1"
Cost=0
ActiveDeadGatewayDetection="no"
CableType="N/A"
NetworkAdapter="inet0"
GroupNumber=0

}

#-------------------------------------------------------------------------------
copy_file()
{
    /usr/bin/cp -pf "$1" "$2"
	cp_result=$?
	if [ cp_result -ne 0 ]
    then
     echo "Copying $1 to $2 failed.";
	  echo "This error is most likely caused by directory permissions"
	  echo "or being out of space on the filesystem assoicated with the"
	  echo "/etc directory which is normally the root filesystem."
	  sync
	  exit 1
    fi

    if ! cmp -s "$1" "$2"
    then
	  echo "Comparing $1 to $2 after copying failed."
	  echo "This error is most likely caused by being out of space on"
	  echo "the filesystem the /etc directory is associated with which"
	  echo "is normally the root filesystem."
	  sync
	  exit 1
    fi
}


#-------------------------------------------------------------------------------
# Function to ping a machine, with a timeout, to help validate the network info
# given by the user.
# If the machine does not respond, the user is asked if he wants to continue or abort.
# Arguments:
# $1: machine hostname or IP address
ping_machine()
{
    MachineID=$1
    echo "        Verifying $MachineID..."
    ping -c 3 -w 5 $MachineID 1> /dev/null 2>&1
    result=$?
    if [ $result -ne 0 ]
    then
        echo "        WARNING: $MachineID is not responding to ping, which may be OK if it is temporarily inaccessible."
        echo "          You will be able to modify this parameter when prompted to confirm input, if desired."
        echo "          Please type <Enter> to continue, or <Ctrl-c> to abort]: \c: "
        read anykey
    fi
}


################################ MAIN SCRIPT BODY ########################################
UseDefaultValues=0

# If any argument provided (such as -h), print usage paragraph. We don't expect any argument.
if [ $# -ge 1 ]
then
    show_usage
    exit 0
fi

Platform=`/bin/uname`
if [ $Platform != "AIX" ]
then
    echo "This script can only be run on AIX."
    exit 1
fi

# Input all the nework parameters required for the mktcpip command and save them in a file.
# NOTE: hostname, domain name and IP addresses are checked with the ping_machine function: if this fails,
#    the ping_machine function prints a warning and allows to abort or continue.
if [ $UseDefaultValues -eq 1 ]
then
    SetDefaultValues
else
    echo " "
    echo " "
    echo "Please enter the TARGET server network configuration parameters."
    echo " "

    echo "    Target server hostname: \c"
    read TargetHostname
    ping_machine $TargetHostname

    echo "    Target host IP address (ex: 1.2.3.4): \c"
    read TargetHostIPaddress
    ping_machine $TargetHostIPaddress

    echo "    Subnetmask (ex: 255.255.255.0): \c"
    read Subnetmask

    echo "    Network interface (ex: en0): \c"
    read NetworkInterface

    echo "    Name Server IP address: \c"
    read NameServerIPaddress
    ping_machine $NameServerIPaddress

    echo "    Domain name (ex: subdomain.domain.com): \c"
    read DomainName
    # To validate the domain name, we concatenate the target hostname with the domain name and try ping_machine
    SimpleHostName=`echo "$TargetHostname" | cut -f1 -d "."`
    ping_machine "$SimpleHostName.$DomainName"

    echo "    Default Gateway IP address: \c"
    read GatewayIPaddress
    ping_machine $GatewayIPaddress

    ValidAnswer=0
    while [ $ValidAnswer -ne 1 ]
    do
        echo "    Cost for default route (-C arg to mktcpip, ex: 0): \c"
        read Cost
        # Check that the cost is a valid number
        if echo $Cost | /bin/grep "[^0-9"] 2>&1 > /dev/null
        then
          echo "        ERROR: the cost value is not a valid number."
        else
          ValidAnswer=1
        fi
    done

    ValidAnswer=0
    while [ $ValidAnswer -ne 1 ]
    do
        echo "    Do active dead gateway detection on default route? (-A arg to mktcpip, enter yes or no): \c"
        read ActiveDeadGatewayDetection
        if [ "$ActiveDeadGatewayDetection" != "yes" -a "$ActiveDeadGatewayDetection" != "no" ]
        then
            echo "        Invalid input [yes or no]."
        else
            ValidAnswer=1
        fi
    done

    ValidAnswer=0
    while [ $ValidAnswer -ne 1 ]
    do
        echo "    CableType (-t arg to mktcpip, dix/bnc or N/A): \c"
        read CableType
        if [ "$CableType" != "dix" -a "$CableType" != "bnc" -a "$CableType" != "N/A" ]
        then
            echo "        Invalid input [dix/bnc or N/A]."
        else
            ValidAnswer=1
        fi
    done

    echo "    Network adapter (ex.: inet0): \c"
    read NetworkAdapter

    ValidAnswer=0
    while [ $ValidAnswer -ne 1 ]
    do
        echo "    TDMF UNIX Replicator group number: \c"
        read GroupNumber
        # Check that the group number is valid
        if echo $GroupNumber | /bin/grep "[^0-9"] 2>&1 > /dev/null
        then
            echo "        ERROR: the group identifier is not a valid number."
        else
            # Check that the group number does not exceed the maximum
            if [ $GroupNumber -gt 999 ]
            then
              echo "        ERROR: the group number maximum value is 999."
            else
                ValidAnswer=1
            fi
        fi
    done

    InputConfirmed=0
    while [ $InputConfirmed -ne 1 ]
    do
        echo " "
        echo "---------------------------------------------------------------------------------------"
        echo "Please confirm that the input is correct:"
        echo "     1) Target server hostname...................................... $TargetHostname"
        echo "     2) Target host IP address (ex: 1.2.3.4)........................ $TargetHostIPaddress"
        echo "     3) Subnetmask (ex: 255.255.255.0):............................. $Subnetmask"
        echo "     4) Network interface (ex: en0):................................ $NetworkInterface"
        echo "     5) Name Server IP address...................................... $NameServerIPaddress"
        echo "     6) Domain name (ex: subdomain.domain.com)...................... $DomainName"
        echo "     7) Default Gateway IP address.................................. $GatewayIPaddress"
        echo "     8) Cost for default route (-C arg to mktcpip, ex: 0)........... $Cost"
        echo "     9) Active dead gateway detection on default route? (yes/no).... $ActiveDeadGatewayDetection"
        echo "    10) CableType (-t arg to mktcpip, dix/bnc or N/A)............... $CableType"
        echo "    11) Network adapter (ex.: inet0)................................ $NetworkAdapter"
        echo "    12) TDMF UNIX Replicator group number........................... $GroupNumber"
        echo "---------------------------------------------------------------------------------------"
        echo " "
        ValidAnswer=0
        while [ $ValidAnswer -ne 1 ]
        do
            echo "  Are all these values correct [y/n]? \c"
            read answer
            if [ $answer = "y" -o $answer = "Y" -o $answer = "n" -o $answer = "N" ]
            then
                ValidAnswer=1
            fi
        done
        if [ $answer = "y" -o $answer = "Y" ]
        then
            InputConfirmed=1
        else
            item_ok=0
            while [ $item_ok -eq 0 ]
            do
                echo "  Which item must be changed [1-12]? \c"
                read ItemToChange
                if [ $ItemToChange -lt 1 -o $ItemToChange -gt 12 ]
                then
                    echo "    Invalid number."
                else
                    item_ok=1
                fi
            done
            echo " "
            case $ItemToChange in	
                 1)
                    echo "    Target server hostname: \c"
                    read TargetHostname
                    ping_machine $TargetHostname
		            ;;
                 2)
                    echo "    Target host IP address (ex: 1.2.3.4): \c"
                    read TargetHostIPaddress
                    ping_machine $TargetHostIPaddress
		            ;;
                 3)
                    echo "    Subnetmask (ex: 255.255.255.0): \c"
                    read Subnetmask
		            ;;
                 4)
                    echo "    Network interface (ex: en0): \c"
                    read NetworkInterface
		            ;;
                 5)
                    echo "    Name Server IP address: \c"
                    read NameServerIPaddress
                    ping_machine $NameServerIPaddress
		            ;;
                 6)
                    echo "    Domain name (ex: subdomain.domain.com): \c"
                    read DomainName
                    SimpleHostName=`echo "$TargetHostname" | cut -f1 -d "."`
                    ping_machine "$SimpleHostName.$DomainName"
		            ;;
                 7)
                    echo "    Default Gateway IP address: \c"
                    read GatewayIPaddress
                    ping_machine $GatewayIPaddress
		            ;;
                 8)
                    ValidAnswer=0
                    while [ $ValidAnswer -ne 1 ]
                    do
                        echo "    Cost for default route (-C arg to mktcpip, ex: 0): \c"
                        read Cost
                        # Check that the cost is a valid number
                        if echo $Cost | /bin/grep "[^0-9"] 2>&1 > /dev/null
                        then
                          echo "        ERROR: the cost value is not a valid number."
                        else
                          ValidAnswer=1
                        fi
                    done
		            ;;
                 9)
                    ValidAnswer=0
                    while [ $ValidAnswer -ne 1 ]
                    do
                        echo "    Do active dead gateway detection on default route? (-A arg to mktcpip, enter yes or no): \c"
                        read ActiveDeadGatewayDetection
                        if [ "$ActiveDeadGatewayDetection" != "yes" -a "$ActiveDeadGatewayDetection" != "no" ]
                        then
                            echo "        Invalid input [yes or no]."
                        else
                            ValidAnswer=1
                        fi
                    done
		            ;;
                 10)
                    ValidAnswer=0
                    while [ $ValidAnswer -ne 1 ]
                    do
                        echo "    CableType (-t arg to mktcpip, dix/bnc or N/A): \c"
                        read CableType
                        if [ "$CableType" != "dix" -a "$CableType" != "bnc" -a "$CableType" != "N/A" ]
                        then
                            echo "        Invalid input [dix/bnc or N/A]."
                        else
                            ValidAnswer=1
                        fi
                    done
		            ;;
                 11)
                    echo "    Network adapter (ex.: inet0): \c"
                    read NetworkAdapter
                    ;;
                 12)
                    ValidAnswer=0
                    while [ $ValidAnswer -ne 1 ]
                    do
                        echo "    TDMF UNIX Replicator group number: \c"
                        read GroupNumber
                        # Check that the group number is valid
                        if echo $GroupNumber | /bin/grep "[^0-9"] 2>&1 > /dev/null
                        then
                            echo "        ERROR: the group identifier is not a valid number."
                        else
                            # Check that the group number does not exceed the maximum
                            if [ $GroupNumber -gt 999 ]
                            then
                              echo "        ERROR: the group number maximum value is 999."
                            else
                                ValidAnswer=1
                            fi
                        fi
                    done
                    ;;
	              *)
                    echo "ERROR: input problem."
                    exit 1
		            ;;
            esac
        fi
    done
fi

# Save the input in the network parameter file that will be read at failover boot on the target
FailoverBootParmFile="/etc/SFTKdtc_rootvg_failover_network_parms.txt"
echo "TargetHostname: $TargetHostname" > $FailoverBootParmFile
echo "TargetHostIPaddress: $TargetHostIPaddress" >> $FailoverBootParmFile
echo "Subnetmask: $Subnetmask" >> $FailoverBootParmFile
echo "NetworkInterface: $NetworkInterface" >> $FailoverBootParmFile
echo "NameServerIPaddress: $NameServerIPaddress" >> $FailoverBootParmFile
echo "DomainName: $DomainName" >> $FailoverBootParmFile
echo "GatewayIPaddress: $GatewayIPaddress" >> $FailoverBootParmFile
echo "Cost: $Cost" >> $FailoverBootParmFile
echo "ActiveDeadGatewayDetection: $ActiveDeadGatewayDetection" >> $FailoverBootParmFile
echo "CableType: $CableType" >> $FailoverBootParmFile
echo "NetworkAdapter: $NetworkAdapter" >> $FailoverBootParmFile
echo "GroupNumber: $GroupNumber" >> $FailoverBootParmFile

echo "The network configuration parameters have been saved in /etc/SFTKdtc_rootvg_failover_network_parms.txt."
echo "These values will be used to configure the network when switching over to the replicated root drive on the target server $TargetHostname." 
echo " "

# Copy the boot-failover network reconfig script to /etc to make sure it is accessible at boot time on the target.
copy_file "/etc/dtc/lib/SFTKdtc_AIX_failover_boot_network_reconfig.sh" "/etc/SFTKdtc_AIX_failover_boot_network_reconfig.sh"
/usr/bin/chmod +x /etc/SFTKdtc_AIX_failover_boot_network_reconfig.sh

# Create the flag file: /etc/SFTKdtc_call_mktcpip_at_boot for when the target will failover-reboot.
# Note: this file will be seen only at failover boot with the replicated rootvg, since it is replicated
# to the new target boot drive, not the original one.
/usr/bin/touch /etc/SFTKdtc_call_mktcpip_at_boot

# THIS MUST BE THE LAST PART OF THIS SCRIPT, BECAUSE IT MAY EXIT IN CERTAIN CONDITIONS
# Add an instruction to /etc/rc.tcpip so that the target server will call our script to reconfigure the network at failover boot,
# unless, for some reason, the current one already has the modification.
/usr/bin/grep SFTKdtc_AIX_failover_boot_network_reconfig /etc/rc.tcpip 1> /dev/null
AlreadyChanged=$?
if [ $AlreadyChanged -eq 0 ]
then
    echo "The /etc/rc.tcpip script already contains the line to call our script SFTKdtc_AIX_failover_boot_network_reconfig.sh. Command completed."
    exit 0
fi

# Backup the original rc.tcpip.
# Take note of the original length of the file and make sure we are not copying from a corrupted file
# (problem which has been seen in the past with rcedit, which uses the same method).
numrclines=`wc -l /etc/rc.tcpip | awk {'print $1'}`
if [ $numrclines -eq 0 ] 
then
    echo "/etc/rc.tcpip has zero length before editing. Abnormal situation. Exiting."
    exit 1
fi
# Note: copy_file verifies the copy result; we can rely on the copy
RcTcpipBackupFile="/etc/rc.tcpip.preSFTKdtc"
copy_file "/etc/rc.tcpip" "$RcTcpipBackupFile"
chmod 774 $RcTcpipBackupFile

# Check for the context to be modified 
lnum=`cat $RcTcpipBackupFile | sed -n -e '/^start()/='`
if [ "${lnum}" = "" ]
then
    echo "Cannot determine the placement of the call to add in /etc/rc.tcpip."
    exit 1
fi

# Now we have to point past the "{" following start(): so add 1 for insertion location.
lnum=`expr ${lnum} + 1`
echo "Inserting call to SFTKdtc_AIX_failover_boot_network_reconfig.sh at line number $lnum of /etc/rc.tcpip."

# Make a sed command to add the line to call our script SFTKdtc_AIX_failover_boot_network_reconfig.sh at failover boot
# WARNING: if you modify what is put in the sed cmd file or the final rc file, you must check that the
#          number of lines matches those in the verification done after
rctcpipsedtmp="/tmp/SFTKdtc_rctcpip_sed_temp"
sedcmdlines=4
linestoinsert=3
cat > ${rctcpipsedtmp} << EOSEDCMD
${lnum} a\\
        # @@@ CALL TO SFTKdtc FAILOVER NETWORK RECONFIGURATION SCRIPT + @@@\\
        /etc/SFTKdtc_AIX_failover_boot_network_reconfig.sh\\
        # @@@ CALL TO SFTKdtc FAILOVER NETWORK RECONFIGURATION SCRIPT - @@@
EOSEDCMD

# Verify that we successfully created the sed cmd file before attempting to apply it
# IMPORTANT: if you change the contents of the sed cmd file, you must adjust the number of lines
# in the verification that follows:
numsedlines=`wc -l ${rctcpipsedtmp} | awk {'print $1'}`
if [ $numsedlines -lt $sedcmdlines ] 
then
    echo "ERROR: preparing sed command file ${rctcpipsedtmp} for editing /etc/rc.tcpip failed."
    echo "expected number of lines in ${rctcpipsedtmp}: $sedcmdlines; actual: $numsedlines."
    echo "this error is most likely caused by /tmp being full."
    rm -f ${rctcpipsedtmp} 1> /dev/null 2>&1
    rm -f $RcTcpipBackupFile 1> /dev/null 2>&1
    exit 1
fi

# Apply the patch.
# Make sure we are not corrupting nor zeroing the rc.tcpip file due to some sed error (problem seen in the past with rcedit)
numrclines=`wc -l /etc/rc.tcpip | awk {'print $1'}`
cat $RcTcpipBackupFile | sed -f ${rctcpipsedtmp} > /etc/rc.tcpip
# Make sure the resulting script is executable
chmod 774 /etc/rc.tcpip

newrclines=`wc -l /etc/rc.tcpip | awk {'print $1'}`
expectedlines=`expr ${numrclines} + ${linestoinsert}`
if [ $newrclines -ne $expectedlines ] 
then
    echo "ERROR: an error occured while trying to insert the call to SFTKdtc_AIX_failover_boot_network_reconfig.sh in rc.tcpip."
    echo "This error is most likely caused by the filesystem associated with rc.tcpip being full."
    echo "Original number of lines in /etc/rc.tcpip: $numrclines; new number of lines: $newrclines; expected: $expectedlines"
    echo "Preserving original /etc/rc.tcpip and removing the failover flag file /etc/SFTKdtc_call_mktcpip_at_boot"
    echo "and also the rootvg failover script that was just copied to /etc/SFTKdtc_AIX_failover_boot_network_reconfig.sh."
    rm -f /etc/rc.tcpip 1> /dev/null 2>&1
    mv $RcTcpipBackupFile /etc/rc.tcpip
    chmod 774 /etc/rc.tcpip
    rm -f /etc/SFTKdtc_call_mktcpip_at_boot
    rm -f /etc/SFTKdtc_AIX_failover_boot_network_reconfig.sh
    rm -f ${rctcpipsedtmp}
    sync
    exit 1
fi

# clean up
rm -f ${rctcpipsedtmp}

# Prompt for verification of rc.tcpip
echo "";
echo "    Please inspect /etc/rc.tcpip";
echo "";
echo "    the following lines have been added after start()";
echo "";
echo "      # @@@ CALL TO SFTKdtc FAILOVER NETWORK RECONFIGURATION SCRIPT + @@@\\"
echo "      /etc/SFTKdtc_AIX_failover_boot_network_reconfig.sh\\"
echo "      # @@@ CALL TO SFTKdtc FAILOVER NETWORK RECONFIGURATION SCRIPT - @@@"
echo "";

exit 0

