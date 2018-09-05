#!/bin/sh
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
# This script kills %PRODUCTNAME% Agent daemons.
#

LANG=C
export LANG

# -- user check
if [ `/bin/uname` = "HP-UX" ]; then
	COMMAND_USR=`/usr/bin/whoami`
elif [ `/bin/uname` = "SunOS" ]; then
	if [ -x /usr/ucb/whoami ]; then COMMAND_USR=`/usr/ucb/whoami`; else COMMAND_USR=`/usr/bin/whoami`; fi
elif [ `/bin/uname` = "AIX" ]; then
	COMMAND_USR=`/bin/whoami`
elif [ `/bin/uname` = "Linux" ]; then
	COMMAND_USR=`/usr/bin/whoami`
fi
if [ "$COMMAND_USR" != "root" ]; then
	echo "You must be root to run this process...aborted"
	exit 1
fi

echo "Stop %COMPANYNAME2% %PRODUCTNAME% Agent daemons"

# -- now kill any instances of in.%QAGN%
FTDPIDS=`/bin/ps -e | /bin/grep "in\.%QAGN%" | /bin/grep -v "grep" | /bin/awk '{print $1}'`
if [ "$FTDPIDS" != "" ]; then
    /bin/kill -15 $FTDPIDS >/dev/null 2>&1  
    
    # -- give it a chance to die
    sleep 1

    # -- if that failed, try harder 
	FTDPIDS=`/bin/ps -e | /bin/grep "in\.%QAGN%" | /bin/grep -v "grep" | /bin/awk '{print $1}'`

    if [ "$FTDPIDS" != "" ]; then
        /bin/kill -9 $FTDPIDS >/dev/null 2>&1 
    fi
    rm /var/opt/%PKGNM%/Agn_tmp/* >/dev/null 2>&1
    echo "%COMPANYNAME2% %PRODUCTNAME% Agent has been shutdown"
else
    echo "%COMPANYNAME2% %PRODUCTNAME% Agent is not running"
fi
