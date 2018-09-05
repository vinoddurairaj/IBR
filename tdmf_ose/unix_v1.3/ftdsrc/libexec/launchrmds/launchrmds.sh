#!/bin/sh
#
# Copyright (c) 2001 Fujitsu Software Technology Corporation. All rights reserved.
#
# This script starts the %PRODUCTNAME% Remote Mirror Daemon to start receiving
# data on the secondary system.  By starting this daemon with "nohup",
# it will continue to execute until the system is shut down.
#
# This script can be customize to include optional in.rmd command line
# arguments:
#
# in.rmd usage:
#   in.rmd [-p portnum]
#
# Do a sanity check to see if we have a license file.  This is
# a common, easily detected error, so we do that here.
#
LF=/%OURCFGDIR%/%PKGNM%/%CAPQ%.lic
if [ ! -f $LF ]; then
  echo "License file $LF for %PRODUCTNAME% not found."
  echo "Please correct and try again."
  exit 1
fi
if /%OPTDIR%/%PKGNM%/bin/%Q%licinfo -q -r; then
  echo > /dev/null
else
  echo "Please correct the above problem with the %PRODUCTNAME% license"
  exit 1
fi
PIDS=`/bin/ps -e |/bin/grep in.rmd |/bin/awk '{print $1}'`
if [ "$PIDS" = "" ]; then
  /%OPTDIR%/%PKGNM%/bin/in.rmd "$@" < /dev/null &
else
  echo "%PRODUCTNAME% RMD daemons already running - run killrmds first."
fi

