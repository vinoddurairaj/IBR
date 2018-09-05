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
# This script runs the utility "%Q%killbackfresh" to kill BFDs 
# (%PRODUCTNAME% Backfresh Daemons) currently running on this system.
#
# This script can be customized to include optional "%Q%killbackfresh" command
# line arguments:
#
# %Q%killbackfresh usage: 
#
#  One of the following two options is mandatory:
#    -g<Logical_Group_Num>  Kill %PRODUCTNAME% Backfresh Daemon for a Logical Group
#    -a                     Kill all %PRODUCTNAME% Backfresh Daemons
#  -h                       Print This Listing
#
#
LANG=C
export LANG

echo "backfresh operation deactivated as of TDMF IP 2.8.0"
exit 1


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

/%OPTDIR%/%PKGNM%/bin/%Q%killbackfresh "${@:--a}" 

