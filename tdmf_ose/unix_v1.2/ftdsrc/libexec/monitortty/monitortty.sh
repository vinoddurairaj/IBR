#!/bin/sh
#-----------------------------------------------------------------------------
#
#  %Q%monitortty -- Health / Info Monitoring shell
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
#-----------------------------------------------------------------------------

#
# -- "USAGEHELP" function start
#
USAGEHELP()
{
	echo "Usage:"
	echo "%Q%monitortty [-p|-s] [-l logfile] [-i interval] [-g lgnum [lgnum] ...]"
	echo "	-p          : Displays data for the primary side"
	echo "	-s	    : Displays data for the secondary side"	
	echo "	-l logfile  : Redirects the output to \"logfile\"" 
	echo "	-i interval : Refreshes the display every \"interval\" seconds"
	echo "		      If interval is not specified then it defaults to 5 seconds"
	echo "	-g lgnum    : Displays information for the logical group \"lgnum\""
	echo "		      If no logical groups are specified then data for all the groups are displayed"
	echo "	-h|-? 	    : Displays this usage help"
}
#
# -- "USAGEHELP" function end
#

#
# -- "PRIMARYDISPLAYSTART" function start
#
PRIMARYDISPLAYSTART()
{
	cd $ETCDIR
	GRPARRAY1=""
	for VAR in $GRPARRAY
	do
		if [ -f p${VAR}.cur ]; then
			GRPARRAY1="${GRPARRAY1} $VAR"
		fi
	done

	cd $VARDIR
        [ -f dtcerror.log ] && {
		tail -10 dtcerror.log | awk -F\[ '{print $2 $3 $6 $7 $8}' | sed 's/proc: //g' |sed 's/]//g'

	}
	echo "+-------------------------------------------------------------------------------------------+"
	echo "|            |  Local Read | Local Write | Net  Actual | Net  Effect | Entries | % BAB used |"

	for VAR1 in $GRPARRAY1
	do
		ONE_LINE=`tail -1 p${VAR1}.prf | sed 's/||/|/g' | sed 's/\/dev\/dtc\/lg*[0-9]*\/rdsk\///g'`

		# -- mode check
		MODE=`echo $ONE_LINE | awk '{print $10}'`
	
		case "$MODE" in
			0)
				MODE1="NORMAL  "
				;;
			1)
				MODE1="TRACKING"
				;;
			2)
				MODE1="PASSTHRU"
				;;
			3)
				MODE1="REFRESH "
				;;
			4)
				MODE1="BACKFRESH"
				;;
            5)
 				MODE1="NETWKEVAL"
				;;
			*)
				MODE1="???MODE "
				;;
		esac

		echo $ONE_LINE | awk -F'|' '{printf("+- %8s",$1)}'
		echo "-+- Logical Group $VAR1 ----------------------------------------------------------+"

		PRFVALUE=`echo $ONE_LINE | awk -F'|' '{ for (i = 2; i < NF+1 ; i++) print $i }' | \
			awk 'BEGIN{s1=0;s2=0;s3=0;s4=0;s5=0;s6=0;s7=0} \
			{s1+=$1;s2+=$9;s3+=$10;s4+=$2;s5+=$3;s6+=$4;s7+=$7} END {print s2,s3,s4,s5,s6,s7}'`

		echo $MODE1 $PRFVALUE | \
			awk '{printf("| %9s  |%10s KB %10s KB %10s KB %10s KB %9s  %9s% | \n",$1,$2,$3,$4,$5,$6,$7)}'

		echo $ONE_LINE | awk -F'|' '{ for (i = 2; i < NF+1 ; i++) print $i }' | \
			awk '{s1=$1;s2=$6;s3=$9;s4=$10;s5=$2;s6=$3;s7=$4;s8=$7} \
			{printf("|%5s %6s|%10s KB %10s KB %10s KB %10s KB %9s  %9s% |\n",s1,s2,s3,s4,s5,s6,s7,s8)}'

	done
	echo "+-------------------------------------------------------------------------------------------+"
	echo "< Quit : [CTRL +C] > "
}
# -- "PRIMARYDISPLAYSTART" function end


