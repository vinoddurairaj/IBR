#!/usr/bin/ksh

#
# define mk parameters for rebranded product versions.
#

#
# do so by passing vars.gmk_in through a stream filter,
# outputting vars.gmk

#
# modify vars.gmk parameters:
#
#	QNAME		     := product name prefix case sensitive
#	CAPQNAME	     := product name prefix, all uppercase
#	OEMNAME		     :=  OEM name, path name component usage
#	PDFQNAME	     := doc file name prefix, lcase
#	QAGNNM		     := Agent product name prefix, case sensitive
#	CAPQAGNNM	     := Agent product name prefix, all uppercase
#	FIXNUM		     := GA (0) or patch/hotfix level (> 0)
#	BUILDNUM	     := build release number
#	BUILDPREFIX	     := version string suffix and build release prefix combined
#	BUILDSUFFIX	     := build release suffix
#	OEMNAME		     :=
#	PRODUCTNAME	     :=
#	CAPPRODUCTNAME	     := unused
#	BRANDTOKEN	     := C Preprocessor testable value
#	GROUPNAME	     := logical group name (replication/migration)
#	CAPGROUPNAME	     := logical group name (Replication/Migration)
#	GIFPREFIX	     := filename prefix for branded gif files
#	PRODUCTNAME_TOKEN    :=
#	COMPANYNAME	     := Company Legal Name like for copyright text
#	COMPANYNAME2	     := Company Short Name like for messages
#	COMPANYNAME3	     :=
#	SUPPORTNAME	     := 
#	OSRELEASE	     :=
#	ARCH		     :=
#	PRODUCTNAME_SHORT    :=
#
# generalities
#
MKFDIR=../mk
INPUT=${MKFDIR}/vars.gmk_in;
OUTPUT=${MKFDIR}/vars.gmk;
DFLTBRAND=TDMFBRAND;
BRAND=${DFLTBRAND};

#
# default brand, direct sales version (dtc).
#

typeset -u CAPQNAME CAPPRODUCTNAME CAPQAGNNM

QNAME=dtc;
CAPQNAME=${QNAME};
OEMNAME=SFTK;
PDFQNAME=rep;
PRODUCTNAME=Replicator
BRANDTOKEN=R
GROUPNAME=replication
CAPGROUPNAME=Replication
GIFPREFIX=REPLICATOR
CAPPRODUCTNAME=${PRODUCTNAME}
COMPANYNAME="IBM Corp."
COMPANYNAME2=""
COMPANYNAME3=""
SYSTYPE=`/bin/uname -s`
ARCH=
if [ ${SYSTYPE} = "Linux" ]; then
	OSRELEASE=`/bin/uname -r | awk '{split($0,a,"-"); print a[1]"-"}'`;
	ARCH=`uname -m`
else
	OSRELEASE=`/bin/uname -r`;
fi
if [ ${SYSTYPE} = "HP-UX" ]; then
	ARCH=pa
	[ `uname -m` = "ia64" ] && ARCH=ipf
fi
QAGNNM=dua;
CAPQAGNNM=${QAGNNM};


#
# all brands...
#
ALLBRANDS="TDMFBRAND REPLBRAND";

#
# usage
#
usg="\
Usage: $0 -b <brandname> -f <fix> -n <build> -p <buildprefix> -s<buildsuffix>\\n\
  where <brandname> is one of: \\n\
  ${ALLBRANDS}"

