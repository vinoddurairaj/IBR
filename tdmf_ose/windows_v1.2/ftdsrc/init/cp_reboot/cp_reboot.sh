#!/bin/sh
#  cp_boot.sh
#
#  For HP-UX 10.20.
#
#  Copyright (c) 1999 Legato Systems, Inc., All rights reserved
#
#########################################################################
#
# This script will run a user defined cp_reboot_s###.sh script (if it exists)
# when a boot occurs while any group is in checkpoint mode on the secondary.
#
#########################################################################

cd /%OURCFGDIR%
CFGFILES=s*.cfg
PATH=$PATH:/bin

if [ ! -f $CFGFILES ]
then 
	exit 0
fi 

for FILE in $CFGFILES
do
    GRPNO=`expr $FILE : 's\([0-9][0-9][0-9]\)\.cfg'`
    JRNDIR=`grep "JOURNAL:" $FILE | awk '{print $2}'` 
    JRNDIRARRAY="${JRNDIRARRAY}$GRPNO $JRNDIR\n"
done

JRNDIRARRAY="${JRNDIRARRAY}x x"

echo $JRNDIRARRAY |
while read LINE
do
    GRPNO=`echo $LINE | awk '{print $1}'`
    JRN=`echo $LINE | awk '{print $2}'`
    
    if [ $GRPNO = x ]; then break;fi

    cd $JRN
    P_FILE=`echo j${GRPNO}.*.p`

    if [ -f $P_FILE ]; then
        CP_REBOOT=/%OURCFGDIR%/cp_reboot_s${GRPNO}.sh
        if [ -x $CP_REBOOT ]; then
            RET=`$CP_REBOOT`
            echo $RET
        fi
    fi
done
