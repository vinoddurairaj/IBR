#!/bin/sh
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
# This script starts PMDs (Primary Mirror Daemons) to start transferring
# data to the secondary system. 
#
# This script can be customized to include optional %Q%pmd command line
# arguments:
#
# %Q%pmd usage:
#
#  %Q%pmd [-continue] [-a] [-g<Logical_Group_Num>] 
#
# Do a sanity check to see if we have a license file.  This is
# a common, easily detected error, so we do that here.
#
LANG=C
export LANG
# -- user check 
if [ `/bin/uname` = "HP-UX" ]; then
    COMMAND_USR=`/usr/bin/whoami`
elif [ `/bin/uname` = "SunOS" ]; then
    if [ -x /usr/ucb/whoami ]; then COMMAND_USR=`/usr/ucb/whoami`; else COMMAND_USR=`/usr/bin/whoami`; fi
elif [ `/bin/uname` = "AIX" ]; then
    COMMAND_USR=`/bin/whoami`
elif [ `/bin/uname` = "Linux" ]; then
    COMMAND_USR=`/usr/bin/whoami`
fi
if [ "$COMMAND_USR" != "root" ]; then
    echo "You must be root to run this process...aborted"
    exit 1
fi

RESULT=`/%OPTDIR%/%PKGNM%/bin/%Q%licinfo`
case $? in
0)     /%OPTDIR%/%PKGNM%/bin/%Q%pmd "${@:--a}" 
;;
1)
       echo $RESULT
       echo "Please correct this %PRODUCTNAME% license problem and try again."
       exit 1
;;
esac

