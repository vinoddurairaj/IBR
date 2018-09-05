#!/bin/sh -x

#
# prepare current builds across platforms and brand names
#

# gotta be privileged
id=`id | awk '{print $1}' | awk -F= '{print $2}'| sed -e '\@(.*)@s@@@'`
if [ $id -ne 0 ]
then
	echo "$0: sorry, must be root.";
	exit 1
fi

# build sequence number
seqfile=./bounds
if [ -f ${seqfile} ]
then
	# increment the sequence number
	seqnum=`cat ${seqfile}`;
	seqnum=`expr ${seqnum} + 1`;
	echo ${seqnum} > ${seqfile};
	seqnum=`echo ${seqnum} | awk '{printf "%03d", $1}'`;
fi


# gotta launch from specific platform
launch=sophie;
host=`hostname`;
if [ ${host} != ${launch} ]
then
	echo "$0: sorry, must be run from host ${launch}";
	exit 1
fi

# usage 
usg="\
usage: $0 -b <build> \[-t <cvstag> \]\n\
  where <cvstag>, if present, is the symbolic tag \n\
      to be applied after the source is updated,\n\
      and prior to the compilation phase\n\
  where <build> is one of:\n\
    hp        - build all HP-UX brands and OS vers\n\
    hp1020    - build all HP-UX 1020 brands\n\
    hp1100    - build all HP-UX 1100 brands\n\
    hp1020dtc - build HP-UX 1020 dtc brand\n\
    hp1100dtc - build HP-UX 1100 dtc brand\n\
    hp1020mti - build HP-UX 1020 mti brand\n\
    hp1100mti - build HP-UX 1100 mti brand\n\
    hp1020stk - build HP-UX 1020 stk brand\n\
    hp1100stk - build HP-UX 1100 stk brand\n\
    sol       - build all SunOS brands\n\
    solmti    - build SunOS mti brand\n\
    soldtc    - build SunOS dtc brand\n\
    solstk    - build SunOS stk brand\n\
    dtc       - build dtc across all platforms\n\
    mti       - build mti across all platforms\n\
    stk       - build stk across all platforms\n\
    world     - build all brands across all platforms";

# conf of confs
. ./reng.conf

# source build host macros
. ${bldhostconf};

# source build target parameters macros
. ${bldtargetsconf}

# parse opts
build="";
tagit=0;
cvstag="";
while getopts b:t: opt
do
	case ${opt} in
		t) 
			tagit=1;
			cvstag=${OPTARG};;
		b) 
			what=${OPTARG};
			case ${what} in
				hp)
					build=${hpbld};;
				hp1020)
					build=${hp1020bld};;
				hp1100)
					build=${hp1100bld};;
				hp1020dtc)
					build=${hp1020dtcbld};;
				hp1100dtc)
					build=${hp1100dtcbld};;
				hp1020mti)
					build=${hp1020mtibld};;
				hp1100mti)
					build=${hp1100mtibld};;
				hp1020stk)
					build=${hp1020stkbld};;
				hp1100stk)
					build=${hp1100stkbld};;
				sol)
					build=${solbld};;
				soldtc)
					build=${soldtcbld};;
				solmti)
					build=${solmtibld};;
				solstk)
					build=${solstkbld};;
				dtc)
					build=${dtcbld};;
				mti)
					build=${mtibld};;
				stk)
					build=${stkbld};;
				world)
					build=${worldbld};;
				*)
					echo ${usg}; 
					exit 1;;
			esac;;
		
		*) 
			echo ${usg}; 
			exit 1;;
	esac
done


# usage
if [ xx"${build}" = "xx" ]
then
	echo ${usg};
	exit 1;
fi
if [ ${tagit} -eq 1 ]
then
	if [ xx"${cvstag}" = "xx" ]
	then
		echo ${usg};
		exit 1;
	fi
fi

# environment
ECHO="";
PATH=$PATH:/usr/local/bin:.;
export PATH;

# namespace
here=`pwd`;
there=$here;

# supporting tools
lndir=/usr/local/bin/lndir;
gmake=/usr/local/bin/gmake;
cvs=/usr/local/bin/cvs;
mail=/usr/ucb/Mail;

# concurrency control
giantlock=`basename ./scmbld.LCK`;

if [ -f ${giantlock} ]
then 
	echo "$0: locked, giving up.";
	exit 1
else
	# lock
	touch ${giantlock}
fi

# clean out and update the source
$ECHO cd ${there}/src;
# $ECHO ${cvs} update;
# $ECHO ${gmake} clean;
# $ECHO ${cvs} co -r;

# lets do it...
$ECHO cd ${here};

# oddities...
# files that must be removed from the shared source 
# prior to the build. these files include brand specifics
# which are defined at branding time.
$ECHO rm -f src/mk/vars.gmk src/installp/mklpp/aixbin/Makefile src/pkg.install/prototype.doc

# launch the builds
echo ${build} |
while read bld h brand
do
	what=${bld}bld_${seqnum}_${brand};
	$ECHO rm -rf ${what};
	$ECHO mkdir ${what};
	$ECHO ${lndir} ${there}/src ${there}/${what};
	$ECHO find ${there}/${what} -type d -name CVS -exec rm -rf {} \;
	$ECHO rsh -n ${h} ${there}/${bld}.bldcmd ${there} ${what} ${brand} ${seqnum} &
done

#
# wait for things to get rolling
#
sleep 180

#
# poll for completion
#
lcks=`find ./* -prune -name "*lck"`
while /bin/true
do
	if [ -z "${lcks}" ]
	then
		break;
	fi
	sleep 60;
	lcks=`find ./* -prune -name "*lck"`
done

# unlock
rm -f ${giantlock};

exit 0;
