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
#  $Id: omp_selectctlr.ksh,v 1.5 2010/01/13 19:37:51 naat0 Exp $
#
# Name: omp_selectctlr
#
#   Returns: 
#           0 - Disk Selection process completed successful
#    non-zero - Not all information about scsi disks could not be processed
#
#  This script will create source and target csv files based on user selection.
#  Script depends on the output file produced from the omp_getscsidsk command
#
######################################################################
#
PROGNAME=${0##*/}
typeset -a schedgrp dvendor dprodid dprodrev dserial dsize dscsiid ddskport dlun dhba dpcislot dsbus
typeset -a dportid dportname dsectors dsectorsz dfile ctlrluns dserialx80
typeset -i exitcode=0 cnt=1 ShowAll=0
typeset -i debugOMP=0 num=0 badnum=0 filenmt_flg=0
typeset arg srcnums tgtnums tgtunits dummy answer='N' filenm filenmt badnumlst hdparm
typeset omp_home="/etc/opt/SFTKomp"
typeset omp_util_dir="${omp_home}/jobs/san"
typeset omp_bin="${omp_home}/bin"
typeset omp_tmp="${omp_home}/tmp"
typeset omp_tunables="omp_tunables"
typeset omp_getscsidsk_out="omp_getdskout.csv"
typeset scsidskall_nohdr="scsiall_nohdr.csv"
typeset scsidskall_nohdr_lowlun="scsiall_nohdr_lowlun.csv"
typeset scsidskall_nohdr_duplun="scsiall_nohdr_duplun.txt"
typeset scsidsk_uniq="scsiall_uniq.csv"
typeset scsidsk_uniq_serial="scsiall_uniq_serial.csv"
typeset srcfile="source.omp"
typeset tgtfile="target.omp"
typeset omp_getdsk="omp_getscsidsk"
typeset WhatTDMF="RFX"
typeset SG_PERSIST="/usr/bin/sg_persist"
typeset RKey="0x426367" REDHAT_DIST="/etc/redhat-release" REDHAT_VER=""
shopt -s extglob

userid=$(/usr/bin/id -u)
if [ "$?" -ne 0 ] || [ "${userid}" -ne 0 ]
then
    echo "${PROGNAME}: root authority required .. login or su to root"
    exitcode=99
    exit ${exitcode}
fi     

#Verify sg3_utils is installed before running script 
	/bin/grep -i "release 4" $REDHAT_DIST >/dev/null && REDHAT_VER="4"
	/bin/rpm -q sg3_utils >/dev/null 2>&1
	if [[ "$?" -ne 0 ]]
		then
			echo "${PROGNAME}: Required sg3_utils package not install on system"
			[[ "$REDHAT_VER" = "4" ]] && {		
			echo "                PLEASE Install sg3_utils-1.22-3.1 or higher and re-run ${PROGNAME}"
			}
			exitcode=90
			exit ${exitcode}
	elif [[ ! -e $SG_PERSIST ]] #Make sure command exist to check persistent reserves
		then
			echo "${PROGNAME}: Required $SG_PERSIST command is missing"
			[[ "$REDHAT_VER" = "4" ]] && {		
			echo "                PLEASE Install sg3_utils-1.22-3.1 or higher and re-run ${PROGNAME}"
			}
			exitcode=190
			exit ${exitcode}
	fi
#Verify TDMFIP is installed before running script 
	(/bin/rpm -q TDMFIP || /bin/rpm -q Replicator) >/dev/null 2>&1
	if [[ "$?" -ne 0 ]]
		then
			echo "${PROGNAME}: Required %PRODUCTNAME% package not install on system."
			echo "                PLEASE Install %PRODUCTNAME% and re-run ${PROGNAME}"
			exitcode=91
			exit ${exitcode}
	fi
	/opt/SFTKdtc/bin/dtclicinfo -q >/dev/null 2>&1 || {
			echo "${PROGNAME}: Required %PRODUCTNAME% license does not exist or invalid on system"
			echo "                PLEASE correct the issue noted below and re-run ${PROGNAME}"
			/opt/SFTKdtc/bin/dtclicinfo
			sleep 4
	}

######### FUNCTIONS #######################################################
read_uniq()
{
cnt=1
IFS=','
while read  schedgrp[cnt] dvendor[cnt] dprodid[cnt] dprodrev[cnt] dserial[cnt] dserialx80[cnt] dsize[cnt] dscsiid[cnt] ddskport[cnt] \
dlun[cnt] dhba[cnt] dpcislot[cnt] dsbus[cnt] dportid[cnt] dportname[cnt] dsectors[cnt] dsectorsz[cnt] dfile[cnt]
	do
		ctlrluns[cnt]=$(/bin/grep -w ${ddskport[cnt]} $omp_tmp/$scsidskall_nohdr | /usr/bin/wc -l)
		if [[ "$debugOMP" -ne 0 ]]
			then
				echo "Line number = $cnt is ..."
				echo "${schedgrp[cnt]},${dvendor[cnt]},${dprodid[cnt]},${dprodrev[cnt]},${dserial[cnt]},${dserialx80[cnt]},${dsize[cnt]},\
${dscsiid[cnt]},${ddskport[cnt]},${dlun[cnt]},${dhba[cnt]},${dpcislot[cnt]},${dsbus[cnt]},${dportid[cnt]},\
${dportname[cnt]},${dsectors[cnt]},${dsectorsz[cnt]},${dfile[cnt]}"
		fi
		cnt=cnt+1
	done < ${omp_tmp}/${scsidsk_uniq}
unset IFS
return ${exitcode}
}
###############
displaysrc_uniq()
{
	echo " Please select the SOURCE Storage units:"
	for (( cnt=1 ; cnt < ${#dserial[*]} ; cnt++ ))
		do
			printf "\n %2d) VEN=%-8s MOD=%-12s REV=%-4s #LUNS=%-3s\n" "$cnt" "${dvendor[cnt]}" "${dprodid[cnt]}" "${dprodrev[cnt]}" "${ctlrluns[cnt]}"
			printf "      SER#=%-34s SER#x80=%-15s\n" "${dserial[cnt]}" "${dserialx80[cnt]}"
			printf "      WWPN=%-18s SCSIID=%-8s HBA=%-5s\n" "${dportname[cnt]}" "${dscsiid[cnt]}" "${dhba[cnt]}"
		done

	echo " "
	echo " Enter only the number(s) separated by spaces"
	read -p " (e.g. 1 3 4): " srcnums
	if [[ "$?" -ne 0 ]]
		then
			if [[ "$debugOMP" -ne 0 ]]
				then
					echo " "
					echo "$PROGNAME: Selection of SOURCE Storage units has been INTERRUPTED"
			fi
		exitcode=100
		exit ${exitcode}
	fi
	if [[ "$debugOMP" -ne 0 ]]
		then 
			echo "$PROGNAME: User selected Sources -> $srcnums"
	fi 
return ${exitcode}
}
###############
displaytgt_uniq()
{
	echo " Please select the TARGET Storage units:"
	for cnt in $(echo ${tgtunits})
		do
			printf "\n %2d) VEN=%-8s MOD=%-12s REV=%-4s #LUNS=%-3s\n" "$cnt" "${dvendor[cnt]}" "${dprodid[cnt]}" "${dprodrev[cnt]}" "${ctlrluns[cnt]}"
			printf "      SER#=%-34s SER#x80=%-15s\n" "${dserial[cnt]}" "${dserialx80[cnt]}"
			printf "      WWPN=%-18s SCSIID=%-8s HBA=%-5s\n" "${dportname[cnt]}" "${dscsiid[cnt]}" "${dhba[cnt]}"
		done

	echo " "
	echo " Enter only the number(s) separated by spaces"
	read -p " (e.g. 2 5 6): " tgtnums
	if [[ "$?" -ne 0 ]]
		then
			if [[ "$debugOMP" -ne 0 ]]
				then
					echo " "
					echo "$PROGNAME: Selection of TARGET Storage units has been INTERRUPTED"
			fi
		exitcode=101
		exit ${exitcode}
	fi
	if [[ "$debugOMP" -ne 0 ]]
		then 
			echo "$PROGNAME: User selected Targets -> $tgtnums"
	fi 
return ${exitcode}
}
############
get_srcunits()
{
while true
do
  while true
  do
	badnum=0
	displaysrc_uniq
	if [[ "$debugOMP" -ne 0 ]]
		then
			echo "${PROGNAME}: srcnums =*$srcnums*"
	fi
	badnumlst=""
	if [[ ! "${srcnums:-NOTSET}" = "NOTSET" ]]
		then 
			for arg in $(echo ${srcnums})
			do
				if [[ "$debugOMP" -ne 0 ]]
					then
						echo "${PROGNAME}: num =*$arg*"
				fi
				if [[ "$arg" != +([0-9]) ]] || [[ ! "$arg" -ge 1 ]]  || [[ ! "$arg" -lt "${#dserial[*]}" ]]
					then
						badnum=1
						exitcode=2
						if [[ "$debugOMP" -ne 0 ]]
							then
								echo "${PROGNAME}: badnum set to 1"
						fi	
						if [[ -z "$badnumlst" ]]
							then
								badnumlst="$arg"
							else
								badnumlst="${badnumlst} $arg"
						fi
				fi
			done
		else
			unset num
			badnum=1
			exitcode=20
			echo " Error: No Source Units entered"
	fi
	if [[ "$badnum" -eq 0 ]]
		then
			exitcode=0
			break
	fi	
	read -p " Your selected number(s) ${badnumlst} is not in the list (press enter to continue)  " dummy
	echo " "
  done
  #User must confirm selection or ask again
  echo " "
  echo "You selected the following as SOURCE units:" 
  for num in $(echo ${srcnums})
  	do
		printf "\n %2d) VEN=%-8s MOD=%-12s REV=%-4s #LUNS=%-3s\n" "$num" "${dvendor[num]}" "${dprodid[num]}" "${dprodrev[num]}" "${ctlrluns[num]}"
		printf "      SER#=%-34s SER#x80=%-15s\n" "${dserial[num]}" "${dserialx80[num]}"
		printf "      WWPN=%-18s SCSIID=%-8s HBA=%-5s\n" "${dportname[num]}" "${dscsiid[num]}" "${dhba[num]}"
	done
	echo " "
	read -p " Is this list correct? [Y or N]: " answer
	echo " "
	if [[ "$debugOMP" -ne 0 ]]
		then 
			echo "$PROGNAME: source list answer = $answer"
	fi 
	case $answer in
		[Yy]) return ${exitcode} ;;
		*) ;;  # Re-Display List and let user select again
	esac
done
return ${exitcode}
}

####################
get_tgtunits()
{
answer='N'
for (( cnt=1 ; cnt < ${#dserial[*]} ; cnt++ ))
	do
		if [[ "$debugOMP" -ne 0 ]]
			then
			echo "$PROGNAME: echo ${srcnums} | /bin/grep -w $cnt > /dev/null "
		fi
		echo ${srcnums} | /bin/grep -w $cnt > /dev/null
		if [[ ! "$?" -eq 0 ]]
			then
				if [[ -z "$tgtunits" ]]
					then
						tgtunits="$cnt"
					else
						tgtunits="${tgtunits} $cnt"
				fi
		fi
	done
if [[ "${tgtunits:-NOTSET}" = "NOTSET" ]]
	then
		echo " ERROR: You have selected all units as Source. "
		echo " PLEASE re-run $PROGNAME and select unique source and target unit(s) lists"
		exitcode="80"
		exit ${exitcode}
fi

while true
do
  while true
  do
	badnum=0
	displaytgt_uniq
	if [[ "$debugOMP" -ne 0 ]]
		then
			echo "${PROGNAME}: tgtnums =*$tgtnums*"
	fi
	badnumlst=""
	if [[ ! "${tgtnums:-NOTSET}" = "NOTSET" ]]
		then 
			for arg in $(echo ${tgtnums})
			do
				if [[ "$debugOMP" -ne 0 ]]
					then
						echo "${PROGNAME}: num(tgt) =*$arg*"
				fi
				if [[ $(echo $srcnums | /bin/grep -w $arg) ]] || [[ "$arg" != +([0-9]) ]] || [[ ! "$arg" -ge 1 ]]  || [[ ! "$arg" -lt "${#dserial[*]}" ]]
					then
						badnum=1
						exitcode=3
						if [[ "$debugOMP" -ne 0 ]]
							then
								echo "${PROGNAME}: badnum (tgt) set to 1"
						fi	
						if [[ -z "$badnumlst" ]]
							then
								badnumlst="$arg"
							else
								badnumlst="${badnumlst} $arg"
						fi
				fi
			done
	else
		unset num
		badnum=1
		exitcode=30
		echo " Error: No Target Units entered"
	fi
	if [[ "$badnum" -eq 0 ]]
		then
			break
	fi	
	read -p " Your selected number(s) ${badnumlst} is not in the target list (press enter to continue)  " dummy
	echo " "
  done
  #User must confirm selection or ask again
  echo " "
  echo "You selected the following as TARGET units:" 
  for num in $(echo ${tgtnums})
  	do
		printf "\n %2d) VEN=%-8s MOD=%-12s REV=%-4s #LUNS=%-3s\n" "$num" "${dvendor[num]}" "${dprodid[num]}" "${dprodrev[num]}" "${ctlrluns[num]}"
		printf "      SER#=%-34s SER#x80=%-15s\n" "${dserial[num]}" "${dserialx80[num]}"
		printf "      WWPN=%-18s SCSIID=%-8s HBA=%-5s\n" "${dportname[num]}" "${dscsiid[num]}" "${dhba[num]}"
	done
	echo " "
	read -p " Is this list correct? [Y or N]: " answer
	if [[ "$debugOMP" -ne 0 ]]
		then 
			echo "$PROGNAME: source list answer = $answer"
	fi 
	case $answer in
		[Yy]) return ${exitcode} ;;
		*) ;;  # Re-Display List and let user select again
	esac
done
return ${exitcode}
}
#################
createcsv()
{
filenmt_flg=0
case $1 in
	srcnums) 
			filenm=$srcfile 
			filenmt="Source"
			;;
	tgtnums) 
			filenm=$tgtfile 
			filenmt="Target"
			;;
	*)
		#Bad parm passed
		echo 'Bad value passed for $1 = '$1
		exitcode=10
		return ${exitcode}
		;;
esac
/bin/grep 'vendor,prodid,prodrev' $omp_tmp/$omp_getscsidsk_out > $omp_util_dir/$filenm
if [[ "$debugOMP" -ne 0 ]]
	then
		echo "$PROGNAME: Saving \$${1} = ""$(eval echo \$${1})"
fi
for num in $(eval echo \$${1})
	do
		if [[ "$debugOMP" -ne 0 ]]
			then
				echo "$PROGNAME: num =*$num*"
				echo "$PROGNAME: /bin/grep -w ${ddskport[num]} $omp_tmp/$scsidskall_nohdr >> $omp_util_dir/$filenm"
		fi

		/bin/grep -w ${ddskport[num]} $omp_tmp/$scsidskall_nohdr >> $omp_util_dir/$filenm
		if [[ "$filenmt_flg" -eq 0 ]]
			then
				filenmt_flg=1
				if [[ "$filenmt" = "Source" ]]
					then
					echo " "
				fi
				echo "${filenmt} Storage Units saved in -> $omp_util_dir/$filenm"
		fi
	done 

return ${exitcode}
}
###############
set_hdparm()
{
#Check/Clear Persistent Reserves and Set SD devices to R/O (Source) and R/W (Target)
answer='N'
cnt=1
IFS=','
case $1 in
	Source) 
			filenm=$srcfile
			hdparm="-r1"
			;;
	Target) 
			filenm=$tgtfile
			hdparm="-r0"
			;;
	*)
		#Bad parm passed
		echo "${PROGNAME}: Bad value passed to set_hdparm = " $1
		exitcode=11
		return ${exitcode}
		;;
