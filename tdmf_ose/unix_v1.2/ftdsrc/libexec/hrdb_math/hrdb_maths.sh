#! /bin/sh
########################################################################
#
#  %Q%hrdb_maths
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
#
########################################################################

##### FUNCTION TO SHOW THE COMMAND USAGE ####
show_usage() {

    echo "-----------------------------------------------------------------------------------"
    echo "USAGE:"
    echo "  dtchrdb_maths <size value> <size unit type> [<device expansion provision factor>]"
    echo "  this script takes as input a device size and calculates the HRDB size"
    echo "  for all level of tracking resolutions."
    echo "  You can also provide a device expansion provision factor for future"
    echo "  device expansion if applicable (optional)."
    echo "  Example: dtchrdb_maths 4 GB 5"
    echo "  Units can be specified as KB, MB, GB or TB (upper or lower case)."
    echo "-----------------------------------------------------------------------------------"
}

##### FUNCTION TO VERIFY THAT A TRACKING RESOLUTION IS A POWER OF 2 ####
# Call format example for low resolution: 
# check_resolution_power_of_2 $LOW_RESOLUTION
# It does not change the resolution value but issues a warning message
# if applicable 

check_resolution_power_of_2() {

        ResKBsPerBit=$1
		CurrentPowerOf2=2
        while [ $CurrentPowerOf2 -lt 2147483648 ] # == 0x80000000
        do
            if [ $ResKBsPerBit -eq $CurrentPowerOf2 ]
			then
			    break
			else
			    if [ $ResKBsPerBit -lt $CurrentPowerOf2 ]
			    then
					echo "  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
					echo "  <<< WARNING: the Tracking Resolution value $ResKBsPerBit KB is not a power of 2 >>>"
					echo "  Proceeding with the calculations but they will not match the actual results used by the product."
					echo "  Please fix the Tracking Resolutions configuration file."
					echo "  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
					break
				fi
			fi
			CurrentPowerOf2=`expr $CurrentPowerOf2 \* 2`
		done
}


#################### FUNCTION TO CALCULATE HRDB size ###################
# Call format example for low resolution: 
# calculate_HRDB_size $LOW_RESOLUTION $MAX_HRDB_SIZE_LOW 
#                     <Device size in KBs including expansion provision> <Resolution string>

calculate_HRDB_size() {

    ResKBsPerBit=$1
    MaxHRDBsizeKBs=$2
	DevSizeKBs=$3
    echo "************************ HRDB calculations for $4 tracking resolution ********************************"

    MaxHRDBsizeBits=`expr $MaxHRDBsizeKBs \* 8192`

    echo "Desired tracking resolution = $ResKBsPerBit KBs per HRDB bit"

    # Calculate the required HRDB size in various units
    # Avoid truncation of low order bits in integer division
    SafeDevSizeKBs=`expr $DevSizeKBs + $ResKBsPerBit - 1`
    HRDBsizeBits=`expr $SafeDevSizeKBs / $ResKBsPerBit`

    #Verify that we are not exceeding max HRDB size for this resolution level
    if [ $HRDBsizeBits -gt $MaxHRDBsizeBits ]
    then
        echo "  Maximum HRDB size exceeded for this resolution level. Setting HRDB size to maximum: $MaxHRDBsizeKBs KBs"
        HRDBsizeBits=$MaxHRDBsizeBits
        SafeDevSizeKBs=`expr $DevSizeKBs + $HRDBsizeBits - 1` # Avoid integer division truncation
        ResKBsPerBit=`expr $SafeDevSizeKBs / $HRDBsizeBits`
    	echo "  Calculated HRDB tracking resolution: $ResKBsPerBit KBs per bit"
		# Verify that the tracking resolution is a power of 2; if not, round up to next power of 2
		CurrentPowerOf2=2
        while [ $CurrentPowerOf2 -lt 2147483648 ] # == 0x80000000
        do
            if [ $ResKBsPerBit -eq $CurrentPowerOf2 ]
			then
				echo "  This tracking resolution becomes effective."
			    break
			else
			    if [ $ResKBsPerBit -lt $CurrentPowerOf2 ]
			    then
				    ResKBsPerBit=$CurrentPowerOf2
					echo "  EFFECTIVE Tracking resolution rounded up to closest power of 2 (driver requirement): $ResKBsPerBit KBs per bit"
					echo "  Recalculating the HRDB size needed for this adjusted resolution..."
                    SafeDevSizeKBs=`expr $DevSizeKBs + $ResKBsPerBit - 1`
                    HRDBsizeBits=`expr $SafeDevSizeKBs / $ResKBsPerBit`
					break
				fi
			fi
			CurrentPowerOf2=`expr $CurrentPowerOf2 \* 2`
		done
    fi

    SafeHRDBsizeBits=`expr $HRDBsizeBits + 7`      # Avoid integer division truncation
    HRDBsizeBytes=`expr $SafeHRDBsizeBits / 8`

    SafeHRDBsizeBytes=`expr $HRDBsizeBytes + 3`    # Avoid integer division truncation
    HRDBsize32bitWords=`expr $SafeHRDBsizeBytes / 4`

    SafeHRDBsizeBytes=`expr $HRDBsizeBytes + 1023` # Avoid integer division truncation
    HRDBsizeKBs=`expr $SafeHRDBsizeBytes / 1024`

    echo "      HRDB size:"
    echo "           $HRDBsizeBits bits"
    echo "           $HRDBsizeBytes bytes"
    echo "           $HRDBsize32bitWords 32-bit words"
    echo "           $HRDBsizeKBs KBs"
    echo " "
}

