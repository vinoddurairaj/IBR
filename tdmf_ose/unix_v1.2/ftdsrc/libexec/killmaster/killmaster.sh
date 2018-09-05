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
# This script kills any instances of in.%Q% and throtd that may be 
# running.  
#
#
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

# -- now kill any instances of in.%Q%
FTDPIDS=`/bin/ps -e | /bin/grep "in\.%Q%" | /bin/grep -v "grep" | /bin/awk '{print $1}'`
if [ "$FTDPIDS" != "" ]; then
    /bin/kill -15 $FTDPIDS >/dev/null 2>&1  
    
    # -- give it a chance to die
    /bin/sleep 1

    # -- if that failed, try harder 
    FTDPIDS=`/bin/ps -e | /bin/grep "in\.%Q%" | /bin/grep -v "grep" | /bin/awk '{print $1}'`

    if [ "$FTDPIDS" != "" ]; then
        /bin/kill -9 $FTDPIDS >/dev/null 2>&1 
    fi
    echo "in.%Q% master %PRODUCTNAME% daemon has been shutdown"
else
    echo "in.%Q% master %PRODUCTNAME% daemon is not running"
fi

# -- now kill any instances of throtd
THROTDPIDS=`/bin/ps -e | /bin/grep "throtd" | /bin/grep -v "grep" | /bin/awk '{print $1}'`
if [ "$THROTDPIDS" != "" ]; then
    /bin/kill -15 $THROTDPIDS >/dev/null 2>&1
    # -- give it a chance to die
    /bin/sleep 1
    
    # -- if that failed, try harder
    THROTDPIDS=`/bin/ps -e | /bin/grep "throtd" | /bin/grep -v "grep" | /bin/awk '{print $1}'`
    if [ "$THROTDPIDS" != "" ]; then
        /bin/kill -9 $THROTDPIDS >/dev/null 2>&1
    fi
    echo "throtd %PRODUCTNAME% throttle daemon has been shutdown"
else
    echo "throtd %PRODUCTNAME% throttle daemon is not running"
fi
# -- kill rmds and pmds that are running
/%OPTDIR%/%PKGNM%/bin/killpmds >/dev/null 2>&1    
# WR43473: commenting out killrmds to prevent mixed behavior on PMDs at RMD clean shutdown
# (some receiving ACKKILLPMD (not relaunching) and some declaring NETBROKE (relaunching))
# /%OPTDIR%/%PKGNM%/bin/killrmds >/dev/null 2>&1    
