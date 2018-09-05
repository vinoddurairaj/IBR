#!/bin/sh
#
# Copyright (c) 2001 Fujitsu Software Technology Corporation. All rights reserved.
#
# This script runs the "%Q%killpmd" utility to kill PMDs (Primary Mirror
# Daemons) currently running on this system. 
#
# This script can be customized to include optional "%Q%killpmd" command
# line arguments:
#
# %Q%killpmd usage: 
#
#  One of the following two options is mandatory:
#    -g<Logical_Group_Num>  Kill Primary Mirror Daemon for a logical group
#    -a                     Kill all Primary Mirror Daemons
#  -h                       Print This Listing
#
PIDS=`/bin/ps -e | /bin/grep in.pmd | /bin/awk '{print $1}'`
if [ "$PIDS" = "" ] ; then
        echo "No %PRODUCTNAME% PMD daemons were running."
else
        /%OPTDIR%/%PKGNM%/bin/%Q%killpmd "${@:--a}" 
fi
    