#
# -- "SECONDARYDISPLAYSTART" function start
#
SECONDARYDISPLAYSTART()
{
	cd $VARDIR
	GRPARRAY1=""
	for VAR in $GRPARRAY
	do
		if [ -f s${VAR}.prf ]; then
			GRPARRAY1="${GRPARRAY1} $VAR"
		fi
	done

	cd $VARDIR
        [ -f dtcerror.log ] && {
		tail -10 dtcerror.log | awk -F\[ '{print $2 $3 $6 $7 $8}' | sed 's/proc: //g' |sed 's/]//g'
	}

	echo "+---------------------------------------------+"
	echo "|                |  Net  Actual | Net  Effect |"

	for VAR1 in $GRPARRAY1
	do
		ONE_LINE=`tail -1 s${VAR1}.prf | sed 's/||/|/g' | sed 's/\/dev\///g'`   # PRFファイルのLINE数指定

		echo $ONE_LINE | awk -F'|' '{printf("+--- %8s",$1)}'
		echo "---+- Logical Group $VAR1 --------+"

		PRFVALUE=`echo $ONE_LINE | awk -F'|' '{ for (i = 2; i < NF+1 ; i++) print $i }' | \
			awk 'BEGIN{s1=0;s2=0} {s1+=$2;s2+=$3} END {print s1,s2}'`

		echo $PRFVALUE | awk '{printf("|                |%10s KB %10s KB | \n",$1,$2)}'

		echo $ONE_LINE | awk -F'|' '{ for (i = 2; i < NF+1 ; i++) print $i }' | \
			awk '{s1=$1;s2=$2;s3=$3} {printf("|%15s |%10s KB %10s KB |\n",s1,s2,s3)}'
	done
	echo "+---------------------------------------------+"
	echo "< Quit : [CTRL +C] > "
}
#
# -- "SECONDARYDISPLAYSTART" function end
#


LANG=C
export LANG
# Check if user is root

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

#
# Set some platform specific variables
#
ETCDIR=/%FTDCFGDIR%
[ -d $ETCDIR ] || {
	echo "Directory $ETCDIR does not exist."
	exit 1
}

VARDIR=/%FTDVAROPTDIR%
[ -d $VARDIR ] || {
	echo "Directory $VARDIR does not exist."
	exit 1
}

GRPS=""
PRIMARYFLAG=0
SECONDARYFLAG=0
INTERVALFLAG=0
LOGFLAG=0
GROUPFLAG=0
NUM=1
ARGS=$#
while [ $NUM -le $ARGS ]
do
    case $1 in
	-p) if [ $SECONDARYFLAG -eq 1 ]; then
		echo "Only one of p or s must be specified"
		exit
       	    else
		PRIMARYFLAG=1
     	    fi;;
	-s) if [ $PRIMARYFLAG -eq 1 ]; then
		echo "Only one of p or s must be specified"
		exit
     	    else
		SECONDARYFLAG=1
     	    fi;;
	-h|-\?) USAGEHELP
        	exit;;
	-i) if [ $INTERVALFLAG -eq 0 ]; then
		NUM=`expr $NUM + 1`
		shift
		if [ -z "$1" ]; then
                        echo "Please specify an interval"
			exit
		fi
		echo $1 | grep "[^0-9]" > /dev/null 2>&1
		if [ $? -eq 0 ]; then
			echo "Enter a valid interval"
			exit
		else
			INTERVAL=$1
		fi
		INTERVALFLAG=1
    	    else
		echo "Specify -i option only once"
		exit
    	    fi;;
	-l) if [ $LOGFLAG -eq 0 ]; then
		NUM=`expr $NUM + 1`
		shift
		if [ -z "$1" ]; then
                        echo "Please specify a filename"
			exit
		fi
		touch $1 > /dev/null 2>&1
		if [ $? -eq 0 ]; then
			LOG=$1
		else
			echo "Invalid filename"
			exit
		fi
		LOGFLAG=1
    	    else
		echo "Specify -l option only once"
		exit
    	    fi;;
	-g) if [ $GROUPFLAG -eq 0 ]; then
		GROUPFLAG=1
		NUM=`expr $NUM + 1`
	    	shift
		if [ -z "$1" ]; then
			echo "Please specify the groups with -g option"
			exit
		fi
		echo $1 | grep "^-" > /dev/null 2>&1
	    	if [ $? -eq 0 ]; then
			echo "Please specify the groups with -g option"
			exit
 	    	fi 
	    	while [ $NUM -le $ARGS ]
	    	do
			echo $1 | grep "^-" > /dev/null 2>&1
			if [ $? -eq 0 ]; then
				break	
			else
				echo $1 | grep "[^0-9]" > /dev/null 2>&1
	   			if [ $? -eq 0 ]; then
					echo "Enter a valid group number"
					exit
	   			else
					ISREP=0
					if [ -n "$GRPS" ]; then
						for ONEGRPS in $GRPS
						do
							if [ "$ONEGRPS" -eq "$1" ]; then
								ISREP=1
							fi
						done
					fi
					if [ $ISREP -eq 0 ]; then
						GRPS="$GRPS $1"
					fi
	   			fi
			fi
			NUM=`expr $NUM + 1`
			shift
	   	done
	   else
		echo "Specify the -g option only once"
		exit
	   fi
	   continue;;
	-*) echo "Invalid option: $1"
	    USAGEHELP
	    exit;;
	*) echo "Invalid usage"
	   USAGEHELP
	   exit;;
    esac
    NUM=`expr $NUM + 1`
    shift
