#!/bin/sh
#
# /%OPTDIR%/%PKGNM%/bin/%Q%launchrefresh
#
# Copyright (c) 1999 Legato Systems, Inc. All Rights Reserved
#
# This script puts the PMDs and driver into refresh mode via the
# %Q%launchrefresh command.
#
# %Q%launchrefresh usage: 
#
#  One of the following three options is mandatory:
#    -g<Logical_Group_Num>    Put a Logical Group into refresh mode
#    -a                       Put all Logical Groups into refresh mode
#    -f                       force a dumb (complete) refresh
#    -h                       Print This Listing
#
# Do a sanity check to see if we have a license file.  This is
# a common, easily detected error, so we do that here.
#
RESULT=`/%OPTDIR%/%PKGNM%/bin/%Q%licinfo`
case $? in
0)     /%OPTDIR%/%PKGNM%/bin/%Q%launchrefresh "${@:--a}" &
;;
1)
       echo $RESULT
       echo "Please correct this FullTime Data license problem and try again."
       exit 1
;;
esac
