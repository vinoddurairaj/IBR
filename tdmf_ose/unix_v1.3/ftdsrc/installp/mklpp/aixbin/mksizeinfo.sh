#!/bin/sh

# make either executables or documentation filesets
rtyp=""
while getopts r: name
do
	case ${name} in
		r)	rtyp="$OPTARG";;
		?)	Usg; exit 2;;
	esac
done
if [ "xx${rtyp}" = "xx" ]
then
	Usg;
	exit 1;
fi

RELTREEDIR=../rel;
if [ ${rtyp} = "E" ]
then
	FSETNAM=`./FSETNAM`;
else
	FSETNAM=`./FSETNAM`.doc;
fi
RELTREEROOT=${RELTREEDIR}/root/${FSETNAM};

# get size of install directories
SEDCMD=`pwd`/mksizeinfo.sed
rm -f ${SEDCMD}
cat > ${SEDCMD} << EOSEDCMD
\@${RELTREEROOT}@s@@@
EOSEDCMD

HERE=`pwd`
find ${RELTREEROOT} -type d |\
while read dir
do

	# skip /
	if [ "${dir}" = "${RELTREEROOT}" ]
	then
		continue;
	fi
	dsiz=0
	cd ${dir}
	for file in *
	do
		if [ -f ${file} ]
		then
			fsiz=`du -s ${file} | awk '{print $1}'`
			dsiz=`expr ${dsiz} + ${fsiz}`
		else 
			continue;
		fi
	done
	if [ ${dsiz} -ne 0 ]
	then
		rdir=`echo ${dir} | sed -f ${SEDCMD}`
		echo ${rdir} ${dsiz} 
	fi
	cd ${HERE}
	
done
rm -r ${SEDCMD}
