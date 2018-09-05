#!/bin/sh
# %PKGNM% Startup Script
#
# chkconfig: 2345 25 90
# description: %PKGNM% Startup Script
#
# LICENSED MATERIALS / PROPERTY OF IBM
# %PRODUCTNAME% version %VERSION%
# (c) Copyright %COMPANYNAME%  %COPYRIGHTYEAR 2001%.  All Rights Reserved.
# The source code for this program is not published or otherwise
# divested of its trade secrets, irrespective of what has been
# deposited with the U.S. Copyright Office.
# US Government Users Restricted Rights - Use, duplication or
# disclosure restricted by GSA ADP Schedule Contract with %COMPANYNAME%
#
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
    echo "Start %COMPANYNAME2% %PRODUCTNAME% daemons"
    ;;
'stop_msg')
    echo "Stop %COMPANYNAME2% %PRODUCTNAME% daemons"
    ;;
'start')		
    /%ETCINITDDIR%/%PKGNM%-loadstartstop load
    /%ETCINITDDIR%/%PKGNM%-loadstartstop start
    ;;
'stop')
    /%ETCINITDDIR%/%PKGNM%-loadstartstop stop
    ;;
*)
    echo "Usage: $0 { start | stop }"
    exit 1
    ;;
esac

exit 0
