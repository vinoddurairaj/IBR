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
#  /%OPTDIR%/%PKGNM%/bin/genlparid
#
########################################################################

CONFIG_FILE='/etc/dtc/lib/%Q%lparid.cfg'

function usage
{
	echo "Usage:"
	echo "%Q%genlparid [-o | -h ]"
    echo "  -o     Overwrites any existing config file."
    echo "  -h     Prints this help message."
}

while getopts "oh" option
do
    case $option in
	o) OVERWRITE_EXISTING=1
    ;;
	*) usage; exit 1
    ;;
    esac
done

if [ -z "$OVERWRITE_EXISTING" -a -f $CONFIG_FILE ]
then
    echo "$CONFIG_FILE already exists, leaving it alone."
    echo
    exit 0
fi

LPARINFO=`uname -L`

echo $LPARINFO | grep " NULL$" 2>&1 > /dev/null

if [ $? -eq 0 ]
then
    echo "LPAR isn't set up, not creating the $CONFIG_FILE file."
else
    LPARID=`echo $LPARINFO | sed  's/\([0-9]*\) .*/\1/'`
    echo "Setting up LPARID of $LPARID into $CONFIG_FILE."
    echo
    echo $LPARID > $CONFIG_FILE
fi

