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
LANG=C
export LANG
IsRHEL7=0
# -- user check 
if [ `/bin/uname` = "HP-UX" ]; then
    COMMAND_USR=`/usr/bin/whoami`
elif [ `/bin/uname` = "SunOS" ]; then
    if [ -x /usr/ucb/whoami ]; then COMMAND_USR=`/usr/ucb/whoami`; else COMMAND_USR=`/usr/bin/whoami`; fi
elif [ `/bin/uname` = "AIX" ]; then
    COMMAND_USR=`/bin/whoami`
elif [ `/bin/uname` = "Linux" ]; then
    COMMAND_USR=`/usr/bin/whoami`
    # Set the flag telling if this is RHEL 7
    if /bin/fgrep -q 'release 7' /etc/redhat-release 2> /dev/null
    then
        IsRHEL7=1
    fi
fi
if [ "$COMMAND_USR" != "root" ]; then
    echo "You must be root to run this process...aborted"
    exit 1
fi

# -- first kill and pmds that are running
/%OPTDIR%/%PKGNM%/bin/%Q%killpmd >/dev/null 2>&1    

# -- now kill any instances of in.%Q%
FTDPIDS=`/bin/ps -e | /bin/grep "in\.%Q%" | /bin/grep -v "grep" | /bin/awk '{print $1}'`
if [ "$FTDPIDS" != "" ]; then
    /bin/kill -15 $FTDPIDS >/dev/null 2>&1  

    # -- if that failed, try harder
    if [ $? != 0 ]; then
        /bin/kill -9 $FTDPIDS >/dev/null 2>&1 
    fi
fi

# -- now kill any instances of throtd
THROTDPIDS=`/bin/ps -e | /bin/grep "throtd" | /bin/grep -v "grep" | /bin/awk '{print $1}'`
if [ "$THROTDPIDS" != "" ]; then
    /bin/kill -15 $THROTDPIDS >/dev/null 2>&1
    # -- if that failed, try harder
    if [ $? != 0 ]; then
        /bin/kill -9 $THROTDPIDS >/dev/null 2>&1
    fi
fi
# If RHEL7, run the startdaemons script and then launch the master daemon and the PMDs because
# startdaemons does not do it on RHEL7
if [ $IsRHEL7 -eq 1 ]
then
    /%ETCINITDDIR%/%PKGNM%-startdaemons start
    cd /var/run/%PKGNM%
    /usr/bin/nohup /%OPTDIR%/%PKGNM%/bin/in.%Q% ${LOGOPT} >/tmp/in.%Q%.log 2>&1 &
    /bin/sleep 5
    /usr/bin/nohup /%OPTDIR%/%PKGNM%/bin/launchpmds >/tmp/in.pmd.log 2>&1 &
else
    /%ETCINITDDIR%/%PKGNM%-startdaemons start;
fi