esac
exec 3<$omp_util_dir/$filenm
while read -u 3 schedgrp[cnt] dvendor[cnt] dprodid[cnt] dprodrev[cnt] dserial[cnt] dserialx80[cnt] dsize[cnt] dscsiid[cnt] ddskport[cnt] \
dlun[cnt] dhba[cnt] dpcislot[cnt] dsbus[cnt] dportid[cnt] dportname[cnt] dsectors[cnt] dsectorsz[cnt] dfile[cnt]
	do
		if [[ ! "${dsize[cnt]}" = "sizeMB" ]] ; then
		     $SG_PERSIST -n -r /dev/${dfile[cnt]} 2>&1 | /bin/grep -i -e "no reservation held" -e "command not supported" > /dev/null
		     [[ "$?" -ne 0 ]] && {
			while true    #Break out of loop only answer = Y or N or ALL 
			do
			  [[ "$answer" != "ALL" ]] && {
				echo " "
				echo "** SCSI RESERVE Detected **"	
				echo "Device SERIAL=${dserial[cnt]} WWPN=${dportname[cnt]} has a SCSI Reserve Held"
				echo "It is NOT recommended to clear a reserve while a server is still using the LUN"
				echo "Would you like to try and Clear the SCSI reserve and all LUN Registrants?"
				echo "(ALL - will automatically clear reserves for every $1 LUN)"
				read  -p " [Y or N or ALL]: " answer
				echo " "
			  }
			  answer=`echo $answer | /usr/bin/tr '[:lower:]' '[:upper:]'`
			  case $answer in
				Y|ALL) 
					if [[ "$debugOMP" -ne 0 ]] ; then
				  	  echo "$PROGNAME: $SG_PERSIST -n --out --register-ignore --param-sark=$RKey /dev/${dfile[cnt]} > /dev/null"
					  echo "$PROGNAME: $SG_PERSIST -n --out --clear --param-rk=$RKey /dev/${dfile[cnt]} > /dev/null"
			  		  echo "$PROGNAME: $SG_PERSIST -n -r /dev/${dfile[cnt]} | /bin/grep -i "no reservation held" > /dev/null"
					fi
					#Register I_T nexus, Clear SCSI Reserve and check if SCSI reserve cleared
					$SG_PERSIST -n --out --register-ignore --param-sark=$RKey /dev/${dfile[cnt]} > /dev/null
					$SG_PERSIST -n --out --clear --param-rk=$RKey /dev/${dfile[cnt]} > /dev/null
			  		$SG_PERSIST -n -r /dev/${dfile[cnt]} | /bin/grep -i "no reservation held" > /dev/null
					[[ "$?" -ne 0 ]] && {
						echo " "
						echo " ** WARNING **"
						echo "Could NOT clear the SCSI reserve for SERIAL=${dserial[cnt]} WWPN=${dportname[cnt]}"
						echo "The SCSI Reserve must be cleared from Storage Unit or Application Node"
						echo "Otherwise OMP may not be able to access LUN"
						read -p " (press enter to continue)  " dummy
						echo " "
						}
					break
				;;
				N) 
					echo " "
					echo " ** WARNING **"
					echo "You chose NOT to clear the SCSI reserve for SERIAL=${dserial[cnt]} WWPN=${dportname[cnt]}"
					echo "OMP may not be able to access this LUN"
					read -p " (press enter to continue)  " dummy
					echo " "
					break
				;;
				*) ;;  # Re-Display message and let user select again
			   esac
			done
		     }
		     if [[ "$debugOMP" -ne 0 ]]
		        then
			   echo "hdparm set Line number = $cnt is ..."
			   echo "$PROGNAME: /sbin/hdparm $hdparm /dev/${dfile[cnt]}"
		     fi
		     /sbin/hdparm $hdparm /dev/${dfile[cnt]} > /dev/null
		fi
		cnt=cnt+1
	done 
