#!/bin/sh
#
# Copyright (c) 2001 Fujitsu Software Technology Corporation. All rights reserved.
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
/%OPTDIR%/%PKGNM%/bin/%Q%killbackfresh "${@:--a}" 

