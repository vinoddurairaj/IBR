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
#  $Id: omp_getscsidsk.ksh,v 1.5 2010/01/13 19:37:51 naat0 Exp $
#
# Name: omp_getscsidsk
#
#   Returns: 
#           0 - All scsi disks information obtained successfully
#    non-zero - Not all information about scsi disks could be resolved
#
#  This script will collect and display the needed information about scsi disks
#  to be used to create TDMF cfg files.
#
######################################################################
#
PROGNAME=${0##*/}
umask 0
typeset dfile dtype dvendor dprodid dprodrev dserial dscsiid ddskport dhba dpcislot dsbus ddir
typeset dserialx80 dSCSIidx83 dSCSIidx80 serialUDEV
typeset schedgrp dlun dportid dportname fld1 fld2 fld3 fld4 fld5 fld6 other devpath dsize devsizes
typeset sg_inq_p scanhost
typeset -i dsectors dsectorsz exitcode=0
typeset -i hdrcount=0 exitcode=0 count=0
typeset -i debugOMP=0
typeset -i sleepINTVL=5 sleepMAXCNT=60 sleepLIP=5
typeset rulesfn="/etc/udev/rules.d/10-omp.rules"
typeset dsysfile
typeset omp_home="/etc/opt/SFTKomp"
typeset omp_tunables="omp_tunables"  
typeset SGINFO tdhba REDHAT_DIST="/etc/redhat-release" REDHAT_VER=""
typeset -i udevTIMEOUT=30

userid=$(/usr/bin/id -u)
if [ "$?" -ne 0 ] || [ "${userid}" -ne 0 ]
then
    echo "${PROGNAME}: root authority required .. login or su to root"
    exitcode=99
    exit ${exitcode}
fi    

#Set Redhat version since udev in RH5 has different a syntax for rules file
/bin/grep -i "release 4" $REDHAT_DIST >/dev/null && REDHAT_VER="4"
[[ -z "$REDHAT_VER" ]] && {
	/bin/grep -i "release 5" $REDHAT_DIST >/dev/null && REDHAT_VER="5"
}

#First source flag file to pick up user changes to variables
if [[ -r  ${omp_home}/${omp_tunables} ]]
then
	# Pick up user variable changes by sourcing file
	if (set -e; . ${omp_home}/${omp_tunables}) >/dev/null 2>&1
	then
		. ${omp_home}/${omp_tunables}
	else
		echo ${PROGNAME}: Errors detected while processing ${omp_home}/${omp_tunables} file.
	fi
	# TODO Validate values
	[[ -n "${DebugGetScsiDsk:-}" ]] && debugOMP=${DebugGetScsiDsk}
	[[ -n "${HotPlugRetryCount:-}" ]] && sleepMAXCNT=${HotPlugRetryCount}
	[[ -n "${HotPlugWaitSleep:-}" ]] && sleepINTVL=${HotPlugWaitSleep}
	[[ -n "${LipWaitSleep:-}" ]] && sleepLIP=${LipWaitSleep}
fi    

#Verify sg3_utils is installed before running script 
	/bin/rpm -q sg3_utils >/dev/null 2>&1
	if [[ "$?" -ne 0 ]]
		then
			echo "${PROGNAME}: Required sg3_utils package not install on system"
			echo "                 PLEASE Install sg3_utils and re-run ${PROGNAME}"
			exitcode=90
			exit ${exitcode}
	fi
# Remove udev rules file if it exists
if [[ -f ${rulesfn} ]]
	then
		if [[ "$debugOMP" -ne 0 ]]
			then
				echo "${PROGNAME}: Issuing /bin/rm -f ${rulesfn}"
		fi
		/bin/rm -f ${rulesfn}
fi

# Issue FC LIP and SCSI rescan on all FC HBAs
for scanhost in $(/bin/ls /sys/class/fc_host)
	do
		if [[ "$debugOMP" -ne 0 ]]
			then
				echo "${PROGNAME}: Issuing FC LIP and SCSI rescan on ${scanhost}"
		fi
		echo "1" > /sys/class/fc_host/${scanhost}/issue_lip
		/bin/sleep ${sleepLIP}
		echo "- - -" > /sys/class/scsi_host/${scanhost}/scan
		# Loop for pre-defined sleep times and counts waiting on hotplug processing
		count=0
		while (( count < sleepMAXCNT ))
			do
		        if [[ "$debugOMP" -ne 0 ]]
		            then
		                echo "${PROGNAME}: Check if any /sbin/hotplug or udev_run_* processes exist"
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

	done
#RH5 udev now has command to check udev finished also
[[ -e /sbin/udevsettle ]] && {
	if [[ "$debugOMP" -ne 0 ]]
		then
		  echo "${PROGNAME}: Running /sbin/udevsettle to verify udev is finshed discovering devices"
	fi 
	/sbin/udevsettle --timeout=$udevTIMEOUT
}

if [[ "$debugOMP" -ne 0 ]]
	then
		#Add pounds signs in output only in debug mode
		echo "################################################"
fi

# Find all scsi devices attached to system
SGINFO=$(/bin/ls -d /sys/block/sd*)
if [[ "$?" -ne 0 ]]
	then
		echo "${PROGNAME}: /bin/ls -d /sys/block/sd* FAILED"
		exitcode=1
		exit ${exitcode}
fi
#Process each scsi disk device in SYSFS filesytem
for dsysfile in $(echo $SGINFO)
	do
		dsysfile=${dsysfile##*/}
		#Print Header before going into while loop since variables values are not saved when exit from piped loop
		if [[ "${hdrcount}" -eq 0 ]]
			then
				echo  'schedgrp,vendor,prodid,prodrev,serial#,serial#x80,sizeMB,scsiid,dskport,lun#,hba,pcislot,sbus,fcportid,fcportname,sectors,sectorsz,devfile'
				#Put first common two lines in udev rules
				echo "# file create and modified by omp_getscsidsk" > ${rulesfn}
				case $REDHAT_VER in
				 4) echo 'KERNEL="sd*", PROGRAM="/sbin/scsi_id -g -s /block/%k", RESULT="TDMFDUMMY", NAME="TDMFDUMMY_%k"' >> ${rulesfn} ;;
				 5) echo 'KERNEL=="sd*", PROGRAM=="/sbin/scsi_id -g -s /block/%k", RESULT=="TDMFDUMMY", NAME="TDMFDUMMY_%k"' >> ${rulesfn} ;;
				 *) 
				    echo "${PROGNAME}: udev header ERROR - Linux version not recognized REDHAT_VER=$REDHAT_VER"
				    exitcode=13
				    exit ${exitcode}
				    ;;
				esac
				hdrcount=hdrcount+1
		fi

	   # find name in /dev for this scsi device
	   dfile=$(/usr/bin/udevinfo -q name -p /block/${dsysfile})
		if [[ "$?" -ne 0 ]]
			then
			echo " "
			echo "${PROGNAME}: /usr/bin/udevinfo -q name -p /block/${dsysfile} FAILED *dfile=${dfile}*"
			echo "${PROGNAME}: udev Database does not have a /dev device mapped to physical device /sys/block/$dsysfile"
			echo "${PROGNAME}: or /sys/block/${dsysfile} is a stale device entry."
			echo "${PROGNAME}: Run omp_cleanup to cleanup /dev/sd* devices"
			echo "${PROGNAME}: Then re-run your last OMP command."
			echo " "
			dfile=""
			exitcode=11
			#Exit and process next scsi disk in list
			echo "################################################"
			break
		fi
		if [[ "$debugOMP" -ne 0 ]]
			then
			echo "${PROGNAME}: The systems device for /dev/${dfile} = /block/${dsysfile}"
		fi

	   #Leave in for now but using ls -l /sys/block/sd* instead of sginfo -l since it was broken past 1024 devices in RH5 sg3_utils