unset IFS
exec 3<&-
return ${exitcode}
}

######### END OF FUNCTIONS ################################################
#
#### MAIN #################################################################
#First source flag file to pick up user changes to variables
if [[ -r  ${omp_home}/${omp_tunables} ]]
then
	#Pick up user variable changes by sourcing file
	if (set -e; . ${omp_home}/${omp_tunables}) >/dev/null 2>&1
	then
		. ${omp_home}/${omp_tunables}
	else
		echo ${PROGNAME}: Errors detected while processing ${omp_home}/${omp_tunables} file.
	fi
	# TODO Validate values
	[[ -n "${DebugSelectCtlr:-}" ]] && debugOMP=${DebugSelectCtlr}
fi  
while getopts ":a" arg
do
    case ${arg} in
        a)
            ShowAll=1
        ;;
        *)
 	    echo "ERROR: The option -$OPTARG is not a valid parameter"
 	    echo "${PROGNAME} [-a]"
 	    echo "               -a  Show all discovered target disk controllers"
            exitcode=223
            exit $exitcode
        ;;
    esac
done


/usr/bin/clear
echo "Collecting SCSI Disk Information"
if [[ ! -f ${omp_bin}/${omp_getdsk} ]]
	then
		echo "${PROGNAME}: Error The SCSI disk program -> ${omp_bin}/${omp_getdsk} <- can't be found"
		exit 1
