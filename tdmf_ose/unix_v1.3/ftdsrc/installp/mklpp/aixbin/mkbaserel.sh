#!/bin/sh

# populate a product AIX base release  hierarchy,  make  a  release
# backup(8) installation package image, layout CD-ROM distribution.
# 
# knows the difference between a distribution of executables and  a
# distribution  of  documentation,  and  can  build either one. see
# Usg().
# 
# update and fix packages are  TBD,  though  this  script  and  its
# buddies  should  serve  as  a nice basis for development of those
# functions.
# 
# AIX doesn't have the use of `opt' directories to root OEM  products,
# nor does it have SYSV init states processing.
# 
# To minimize the affect of these differences  for  the  AIX  port,
# wrote  this  script  to figure out where the product is rooted in
# the SOLARIS and HPUX namespaces, and apply  a  mapping  of  those
# roots to appropriate AIX directories.
# 
# Additional complexity is introduced into the AIX installation  by
# the  partition  of  the base installation fileset into root, usr,
# and share parts.
# 
# The mapping operation prepends a pattern to each file name in the
# base  installation fileset. This pattern is then used to sort the
# names into root, usr, and share parts, so named according to  the
# AIX  filesystem  partitions that the fileset members are restored
# to at installation time.
# 
# this and other scripts in  this  directory  encapsulate  whatever
# little  wisdom was derived from several days of digging around in
# inacurate documentation and the the guts  of  other  installation
# packages,  finally ariving at something that seems to work OK. if
# not necessary, i wouldn't recommend changing it much were i  you,
# unless you enjoy pain...
Usg()

{
	echo "Usage: $0 -r <release fileset> [-f]"
	echo "  -r <release fileset> make a base release"
	echo "where <release fileset> is one of:"
	echo "  E - executables"
	echo "  D - documentation"
	echo "  [-f] rebuild inventory tables from scratch"
	echo "  [-c] clean up only "
}

bar="-----------------------------------------------------------"

# whether to use cached data
needinvfils=0
cleanonly=0

# make either executables or documentation filesets
rtyp=""

while getopts cfr: name
do
	case ${name} in
		r)	rtyp="$OPTARG";;
		f)	needinvfils=1;;
		c)	cleanonly=1;;
		?)	Usg; exit 2;;
	esac
done
if [ "xx${rtyp}" = "xx" ]
then
	Usg;
	exit 1;
fi

# check user
whoami=`whoami`
if [ "${whoami}" != "root" ]
then
	echo "$0: must be root user.";
	exit 1;
fi

# where to work and where the image goes
TOP=../../..
RELTREEDIR=../rel;

# who owns the files...
OWNGRP=root.system

# package and fileset names
# processing either executables or documents...
PKGANDLVL=`./PKGNAM`.`./VERSLVL`.`./RELLVL`.`./MODLVL`.`./FXLVL`;
PKGNM=`./PKGNAM`;
Q=${PKGNM};
PRODUCT=`./PRODUCT`;
PKGINSTDIR=${TOP}/pkg.install;
if [ ${rtyp} = "E" ]
then
	FSETNAM=`./FSETNAM`;
else
	FSETNAM=`./FSETNAM`.doc;
fi
IMGSDIR=${RELTREEDIR}/imgs;
RELTREEROOT=${RELTREEDIR}/root/${FSETNAM};
CDDISTTREEDIR=${RELTREEDIR}/dist/CD-ROM;

# where we be
HERE=`pwd`

# file input during processing
if [ ${rtyp} = "E" ]
then
	AMBSRCS=AMBSRCS.exec
else
	AMBSRCS=AMBSRCS.doc
fi

# files output during processing

# prototype sources
MKINVENT=./mkinvent_PKG.sh;
FILEINVENT=./INVENT.FILES.${FSETNAM};
ROOTFILES=./FILESET.ROOT.${FSETNAM};
USRFILES=./FILESET.USR.${FSETNAM};
SHAREFILES=./FILETSET.SHARE.${FSETNAM};

# map of files in the build source
GRAPHOUT=${HERE}/SRCGRAPH.${FSETNAM};

# lists of package sources we're trying to fullfil.
#   SOURCES    are results of matching a package 
#              source obj with a build source obj.
#   NOSOURCES  are results of failing to match
#   AMBSOURCES are results of ambiguous matches.
#   NMSOURCES  is a digest of results from the match.
#
SOURCES=./SOURCES.${FSETNAM};
NOSOURCES=./NOSOURCES.${FSETNAM};
AMBSOURCES=./AMBSOURCES.${FSETNAM};
NMSOURCES=./NMSOURCES.${FSETNAM};
STUBBY=./ZEROLEN.${FSETNAM};

# files inventories 
ROOTAL=./ROOT.FILES.${FSETNAM}.al;
ROOTINVENT=./ROOT.FILES.${FSETNAM}.inventory;
USRAL=./USR.FILES.${FSETNAM}.al;
USRINVENT=./USR.FILES.${FSETNAM}.inventory;

# directories inventories
ROOTDIRSAL=./DIRS.ROOT.${FSETNAM}.al;
ROOTDIRSINVENT=./DIRS.ROOT.${FSETNAM}.inventory;
USRDIRSAL=./DIRS.USR.${FSETNAM}.al;
USRDIRSINVENT=./DIRS.USR.${FSETNAM}.inventory;

# symlinks inventories
ROOTSYMLSAL=./SYMLS.ROOT.${FSETNAM}.al;
ROOTSYMLSINVENT=./SYMLS.ROOT.${FSETNAM}.inventory;
USRSYMLSAL=./SYMLS.USR.${FSETNAM}.al;
USRSYMLSINVENT=./SYMLS.USR.${FSETNAM}.inventory;

# temps, pattern files
SEDCMD_MAPPART=./MAP.sed;
SEDCMD_RSLVPART=./RESOLV.sed;
SIZEINFO=./SIZEINFO.${FSETNAM};
ROOT_LIBLPP_TMPD=./ROOT.liblpp.a.d;
USR_LIBLPP_TMPD=./USR.liblpp.a.d;

# installation control files

# wrappers
INSTAL=./instal;
CLEANUP=./lpp.cleanup;
DEINSTAL=./lpp.deinstal;
REJECT=./lpp.reject;
UPDATE=./update;

# files to preserve 
ROOTCFGFILES=./ROOT.${FSETNAM}.cfgfiles
USRCFGFILES=./USR.${FSETNAM}.cfgfiles

# how to postinstall configure
# or preremove unconfigure ODM
CONFIG=./${FSETNAM}.config
UNCONFIG=./${FSETNAM}.unconfig_d
PRERM=./${FSETNAM}.pre_rm

# clean up...
if [ ${cleanonly} -eq 1 ]
then

echo ${bar}
echo "Cleaning up ${FSETNAM} release generation files."
echo ${bar}