#	   [[ "sg" = "${dfile:0:2}" ]] && {
#	   	#Found first /dev/sg* device in sginfo list so break out of Processing each scsi device loop
#		if [[ "$debugOMP" -ne 0 ]] ; then
#			echo "${PROGNAME}: sg device found stop processing sginfo -l list"
#		fi
#		exitcode=12
#		break
#	     }
	   # Only process file if it starts with sd (it is a scsi disk)
	   [[ "sd" = "${dfile:0:2}" ]] && {
		# Store values for device from sg_inq output
		sg_inq_p="$(/usr/bin/sg_inq  /dev/${dfile})"
		if [[ "$?" -ne 0 ]]
			then
				echo "${PROGNAME}: /usr/bin/sg_inq  /dev/${dfile} FAILED "
				exitcode=2
#				exit ${exitcode}
		fi
		echo "${sg_inq_p}" | while read fld1 fld2 fld3 fld4 fld5 fld6 other
		do
			#First make sure device is a disk then store disk attributes only if it is a disk
			if [[ "${fld5}" = "type:" ]] && [[ "${fld6}" = "disk" ]]
				then
					if [[ "$debugOMP" -ne 0 ]]
						then
					echo "${PROGNAME}: For Device type fld1=$fld1 fld2=$fld2 fld3=$fld3 fld4=$fld4 fld5=$fld5 fld6=$fld6 other=$other"
					fi
					dtype=${fld6}
					#Now read Vendor ID
					read fld1 fld2 fld3 fld4 fld5 fld6 other
					if [[ "$debugOMP" -ne 0 ]]
						then
					echo "${PROGNAME}: For Vendor ID fld1=$fld1 fld2=$fld2 fld3=$fld3 fld4=$fld4 fld5=$fld5 fld6=$fld6 other=$other"
					fi
					if [[ "${fld1}" = "Vendor" ]] && [[ "${fld2}" = "identification:" ]]
						then
							dvendor=${fld3}
						else
					   		echo "${PROGNAME}: /dev/${dfile} Expected Vendor identification: line missing"
							exitcode=3
					fi
					#Now read Product ID
					read fld1 fld2 fld3 fld4 fld5 fld6 other
					if [[ "$debugOMP" -ne 0 ]]
						then
					echo "${PROGNAME}: For Product ID fld1=$fld1 fld2=$fld2 fld3=$fld3 fld4=$fld4 fld5=$fld5 fld6=$fld6 other=$other"
					fi
					if [[ "${fld1}" = "Product" ]] && [[ "${fld2}" = "identification:" ]]
						then
							#Skip disk if it is a floppy or cdrom - to support ILOM system
							echo "$fld3 $fld4 $fld5 $fld6 $other" | /bin/grep -i -e "floppy" -e "cdrom" > /dev/null
							[[ "$?" -eq 0 ]] && {
								if [[ "$debugOMP" -ne 0 ]] ; then
									echo "${PROGNAME}: /dev/${dfile} is a floppy or cdrom - Skipping Device"
								fi
								break
							}	
							dprodid=${fld3}
						else
					   		echo "${PROGNAME}: /dev/${dfile} Expected Product identification: line missing"
							exitcode=4
					fi
					#Now read Product Rev level
					read fld1 fld2 fld3 fld4 fld5 fld6 other
					if [[ "$debugOMP" -ne 0 ]]
						then
					echo "${PROGNAME}: For Product Rev fld1=$fld1 fld2=$fld2 fld3=$fld3 fld4=$fld4 fld5=$fld5 fld6=$fld6 other=$other"
					fi
					if [[ "${fld1}" = "Product" ]] && [[ "${fld2}" = "revision" ]] && [[ "${fld3}" = "level:" ]]
						then
							dprodrev=${fld4}
						else
					   		echo "${PROGNAME}: /dev/${dfile} Expected Product revision level: line missing"
							exitcode=5
					fi
					#Now read Storage serial number
					read fld1 fld2 fld3 fld4 fld5 fld6 other
					if [[ "$debugOMP" -ne 0 ]]
						then
					echo "${PROGNAME}: For Storage serial fld1=$fld1 fld2=$fld2 fld3=$fld3 fld4=$fld4 fld5=$fld5 fld6=$fld6 other=$other"
					fi
					if [[ "${fld1}" = "Unit" || "${fld1}" = "Product" ]] && [[ "${fld2}" = "serial" ]] && [[ "${fld3}" = "number:" ]]
						then
							dserial=${fld4}
						else
					   		echo "${PROGNAME}: /dev/${dfile} Expected Storage serial number: line missing"
							exitcode=6
					fi
					
					#Collect both page 83 and 80 serial numbers and serial# for udev usage
					dSCSIidx83="$(/sbin/scsi_id -g -p 0x83 -s /block/${dsysfile} 2>/dev/null)"
					if [[ "$?" -eq 0 ]]
						then
							dserial="${dSCSIidx83}"
							serialUDEV="${dSCSIidx83}"
						else
							dserial="No-x83"
							dSCSIidx83="No-x83"
					fi
					dSCSIidx80="$(/sbin/scsi_id -g -p 0x80 -s /block/${dsysfile} 2>/dev/null)"
					if [[ "$?" -eq 0 ]]
						then
							for dserialx80 in ${dSCSIidx80}
								do
								#Get last value no matter how many are output
								 if [[ "$debugOMP" -ne 0 ]]
									then
									echo "${PROGNAME}: dserialx80=$dserialx80"
								 fi
								done
							case "${dSCSIidx83}" in
								No-x83)
									dserial="${dserialx80}"
									serialUDEV="* ${dserialx80}"
									;;
								*) ;;	# do nothing for now
							esac
						else
							dSCSIidx80="No-x80"
							dserialx80="No-x80"
					fi
					if [[ "$debugOMP" -ne 0 ]]
						then
						echo "${PROGNAME}: Serial=$dserial IDx83=$dSCSIidx83 IDx80=$dserialx80"
					fi

					# Collect fields from sysfs real physical path to device
					devpath="$(/bin/ls -l /sys/block/${dsysfile}/device)"
					if [[ "$?" -ne 0 ]]
						then
							echo "${PROGNAME}: /bin/ls -l /sys/block/${dsysfile}/device FAILED "
							exitcode=7
					fi
					devpath=$(echo "${devpath}" | /bin/gawk -F '-> ' ' { print $2 } ' - )
					if [[ "$debugOMP" -ne 0 ]]
						then
						echo "${PROGNAME}: Device pathname for ${dfile}=${devpath}"
					fi
					dscsiid=${devpath##*/}
					dlun=${dscsiid##*:}
					devpath=${devpath%/*}
					ddskport=${devpath##*/}
					schedgrp=$(echo ${ddskport} | /usr/bin/tr -d ':')
					devpath=${devpath%/*}
					dhba=${devpath##*/}
					#On RH5 the HBA name was proceeded by rport info .. so skip rport in path
					tdhba=$(echo $dhba | /usr/bin/tr '[:upper:]' '[:lower:]')
					if [[ "host" != "${tdhba:0:4}" ]]
						then
							devpath=${devpath%/*}
							dhba=${devpath##*/}
					fi
							
					devpath=${devpath%/*}
					dpcislot=${devpath##*/}
					devpath=${devpath%/*}
					dsbus=${devpath##*/}
					devpath=${devpath%/*}
					ddir=${devpath##*/}

					#Collect FC information about scsi device
					if [[ -r /sys/class/fc_transport/${ddskport}/port_id ]]
						then
							dportid=$(/bin/cat /sys/class/fc_transport/${ddskport}/port_id)
							if [[ "$?" -ne 0 ]]
								then
									echo "${PROGNAME}: /bin/cat /sys/class/fc_transport/${ddskport}/port_id  FAILED "
									exitcode=8
							fi
						else
							if [[ "$debugOMP" -ne 0 ]]
								then
									echo "${PROGNAME}: /sys/class/fc_transport/${ddskport}/port_id does not exist"
							fi
					fi
					if [[ -r /sys/class/fc_transport/${ddskport}/port_name ]]
						then
							dportname=$(/bin/cat /sys/class/fc_transport/${ddskport}/port_name)
							if [[ "$?" -ne 0 ]]
								then
									echo "${PROGNAME}: /bin/cat /sys/class/fc_transport/${ddskport}/port_name  FAILED "
									exitcode=9
							fi
						else
							if [[ "$debugOMP" -ne 0 ]]
								then
									echo "${PROGNAME}: /sys/class/fc_transport/${ddskport}/port_name does not exist"
							fi
					fi

					
					#Get Disk size and sector information
					#Do double sg_readcap because on RH5 first one always fails after LIP in SAN was done
					devsizes="$(/usr/bin/sg_readcap /dev/${dfile} >/dev/null 2>&1)"
					devsizes="$(/usr/bin/sg_readcap /dev/${dfile})"
					if [[ "$?" -ne 0 ]]
						then
							echo "${PROGNAME}: /usr/bin/sg_readcap /dev/${dfile} FAILED "
							exitcode=10
					fi
					devsizes=$(echo "${devsizes}" | /usr/bin/tr -s ' =,:' ' ' | /bin/gawk '
						/Last block address/ { dsectors = $9 }
						/Last logical block address/ { dsectors = $10 }
						/Block size/ { dsectorsz = $3 }
						/Logical block length/ { dsectorsz = $4 }
						/Device size/ { dsize = $5 }
						END { print dsectors ":" dsectorsz ":" dsize }
						' - )
					if [[ "$debugOMP" -ne 0 ]]
						then
					echo "${PROGNAME}: Device sectors:sector-size:number-Mbytes=${devsizes}"
					fi
					dsize=${devsizes##*:}
					devsizes=${devsizes%:*}
					dsectorsz=${devsizes##*:}
					devsizes=${devsizes%:*}
					dsectors=${devsizes##*:}

					#Create entry in udev rules file for device
					case $REDHAT_VER in
					 4) echo 'KERNEL="sd*", SYSFS{vendor}="'$dvendor'", SYSFS{model}="'$dprodid'", ID="'$dscsiid'", RESULT="'$serialUDEV'", NAME="'$dfile'"' >> ${rulesfn} ;;
					 5) echo 'KERNEL=="sd*", SYSFS{vendor}=="'$dvendor'", SYSFS{model}=="'$dprodid'", ID=="'$dscsiid'", RESULT=="'$serialUDEV'", NAME="'$dfile'"' >> ${rulesfn} ;;
				 	 *) 
				    	    echo "${PROGNAME}: udev entry for $serialUDEV ERROR - Linux version not recognized REDHAT_VER=$REDHAT_VER"
				    	    exitcode=14
				    	    exit ${exitcode}
				    	    	;;
					esac
					
					#Print out all attributes collected for one particular disk
					echo  $schedgrp','$dvendor','$dprodid','$dprodrev','$dserial','$dserialx80','$dsize','$dscsiid','$ddskport','$dlun','$dhba','$dpcislot','$dsbus','$dportid','$dportname','$dsectors','$dsectorsz','$dfile
					if [[ "$debugOMP" -ne 0 ]]
						then
							echo "################################################"
					fi

			#Finished ALL processing for a Single Disk
			fi
#			if [[ "${exitcode}" -ne 0 ]]
#				then
#					exit ${exitcode}
#			fi
		done
		exitcode=$?
	   }
	done
exit ${exitcode}
