#! /bin/sh
########################################################################
#
#  dtc_Linux_bootdrive_post_failover
#
# LICENSED MATERIALS / PROPERTY OF IBM
# (c) Copyright IBM 2012.  All Rights Reserved.
# The source code for this program is not published or otherwise divested of
# its trade secrets, irrespective of what has been deposited with the U.S.
# Copyright Office.
# US Government Users Restricted Rights - Use, duplication or disclosure
# restricted by GSA ADP Schedule Contract with IBM Corp.
#
#
########################################################################
#
# The purpose of this script is to automate as much as possible
# the final sequence of operations to perform in order to complete
# the migration of a Linux boot drive from one machine to another,
# such as in a P2V case, where we want to keep the same target machine
# (physical or VM) but change its root (boot) drive with the image of a
# source server migrated boot drive.
#
########################################################################

#-------------------------------------------------------------------------------
##### FUNCTION TO SHOW THE COMMAND USAGE ####
show_usage() {

    echo "----------------------------------------------------------------------------------------------------------------------------"
    echo "USAGE:"
    echo "  dtc_Linux_bootdrive_post_failover -g <group number> [-n] [-q] [-h]"
    echo "----------------------------------------------------------------------------------------------------------------------------"
    echo "  This script performs the final sequence of operations in order to complete the"
    echo "  migration of a Linux boot drive from one machine to another, such as in a P2V case."
    echo "  -g: specifies the group number;"
    echo "  -n: specifies that the original network configuration files of the target server must be restored prior to switch over;"
    echo "  -q: quiet mode; just log messages and results; do not print them on server console;"
    echo "  -h: display this usage paragraph."
    echo "------------------------------------------------------------------------------------------------------------------------------"
}


#-------------------------------------------------------------------------------
##### FUNCTION TO LOG A MESSAGE AND DISPLAY IT IF VERBOSE IS SET ####
# Arguments:
# $1: message string
log_message() {

    datestr=`date "+%Y/%m/%d %T"`;
    errhdr="[${datestr}] dtc_Linux_bootdrive_post_failover: ";
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

    IsRHEL7=0

    Platform=`/bin/uname`
    if [ $Platform = "Linux" ]; then
        logf=/var/opt/SFTKdtc/dtcerror.log;
        DtcLibPath=/etc/opt/SFTKdtc;
        DtcBinPath=/opt/SFTKdtc/bin;
	    COMMAND_USR=`/usr/bin/whoami`;
        PartitionProbe=`which partprobe`;
        ZcatCommand=`which zcat`;
        CpioCommand=`which cpio`;
        LsinitrdCommand=`which lsinitrd`;
        KernelVersion=`/bin/uname -r`;
        # Determine if we are on Redhat or SuSE
        release_file="/etc/redhat-release"
        /bin/ls $release_file 1> /dev/null 2>&1
        file_found=$?
        if [ $file_found -eq 0 ]
        then
            IsRedHat=1
            if /usr/bin/fgrep -q 'release 7' /etc/redhat-release 2> /dev/null
            then
                IsRHEL7=1
            fi
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
    else
        log_message "This script can be executed only on Linux." 
        exit 1
    fi
}

#-------------------------------------------------------------------------------
##### FUNCTION TO PARSE THE ARGUMENTS PROVIDED TO THIS SCRIPT #####
parse_command_arguments() {

    ShowUsage=0
    Verbose=1
    GroupNumber=1000
    RestoreTargetNetworkConfig=0
    NoPrompt=0
    error=0

    while getopts "g:nqph" opt
    do
            case $opt in
            g)
                    GroupNumber=$OPTARG;;
            n)
                    RestoreTargetNetworkConfig=1;;
            h)
                    ShowUsage=1;;
            q)
                    Verbose=0;;
            p)
                    NoPrompt=1;;
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
    if /bin/echo $GroupNumber | grep "[^0-9"] 2>&1 > /dev/null
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
##### FUNCTION TO CHECK THE TYPE OF CONFIGURATION ON THIS SERVER #####
# This script can only be run on the RMD side of a 1-to-1 configuration.
# Arguments:
# $1: group number
CheckConfigurationType() {

    group=$1

    # Format the 3-digit group number for PMD config file name
    FormatGroupNumberSubstring $group

    # Check if a PMD configuration file exists on this machine
    PMDfilename="$DtcLibPath/p$GroupNumberSubstring.cfg"
    /bin/ls $PMDfilename 1> /dev/null 2>&1
    result=$?
    if [ $result -eq 0 ]
    then
        # This group has a PMD cfg file on this machine; check if it has
        # an RMD cfg file; if not, this is the PMD side of a 1-to-1 cfg and
        # this script cannot run here.
        RMDfilename="$DtcLibPath/s$GroupNumberSubstring.cfg"
        /bin/ls $RMDfilename 1> /dev/null 2>&1
        result=$?
        if [ $result -ne 0 ]
        then
            log_message "This script cannot be run on the PMD side of a 1-to-1 configuration."
            exit 1
        else
            # RMD cfg file found; check if we are in a loopback config
            loopback=`/bin/grep "HOST" $RMDfilename | /usr/bin/uniq -d | /usr/bin/wc -l | /bin/awk '{print $1}'`
            if [ $loopback -ne 0 ]
            then
                log_message "You do not need to run this script in a loopback configuration."
                exit 1
            fi
        fi
    fi
}

##### FUNCTION TO SET THE DEVICE NAME OF THE TARGET DEVICE USED FOR MIGRATION #####
# Arguments:
# $1: group number
SetMigrationDeviceName() {

    group=$1

    # Format the 3-digit group number for RMD config file name
    FormatGroupNumberSubstring $GroupNumber

    RMDfilename="$DtcLibPath/s$GroupNumberSubstring.cfg"
    # Take only the first mirror device name in the file (first pair) if there is more than one pair
    # of devices and log a warning
    numofpairs=`/bin/grep MIRROR-DISK $RMDfilename | /bin/grep -v "# " | /usr/bin/wc -l | /bin/awk '{print $1}'`
    if [ $numofpairs -gt 1 ]
    then
        log_message "WARNING: the configuration group has more than 1 pair of devices; it is expected that pair 1 contains the boot partition and OS. Proceeding..."
    fi
    TargetDeviceName=`/bin/grep -v "# " $RMDfilename | /bin/grep -m 1 MIRROR-DISK | /bin/awk '{print $2}'`

}