done

if [ $PRIMARYFLAG -eq 0 -a $SECONDARYFLAG -eq 0 ]; then
	PRIMARYFLAG=1
fi

# -- Set variable
DIR=`pwd`

# WR PROD7052: replacing $() (needs ksh, taken out for RH6 support) by backquotes
cd $ETCDIR
if [ $PRIMARYFLAG -eq 1 ]; then
	CFGFILES=`ls -1d p[0-9][0-9][0-9].cfg 2>/dev/null`
else
	CFGFILES=`ls -1d s[0-9][0-9][0-9].cfg 2>/dev/null`
fi
if [ $INTERVALFLAG -eq 0 ]; then
	INTERVAL=5
fi

#
# check the CFG files
#
if [ -z "$CFGFILES" ]
then
	echo "cfg files not found"
	exit 0
fi
if [ $PRIMARYFLAG -eq 1 ]; then
for FILE in $CFGFILES
do
	GRPNO=`expr $FILE : 'p\([0-9][0-9][0-9]\)\.cfg'`
	TMPARRAY="$TMPARRAY $GRPNO"
done
fi

if [ $SECONDARYFLAG -eq 1 ]; then
for FILE in $CFGFILES
do
	GRPNO=`expr $FILE : 's\([0-9][0-9][0-9]\)\.cfg'`
	TMPARRAY="$TMPARRAY $GRPNO"
done
fi

if [ -n "$GRPS" ]; then
for EACHGRPS in $GRPS
do
	FOUND=0
	for EACHTMPARRAY in $TMPARRAY
	do
		if [ "$EACHTMPARRAY" -eq "$EACHGRPS" ]; then
			GRPARRAY="$GRPARRAY $EACHTMPARRAY"
			FOUND=1
			break
		fi
	done
	if [ $FOUND -eq 0 ]; then
		echo "Group $EACHGRPS not found"
	fi
done
else
	GRPARRAY=$TMPARRAY
fi

if [ -z "$GRPARRAY" ]; then
	exit
fi

while [ 1 ]
do
	cd $DIR
	if [ $PRIMARYFLAG -eq 1 ]; then
		if [ $LOGFLAG -eq 1 ]; then
		    PRIMARYDISPLAYSTART>>$LOG
		else
		    PRIMARYDISPLAYSTART
		fi
	else
		if [ $LOGFLAG -eq 1 ]; then
	  	    SECONDARYDISPLAYSTART>>$LOG
		else
		    SECONDARYDISPLAYSTART
		fi
	fi
        sleep $INTERVAL
	if [ $LOGFLAG -eq 0 ]; then
 	   clear
	fi
done
exit 0
