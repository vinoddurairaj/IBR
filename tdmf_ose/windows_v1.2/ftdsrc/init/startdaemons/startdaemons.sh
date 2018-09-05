#!/bin/sh
# %PKGNM% Startup Script
#
# Copyright (c) 1997, 1998, 1999 Legato Systems, Inc. All Rights Reserved
#
# This script will start the daemons for the %PRODUCTNAME%
# product.  The RMD performs two tasks:  its primary task is to move
# data from the network to a mirror disk.  Its other task is to provide
# useful information to the gui tools.
# 
# Generally, this should run in init mode 3 (in the /etc/rc3.d directory),
# but can be moved, if needed in a local installation, to other places after
# the network has started.  It must start after %PKGNM%-scan.
#
# exit values: 0 (OK), 1 (FAIL), and 2 (N/A)
#
case "$1" in
'start_msg')
    echo "Start %PRODUCTNAME% daemons"
    ;;
'stop_msg')
    echo "Stop %PRODUCTNAME% daemons"
    ;;
'start')		
    # Start the PMDs	
    echo "Starting %PRODUCTNAME% daemons"
    /usr/bin/nohup /%OPTDIR%/%PKGNM%/bin/in.%Q% >/tmp/in.%Q%.log 2>&1 &
    /bin/sleep 5
    /usr/bin/nohup /%OPTDIR%/%PKGNM%/bin/launchpmds >/tmp/in.pmd.log 2>&1 &
    ;;
'stop')
    echo "Stopping %PRODUCTNAME% daemons"
    /%OPTDIR%/%PKGNM%/bin/kill%Q%master
    ;;
*)
    echo "Usage: $0 { start | stop }"
    exit 1
    ;;
esac

exit 0