CLEANUP="${FILEINVENT} ${ROOTFILES} ${USRFILES} ${SHAREFILES} \
         ${SEDCMD_MAPPART} ${GRAPHOUT} ${SOURCES} \
         ${SEDCMD_RSLVPART} ${AMBSOURCES} \
         ${SOURCES} ${NOSOURCES} ${AMBSOURCES} ${NMSOURCES} \
         ${ROOTDIRSINVENT} ${ROOTDIRSAL} ${USRDIRSINVENT} ${USRDIRSAL} \
         ${ROOTSYMLSINVENT} ${ROOTSYMLSAL} ${USRSYMLSINVENT} ${USRSYMLSAL} \
         ${ROOTAL} ${ROOTINVENT} ${USRAL} ${USRINVENT} \
         ${INSTAL} ${CLEANUP} ${DEINSTAL} ${REJECT} \
         ${UPDATE} ${CONFIG} ${UNCONFIG} ${SHAREAL} ${SHAREINVENT} \
         ${ROOTCFGFILES} ${USRCFGFILES} ${PRERM} \
         ${SIZEINFO} ${STUBBY} ${USR_LIBLPP_TMPD} ${ROOT_LIBLPP_TMPD}";

rm -rf ${CLEANUP};
exit 0;
fi

echo ${bar}
echo ""
echo "Generating release backup(8) image of ${FSETNAM} in ${IMGSDIR}"
echo ""
echo ${bar}


# chk whether there is cached data
if [ ${cleanonly} -eq 0 -a ${needinvfils} -eq 0 ]
then
	if [ ! -f ${SOURCES} ]
	then
		echo "Hmmm. Cannot find cached data. Use -f option";
		exit 1;
	fi
fi

# destroy any old release tree. don't molest images or cd-rom dist...
echo "Destroying any previous ${FSETNAM} release tree in ${RELTREEDIR}/root..."
rm -rf ${RELTREEROOT}
mkdir -p ${RELTREEROOT} ${IMGSDIR} ${CDDISTTREEDIR};

echo "Defining mapping func's...";
# applylist file names include patterned components.
# map them to fileset names, according to whether 
# they are components of the Root, Usr, or Share part 
# of the installation. 
#
# here is a good place to add ad-hoc filter for 
# file or directory names that don't apply in 
# the case of an AIX install.
#
# mapping of SYSV namespace roots to AIX roots
# applied below is as follows:
#
# %ROOTDIR% and %USRDIR% are treated specially. 
#
#      SYSV root                  AIX root
#--------------------------------------------------
# %PKGNM%             ->        %USRPART%/usr/
# %OPTDIR%            ->        %USRPART%/usr/
# %ROOTDIR%           ->        %ROOTPART%
# %USRDIR%            ->        %USRPART%
# %VAROPTDIR%         ->        %ROOTPART%/var/
# %ETCOPTDIR%         ->        %ROOTPART%/etc/
# %ETCINITDDIR%       ->        %ROOTPART%/etc/%%Q%%/init.d/
# %ETCRSDDIR%         ->        %ROOTPART%/etc/%%Q%%/rS.d/
# %ETCR3DDIR%         ->        %ROOTPART%/etc/%%Q%%/r3.d/
# %USRKERNELDRVDIR%   ->        %USRPART%/usr/lib/drivers/
# %USRSBINDIR%        ->        %USRPART%/usr/sbin/

# filtering: 
#   source files or directories that don't apply: 
#      (TBD)

# source some of the mapping definitions from mk/vars.gmk.
# in that file, VARDIR_START and VARDIR_STOP delimit AIX 
# definitions of the installation roots, used there in the
# PKGIFY macro. this little dependency is fragile, so please
# be tender here and with vars.gmk...
VARS_GMK=${TOP}/mk/vars.gmk
eval ` cat ${VARS_GMK} |\
	 sed -n -e '/%VARDIR_START%/,/%VARDIR_STOP%/p' |\
	 sed -e '/%VARDIR_START%/d' -e '/%VARDIR_STOP%/d' `

# make a sed(1) filter to implement mapping function
rm -f ${SEDCMD_MAPPART};
cat > ${SEDCMD_MAPPART} << EOSEDCMD 
\@%PKGNM%@s@@%USRPART%/usr@
\@/%ROOTDIR%@s@@%ROOTPART%@
\@/%USRDIR%@s@@%USRPART%@
\@%OPTDIR%@s@@%USRPART%/${OPTDIR}@
\@%VAROPTDIR%@s@@%ROOTPART%/${VAROPTDIR}@
\@%ETCOPTDIR%@s@@%ROOTPART%/${ETCOPTDIR}@
\@%ETCINITDDIR%@s@@%ROOTPART%/${ETCINITDDIR}@
\@%ETCRSDDIR%@s@@%ROOTPART%/${ETCRSDDIR}@
\@%ETCR3DDIR%@s@@%ROOTPART%/${ETCR3DDIR}@
\@%USRKERNELDRVDIR%@s@@%USRPART%/${USRKERNELDRVDIR}@
\@%USRSBINDIR%@s@@%USRPART%/${USRSBINDIR}@
EOSEDCMD

# use sed script for yet another mapping function
# this one maps roots to thier position in the
# release tree we are building.
rm -f ${SEDCMD_RSLVPART};
cat > ${SEDCMD_RSLVPART} << EOSEDCMD1
\@%ROOTPART%@s@@/usr/lpp/${FSETNAM}\/inst_root@
\@%USRPART%@s@@@
\@%SHAREPART%@s@@@
EOSEDCMD1

# rebrand defines
OEMNM=%%OEMNM%%;

if [ ${needinvfils} -eq 1 ]
then
	rm -f ${FILEINVENT} ${ROOTFILES} ${USRFILES} ${SHAREFILES};
	rm -f ${SOURCES} ${NOSOURCES} ${AMBSOURCES} ${NMSOURCES};
fi

# skip the next section if there is already
# a cache of inventory files to work from,
# as it expensive...
if [ ${needinvfils} -eq 1 ]
then

# apply filter/mapping
echo "Mapping fileset roots from SYSV to AIX namespaces..."
echo "Partitioning ${FSETNAM} fileset into Root, Usr, and Share parts..."

# make an inventory of regular files from a pkginst 
# manifest. directories and symlinks are handled
# specially later.
echo "Generating a files inventory for ${FSETNAM}..."
${MKINVENT} -r ${rtyp} -f A -t f -o -g -m 1> ${FILEINVENT} 2>&1;
stat=$?;
if [ ${stat} -ne 0 ]
then
	echo "${MKINVENT} failed. Status ${stat}. Bailing Out.";
	exit 1;
fi

# apply the mapping. 
# sort into  Root, Usr, and Share parts.
cat ${FILEINVENT} |\
sed -f ${SEDCMD_MAPPART} |\
while read pnm
do
	echo ${pnm} | egrep "(^%ROOTPART%)" 1> /dev/null 2>&1;
	if [ $? -eq 0 ]
	then
		echo ${pnm} 1>> ${ROOTFILES} 2>&1;
		continue;
	fi
	echo ${pnm} | egrep "(^%USRPART%)" 1> /dev/null 2>&1;
	if [ $? -eq 0 ]
	then
		echo ${pnm} 1>> ${USRFILES} 2>&1;
		continue;
	fi
	echo ${pnm} | egrep "(^%SHAREPART%)" 1> /dev/null 2>&1;
	if [ $? -eq 0 ]
	then
		echo ${pnm} 1> ${SHAREFILES} 2>&1;
		continue;
	fi
	echo $0: Cannot sort ${pnm}. Bailing out.;
	exit 1;
done