#--------------------------------------------------------------------------------------
#                        MAIN LOGIC FOR SuSE POSTFAILOVER
#--------------------------------------------------------------------------------------
SuSE_Main_Logic() {

    #         GET ACCESS TO THE MIGRATED BOOT PARTITION
    # Determine which device was used as target for the migration of the boot drive
    SetMigrationDeviceName
    # Make sure the OS is aware of the migrated partitions
    $PartitionProbe $TargetDeviceName
    sleep 3

    # Determine which partition of the migration drive is the boot partition
    BootPartition=`/sbin/fdisk -l | /bin/grep $TargetDeviceName | /bin/grep " * " | /bin/grep -v Disk | /bin/grep -v -i swap | /bin/awk '{print $1}'`

    # Mount the migrated boot partition
    /bin/mkdir /SFTKdtc_migrated_boot_partition 1> /dev/null 2>&1
    /bin/mount $BootPartition /SFTKdtc_migrated_boot_partition 1> /dev/null 2>&1
    result=$?
    if [ $result -ne 0 ]
    then
        # Check if it is already mounted on the expected mount point:
        /bin/mount | /bin/grep $BootPartition | /bin/grep SFTKdtc_migrated_boot_partition 1> /dev/null 2>&1
        result2=$?
        if [ $result2 -ne 0 ]
        then
            log_message "Failed mounting migrated boot partition ($BootPartition) on /SFTKdtc_migrated_boot_partition: error code = $result."
            exit 1
        fi
    fi

    #         VERIFY BOOT LOADER CONFIG FILE REGARDING SERIAL PORT USAGE
    # Check if the target server uses a serial port at boot time
    SourceUsesSerialPort=0
    log_message "Verifying specification of a serial port in the boot loader configuration file..."
    TargetUsesSerialPort=`/bin/grep "console=" /boot/grub/menu.lst | /bin/grep -v "#" | /usr/bin/wc -l | /bin/awk '{print $1}'`
    if [ $TargetUsesSerialPort -eq 0 ]
    then
        # If the target VM does not use a console definition, check if the source server from which we migrated the boot drive uses a serial console
        SourceUsesSerialPort=`/bin/grep "console=" /SFTKdtc_migrated_boot_partition/boot/grub/menu.lst | /bin/grep -v "#" | /usr/bin/wc -l | /bin/awk '{print $1}'`
    
        # If so, log a message to warn the user that at switch over a serial port may be expected,
        # and if it is a VM, its output can be redirected to a datastore file. If there are problems,
        # this datastore output file will be accessible regardless of boot success or failure.
        if [ $SourceUsesSerialPort -ne 0 ]
        then
            log_message "  The source server from which the boot drive was migrated used a serial port at boot time, whereas the target server did not."
            log_message "  At switch over to the new boot drive, some configuration work may be needed (such as removing the console specification, or \
                           configuring a serial port and redirecting the output to a file) if you do not get the boot time messages."
            log_message "  If you do not get boot messages on main console, wait and check by typing the <Enter> key to get the loggin prompt."
            log_message "  The serial port specification is defined in the bootloader configuration file /boot/grub/menu.lst."
            log_message "  Before switch over, you can mount $BootPartition and look into the boot/grub directory."
        fi
    fi

    #            SET THE TARGET NETWORK CONFIGURATION FROM ITS ORIGINAL CONFIGURATION
    # The files to restore are: /etc/hosts and /etc/sysconfig/network/<all files>
    if [ $RestoreTargetNetworkConfig -ne 0 ]
    then
        log_message "Copying network config files /etc/hosts and /etc/sysconfig/network/<all files> to the migrated boot partition $BootPartition..."
        /bin/mv /SFTKdtc_migrated_boot_partition/etc/hosts /SFTKdtc_migrated_boot_partition/etc/hosts_migrated_from_source_server
        /bin/cp -p /etc/hosts /SFTKdtc_migrated_boot_partition/etc/.
        /bin/mv /SFTKdtc_migrated_boot_partition/etc/sysconfig/network /SFTKdtc_migrated_boot_partition/etc/sysconfig/network_migrated_from_source_server
        /bin/cp -pr /etc/sysconfig/network /SFTKdtc_migrated_boot_partition/etc/sysconfig/.

        if [ -e /etc/yp.conf ]
        then
            log_message "Copying NIS server config file /etc/yp.conf to the migrated boot partition $BootPartition..."
            /bin/mv /SFTKdtc_migrated_boot_partition/etc/yp.conf /SFTKdtc_migrated_boot_partition/etc/yp.conf.migrated_from_source_server
            /bin/cp -p /etc/yp.conf /SFTKdtc_migrated_boot_partition/etc/.
        fi

        hostname_to_restore=`/bin/hostname -f | /bin/awk '{print $1}'`
        log_message "Setting hostname $hostname_to_restore in the migrated /etc/HOSTNAME file of $BootPartition..."
        /bin/mv /SFTKdtc_migrated_boot_partition/etc/HOSTNAME /SFTKdtc_migrated_boot_partition/etc/HOSTNAME_migrated_from_source_server
        /bin/echo $hostname_to_restore > /SFTKdtc_migrated_boot_partition/etc/HOSTNAME

        # Restore also the udev rules
        log_message "Copying /etc/udev/rules.d/<all files> to the migrated boot partition $BootPartition"
        /bin/mv /SFTKdtc_migrated_boot_partition/etc/udev/rules.d /SFTKdtc_migrated_boot_partition/etc/udev/rules.d_migrated_from_source_server
        /bin/cp -pr /etc/udev/rules.d /SFTKdtc_migrated_boot_partition/etc/udev/. 

        /bin/sync
    fi

    #            VERIFY IF THE MIGRATED INITRD FILE CAN POTENTIALLY CAUSE PROBLEMS
    # Determine the names of the initrd files of both the source and target servers
    log_message "Verifying migrated initrd file..."
    SourceInitrd=`/bin/grep -m 1 initrd /SFTKdtc_migrated_boot_partition/boot/grub/menu.lst | /bin/grep -v "# " | /bin/awk '{print $2}' | /bin/sed s,/boot,,g`
    TargetInitrd=`/bin/grep -m 1 initrd /boot/grub/menu.lst | /bin/grep -v "# " | /bin/awk '{print $2}' | /bin/sed s,/boot,,g`
    /usr/bin/cmp /SFTKdtc_migrated_boot_partition/boot$SourceInitrd /boot$TargetInitrd 1> /dev/null 2>&1
    InitrdDiffer=$?
    if [ $InitrdDiffer -eq 1 ]
    then
        # The initrd files differ; check if kernel modules are missing in the initrd that will become the active one.
        # Extract the list of modules from the files.
        # Target's original initrd:
        /bin/mkdir /tmp/contents_of_target_original_initrd 1> /dev/null 2>&1
        cd /tmp/contents_of_target_original_initrd
        /bin/cp -p /boot/$TargetInitrd .
        $ZcatCommand ./$TargetInitrd | $CpioCommand -tv --quiet > ./contents_of_initrd.txt
        /bin/grep ".ko" ./contents_of_initrd.txt > ./kernel_objects.txt
        # The above provides a list of the kernel modules in an ls -l format.
        # We want to extract only the kernel module names, sorted:
        /bin/cat ./kernel_objects.txt | /bin/awk '{print $9}' | /bin/sed s,lib/,,g | /bin/sort > ./kernel_module_names.txt

        # Source's initrd:
        /bin/mkdir /tmp/contents_of_source_original_initrd 1> /dev/null 2>&1
        cd /tmp/contents_of_source_original_initrd
        /bin/cp -p /SFTKdtc_migrated_boot_partition/boot/$SourceInitrd .
        $ZcatCommand ./$SourceInitrd | $CpioCommand -tv --quiet > ./contents_of_initrd.txt
        /bin/grep ".ko" ./contents_of_initrd.txt > ./kernel_objects.txt
        /bin/cat ./kernel_objects.txt | /bin/awk '{print $9}' | /bin/sed s,lib/,,g | /bin/sort > ./kernel_module_names.txt

        # Determine if some of the target's original initrd modules are missing from the source migrated initrd:
        found_missing_module=0
        module_in_dmesg_output=1
        cd /tmp
        logged_message=0
        for ModuleName in `/usr/bin/diff ./contents_of_target_original_initrd/kernel_module_names.txt ./contents_of_source_original_initrd/kernel_module_names.txt | /bin/grep "< " | /bin/awk '{print $2}'`
        do
           if [ $logged_message -eq 0 ]
           then
               logged_message=1
               log_message "Some module(s) of the original Target server initrd is (are) not in the migrated Source initrd which will become the active one:"
               /bin/sync
               /bin/sleep 2 # To avoid line mix up in the DMC Events tab
           fi
           found_missing_module=1
           strippedModuleName=`/bin/echo $ModuleName | /bin/sed s,.ko,,g`
           /bin/dmesg | /bin/grep -i $strippedModuleName 1> /dev/null 2>&1
           module_in_dmesg_output=$?
           if [ $module_in_dmesg_output -eq 0 ]
           then
               log_message "$strippedModuleName (missing from new initrd and appears in previous boot messages of the target Server, potential problems)"
           else
               log_message "$strippedModuleName (missing from new initrd but not in previous boot messages of the target Server, might not cause problems)"
           fi
        done
        if [ $found_missing_module -eq 1 ]
        then
            /bin/sleep 1 # For message time stamp to avoid mix up on DMC
            log_message "Missing modules in the migrated initrd file might cause problems upon booting from the migrated drive."
            /bin/sleep 1
            backup_initrd_name_suffix="_migrated_from_source_server"
            log_message "Renaming the migrated initrd /SFTKdtc_migrated_boot_partition/boot$SourceInitrd to /SFTKdtc_migrated_boot_partition/boot$SourceInitrd$backup_initrd_name_suffix"
            /bin/mv /SFTKdtc_migrated_boot_partition/boot$SourceInitrd /SFTKdtc_migrated_boot_partition/boot$SourceInitrd$backup_initrd_name_suffix
            /bin/sleep 1
            log_message "Copying the original initrd /boot$TargetInitrd of this target machine to the migrated boot volume $SFTKdtc_migrated_boot_partition as $SourceInitrd"
            /bin/cp -p /boot$TargetInitrd /SFTKdtc_migrated_boot_partition/boot$SourceInitrd
        else
            log_message "No missing module found in migrated initrd file."
        fi
    fi
    /bin/sync
    /bin/sleep 2
    if [ $SourceUsesSerialPort -ne 0 -o $found_missing_module -eq 1 ]
    then
        log_message "Please look at messages logged regarding serial port usage and/or initrd and take actions if applicable. \
                     After that you will be ready to switch over to the migrated boot drive."
    fi

    #             CHECK fstab and menu.lst
    # Now check if fstab and/or menu.lst use soft links to locate the root filesystem and swap partitions.
    # If so, the source server will have prepared files to alow identifying the real physical partitions
    log_message "Verifying if swap and root definitions need to be adjusted in the migrated /SFTKdtc_migrated_boot_partition/etc/fstab..."
    if [ -e /SFTKdtc_migrated_boot_partition/boot/SFTKdtc_check_fstab_swap_root_definitions ]
    then
        # 1) fstab, swap definition
        /bin/grep -i swap /SFTKdtc_migrated_boot_partition/etc/fstab | /bin/grep -v "#" 1> /dev/null 2>&1
        result=$?
        if [ $result -eq 0 ]
        then
            # Swap partition is defined in migrated fstab; check it
            SwapPartitionLink=`/bin/grep -i swap /SFTKdtc_migrated_boot_partition/boot/SFTKdtc_check_fstab_swap_root_definitions | /bin/awk '{print $2}'`
            SwapRealPartition=`/bin/grep -i swap /SFTKdtc_migrated_boot_partition/boot/SFTKdtc_check_fstab_swap_root_definitions | /bin/awk '{print $3}'`
            first_line=1
            while read line
            do
                partition=`/bin/echo $line | /bin/awk '{print $1}'`
                if [ "$partition" = "$SwapPartitionLink" ]
                then
                    # Replace the link by the real partition name
                    line=`/bin/echo $line | /bin/sed s,$SwapPartitionLink,$SwapRealPartition,g`
                fi
                if [ $first_line -eq 1 ]
                then
                    /bin/echo $line > /SFTKdtc_migrated_boot_partition/etc/SFTKdtc_new_fstab
                    first_line=0
                else
                    /bin/echo $line >> /SFTKdtc_migrated_boot_partition/etc/SFTKdtc_new_fstab
                fi
            done < /SFTKdtc_migrated_boot_partition/etc/fstab
        fi
        # Rename the old and the new fstab files; we must work from the new fstab for the next part checking the root partition
        /bin/mv /SFTKdtc_migrated_boot_partition/etc/fstab /SFTKdtc_migrated_boot_partition/etc/SFTKdtc_fstab_migrated_from_source_server
        /bin/cp /SFTKdtc_migrated_boot_partition/etc/SFTKdtc_new_fstab /SFTKdtc_migrated_boot_partition/etc/fstab
        log_message "Renamed the swap partition link $SwapPartitionLink to its real partition name $SwapRealPartition in the migrated fstab file."

        # 2) fstab, root definition
        /bin/grep " / " /SFTKdtc_migrated_boot_partition/etc/fstab | /bin/grep -v "#" 1> /dev/null 2>&1
        result=$?
        if [ $result -eq 0 ]
        then
            # root partition is defined in migrated fstab; check it
            RootPartitionLink=`/bin/grep -i root /SFTKdtc_migrated_boot_partition/boot/SFTKdtc_check_fstab_swap_root_definitions | /bin/awk '{print $2}'`
            RootRealPartition=`/bin/grep -i root /SFTKdtc_migrated_boot_partition/boot/SFTKdtc_check_fstab_swap_root_definitions | /bin/awk '{print $3}'`
            first_line=1
            while read line
            do
                partition=`/bin/echo $line | /bin/awk '{print $1}'`
                if [ "$partition" = "$RootPartitionLink" ]
                then
                    # Replace the link by the real partition name
                    line=`/bin/echo $line | /bin/sed s,$RootPartitionLink,$RootRealPartition,g`
                fi
                if [ $first_line -eq 1 ]
                then
                    /bin/echo $line > /SFTKdtc_migrated_boot_partition/etc/SFTKdtc_new_fstab
                    first_line=0
                else
                    /bin/echo $line >> /SFTKdtc_migrated_boot_partition/etc/SFTKdtc_new_fstab
                fi
            done < /SFTKdtc_migrated_boot_partition/etc/fstab
        fi
        # Rename the new fstab file
        /bin/cp -f /SFTKdtc_migrated_boot_partition/etc/SFTKdtc_new_fstab /SFTKdtc_migrated_boot_partition/etc/fstab
        log_message "Renamed the root partition link $RootPartitionLink to its real partition name $RootRealPartition in the migrated fstab file."
    else
        log_message "Did not find the /SFTKdtc_migrated_boot_partition/boot/SFTKdtc_check_fstab_swap_root_definitions file."
    fi

    # Now the boot loader configuration file menu.lst
    log_message "Verifying if swap and root definitions need to be adjusted in the migrated /SFTKdtc_migrated_boot_partition/boot/grub/menu.lst..."
    if [ -e /SFTKdtc_migrated_boot_partition/boot/SFTKdtc_check_menu_lst_swap_root_definitions ]
    then
        # 3) menu.lst, swap definition (keyword "resume")
        /bin/grep -i resume /SFTKdtc_migrated_boot_partition/boot/grub/menu.lst | /bin/grep -v "#" 1> /dev/null 2>&1
        result=$?
        if [ $result -eq 0 ]
        then
            # Swap partition is defined in migrated menu.lst; check it
            SwapPartitionLink=`/bin/grep -i swap /SFTKdtc_migrated_boot_partition/boot/SFTKdtc_check_menu_lst_swap_root_definitions | /bin/awk '{print $2}'`
            SwapRealPartition=`/bin/grep -i swap /SFTKdtc_migrated_boot_partition/boot/SFTKdtc_check_menu_lst_swap_root_definitions | /bin/awk '{print $3}'`
            first_line=1
            while read line
            do
                /bin/echo $line | /bin/grep $SwapPartitionLink | /bin/grep -v "#" 1> /dev/null 2>&1
                result=$?
                if [ $result -eq 0 ]
                then
                    # Replace the link by the real partition name
                    line=`/bin/echo $line | /bin/sed s,$SwapPartitionLink,$SwapRealPartition,g`
                fi
                if [ $first_line -eq 1 ]
                then
                    /bin/echo $line > /SFTKdtc_migrated_boot_partition/boot/grub/SFTKdtc_new_menu.lst
                    first_line=0
                else
                    /bin/echo $line >> /SFTKdtc_migrated_boot_partition/boot/grub/SFTKdtc_new_menu.lst
                fi
            done < /SFTKdtc_migrated_boot_partition/boot/grub/menu.lst
        fi
        # Rename the old and the new menu.lst files; we must work from the new menu.lst for the next part checking the root partition
        /bin/mv /SFTKdtc_migrated_boot_partition/boot/grub/menu.lst /SFTKdtc_migrated_boot_partition/boot/grub/SFTKdtc_menu_lst_migrated_from_source_server
        /bin/cp /SFTKdtc_migrated_boot_partition/boot/grub/SFTKdtc_new_menu.lst /SFTKdtc_migrated_boot_partition/boot/grub/menu.lst
        log_message "Renamed the swap partition link $SwapPartitionLink to its real partition name $SwapRealPartition in the migrated menu.lst file."

        # 4) menu.lst, root definition
        RootPartitionLink=`/bin/grep -i root /SFTKdtc_migrated_boot_partition/boot/SFTKdtc_check_menu_lst_swap_root_definitions | /bin/awk '{print $2}'`
        RootRealPartition=`/bin/grep -i root /SFTKdtc_migrated_boot_partition/boot/SFTKdtc_check_menu_lst_swap_root_definitions | /bin/awk '{print $3}'`
        first_line=1
        while read line
        do
            /bin/echo $line | /bin/grep $RootPartitionLink | /bin/grep -v "#" 1> /dev/null 2>&1
            result=$?
            if [ $result -eq 0 ]
            then
                # Replace the link by the real partition name
                line=`/bin/echo $line | /bin/sed s,$RootPartitionLink,$RootRealPartition,g`
            fi
            if [ $first_line -eq 1 ]
            then
                /bin/echo $line > /SFTKdtc_migrated_boot_partition/boot/grub/SFTKdtc_new_menu.lst
                first_line=0
            else
                /bin/echo $line >> /SFTKdtc_migrated_boot_partition/boot/grub/SFTKdtc_new_menu.lst
            fi
        done < /SFTKdtc_migrated_boot_partition/boot/grub/menu.lst
        # Copy the new menu.lst file to the real name
        /bin/cp -f /SFTKdtc_migrated_boot_partition/boot/grub/SFTKdtc_new_menu.lst /SFTKdtc_migrated_boot_partition/boot/grub/menu.lst
        log_message "Renamed the root partition link $RootPartitionLink to its real partition name $RootRealPartition in the migrated menu.lst file."
    else
        log_message "Did not find the /SFTKdtc_migrated_boot_partition/boot/SFTKdtc_check_menu_lst_swap_root_definitions file."
    fi



    /bin/umount /SFTKdtc_migrated_boot_partition

}


