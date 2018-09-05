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
#  /%OPTDIR%/%PKGNM%/bin/dtcmodfs
########################################################################

readonly CMD=${0##*/}

typeset groups
typeset groupfiles

typeset -ri YES=1 
typeset -ri NO=0 
typeset -i doAllGroups=NO
typeset -i checkOnly=NO
typeset -i requiresRawDevice=NO
typeset -i deleteBackupFiles=NO
typeset -i restoreFstab=NO

typeset -ri WANTNONE=-1
typeset -ri WANTDEVICE=0
typeset -ri WANTSOURCE=1
typeset -i conversionDirection=WANTNONE

readonly tmpAwkCmds=/tmp/${CMD}_awk.$$

################################################################################
######                      Return codes                                  ######
################################################################################
readonly EC_TRAP=4
readonly EC_FAIL=2
readonly EC_WARN=1
readonly EC_OK=0
################################################################################
#####   Usage function demonstrates the usage of dtcmodfs command         ######
################################################################################
function Usage
{
   echo " Usage: ${CMD} [-c] {-g <group#>]...|-a|-r<group#>..|-ra}"
   echo "        ${CMD} -d\n"
   echo " The ${CMD} program modifies ${FSTAB} file to replace"
   echo " data devices with dtc devices and back. This program maintains"
   echo " up to 10 backup copies of ${FSTAB} file which can be manually"
   echo " restored in case of problems.\n"
   echo " The backup files are named ${FSTAB}.orig.<number>"
   echo " where <number> is replaced with a decimal number."
   echo " The largest number is last saved copy of filesystems file.\n"
   echo "    -a              Change entries for all defined dtc devices"
   echo "    -g<group#>      Change entries for dtc devices with the"
   echo "                    specified group."
   echo "    -r<group# | a>  Change mount points from dtc devices to "
   echo "                    to the data devices"
   echo "    -c              Check if the file system mount table needs "
   echo "                    to be updated."
   echo "    -d              Delete all filesystems file backup copies"
}
################################################################################
######                        End of Usage function                       ######
################################################################################

################################################################################
######                      Display function                              ######
################################################################################
function display
{
    typeset message
    case $1 in
	NotRootUser)
	     message[0]="FATAL: You must be super user to run this program."
	    ;;
	OsNotRecognized)
	    message[0]="FATAL: ${CMD} does not recognize this operating system."
	    ;;
	RestoreOk)
	    message[0]="Restoring ${FSTAB} from ${backupFSTAB} completed successfully."
	    ;;
	RestoreFailed)
	    message[0]="FATAL: Restoring ${FSTAB} from ${backupFSTAB} failed."
	    message[1]="FATAL: You will need to recover ${FSTAB} manually from the backup"
	    message[2]="FATAL: copies found by running the command \"ls  $prefixFSTAB*\"."
	    ;;
	CopyFail)
	    message[0]="ERROR: Copy failed.  Need to restore ${FSTAB}"
	    ;;
	BackupFail)
	    message[0]="ERROR: Backup copy failed.  No changes made to ${FSTAB}"
	    ;;
	BackingUp)
	    message[0]="Backing up ${FSTAB} to ${backupFSTAB}"
	    ;;
	WritingChanges)
	    message[0]="Writing changes..."
	    ;;
	UpdatingFstab)
	    message[0]="Checking ${FSTAB}..."
	    ;;
	ChangesSaved)
	    message[0]="Changes have been saved to ${FSTAB}.  You must re-mount the"
	    message[1]="file systems or reboot the system for your changes to take effect."
	    ;;
	NoChange)
	    message[0]="${FSTAB} is up to date"
	    ;;
	FstabNeedsUpdating)
	    message[0]="${FSTAB} needs to be updated"
	    ;;
	FstabRmFailed)
	    message[0]="Removal of backup filesystem files failed"
	    ;;
	FstabRmSuccess)
	    message[0]="Removal of backup filesystem files succeeded"
	    ;;
	FstabRmNoFiles)
	    message[0]="No backup filesystem files to remove"
            ;; 
	InvalidGroup)
	    message[0]="The group parameter \"$2\" must be numeric and between 0 and 999."
	    ;;
	NonExistentGroup)
	    message[0]="Group \"$2\" does not exist"
	    ;;
	NoCfgFile)
	    message[0]="cfg files not found"
	    ;;
	MixedConversion)
	    message[0]="ERROR: Cannot mix -s and -m parameters"
	    ;;
	MixedGroups)
	    message[0]="ERROR: Cannot mix -a and -g parameters together"
	    ;;
	RepeatedParameter)
	    message[0]="ERROR: Parameter \"$2\" has been repeated."
	    ;;
	BadParameter)
	    message[0]="ERROR: Parameter \"$2\" is not valid."
	    ;;
	MissingAction)
	    message[0]="ERROR: Parameters -s -m or -d must be specified."
	    ;;
	MissingGroups)
	    message[0]="ERROR: Parameters -g or -a must be specified."
	    ;;
	AwkFail)
	    message[0]="ERROR: $FSTAB modification failed.  No changes made to $FSTAB"
	    ;;
	WarnNoChanges)
	    message[0]="WARNING: No changes made to $FSTAB"
	    ;;
	NeedMigrationLogDevice)
	    message[0]="ERROR: Mount point \"$2\" must have its log device \"$4\" converted to a migration volume."
	    ;;
	NeedMigrationDataDevice)
	    message[0]="ERROR: Mount point \"$2\" must have its data device \"$3\" converted to a migration volume."
	    ;;
	NeedRegularLogDevice)
	    message[0]="ERROR: Mount point \"$2\" must have its log device \"$4\" converted to a non-migration volume."
	    ;;
	NeedRegularDataDevice)
	    message[0]="ERROR: Mount point \"$2\" must have its data device \"$3\" converted to a non-migration volume."
	    ;;
	*)
	    message[0]="ERROR: Message $1 is not defined."
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

