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
#       PDFQNAME  := doc file name prefix, lcase
#
# generalities
#
MKFDIR=../mk
INPUT=${MKFDIR}/vars.gmk_in;
OUTPUT=${MKFDIR}/vars.gmk;
DFLTBRAND=TDMFBRAND;
BRAND=${DFLTBRAND};
DFLTSEQNUM=000;
SEQNUM=${DFLTSEQNUM};

#
# default brand, direct sales version (dtc).
#
QNAME=dtc;
CAPQNAME=DTC;
OEMNAME=SFTK;
PDFQNAME=tdmf;
PRODUCTNAME=TDMF
CAPPRODUCTNAME=TDMF
COMPANYNAME="Fujitsu Software Technology Corporation"


#
# all brands...
#
ALLBRANDS="DTCBRAND TDMFBRAND MTIBRAND STKBRAND";

#
# usage
#
usg="\
Usage: $0 -b <brandname> -n <seqnum> \\n\
  where <brandname> is one of: \\n\
  ${ALLBRANDS}"

# parse args, if any
if [ $# -ge 1 ]
then
	brand="";
	while getopts b:n: opt
	do
		brand=${OPTARG};
		case ${opt} in
			n)
				SEQNUM=$OPTARG;;
			b)
				case ${brand} in
					TDMFBRAND) 
						#
						QNAME=dtc;
						CAPQNAME=DTC;
						OEMNAME=SFTK;
						PDFQNAME=tdmf;
						PRODUCTNAME=TDMF;
						CAPPRODUCTNAME=TDMF;
						COMPANYNAME="Fujitsu Software Technology Corporation";;
						#
					DTCBRAND)
						#
						QNAME=dtc;
						CAPQNAME=DTC;
						OEMNAME=LGTO;
						PDFQNAME=rep;
						PRODUCTNAME=Replication;
						CAPPRODUCTNAME=REPLICATION;
						COMPANYNAME="Legato Systems Incorporated";;
						#
					MTIBRAND) 
						#
						# MTI OEM sales version (mds)
						#
						QNAME=mds;
						CAPQNAME=MDS;
						OEMNAME=MTI;
                                                PDFQNAME=mti;
						PRODUCTNAME=DataSentry;
						CAPPRODUCTNAME=DATASENTRY;
						COMPANYNAME="MTI Technology Corporation";;
					STKBRAND) 
						#
						# STK OEM sales version (???)
						#
						QNAME=stk;
						CAPQNAME=STK;
						OEMNAME=STK;
                                                PDFQNAME=stk;
						PRODUCTNAME='OPENstorage\\ Data\\ Replication';
						CAPPRODUCTNAME='OPENSTORAGE\\ DATA\\ REPLICATION';
						COMPANYNAME="Storage Technology Corporation";;
					*)echo there: ${usg};exit 1;;
				esac ;;
			*)echo ${usg};exit 1;;
			esac
	done
fi

SEQNUM=`echo ${SEQNUM} | awk '{printf "%03d", $1}'`;

# tokenize PRODUCTNAME and CAPPRODUCTNAME
PRODUCTNAME_TOKEN=`echo ${PRODUCTNAME} | sed -e 's/ /_/g'`;
CAPPRODUCTNAME_TOKEN=`echo ${CAPPRODUCTNAME} | sed -e 's/ /_/g'`;

#
# make the filter
#
SEDFIL="/tmp/$$.sed";
cat > ${SEDFIL} << EOSEDFIL
\@%%QNAME%%@s@@${QNAME}@
\@%%PDFQNAME%%@s@@${PDFQNAME}@
\@%%SEQNUM%%@s@@${SEQNUM}@
\@%%CAPQNAME%%@s@@${CAPQNAME}@
\@%%OEMNAME%%@s@@${OEMNAME}@
\@%%PRODUCTNAME%%@s@@${PRODUCTNAME}@
\@%%CAPPRODUCTNAME%%@s@@${CAPPRODUCTNAME}@
\@%%PRODUCTNAME_TOKEN%%@s@@${PRODUCTNAME_TOKEN}@
\@%%CAPPRODUCTNAME_TOKEN%%@s@@${CAPPRODUCTNAME_TOKEN}@
\@%%COMPANYNAME%%@s@@${COMPANYNAME}@
EOSEDFIL

#
# apply filter, create `branded' vars.gmk
#
echo "Making ${OUTPUT} from ${INPUT} for brand ${brand}";
cat ${INPUT} | sed -f ${SEDFIL} > ${OUTPUT}

#
# mop up
#
#rm -f ${SEDFIL};
exit 0;
