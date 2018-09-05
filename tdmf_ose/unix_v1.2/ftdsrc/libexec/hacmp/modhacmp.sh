#!/bin/ksh
########################################################################
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
#  Program to incorporate Softek %PRODUCTNAME% interfaces into HACMP
#  script utilities named cl_activate_vgs and cl_deactivate_vgs 
#
#  Variable Conventions
#
#  All Uppercase - exported variable
#  first character lower case - local read/write variable
#  first charactr upper case - local read only variable
#
########################################################################

# Make sure the caller does not override standard OS commands

unalias -a

export PATH=/usr/bin:/etc:/usr/sbin:/usr/ucb:/sbin:/usr/es/sbin/cluster:/usr/es/sbin/cluster/utilities

#
# Get command name for error messages
#

typeset -r Cmd=${0##*/}
typeset -r CmdDir=${0%/*}

#
# Exit codes
#

typeset -ri ExitCodeTrap=8
typeset -ri ExitCodeFatal=4
typeset -ri ExitCodeError=2
typeset -ri ExitCodeWarning=1
typeset -ri ExitCodeSuccess=0

typeset -ri OldUmask=$(umask)
umask 0

typeset -r CfgDir="/%ETCOPTDIR%"
typeset -r DtcDir="/etc/dtc/hacmp"
typeset -r DtcResourceFile="${DtcDir}/cl_%Q%_rsgrps"
typeset -r DtcFlagFile="${DtcDir}/cl_%Q%_flags"  
typeset -r HacmpUtilsDir="/usr/es/sbin/cluster/events/utils"

typeset -i exitCode=$ExitCodeSuccess


##### For symbolic links source volume name changes defined ####
typeset symbolicLinkPrefix=""
typeset symbolicLinkSuffix="_OLD"
typeset symbolicLinkTemplate="%_OLD"	# avoiding the prompt for now
typeset usingSymbolicLinks=1
typeset forceCopyFlag=
typeset verboseFlag=
typeset updateCfgFlag=

typeset resourceGroup
typeset -Z3 logicalGroup
typeset logicalGroupUsed
typeset dataRenamedDevice
typeset tmpAwkCmds

typeset -i debugScript=0

typeset product="%Q%"
typeset dynamic_activation=""
typeset -i DA_on=0

function display
{
    typeset message
    typeset -i i
    case $1 in
	UsageTdmf)
	    typeset -i len
	    message="Usage: ${Cmd} "
	    len=${#message}
	    print ${message} "[-t <Target_Volume_Template>] [-s <Symbolic_Link_Template>]"
	    printf "%${len}s%s\n\n" "" "[-m <Migration_Volume_Template>] [-f] Resource_Group..."
	    printf "%${len}s%s\n" "" "This program modifies HACMP/PowerHA Script Utilities"
	    printf "%${len}s%s\n" "" "for shared volume groups to include %PRODUCTNAME% commands."
	    printf "%${len}s%s\n" "" "This script generates migration volume names and"
	    printf "%${len}s%s\n" "" "migration target volumes names dynamically using"
	    printf "%${len}s%s\n" "" "a naming convention based upon the logical volume names in"
	    printf "%${len}s%s\n" "" "shared Volume Groups for HACMP Resource Groups." 
	    printf "%${len}s%s\n" "" "Adding a new logical volume to a shared volume group will"
	    printf "%${len}s%s\n" "" "automatically generate a new migration volume and" 
	    printf "%${len}s%s\n" "" "migration target volume name. %PRODUCTNAME% HACMP automation" 
	    printf "%${len}s%s\n" "" "is triggered by taking resource groups offline" 
	    printf "%${len}s%s\n\n" "" "or online on a node with %PRODUCTNAME% HACMP automation enabled." 

	    printf "%${len}s%s\n" "" "%Q%modhacmp prompts the user for the naming templates for"
	    printf "%${len}s%s\n" "" "target volumes, migration volumes, symbolic links "
	    printf "%${len}s%s\n\n" "" "unless they are passed on the command line."

	    printf "%${len}s%s\n" "" "The HACMP script utilities to be %PRODUCTNAME% enabled are"
	    printf "%${len}s%s\n" "" "cl_activate_vgs and cl_deactivate_vgs located under"
	    printf "%${len}s%s\n" "" "events/utils in the Home directory for HACMP."
	    printf "%${len}s%s\n" "" "Executing %Q%modhacmp creates \"cl_activate_vgs.%Q%\" and"
	    printf "%${len}s%s\n" "" "\"cl_deactivate_vgs.%Q%\" in the HACMP events/utils"
	    printf "%${len}s%s\n" "" "directory."
	    printf "%${len}s%s\n" "" "The -f option overwrites existing %Q% files in the"
	    printf "%${len}s%s\n\n" "" "events/utils directory, without prompting the user."

	    printf "%${len}s%s\n" "" "Follow the steps below to enable %PRODUCTNAME% HACMP on a HACMP node."
	    printf "%${len}s%s\n" "" "1) Manually offline or move resource groups away"
	    printf "%${len}s%s\n" "" "   from the node to be %PRODUCTNAME% HACMP enabled."
	    printf "%${len}s%s\n" "" "2) Backup the original cl_activate_vgs "
	    printf "%${len}s%s\n" "" "   and cl_deactivate_vgs scripts."
	    printf "%${len}s%s\n" "" "3) %Q%modhacmp  (with or without command line arguments)"
	    printf "%${len}s%s\n" "" "4) cd /usr/es/sbin/cluster/events/utils"
	    printf "%${len}s%s\n" "" "5) mv cl_activate_vgs.%Q% cl_activate_vgs"
	    printf "%${len}s%s\n" "" "6) mv cl_deactivate_vgs.%Q% cl_deactivate_vgs"
	    printf "%${len}s%s\n" "" "7) Bring the resource groups back online to"
	    printf "%${len}s%s\n\n" "" "  the %PRODUCTNAME% HACMP enabled node."

	    printf "%${len}s%s\n" "" "The migration volume name is created by concatenating the"
	    printf "%${len}s%s\n" "" "volume group name, a separator and the logical volume name."
	    printf "%${len}s%s\n" "" "The migration volume prefix and suffix strings are optional"
	    printf "%${len}s%s\n" "" "and not set by default."
	    printf "%${len}s%s\n" "" "The prefix and suffix strings are changeable"
	    printf "%${len}s%s\n\n" "" "using %Q%modhacmp."

	    printf "%${len}s%s\n" "" "Example: The source logical volume \"/dev/lvol1\" in vg01"
	    printf "%${len}s%s\n" "" "causes the migration volume name \"/dev/mig/vg01-lvol1\""
	    printf "%${len}s%s\n\n" "" "to be generated using %Q%modhacmp default behavior."

	    printf "%${len}s%s\n" "" "The target volume name is formed by applying "
	    printf "%${len}s%s\n" "" "prefix and suffix strings to the source logical volume name."
	    printf "%${len}s%s\n" "" "A suffix is required because AIX requires"
	    printf "%${len}s%s\n" "" "unique logical volume names."
	    printf "%${len}s%s\n" "" "The default operation is to append the suffix \"X\" to"
	    printf "%${len}s%s\n\n" "" "the source logical volume name."

	    printf "%${len}s%s\n" "" "Example: The target volume using the above example"
	    printf "%${len}s%s\n" "" "would be \"/dev/lvol1X\" , using the default behavior."
	    printf "%${len}s%s\n" "" "The prefix and suffix strings are changeable"
	    printf "%${len}s%s\n" "" "using %Q%modhacmp."
	    printf "%${len}s%s\n" "" "Note: The user must create each target logical volume"
	    printf "%${len}s%s\n" "" "if data migration to the target is desired."
	    printf "%${len}s%s\n" "" "%Q%modhacmp enables %PRODUCTNAME% to automatically"
	    printf "%${len}s%s\n\n" "" "discover and use the target volume."
	    
	    printf "%${len}s%s\n" "" "If user desires %Q% to dynamically move the original"
	    printf "%${len}s%s\n" "" "source volume and replace it with a symbolic link,"
	    printf "%${len}s%s\n" "" "then the source logical volume is renamed by applying"
	    printf "%${len}s%s\n" "" "prefix and suffix strings to its original device names."
	    printf "%${len}s%s\n" "" "The default action is to rename by appending the suffix"
	    printf "%${len}s%s\n\n" "" "\"_OLD\" to the source logical volume device names in /dev."

	    printf "%${len}s%s\n" "" "Example: The new source logical volume name using the same"
	    printf "%${len}s%s\n" "" "example as above would be \"/dev/lvol1_OLD\" ,"
	    printf "%${len}s%s\n" "" "using the default behavior."
	    printf "%${len}s%s\n" "" "The prefix and suffix strings are changeable"
	    printf "%${len}s%s\n" "" "using %Q%modhacmp."
	    printf "%${len}s%s\n" "" "Note: The original source device names will become"
	    printf "%${len}s%s\n" "" "symbolic links to the migration device names."
	    printf "%${len}s%s\n" "" "i.e. /dev/lvol1 -> /dev/mig/vg01-lvol1" 
	    printf "%${len}s%s\n\n" "" "     /dev/rlvol1 -> /dev/mig/rvg01-lvol1" 

	    printf "%6s%-24s%s\n" "" "[-s <Symbolic_Link_Template>]"
	    printf "%16s%s\n" "" "The -s option represents the template parameter"
	    printf "%16s%s\n" "" "that determine the naming convention for symbolic links."
	    printf "%16s%s\n" "" "It is recommended"
	    printf "%16s%s\n" "" "that the prefix and suffix strings only"
	    printf "%16s%s\n" "" "contain alphanumeric characters to avoid"
	    printf "%16s%s\n" "" "unexpected shell interpretation errors.  The"
	    printf "%16s%s\n" "" "character \"%\" represents the source logical"
	    printf "%16s%s\n" "" "volume name or for migration volume the"
	    printf "%16s%s\n" "" "source volume group-logical volume name."
	    printf "%16s%s\n" "" "The string before the \"%\""
	    printf "%16s%s\n" "" "character becomes the prefix string.  The string"
	    printf "%16s%s\n" "" "after the \"%\" character is the suffix string."
	    printf "%16s%s\n" "" "If no \"%\" character exists, then the string is"
	    printf "%16s%s\n" "" "assumed to be a suffix string.  The default"
	    printf "%16s%s\n" "" "behavior is to append a suffix string to the"
	    printf "%16s%s\n\n" "" "source logical volume name."

	    printf "%16s%s\n" "" "For Example: A source logical volume named"
	    printf "%16s%s\n" "" "\"/dev/lvol1\" and target volume template specification"
	    printf "%16s%s\n" "" "of \"-t ABC%NEW\" will generate a target volume name of"
	    printf "%16s%s\n\n" "" "\"/dev/ABClvol1NEW\" to be used for migration."

	    printf "%16s%s\n" "" "Additional Controls:"
	    printf "%16s%s\n" "" "When Softek %PRODUCTNAME% HACMP automation executes it reads the file"
	    printf "%16s%s\n\n" "" "/etc/opt/%PKGPREFIX%/hacmp/cl_%Q%_flags for additional controls."

	    printf "%16s%s\n\n" "" "The addition control flags are as follows:"

	    printf "%16s%s\n" "" "UsingSFTK%Q% - the value should only be set to a 0 or 1"
	    printf "%16s%s\n" "" "  UsingSFTK%Q%=0 , Disables %PRODUCTNAME% HACMP automation in the Cluster"
	    printf "%16s%s\n\n" "" "  UsingSFTK%Q%=1 , Enables %PRODUCTNAME% HACMP automation in the Cluster"

	    printf "%16s%s\n" "" "debugSFTK%Q% - the value should only be set to a 0 or 1"
	    printf "%16s%s\n" "" "  debugSFTK%Q%=0 , Disables extra logging"
	    printf "%36s%s\n" "" "for %PRODUCTNAME% HACMP automation in the Cluster"
	    printf "%16s%s\n" "" "  debugSFTK%Q%=1 , Enables extra logging"
	    printf "%36s%s\n\n" "" "for %PRODUCTNAME% HACMP automation in the Cluster"
		   
		unset message
		;;
		
	UsageDtc)
	    typeset -i len
	    message="Usage: ${Cmd} "
	    len=${#message}
	    print ${message} "resourcegroup1:<group#> resourcegroup2:<group#> ..."
	    printf "\n%${len}s%s\n" "" "resourcegroup refers to HACMP resource groups that need to be replicated."
        printf "%${len}s%s\n" "" "group# refers to Mobility group that has all the volumes associated with the resource groups."	    
		unset message
		;;
		
	    RootPrivledge)
		message[0]="ERROR: You must have root privledge to run $Cmd command."
		;;
	    FileCmdFailure)
		message[0]="ERROR: The command "file" failed to execute correctly."
		;;
	    NotAFile)
		message[0]="ERROR: The file \"$2\" does exist or is not a file"
		;;
	    MissingLV)
		message[0]="ERROR: A logical volume parameter is missing."
		;;
	    MissingFile)
		message[0]="ERROR: File \"$2\" cannot be found or is missing."
		;;
	    MissingCfgFile)
		message[0]="ERROR: Configuration file \"$2\" cannot be found or is missing."
		;;
	    NotARawDevice)
		message[0]="ERROR: The pathname \"$2\" does not follow the raw device naming conventions."
		;;
	    AlreadyConverted)
		message[0]="WARNING: The pathname \"$2\" looks to be already converted."
		;;
	    NotAShellScript)
		message[0]="ERROR: The file \"$2\" is not a shell script."
		;;
	    ConversionError)
		message[0]="ERROR: The file \"$2\" failed to convert."
		;;
	    CopyValidation)
		message[0]="ERROR: The file \"$2\" failed to be copied."
		;;
	    CopiedFile)
		message[0]="Copied $2 to $3."
		;;
	    CreatedFile)
		message[0]="Created $2 from $3."
		;;
	    WcFailure)
		message[0]="ERROR: The wc program failed to execute on file \"$2\"."
		;;
	    UnexpectedOption)
		message[0]="ERROR: Option -$2 is not a valid option."
		;;
	    ForceCopySet)
		message[0]="ERROR: Option -f has already been set."
		;;
	    VerboseSet)
		message[0]="ERROR: Option -v has already been set."
		;;
	    UpdateCfgSet)
		message[0]="ERROR: Option -c has already been set."
		;;
	    YesNoInvalid)
		message[0]="ERROR: You must answer y[es] or n[o]."
		;;
	    NotAValidResourceGroup)
		message[0]="WARNING: The resource group \"$2\" does not exist on this system."
		;;
	    InvalidResourceGroup)
		message[0]="WARNING: The resource group \"$2\" is missing or not valid."
		;;
	    InvalidResourceGroupParameter)
		message[0]="ERROR: The resource group parameter \"$2\" is not valid."
		;;
	    MissingLogicalGroup)
		message[0]="ERROR: The resource group parameter \"$2\" needs to have a logical"
		message[1]="ERROR: group number specified."
		;;
	    LogicalGroupNotNumeric)
		message[0]="ERROR: The logical group number \"$2\" is not numeric for resource"
		message[1]="ERROR: group parameter \"$3\"."
		;;
	    LogicalGroupTooLarge)
		message[0]="ERROR: The logical group number \"$2\" must be a number between 0 and 999"
		message[1]="ERROR: for resource group parameter \"$3\"."
		;;
	    LogicalGroupAlreadyUsed)
		message[0]="ERROR: The resource group parameter \"$2\" has its logical group"
		message[1]="ERROR: number used by resource group \"$3\"."
		;;
	    UtilFileConverted)
		message[0]="WARNING: The HACMP/PowerHA file \"$2\""
		message[1]="WARNING: has already been converted to include %PRODUCTNAME% modifications."
		;;
	    *) message[0]="Unknown message $1.  Args $2 $3 $4 $5"
		;;
    esac

    if (( ${#message[*]} > 0 ))
    then
	(( i = 0 ))
	while (( i < ${#message[*]} ))
	do
	    print ${message[i]}
	    (( i += 1 ))
	done
    fi
}

function cleanup
{
    trap '' HUP QUIT INT TERM EXIT

    [[ -n "${tmpAwkCmds}" && -f "${tmpAwkCmds}" ]] && rm -f "${tmpAwkCmds}" 
    umask $OldUmask
    exit $1
}

################################################################################
# 
# Checks line counts to see if two files are the same number of lines
#
# Arguments: original_file changed_file expected_difference_line_count
################################################################################

function validate_file_by_line_count
{
    typeset source=$1
    typeset target=$2
    typeset difference=$3
    typeset source_cnt
    typeset target_cnt

    [[ ! -f "$1" ]] && {
	display MissingFile "$1" 
	return $ExitCodeError
    }
    [[ ! -f "$2" ]] && {
	display MissingFile "$2" 
	return $ExitCodeError
    }
    source_cnt=$(wc -l "$1" | awk -F " " '{print $1}')
    if [[ $? != 0 ]]
    then
	display WcFailure "$1" 
	return $ExitCodeError
    fi
    target_cnt=$(wc -l "$2" | awk -F " " '{print $1}')
    if [[ $? != 0 ]]
    then
	display WcFailure "$2" 
	return $ExitCodeError
    fi

    if (( ${source_cnt} + $3 != ${target_cnt} ))
    then
	display ConversionError "$2" 
	return $ExitCodeError
    fi
    return $ExitCodeSuccess
}

################################################################################
# 
# Creates the modified HACMP files that will call the dtc HACMP scripts
#
# Arguments: cl_activate_vgs | cl_deactivage_vgs 
################################################################################

function convertHacmpUtils
{
    typeset filename=${HacmpUtilsDir}/$1
    typeset line_diffs=3
    #
    # Make sure input file is a shell script 
    #

    typeset -l type1 type2	# force lowercase

    if [[ -z "${filename}" || ! -f "${filename}" ]]
    then
	display NotAFile "${filename}"
	exit $((exitCode=$ExitCodeError))
    fi
    file ${filename} | read name type1 type2 rest
    if [[ $? -ne 0 ]]
    then
	display FileCmdFailure "${filename}"
	exit $((exitCode=$ExitCodeFatal))
    fi

    if [[ "${type1}" != "shell" && "${type2}" != "script"  ]]
    then
	display NotAShellScript "${filename}"
	exit $((exitCode=$ExitCodeFatal))
    fi

    utilsfile="${filename}.%Q%"
    rm -f ${utilsfile}

    # get the file attributes set and copy was successful

    if grep "${DtcDir}" ${filename} >/dev/null 2>&1
    then
	display UtilFileConverted "${filename}"
	return $ExitCodeSuccess
    fi

    cp -p "${filename}" "${utilsfile}" && cmp -s "${filename}" "${utilsfile}"
    if [[ "$?" -ne 0 ]]
    then
	display CopyValidation "${filename}";	# copy failed
	exit $((exitCode=$ExitCodeError))
    fi

    # Only two changes on older versions of HACMP

    if egrep -s "haes_r520|haes_r510" "${filename}"
    then
	line_diffs=2
    fi

    case $1 in
	cl_activate_vgs)
	    sed '
		/^[ 	]*vgs_list[ 	][ 	]*\$/ {
		p
		s:vgs_list:wait; '${DtcDir}/cl_activate_%Q%:'
		s:&::
		s/$/	# %PRODUCTNAME% modified/
		}
		/^[ 	]*activate_oem_vgs[ 	][ 	]*\$/ {
		p
		s:activate_oem_vgs:wait; '${DtcDir}/cl_activate_%Q%:'
		s:&::
		s/$/	# %PRODUCTNAME% modified/
		}' $filename > $utilsfile
	    validate_file_by_line_count ${filename} ${utilsfile} ${line_diffs}
	;;

	cl_deactivate_vgs)
	    sed '
		/^[ 	]*vgs_varyoff[ 	][ 	]*\$/ {
		h
		s:vgs_varyoff:wait; '${DtcDir}/cl_deactivate_%Q%:'
		s:&::
		s/$/	# %PRODUCTNAME% modified/
		p
		g
		}
		/^[ 	]*deactivate_oem_vgs[ 	][ 	]*\$/ {
		h
		s:deactivate_oem_vgs:wait; '${DtcDir}/cl_deactivate_%Q%:'
		s:&::
		s/$/	# %PRODUCTNAME% modified/
		p
		g
	    }' $filename > $utilsfile
	    validate_file_by_line_count ${filename} ${utilsfile} ${line_diffs}
	;;
    esac

    if [[ "$?" -ne 0 ]]
    then
	display ConversionError "${filename}"
	exit $((exitCode=$ExitCodeFatal))
    fi
    display CreatedFile ${utilsfile} ${filename}  
    return $ExitCodeSuccess
}

################################################################################
# 
# validates the resource group parameter
# 
# Syntax [resource_group]:lgnum
#
# Arguments: resource_group_parameter
################################################################################

function parse_resource_group_parameter
{
    typeset original="$1"
    typeset old_ifs="${IFS}";

    IFS=':'
    set -- $1 
    IFS=${old_ifs}

    (( ${#} <= 0 )) && {
	display InvalidResourceGroupParameter "${original}" 
	return $ExitCodeError
    }

    (( ${#} <  2 )) && {
	display MissingLogicalGroup "${original}" 
	return $ExitCodeError
    }
    (( ${#} >  2 )) && {
	display InvalidResourceGroupParameter "${original}" 
	return $ExitCodeError
    }

    [[ $2 != +([0-9]) ]]  && {
	display LogicalGroupNotNumeric $2 "${original}" 
	return $ExitCodeError
    }
    (( $2 >= 1000 )) && {
	display LogicalGroupTooLarge $2 "${original}" 
	return $ExitCodeError
    }
    [[ -n "${logicalGroupUsed[$2]}" ]] && {
	display LogicalGroupAlreadyUsed ${original} "${logicalGroupUsed[$2]}" 
	return $ExitCodeError
    }

    # Checking the resoure group name outside of this routine 

    resourceGroup=$1
    logicalGroupUsed[$2]="$1"
    logicalGroup=$2
    return $ExitCodeSuccess
}

################################################################################
# 
# Copys files to the HACMP directory
# 
# Arguments: dtc_hacmp_file hacmp_vgs_file
################################################################################

function copy_dtc_vgs_file
{
    typeset file2="${HacmpUtilsDir}/$1.%Q%"
    typeset -l copyFlag=${forceCopyFlag}

    [[ ! -f ${file2} ]] && copyFlag="1"

    [[ ${copyFlag} = "1" ]] && {
	convertHacmpUtils $1
    }
}

################################################################################
# 
# Applies the symbolic link template to a raw device name
# 
# Arguments: raw_device_pathname
################################################################################

function raw_to_renamed_raw
{
    typeset source=$1
    typeset devname
    typeset space=" "
    typeset prefix=${symbolicLinkPrefix}
    typeset suffix=${symbolicLinkSuffix}
    typeset tab=$(print "\t\c")

    if [[ -z "${source}" || "${source}" = +([${space}${tab}]) ]]
    then
	display MissingLV
	return $ExitCodeError
    fi
    if [[ $source = */rdsk/* ]]
    then
	devname=${source##*/}
	if [[ ${devname} = ${devname%${suffix}} && ${devname} = ${devname#${prefix}} ]] 
	then
	    dataRenamedDevice="${source%/*}/${prefix}${devname}${suffix}"
	else
	    display AlreadyConverted ${source}
	    return $ExitCodeWarning
	fi
    elif [[ $source = */dsk/* ]]
    then
	display NotARawDevice ${source}
	return $ExitCodeError
    else
	devname=${source##*/}
	devname="${devname#r*}"
	if [[ ${devname} = ${devname%${suffix}} && ${devname} = ${devname#${prefix}} ]] 
	then
	    dataRenamedDevice="${source%/*}/r${prefix}${devname}${suffix}"
	else
	    display AlreadyConverted ${source}
	    return $ExitCodeWarning
	fi
    fi
    return $ExitCodeSuccess
}

################################################################################
# 
# Generates the awk statements to change the data devices in p*.cfg files
# 
# Arguments: old_device_name new_device_name
################################################################################

function generate_replacement
{
    typeset match
    typeset replace

    match="$1"
    replace="$2"

    escaped_match=$(echo ${match} | sed -e 's:/:\\/:g' )
    cat <<-EOF >>$tmpAwkCmds
	/DATA-DISK:/ && ( /${escaped_match}[ \\t\\"\\*#:]/ || /${escaped_match}\$/ ) {
	    sub( "${match}", "${replace}", \$0 )
	}
	EOF
}

################################################################################
# 
# Creates the awk script that updates p*.cfg files
# 
# Arguments: full_pathname_to_cfg_file
################################################################################

function create_awk_script
{
    typeset -u keyword
    typeset dtcRawDevice=""
    typeset dataRawDevice=""
    typeset device
    typeset rest
    typeset devname

    # Parsing more than we need in case we change our minds again

    exec 4<${1}
    while read -u4 keyword device rest
    do
	[[ -z "${keyword}" ]] && continue
	[[ "${keyword}" = \#* ]] && continue
	case $keyword in
	    PROFILE\:)
		dtcRawDevice=""
		dataRawDevice=""
		dataRenamedDevice=""
		continue
	    ;;
	    DTC-DEVICE\:)
		dtcRawDevice="${device}"
	    ;;
	    DATA-DISK\:)
		dataRawDevice="${device}"
	    ;;
	esac

	if [[ -n ${dtcRawDevice} && -n ${dataRawDevice} ]]
	then
	    raw_to_renamed_raw ${dataRawDevice} && {
		generate_replacement ${dataRawDevice} ${dataRenamedDevice}
	    }

	    dtcRawDevice=""
	    dataRawDevice=""
	    dtcRenamedDevice=""
	fi
    done
    exec 4<&-

    cat <<-EOF >>$tmpAwkCmds
	{
	    print \$0
	}
	EOF
}

################################################################################
# 
# Copies a file and validates that it compares
# 
# Arguments: copy_from_file copy_to_file
################################################################################

function copy_cfg_file
{
    typeset source=$1
    typeset target=$2

    cp ${source} ${target} && cmp ${source} ${target}
    if [[ $? -ne 0 ]]
    then
	display CopyValidation ${source} ${target} 
	return $ExitCodeError
    fi
    return $ExitCodeSuccess
}

################################################################################
# 
# Creates pxxx.cfg_orig and pxxx.cfg_cl
# 
# Arguments: None
# Uses global variables logicalGroup
################################################################################

function create_hacmp_cfg_files 
{
    typeset cfgFile

    cfgFile="${CfgDir}/p${logicalGroup}.cfg"

    # Get the dynamic activation state for the current group in order to copy config files or not.
    dynamic_activation=$(awk '/DYNAMIC-ACTIVATION/ {printf $2 }' $cfgFile)
    
    if [[ "$dynamic_activation" = "on" ]] || [[ "$dynamic_activation" = "ON" ]]; then
      DA_on=1    
      # Disable the use of Symbolic link    
      usingSymbolicLinks="0"  
      
      symbolicLinkSuffix=""
      symbolicLinkTemplate="%"

    fi

    [[ ! -s ${cfgFile} ]]  && {
      display MissingCfgFile ${cfgFile}
	  return $ExitCodeError
    }

    copy_cfg_file ${cfgFile} ${cfgFile}_orig || return $?

    [[ -n ${tmpAwkCmds} && -f ${tmpAwkCmds} ]] && rm -f ${tmpAwkCmds}

    tmpAwkCmds=/tmp/${Cmd}${logicalGroup}_awk.$$
    create_awk_script ${cfgFile} || return $?
    awk -f ${tmpAwkCmds} ${cfgFile} >${cfgFile}_cl || return $?

    validate_file_by_line_count ${cfgFile}_orig ${cfgFile}_cl 0 || return $?

    rm -f ${tmpAwkCmds}
    return $ExitCodeSuccess

}

#
# Enable debugging if required
#

if (( debugScript != 0 ))
then
    set -x
    for func in $(typeset -f)
    do
	typeset -ft ${func}
    done
fi

trap "cleanup $ExitCodeTrap" HUP QUIT INT TERM 

trap "cleanup \$exitCode" EXIT

if [[ $# -le 0 ]]
then
  if [[ "${product}" = "dtc" ]]; then
        display UsageDtc
  else  
        display UsageTdmf
  fi
    exit $((exitCode=$ExitCodeWarning))
fi

#
# Do not run script unless the user is root
#

effectiveUser=$(whoami 2>/dev/null)
if [[ $? -ne 0 || "$effectiveUser" != "root" ]]
then
    display RootPrivledge
    exit $((exitCode=$ExitCodeError))
fi

#
# Get command line args
#
while getopts ":fgs:v" option
do
   case $option in
      s)          
	symbolicLinkTemplate=$(print ${OPTARG})		# strip spaces
	if [[ "${symbolicLinkTemplate}" != *%* ]]
	then
	    symbolicLinkSuffix=$(print ${symbolicLinkTemplate})
	    symbolicLinkPrefix=
	else
	    symbolicLinkPrefix=${symbolicLinkTemplate%\%*}
	    symbolicLinkSuffix=${symbolicLinkTemplate##*%}
	fi
	usingSymbolicLinks="1"
	;;
      f)          
	if [[ -z ${forceCopyFlag} ]]
	then
	    forceCopyFlag="1"
	else
	    display ForceCopySet
	    exit $((exitCode=$ExitCodeError))
	fi
	;;
      v)          
	if [[ -z ${verboseFlag} ]]
	then
	    verboseFlag="1"
	else
	    display VerboseSet
	    exit $((exitCode=$ExitCodeError))
	fi
	;;
      g)          
	if [[ -z ${updateCfgFlag} ]]
	then
	    updateCfgFlag="1"
	else
	    display UpdateCfgSet
	    exit $((exitCode=$ExitCodeError))
	fi
	;;
      *)
	display UnexpectedOption ${OPTARG}
	exit $((exitCode=$ExitCodeError))
	;;
    esac
done

shift $(($OPTIND - 1))

if [[ -n "${updateCfgFlag}" ]]
then
    for resourceGroupParameter
    do
	parse_resource_group_parameter ${resourceGroupParameter} || {
	    exit $((exitCode=$ExitCodeError))
	}
	create_hacmp_cfg_files || exit $((exitCode=$?))
    done
    exit $((exitCode=$ExitCodeSuccess))
fi


while [[ -z "${usingSymbolicLinks}" ]]
do
    read usingSymbolicLinks?"Do you want to use symbolic links[no]? " || { print; exit $((exitCode=$ExitCodeError)); }
    if [[ yes  = "${usingSymbolicLinks}"* ]]
    then
	usingSymbolicLinks="1"
    elif [[ -z "${usingSymbolicLinks}" || no = "${usingSymbolicLinks}"* ]]
    then
	usingSymbolicLinks="0"
    else
	display YesNoInvalid
	usingSymbolicLinks=
    fi
done

if [[ "${usingSymbolicLinks}" = "1" ]]
then

    while [[ "${symbolicLinkTemplate}" = "%" ]]
    do
	read symbolicLinkTemplate?"Enter symbolic link volume template[_OLD]? " || { print; exit $((exitCode=$ExitCodeError)); }
	if [[ -z "${symbolicLinkTemplate}" ]]
	then
	    symbolicLinkSuffix="_OLD"
	    symbolicLinkPrefix=
	elif [[ "${symbolicLinkTemplate}" != *%* ]]
	then
	    symbolicLinkSuffix=$(print ${symbolicLinkTemplate})
	    symbolicLinkPrefix=
	else
	    symbolicLinkPrefix="${symbolicLinkTemplate%\%*}"
	    symbolicLinkSuffix="${symbolicLinkTemplate##*%}"
	fi
    done
fi

copy_dtc_vgs_file cl_activate_vgs
copy_dtc_vgs_file cl_deactivate_vgs

# Create the flags file

cat <<-EOF >${DtcFlagFile}
%Q%sep="-"
##### For symbolic links source volume name changes defined ####
srcprefix="${symbolicLinkPrefix}"
srcsuffix="${symbolicLinkSuffix}"
###########################################
usingSFTK%Q%=1
debugSFTK%Q%=0
usingSymbolicSFTK%Q%=${usingSymbolicLinks}
# By default, launching the PMDs is delayed for 150 seconds (2:30 minutes).
launchPMDsDelay=150
EOF

chmod 644 ${DtcFlagFile}

# Create the resource file

rm -f ${DtcResourceFile}

exec 3>${DtcResourceFile}
chmod 644 ${DtcResourceFile}
for resourceGroupParameter
do
    parse_resource_group_parameter ${resourceGroupParameter} || continue
    [[ -z "${resourceGroup}" ]] && {
	display InvalidResourceGroup "${resourceGroup}"
	continue
    }
    cllsgrp | fgrep -w -s "${resourceGroup}"
    if [[ $? -ne 0 ]]
    then
	display NotAValidResourceGroup ${resourceGroup}
    fi
    create_hacmp_cfg_files || continue
    print -u3 ${resourceGroup} ${logicalGroup} 
done

exit $((exitCode=$ExitCodeSuccess))