# find fileset sources
echo "Generating graph of ${FSETNAM} fileset sources..."
rm -f ${GRAPHOUT};
MKSRCS=./mksrcs.sh
${MKSRCS} ${TOP} 1> ${GRAPHOUT} 2>&1;
stat=$?
if [ ${stat} -ne 0 ]
then
	echo "$0: ${MKSRCS} returned ${stat}";
	exit 1;
fi

# sort fileset sources into lists according to 
# whether they are unambiguously determined,
# determined ambiguously, or not found.

# first try using fileset member basenames to find sources
echo "Comparing fileset sources graph and inventory..."
mcnt=0;
nmcnt=0;
ambcnt=0;
for part in ${ROOTFILES} ${USRFILES} ${SHAREFILES}
do
	if [ ! -f ${part} ]
	then
		echo "Comparing ${part}. Fileset empty.";
		continue;
	fi
	echo "Comparing ${part} fileset sources:";
	cat ${part} |\
	while read inv own grp mod
	do
		ifnm=`basename ${inv}`;
		idnm=`dirname ${inv}`;
		# count occurences in the graph, 
		# one-to-one is what we're hoping for...
		cnt=0;
		svdnm="";
		svfnm="";
		cat ${GRAPHOUT} |\
		egrep "(${ifnm}\$)" |\
		while read grph
		do
			gfnm=`basename ${grph}`;
			gdnm=`dirname ${grph}`;
			if [ "${gfnm}" = "${ifnm}" ]
			then
				cnt=`expr ${cnt} + 1`;
				svfnm=${gfnm};
				svdnm=${gdnm};
			fi
		done
		if [ ${cnt} -eq 1 ]
		then
			echo "${svdnm}/${svfnm} ${inv} ${own} ${grp} ${mod}" 1>> ${SOURCES} 2>&1;
			mcnt=`expr ${mcnt} + 1`;
		else
			if [ ${cnt} -eq 0 ]
			then
				rsn="No Match:";
				nmcnt=`expr ${nmcnt} + 1`;
				echo "${inv} ${own} ${grp} ${mod}" 1>> ${NMSOURCES} 2>&1;
			else
				rsn="Ambiguous (${ifnm}):";
				ambcnt=`expr ${ambcnt} + 1`;
				echo "${inv} ${own} ${grp} ${mod}" 1>> ${AMBSOURCES} 2>&1;
			fi
			echo "${rsn} ${inv} ${own} ${grp} ${mod}" 1>> ${NOSOURCES} 2>&1;
		fi
	done
	echo "Sources: Matched: ${mcnt}. Not Matched: ${nmcnt}. Ambiguous: ${ambcnt}."
done

rambcnt=0;
# attempt to resolve ambiguities, first look at directory and parent directory
if [ ${ambcnt} -gt 0 ]
then
	echo "Attempting to Resolve Amibuous Sources:"
	echo ${bar}
	cat ${AMBSOURCES}
	echo ${bar}
	cat ${AMBSOURCES} |\
	while read ambs own grp mod
	do
		fnm=`basename ${ambs}`;
		dnm=`dirname ${ambs}`;
		pdnm=`basename ${dnm}`;

		ddnm=`dirname ${dnm}`;
		pddnm=`basename ${ddnm}`;

		cnt=0;
		cnt1=0;
		svdnm="";
		svfnm="";
		cat ${GRAPHOUT} |\
		egrep "(${fnm}\$)" |\
		while read grph
		do
			gfnm=`basename ${grph}`;
			gdnm=`dirname ${grph}`;
			gpdnm=`basename ${gdnm}`;
			if [ "${gpdnm}/${gfnm}" = "${pdnm}/${fnm}" ]
			then
				cnt=`expr ${cnt} + 1`;
				svfnm=${gfnm};
				svdnm=${gdnm};
			fi
		done
		# last ditch attempt to resolve ambiguous file names...
		if [ ${cnt} -ne 1 ]
		then
			cnt=0;
			cnt1=0;
			if [ -f ${AMBSRCS} ]
			then 
				# ${AMBSRCS} is a table of 
				# mappings for which previous methods
				# aren't working. search this file
				# rather than the graph...
				cat ${AMBSRCS} |\
				egrep "(${fnm}\$)" |\
				sed -e '\@%OEMNAME%@ s@@%%OEMNM%%@g' |\
				while read grph
				do
					gfnm=`basename ${grph}`;

					gdnm=`dirname ${grph}`;
					gpdnm=`basename ${gdnm}`;

					gddnm=`dirname ${gdnm}`;
					gdddnm=`basename ${gddnm}`;

					if [ "${gdddnm}/${gpdnm}/${gfnm}" = "${pddnm}/${pdnm}/${fnm}" ]
					then
						cnt=`expr ${cnt} + 1`;
						svfnm=${gfnm};
						svdnm=${gdnm};
					elif [ "${gpdnm}/${gfnm}" = "${pdnm}/${fnm}" ]
					then
						cnt1=`expr ${cnt} + 1`;
						svfnm=${gfnm};
						svdnm=${gdnm};
					fi
				done
			fi
		fi
		if [ ${cnt} -eq 1 -o ${cnt1} -eq 1  ]
		then
			echo "Resolved Ambiguous Source: ${ambs}"
			echo "${svdnm}/${svfnm} ${ambs} ${own} ${grp} ${mod}" 1>> ${SOURCES} 2>&1;
			mcnt=`expr ${mcnt} + 1`;
			rambcnt=`expr ${rambcnt} + 1 `
		else
			echo "WARNING: Unresolved Ambiguous Source: ${ambs}"
		fi
	done
fi


echo "Resolved ${rambcnt} of ${ambcnt} Ambiguous Sources."
if [ ${rambcnt} -ne ${ambcnt} ]
then
	echo "WARNING: `expr ${ambcnt} - ${rambcnt}` Unresolved Ambiguous Sources"
fi
echo ${bar}

# attempt to resolve matchless sources
rnmcnt=0;
if [ ${nmcnt} -gt 0 ]
then
	echo "Attempting to Resolve Matchless Sources:";
	echo ${bar}
	cat ${NMSOURCES};
	echo ${bar}
fi
echo "Resolved ${rnmcnt} of ${nmcnt} Matchless Sources.";
if [ ${rnmcnt} -ne ${nmcnt} ]
then
	echo "WARNING: `expr ${nmcnt} - ${rnmcnt}` Unresolved Matchless Sources";
fi
echo ${bar}

#
#
fi # (end of if [ ${needinvfils} -eq 1 ])
#
#

# this is where we start if prefered and there is a
# cache of trustworthy source inventories built on
# a previous run ...

# populate a release tree...
echo "Populating release tree...";
CPCMD=/usr/bin/cp;
CLASS="apply,inventory,${FSETNAM}"

# move files around
cat ${SOURCES} |\
while read src dst own grp mod
do
	rdst=${RELTREEROOT}/`echo ${dst} | sed -f ${SEDCMD_RSLVPART}`;
	drdst=`dirname ${rdst}`;
	cpcmd="${CPCMD} ${TOP}/${src} ${rdst}";
	pavecmd="mkdir -p ${drdst}";
	${pavecmd};
	${cpcmd};
done