#--------------------------------------------------------------------------------------
#                        MAIN LOGIC FOR REDHAT POSTFAILOVER
#--------------------------------------------------------------------------------------
# Arguments: none, all is set in global variables or locally in here
RedHat_Main_Logic() {

    #         GET ACCESS TO THE MIGRATED BOOT PARTITION
    # Determine which device was used as target for the migration of the boot drive
    SetMigrationDeviceName
    # Make sure the OS is aware of the migrated partitions
    $PartitionProbe $TargetDeviceName
    /bin/sleep 3

    # The boot partition is the first partition of the target device used for migration
    BootPartition="$TargetDeviceName"1

    # Mount the migrated boot partition
    /bin/mkdir /SFTKdtc_migrated_bootvol 1> /dev/null 2>&1
    /bin/mount $BootPartition /SFTKdtc_migrated_bootvol 1> /dev/null 2>&1
    result=$?
    if [ $result -ne 0 ]
    then
        # Check if it is already mounted on the expected mount point:
        /bin/mount | /bin/grep $BootPartition | /bin/grep SFTKdtc_migrated_bootvol 1> /dev/null 2>&1
        result2=$?
        if [ $result2 -ne 0 ]
        then
            log_message "Failed mounting migrated boot partition ($BootPartition) on /SFTKdtc_migrated_bootvol: error code = $result."
            exit 1
        fi
    fi

    #         VERIFY BOOT LOADER CONFIG FILE REGARDING SERIAL PORT USAGE
    # Check if the target server uses a splash screen and graphical interface
    SourceUsesSerialPort=0
    log_message "Verifying specification of a serial port in the boot loader configuration file..."
    TargetUsesSplashImage=`/bin/grep splashimage /boot/grub/grub.conf | /usr/bin/wc -l | /bin/awk '{print $1}'`
    if [ $TargetUsesSplashImage -ne 0 ]
    then
        # If it does, check if the source server uses a serial console instead
        SourceUsesSerialPort=`/bin/grep serial /SFTKdtc_migrated_bootvol/grub/grub.conf | /bin/grep -v "# " | /usr/bin/wc -l | /bin/awk '{print $1}'`
    
        # If so, log a message to warn the user that at switch over a serial port will be expected,
        # and if it is a VM, its output can be redirected to a datastore file. If there are problems,
        # this datastore output file will be accessible regardless of boot success or failure.
        if [ $SourceUsesSerialPort -ne 0 ]
        then
            log_message "  The source server from which the boot drive was migrated used a serial port at boot time. \
                           At switch over to the new boot drive, a serial port may be expected."
            log_message "  The serial port specification is defined in the bootloader configuration file /boot/grub/grub.conf. \
                           Before switch over, you can mount $BootPartition and look into the grub directory."
        fi
    fi

    #            PREPARE FOR TARGET NETWORK CONFIGURATION RESTORE IF APPLICABLE
    # The files to backup are: /etc/hosts, /etc/sysconfig/network and /etc/sysconfig/network-scripts/ifcfg-eth0
    if [ $RestoreTargetNetworkConfig -ne 0 ]
    then
        log_message "Copying network config files /etc/hosts, /etc/sysconfig/network and /etc/sysconfig/network-scripts/ifcfg-eth0 \
                     to $BootPartition, in directory backup_original_target_net_config_files..."
        log_message "These network configuration files will be restored to the migrated drive at reboot with new drive."
        /bin/mkdir /SFTKdtc_migrated_bootvol/backup_original_target_net_config_files 1> /dev/null 2>&1
        /bin/cp -p /etc/hosts /SFTKdtc_migrated_bootvol/backup_original_target_net_config_files/.
        /bin/cp -p /etc/sysconfig/network /SFTKdtc_migrated_bootvol/backup_original_target_net_config_files/.
        /bin/cp -p /etc/sysconfig/network-scripts/ifcfg-eth0 /SFTKdtc_migrated_bootvol/backup_original_target_net_config_files/.
        if [ -e /etc/yp.conf ]
        then
            log_message "Copying NIS server config file /etc/yp.conf also for restore at switch over reboot time"
            /bin/cp -p /etc/yp.conf /SFTKdtc_migrated_bootvol/backup_original_target_net_config_files/.
        fi
        # The following is done because the hostname will be set early in the switch over boot sequence, potentially before our boot scripts are run.
        hostname_output=`/bin/hostname | awk '{print $1}'`
        /bin/echo $hostname_output > /SFTKdtc_migrated_bootvol/backup_original_target_net_config_files/hostname_restore.txt

        # Now also the udev rules
        log_message "Copying /etc/udev/rules.d/<files> (except net rules, modified by udev) to the migrated boot partition $BootPartition, in directory backup_original_target_net_config_files..."
        /bin/cp -pr /etc/udev/rules.d /SFTKdtc_migrated_bootvol/backup_original_target_net_config_files/.
        # Keep all the rules in the backup directory except the network rule files, which will get regenerated at switchover by udev.
        /bin/rm -f /SFTKdtc_migrated_bootvol/backup_original_target_net_config_files/rules.d/*-persistent-net.rules
        /bin/sync
        log_message "If the network is not functional after switch over to the migrated boot drive, verify if udev has changed the definition of the NIC in the \
                     /etc/udev/rules.d/xx-persistent-net.rules file. If so, edit the rules file, put back the eth<number> as it was on the target machine before failover and reboot."

        # Create a marker file to indicate that the network configuration files must be restored after switchover to the new boot drive:
        /bin/touch /SFTKdtc_migrated_bootvol/SFTKdtc_restore_net_config_files
        /bin/sync
    fi
    #<<< TODO: do we want an option to set to DHCP mode?

    #            VERIFY IF THE MIGRATED INITRD FILE CAN POTENTIALLY CAUSE PROBLEMS
    # Determine the names of the initrd files of both the source and target servers
    log_message "Verifying migrated initrd file..."
    SourceInitrd=`/bin/grep initrd /SFTKdtc_migrated_bootvol/grub/grub.conf | /bin/grep -v "# " | /bin/awk '{print $2}'`
    TargetInitrd=`/bin/grep initrd /boot/grub/grub.conf | /bin/grep -v "# " | /bin/awk '{print $2}'`
    /usr/bin/cmp /SFTKdtc_migrated_bootvol$SourceInitrd /boot$TargetInitrd 1> /dev/null 2>&1
    InitrdDiffer=$?
    if [ $InitrdDiffer -eq 1 ]
    then
        # The initrd files differ; check if kernel modules are missing in the initrd that will become the active one.
        # Extract the list of modules from the files.
        # Target's original initrd:
        /bin/mkdir /tmp/contents_of_target_original_initrd 1> /dev/null 2>&1
        cd /tmp/contents_of_target_original_initrd
        /bin/cp -p /boot/$TargetInitrd .
        $ZcatCommand ./$TargetInitrd | $CpioCommand -tv --quiet > ./contents_of_initrd.txt
        /bin/grep ".ko" ./contents_of_initrd.txt > ./kernel_objects.txt
        # The above provides a list of the kernel modules in an ls -l format.
        # We want to extract only the kernel module names, sorted:
        /bin/cat ./kernel_objects.txt | /bin/awk '{print $9}' | /bin/sed s,lib/,,g | /bin/sort > ./kernel_module_names.txt

        # Source's initrd:
        /bin/mkdir /tmp/contents_of_source_original_initrd 1> /dev/null 2>&1
        cd /tmp/contents_of_source_original_initrd
        /bin/cp -p /SFTKdtc_migrated_bootvol/$SourceInitrd .
        $ZcatCommand ./$SourceInitrd | $CpioCommand -tv --quiet > ./contents_of_initrd.txt
        /bin/grep ".ko" ./contents_of_initrd.txt > ./kernel_objects.txt
        /bin/cat ./kernel_objects.txt | /bin/awk '{print $9}' | /bin/sed s,lib/,,g | /bin/sort > ./kernel_module_names.txt

        # Determine if some of the target's original initrd modules are missing from the source migrated initrd:
        found_missing_module=0
        module_in_dmesg_output=1
        cd /tmp
        logged_message=0
        for ModuleName in `/usr/bin/diff ./contents_of_target_original_initrd/kernel_module_names.txt ./contents_of_source_original_initrd/kernel_module_names.txt | /bin/grep "< " | /bin/awk '{print $2}'`
        do
           if [ $logged_message -eq 0 ]
           then
               logged_message=1
               log_message "Some module(s) of the original Target server initrd is (are) not in the migrated Source initrd which will become the active one:"
               /bin/sync
               /bin/sleep 2 # To avoid line mix up in the DMC Events tab
           fi
           found_missing_module=1
           strippedModuleName=`/bin/echo $ModuleName | /bin/sed s,.ko,,g`
           /bin/dmesg | /bin/grep -i $strippedModuleName 1> /dev/null 2>&1
           module_in_dmesg_output=$?
           if [ $module_in_dmesg_output -eq 0 ]
           then
               log_message "$strippedModuleName (missing from new initrd and appears in previous boot messages of the target Server, potential problems)"
           else
               log_message "$strippedModuleName (missing from new initrd but not in previous boot messages of the target Server, might not cause problems)"
           fi
        done
        if [ $found_missing_module -eq 1 ]
        then
            /bin/sleep 1 # For message time stamp to avoid mix up on DMC
            log_message "Missing modules in the migrated initrd file might cause problems upon booting from the migrated drive."
            /bin/sleep 1
            backup_initrd_name_suffix="_migrated_from_source_server"
            log_message "Renaming the migrated initrd /SFTKdtc_migrated_bootvol$SourceInitrd to /SFTKdtc_migrated_bootvol$SourceInitrd$backup_initrd_name_suffix"
            /bin/mv /SFTKdtc_migrated_bootvol$SourceInitrd /SFTKdtc_migrated_bootvol$SourceInitrd$backup_initrd_name_suffix
            /bin/sleep 1
            log_message "Copying the original initrd /boot$TargetInitrd of this target machine to the migrated boot volume $BootPartition as $SourceInitrd"
            /bin/cp -p /boot$TargetInitrd /SFTKdtc_migrated_bootvol$SourceInitrd
        else
            log_message "No missing module found in migrated initrd file."
        fi
    fi
    /bin/sync
    /bin/sleep 2
    if [ $SourceUsesSerialPort -ne 0 -o $found_missing_module -eq 1 ]
    then
        log_message "Please look at messages logged regarding serial port usage and/or initrd. \
                     After that you will be ready to switch over to the migrated boot drive."
    fi

    /bin/umount /SFTKdtc_migrated_bootvol

}


#--------------------------------------------------------------------------------------
#                        MAIN LOGIC FOR REDHAT 7 POSTFAILOVER
#--------------------------------------------------------------------------------------
# Arguments: none, all is set in global variables or locally in here
RHEL7_Main_Logic() {

    #         GET ACCESS TO THE MIGRATED BOOT PARTITION
    # Determine which device was used as target for the migration of the boot drive
    SetMigrationDeviceName
    # Make sure the OS is aware of the migrated partitions
    $PartitionProbe $TargetDeviceName
    /bin/sleep 3

    # The boot partition is the first partition of the target device used for migration
    BootPartition="$TargetDeviceName"1
    log_message "From the configuration file, the migrated boot volume is on $TargetDeviceName and the boot partition is $BootPartition."

    # Mount the migrated boot partition
    /bin/mkdir /SFTKdtc_migrated_bootvol 1> /dev/null 2>&1
    /bin/mount $BootPartition /SFTKdtc_migrated_bootvol 1> /dev/null 2>&1
    result=$?
    if [ $result -ne 0 ]
    then
        # Check if it is already mounted on the expected mount point:
        /bin/mount | /bin/grep $BootPartition | /bin/grep SFTKdtc_migrated_bootvol 1> /dev/null 2>&1
        result2=$?
        if [ $result2 -ne 0 ]
        then
            log_message "Failed mounting migrated boot partition ($BootPartition) on /SFTKdtc_migrated_bootvol: error code = $result."
            exit 1
        fi
    fi
    log_message "Boot partition $BootPartition mounted on /SFTKdtc_migrated_bootvol."

    # Get the source server kernel version stored by the source in the file /boot/SFTKdtc_source_kernel_version.txt
    # Check if the source kernel version file exists; if not, assume same kernel version for both source and target servers.
    if [ -e /SFTKdtc_migrated_bootvol/SFTKdtc_source_kernel_version.txt ]
    then
       SourceKernelVersion=`/bin/cat /SFTKdtc_migrated_bootvol/SFTKdtc_source_kernel_version.txt | /bin/awk '{print $1}'`
       log_message "Source server kernel version: $SourceKernelVersion"
    else
        log_message "Did not find the file /SFTKdtc_migrated_bootvol/SFTKdtc_source_kernel_version.txt. Will assume same kernel version as target server: $KernelVersion."
        SourceKernelVersion=$KernelVersion
    fi

    # Make sure we can locate the menuentry in the grub configuration files for the kernel verions of both the source and target servers:
    ok_to_check_serial_port_and_initrd=1
    /bin/grep menuentry /boot/grub2/grub.cfg | /bin/grep $KernelVersion 1> /dev/null 2>&1
    found_target_menu_entry=$?
    if [ $found_target_menu_entry -ne 0 ]
    then
        log_message "WARNING: did not find the menu entry of the target server for kernel version $KernelVersion in /boot/grub2/grub.cfg"
        log_message "Will not be able to verify usage of a serial port nor to compare the initrd files of the source and target servers but will proceed with the sequence."
        ok_to_check_serial_port_and_initrd=0
    fi
    /bin/grep menuentry /SFTKdtc_migrated_bootvol/grub2/grub.cfg | /bin/grep $SourceKernelVersion 1> /dev/null 2>&1
    found_source_menu_entry=$?
    if [ $found_source_menu_entry -ne 0 ]
    then
        log_message "WARNING: did not find the menu entry of the source server for kernel version $SourceKernelVersion in /SFTKdtc_migrated_bootvol/grub2/grub.cfg"
        log_message "Will not be able to verify usage of a serial port nor to compare the initrd files of the source and target servers but will proceed with the sequence."
        ok_to_check_serial_port_and_initrd=0
    fi

    if [ $ok_to_check_serial_port_and_initrd -eq 1 ]
    then
        #         VERIFY BOOT LOADER CONFIG FILE REGARDING SERIAL PORT USAGE
        # Documented at: https://access.redhat.com/documentation/en-US/Red_Hat_Enterprise_Linux/7/html/System_Administrators_Guide/sec-GRUB_2_over_Serial_Console.html
        # Check if the target server uses a splash screen and graphical interface or a serial port. And then check the source server.
        SourceUsesSerialPort=0
        log_message "Verifying specification of a serial port in the boot loader configuration file..."

        # Locate the menuentry line associated to the target kernel version string and check if it has the substring "console=tty" in it. If so, it uses a serial port.
        TargetUsesSerialPort=`/bin/grep linux16 /boot/grub2/grub.cfg | /bin/grep $KernelVersion | /bin/grep "console=tty" | /usr/bin/wc -l | /bin/awk '{print $1}'`
        if [ $TargetUsesSerialPort -eq 0 ]
        then
            # If it does not, check if the source server DOES use a serial port
            SourceUsesSerialPort=`/bin/grep linux16 /SFTKdtc_migrated_bootvol/grub2/grub.cfg | /bin/grep $SourceKernelVersion | /bin/grep "console=tty" | /usr/bin/wc -l | /bin/awk '{print $1}'`
    
            # If so, log a message to warn the user that at switch over a serial port will be expected,
            # and if it is a VM, its output can be redirected to a datastore file. If there are problems,
            # this datastore output file will be accessible regardless of boot success or failure.
            if [ $SourceUsesSerialPort -ne 0 ]
            then
                log_message "  The source server from which the boot drive was migrated used a serial port at boot time. \
                               At switch over to the new boot drive, a serial port may be expected."
                log_message "  The serial port specification is defined in the bootloader configuration file /etc/default/grub, from which is generated /boot/grub2/grub.cfg"
            else
                log_message "  No serial port specified in the menu entry for source server kernel $SourceKernelVersion"
            fi
        fi
    fi #... of ok_to_check_serial_port_and_initrd

    #            PREPARE FOR TARGET NETWORK CONFIGURATION RESTORE IF APPLICABLE
    # The files to backup are: /etc/hosts, /etc/sysconfig/network, /etc/sysconfig/network-scripts/*, udev rule files
    if [ $RestoreTargetNetworkConfig -ne 0 ]
    then
        log_message "Copying network config files /etc/hosts, /etc/sysconfig/network and /etc/sysconfig/network-scripts/<all files> \
                     to $BootPartition, in directory backup_original_target_net_config_files..."
        log_message "These network configuration files will be restored to the migrated drive at reboot with the new drive."
        /bin/mkdir /SFTKdtc_migrated_bootvol/backup_original_target_net_config_files 1> /dev/null 2>&1
        /bin/mkdir /SFTKdtc_migrated_bootvol/backup_original_target_net_config_files/network-scripts 1> /dev/null 2>&1
        /bin/cp -p /etc/hosts /SFTKdtc_migrated_bootvol/backup_original_target_net_config_files/.
        /bin/cp -p /etc/sysconfig/network /SFTKdtc_migrated_bootvol/backup_original_target_net_config_files/.
        /bin/cp -p --dereference --preserve=links /etc/sysconfig/network-scripts/* /SFTKdtc_migrated_bootvol/backup_original_target_net_config_files/network-scripts/.
        if [ -e /etc/yp.conf ]
        then
            log_message "Copying NIS server config file /etc/yp.conf also for restore at switch over reboot time"
            /bin/cp -p /etc/yp.conf /SFTKdtc_migrated_bootvol/backup_original_target_net_config_files/.
        fi

        # The following is done because the hostname will be set early in the switch over boot sequence, potentially before our boot scripts are run.
        hostname_output=`/bin/hostname | awk '{print $1}'`
        /bin/echo $hostname_output > /SFTKdtc_migrated_bootvol/backup_original_target_net_config_files/hostname_restore.txt

        # Now also the udev rules
        log_message "Copying /etc/udev/rules.d/<files> (except net rules, modified by udev) to the migrated boot partition $BootPartition, in directory backup_original_target_net_config_files..."
        /bin/cp -pr /etc/udev/rules.d /SFTKdtc_migrated_bootvol/backup_original_target_net_config_files/.
        # Keep all the rules in the backup directory except the network rule files, which will get regenerated at switchover by udev.
        /bin/rm -f /SFTKdtc_migrated_bootvol/backup_original_target_net_config_files/rules.d/*-persistent-net.rules
        /bin/sync

        # Create a marker file to indicate that the network configuration files must be restored after switchover to the new boot drive:
        /bin/touch /SFTKdtc_migrated_bootvol/SFTKdtc_restore_net_config_files
        /bin/sync
    fi
    #<<< TODO: do we want an option to set to DHCP mode?

    if [ $ok_to_check_serial_port_and_initrd -eq 1 ]
    then
        #            VERIFY IF THE MIGRATED INITRD FILE CAN POTENTIALLY CAUSE PROBLEMS
        # Determine the names of the initrd files of both the source and target servers
        log_message "Verifying migrated initrd file..."
        SourceInitrd=`/bin/grep initrd /SFTKdtc_migrated_bootvol/grub2/grub.cfg | /bin/grep -v "# " | /bin/grep -v kdump | /bin/grep $SourceKernelVersion | /bin/awk '{print $2}'`
        TargetInitrd=`/bin/grep initrd /boot/grub2/grub.cfg | /bin/grep -v "# " | /bin/grep -v kdump  | /bin/grep $KernelVersion | /bin/awk '{print $2}'`
        log_message "Comparing initrd files /SFTKdtc_migrated_bootvol$SourceInitrd and /boot$TargetInitrd..."
        /usr/bin/cmp /SFTKdtc_migrated_bootvol$SourceInitrd /boot$TargetInitrd 1> /dev/null 2>&1
        InitrdDiffer=$?
        if [ $InitrdDiffer -eq 1 ]
        then
            # The initrd files differ; check if kernel modules are missing in the initrd that will become the active one.
            # Extract the list of modules from the files.

            # Target's original initrd:
            /bin/mkdir /tmp/contents_of_target_original_initrd 1> /dev/null 2>&1
            cd /tmp/contents_of_target_original_initrd
            $LsinitrdCommand /boot/$TargetInitrd > ./contents_of_initrd.txt
            /bin/grep "\.ko" ./contents_of_initrd.txt > ./kernel_objects.txt

            # The above provides a list of the kernel modules in an ls -l format.
            # We want to extract only the kernel module names, sorted:
            /usr/bin/touch ./unsorted_module_names.txt
            for file_entry in `/bin/cat ./kernel_objects.txt`
            do
                /usr/bin/echo $file_entry | /bin/grep "\.ko" 1> /dev/null 2>&1
                result=$?
                 if [ $result -eq 0 ]
                then
                    module_name=`/usr/bin/basename $file_entry`
                    /usr/bin/echo $module_name >> ./unsorted_module_names.txt
                fi
            done
            /bin/sort ./unsorted_module_names.txt > ./sorted_kernel_module_names.txt
            /usr/bin/rm -f ./unsorted_module_names.txt

            # Source's initrd:
            /bin/mkdir /tmp/contents_of_source_original_initrd 1> /dev/null 2>&1
            cd /tmp/contents_of_source_original_initrd
            $LsinitrdCommand /SFTKdtc_migrated_bootvol/$SourceInitrd > ./contents_of_initrd.txt
            /bin/grep "\.ko" ./contents_of_initrd.txt > ./kernel_objects.txt

            # Extract only the kernel module names, sorted:
            /usr/bin/touch ./unsorted_module_names.txt
            for file_entry in `/bin/cat ./kernel_objects.txt`
            do
                /usr/bin/echo $file_entry | /bin/grep "\.ko" 1> /dev/null 2>&1
                result=$?
                 if [ $result -eq 0 ]
                then
                    module_name=`/usr/bin/basename $file_entry`
                    /usr/bin/echo $module_name >> ./unsorted_module_names.txt
                fi
            done
            /bin/sort ./unsorted_module_names.txt > ./sorted_kernel_module_names.txt
            /usr/bin/rm -f ./unsorted_module_names.txt

            # Determine if some of the target's original initrd modules are missing from the source migrated initrd:
            found_missing_module=0
            module_in_dmesg_output=1
            cd /tmp
            logged_message=0
            for ModuleName in `/usr/bin/diff ./contents_of_target_original_initrd/sorted_kernel_module_names.txt ./contents_of_source_original_initrd/sorted_kernel_module_names.txt | /bin/grep "< " | /bin/awk '{print $2}'`
            do
               if [ $logged_message -eq 0 ]
               then
                   logged_message=1
                   log_message "Some module(s) of the original Target server initrd is (are) not in the migrated Source initrd which will become the active one:"
                   /bin/sync
                   /bin/sleep 2 # To avoid line mix up in the DMC Events tab
               fi
               found_missing_module=1
               strippedModuleName=`/bin/echo $ModuleName | /bin/sed s,.ko,,g`
               /bin/dmesg | /bin/grep -i $strippedModuleName 1> /dev/null 2>&1
               module_in_dmesg_output=$?
               if [ $module_in_dmesg_output -eq 0 ]
               then
                   log_message "$strippedModuleName (missing from new initrd and appears in previous boot messages of the target Server, potential problems)"
               else
                   log_message "$strippedModuleName (missing from new initrd but not in previous boot messages of the target Server, might not cause problems)"
               fi
            done
            if [ $found_missing_module -eq 1 ]
            then
                /bin/sleep 1 # For message time stamp to avoid mix up on DMC
                log_message "Missing modules in the migrated initrd file might cause problems upon booting from the migrated drive."
                /bin/sleep 1
                backup_initrd_name_suffix="_migrated_from_source_server"
                log_message "Renaming the migrated initrd /SFTKdtc_migrated_bootvol$SourceInitrd to /SFTKdtc_migrated_bootvol$SourceInitrd$backup_initrd_name_suffix"
                /bin/mv /SFTKdtc_migrated_bootvol$SourceInitrd /SFTKdtc_migrated_bootvol$SourceInitrd$backup_initrd_name_suffix
                /bin/sleep 1
                log_message "Copying the original initrd /boot$TargetInitrd of this target machine to the migrated boot volume $BootPartition as $SourceInitrd"
                /bin/cp -p /boot$TargetInitrd /SFTKdtc_migrated_bootvol$SourceInitrd
            else
                log_message "No missing module found in migrated initrd file."
            fi
        else
            log_message "No difference found between the initrd files of the source and target machines."
        fi
        /bin/sync
        /bin/sleep 2
        if [ $SourceUsesSerialPort -ne 0 -o $found_missing_module -eq 1 ]
        then
            log_message "Please look at messages logged regarding serial port usage and/or initrd. \
                         After that you will be ready to switch over to the migrated boot drive."
        fi
    fi #... of ok_to_check_serial_port_and_initrd

    /bin/umount /SFTKdtc_migrated_bootvol

}



########################################################################################
################################ MAIN SCRIPT BODY ######################################
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

#----- Check the type of configuration; this script can run only on RMD side of 1-to-1 configuration
CheckConfigurationType $GroupNumber

#----- Format the 3-digit group number for PMD_ and RMD_ pre/post failover script strings. -----
FormatGroupNumberSubstring $GroupNumber

# Go to the main logic for RedHat, RHEL7, or SuSE, depending on platform identifier
if [ $IsRedHat -eq 1 ]
then
    if [ $IsRHEL7 -eq 1 ]
    then
        RHEL7_Main_Logic
    else
        RedHat_Main_Logic
    fi
else
    SuSE_Main_Logic
fi

log_message "End of automated Linux boot drive migration for failover sequence."

exit 0


