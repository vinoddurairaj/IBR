#!/bin/bash
######################################################################
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
#  $Id: omp_cleanup.ksh,v 1.4 2009/02/26 07:53:28 naat0 Exp $
#
#
# Name: omp_cleanup
#
#   Returns: 
#           0 - clean up successful
#    non-zero - Not all information for clean-up could not be processed
#
#  This script will Clean-up OMP by stopping all migrations, Archiving
#  .cfg/done/log, SCSI output, csv, udev rules Files. 
#  It will also delete all FC SCSI disks devices unless given -d option
#  It will also archive all OMP staging files unless given -s option
#  -c will force cleanup even when TDMF-IP *.cfg and *.cur files exists
#
######################################################################
#
PROGNAME=${0##*/}
typeset -i debugOMP=0 rtncode=0 exitcode=0 count=0
typeset -i sleepINTVL=5 sleepMAXCNT=60
typeset delCFG="" delSTAG=1  delSCSI=1 arg
typeset omp_home="/etc/opt/SFTKomp"
typeset omp_tunables="omp_tunables"
typeset WhatTDMF="RFX"
typeset OMPDIR_ARC FCdev FCctlr
typeset SFTKetc="/etc/opt/SFTKdtc"
typeset SFTKbin="/opt/SFTKdtc/bin"
typeset SFTKtmp="/var/opt/SFTKdtc"
typeset OMPtmp="/var/opt/SFTKomp"
typeset UDEVdir="/etc/udev/rules.d"
typeset -i udevTIMEOUT=30

userid=$(/usr/bin/id -u)
if [ "$?" -ne 0 ] || [ "${userid}" -ne 0 ]
then
    echo "${PROGNAME}: root authority required .. login or su to root"
    exitcode=100
    exit ${exitcode}
fi     

#First source flag file to pick up user changes to variables
if [[ -r  ${omp_home}/${omp_tunables} ]]
   then
	if (set -e; . ${omp_home}/${omp_tunables}) >/dev/null 2>&1
        then
           . ${omp_home}/${omp_tunables}
        else
           echo ${PROGNAME}: Errors detected while processing ${omp_home}/${omp_tunables} file.
        fi
        [[ -n "${DebugCleanup:-}" ]] && debugOMP=${DebugCleanup}
        [[ -n "${HotPlugRetryCount:-}" ]] && sleepMAXCNT=${HotPlugRetryCount}
        [[ -n "${HotPlugWaitSleep:-}" ]] && sleepINTVL=${HotPlugWaitSleep}
fi  

#####FUNCTIONS #################################################
#### END FUNCTIONS #############################################

####### MAIN ###################################################
while getopts 'sdhc?' arg
do
	case $arg in
	s)	delSTAG=""
		;;
	d)	delSCSI=""
		;;
	c)	delCFG=1
		;;
	?|h)	echo -e "Usage: ${PROGNAME} [-s] [-d] [-c]\n" >&2
		echo -e "-s: Leave OMP Staged Jobs Files" >&2
		echo -e "-d: Leave SCSI devices and UDEV files" >&2
		echo -e "-c: Force cleanup when TDMF migration *.cfg files exist" >&2
		echo -e "-h: Display syntax usage for ${PROGNAME}" >&2
		exitcode=101
		exit ${exitcode}
		;;
	*) echo -e "${PROGNAME}: Error processing command line ... arg=$arg" >&2
		;;
	esac
done
if  [[ "$debugOMP" -ne 0 ]]
	then
		echo -e "${PROGNAME}: delSTAG=${delSTAG} delSCSI=${delSCSI} arg=$arg"