# ROOT_LIBLPPD and USR_LIBLPPD always...
ROOT_LIBLPPD=${RELTREEROOT}/usr/lpp/${FSETNAM}/inst_root;
USR_LIBLPPD=${RELTREEROOT}/usr/lpp/${FSETNAM};
rm -rf ${USR_LIBLPP_TMPD} ${ROOT_LIBLPP_TMPD};
mkdir -p ${USR_LIBLPPD} ${USR_LIBLPP_TMPD} ${ROOT_LIBLPPD} ${ROOT_LIBLPP_TMPD}

# make fileset installation control files

# directories 
rm -f ${ROOTDIRSAL} ${ROOTDIRSINVENT} ${USRDIRSAL} ${USRDIRSINVENT}
touch ${ROOTDIRSAL} ${ROOTDIRSINVENT} ${USRDIRSAL} ${USRDIRSINVENT}

# symlinks 
rm -f ${ROOTSYMLSAL} ${ROOTSYMLSINVENT} ${USRSYMLSAL} ${USRSYMLSINVENT}
touch ${ROOTSYMLSAL} ${ROOTSYMLSINVENT} ${USRSYMLSAL} ${USRSYMLSINVENT}

# root part directories and symlinks

# make a directories inventory file.
echo "Generating a root fileset directories inventory..."
${MKINVENT} -r ${rtyp} -f I -t d -s r | \
	sed -f ${SEDCMD_MAPPART} |\
	sed -f ${SEDCMD_RSLVPART} 1>> ${ROOTDIRSINVENT} 2>&1;
# make a directories applylist file.
${MKINVENT} -r ${rtyp} -f A -t d -s r | \
        sed -f ${SEDCMD_MAPPART} |\
        sed -f ${SEDCMD_RSLVPART}|\
	while read dir
	do
		echo .${dir} 1>> ${ROOTDIRSAL} 2>&1;
	done

# make a symlinks inventory file.
echo "Generating a root fileset symlinks inventory..."
${MKINVENT} -r ${rtyp} -f I -t l -s r | 
	sed -f ${SEDCMD_MAPPART} |\
	sed -f ${SEDCMD_RSLVPART} |\
	sed -e '\@=.*inst_root@s@@= @' 1>> ${ROOTSYMLSINVENT} 2>&1;
# make a symlinks applylist file.
${MKINVENT} -r ${rtyp} -f A -t l -s r | 
	sed -f ${SEDCMD_MAPPART} |\
	sed -f ${SEDCMD_RSLVPART}|\
	while read syml
	do
		echo .${syml} 1>> ${ROOTSYMLSAL} 2>&1;
	done

# make these directories
cat ${ROOTDIRSAL} |\
while read dst 
do
	rdst=${RELTREEROOT}/${dst};
	pavecmd="mkdir -p ${rdst}";
	${pavecmd};
done

# make these symlinks
lookpath=1
foundpath=0;
looktgt=0
foundtgt=0;
cat ${ROOTSYMLSINVENT} |\
while read path eq val
do
	
	echo ${path} | egrep "(^/)" 1> /dev/null 2>&1
	if [ ${lookpath} -eq 1 ]
	then
		echo ${path} | egrep "(^/)" 1> /dev/null 2>&1
		if [ $? -eq 0 ]
		then
			foundpath=1;
			pathfound=`echo ${path} | sed -e '\@:$@s@@@'`
			lookpath=0;
			looktgt=1;
		fi
		continue;
	fi
	if [ ${looktgt} -eq 1 ]
	then
		if [ "${path}" != "target" ]
		then
			continue;
		else
			foundtgt=1;
			tgtfound=`echo ${val} | sed -e '\@/.*inst_root@s@@@'`
			lookpath=1;
			looktgt=0;
		fi
	fi
	if [ ${foundpath} -eq 1 -a ${foundtgt} -eq 1 ]
	then
		dpathfound=`dirname ${RELTREEROOT}/${pathfound}`;
		pavecmd="mkdir -p ${dpathfound}";
		${pavecmd};
		ln -s ${tgtfound} ${RELTREEROOT}/${pathfound};
	fi
done

# usr part
# make a directories inventory file.
echo "Generating a usr fileset directories inventory..."
${MKINVENT} -r ${rtyp} -f I -t d -s u | \
	sed -f ${SEDCMD_MAPPART} |\
	sed -f ${SEDCMD_RSLVPART} 1>> ${USRDIRSINVENT} 2>&1;
# make a directories applylist file.
${MKINVENT} -r ${rtyp} -f A -t d -s u | \
	sed -f ${SEDCMD_MAPPART} |\
	sed -f ${SEDCMD_RSLVPART}|\
	while read dir
	do
		echo .${dir} 1>> ${USRDIRSAL} 2>&1;
	done

# make a symlinks inventory file.
echo "Generating a usr fileset symlinks inventory..."
${MKINVENT} -r ${rtyp} -f I -t l -s u | 
	sed -f ${SEDCMD_MAPPART} |\
	sed -f ${SEDCMD_RSLVPART} 1>> ${USRSYMLSINVENT} 2>&1;
# make a symlinks applylist file.
${MKINVENT} -r ${rtyp} -f A -t l -s u | 
	sed -f ${SEDCMD_MAPPART} |\
	sed -f ${SEDCMD_RSLVPART}|\
	while read syml
	do
		echo .${syml} 1>> ${USRSYMLSAL} 2>&1;
	done

# make these directories
cat ${USRDIRSAL} |\
while read dst 
do
	rdst=${RELTREEROOT}/${dst};
	pavecmd="mkdir -p ${rdst}";
	${pavecmd};
done

# make these symlinks
lookpath=1
foundpath=0;
looktgt=0
foundtgt=0;
cat ${USRSYMLSINVENT} |\
while read path eq val
do
	
	echo ${path} | egrep "(^/)" 1> /dev/null 2>&1
	if [ ${lookpath} -eq 1 ]
	then
		echo ${path} | egrep "(^/)" 1> /dev/null 2>&1
		if [ $? -eq 0 ]
		then
			foundpath=1;
			pathfound=`echo ${path} | sed -e '\@:$@s@@@'`
			lookpath=0;
			looktgt=1;
		fi
		continue;
	fi
	if [ ${looktgt} -eq 1 ]
	then
		if [ "${path}" != "target" ]
		then
			continue;
		else
			foundtgt=1;
			tgtfound=`echo ${val} | sed -e '\@/.*inst_root@s@@@'`
			lookpath=1;
			looktgt=0;
		fi
	fi
	if [ ${foundpath} -eq 1 -a ${foundtgt} -eq 1 ]
	then
		dpathfound=`dirname ${RELTREEROOT}/${pathfound}`;
		pavecmd="mkdir -p ${dpathfound}";
		${pavecmd};
		ln -s ${tgtfound} ${RELTREEROOT}/${pathfound};
	fi
done

# while we're doing the inventory, 
# look for suspicious stuff like zero length files...
rm -f ${STUBBY};

