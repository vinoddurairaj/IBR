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
# This script runs the "%Q%killpmd" utility to kill PMDs (Primary Mirror
# Daemons) currently running on this system. 
#
# This script can be customized to include optional "%Q%killpmd" command
# line arguments:
#
# %Q%killpmd usage: 
#
#  One of the following two options is mandatory:
#    -g<Logical_Group_Num>  Kill Primary Mirror Daemon for a logical group
#    -a                     Kill all Primary Mirror Daemons
#  -h                       Print This Listing
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

# HP-UX 11.23 /bin/ps -e produces a different result.
# This particular version prints the process's argv0 value (PMD_??) by
# default (like -ef does) rather than the name of the binary that was
# given to exec when the master agent forked and executed the pmds (in.pmd).

# Because version 11.31 of HP-UX reverted the 'ps -e' output back to
# the one of 11.11, combined with the fact that it is not clear if the
# exact 'ps -e' output could be modified by some standards related
# environment variable (UNIX95, UNIX), we'll simply lookout for both
# possible values regardless of the os in use.
PMD_REGEXP='PMD_[0-9]+|in\.pmd'
PIDS=`/bin/ps -e | /bin/egrep $PMD_REGEXP | /bin/awk '{print $1}'`

if [ "$PIDS" = "" ] ; then
        echo "No %PRODUCTNAME% PMD daemons were running."
else
        /%OPTDIR%/%PKGNM%/bin/%Q%killpmd "${@:--a}" 
fi
    