################################################################################
######                  process the group parameter                       ######
################################################################################
function process_group_parameter
{
    typeset optArg=$1
    typeset -Z3 lgnum

    (( doAllGroups == YES ))  && {
	display MixedGroups
	exit ${EC_FAIL}
    }
    if [[ "${optArg}" = +([0-9]) && "${#optArg}" -le 3 ]]
    then
	lgnum="${optArg}"
	[[ ! -f ${CFGPATH}/p${lgnum}.cfg ]] && {
	    display NonExistentGroup $lgnum
	    exit ${EC_FAIL}
	}
	groups="${groups} ${lgnum}"
	groupfiles="${groupfiles} p${lgnum}.cfg"
    else
	display InvalidGroup "${OPTARG}"
	exit ${EC_FAIL}
    fi
}

################################################################################
######                    process the all parameter                       ######
################################################################################
function process_all_parameter 
{
    typeset name=$1
    [[ ${#groups} -gt 0 ]] && {
	display MixedGroups
	exit ${EC_FAIL}
    }
    (( doAllGroups == YES )) && {
	display RepeatedParameter -${name}
	exit ${EC_FAIL}
    }

    (( doAllGroups=YES ))
}

################################################################################
######                    character to block device conversion            ######
################################################################################
function raw_to_block
{
    typeset source=$1
    typeset devname
    typeset space=" "
    typeset tab=$(print "\t\c")

    if [[ -z "${source}" || "${source}" = +([${space}${tab}]) ]]
    then
	display MissingLV
	return ${EC_FAIL}
    fi
    if [[ $source = */rdsk/* ]]
    then
	print ${source%/rdsk/*}/dsk/${source#*/rdsk/}
    elif [[ $source = */dsk/* ]]
    then
	print $source
    else
	devname=${source##*/}
	[[ -b ${source} ]] || devname="${devname#r*}"
	print ${source%/*}/${devname}
    fi
}

################################################################################
######                  generate awk replacment commands                  ######
################################################################################
function generate_replacement
{
    typeset -ri direction=$1
    typeset match
    typeset replace

    if [[ ${direction} -eq WANTDEVICE ]]
    then
	match="$3"
	replace="$2"
    else
	match="$2"
	replace="$3"
    fi

   escaped_match=`echo ${match} | sed -e 's:/:\\\\/:g' -e 's:\.:\\\.:g'`
   cat <<-EOF >>$tmpAwkCmds
	/${escaped_match}[ \\t\\"\\*#:]/ || /${escaped_match}\$/ {
	    sub( "${match}", "${replace}", \$0 )
	}
	EOF
}

################################################################################
######                  generate awk replacment commands                  ######
################################################################################
function create_awk_script
{
    typeset -u keyword
    typeset dtcRawDevice=""
    typeset dataRawDevice=""
    typeset dtcBlockDevice=""
    typeset dataBlockDevice=""
    typeset device
    typeset rest

    cat <<-EOF >$tmpAwkCmds
	/^[ \\t]*[\\*#] %COMPANYNAME2% %PRODUCTNAME%/ {
	    next
	}
	/^[ \\t]*[\\*#]/ {
	    print \$0
	    next
	}
	END {
	    ("date +%d-%B-%Y-%X") | getline changeDate
	    print "${fstabCOMMENT} %COMPANYNAME2% %PRODUCTNAME% " changeDate
	}
	EOF

    (cd $CFGPATH; 
    for cfgfile in ${groupfiles}
    do
	cat ${cfgfile} | while read keyword device rest
	do
	    [[ -z "${keyword}" ]] && continue
	    [[ "${keyword}" = \#* ]] && continue
	    case $keyword in
              # Checking for Dynamic Activation
	      DYNAMIC-ACTIVATION\:)
		if [[ ${device} = "on" || ${device} = "ON" ]]
         	then
                  # Refuse if conversionDirection is from data devices to dtc devices	(WANTDEVICE == 0)
                  if [[ ${conversionDirection} -eq WANTDEVICE ]]
		    then
		      echo "Dynamic Activation enabled in ${cfgfile}; ${cfgfile} dtc devices not inserted in mount table"
                      # Go to next cfgfile (group)
	              break
	          fi
		fi
		;;
		PROFILE\:)
		    dtcRawDevice=""
		    dataRawDevice=""
		    dtcBlockDevice=""
		    dataBlockDevice=""
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
	        # For AIX we need to use the short form device name
		if [[ $ostype = "AIX" ]]
         	then
	            dtcRawDevice=`echo $dtcRawDevice | sed -e 's/\/dtc//' -e 's/\/rdsk\///'`
                fi

		if [[ $ostype = "Linux" ]]
		then
			dtcBlockDevice="$dtcRawDevice"
			dataBlockDevice="$dataRawDevice"
		else
			dtcBlockDevice=$(raw_to_block ${dtcRawDevice})
			dataBlockDevice=$(raw_to_block ${dataRawDevice})
		fi

		generate_replacement ${conversionDirection} ${dtcBlockDevice} ${dataBlockDevice}
		(( requiresRawDevice == YES )) && generate_replacement ${conversionDirection} ${dtcRawDevice} ${dataRawDevice}

		dtcRawDevice=""
		dataRawDevice=""
		dtcBlockDevice=""
		dataBlockDevice=""
	    fi
	done
    done )

    cat <<-EOF >>$tmpAwkCmds
	{
	    print \$0
	}
	EOF
}

################################################################################
######                         Cleanup function                           ######
################################################################################
#  cleanup function helps us to restore the previous filesystems file in case  #
#  the executions of script is interrupted                                     #
function cleanup
{
    trap '' HUP INT QUIT TERM
    [ -f "$tmpFSTAB" ] && rm -f $tmpFSTAB 2>/dev/null
    [ -f ${tmpAwkCmds} ] && rm -f ${tmpAwkCmds} 2>/dev/null
    if (( restoreFstab == YES ))
    then
	if [[ -s "${backupFSTAB}" ]] && cp -p ${backupFSTAB} ${FSTAB} 2>/dev/null && cmp -s "${backupFSTAB}" "${FSTAB}"
	then
	    display RestoreOk
	else
	    display RestoreFailed
	fi
    exit ${EC_TRAP}
    fi
}

################################################################################
######                     log and device entries matching                ######
################################################################################

function parse_filesystems_attribute
{
    old_ifs="${IFS}";
    IFS='='
    set -- $1 
    IFS=${old_ifs}
    keyword=$1
    (( ${#} > 1 )) && shift
    print "$keyword" "$*"
}

# A mount point cannot mix migration and non-migration volumes

function bad_migration_usage
{
    [[ "${1}" = /dev/lg+([0-9])dtc+([0-9]) && "${2}" != /dev/lg+([0-9])dtc+([0-9]) && "${2}" != [Ii][Nn][Ll][Ii][Nn][Ee] ]] && return 1
    [[ "${1}" != /dev/lg+([0-9])dtc+([0-9]) && "${2}" = /dev/lg+([0-9])dtc+([0-9]) ]] && return 2
    return 0
}

function validate_filesystems_file
{
    typeset line
    typeset mountpoint
    typeset device
    typeset log
    typeset retcode

    retcode=0

    mountpoint=
    cat $1 | tr -d ' ' | tr -d '\t' | while read line
    do
	[[ -z "${line}" ]] && continue;	# ignore blank lines
	[[ "${line}" = [*] ]] && continue;	# ignore comments

	# get the mount point

	[[ "${line}" = *[:] ]] && {
	    mountpoint=${line%%:}
	    device=
	    log=
	    continue
	}

	parse_filesystems_attribute "${line}" | read keyword value

	[[ "${keyword}" = dev ]] && {
	    device="${value}"
	    continue
	}

	[[ "${keyword}" = log ]] && {
	    log="${value}"
	    continue
	}

	[[ -n "${device}" && -n "${log}" ]] && {

	    bad_migration_usage "${device}" "${log}"
	    case $? in

		1)  if [[ ${conversionDirection} -eq WANTDEVICE ]]
		    then
			display NeedMigrationLogDevice ${mountpoint} ${device} ${log}
		    else
			display NeedRegularDataDevice ${mountpoint} ${device} ${log}
		    fi
		    retcode=1
		    ;;
		2)  if [[ ${conversionDirection} -eq WANTDEVICE ]]
		    then
			display NeedMigrationDataDevice ${mountpoint} ${device} ${log}
		    else
			display NeedRegularLogDevice ${mountpoint} ${device} ${log}
		    fi
		    retcode=1
		    ;;
	    esac
	    log=
	    device=
	    mountpoint=
	}
    done
    return ${retcode}
}


################################################################################
######                           Main Program                             ######
################################################################################

########################## Turn on shell tracing ###############################

[[ -n "${TDMF_DEBUG}" ]] && {
    TDMF_DEBUG="DEBUG"
    set -x
    for func in $(typeset -f)
    do
	typeset -ft ${func}
    done
}

##########################Checking the type of OS###############################
ostype=`uname`
case $ostype in
    AIX*)
        readonly FSTAB="/etc/filesystems"
        readonly CFGPATH="/etc/dtc/lib"
	alias awk='/usr/bin/awk'
	readonly fstabCOMMENT='\*'
        readonly tmpFSTAB="/tmp/FSTAB.tmp.$$"
        ostype="AIX"
        ;;
    SunOS*)
        readonly FSTAB="/etc/vfstab"
        readonly CFGPATH="/etc/opt/SFTKdtc"
	alias awk='/usr/bin/nawk'
	readonly fstabCOMMENT='#'
        readonly tmpFSTAB="/tmp/FSTAB.tmp.$$"
	(( requiresRawDevice=YES ))
        ostype="SunOS"
        ;;
    HP-UX*)
        readonly FSTAB="/etc/fstab"
        readonly CFGPATH="/etc/opt/SFTKdtc"
	alias awk='/usr/bin/awk'
	readonly fstabCOMMENT='#'
        readonly tmpFSTAB="/tmp/FSTAB.tmp.$$"
        ostype="HP-UX"
        ;;
   Linux*)
        readonly FSTAB="/etc/fstab"
        readonly CFGPATH="/etc/opt/SFTKdtc"
	alias awk='/usr/bin/awk'
	readonly fstabCOMMENT='#'
        readonly tmpFSTAB="/tmp/FSTAB.tmp.$$"
        ostype="Linux"
        ;;
      *)
        display OsNotRecognized
        exit ${EC_FAIL}
        ;;
esac

[[ $# -lt 1 ]] && {
    Usage
    exit ${EC_WARN}
}

#########################Checking if the user is root###########################
userid=$(id | cut -d '(' -f 1 | cut -d '=' -f 2)
if [ $? -ne 0 ] || [ ${userid} -ne 0 ]
then
    display "NotRootUser"
    exit ${EC_FAIL}
fi

######Check to see if another instance of dtcmodfs is already present###########
ins=$(ps -ef | grep -v grep | grep -v ksh | egrep ${CMD} | wc -l)

if [ ${ins} -gt 1 ]
   then
   echo "Another instance of the ${CMD} application is already running....."
   exit ${EC_WARN}
fi

##########################Scanning the user options#############################
while getopts ":g:adsmcr:" name
do
case $name in
    g)
	process_group_parameter "${OPTARG}"
	;;
    a)
	process_all_parameter ${name}
	;;
    s)
	(( conversionDirection != WANTNONE )) && {
	    display MixedConversion
	    exit ${EC_FAIL}
	}
	(( conversionDirection=WANTSOURCE ))
	;;
    m)
	(( conversionDirection != WANTNONE )) && {
	    display MixedConversion
	    exit ${EC_FAIL}
	}
	(( conversionDirection=WANTDEVICE ))
	;;
    r)
	(( conversionDirection == WANTDEVICE )) && {
	    display MixedConversion
	    exit ${EC_FAIL}
	}
	(( conversionDirection=WANTSOURCE ))
	if [[ "${OPTARG}" = "a" ]]
	then
	    process_all_parameter "ra"
	else
	    process_group_parameter "${OPTARG}"
	fi
	;;
    c)
	(( checkOnly == YES )) && {
	    display RepeatedParameter -${name}
	    exit ${EC_FAIL}
	}
	(( checkOnly=YES ))
	;;
    d)
	(( deleteBackupFiles == YES )) && {
	    display RepeatedParameter -${name}
	    exit ${EC_FAIL}
	}
	(( deleteBackupFiles=YES ))
	;;
    *)
	display BadParameter -${name}
	Usage
	exit ${EC_FAIL}
	;;
esac
done

(( deleteBackupFiles == NO && doAllGroups == NO  && ${#groups} == 0 )) && {
    if (( conversionDirection == WANTNONE && checkOnly==NO ))
    then
	display MissingAction
    else
	display MissingGroups
    fi
    exit ${EC_FAIL}
}
(( conversionDirection == WANTNONE )) && {
    (( conversionDirection = WANTDEVICE ))
}

##################Delete all saved copies of mount table file###################
prefixFSTAB="${FSTAB}.orig."

if (( deleteBackupFiles == YES )) 
then
    ls $prefixFSTAB+([0-9]) >/dev/null 2>&1
    if [ $? -eq 0 ]
    then
        rm -f $prefixFSTAB+([0-9]) 2>/dev/null 
        if [ $? -ne 0 ]
        then
            # this should never happen
            display FstabRmFailed
            exit ${EC_FAIL}
        fi 
        display FstabRmSuccess
        exit ${EC_OK}
    else 
        display FstabRmNoFiles
        exit ${EC_FAIL}
    fi
fi

#######################Checking for the fstab index#############################

MAX_BACKUPS=10

set -A fstablist `ls -1d $prefixFSTAB+([0-9]) 2>/dev/null | sort -t. -k 3nr`
if [ ${#fstablist[*]} -gt 0 ]
then
    backupindex=${fstablist[0]#"${prefixFSTAB}"}
    (( backupindex=backupindex+1 ))
else
     # no existing fstab backups
     (( backupindex=0 ))
fi

############################Save the fstab file#################################

backupFSTAB=${prefixFSTAB}${backupindex}

display BackingUp
cp ${FSTAB} ${backupFSTAB} > /dev/null 2>&1 && cmp -s ${FSTAB} ${backupFSTAB}
if (( $? != 0 ))
then
    display BackupFail
    exit ${EC_FAIL}
fi

#########################Cleanup old backup files ##############################

if [[ ${#fstablist[*]} -ge $MAX_BACKUPS ]]
then
    let skip=0
    for oldfstab in ${fstablist[@]}
    do
        if [[ -f $oldfstab ]]
        then
            let skip=skip+1
            if [[ $skip -ge $MAX_BACKUPS ]]
            then
                rm -f $oldfstab 2>/dev/null
            fi
        fi
    done
fi
############################Trap the Signals####################################

trap "cleanup" HUP INT QUIT TERM

display UpdatingFstab

if (( doAllGroups == YES ))
then
    groupfiles=$(cd ${CFGPATH}; ls -1d p[0-9][0-9][0-9].cfg 2>/dev/null)
    [[ $? -ne 0 ]] && {
	display NoCfgFile
	exit ${EC_OK}
    }
    for cfgfile in ${groupfiles}
    do
	x1="${cfgfile#p*}"
	x1="${x1%*.cfg}"
	groups="${groups} ${x1}"
    done
fi

create_awk_script

display WritingChanges

awk -f ${tmpAwkCmds} ${FSTAB} >> ${tmpFSTAB}

[[ $? != 0 ]] && {
    display AwkFail
    exit ${EC_FAIL}
}

# Make sure log and devices are consistent

[[ ${ostype} = "AIX" ]] && {
    validate_filesystems_file  ${tmpFSTAB} || {
	display WarnNoChanges
	exit ${EC_FAIL}
    }
}

# deal with the Softek comment differences by stripping out line, saving to temp files
cat $FSTAB | grep -v ".*%COMPANYNAME2% %PRODUCTNAME%" > ${tmpFSTAB}.orig.nocomment
cat $tmpFSTAB |  grep -v ".*%COMPANYNAME2% %PRODUCTNAME%" > ${tmpFSTAB}.nocomment

cmp -s ${tmpFSTAB}.orig.nocomment ${tmpFSTAB}.nocomment && {
    display WarnNoChanges
    rm -f ${tmpFSTAB}.orig.nocomment ${tmpFSTAB}.nocomment 
    rm -f $backupFSTAB
    (( checkOnly == YES )) && exit ${EC_OK}
    exit ${EC_WARN}
}

rm -f ${tmpFSTAB}.orig.nocomment ${tmpFSTAB}.nocomment

(( checkOnly == YES )) && {
    display FstabNeedsUpdating
    exit ${EC_K}
}

(( restoreFstab=YES ))
(cp $tmpFSTAB $FSTAB && cmp -s $tmpFSTAB $FSTAB) || {
    display CopyFail;
    exit ${EC_FAIL}
}
display ChangesSaved
rm -f ${tmpFSTAB} 2>/dev/null
rm -f ${tmpAwkCmds} 2>/dev/null
(( restoreFstab=NO ))

exit 0
