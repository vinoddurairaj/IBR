#!/bin/sh
#
# Copyright (c) 1999 Legato Systems, Inc. All Rights Reserved
#
# /%OPTDIR%/%PKGNM%/bin/launchbackfresh
#
# This script runs the utility "%Q%launchbackfresh" to start backfresh
# operations on this system. Backfresh will transfer all 
# data on mirror disk devices on a Secondary system to their 
# corresponding %Q% devices on the Primary system (employs smart
# backfresh technology).
#
# %Q%launchbackfresh usage: 
#
#  One of the following three options is mandatory:
#    -g<Logical_Group_Num>  Put all %Q% Devices in a Logical Group into 
#                           Backfresh mode
#    -a                     Put all %Q% Devices into Backfresh mode
#    -h                     Print This Listing
#
# Do a sanity check to see if we have a license file.  This is
# a common, easily detected error, so we do that here.
#
RESULT=`/%OPTDIR%/%PKGNM%/bin/%Q%licinfo`
case $? in
0)     /%OPTDIR%/%PKGNM%/bin/%Q%launchbackfresh "${@:--a}" &
;;
1)
       echo $RESULT
       echo "Please correct this FullTime Data license problem and try again."
       exit 1
;;
esac

