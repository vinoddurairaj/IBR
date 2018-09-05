#!/bin/sh

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
#  /%OPTDIR%/%PKGNM%/bin/dtcupdateodm
#
########################################################################
#  $1 = dtc dev num
#  $2 = lg num
#  $3 = source dev name
TMPFILE=/tmp/odmout
function usage {

   echo "Usage: dtcupdateodm < add | delete > <dtc device name> <source device name>"
   echo
   echo "       Updates the odm database with the required information for"
   echo "       the dtc device."
   echo "       Returns 0 on success 1 on failure."
   exit 1
}

function already_present {
  odmget -q name=$1 CuAt 2>&1 | grep name >/dev/null
  return $?
}

function get_odm_add_input {

  SEDSTR="s/"$1"/"$2"/g"
  odmget -q name=$1 CuAt | grep name >/dev/null 2>&1
  if [ $? -eq 0 ]
  then
      odmget -q name=$1 CuAt  | sed -e $SEDSTR
  fi
  odmget -q name=$1 CuDv | grep name >/dev/null 2>&1
  if [ $? -eq 0  ]
  then
      odmget -q name=$1 CuDv | sed -e $SEDSTR
  fi
}

function add_dev_to_odm {
   get_odm_add_input $2 $1 | odmadd  >/dev/null 2>&1
   already_present $1
   return $?
}

function add_entries {
    DTCDEV=$1
    SRCDEV=$2

    if already_present $DTCDEV
    then
       exit 1;
    else
       add_dev_to_odm $DTCDEV $SRCDEV
       RET=$?
       exit $RET
    fi
}

function rm_entries {
    odmdelete -o CuAt -q name=$1 > /dev/null 2>&1 
    odmdelete -o CuDv -q name=$1 > /dev/null 2>&1
    if already_present $1
    then
       return 1
    else
       return 0
    fi
}

if [ $# -lt 2 ]
then
   usage
fi

if [ "$1" = "add" ] 
then
    add_entries $2 $3
else
    if [ "$1" = "delete" ]
    then
        rm_entries $2 
        return $?
    else
        usage
    fi
fi



