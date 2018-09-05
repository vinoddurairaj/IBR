#!/bin/sh
#
# Copyright (c) 2001 Fujitsu Software Technology Corporation. All rights reserved.
#
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
if [ `/bin/uname` = "HP-UX" ]
then
   /sbin/init.d/%PKGNM%-startdaemons start;
elif [ `/bin/uname` = "SunOS" ]
then
    /etc/init.d/%PKGNM%-startdaemons start;
elif [ `/bin/uname` = "AIX" ]
then
    /%ETCINITDDIR%/%PKGNM%-startdaemons start;
fi

