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
#
###############################################################################
#  
#  Name:  cl_activate_dtc
#
#  Returns:
#       0 - All of the dtc devices are successfully varied on
#
#  This function will activate the the dtc devices for volume groups passed in as arguments.
#
#  Arguments:   volume group list
#              
#  Environment: VERBOSE_LOGGING, PATH
#
#
###############################################################################
PROGNAME=${0##*/}
typeset vgargs="$*"
typeset -i vgargs_num=$#
#env > /tmp/cl_dtc_act_env
ClustPath="/usr/es/sbin/cluster"
##### Target lvol name changes defined #####
typeset tgtprefix=""
typeset tgtsuffix=""
##### DTC volume name changes defined ######
typeset dtcprefix="lg"
typeset dtcsuffix="dtc"
typeset dtcsep="-"
##### For symbolic links source volume name changes defined ####
typeset srcprefix=""
typeset srcsuffix="_OLD"
###########################################
typeset -i usingSFTKdtc=1
typeset -i debugSFTKdtc=0
typeset -i createTargetsSFTKdtc=1
typeset -i usingSymbolicSFTKdtc=1
# The default value of 2:30 minutes comes from the analysis of the WR PROD00008316,
# where bringing the cluster up took slightly over 2 minutes.
typeset -i launchPMDsDelay=150
typeset dtcvolname="" dtcvol="" dtcrvol="" srcvol="" tgtvol="" volgrp="" logvol="" rscgrp=""  clustrscs="" lsvglvols=""
typeset srcrvol=""  cfgfile="" cfgOrig="" cfgClus=""
typeset dynamic_activation=""
typeset -i DA_on=0
set -A  rslist vglist lvlist lglist lgnumlist
typeset dtcdir="/dev/"
typeset srcdir="/dev/"
typeset tgtdir="/dev/"
typeset dtcrsc_file="/etc/dtc/hacmp/cl_dtc_rsgrps"
typeset dtcflag_file="/etc/dtc/hacmp/cl_dtc_flags"
typeset dtclib="/etc/dtc/lib/"
typeset dtc_rscgrps="" other="" fieldname="" dtcstartgrps="" dtcgroup=""
typeset -i rscount exitcode dtccount

TMP_FILE="/tmp/cl_activate_dtc.tmp"

#   Remove the status file if already exists
rm -f ${TMP_FILE} 

###############################################################################
move_logvol_src_dtc()
{
  if [[ ${usingSymbolicSFTKdtc} -eq 0 ]]
    then
    #Exit function using symbolic links is disabled
    if (( debugSFTKdtc != 0 ))
      then
       echo "${PROGNAME}: DTC will not be moving source files because flag usingSymbolicSFTKdtc = ${usingSymbolicSFTKdtc} "
    fi
    return 0
  fi   

#Move the logical Volume to Source device names with prefix and suffix to allow symbolic links to be created.
exitcode=0
 if [[ ! -r ${srcvol} ]]  && [[ -r ${srcdir}${logvol} ]]
  then
    if (( debugSFTKdtc != 0 ))
      then
      echo "mv ${srcdir}${logvol} ${srcvol}"
    fi
    mv ${srcdir}${logvol} ${srcvol}
    if [[ "$?" -ne 0 ]]
      then
	echo "${PROGNAME}: mv ${srcdir}${logvol} ${srcvol} Failed for ${rslist[rscount]} on node ${LOCALNODENAME}"
        exitcode=1
    fi
 fi

 if [[ ! -r ${srcrvol} ]]  && [[ -r ${srcdir}r${logvol} ]]
  then
    if (( debugSFTKdtc != 0 ))
      then
      echo "mv ${srcdir}r${logvol} ${srcrvol}"
    fi
    mv ${srcdir}r${logvol} ${srcrvol}
    if [[ "$?" -ne 0 ]]
      then
	echo "${PROGNAME}: mv ${srcdir}r${logvol} ${srcrvol} Failed for ${rslist[rscount]} on node ${LOCALNODENAME}"
        exitcode=1
    fi
 fi      


return ${exitcode}
}

create_symbolic_links_dtc()
{
  if [[ ${usingSymbolicSFTKdtc} -eq 0 ]]
    then
    #Exit function using symbolic links is disabled
    if (( debugSFTKdtc != 0 ))
      then
       echo "${PROGNAME}: DTC will not be creating symbolic links because flag usingSymbolicSFTKdtc = ${usingSymbolicSFTKdtc} "
    fi
    return 0
  fi   
#Create Symbolic links for the real logical volume names that point to the dtc device names
exitcode=0
 if [[ ! -r ${srcdir}${logvol} ]] 
  then
    if (( debugSFTKdtc != 0 ))
      then
  	echo "${PROGNAME}: ln -n -s ${dtcvol} ${srcdir}${logvol}"
    fi
    ln -n -s ${dtcvol} ${srcdir}${logvol}
    if [[ "$?" -ne 0 ]]
      then
	echo "${PROGNAME}: Failed creating ${srcdir}${logvol} BLOCK symbolic link for ${rslist[rscount]} on node ${LOCALNODENAME}"
        exitcode=1
    fi
 fi
 if [[ ! -r ${srcdir}r${logvol} ]]
  then
    if (( debugSFTKdtc != 0 ))
      then
       echo "${PROGNAME}: ln -n -s ${dtcrvol} ${srcdir}r${logvol}"
    fi
    ln -n -s ${dtcrvol} ${srcdir}'r'${logvol}
    if [[ "$?" -ne 0 ]]
      then
	echo "${PROGNAME}: Failed creating ${srcdir}r${logvol} CHAR symbolic link for ${rslist[rscount]} on node ${LOCALNODENAME}"
        exitcode=1
    fi
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
#####################
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
######################
update_startgrps()
{
    if [ -z "${dtcstartgrps}" ]
       then
	      # save the logical group number to be started after all source volumes are in place
          dtcstartgrps="${lgnumlist[rscount]}"
		  if (( debugSFTKdtc != 0 ))
		    then
		    echo "${PROGNAME}: Updated Groups to be started = ${dtcstartgrps} "
		  fi  
       else
	      # save the logical group number to be started after all source volumes are in place
		  echo ${dtcstartgrps} | fgrep -w -s ${lgnumlist[rscount]}
	      if [[ $? -ne 0 ]]
	     	then
	          # add the logical group to the saved group list to be started at the end
	      	  dtcstartgrps="${dtcstartgrps} ${lgnumlist[rscount]}"
              if (( debugSFTKdtc != 0 ))
                 then
           	       echo "${PROGNAME}: Updated Groups to be started = ${dtcstartgrps} "
              fi 
	      fi
    fi
return 0
}
########################
activate_dtc_volumes()
{
echo "${PROGNAME}: activate_dtc_volumes is processing Volume Group: ${volgrp}"
rscount=0
exitcode=0

while (( rscount < ${#vglist[*]} ))
do
  #Find where volgrp is indexed in the vglist array
  echo ${vglist[rscount]} | fgrep -w -s ${volgrp}
  if [[ $? -eq 0 ]]
    then
     #  Get the list of lvols for the volume group we are processing
     lsvglvols=`lsvg -L -l ${volgrp} | awk ' NF > 4 && !/^LV NAME/ { print $1 }'`
     #If no Active logical volumes for this volume group then return to caller
          case ${lsvglvols} in
           *[a-zA-Z0-9]*) : ;;
           *)
             echo "${PROGNAME}: No active logical volumes for these volume group : ${volgrp} "
             return 1
             ;;
          esac       
     # Read the dtc config file for the current Resource Group
     read_cfg_file 

     # For each logical volume verify they are in the cluster then activate dtc volume
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
	            # Setup volume names for the dtc commands
	            if [[ ${usingSymbolicSFTKdtc} -eq 0 ]]
	             then
	               #If not using symbolic links then only use logvol name
	               srcvol=${srcdir}${logvol}
	               srcrvol=${srcdir}'r'${logvol}
	             else
	               #If using symbolic links then use prefix and suffix
	               srcvol=${srcdir}${srcprefix}${logvol}${srcsuffix}
	               srcrvol=${srcdir}'r'${srcprefix}${logvol}${srcsuffix}
	            fi
		        #
	            #Move logical volume to source volume (function checks flag before processing)
	      	    move_logvol_src_dtc
	            #Create symbolic links for logical volume moved (function checks flag before processing)
		        create_symbolic_links_dtc
            fi
            #Save the current logical Group number if not already stored
		    update_startgrps
             fi  
       ;;
       esac
      done
  fi
  rscount=rscount+1

done

return 1
}

###############################################################################
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
    #Exit script using DTC is disabled
    echo "${PROGNAME}: DTC has been disabled flag usingSFTKdtc = ${usingSFTKdtc} "
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
#Get list of active Cluster Resource Groups
clustrscs=`cllsgrp`
#If there are no resources in the cluster then exit
         if [[ -z "$clustrscs" ]]
           then
            echo "${PROGNAME}: cllsgrp did not return any resource groups"
            exit 1
         fi

# Get an array list of Volume Groups for All the Active Resource groups we migrating
rscount=0
dtccount=0
for rscgrp in ${dtc_rscgrps}
 do
   # Check if our selected resource group is active in the cluster
    echo ${clustrscs} | fgrep -w -s ${rscgrp}
    if [[ $? -eq 0 ]]
     then
      # Populate Groups Arrays for Selected Resource Group, Volume Groups and Logical Volumes
      rslist[rscount]=${rscgrp}
      vglist[rscount]=`_SPOC_FORCE=Y cl_lsvg | grep -v "#Resource Group" | grep -w ${rscgrp} | awk '{ print $2 }'`
      lgnumlist[rscount]=${lglist[dtccount]}
      rscount=rscount+1
    fi
    #Increment counter to keep logical group num in sync with resource group name
      dtccount=dtccount+1
 done


#Activate the dtc volumes based on the volume groups passed in arguments
for volgrp in ${vgargs}
 do
  activate_dtc_volumes
 done 

#start dtc groups based on the saved logical group numbers
for dtcgroup in ${dtcstartgrps}
 do
	cfgfile=${dtclib}p${dtcgroup}.cfg
	cfgOrig=${dtclib}p${dtcgroup}.cfg_orig
	cfgClus=${dtclib}p${dtcgroup}.cfg_cl

    # Get the dynamic activation state for the current group in order to enable or not DTC device  
    dynamic_activation=$(awk '/DYNAMIC-ACTIVATION/ {printf $2 }' $cfgfile)
    if [[ "$dynamic_activation" = "on" ]] || [[ "$dynamic_activation" = "ON" ]]; then   
        DA_on=1

        if (( debugSFTKdtc != 0 ))
           then
              echo "${PROGNAME}: /usr/dtc/bin/dtcmodhacmp -g :${dtcgroup}"
           fi 
          /usr/dtc/bin/dtcmodhacmp -g :${dtcgroup}
    else
	    if [[  ${cfgfile} -nt ${cfgClus} ]]
	       then
              if (( debugSFTKdtc != 0 ))
                 then
                    echo "${PROGNAME}: /usr/dtc/bin/dtcmodhacmp -s ${srcprefix}%${srcsuffix} -g :${dtcgroup}"
              fi 
		    /usr/dtc/bin/dtcmodhacmp -s ${srcprefix}%${srcsuffix} -g :${dtcgroup}
	    fi
    fi

	if [[ ${cfgfile} -ot ${cfgClus} ]]
	   then
       		if (( debugSFTKdtc != 0 ))
       	   	  then
             	     echo "${PROGNAME}: cp -p ${cfgClus} ${cfgfile}"
       		fi
		cp -p ${cfgClus} ${cfgfile}
	fi

	if (( debugSFTKdtc != 0 ))
	  then
	    echo "${PROGNAME}: /usr/sbin/dtcstart -g ${dtcgroup} "
	    echo "${PROGNAME}: Will call 'launchpmds -g ${dtcgroup}' in ${launchPMDsDelay} seconds."
	fi  
	/usr/sbin/dtcstart -g ${dtcgroup}
    # We are delaying the start of the launchpmds in order to avoid conflicts with fscks run while the cluster is activated
    # and which require an exclusive access to the devices.
    # C.F. WR PROD00008316
    (sleep ${launchPMDsDelay}; echo "${PROGNAME} : `date +%T` /usr/dtc/bin/launchpmds -g ${dtcgroup}"; /usr/dtc/bin/launchpmds -g ${dtcgroup}) &
 done 

# Before the changes for PROD00008316, we used to call wait here, but no reason for it could be found and we couldn't
# rely on it now that the launchpmds are delayed.

echo "${PROGNAME}: `date +%T` Done: ${exitcode}."

exit ${exitcode}
