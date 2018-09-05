#!/bin/sh
#
# Copyright (c) 1999 Legato Systems, Inc. All Rights Reserved
#
# This script runs the utility "%Q%killbackfresh" to kill BFDs 
# (FullTime Data Backfresh Daemons) currently running on this system.
#
# This script can be customized to include optional "%Q%killbackfresh" command
# line arguments:
#
# %Q%killbackfresh usage: 
#
#  One of the following two options is mandatory:
#    -g<Logical_Group_Num>  Kill FullTime Data Backfresh Daemon for a Logical Group
#    -a                     Kill all FullTime Data Backfresh Daemons
#  -h                       Print This Listing
#
#
/%OPTDIR%/%PKGNM%/bin/%Q%killbackfresh "${@:--a}" 

