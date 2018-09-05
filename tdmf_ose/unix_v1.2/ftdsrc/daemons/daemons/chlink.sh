#!/bin/sh
########################################################################################
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
# Description:
# -----------
#		This sample script for Oracle to make changes to the symbolic links 
#		to the devices.
#
# Remarks:
# --------
#		This script needs to be modified to meet your oracle setting, and please
#		run this script on your own responsibility.
#
#########################################################################################
usage()
{
	echo "Usage: chlink -g %CAPGROUPNAME%_Group_Num  Symbolic_link_name"
	echo "                                                 "
	echo "       -g %CAPGROUPNAME%_Group_Num     Specify the %GROUPNAME% group number that the targeted Symbolic link name is defined for"
	echo "          Symbolic_link_name    Symbolic link name that is targeted to change in absolute path form"
	exit 1
}
#
#Remove "exit 1" or convert it in to comment form by replacing with "#exit 1", two lines below, when you modify this script
#
echo "** Warning ** This script needs to be modified to meet your oracle setting, and please run this script on your own responsibility."
exit 1


#
# Verify cfg file path and whoami
# 
if [ `/bin/uname` = "HP-UX" ]; then
    COMMAND_USR=`/usr/bin/whoami`
    CFG_FILE_DIR=/etc/opt/%PKGNM%
elif [ `/bin/uname` = "SunOS" ]; then
    if [ -x /usr/ucb/whoami ]; then COMMAND_USR=`/usr/ucb/whoami`; else COMMAND_USR=`/usr/bin/whoami`; fi
    CFG_FILE_DIR=/etc/opt/%PKGNM%
elif [ `/bin/uname` = "AIX" ]; then           
    COMMAND_USR=`/bin/whoami`
    CFG_FILE_DIR=/etc/%Q%/lib
elif [ `/bin/uname` = "Linux" ]; then
    COMMAND_USR=`/usr/bin/whoami`
    CFG_FILE_DIR=/etc/opt/%PKGNM%
fi

#
# user check
#
if [ "$COMMAND_USR" != "root" ]
then
    echo "You must be root to run this script"
    exit 1
fi

#
# Verify executables
# 
if [ `/bin/uname` = "Linux" ]; then
    ls="/bin/ls"
    awk="/bin/awk"
    cat="/bin/cat"
    expr="/usr/bin/expr"
    egrep="/bin/egrep"
else
    ls="/usr/bin/ls"
    awk="/usr/bin/awk"
    cat="/usr/bin/cat"
    expr="/usr/bin/expr"
    egrep="/usr/bin/egrep"
fi

#
# Get arguments
#
LGNUMBER=""
SYMBOLIC_LINK_NAME=""
GFLAG=0
while getopts g: arg 
do
	case ${arg} in
	g)	if [ ${GFLAG} -ne 0 ] 
		then
			echo "The -g option is multiple defined"
			exit 1
		fi	
		GFLAG=1;
		LGNUMBER=`echo ${OPTARG}|${awk} '{printf "%03d",$1}'`
		;;
	?)	usage 
		exit 1
		;;
	esac
done
shift `${expr} ${OPTIND} - 1`
if [ ${#} -lt 1 -o ${#} -gt 1 ]  
then
	usage
	exit 1
fi
if [ "${LGNUMBER}" = "" ]
then
	usage
	exit 1
fi
SYMBOLIC_LINK_NAME=${1}

#
# Check %GROUPNAME% group
#
if [ ! -f ${CFG_FILE_DIR}/p${LGNUMBER}.cfg ]
then
	echo "The group number specified with -g is incorrect"
	exit 1
fi

#
# Check raw device is specified as symbolic link
#
if [ `/bin/uname` = "SunOS" ]; then
RAW_DEVICE_PATH=`echo ${SYMBOLIC_LINK_NAME} | ${egrep} "^/dev/rdsk/"`
	if [ ${?} -eq 0 ] 
	then
		echo "${SYMBOLIC_LINK_NAME} is a raw device. Specify a symbolic link."
		exit 1
	fi
fi

#
# Check specified link is a symbolic link
#
if [ ! -h ${SYMBOLIC_LINK_NAME} ]
then
	echo " ${SYMBOLIC_LINK_NAME} is not a symbolic link, or does not specify in absolute path form"
	exit 1
fi

#
# Check specified device is a raw device
#
if [ ! -c ${SYMBOLIC_LINK_NAME} ]
then
	echo "Specified device to Oracle is not a raw device "
	exit 1
fi

#
# Get raw device and %Q% device
#
RAW_DEVICE=${SYMBOLIC_LINK_NAME}
%CAPQ%_DEVICE=`${cat} ${CFG_FILE_DIR}/p${LGNUMBER}.cfg|${awk} '{if($1=="DATA-DISK:" && $2=="'${RAW_DEVICE}'"&& %Q%!="")print %Q%;if($1=="%CAPQ%-DEVICE:")%Q%=$2;if($1=="PROFILE:")%Q%=""}'`

while [ -z "${%CAPQ%_DEVICE}" ] && [ -h "${RAW_DEVICE}" ]
do
	NEXT_DEVICE=`${ls} -l ${RAW_DEVICE} | ${awk} '{print $11}'`
	ABSOLUTE_PATH=`echo ${NEXT_DEVICE} | ${egrep} "^/"`
	if [ ${?} -ne 0 ]
	then
		echo "All links must be in absolute path form"
		exit 1
	fi
	%CAPQ%_DEVICE=`${cat} ${CFG_FILE_DIR}/p${LGNUMBER}.cfg|${awk} '{if($1=="DATA-DISK:" && $2=="'${NEXT_DEVICE}'"&& %Q%!="")print %Q%;if($1=="%CAPQ%-DEVICE:")%Q%=$2;if($1=="PROFILE:")%Q%=""}'`
	RAW_DEVICE=${NEXT_DEVICE}
done
if [ -z "${%CAPQ%_DEVICE}" ]
then
	echo "Raw device not found "
	exit 1
fi

#
# Message output
#
echo "Change Symbolic link ${SYMBOLIC_LINK_NAME}"
echo "Original Link destination is: ${RAW_DEVICE}"
echo "New Link destination is: ${%CAPQ%_DEVICE}"

#
# Ask  "yes" or "no"
#
echo "Do you want to change it now? (Y or N) > \c"
read ANSWER
while [ "${ANSWER}" != "Y" ] && [ "${ANSWER}" != "y" ] && [ "${ANSWER}" != "YES" ] && [ "${ANSWER}" != "yes" ]
do
	if [  "$ANSWER" = "n" -o "$ANSWER" = "no" -o "$ANSWER" = "N" -o "$ANSWER" = "NO"  ]
	then
		echo "Symbolic link does not change"
		exit 1
	else
		echo "Do you want to change it now? (Y or N) > \c"
		read ANSWER
	fi
done

rm ${SYMBOLIC_LINK_NAME}
if [ $? -ne 0 ]
then
	echo "[rm] command failed"
	exit 1
fi
ln -s ${%CAPQ%_DEVICE} ${SYMBOLIC_LINK_NAME}
if [ $? -ne 0 ]
then
	echo "[ln -s] command failed"
	exit 1
fi
echo "Completed"
exit 0