fi
${omp_bin}/${omp_getdsk} > ${omp_tmp}/${omp_getscsidsk_out}
## Show any Errors or non-Data output in the file
/bin/grep -v ',' ${omp_tmp}/${omp_getscsidsk_out}
echo "################################"
# Save without header and sort by lun number within each Target port name
/bin/grep ',' $omp_tmp/$omp_getscsidsk_out | /bin/grep -v 'vendor,prodid,prodrev' | LC_ALL='C' /bin/sort -t, +8 -9 +9n -10 > $omp_tmp/$scsidskall_nohdr

#Warn user if any LUNs are detected on more than one target disk controller
/bin/gawk -F"," '{ print $5 }' $omp_tmp/$scsidskall_nohdr | LC_ALL='C' /bin/sort | /usr/bin/uniq -d > $omp_tmp/$scsidskall_nohdr_duplun
if [[ -s $omp_tmp/$scsidskall_nohdr_duplun ]] ; then
	echo "***********************************************************************"
	echo "* WARNING ** Some LUNs were discovered on more than one Storage Unit **"
  if [[ "$ShowAll" -eq 0 ]] ; then
	echo "* If you need to also select excluded Storage Unit(s) then you must   *"
	echo "* CTRL-c and run the command:                                         *" 
	echo "* ${PROGNAME} -a                                                   *"
  fi
	echo "***********************************************************************"