# root part .al and .inventory files...
if [ -f ${ROOTFILES} ]
then
	echo "Making liblpp.a(${ROOTINVENT},${ROOTAL})...";
	rm -f ${ROOTINVENT} ${ROOTAL};
	#
	# prepend directories and symlinks applylist
	# and inventories to the root fileset applylist 
	# and inventories
	#
	cat ${SOURCES} |\
	egrep "(%ROOTPART%)" |\
	while read src dst own grp mod
	do
		# root inventory file needs absolute restore path
		# need to resolve this one special...
		#rdst=`echo ${dst} | sed -f ${SEDCMD_RSLVPART}`;
		rdst=`echo ${dst} | sed -e '/%ROOTPART%/s///'`;
		# can't make sysck happy with size attr 
		# neither blocks or bytes seem to gratify.
		# size=`du -s ${TOP}/${src} | awk '{print $1}'`;
		size=`ls -l ${TOP}/${src} | awk '{print $5}'`;
		if [ ${size} -eq 0 ]
		then
			echo "${TOP}/${src}: size ${size}" 1>> ${STUBBY} 2>&1;
		fi
		sum=`sum ${TOP}/${src} | awk '{printf "%d %d", $1, $2}'`;
		echo ".${rdst}"           1>> ${ROOTAL} 2>&1;
		echo "${rdst}:"           1>> ${ROOTINVENT} 2>&1;
		echo "  type = FILE"        1>> ${ROOTINVENT} 2>&1;
		echo "  owner = ${own}"     1>> ${ROOTINVENT} 2>&1;
		echo "  group = ${grp}"     1>> ${ROOTINVENT} 2>&1;
		echo "  mode = ${mod}"      1>> ${ROOTINVENT} 2>&1;
		# source attr must be abs pathname...
		# echo "  source = ${src}"    1>> ${ROOTINVENT} 2>&1;
		# sysck barfs given real size attribute,
		# giving VOLATILE elides the size check
		# echo "  size = ${size}"     1>> ${ROOTINVENT} 2>&1;
		echo "  size = VOLATILE"     1>> ${ROOTINVENT} 2>&1;
		echo "  sum = \"${sum} \""  1>> ${ROOTINVENT} 2>&1;
		echo "  class = ${CLASS}" 1>> ${ROOTINVENT} 2>&1;
		echo "" 1>> ${ROOTINVENT} 2>&1;
	done
	cat ${ROOTDIRSAL} | sed -e '\@/.*inst_root@s@@@' >> ${ROOTAL};
	cat ${ROOTSYMLSAL} | sed -e '\@/.*inst_root@s@@@'>> ${ROOTAL};
	cat ${ROOTDIRSINVENT} | sed -e '\@/.*inst_root@s@@@' >> ${ROOTINVENT};
	cat ${ROOTSYMLSINVENT}| sed -e '\@^/.*inst_root@s@@@'  >> ${ROOTINVENT};
fi

# usr part files
if [ -f ${USRFILES} ]
then
	echo "Making liblpp.a(${USRINVENT},${USRAL})...";
	rm -f ${USRINVENT} ${USRAL};
	touch ${USRINVENT} ${USRAL};
	#
	# prepend directories and symlinks applylist
	# and inventories to the usr fileset applylist 
	# and inventories
	#
	cat ${SOURCES} |\
	egrep "(%USRPART%|%ROOTPART%)" |\
	while read src dst own grp mod
	do
		rdst=`echo ${dst} | sed -f ${SEDCMD_RSLVPART}`;
		# can't make sysck happy with size attr 
		# neither blocks or bytes seem to gratify.
		# size=`du -s ${TOP}/${src} | awk '{print $1}'`;
		size=`ls -l ${TOP}/${src} | awk '{print $5}'`;
		if [ ${size} -eq 0 ]
		then
			echo "${TOP}/${src}: size ${size}" 1>> ${STUBBY} 2>&1;
		fi
		sum=`sum ${TOP}/${src} | awk '{printf "%d %d", $1, $2}'`;
		echo ".${rdst}"           1>> ${USRAL} 2>&1;
		echo "${rdst}:"           1>> ${USRINVENT} 2>&1;
		echo "  type = FILE"      1>> ${USRINVENT} 2>&1;
		echo "  owner = ${own}"   1>> ${USRINVENT} 2>&1;
		echo "  group = ${grp}"   1>> ${USRINVENT} 2>&1;
		echo "  mode = ${mod}"    1>> ${USRINVENT} 2>&1;
		# source attr must be abs pathname...
		# echo "  source = ${src}"    1>> ${USRINVENT} 2>&1;
		# sysck barfs given real size attribute,
		# giving VOLATILE elides the size check
		# echo "  size = ${size}"     1>> ${USRINVENT} 2>&1;
		echo "  size = VOLATILE"   1>> ${USRINVENT} 2>&1;
		echo "  sum =  \"${sum} \""     1>> ${USRINVENT} 2>&1;
		echo "  class = ${CLASS}" 1>> ${USRINVENT} 2>&1;
		echo "" 1>> ${USRINVENT} 2>&1;
	done
	cat ${USRDIRSAL} >> ${USRAL};
	cat ${USRSYMLSAL} >> ${USRAL};
	cat ${ROOTDIRSAL} >> ${USRAL};
	cat ${ROOTSYMLSAL} >> ${USRAL};
	cat ${USRDIRSINVENT} >> ${USRINVENT};
	cat ${USRSYMLSINVENT} >> ${USRINVENT};
	cat ${ROOTDIRSINVENT} >> ${USRINVENT};
	cat ${ROOTSYMLSINVENT} >> ${USRINVENT};
fi

# share part files
if [ -f ${SHAREFILES} ]
then
	SHAREAL=./SHARE.FILES.${FSETNAM}.al;
	SHAREINVENT=./SHARE.FILES.${FSETNAM}.inventory;
	echo "Making liblpp.a(${SHAREINVENT},${SHAREAL})..."
	rm -f ${SHAREINVENT} ${SHAREAL};
	touch ${SHAREINVENT} ${SHAREAL};
	cat ${SOURCES} |\
	egrep "(%SHAREPART%)" |\
	while read src dst own grp mod
	do
		rdst=`echo ${dst} | sed -f ${SEDCMD_RSLVPART}`
		# can't make sysck happy with size attr 
		# neither blocks or bytes seem to gratify.
		# size=`du -s ${TOP}/${src} | awk '{print $1}'`;
		size=`ls -l ${TOP}/${src} | awk '{print $5}'`;
		if [ ${size} -eq 0 ]
		then
			echo "${TOP}/${src}: size ${size}" 1>> ${STUBBY} 2>&1;
		fi
		sum=`sum ${TOP}/${src} | awk '{printf "%d %d", $1, $2}'`;
		echo ".${rdst}"           1>> ${SHAREAL} 2>&1;
		echo "${rdst}:"           1>> ${SHAREINVENT} 2>&1;
		echo "  type = FILE"      1>> ${SHAREINVENT} 2>&1;
		echo "  owner = ${own}"   1>> ${SHAREINVENT} 2>&1;
		echo "  group = ${grp}"   1>> ${SHAREINVENT} 2>&1;
		echo "  mode = ${mod}"    1>> ${SHAREINVENT} 2>&1;
		# source attr must be abs pathname...
		# echo "  source = ${src}"    1>> ${SHAREINVENT} 2>&1;
		# sysck barfs given real size attribute,
		# giving VOLATILE elides the size check
		# echo "  size = ${size}"     1>> ${SHAREINVENT} 2>&1;
		echo "  size = VOLATILE"   1>> ${SHAREINVENT} 2>&1;
		echo "  sum = \"${sum} \""     1>> ${SHAREINVENT} 2>&1;
		echo "  class = ${CLASS}" 1>> ${SHAREINVENT} 2>&1;
		echo "" 1>> ${SHAREINVENT} 2>&1;
	done