# parse args, if any
if [ $# -ge 1 ]
then
	brand="";
	while getopts b:n:f:p:s: opt
	do
		case ${opt} in
			n)
				BUILDNUM=$OPTARG;;
			f)
				FIXNUM=$OPTARG;;
			p)
				BUILDPREFIX=$OPTARG;;
			s)
				BUILDSUFFIX=$OPTARG;;
			b)
				brand=${OPTARG};
				case ${brand} in
					TDMFBRAND) 
						#
						QNAME=dtc;
						CAPQNAME=${QNAME};
						QAGNNM=dua;
						CAPQAGNNM=${QAGNNM};
						OEMNAME=SFTK;
						PDFQNAME=tdmf;
						PRODUCTNAME="Softek TDMF(IP) for UNIX*";
						PRODUCTNAME_TOKEN="TDMFIP";
						PRODUCTNAME_SHORT="TUIP";
						CAPPRODUCTNAME=${PRODUCTNAME};
						TECHSUPPORTNAME="Data Mobility Soltions";
						GIFPREFIX="TDMF";
						PRODUCTBRAND="tdmf";
						BRANDTOKEN="M";
						GROUPNAME="migration";
						CAPGROUPNAME="Migration";
						COMPANYNAME="IBM Corp.";
						COMPANYNAME2="IBM";
						COMPANYNAME3="";;
						#
					REPLBRAND) 
						#
						QNAME=dtc;
						CAPQNAME=${QNAME};
						QAGNNM=dua;
						CAPQAGNNM=${QAGNNM};
						OEMNAME=SFTK;
						PDFQNAME=rep;
						PRODUCTNAME="Softek Replicator for UNIX*";
						PRODUCTNAME_TOKEN="Replicator";
						PRODUCTNAME_SHORT="RFX";
						CAPPRODUCTNAME=${PRODUCTNAME};
						TECHSUPPORTNAME="Data Mobility Soltions";
						GIFPREFIX="REPLICATOR";
						BRANDTOKEN="R";
						PRODUCTBRAND="replicator";
						GROUPNAME="replication";
						CAPGROUPNAME="Replication";
						COMPANYNAME="IBM Corp.";
						COMPANYNAME2="IBM";
						COMPANYNAME3="";;
						#
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
exec 3>${SEDFIL}
cat >&3 <<-EOSEDFIL
\@%%QNAME%%@s@@${QNAME}@
\@%%PDFQNAME%%@s@@${PDFQNAME}@
\@%%CAPQNAME%%@s@@${CAPQNAME}@
\@%%OEMNAME%%@s@@${OEMNAME}@
\@%%PRODUCTNAME%%@s@@${PRODUCTNAME}@
\@%%PRODUCTBRAND%%@s@@${PRODUCTBRAND}@
\@%%CAPPRODUCTNAME%%@s@@${CAPPRODUCTNAME}@
\@%%PRODUCTNAME_TOKEN%%@s@@${PRODUCTNAME_TOKEN}@
\@%%COMPANYNAME%%@s@@${COMPANYNAME}@
\@%%COMPANYNAME2%%@s@@${COMPANYNAME2}@
\@%%COMPANYNAME3%%@s@@${COMPANYNAME3}@
\@%%OSRELEASE%%@s@@${OSRELEASE}@
\@%%ARCH%%@s@@${ARCH}@
\@%%QAGNNM%%@s@@${QAGNNM}@
\@%%CAPQAGNNM%%@s@@${CAPQAGNNM}@
\@%%GROUPNAME%%@s@@${GROUPNAME}@
\@%%CAPGROUPNAME%%@s@@${CAPGROUPNAME}@
\@%%BRANDTOKEN%%@s@@${BRANDTOKEN}@
\@%%GIFPREFIX%%@s@@${GIFPREFIX}@
\@%%PRODUCTNAME_SHORT%%@s@@${PRODUCTNAME_SHORT}@
EOSEDFIL

if [[ "${FIXNUM+FixnumHasValue}" = 'FixnumHasValue' ]]
then
    cat >&3 <<- EOSEDFIL
	\@%%FIXNUM%%@s@@${FIXNUM}@
	EOSEDFIL
fi
if [[ "${BUILDNUM+BuildnumHasValue}" = 'BuildnumHasValue' ]]
then
    cat >&3 <<- EOSEDFIL
	\@%%BUILDNUM%%@s@@${BUILDNUM}@
	EOSEDFIL
fi
if [[ "${BUILDPREFIX+BuildprefixHasValue}" = 'BuildprefixHasValue' ]]
then
    cat >&3 <<- EOSEDFIL
	\@%%BUILDPREFIX%%@s@@${BUILDPREFIX}@
	EOSEDFIL
fi
if [[ "${BUILDSUFFIX+BuildsuffixHasValue}" = 'BuildsuffixHasValue' ]]
then
    cat >&3 <<- EOSEDFIL
	\@%%BUILDSUFFIX%%@s@@${BUILDSUFFIX}@
	EOSEDFIL
fi
exec 3>&-

#
# apply filter, create `branded' vars.gmk
#
echo "Making ${OUTPUT} from ${INPUT} for brand ${brand}";
rm -f ${OUTPUT}
cat ${INPUT} | sed -f ${SEDFIL} > ${OUTPUT}
(cd ../mk; gmake clean all )

#
# mop up
#
rm -f ${SEDFIL};
exit 0;
