#!/bin/sh
#
# Copyright (c) 2001 Fujitsu Software Technology Corporation. All rights reserved.
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