fi
#Check for Active *.cfg files before processing
if [[ "${WhatTDMF}" = "RFX" ]]
	then
		/bin/ls ${SFTKetc}/*.cfg >/dev/null 2>&1
		if [[ "$?" -eq 0 ]]
			then
		  	   if [[ -z "${delCFG}" ]]
			    then	
			    	echo "${PROGNAME}: Exited because active *.cfg files exists from previous migration"
			    	echo "   Run ${PROGNAME} with -c added to force cleanup"
				exitcode=28
				exit ${exitcode}
			   fi
		fi
fi
#Check for Directories and files and exit if can't be create
		if [[ ! -d ${OMPtmp} ]]
			then
				/bin/mkdir -p ${OMPtmp}
				if [[ "$?" -ne 0 ]]
					then
						echo "${PROGNAME}: FAILED to create ${OMPtmp}" >&2
						echo "${PROGNAME}: FAILURE Clean-up did not complete!" >&2
						exitcode=6
						exit ${exitcode}
				fi
		fi
		OMPDIR_ARC=$(/bin/mktemp -dq -p ${OMPtmp} OMParc_XXXXXXXXXX) 
		if [[ "$?" -ne 0 ]]
			then
   		 		echo "${PROGNAME}: FAILED to make mktemp directory $OMPDIR_ARC" >&2
				echo "${PROGNAME}: FAILURE Clean-up did not complete!" >&2
				exitcode=7
				exit ${exitcode}
		fi
		/bin/mkdir -p ${OMPDIR_ARC}/var ${OMPDIR_ARC}/var/SFTKdtc
		if [[ "$?" -ne 0 ]]
			then
				echo "${PROGNAME}: FAILED to create ${OMPDIR_ARC}/var and subdirs" >&2 
				echo "${PROGNAME}: FAILURE Clean-up did not complete!" >&2
				exitcode=8
				exit ${exitcode}
		fi
		/bin/mkdir -p ${OMPDIR_ARC}/etc
		if [[ "$?" -ne 0 ]]
			then
				echo "${PROGNAME}: FAILED to create ${OMPDIR_ARC}/etc" >&2
				echo "${PROGNAME}: FAILURE Clean-up did not complete!" >&2
				exitcode=9
				exit ${exitcode}
		fi
		/bin/mkdir -p ${OMPDIR_ARC}/udev
		if [[ "$?" -ne 0 ]]
			then
				echo "${PROGNAME}: FAILED to create ${OMPDIR_ARC}/udev" >&2
				echo "${PROGNAME}: FAILURE Clean-up did not complete!" >&2
				exitcode=26
				exit ${exitcode}
		fi
		/bin/mkdir -p ${OMPDIR_ARC}/etc/SFTKomp ${OMPDIR_ARC}/etc/SFTKomp/cfg \
		${OMPDIR_ARC}/etc/SFTKomp/done ${OMPDIR_ARC}/etc/SFTKomp/jobs/san \
		${OMPDIR_ARC}/etc/SFTKomp/jobs/staging ${OMPDIR_ARC}/etc/SFTKomp/log \
		${OMPDIR_ARC}/etc/SFTKomp/tmp ${OMPDIR_ARC}/etc/SFTKdtc
		if [[ "$?" -ne 0 ]]
			then
				echo "${PROGNAME}: FAILED to create ${OMPDIR_ARC}/etc Directories" >&2
				echo "${PROGNAME}: FAILURE Clean-up did not complete!" >&2
				exitcode=16
				exit ${exitcode}
		fi
# Disable and Stop the OMP Scheduler
${omp_home}/bin/omp_admin disable >/dev/null 2>&1
rtncode="$?"
if [[ "$rtncode" -ne 0 ]] && [[ "$rtncode" -ne 1 ]]
	then 
		echo "${PROGNAME}: omp_admin disable FAILED errcode=$?" >&2
		echo "${PROGNAME}: FAILURE Clean-up did not complete!" >&2
		exitcode=14
		exit ${exitcode}
fi
${omp_home}/bin/omp_admin stop >/dev/null 2>&1
rtncode="$?"
if [[ "$rtncode" -ne 0 ]] && [[ "$rtncode" -ne 1 ]]
	then 
		echo "${PROGNAME}: omp_admin stop FAILED" >&2
		echo "${PROGNAME}: FAILURE Clean-up did not complete!" >&2
		exitcode=15
		exit ${exitcode}
fi

# Stop all TDMF-IP Processing and Archive files and re-launch needed TDMF-IP processes
if [[ "${WhatTDMF}" = "RFX" ]]
	then
		#kill all PMDs 
		${SFTKbin}/dtckillpmd -a
		if [[ "$?" -ne 0 ]]
			then 
				echo "${PROGNAME}: ERROR dtckillpmd -a Failed"
				echo "${PROGNAME}: FAILURE Clean-up did not complete!"
				exitcode=1
				exit ${exitcode}
		fi
		#kill all RMDs
		${SFTKbin}/dtckillrmd -a
		if [[ "$?" -ne 0 ]]
			then 
				echo "${PROGNAME}: ERROR dtckillpmd -a Failed"
				echo "${PROGNAME}: FAILURE Clean-up did not complete!"
				exitcode=2
				exit ${exitcode}
		fi
		#stop all TDMF-IP Groups
		${SFTKbin}/dtcstop -a >/dev/null 2>&1
		if [[ "$?" -ne 0 ]]
			then 
				echo "${PROGNAME}: ERROR dtcstop -a Failed"
				echo "${PROGNAME}: FAILURE Clean-up did not complete!"
				exitcode=3
				exit ${exitcode}
		fi
		#Killing agent
		${SFTKbin}/killagent > /dev/null
		if [[ "$?" -ne 0 ]]
			then 
				echo "${PROGNAME}: ERROR killagent Failed"
				exitcode=4
		fi
		#Kill the master deamon
		${SFTKbin}/killdtcmaster > /dev/null
		if [[ "$?" -ne 0 ]]
			then 
				echo "${PROGNAME}: ERROR killdtcmaster Failed"
				echo "${PROGNAME}: FAILURE Clean-up did not complete!"
				exitcode=5
				exit ${exitcode}
		fi
		#Now Archive /etc TDMF-IP Data
		/bin/ls ${SFTKetc}/*.cfg >/dev/null 2>&1
		if [[ "$?" -eq 0 ]] && [[ "${delCFG}" -eq 1 ]]
			then
				/bin/mv -f ${SFTKetc}/*.cfg ${OMPDIR_ARC}/etc/SFTKdtc
				if [[ "$?" -ne 0 ]]
					then
						echo "${PROGNAME}: FAILED to move *.cfg files from ${SFTKetc} into ${OMPDIR_ARC}/etc/SFTKdtc"
						exitcode=12
				fi
		fi
		/bin/ls ${SFTKetc}/*.cur >/dev/null 2>&1
		if [[ "$?" -eq 0 ]] && [[ "${delCFG}" -eq 1 ]]
			then
				/bin/mv -f ${SFTKetc}/*.cur ${OMPDIR_ARC}/etc/SFTKdtc
				if [[ "$?" -ne 0 ]]
					then
						echo "${PROGNAME}: FAILED to move *.cur files from ${SFTKetc} into ${OMPDIR_ARC}/etc/SFTKdtc"
						exitcode=29
				fi
		fi
		#Move /var TDMF-IP files to a OMP archive directory
		/bin/ls ${SFTKtmp}/* >/dev/null 2>&1 && {
		/bin/mv -f ${SFTKtmp}/* ${OMPDIR_ARC}/var/SFTKdtc
		if [[ "$?" -ne 0 ]]
			then
				echo "${PROGNAME}: FAILED to move all files from ${SFTKtmp} into ${OMPDIR_ARC}/var/SFTKdtc "
				exitcode=10
		fi
		}
		/bin/ls ${OMPDIR_ARC}/var/SFTKdtc/Agn_tmp >/dev/null 2>&1 && {
		/bin/mv -f ${OMPDIR_ARC}/var/SFTKdtc/Agn_tmp ${OMPDIR_ARC}/var/SFTKdtc/dtcsock.msg* ${SFTKtmp} 
		if [[ "$?" -ne 0 ]]
			then
				echo "${PROGNAME}: FAILED to move Agn_tmp and dtcsock.msg* back into ${SFTKtmp}"
				exitcode=11
		fi
		}
		#Now Start back up TDMF-IP processes killed previously
		#Launch the master deamon
		${SFTKbin}/launchdtcmaster > /dev/null
		if [[ "$?" -ne 0 ]]
			then 
				echo "${PROGNAME}: ERROR launchdtcmaster Failed"
				exitcode=13
		fi
		#Launch agent
		${SFTKbin}/launchagent > /dev/null

fi
#Archive OMP Files
if [[ "$delSTAG" ]]
	then
		/bin/ls $omp_home/jobs/staging/* >/dev/null 2>&1 && {
		/bin/mv -f $omp_home/jobs/staging/* ${OMPDIR_ARC}/etc/SFTKomp/jobs/staging
		if [[ "$?" -ne 0 ]]
			then
				echo "${PROGNAME}: /bin/mv -f $omp_home/jobs/staging/* ${OMPDIR_ARC}/etc/SFTKomp/jobs/staging FAILED"
				exitcode=17
		fi
		}
		/bin/ls  ${OMPDIR_ARC}/etc/SFTKomp/jobs/staging/pstore_devices  >/dev/null 2>&1 && {
		/bin/mv -f ${OMPDIR_ARC}/etc/SFTKomp/jobs/staging/pstore_devices $omp_home/jobs/staging
		if [[ "$?" -ne 0 ]]
			then
				echo "${PROGNAME}: /bin/mv -f ${OMPDIR_ARC}/etc/SFTKomp/jobs/staging/pstore_devices $omp_home/jobs/staging"
				exitcode=18
		fi
		}

fi
/bin/ls $omp_home/jobs/san/* >/dev/null 2>&1 
if [[ "$?" -eq 0 ]]
	then
		if [[ -z "$delSTAG" ]]
			then
				/bin/ls $omp_home/jobs/san/*.csv >/dev/null 2>&1 && {
				/bin/cp -p $omp_home/jobs/san/*.csv ${OMPDIR_ARC}/etc/SFTKomp/jobs/san >/dev/null 2>&1
				if [[ "$?" -ne 0 ]]
				 then
					echo "${PROGNAME}: /bin/cp -p $omp_home/jobs/san/*.csv ${OMPDIR_ARC}/etc/SFTKomp/jobs/san FAILED"
					exitcode=19
				fi
				/bin/ls $omp_home/jobs/san/*.omp >/dev/null 2>&1 && {
				/bin/cp -p $omp_home/jobs/san/*.omp ${OMPDIR_ARC}/etc/SFTKomp/jobs/san >/dev/null 2>&1
				 }
				}
			else
				/bin/ls $omp_home/jobs/san/*.csv >/dev/null 2>&1 && {
				/bin/mv -f $omp_home/jobs/san/*.csv ${OMPDIR_ARC}/etc/SFTKomp/jobs/san >/dev/null 2>&1
				if [[ "$?" -ne 0 ]]
				 then
					echo "${PROGNAME}: /bin/mv -f $$omp_home/jobs/san/*.csv ${OMPDIR_ARC}/etc/SFTKomp/jobs/san FAILED"
					exitcode=30
				fi
				/bin/ls $omp_home/jobs/san/*.omp >/dev/null 2>&1 && {
				/bin/mv -f $omp_home/jobs/san/*.omp ${OMPDIR_ARC}/etc/SFTKomp/jobs/san >/dev/null 2>&1
				 }
				}
		fi
fi
 
/bin/ls $omp_home/cfg/* >/dev/null 2>&1 && {
/bin/mv -f $omp_home/cfg/* ${OMPDIR_ARC}/etc/SFTKomp/cfg
if [[ "$?" -ne 0 ]]
	then
		echo "${PROGNAME}: /bin/mv -f $omp_home/cfg/* ${OMPDIR_ARC}/etc/SFTKomp/cfg FAILED"
		exitcode=20
fi
/bin/mv -f ${OMPDIR_ARC}/etc/SFTKomp/cfg/scheduler.disabled $omp_home/cfg
if [[ "$?" -ne 0 ]]
	then
		echo "${PROGNAME}: /bin/mv -f ${OMPDIR_ARC}/etc/SFTKomp/cfg/scheduler.disabled $omp_home/cfg"
		exitcode=24
fi
}
/bin/ls $omp_home/done/* >/dev/null 2>&1 && {
/bin/mv -f $omp_home/done/* ${OMPDIR_ARC}/etc/SFTKomp/done
if [[ "$?" -ne 0 ]]
	then
		echo "${PROGNAME}: /bin/mv -f $omp_home/done/* ${OMPDIR_ARC}/etc/SFTKomp/done FAILED"
		exitcode=21
fi
}
/bin/ls $omp_home/log/* >/dev/null 2>&1 && {
/bin/mv -f $omp_home/log/* ${OMPDIR_ARC}/etc/SFTKomp/log
if [[ "$?" -ne 0 ]]
	then
		echo "${PROGNAME}: /bin/mv -f $omp_home/log/* ${OMPDIR_ARC}/etc/SFTKomp/log FAILED"
		exitcode=22
fi
}
/bin/ls -f $omp_home/tmp/* >/dev/null 2>&1 && {
/bin/mv -f $omp_home/tmp/* ${OMPDIR_ARC}/etc/SFTKomp/tmp
if [[ "$?" -ne 0 ]]
	then
		echo "${PROGNAME}: /bin/mv -f $omp_home/tmp/* ${OMPDIR_ARC}/etc/SFTKomp/tmp FAILED"
		exitcode=23
fi
}
# Delete FC SCSI devices if Flag is = 1
if [[ "${delSCSI}" ]]
	then
		for FCctlr in $(/bin/ls /sys/class/fc_transport)
			do	
				FCctlr="${FCctlr##*target}"
				for FCdev in $(/bin/ls -d /sys/class/scsi_device/${FCctlr}*)
					do
						if  [[ "$debugOMP" -ne 0 ]]
							then
								echo "${PROGNAME}:  echo 1 > ${FCdev}/device/delete"
						fi
						echo 1 > ${FCdev}/device/delete
					done	
			done
	/bin/ls -f ${UDEVdir}/*omp.rules >/dev/null 2>&1 && {
	/bin/mv -f ${UDEVdir}/*omp.rules ${OMPDIR_ARC}/udev
	if [[ "$?" -ne 0 ]]
		then
			echo "${PROGNAME}: /bin/mv -f ${UDEVdir}/*omp.rules ${OMPDIR_ARC}/udev FAILED"
			exitcode=27
	fi
	}
	
fi
#Wait for all devices to be Deleted
count=0
while (( count < sleepMAXCNT ))
	do
        if [[ "$debugOMP" -ne 0 ]]
            then
                echo "${PROGNAME}: Check if any /sbin/hotplug processes exist"
        fi 
		/bin/ps -ef | /bin/grep -e "/sbin/[h]otplug" -e "[u]dev_run_" > /dev/null
        if [[ "$?" -eq 0 ]]
			then
			   	count=count+1
	       		if [[ "$debugOMP" -ne 0 ]]
   	        		then
   	            		echo "${PROGNAME}: /sbin/hotplug or udev_run_* processes still running count=${count} maxcount=${sleepMAXCNT}"
   	            		echo "${PROGNAME}: sleeping for ${sleepINTVL} seconds"
   	    		fi 
				/bin/sleep ${sleepINTVL}
            else
				# all /sbin/hotplug or udev_run_* processes have finshed... break while loop
				break
		fi        

done

#RH5 udev now has command to check udev finished also
[[ -e /sbin/udevsettle ]] && {
	if [[ "$debugOMP" -ne 0 ]]
		then
		  echo "${PROGNAME}: Running /sbin/udevsettle to verify udev is finshed discovering devices"
	fi 
	/sbin/udevsettle --timeout=$udevTIMEOUT
}

# Enable the OMP Scheduler
${omp_home}/bin/omp_admin enable >/dev/null 2>&1
rtncode="$?"
if [[ "$rtncode" -ne 0 ]] && [[ "$rtncode" -ne 1 ]]
	then 
		echo "${PROGNAME}: omp_admin disable FAILED errcode=$?" >&2
		echo "${PROGNAME}: FAILURE Clean-up did not complete!" >&2
		exitcode=25
fi
if [[ "$exitcode" -ne 0 ]]
	then
		echo "${PROGNAME}: Proccessing has completed with ERRORS"
	else
		echo "${PROGNAME}: Processing completed successfully"
fi

exit ${exitcode}

