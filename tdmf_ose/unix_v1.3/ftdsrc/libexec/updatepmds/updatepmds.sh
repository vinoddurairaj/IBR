#!/bin/sh
#
# /%OPTDIR%/%PKGNM%/bin/%Q%updatepmd
#
# Copyright (c) 2001 Fujitsu Software Technology Corporation. All rights reserved.
# All rights reserved
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

