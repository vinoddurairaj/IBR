#!/bin/sh
#
# /%OPTDIR%/%PKGNM%/bin/%Q%updatepmd
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
# This script runs the utility "%Q%updatepmd" to start/restart PMDs 
# (Primary Mirror Daemons) on this system. Specified PMDs that are 
# currently running are updated via a exit and restart. Specified PMDs 
# that are not currently running are started. 
#
# %Q%updatepmd usage: 
#
#  One of the following two options is mandatory:
#    -g<Logical_Group_Num>  Update Primary Mirror Daemon for a logical Group
#    -a                     Update all Primary Mirror Daemons 
#  -h                       Print this listing 
#
PIDS=`/bin/ps -ef | /bin/grep in.%Q% | /bin/awk '{print $1}'`
if [ "$PIDS" = "" ]; then
  echo "%PRODUCTNAME% Master daemon (in.%Q%) not running!"
else
  /%OPTDIR%/%PKGNM%/bin/%Q%updatepmd "${@:--a}" 
fi