################################ MAIN SCRIPT BODY ########################################

# Check if not enough arguments have been provided to the script.
# If so, print usage paragraph.
if [ $# -lt 2 ]
then
    show_usage
    exit 0
fi

# Check that the first argument is a valid number
if echo $1 | grep "[^0-9"] 2>&1 > /dev/null
then
  echo "ERROR: the first argument is not a valid number."
  show_usage
  exit 1
fi

# Check if a device expansion provision factor has been specified
if [ $# -eq 3 ]
then
    if echo $3 | grep "[^0-9"] 2>&1 > /dev/null
    then
      echo "ERROR: the third argument (device expansion provision factor) is not a valid number."
      show_usage
      exit 1
    fi
    ExpansionFactor=$3
else
    ExpansionFactor=3
fi

# Source the tracking resolution config file if it exists
OS=`uname -s`
if [ $OS = "AIX" ]; then
        TRACKING_RES_FILE_LOC=/etc/dtc/lib/
else
        TRACKING_RES_FILE_LOC=/etc/opt/SFTKdtc/
fi

echo "****************************************************************************************************************"
if ls ${TRACKING_RES_FILE_LOC}dtcTracking_Resolutions.txt 2>&1 >/dev/null
then
  PROMPT=0
  . ${TRACKING_RES_FILE_LOC}dtcTracking_Resolutions.txt
  echo "HRDB parameters read from tracking resolution configuration file ${TRACKING_RES_FILE_LOC}dtcTracking_Resolutions.txt:"
  check_resolution_power_of_2 $LOW_RESOLUTION
  check_resolution_power_of_2 $MEDIUM_RESOLUTION
  check_resolution_power_of_2 $HIGH_RESOLUTION
else
    PROMPT=1
    echo "Could not source the tracking resolution config file: ${TRACKING_RES_FILE_LOC}dtcTracking_Resolutions.txt"
    echo "Will use the following default tracking resolution values:"
	# WARNING: if you modify the resolution values, they must be a power of 2
    LOW_RESOLUTION=64
    MAX_HRDB_SIZE_LOW=4096
    MEDIUM_RESOLUTION=32
    MAX_HRDB_SIZE_MEDIUM=8192
    HIGH_RESOLUTION=8
    MAX_HRDB_SIZE_HIGH=16384
fi

echo "    LOW_RESOLUTION = $LOW_RESOLUTION KBs per bit"
echo "    MEDIUM_RESOLUTION = $MEDIUM_RESOLUTION KBs per bit"
echo "    HIGH_RESOLUTION = $HIGH_RESOLUTION KBs per bit"
echo "    MAX_HRDB_SIZE_LOW = $MAX_HRDB_SIZE_LOW KBs"
echo "    MAX_HRDB_SIZE_MEDIUM = $MAX_HRDB_SIZE_MEDIUM KBs"
echo "    MAX_HRDB_SIZE_HIGH = $MAX_HRDB_SIZE_HIGH KBs"

if [ $PROMPT -ne 0 ]
then
    echo "Please type enter to continue: "
    read any_key
fi

DevSize=$1
DevSizeUnitString=`echo $2 | tr '[:lower:]' '[:upper:]'`

case $DevSizeUnitString in	
     "KB")
		DevSizeUnit=1
		;;
     "MB")
		DevSizeUnit=2
		;;
     "GB")
		DevSizeUnit=3
		;;
     "TB")
		DevSizeUnit=4
		;;
	  *)
        echo "ERROR: invalid device size unit (must be KB, MB, GB or TB)"
        show_usage
        exit 1
		;;
esac

echo " "
echo "*************************** DEVICE INFORMATION ************************************"

echo "Applying device size expansion provision factor of $ExpansionFactor to allow for later device expansion (dtcexpand)"
ExpansionDevSize=`expr $DevSize \* $ExpansionFactor`
echo "Device size used in HRDB calculations (after applying expansion provision factor): $ExpansionDevSize $DevSizeUnitString"

# Calculate multiplication factor to convert device size to KBs
i=1
KBmultFactor=1
while [ $i -lt $DevSizeUnit ]
do
    KBmultFactor=`expr $KBmultFactor \* 1024`
    i=`expr $i + 1`
done

ExpansionDevSizeInKBs=`expr $KBmultFactor \* $ExpansionDevSize`
echo "Device size in KBs (including expansion provision factor $ExpansionFactor) = $ExpansionDevSizeInKBs"

RealDevSizeKBs=`expr $ExpansionDevSizeInKBs / $ExpansionFactor`
RealDevNumSectors512=`expr $RealDevSizeKBs \* 2`

echo "Number of 512-byte sectors on device (real): $RealDevNumSectors512"

echo " "

###########################################################################################
# Calculate and display HRDB information for all tracking resolutions 

calculate_HRDB_size $LOW_RESOLUTION $MAX_HRDB_SIZE_LOW	$ExpansionDevSizeInKBs	"LOW"

calculate_HRDB_size $MEDIUM_RESOLUTION $MAX_HRDB_SIZE_MEDIUM $ExpansionDevSizeInKBs "MEDIUM"

calculate_HRDB_size $HIGH_RESOLUTION $MAX_HRDB_SIZE_HIGH $ExpansionDevSizeInKBs "HIGH"