fi

# gripe about stubby files...
if [ -f ${STUBBY} ]
then
	echo ${bar}
	echo "WARNING: Zero length files in release tree:";
	cat ${STUBBY};
	echo ${bar}
fi

# generate lpp_name install control file
# need to know content type.
mklpp_name_args="-r ${rtyp} -p I"
if [ -f ${ROOTFILES} ]
then
	mklpp_name_args="${mklpp_name_args} -c R"
fi
if [ -f ${USRFILES} ]
then
	mklpp_name_args="${mklpp_name_args} -c U"
fi
if [ -f ${SHAREFILES} ]
then
	mklpp_name_args="${mklpp_name_args} -c S"
fi

MKLPP_NAME=./mklpp_name.sh;
echo "Generating ${RELTREEROOT}/lpp_name...";
# echo "${MKLPP_NAME} ${mklpp_name_args}"
${MKLPP_NAME} ${mklpp_name_args}
mv ./lpp_name ${RELTREEROOT};

# populate ROOT.liblpp.a install control library members
if [ -f ${ROOTFILES} ]
then
	echo "Archiving ${ROOT_LIBLPPD}/${FSETNAM}.al";
	echo "Archiving ${ROOT_LIBLPPD}/${FSETNAM}.inventory";
	cp ${ROOTINVENT} ${ROOT_LIBLPP_TMPD}/${FSETNAM}.inventory;
	cp ${ROOTAL} ${ROOT_LIBLPP_TMPD}/${FSETNAM}.al;
	# get ROOT part sizes
	echo "Archiving ${ROOT_LIBLPP_TMPD}/${FSETNAM}.size";
	grep inst_root ${SIZEINFO} | \
	sed -e '/.*\/inst_root/ s///' 1> ${ROOT_LIBLPP_TMPD}/${FSETNAM}.size 2>&1;
fi

# populate USR.liblpp.a install control library members
if [ -f ${USRFILES} ]
then
	echo "Archiving ${USR_LIBLPPD}/${FSETNAM}.al";
	echo "Archiving ${USR_LIBLPPD}/${FSETNAM}.inventory";
	cp ${USRINVENT} ${USR_LIBLPP_TMPD}/${FSETNAM}.inventory;
	cp ${USRAL} ${USR_LIBLPP_TMPD}/${FSETNAM}.al;
	# get USR part sizes
	echo "Archiving ${USR_LIBLPP_TMPD}/${FSETNAM}.size";
	grep -v inst_root ${SIZEINFO} 1> ${USR_LIBLPP_TMPD}/${FSETNAM}.size 2>&1;
fi

# installp control script files. 
# these be 'sposed to be optional, since installp ought to look 
# for defaults if the package doesn't provide them.
# yet, in our case it just gives up, so here we're helping it
# along by providing scripts that wrap the defaults. sigh.
echo "Archiving installp scripts...";

# root part cfg files to save
cat > ${ROOTCFGFILES} << EOF
./etc/%%Q%%/lib/FTD.lic preserve
EOF

# usr part cfg files to save
cat > ${USRCFGFILES} << EOF
./usr/lib/drivers/%%Q%%.conf	preserve
EOF

# how to prepare for a forced base installation
cat > ${PRERM} << EOF
#!/bin/sh

# prior to removing any of the fileset,
# save off user config files so that 
# they may be restored upon reinstallation.

FSETNAM=${FSETNAM}

%%Q%%INFO=/${FTDBINDIR}/%%Q%%info

# check running
if [ -f \${%%Q%%INFO} ]
then
	\${%%Q%%INFO} 1> /dev/null 2>&1;
	if [ \$? -eq 0 ]
	then
		echo "\$0: cannot install. (%%Q%% driver is started)."
		exit 1;
	fi
fi

# where to save configuration files
SVCFG=/${VAROPTDIR}/%%Q%%/${PKGANDLVL};
rm -rf \${SVCFG};
mkdir \${SVCFG};

# include any driver configuration file,
# any found logical group configurations,
# any found license files
# any found shell scripts
# and any found log files
DRVCONFDIR=/usr/lib/drivers;
LGCONFDIR=/etc/%%Q%%/lib;
LOGDIR=/var/%%Q%%;

