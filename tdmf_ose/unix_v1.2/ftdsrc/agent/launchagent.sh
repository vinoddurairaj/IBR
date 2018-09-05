#!/bin/sh
# launchagent - starts agent daemon  
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
# This script will start the daemons for the %PRODUCTNAME% Agent
# product.  
# 
# exit values: 0 (OK), 1 (FAIL), and 2 (N/A)
#
LANG=C
export LANG

PATH=/usr/bin:/usr/sbin:/sbin:
export PATH

cd /%FTDVAROPTDIR%

if [ `/bin/uname` = "HP-UX" ]; then
	COMMAND_USR=`/usr/bin/whoami`
elif [ `/bin/uname` = "SunOS" ]; then
	if [ -x /usr/ucb/whoami ]; then COMMAND_USR=`/usr/ucb/whoami`; else COMMAND_USR=`/usr/bin/whoami`; fi
elif [ `/bin/uname` = "AIX" ]; then
	COMMAND_USR=`/bin/whoami`
elif [ `/bin/uname` = "Linux" ]; then
	COMMAND_USR=`/usr/bin/whoami`
fi

# -- user check
if [ "$COMMAND_USR" != "root" ]; then
	echo "You must be root to run this process...aborted"
	exit 1
fi

CFG_DIR=/%FTDCFGDIR%
# -- Agent config file check
if [ ! -f $CFG_DIR/%Q%Agent.cfg ]; then
	echo "Agent is inactive. Please run %Q%agentset to activate the Agent."
	exit 1
fi

if [ `/bin/uname` = "Linux" ]
then
    # Check if SFTKdtc services are disabled (introduced for feature of Linux boot drive migration, RFX 2.7.2).
    if [ -e $CFG_DIR/SFTKdtc_services_disabled ]
    then
        /bin/echo "Not starting the %COMPANYNAME2% %PRODUCTNAME% agent because SFTKdtc services are disabled"
        /bin/echo "    (presence of file $CFG_DIR/SFTKdtc_services_disabled has been detected)."
        exit 1
    fi
fi

if [ `/bin/uname` = "Linux" ]; then
	CONF_FILE=/%OPTDIR%/%PKGNM%/driver/%Q%.conf
else
	CONF_FILE=/%FTDCONFDIR%/%Q%.conf
fi

# -- BAB check not needed here
# must allow Windows Common Console to init BAB parameters and load driver
# if BAB parameters are not yet set and driver is not yet loaded
#

# -- Agent start
echo "Start %COMPANYNAME2% %PRODUCTNAME% Agent daemon"
/usr/bin/nohup /%OPTDIR%/%PKGNM%/bin/in.%QAGN% >/tmp/in.%QAGN%.log 2>&1 &

exit 0
