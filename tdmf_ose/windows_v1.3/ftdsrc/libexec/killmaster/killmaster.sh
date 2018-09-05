#!/bin/sh
#
# Copyright (c) 1997, 1998 FullTime Software Inc.  All Rights Reserved
#
# This script kills any instances of in.%Q% and throtd that may be 
# running.  
#
#
#

# -- now kill any instances of in.%Q%
FTDPIDS=`/bin/ps -e | /bin/grep "in\.%Q%" | /bin/grep -v "grep" | /bin/awk '{print $1}'`
if [ "$FTDPIDS" != "" ]; then
    /bin/kill -15 $FTDPIDS >/dev/null 2>&1  
    
    # -- give it a chance to die
    sleep 1

    # -- if that failed, try harder 
    FTDPIDS=`/bin/ps -e | /bin/grep "in\.%Q%" | /bin/grep -v "grep" | /bin/awk '{print $1}'`

    if [ "$FTDPIDS" != "" ]; then
        /bin/kill -9 $FTDPIDS >/dev/null 2>&1 
    fi
    echo "in.%Q% master FullTime Data daemon has been shutdown"
else
    echo "in.%Q% master FullTime Data daemon is not running"
fi

# -- now kill any instances of throtd
THROTDPIDS=`/bin/ps -e | /bin/grep "throtd" | /bin/grep -v "grep" | /bin/awk '{print $1}'`
if [ "$THROTDPIDS" != "" ]; then
    /bin/kill -15 $THROTDPIDS >/dev/null 2>&1
    # -- give it a chance to die
    sleep 1
    
    # -- if that failed, try harder
    THROTDPIDS=`/bin/ps -e | /bin/grep "throtd" | /bin/grep -v "grep" | /bin/awk '{print $1}'`
    if [ "$THROTDPIDS" != "" ]; then
        /bin/kill -9 $THROTDPIDS >/dev/null 2>&1
    fi
    echo "throtd FullTime Data throttle daemon has been shutdown"
else
    echo "throtd FullTime Data throttle daemon is not running"
fi
# -- kill rmds and pmds that are running
/%OPTDIR%/%PKGNM%/bin/killpmds >/dev/null 2>&1    
/%OPTDIR%/%PKGNM%/bin/killrmds >/dev/null 2>&1    