fi

#Save only Unique Target ports thus only lowest lun per target will be saved
LC_ALL='C' /bin/sort -t, -u +8 -9 $omp_tmp/$scsidskall_nohdr > $omp_tmp/$scsidskall_nohdr_lowlun

#Now Get rid of duplicate LUNs in the file
LC_ALL='C' /bin/sort -t, -u +4 -5 $omp_tmp/$scsidskall_nohdr_lowlun > ${omp_tmp}/${scsidsk_uniq_serial}
   
# Sort by Vendor and Model
if [[ "$ShowAll" -eq 1 ]]
  then
   # Save all discovered target disk controllers
   LC_ALL='C' /bin/sort -t, +1 -2 $omp_tmp/${scsidskall_nohdr_lowlun} > ${omp_tmp}/${scsidsk_uniq}
  else
   # If multiple controllers have the same LUNs only show one controller
   LC_ALL='C' /bin/sort -t, +1 -2 $omp_tmp/${scsidsk_uniq_serial} > ${omp_tmp}/${scsidsk_uniq}
fi

## Read unique Disk units discovered 
read_uniq

## Display uniq Unit list and read user selection(s) for Source Storage Units
get_srcunits

## Display Unit list minus Source units to get  user selection(s) for Target Storage Units
get_tgtunits

## Create Source CSV file
createcsv srcnums 

## Create Target CSV file
createcsv tgtnums 

## Set R/O or R/W for each /dev/sd.. file and Check/clear SCSI Reserve
set_hdparm Source
set_hdparm Target

exit ${exitcode}

