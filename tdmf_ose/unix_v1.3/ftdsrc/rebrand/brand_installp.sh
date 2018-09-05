#!/bin/ksh

#
# define mk parameters for rebranded product versions.
#

#
# do so by passing vars.gmk_in through a stream filter,
# outputting vars.gmk

#
# modify vars.gmk parameters:
#
#	QNAME     := product name prefix, lcase
#	CAPQNAME  := product name prefix, ucase
#	OEMNAME   := OEM name, path name component usage

#
# generalities
#
INSTALLPDIR=../installp/mklpp/aixbin
INPUT=${INSTALLPDIR}/MAKEFILE
OUTPUT=${INSTALLPDIR}/Makefile
DFLTBRAND=TDMFBRAND;
BRAND=${DFLTBRAND};

#
# default brand, direct sales version (dtc).
#
QNAME=dtc;
CAPQNAME=DTC;
OEMNAME=SFTK;


#
# all brands...
#
ALLBRANDS="DTCBRAND TDMFBRAND MTIBRAND STKBRAND";

#
# usage
#
usg="\
Usage: $0 -b <brandname> \\n\
  where <brandname> is one of: \\n\
  ${ALLBRANDS}"

# parse args, if any
if [ $# -ge 1 ]
then
	brand="";
	while getopts b: opt
	do
		brand=${OPTARG};
		case ${opt} in
			b)
				case ${brand} in
					TDMFBRAND) 
						#
						QNAME=dtc;
						CAPQNAME=DTC;
						OEMNAME=SFTK;;
						#
					DTCBRAND)
						#
						QNAME=dtc;
						CAPQNAME=DTC;
						OEMNAME=LGTO;;
						#
					MTIBRAND) 
						#
						# MTI OEM sales version (mds)
						#
						QNAME=mds;
						CAPQNAME=MDS;
						OEMNAME=MTI;;
					STKBRAND) 
						#
						# STK OEM sales version (???)
						#
						QNAME=stk;
						CAPQNAME=STK;
						OEMNAME=STK;;
					*)echo there: ${usg};exit 1;;
				esac ;;
			*)echo ${usg};exit 1;;
			esac
	done
fi

#
# make the filter
#
SEDFIL="/tmp/$$.sed";
cat > ${SEDFIL} << EOSEDFIL
\@%QNAME%@s@@${QNAME}@
\@%CAPQNAME%@s@@${CAPQNAME}@
\@%OEMNAME%@s@@${OEMNAME}@
EOSEDFIL

#
# apply filter, create `branded' vars.gmk
#
echo "Making ${OUTPUT} from ${INPUT} for brand ${brand}";
cat ${INPUT} | sed -f ${SEDFIL} > ${OUTPUT}


#
# mop up
#
rm -f ${SEDFIL};
exit 0;
