#!/bin/sh
#
# Copyright (c) 1999 Legato Systems, Inc. All Rights Reserved
#
# This script runs the "%Q%killrmd" utility to kill RMDs (Remote Mirror
# Daemons) currently running on this system. 
#
# This script can be customized to include optional "%Q%killrmd" command
# line arguments:
#
# %Q%killrmd usage: 
#
#  One of the following two options is mandatory:
#    -g<Logical_Group_Num>  Kill Remote Mirror Daemon for a logical group
#    -a                     Kill all Remote Mirror Daemons
#  -h                       Print This Listing
#
PIDS=`/bin/ps -e | /bin/grep in.rmd | /bin/awk '{print $1}'`
if [ "$PIDS" = "" ] ; then
        echo "No FullTime Data RMDs were running."
else
        /%OPTDIR%/%PKGNM%/bin/%Q%killrmd "${@:--a}" 
fi
    

