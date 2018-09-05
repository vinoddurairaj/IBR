#!/bin/sh

#
# prepare CDROM distribution
#

# CDROM burners have guaranteed rate I/O requirements.
# these are generally met if the source disk being copied 
# is local. 
# so ... 

# conf of confs
. ./reng.conf;

# run this on the CDROM production system named below:
. ${cdromprodsysconf};

# gotta launch from specific platform
launch=${cdromprodsys};
host=`hostname`;
if [ ${host} != ${launch} ]
then
        echo "$0: sorry, must be run from host ${launch}";
        exit 1
fi

# gotta be root
id=`id | awk '{print $1}' | awk -F= '{print $2}'| sed -e '\@(.*)@s@@@'`
if [ $id -ne 0 ]
then
	echo "$0: sorry, must be root.";
	exit 1
fi

# usage 
usg="\
usage: $0 -d <distribution root directory> -b <build> -n <seqnum> \n\
  where <seqnum> is the build number of the compilation phase:\n\
  where <build> is one of:\n\
    hp    - make distribution of all brands, HP-UX only\n\
    aix   - make distribution of all brands, AIX only\n\
    sol   - make distribution of all brands, SunOS only\n\
    dtc   - make distribution of dtc brand across all platforms\n\
    mti   - make distribution of mti brand across all platforms\n\
    stk   - make distribution of stk brand across all platforms\n\
    world - make distribution of all brands across all platforms";

# parse opts
what="";
dist="";
distroot="";
seqnum="";
while getopts d:b:n: opt
do
	case ${opt} in
		d)
			distroot="$OPTARG";;
		n) 
			seqnum=$OPTARG;
			seqnum=`echo ${seqnum} | awk '{printf "%03d", $1}'`;;
		b) 
			what=${OPTARG};;
		
		*) 
			echo ${usg}; 
			exit 1;;
	esac
done

# usage chk
if [ xx"${what}" = "xx" ]
then
	echo "Usage: $0 ... -b <build> ...";
	echo "  need to specify distribution type.";
	exit 1;
fi
if [ "xx${distroot}" = "xx" ]
then
	echo "Usage: $0 ... -d <distribution source root> ...";
	echo "  need to specify distribution source root directory.";
	exit 1;
fi
if [ ! -d ${distroot} ]
then
	echo "$0: No directory: ${distroot}.";
	exit 1;
fi
if [ "xx${seqnum}" = "xx" ]
then
	echo "Usage: $0 ... -n <seqnum> ...";
	echo "  need to specify build sequence number.";
	exit 1;
fi

# production source parameters, 
# according to brand and platform,
# or combinations...

# production targets
. ${disttargetsconf};

# collect parameters for laying out the production source.
case ${what} in
	hp)
		target="hp    - make distribution of all brands, HP-UX only";
		dist=${hpdist};;
	hpstk)
		target="hp    - make distribution of all brands, HP-UX only";
		dist=${hpstkdist};;
	aix)
		target="aix   - make distribution of all brands, AIX only";
		dist=${aixdist};;
	sol)
		target="sol   - make distribution of all brands, SunOS only";
		dist=${soldist};;
	dtc)
		target="dtc   - make distribution of dtc brand across all platforms";
		dist=${dtcdist};;
	mti)
		target="mti   - make distribution of mti brand across all platforms";
		dist=${mtidist};;
	stk)
		target="stk   - make distribution of stk brand across all platforms";
		dist=${stkdist};;
	world)
		target="world - make distribution of all brands across all platforms";
		dist="${hpdist}\n${aixdist}\n${soldist}";;
	*)
		echo ${usg}; 
		exit 1;;
esac

# localities
here=`pwd`
there=/scmbld/SCMBLD/bldrls;
aixcdrom=installp/mklpp/rel/dist/CD-ROM/usr/sys/inst.images;

# supporting tools
lndir=/usr/local/bin/lndir;
gmake=/usr/local/bin/gmake;
cvs=/usr/local/bin/cvs;
mail=/usr/ucb/Mail;

ECHO=""

#
# clean:
#

# target to build
echo;
echo "Building target: ${target}";
echo;


# confirm
#echo "NOTICE: this will clear contents of ${distroot}.";
#echo "proceed?[y/n][n]:";
#read ans
#if [ "${ans}" != "y" ]
#then
	#echo "exiting"
	#exit 1;
#fi

# do it
#${ECHO} rm -rf ${distroot};

#
# pave 
#
echo ${dist} |
while read p w s
do
	${ECHO} mkdir -p ${s};
done

# misc
bar="=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=";

#
# load
#
echo ${dist} |
while read p b s
do

	# source from which to populate the build area
	what=${p}bld_${seqnum}_${b};

	
	# explain what's happening
	echo ${bar}
	echo "Populating the distribution source:";
	echo "  ${s} ";
	echo "     from";
	echo "  ${there}/${what}";
	echo ;

	case ${p} in

		"hp") 
			. ${hpbldconf};
			echo "cd ${cdromdir}/${what}";
			cd ${cdromdir}/${what};
			tar cvf - ./* | ( cd ${s}; tar xvf -);;
		
		"aix")
			if [ ! -d ${there}/${what} ]
			then
				echo "No such file or directory: ${there}/${what}.";
				exit 1;
			fi
			echo "cd ${there}/${what}";
			cd ${there}/${what};
			echo "cd ${aixcdrom}";
			cd ${aixcdrom};
			tar cvf - . | ( cd ${s}; tar xvf -);;
			
		"sol")
			if [ ! -d ${there}/${what} ]
			then
				echo "No such file or directory: ${there}/${what}.";
				exit 1;
			fi
			echo "cd ${there}/${what}";
			cd ${there}/${what};
			cat *.tar | ( cd ${s}; tar xvf - );;

	esac
	cd ${here};
done

#
# owner/group mode
#
cd ${distroot};
chown -R root:sys ./*;


exit 0;