# files to save
SVF=\`echo \${DRVCONFDIR}/%%Q%%.conf\\
          \${LGCONFDIR}/*.lic \\
          \${LGCONFDIR}/p*.cfg \\
          \${LGCONFDIR}/s*.cfg \\
          \${LGCONFDIR}/*.sh \\
          \${LOGDIR}/*.log\`

filescnt=0;
firstmem=0;
echo \${SVF} |\\
awk '{ for ( i = 1 ; i <= NF ; i = i + 1) {printf "%s\n", \$i} }' |\\
while read svfil
do

	if [ -f \${svfil} ]
	then
		cp \${svfil} \${SVCFG};
		filescnt=\`expr \${filescnt} + 1\`
	fi

done

# list saved config files
if [ \${filescnt} -gt 0 ]
then
	echo "configuration files saved in \${SVCFG}:"
	ls \${SVCFG}
else
	echo "no configuration files saved, new install."
fi

exit 0;
EOF

# build/package installp control script files
# one for each of root and usr parts...

# make instal script
cat > ${INSTAL} << EOF
#!/bin/sh

# use default installp instal 

/usr/lib/instl/instal \$*
opstat=\$?
exit \${opstat}
EOF

# make cleanup script
cat > ${CLEANUP} << EOF
#!/bin/sh

# use default installp cleanup 

echo "INUTEMPDIR: $INUTEMPDIR"

/usr/lib/instl/cleanup \$*
opstat=\$?
exit \${opstat}
EOF

# make deinstal script
cat > ${DEINSTAL} << EOF
#!/bin/sh

# use default installp deinstal 

/usr/lib/instl/deinstal \$*
opstat=\$?
exit \${opstat}
EOF

# make reject script
cat > ${REJECT} << EOF
#!/bin/sh
# use default installp reject

/usr/lib/instl/reject \$*
opstat=\$?
exit \${opstat}
EOF

if [ ${rtyp} = "E" ]
then
# make config script
cat > ${CONFIG} << EOF
#!/bin/sh

# logging
logf=/var/${PRODUCT}/${PRODUCT}error.log;
datestr=\`date "+%Y/%m/%d %T"\`;
errhdr="[\${datestr}] ${PRODUCT}: ";

errmsg=\`echo "\${errhdr}\" "\$0: ${PRODUCT} ODM PdDv and CuDv configs."\`;
echo \${errmsg};


# whether any driver instance is in Available state
/usr/sbin/lsdev -C -c ${PRODUCT}_class |\
   egrep "(Available)" 1> /dev/null 2>&1
if [ \$? -eq 0 ]
then
	# unconfigure it
	/usr/lib/methods/ucfg${PRODUCT} -l ${PRODUCT}0
	if [ \$? -ne 0 ]
	then
		errmsg=\`echo "\${errhdr}\" "\$0: ucfg${PRODUCT} failed."\`;
		echo \${errmsg};
		echo \${errmsg} 1>> \${logf} 2>&1 
		exit 2;
	fi
fi

# whether any driver instance is defined
/usr/sbin/lsdev -C -c ${PRODUCT}_class |\
  egrep "(Defined)" 1> /dev/null 2>&1
if [ \$? -eq 0 ]
then
	# undefine it
	/usr/lib/methods/udef${PRODUCT} -l ${PRODUCT}0
	if [ \$? -ne 0 ]
	then
		errmsg=\`echo "\${errhdr}\" "\$0: udef${PRODUCT} failed."\`;
		echo \${errmsg};
		echo \${errmsg} 1>> \${logf} 2>&1 
		exit 2;
	fi
fi

# kill any ODM record of the driver
/usr/lib/methods/${PRODUCT}.delete

# reload ODM records
/bin/odmadd /usr/lib/methods/${PRODUCT}.add 1> /dev/null 2>&1;

# whether loaded
/usr/sbin/lsdev -P -c ${PRODUCT}_class |\
   egrep "(_class)" 1> /dev/null 2>&1
if [ \$? -ne 0 ]
then
	errmsg=\`echo "\${errhdr}\" "\$0: ODM PdDv definition failed."\`;
	echo \${errmsg};
	echo \${errmsg} 1>> \${logf} 2>&1 
	exit 3;
fi

# assert CuDv undefined
/usr/sbin/lsdev -C -c ${PRODUCT}_class |\
  egrep "(Defined)" 1> /dev/null 2>&1
if [ \$? -eq 0 ]
then
	errmsg=\`echo "\${errhdr}\" "\$0: ODM CuDv definition failed."\`;
	echo \${errmsg};
	echo \${errmsg} 1>> \${logf} 2>&1 
	exit 4;
fi

# define CuDv
/usr/lib/methods/def%%Q%% -c ${PRODUCT}_class -s ${PRODUCT}_subclass -t ${PRODUCT} 

# assert CuDv defined
/usr/sbin/lsdev -C -c ${PRODUCT}_class |\
 egrep "(Defined)" 1> /dev/null 2>&1
if [ \$? -ne 0 ]
then
	errmsg=\`echo "\${errhdr}\" "\$0: ODM CuDv definition failed."\`;
	echo \${errmsg};
	echo \${errmsg} 1>> \${logf} 2>&1 
	exit 5;
fi

# edit the system boot RC file, (also /etc/services)
/etc/%%Q%%/init.d/%%Q%%-rcedit
if [ \$? -ne 0 ]
then
	errmsg=\`echo "\${errhdr}\" "\$0: Customize /etc/rc failed."\`;
	echo \${errmsg};
	echo \${errmsg} 1>> \${logf} 2>&1 
	exit 6;
fi

# expect configtool to be run 
# immediately after this, start daemons.
/etc/%%Q%%/init.d/%%Q%%-startdaemons start
if [ \$? -ne 0 ]
then
	errmsg=\`echo "\${errhdr}\" "\$0: %%Q%%-startdaemons failed."\`;
	echo \${errmsg};
	echo \${errmsg} 1>> \${logf} 2>&1 
	exit 7;
fi

# all's well...
errmsg=\`echo "\${errhdr}\" "\$0: Driver ODM PdDv and CuDv defined. "\`;
echo \${errmsg};
echo \${errmsg} 1>> \${logf} 2>&1 
errmsg=\`echo "\${errhdr}\" "\$0: System boot file /etc/rc customized. "\`;
echo \${errmsg};
echo \${errmsg} 1>> \${logf} 2>&1 
errmsg=\`echo "\${errhdr}\" "\$0: %%Q%% daemons started."\`;
echo \${errmsg};
echo \${errmsg} 1>> \${logf} 2>&1 
exit 0;

EOF

# make unconfigure script
cat > ${UNCONFIG} << EOF
#!/bin/sh

# logging
logf=/var/${PRODUCT}/${PRODUCT}error.log;
datestr=\`date "+%Y/%m/%d %T"\`;
errhdr="[\${datestr}] ${PRODUCT}: ";


%%Q%%INFO=/${FTDBINDIR}/%%Q%%info

# check running
if [ -f \${%%Q%%INFO} ]
then
	\${%%Q%%INFO} 1> /dev/null 2>&1;
	if [ \$? -eq 0 ]
	then
	  echo "\$0: cannot unconfigure. (%%Q%% driver is started)."
	  exit 1;
	fi
fi

errmsg=\`echo "\${errhdr}\" "\$0: ${PRODUCT} ODM PdDv and CuDv configs."\`;
echo \${errmsg};

# whether any driver instance is in Available state
/usr/sbin/lsdev -C -c ${PRODUCT}_class |\
   egrep "(Available)" 1> /dev/null 2>&1
if [ \$? -eq 0 ]
then
	# unconfigure it
	/usr/lib/methods/ucfg${PRODUCT} -l ${PRODUCT}0
	if [ \$? -ne 0 ]
	then
		errmsg=\`echo "\${errhdr}\" "\$0: ucfg${PRODUCT} failed."\`;
		echo \${errmsg};
		echo \${errmsg} 1>> \${logf} 2>&1 
		exit 2;
	fi
fi

# whether any driver instance is defined
/usr/sbin/lsdev -C -c ${PRODUCT}_class |\
  egrep "(Defined)" 1> /dev/null 2>&1
if [ \$? -eq 0 ]
then
	# undefine it
	/usr/lib/methods/udef${PRODUCT} -l ${PRODUCT}0
	if [ \$? -ne 0 ]
	then
		errmsg=\`echo "\${errhdr}\" "\$0: udef${PRODUCT} failed."\`;
		echo \${errmsg};
		echo \${errmsg} 1>> \${logf} 2>&1 
		exit 2;
	fi
fi

# kill any ODM record of the driver
/usr/lib/methods/${PRODUCT}.delete

# clean up boot record of our devices
savebase -v

# assert PdDv undefined
/usr/sbin/lsdev -P -c ${PRODUCT}_class |\
   egrep "(_class)" 1> /dev/null 2>&1
if [ \$? -eq 0 ]
then
	errmsg=\`echo "\${errhdr}\" "\$0: ODM PdDv undefinition failed."\`;
	echo \${errmsg};
	echo \${errmsg} 1>> \${logf} 2>&1 
	exit 3;
fi

# assert CuDv undefined
/usr/sbin/lsdev -C -c ${PRODUCT}_class |\
  egrep "(Defined|Available)" 1> /dev/null 2>&1
if [ \$? -eq 0 ]
then
	errmsg=\`echo "\${errhdr}\" "\$0: ODM CuDv undefinition failed."\`;
	echo \${errmsg};
	echo \${errmsg} 1>> \${logf} 2>&1 
	exit 4;
fi

# uninstall the driver start hook, (also /etc/services).
/etc/%%Q%%/init.d/%%Q%%-rcedit -u

# all's well...
errmsg=\`echo "\${errhdr}\" "\$0: Driver ODM PdDv and CuDv undefine succeeded. "\`;
echo \${errmsg};
echo \${errmsg} 1>> \${logf} 2>&1 
exit 0;
EOF
fi

# make update script
cat > ${UPDATE} << EOF
#!/bin/sh
# use default installp update

/usr/lib/instl/update \$*
opstat=\$?
exit \${opstat}
EOF

# make install control library archives
# root archive
if [ -f ${ROOTFILES} ]
then
	cp ${CLEANUP} ${ROOT_LIBLPP_TMPD}/${CLEANUP};
	cp ${DEINSTAL} ${ROOT_LIBLPP_TMPD}/${DEINSTAL};
	cp ${REJECT} ${ROOT_LIBLPP_TMPD}/${REJECT};
	cp ${INSTAL} ${ROOT_LIBLPP_TMPD}/${INSTAL};
	cp ${UPDATE} ${ROOT_LIBLPP_TMPD}/${UPDATE};
	chmod 544 ${ROOT_LIBLPP_TMPD}/${CLEANUP};
	chmod 544 ${ROOT_LIBLPP_TMPD}/${DEINSTAL};
	chmod 544 ${ROOT_LIBLPP_TMPD}/${REJECT};
	chmod 544 ${ROOT_LIBLPP_TMPD}/${INSTAL};
	chmod 544 ${ROOT_LIBLPP_TMPD}/${UPDATE};
	if [ ${rtyp} = "E" ]
	then
		cp ${CONFIG} ${ROOT_LIBLPP_TMPD}/${CONFIG};
		cp ${UNCONFIG} ${ROOT_LIBLPP_TMPD}/${UNCONFIG};
		cp ${PRERM} ${ROOT_LIBLPP_TMPD}/${PRERM};
		cp ${ROOTCFGFILES} ${ROOT_LIBLPP_TMPD}/${FSETNAM}.cfgfiles;
		chmod 544 ${ROOT_LIBLPP_TMPD}/${CONFIG};
		chmod 544 ${ROOT_LIBLPP_TMPD}/${UNCONFIG};
		chmod 544 ${ROOT_LIBLPP_TMPD}/${PRERM};
		chmod 544 ${ROOT_LIBLPP_TMPD}/${FSETNAM}.cfgfiles;
	fi
	cp ${PKGINSTDIR}/copyright ${ROOT_LIBLPP_TMPD}/${FSETNAM}.copyright;


	ROOT_LIBLPP_MEM="${FSETNAM}.al ${FSETNAM}.inventory\
                	${FSETNAM}.size ${FSETNAM}.copyright\
                	${INSTAL} ${CLEANUP} ${DEINSTAL} \
                	${REJECT} ${UPDATE} ${CONFIG} \
                	${UNCONFIG} ${PRERM} ${FSETNAM}.cfgfiles";
	chown -R ${OWNGRP} ${ROOT_LIBLPP_TMPD};
	cd ${ROOT_LIBLPP_TMPD};
	ar rv liblpp.a ${ROOT_LIBLPP_MEM};
	cd ${HERE};
	mkdir -p ${ROOT_LIBLPPD};
	cp ${ROOT_LIBLPP_TMPD}/liblpp.a ${ROOT_LIBLPPD};
fi

# usr archive
if [ -f ${USRFILES} ]
then
	cp ${CLEANUP} ${USR_LIBLPP_TMPD}/${CLEANUP};
	cp ${DEINSTAL} ${USR_LIBLPP_TMPD}/${DEINSTAL};
	cp ${REJECT} ${USR_LIBLPP_TMPD}/${REJECT};
	cp ${INSTAL} ${USR_LIBLPP_TMPD}/${INSTAL};
	cp ${UPDATE} ${USR_LIBLPP_TMPD}/${UPDATE};
	cp ${USRCFGFILES} ${USR_LIBLPP_TMPD}/${FSETNAM}.cfgfiles;
	chmod 544 ${USR_LIBLPP_TMPD}/${CLEANUP};
	chmod 544 ${USR_LIBLPP_TMPD}/${DEINSTAL};
	chmod 544 ${USR_LIBLPP_TMPD}/${REJECT};
	chmod 544 ${USR_LIBLPP_TMPD}/${INSTAL};
	chmod 544 ${USR_LIBLPP_TMPD}/${UPDATE};
	chmod 544 ${USR_LIBLPP_TMPD}/${FSETNAM}.cfgfiles;
	cp ${PKGINSTDIR}/copyright ${USR_LIBLPP_TMPD}/${FSETNAM}.copyright;

	USR_LIBLPP_MEM="${FSETNAM}.al ${FSETNAM}.inventory\
                	${FSETNAM}.size ${FSETNAM}.copyright\
                	${INSTAL} ${CLEANUP} ${DEINSTAL} \
                	${REJECT} ${UPDATE} ${FSETNAM}.cfgfiles";
	chown -R ${OWNGRP} ${USR_LIBLPP_TMPD};
	cd ${USR_LIBLPP_TMPD};
	ar rv liblpp.a ${USR_LIBLPP_MEM};
	cd ${HERE};
	mkdir -p ${USR_LIBLPPD};
	cp ${USR_LIBLPP_TMPD}/liblpp.a ${USR_LIBLPPD};
fi

# make backup image of the release
cd ${IMGSDIR};
ABS_RELTREEIMGSDIR=`pwd`;
cd ${HERE};

# set owner.group of everthing in the release tree before its backed up...
echo "Setting own.grp = ${OWNGRP} of all under ${RELTREEROOT}"
chown -R ${OWNGRP} ${RELTREEROOT};

echo "Creating backup(8) image of release in ${ABS_RELTREEIMGSDIR}/${FSETNAM}";
cd ${RELTREEROOT};
find . | backup -i -v -f - | dd of=${ABS_RELTREEIMGSDIR}/${FSETNAM};
cd ${HERE};

# layout a CD-ROM distribution 
DISTIMGDIR=${CDDISTTREEDIR}/usr/sys/inst.images
echo "";
echo "Generating CD-ROM distribution layout in ${DISTIMGDIR}";
echo "";
mkdir -p ${DISTIMGDIR};
cp ${ABS_RELTREEIMGSDIR}/${FSETNAM} ${DISTIMGDIR};
cd ${DISTIMGDIR};
inutoc .;
cd ${HERE};

# that oughta do it. yikes.

# clean up...
CLEANUP="${FILEINVENT} ${ROOTFILES} ${USRFILES} ${SHAREFILES} \
         ${SEDCMD_MAPPART} ${GRAPHOUT} ${SOURCES} \
         ${SEDCMD_RSLVPART} ${AMBSOURCES} \
         ${SOURCES} ${NOSOURCES} ${AMBSOURCES} ${NMSOURCES} \
         ${ROOTDIRSINVENT} ${ROOTDIRSAL} ${USRDIRSINVENT} ${USRDIRSAL} \
         ${ROOTSYMLSINVENT} ${ROOTSYMLSAL} ${USRSYMLSINVENT} ${USRSYMLSAL} \
         ${ROOTAL} ${ROOTINVENT} ${USRAL} ${USRINVENT} \
         ${INSTAL} ${CLEANUP} ${DEINSTAL} ${REJECT} \
         ${UPDATE} ${CONFIG} ${UNCONFIG} ${SHAREAL} ${SHAREINVENT} \
         ${SIZEINFO} ${STUBBY} ${USR_LIBLPP_TMPD} ${ROOT_LIBLPP_TMPD}";


# rm -rf ${CLEANUP};

exit 0;
