#!/bin/ksh93
###############################################################################
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
###############################################################################

###############################################################################
#  
#  Name:  cl_deactivate_dtc
#
#  Returns:
#       0 - All of the dtc devices are successfully varied off for Resource Group
#
#  This function will activate the the dtc devices for volume groups passed in as arguments.
#
#  Arguments:   volume group list
#              
#  Environment: VERBOSE_LOGGING, PATH
#
#
###############################################################################
#

PROGNAME=${0##*/}
#env > /tmp/cl_dtc_deact_env
# Taking all parameters as a list of volume groups is most likely a bug, since the cl_deactivate_vgs script calls us with
# a single group parameter, and in some versions (AIX 6.1, RFX 2.7.0) with a $MODE argument as well.
typeset vgargs="$*"
typeset -i vgargs_num=$# 
ClustPath="/usr/es/sbin/cluster"

##### Target lvol name changes defined #####
typeset tgtprefix=""
typeset tgtsuffix="X"
##### DTC volume name changes defined ######
typeset dtcprefix="lg"
typeset dtcsuffix="dtc"
typeset dtcsep="-"
##### For symbolic links source volume name changes defined ####
typeset srcprefix=""
typeset srcsuffix="_OLD"
###########################################  

typeset -i debugSFTKdtc=0
typeset -i usingSFTKdtc=1
typeset -i usingSymbolicSFTKdtc=1
typeset -i SleepIntervalSFTKdtc=3
typeset dtcvolname="" dtcvol="" dtcrvol="" srcvol="" tgtvol="" volgrp="" logvol="" rscgrp=""  clustrscs="" lsvglvols=""
typeset srcrvol=""  cfgfile=""
typeset dynamic_activation=""
typeset -i DA_on=0
set -A  rslist vglist lvlist lglist lgnumlist 
typeset dtcdir="/dev/"
typeset srcdir="/dev/"
typeset tgtdir="/dev/"
typeset dtcrsc_file="/etc/dtc/hacmp/cl_dtc_rsgrps"
typeset dtcflag_file="/etc/dtc/hacmp/cl_dtc_flags"
typeset dtclib="/etc/dtc/lib/"
typeset dtc_rscgrps="" other="" fieldname="" dtcstopgrps="" dtcgroup=""
typeset -i rscount exitcode dtccount 

TMP_FILE="/tmp/cl_deactivate_dtc.tmp"
#   Remove the status file if already exists
rm -f $TMP_FILE
##############################################################################

dtc_killpmd_and_stop_group()
{
    if (( debugSFTKdtc != 0 ))
    then
        echo "${PROGNAME}: DEBUG: /usr/dtc/bin/killpmds -g ${lgnumlist[rscount]}"
    fi
    /usr/dtc/bin/killpmds -g ${lgnumlist[rscount]}  2>/dev/null | grep "No .* PMD daemons were running"
    
    if [[ $? -ne 0 ]]
    then
        # If PMDs were running, we need to leave them time to shut down properly before dtcstop is run.
        if (( debugSFTKdtc != 0 ))
        then
            echo "${PROGNAME}: DEBUG: sleep ${SleepIntervalSFTKdtc}"
        fi
	    sleep ${SleepIntervalSFTKdtc}
    fi
    
    if (( debugSFTKdtc != 0 ))
    then
        echo "${PROGNAME}: DEBUG: /usr/sbin/dtcstop -o -g ${lgnumlist[rscount]}"
    fi

    /usr/sbin/dtcstop -o -g ${lgnumlist[rscount]}  2>/dev/null
    exitcode=$?

    return ${exitcode}
}

dtc_stop_group()
{
exitcode=0
    if [ -z "${dtcstopgrps}" ]
          then
                # save the logical group number of first stopped group
               dtcstopgrps="${lgnumlist[rscount]}"
                if (( debugSFTKdtc != 0 ))
                  then
                    echo "${PROGNAME}: First Group to be stopped = ${dtcstopgrps} "
                fi

                dtc_killpmd_and_stop_group
     		    exitcode=$?
          else
                # add the logical group number to the list of groups stopped
                echo ${dtcstopgrps} | fgrep -w -s ${lgnumlist[rscount]}
                if [[ $? -ne 0 ]]
                    then

                        dtc_killpmd_and_stop_group
                        exitcode=$?

                        # add the logical group to the saved group list to be started at the end
                        dtcstopgrps="${dtcstopgrps} ${lgnumlist[rscount]}"
                        if (( debugSFTKdtc != 0 ))
                           then
                              echo "${PROGNAME}:List of groups that have been stopped = ${dtcstopgrps} "
                        fi
                fi
    fi

     if [[ ${exitcode} -ne 0 ]]
     then
 	echo "${PROGNAME}: Problems stopping group ${lgnumlist[rscount]} "
     fi
return ${exitcode}
}
#########################
read_cfg_file()
{
cfgfile=${dtclib}p${lgnumlist[rscount]}.cfg
echo "${PROGNAME}: read_cfg_file is processing: ${cfgfile}"
unset dtcDevice dataDisk
set -A  dtcDevice dataDisk
dtccount=0
exitcode=0

# Get the dynamic activation state for the current group in order to enable or not DTC device  
dynamic_activation=$(awk '/DYNAMIC-ACTIVATION/ {printf $2 }' $cfgfile)
if [[ "$dynamic_activation" = "on" ]] || [[ "$dynamic_activation" = "ON" ]]; then   
  DA_on=1
fi

cat ${cfgfile} | while read fieldname dtcDevice[dtccount] other
do
        #Find the DTC-DEVICE:
        if [[ "${fieldname}" = "DTC-DEVICE:" ]]
           then
                #Now read the corresponding DATA-DISK:
                read fieldname dataDisk[dtccount] other
                if [[ "${fieldname}" = "DATA-DISK:" ]]
                   then
                     if (( debugSFTKdtc != 0 ))
                       then
                         echo "${PROGNAME}: ${cfgfile} DTC-DEVICE=${dtcDevice[dtccount]} assigned DATA-DISK=${dataDisk[dtccount]}"
                     fi
                     dtccount=dtccount+1
                   else
                     unset dataDisk[dtccount]
                     echo "${PROGNAME}: ERROR: ${cfgfile} DTC-DEVICE=${dtcDevice[dtccount]} does not have a DATA-DISK"
                     exitcode=1

                fi
        fi
done
return ${exitcode}
}
   
########################################
find_dtc_device()
{
echo "${PROGNAME}: find_dtc_device is processing logical volume: ${logvol}"
dtccount=0
exitcode=0

while (( dtccount < ${#dataDisk[*]} ))
do
  #Find where logvol is indexed in the dataDisk array
  if [ "${srcdir}r${logvol}" = "${dataDisk[dtccount]}" ] || [ "${srcdir}r${srcprefix}${logvol}${srcsuffix}" = "${dataDisk[dtccount]}" ]
    then
       #set the dtc block and character device names then return to caller
        IFS="/"
        eval set -- `echo ${dtcDevice[dtccount]}`
        unset IFS
        if (( debugSFTKdtc != 0 ))
          then
            echo "${PROGNAME}: ${srcdir}r${logvol} DTC-DEVICE levels L1=$1 L2=$2 L3=$3 L4=$4 l5=$5"
        fi
        dtcvol=${dtcdir}${3}${5}
        dtcrvol=${dtcdir}'r'${3}${5}
        return ${exitcode}
  fi
  dtccount=dtccount+1

done
#dtc device was not found for the current logical volume
echo "${PROGNAME}: ${srcdir}r${logvol} for group ${rslist[rscount]} not found in ${cfgfile}"
echo "${PROGNAME}: ${logvol} in VG=${volgrp} will not be Processed"
unset dtcvol
unset dtcrvol
return 1
} 
############################################################
deactivate_dtc_volumes()
{
echo "${PROGNAME}: deactivate_dtc_volumes is processing Volume Group: ${volgrp}"
rscount=0
exitcode=0 

while (( rscount < ${#vglist[*]} ))
do
  #Find where volgrp is indexed in the vglist array
  echo ${vglist[rscount]} | fgrep -w -s ${volgrp}
  if [[ $? -eq 0 ]]
    then
     dtc_stop_group

     # If using symbolic link is set then move source LVs back to original name
     if [[ ${usingSymbolicSFTKdtc} -eq 1 ]]
       then
          #  Get the list of lvols for the volume group we are processing
          lsvglvols=`lsvg -L -l ${volgrp} | awk ' NF > 4 && !/^LV NAME/ { print $1 }'`
          #If no Active logical volumes for this volume group then return to caller
               case ${lsvglvols} in
                *[a-zA-Z0-9]*) : ;;
                *)
                  echo "${PROGNAME}: No active logical volumes for these volume group : ${volgrp} "
                  return 1 ;;
               esac       
          # Read the dtc config file for the current Resource Group
          read_cfg_file 

          # For each logical volume verify they are in the cluster and process symbolic links
          for logvol in ${lsvglvols}
          do
          # If the logvol has pstore in its name then do not process a source volume
          case ${logvol} in
           *pstore*)
                    if (( debugSFTKdtc != 0 ))
                    then
                        echo "${PROGNAME}: ${srcdir}${logvol} is the pstore for group ${rslist[rscount]} lg=${lgnumlist[rscount]}"
                    fi
                ;;
            *)  
            # Find the DTC and Data Device for the current logical volume
            find_dtc_device 
            # If the dtc device for the current logical volume was found the process logical volume
            if [[ ! -z "${dtcvol}" ]] && [[ ! -z "${dtcrvol}" ]]
              then

                # Dynamic activation is not enable 'off' then use the old fashion mecanism to hook the device
                if ((DA_on == 0 ));
                   then    
              
                     # Setup volume names and move source volume back 
                     srcvol=${srcdir}${srcprefix}${logvol}${srcsuffix}
                     srcrvol=${srcdir}'r'${srcprefix}${logvol}${srcsuffix}
		             if [[ -r ${srcvol} ]]
    	                 then
 	   	                   if (( debugSFTKdtc != 0 ))
    		                   then
 		                          echo "${PROGNAME}: mv ${srcvol} ${srcdir}${logvol}"
 		                   fi
                           #Move source block device to current lvol block name
		                   mv ${srcvol} ${srcdir}${logvol}
                           if [[ "$?" -ne 0 ]]
                              then
	                             echo "${PROGNAME}: Failed to move ${srcvol} TO ${srcdir}${logvol} for ${rslist[rscount]} on node ${LOCALNODENAME}"
                                 exitcode=1
                           fi
	                 fi

		             if [[ -r ${srcrvol} ]]
 	                     then
 	   	                    if (( debugSFTKdtc != 0 ))
 		                       then
 		                         echo "${PROGNAME}: mv ${srcrvol} ${srcdir}r${logvol}"
 		                    fi
                            #Move source char device to current lvol char name
		                    mv ${srcrvol} ${srcdir}'r'${logvol}
                            if [[ "$?" -ne 0 ]]
                               then
	                              echo "${PROGNAME}: Failed to move ${srcrvol} TO ${srcdir}r${logvol} for ${rslist[rscount]} on node ${LOCALNODENAME}"
                                  exitcode=1
                               fi
	                 fi
                fi
            fi  
             ;;
            esac
           done
          #End of Symbolic link moves
     fi
  fi
  rscount=rscount+1

done
return 1
}
##############################################################################
#
# Start of main
#
###############################################################################

echo "${PROGNAME}: `date +%T` Starting. Params: ${vgargs}"

  #First source flag file to pick up user changes to variables
  if [[ -r  ${dtcflag_file} ]]
   then
     #Pick up user variable changes by sourcing file
     . ${dtcflag_file}
  fi
#Force symbolic links since static to dtc device not implemented
usingSymbolicSFTKdtc=1 

 if [[ ${usingSFTKdtc} -eq 0 ]]
    then
    #Exit script using TDMF is disabled
    echo "${PROGNAME}: TDMF has been disabled flag usingSFTKdtc = ${usingSFTKdtc} "
    exit 0
 fi      
exitcode=0 
export PATH="$(${ClustPath}/utilities/cl_get_path all)"
if [[ "$VERBOSE_LOGGING" == "high" ]] ; then
    set -x
fi
HA_DIR="$(cl_get_path)"
# Check for our Resource Group file and populate variables
  if [[ -r  ${dtcrsc_file} ]]
   then
        rscount=0
        cat ${dtcrsc_file} | while read rscgrp lglist[rscount] other
        do
          if [ "$rscgrp" = "" ] || [ "${lglist[rscount]}" = "" ] || [ -z "$rscgrp" ] || [ -z "${lglist[rscount]}" ]
            then
              echo "${PROGNAME}: Error found in ${dtcrsc_file}, for Group=$rscgrp LG=${lglist[rscount]}"
              exit 1
          fi
          if [ -z "${dtc_rscgrps}" ]
                   then
                        dtc_rscgrps="${rscgrp}"
                        if (( debugSFTKdtc != 0 ))
                           then
                                echo "${PROGNAME}: DEBUG - rscount=${rscount} Group=${rscgrp} LG#=${lglist[rscount]}"
                        fi
                        rscount=rscount+1
                   else
                        dtc_rscgrps="${dtc_rscgrps} ${rscgrp}"
                        if (( debugSFTKdtc != 0 ))
                           then
                                echo "${PROGNAME}: DEBUG - rscount=${rscount} Group=${rscgrp} LG#=${lglist[rscount]}"
                        fi
                        rscount=rscount+1
          fi 
        done
        #If the dtc group file is not populated then exit
          if [ "$dtc_rscgrps" = " " ] || [ -z "$dtc_rscgrps" ] || [ "$dtc_rscgrps" = "" ]
            then
              echo "${PROGNAME}: No resource groups found in ${dtcrsc_file}"
              exit 1
          fi
   else
     echo "${PROGNAME}: file ${dtcrsc_file} is missing or unreadable"
     exit 1
  fi   
# Get an array list of Volume Groups for All the Active Resource groups we migrating
rscount=0
dtccount=0
for rscgrp in ${dtc_rscgrps}
 do
      # Populate Groups Arrays for Selected Resource Group, Volume Groups and Logical Volumes
      rslist[rscount]=${rscgrp}
      vglist[rscount]=`_SPOC_FORCE=Y cl_lsvg | grep -v "#Resource Group" | grep -w ${rscgrp} | awk '{ print $2 }'`
      lgnumlist[rscount]=${lglist[dtccount]}
      rscount=rscount+1
    #Increment counter to keep logical group num in sync with resource group name
      dtccount=dtccount+1
 done
   
#Deactivate the dtc group(s) associated with the volume group(s) passed in arguments
for volgrp in ${vgargs}
 do
  deactivate_dtc_volumes
 done 
#   Wait to sync all the processes
wait

echo "${PROGNAME}: `date +%T` Done ${exitcode}."

exit ${exitcode}
