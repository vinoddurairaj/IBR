#!/bin/sh
#
# Copyright (c) 2001 Fujitsu Software Technology Corporation. All rights reserved.
#
# This script runs the "%Q%killrefresh" utility to kill RFDs (Refresh
# Daemons) currently running on this system. 
#
# This script can be customized to include optional "%Q%killrefresh"
# command line arguments:
#
# %Q%killrefresh usage: 
#
#  One of the following two options is mandatory:
#    -g<Logical_Group_Num>  Kill Refresh Daemon for a logical group
#    -a                     Kill all Refresh Daemons
#  -h                       Print This Listing
#
/%OPTDIR%/%PKGNM%/bin/%Q%killrefresh "${@:--a}" 


