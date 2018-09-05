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
	echo "  [-t] skip tar'ing package"
}

osmajor()
{
	OLDIFS=${IFS}
	IFS="."; set -- `oslevel`
	echo $1
	IFS=${OLDIFS}
}

bar="-----------------------------------------------------------"

# whether to use cached data
needinvfils=0
cleanonly=0
skiptaring=0

# make either executables or documentation filesets
rtyp=""

while getopts cfr:t name
do
	case ${name} in
		r)	rtyp="$OPTARG";;
		f)	needinvfils=1;;
		c)	cleanonly=1;;
		t)	skiptaring=1; echo skiptaring;;
		?)	Usg; exit 2;;
	esac
done
if [ "xx${rtyp}" = "xx" ]
then
	Usg;
	exit 1;
fi

# where to work and where the image goes
TOP=../../..
RELTREEDIR=../rel;

# who owns the files...
OWNGRP=root.system

# package and fileset names
# processing either executables or documents...
VERSION=`./VERSLVL`.`./RELLVL`.`./MODLVL`.`./FXLVL`;
PKGNM=`./PKGNAM`;
PKGANDLVL=${PKGNM}.${VERSION};
BUILDLVL=`./BUILDLVL`;
PRODUCTNAME=`./PRODUCTNAME`;
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
ROOTDEINSTAL=./ROOT.lpp.deinstal
USRDEINSTAL=./USR.lpp.deinstal

# how to postinstall configure
# or preremove unconfigure ODM
PREI=./${FSETNAM}.pre_i
CONFIG=./${FSETNAM}.config
UNCONFIG=./${FSETNAM}.unconfig_d
PRERM=./${FSETNAM}.pre_rm
POSTI=./${FSETNAM}.post_i

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
         ${UPDATE} ${PREI} ${CONFIG} ${UNCONFIG} ${SHAREAL} ${SHAREINVENT} \
         ${ROOTCFGFILES} ${USRCFGFILES} ${PRERM} ${POSTI}\
         ${ROOTDEINSTAL} ${USRDEINSTAL} \
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
\@%ETCBASEDIR%@s@@%ROOTPART%/etc/%%Q%%@
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
unamevers=`${TOP}/mk/unamevers.sh`;
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
			if [ "${unamevers}" -gt "500" ] 
			then
				if [ ${gdnm} != "./driver/aix43" -a ${gdnm} != "./driver/aix43/aixmethods" ]
				then 
					if [ "${gfnm}" = "${ifnm}" ]
					then
						cnt=`expr ${cnt} + 1`;
						svfnm=${gfnm};
						svdnm=${gdnm};
					fi
				fi
			elif [ "${unamevers}" -le "430" ]  
			then
				if [ ${gdnm} != "./driver/aix51" -a ${gdnm} != "./driver/aix51/aixmethods" ]
				then
					if [ "${gfnm}" = "${ifnm}" ]	
					then
						cnt=`expr ${cnt} + 1`;
						svfnm=${gfnm};
						svdnm=${gdnm};
					fi
				fi
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
FTDBINDIR=/${FTDBINDIR};

# files to save
SVF=\`echo \${DRVCONFDIR}/%%Q%%.conf\\
          \${LGCONFDIR}/*.lic \\
          \${LGCONFDIR}/p*.cfg \\
          \${LGCONFDIR}/s*.cfg \\
          \${LGCONFDIR}/%%Q%%Agent.cfg \\
          \${LGCONFDIR}/*.sh \\
          \${LOGDIR}/*_migration_tracking.csv \\
          \${LOGDIR}/*_migration_tracking.chk \\
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

for a in \${FTDBINDIR}/dtcmklv  \${FTDBINDIR}/dtcmklv.orig.*
do
    test -f \$a && rm -f \$a
done


rm -fr /var/%%Q%%/run/*

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

cat > ${ROOTDEINSTAL} << EOF
#!/bin/sh

LC_ALL=C; export LC_ALL; 

PRE_ODMDIR=\$ODMDIR
ODMDIR=/etc/objrepos; export ODMDIR

abort()
{
	ret=\$1
	echo "f ${PRODUCT}.rte" >>\$INUTEMPDIR/status
	exit \$ret
}

# logging
logf=/var/${PRODUCT}/${PRODUCT}error.log;
datestr=\`date "+%Y/%m/%d %T"\`;
errhdr="[\${datestr}] ${PRODUCT}: ";

IS_USR=\`/usr/bin/lslpp -icOu ${PRODUCT}.rte 2> /dev/null | egrep ^[^#] | wc -l |xargs echo\`
if [ \$IS_USR -gt 0 ]
then
	# Kill any daemons
	/${FTDBINDIR}/killagent
	/${FTDBINDIR}/killpmds
	/${FTDBINDIR}/killrmds
	/${FTDBINDIR}/killrefresh
	/${FTDBINDIR}/kill%%Q%%master
	/${FTDBINDIR}/%%Q%%stop -a

	# Check running
	/${FTDBINDIR}/%%Q%%info -a 1> /dev/null 2>&1
	if [ \$? -eq 0 ]
	then
		errmsg=\`echo "\${errhdr}\" "\$0: All the groups couldn't be stopped."\`
		echo \${errmsg}
		echo \${errmsg} 1>> \${logf} 2>&1
		abort 1
	fi
fi

ODMDIR=\$PRE_ODMDIR; export ODMDIR

/usr/lib/instl/deinstal \$*
opstat=\$?
exit \${opstat}
EOF

cat > ${USRDEINSTAL} << EOF
#!/bin/sh

LC_ALL=C; export LC_ALL; 

PRE_ODMDIR=\$ODMDIR
ODMDIR=/etc/objrepos; export ODMDIR

abort()
{
	ret=\$1
	echo "f ${PRODUCT}.rte" >>\$INUTEMPDIR/status
	exit \$ret
}

# logging
logf=/var/${PRODUCT}/${PRODUCT}error.log;
datestr=\`date "+%Y/%m/%d %T"\`;
errhdr="[\${datestr}] ${PRODUCT}: ";

IS_ROOT=\`/usr/bin/lslpp -icOr ${PRODUCT}.rte 2> /dev/null | egrep ^[^#] | wc -l |xargs echo\`
if [ \$IS_ROOT -gt 0 ]
then
	errmsg=\`echo "\${errhdr}\" "\$0: root part is not uninstalled."\`
	echo \${errmsg}
	echo \${errmsg} 1>> \${logf} 2>&1
	abort 1
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
		abort 2;
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
		abort 2;
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
	abort 3;
fi

# assert CuDv undefined
/usr/sbin/lsdev -C -c ${PRODUCT}_class |\
  egrep "(Defined|Available)" 1> /dev/null 2>&1
if [ \$? -eq 0 ]
then
	errmsg=\`echo "\${errhdr}\" "\$0: ODM CuDv undefinition failed."\`;
	echo \${errmsg};
	echo \${errmsg} 1>> \${logf} 2>&1 
	abort 4;
fi

# all's well...
errmsg=\`echo "\${errhdr}\" "\$0: Driver ODM PdDv and CuDv undefine succeeded. "\`;
echo \${errmsg};
echo \${errmsg} 1>> \${logf} 2>&1 

ODMDIR=\$PRE_ODMDIR; export ODMDIR

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

# make pre_i script
cat > ${PREI} << EOF
#!/bin/sh

# Starting at RFX 2.7.0, the package is built on AIX 5.3 for all AIX platforms and we no longer support AIX 5.1, 5.2 or earlier levels.
# An attempt to install this product on these platforms may damage the system.
# These platforms require that the package be compiled on the same oslevel as theirs. In case an exception is
# made for a customer and the package is rebuilt using a build machine with the same OS level as the target machine,
# then we would accept the install. Otherwise we reject it.

# The following are evaluated on run time and identify the OSlevel of the target machine:
PCRELEASE=\"\`uname -r\`\";
PCVERSION=\"\`uname -v\`\";
REJECT_INSTALL=0;

# In the following, the uname commands are preformed at build time (providing OSlevel of build machine)
# and compared to the run time evaluated variables (providing the OSlevel of the target machine):
if [ \${PCVERSION} = \"4\" ]
then
#   We no longer support AIX 4.x
    REJECT_INSTALL=1;
fi
if [ \${PCVERSION} = \"5\" ]
then
    if [ \${PCRELEASE} = \"1\" -o \${PCRELEASE} = \"2\" ]
    then
#   Target machine is AIX 5.1 or 5.2
        if [ \${PCRELEASE} != \"`uname -r`\" -o \${PCVERSION} != \"`uname -v`\" ]
        then
#           We no longer support AIX 5.1 nor 5.2 UNLESS the product is recompiled
#           on a Build machine of the same OSlevel
            REJECT_INSTALL=1;
        fi
    fi
fi

if [ \${REJECT_INSTALL} = \"1\" ]
then
    errmsg=\`echo "[ERROR] The package(${FSETNAM}) selected for installation differs from the target system, identified as AIX \${PCVERSION}.\${PCRELEASE}."\`;
	echo "";
	echo \${errmsg} 1>> \${logf} 2>&1 
    errmsg=\`echo "[ERROR] Please select the correct package for this system."\`;
	echo \${errmsg} 1>> \${logf} 2>&1 
	echo "";
	exit 1;
fi

exit 0;

EOF

if [ ${rtyp} = "E" ]
then
# make config script
cat > ${CONFIG} << EOF
#!/bin/sh

LC_ALL=C; export LC_ALL; 

# logging
logf=/var/${PRODUCT}/${PRODUCT}error.log;
datestr=\`date "+%Y/%m/%d %T"\`;
errhdr="[\${datestr}] ${PRODUCT}: ";

PCRELEASE=\"\`uname -r\`\";
OSLEVEL=\"\`/usr/bin/oslevel\`\";
# We keep the first 2 digits of the OS level (ex.: 5.3)
OSLEVEL=\`echo \${OSLEVEL} | /usr/bin/cut -c 1-3\`
 
# Verify that the target AIX platform is among the platforms the we have validated.
# Qualified platforms are taken from the lookup file /etc/%%Q%%/lib/validated_AIX_platforms.txt
PLATFORM_FOUND=0
if [ -f /etc/%%Q%%/lib/validated_AIX_platforms.txt ]
then
  for VALIDATED_PLATFORM in \`/usr/bin/cat /etc/%%Q%%/lib/validated_AIX_platforms.txt | /bin/awk '{print \$1}' 2>/dev/null\`
  do
    if [ \$PLATFORM_FOUND = 0 ]
    then
      CHECK_COMMENT=\`echo \${VALIDATED_PLATFORM} | /usr/bin/cut -c 1-1\`
      if [ \$CHECK_COMMENT != "#" ]
      then
        VALIDATED_PLATFORM=\`echo \${VALIDATED_PLATFORM} | /usr/bin/cut -c 1-3\`
        if [ "\${OSLEVEL}" = "\${VALIDATED_PLATFORM}" ]
        then
          PLATFORM_FOUND=1
        fi
      fi
    fi
  done
  if [ \$PLATFORM_FOUND = 0 ]
  then
    # The current target platform is not among our validated platforms
    errmsg=\`echo "\${errhdr}\" "\$0: WARNING: Target platform AIX \${OSLEVEL} was not a qualified platform at release time for ${PRODUCTNAME} v${VERSION}-${BUILDLVL}."\`
    echo \${errmsg};
    echo \${errmsg} 1>> \${logf} 2>&1 
    errmsg=\`echo "\${errhdr}\" "\$0: WARNING: Installation still proceeding but see compatibility matrix for qualified platform updates or contact support."\`
    echo \${errmsg};
    echo \${errmsg} 1>> \${logf} 2>&1
    sleep 5 
  fi
else
# Validated platform lookup file not found
  errmsg=\`echo "\${errhdr}\" "\$0: WARNING: Validated platform list not found at installation of ${PRODUCTNAME} v${VERSION}-${BUILDLVL} (/etc/%%Q%%/lib/validated_AIX_platforms.txt)."\`
  echo \${errmsg};
  echo \${errmsg} 1>> \${logf} 2>&1 
  errmsg=\`echo "\${errhdr}\" "\$0: WARNING: Installation still proceeding but please check with Technical Support related to this file not being in your package."\`
  echo \${errmsg};
  echo \${errmsg} 1>> \${logf} 2>&1 
  sleep 5 
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

# Check if the flag file exists indicating that we use the legacy licensing mechanism,
# in which case the license is provided by Product Support or we reuse the already existing customer license.
# For TDMFIP 2.9.0, force the legacy licensing mechanism (but preserve the old code based on the flag file).
touch /var/dtc/SFTKdtc_use_legacy_mechanism
if [ -f /var/dtc/SFTKdtc_use_legacy_mechanism ]
then
	rm -f /etc/%%Q%%/lib/DTC.lic.perm
fi

# Copy the config files from past revs
if [ -d /${VAROPTDIR}/%%Q%% ]; then
    currentdir=\`pwd\`;    
    cd /${VAROPTDIR}/%%Q%%
    a=\`find . -type d -print | xargs ls -td | egrep /%%Q%%. | head -1\`
    if [ "\$a" != "" ]; then
        aa=\`echo \$a | sed "s/.\///"\`

        if [ -f /var/dtc/SFTKdtc_use_legacy_mechanism ]
        then
	        echo Restoring earlier configuration and license files and devlist file from /${VAROPTDIR}/%%Q%%/\$aa.
        else
	        echo Restoring earlier configuration files and devlist file from /${VAROPTDIR}/%%Q%%/\$aa.
        fi
	    mkdir -p /etc/%%Q%%/lib
        # Starting with release 2.8.0 (product ownership going to IBM STG), the license file is now
        # part of the package; we do not need to restore a previously installed license,
        # UNLESS the flag file exists indicating that we use the legacy licensing mechanism.
        if [ -f /var/dtc/SFTKdtc_use_legacy_mechanism ]
        then
	        for i in \$a/*.lic
	        do	
	            cp \$i /etc/%%Q%%/lib/ > /dev/null 2>&1
	        done
        else
            # Rename the new packaged permanent license file to the effective name
            echo Making DTC.lic.perm the effective DTC.lic.
            mv /etc/%%Q%%/lib/DTC.lic.perm /etc/%%Q%%/lib/DTC.lic
            chmod 0444 /etc/%%Q%%/lib/DTC.lic
        fi
	    for i in \$a/*.sh
	    do	
	        if [ "\$i" != "\$a/SFTKdtc_AIX_failover_boot_network_reconfig.sh" ] # Not restoring this one
            then
	            cp \$i /etc/%%Q%%/lib/ > /dev/null 2>&1
            fi
	    done
	    if [ -f \$a/devlist.conf ]; then
               cp \$a/devlist.conf /etc/%%Q%%/lib/ > /dev/null 2>&1
	    fi
	else
        echo /var/dtc exists but no configuration and license files to restore from past installations.
        # Rename the new packaged permanent license file to the effective name unless the legacy flag file exists
        if [ ! -f /var/dtc/SFTKdtc_use_legacy_mechanism ]
        then
            echo Making DTC.lic.perm the effective DTC.lic.
            mv /etc/%%Q%%/lib/DTC.lic.perm /etc/%%Q%%/lib/DTC.lic
            chmod 0444 /etc/%%Q%%/lib/DTC.lic
        else
            echo The file /var/dtc/SFTKdtc_use_legacy_mechanism is present: it is expected that a license has been or will be provided for this installation.
        fi
    fi
    cd \${currentdir}
else
    # /var/dtc does not exist. No backup directory from previous installs
    # Rename the new packaged permanent license file to the effective name
    # NOTE: since /var/dtc does not exist, the flag file for legacy licensing mechanism cannot exist either
    echo Making DTC.lic.perm the effective DTC.lic.
    mv /etc/%%Q%%/lib/DTC.lic.perm /etc/%%Q%%/lib/DTC.lic
    chmod 0444 /etc/%%Q%%/lib/DTC.lic
fi

# replace stock chfs with our 'passthrough' version
/usr/dtc/libexec/chfs_install

# set up our lparid
/${FTDBINDIR}/%%Q%%genlparid

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

# make post_i script
cat > ${POSTI} << EOF
#!/bin/sh

FTDBINDIR=/${FTDBINDIR}

if [ -f \${FTDBINDIR}/genmklv ]
then
    \${FTDBINDIR}/genmklv -s || echo "Warning: Problems creating dtcmklv." 
fi

exit 0
EOF


# make unconfigure script
cat > ${UNCONFIG} << EOF
#!/bin/sh

LC_ALL=C; export LC_ALL;

status="0"

BackupFiles()
{
    if [ "\$#" -eq 0 ]
    then
	return 0
    fi

    substatus="0"

    # Create /${VAROPTDIR}/%%Q%%/${PKGANDLVL}, if necessary
    if [ ! -d "/${VAROPTDIR}/%%Q%%/${PKGANDLVL}" ]
    then
	mkdir -p -m 755 /${VAROPTDIR}/%%Q%%/${PKGANDLVL}
	if [ \$? -ne 0 ]
	then
	    echo "Creating directory /${VAROPTDIR}/%%Q%%/${PKGANDLVL} failed.";
	    echo "This error is most likely caused by directory permissions, "
	    echo "a file exists by the same name, or being out of space on the"
	    echo "filesystem assoicated with the /${VAROPTDIR}/%%Q%% directory."
	    return 1
	fi
    fi

    for a 
    do
	b="/${VAROPTDIR}/%%Q%%/${PKGANDLVL}/\`basename \${a}\`"
	cp -p "\${a}" "\${b}"
	if [ \$? -ne 0 ]
	then
	    echo "Copying \${a} to file";
	    echo "\${b} failed.";
	    echo "This error is most likely caused by directory permissions"
	    echo "or being out of space on the filesystem assoicated with the"
	    echo "file \${b}."
	    substatus=1
	fi

	cmp -s "\${a}" "\${b}"
	if [ \$? -ne 0 ]
	then
	    echo "Comparing \${a} to file";
	    echo "\${b} failed.";
	    echo "This error is most likely caused by being out of space on the"
	    echo "filesystem associated with the"
	    echo "/${VAROPTDIR}/%%Q%%/${PKGANDLVL} directory."
	    return 1
	fi
	rm -f \${a}
    done
    return 0
}

BackupFiles_with_timestamp()
{
    if [ "\$#" -eq 0 ]
    then
	return 0
    fi

    substatus="0"

    # Create /${VAROPTDIR}/%%Q%%/${PKGANDLVL}, if necessary
    if [ ! -d "/${VAROPTDIR}/%%Q%%/${PKGANDLVL}" ]
    then
	mkdir -p -m 755 /${VAROPTDIR}/%%Q%%/${PKGANDLVL}
	if [ \$? -ne 0 ]
	then
	    echo "Creating directory /${VAROPTDIR}/%%Q%%/${PKGANDLVL} failed.";
	    echo "This error is most likely caused by directory permissions, "
	    echo "a file exists by the same name, or being out of space on the"
	    echo "filesystem assoicated with the /${VAROPTDIR}/%%Q%% directory."
	    return 1
	fi
    fi

    date_suffix=\`date +\%Y\%m\%d-\%H\%M\`
    for a 
    do
	b="/${VAROPTDIR}/%%Q%%/${PKGANDLVL}/\`basename \${a}\`.\${date_suffix}"
	cp -p "\${a}" "\${b}"
	if [ \$? -ne 0 ]
	then
	    echo "Copying \${a} to file";
	    echo "\${b} failed.";
	    echo "This error is most likely caused by directory permissions"
	    echo "or being out of space on the filesystem assoicated with the"
	    echo "file \${b}."
	    substatus=1
	fi

	cmp -s "\${a}" "\${b}"
	if [ \$? -ne 0 ]
	then
	    echo "Comparing \${a} to file";
	    echo "\${b} failed.";
	    echo "This error is most likely caused by being out of space on the"
	    echo "filesystem associated with the"
	    echo "/${VAROPTDIR}/%%Q%%/${PKGANDLVL} directory."
	    return 1
	fi
	rm -f \${a}
    done
    return 0
}

# uninstall the driver start hook, (also /etc/services).
/etc/%%Q%%/init.d/%%Q%%-rcedit -u
if [ \$? -ne 0 ]
then
    status="1"
    echo "WARNING: %%Q%%-rcedit had problems while executing"
fi

# Check if any .cfg files exist
a=\`ls -d /etc/%%Q%%/lib/[ps]???.cfg 2>/dev/null\`
if [ \$? -eq 0 -a -n \"\${a}\" ]
then

    # if any prior config files exist in save directory
    rm -f \`ls -d /${VAROPTDIR}/%%Q%%/${PKGANDLVL}/[ps]???.cfg 2>/dev/null\`

    echo Moving %%Q%% Config files to /${VAROPTDIR}/%%Q%%/${PKGANDLVL}
    BackupFiles \${a}
    [ \${status} -eq 0 -a \$? -ne 0 ] && status="1"
fi

# Check if any Agent cfg files exist
a=\`ls -d /etc/%%Q%%/lib/%%Q%%Agent.cfg 2>/dev/null\`
if [ \$? -eq 0 -a -n \"\${a}\" ]
then
    # if any prior config files exist in save directory
    rm -f /${VAROPTDIR}/%%Q%%/${PKGANDLVL}/%%Q%%Agent.cfg

    echo Moving Agent Config files to /${VAROPTDIR}/%%Q%%/${PKGANDLVL}
    BackupFiles \${a}
    [ \${status} -eq 0 -a \$? -ne 0 ] && status="1"
fi

# Check if any .sh files exist
a=\`ls -d /etc/%%Q%%/lib/*.sh 2>/dev/null\`
if [ \$? -eq 0 -a -n \"\${a}\" ]
then
    # if any prior .sh files exist in save directory
    rm -f /${VAROPTDIR}/%%Q%%/${PKGANDLVL}/*.sh

    echo Moving %%Q%% Shell files to /${VAROPTDIR}/%%Q%%/${PKGANDLVL}
    BackupFiles \${a}
    [ \${status} -eq 0 -a \$? -ne 0 ] && status="1"
fi

# Check if any .lic files exist
a=\`ls -d /etc/%%Q%%/lib/*.lic 2>/dev/null\`
if [ \$? -eq 0 -a -n \"\${a}\" ]
then
    # if any prior %%Q%%.lic file exist in save directory
    rm -f /${VAROPTDIR}/%%Q%%/${PKGANDLVL}/*.lic

    echo Moving %%Q%% License files to /${VAROPTDIR}/%%Q%%/${PKGANDLVL}
    BackupFiles \${a}
    [ \${status} -eq 0 -a \$? -ne 0 ] && status="1"
fi

# Backup pxxx_migration_tracking csv and chk files
# Check if any pxxx_migration_tracking.csv files exist (product usage tracking files)
a=\`ls -d /var/%%Q%%/p???_migration_tracking.csv 2>/dev/null\`
if [ \$? -eq 0 -a -n \"\${a}\" ]
then

    echo Moving %%Q%% Product usage statistics files to /${VAROPTDIR}/%%Q%%/${PKGANDLVL}
    BackupFiles_with_timestamp \${a}
    [ \${status} -eq 0 -a \$? -ne 0 ] && status="1"
fi

# Check if any pxxx_migration_tracking.chk files exist (product usage tracking checksum files)
a=\`ls -d /var/%%Q%%/p???_migration_tracking.chk 2>/dev/null\`
if [ \$? -eq 0 -a -n \"\${a}\" ]
then

    echo Moving %%Q%% Product usage checksum files to /${VAROPTDIR}/%%Q%%/${PKGANDLVL}
    BackupFiles_with_timestamp \${a}
    [ \${status} -eq 0 -a \$? -ne 0 ] && status="1"
fi

# Check if any global product_usage_stats csv files exist (product usage global tracking files of this server)
a=\`ls -d /var/%%Q%%/*_product_usage_stats_*.csv 2>/dev/null\`
if [ \$? -eq 0 -a -n \"\${a}\" ]
then
    echo Moving %%Q%% Global Server Product usage statistics files to /${VAROPTDIR}/%%Q%%/${PKGANDLVL}
    BackupFiles \${a}
    [ \${status} -eq 0 -a \$? -ne 0 ] && status="1"
fi

# Check if the %%Q%%.conf file exists
a=\`ls -d /usr/lib/drivers/%%Q%%.conf 2>/dev/null\`
if [ \$? -eq 0 -a -n \"\${a}\" ]
then
    # if any prior %%Q%%.conf file exist in save directory
    rm -f /${VAROPTDIR}/%%Q%%/${PKGANDLVL}/%%Q%%.conf

    echo Moving the %%Q%%.conf file to /${VAROPTDIR}/%%Q%%/${PKGANDLVL}
    BackupFiles \${a}
    [ \${status} -eq 0 -a \$? -ne 0 ] && status="1"
fi

# Avoid any leftover dtclparid.cfg, net_analysis_group.cfg and SFTKdtc_net_analysis_parms.txt
# dtclparid.cfg:
a=\`ls -d /etc/%%Q%%/lib/%%Q%%lparid.cfg 2>/dev/null\`
if [ \$? -eq 0 -a -n \"\${a}\" ]
then
    echo Removing /etc/%%Q%%/lib/%%Q%%lparid.cfg
    rm -f /etc/%%Q%%/lib/%%Q%%lparid.cfg
    [ \${status} -eq 0 -a \$? -ne 0 ] && status="1"
fi
# net_analysis_group.cfg:
a=\`ls -d /etc/%%Q%%/lib/net_analysis_group.cfg 2>/dev/null\`
if [ \$? -eq 0 -a -n \"\${a}\" ]
then
    echo Removing /etc/%%Q%%/lib/net_analysis_group.cfg
    rm -f /etc/%%Q%%/lib/net_analysis_group.cfg
    [ \${status} -eq 0 -a \$? -ne 0 ] && status="1"
fi
# SFTKdtc_net_analysis_parms.txt:
a=\`ls -d /etc/%%Q%%/lib/SFTKdtc_net_analysis_parms.txt 2>/dev/null\`
if [ \$? -eq 0 -a -n \"\${a}\" ]
then
    echo Removing /etc/%%Q%%/lib/SFTKdtc_net_analysis_parms.txt
    rm -f /etc/%%Q%%/lib/SFTKdtc_net_analysis_parms.txt
    [ \${status} -eq 0 -a \$? -ne 0 ] && status="1"
fi

# Check if devlist.conf files exist
a=\`ls -d /etc/%%Q%%/lib/devlist.conf 2>/dev/null\`
if [ \$? -eq 0 -a -n \"\${a}\" ]
then
    # if any prior devlist.conf file exist in save directory
    rm -f /${VAROPTDIR}/%%Q%%/${PKGANDLVL}/devlist.conf

    echo Moving the devlist.conf files to /${VAROPTDIR}/%%Q%%/${PKGANDLVL}
    BackupFiles \${a}
    [ \${status} -eq 0 -a \$? -ne 0 ] && status="1"
fi

# exit status is not checked because it is ok if these
# files and directories do not exist

# Verify if the SFTKdtc_use_legacy_mechanism for licensing exists; if so, after cleaning up
# /var/dtc, it must be restored (just created, as it is empty)
Restore_SFTKdtc_use_legacy_mechanism=0
if [ -f /${VAROPTDIR}/%%Q%%/SFTKdtc_use_legacy_mechanism ]
then
    Restore_SFTKdtc_use_legacy_mechanism=1
fi

# Removeing temporary files from /${VAROPTDIR}/%%Q%%
# Hide output because of backup directory
rm -f /${VAROPTDIR}/%%Q%%/*  >/dev/null 2>&1

# delete /${VAROPTDIR}/%%Q%%/Agn_tmp directory
rm -fr /${VAROPTDIR}/%%Q%%/Agn_tmp

# delete  dtcmklv and any backed up copies

rm -f /${FTDBINDIR}/dtcmklv  /${FTDBINDIR}/dtcmklv.orig.*

/usr/dtc/libexec/chfs_remove

if [ \$Restore_SFTKdtc_use_legacy_mechanism -eq 1 ]
then
    echo Cleaned up /${VAROPTDIR}/%%Q%% but preserving /${VAROPTDIR}/%%Q%%/SFTKdtc_use_legacy_mechanism.
    touch /${VAROPTDIR}/%%Q%%/SFTKdtc_use_legacy_mechanism
fi

exit \${status}
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
	cp ${REJECT} ${ROOT_LIBLPP_TMPD}/${REJECT};
	cp ${INSTAL} ${ROOT_LIBLPP_TMPD}/${INSTAL};
	cp ${UPDATE} ${ROOT_LIBLPP_TMPD}/${UPDATE};
	chmod 544 ${ROOT_LIBLPP_TMPD}/${CLEANUP};
	chmod 544 ${ROOT_LIBLPP_TMPD}/${REJECT};
	chmod 544 ${ROOT_LIBLPP_TMPD}/${INSTAL};
	chmod 544 ${ROOT_LIBLPP_TMPD}/${UPDATE};
	if [ ${rtyp} = "E" ]
	then
		cp ${PREI} ${ROOT_LIBLPP_TMPD}/${PREI};
		cp ${CONFIG} ${ROOT_LIBLPP_TMPD}/${CONFIG};
		cp ${UNCONFIG} ${ROOT_LIBLPP_TMPD}/${UNCONFIG};
		cp ${PRERM} ${ROOT_LIBLPP_TMPD}/${PRERM};
		cp ${ROOTCFGFILES} ${ROOT_LIBLPP_TMPD}/${FSETNAM}.cfgfiles;
		cp ${ROOTDEINSTAL} ${ROOT_LIBLPP_TMPD}/${DEINSTAL};
		cp ${POSTI} ${ROOT_LIBLPP_TMPD}/${POSTI};
		chmod 544 ${ROOT_LIBLPP_TMPD}/${PREI};
		chmod 544 ${ROOT_LIBLPP_TMPD}/${CONFIG};
		chmod 544 ${ROOT_LIBLPP_TMPD}/${UNCONFIG};
		chmod 544 ${ROOT_LIBLPP_TMPD}/${PRERM};
		chmod 544 ${ROOT_LIBLPP_TMPD}/${FSETNAM}.cfgfiles;
		chmod 544 ${ROOT_LIBLPP_TMPD}/${DEINSTAL};
		chmod 544 ${ROOT_LIBLPP_TMPD}/${POSTI};
	else
		cp ${DEINSTAL} ${ROOT_LIBLPP_TMPD}/${DEINSTAL};
		chmod 544 ${ROOT_LIBLPP_TMPD}/${DEINSTAL};
	fi
	cp ${PKGINSTDIR}/copyright ${ROOT_LIBLPP_TMPD}/${FSETNAM}.copyright;


	ROOT_LIBLPP_MEM="${FSETNAM}.al ${FSETNAM}.inventory\
                	${FSETNAM}.size ${FSETNAM}.copyright\
                	${INSTAL} ${CLEANUP} ${DEINSTAL} \
                	${REJECT} ${UPDATE} ${CONFIG} ${PREI} ${POSTI} \
                	${UNCONFIG} ${PRERM} ${FSETNAM}.cfgfiles";
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
	cp ${REJECT} ${USR_LIBLPP_TMPD}/${REJECT};
	cp ${INSTAL} ${USR_LIBLPP_TMPD}/${INSTAL};
	cp ${UPDATE} ${USR_LIBLPP_TMPD}/${UPDATE};
	cp ${USRCFGFILES} ${USR_LIBLPP_TMPD}/${FSETNAM}.cfgfiles;
	cp ${PREI} ${USR_LIBLPP_TMPD}/${PREI};
	chmod 544 ${USR_LIBLPP_TMPD}/${CLEANUP};
	chmod 544 ${USR_LIBLPP_TMPD}/${REJECT};
	chmod 544 ${USR_LIBLPP_TMPD}/${INSTAL};
	chmod 544 ${USR_LIBLPP_TMPD}/${UPDATE};
	chmod 544 ${USR_LIBLPP_TMPD}/${FSETNAM}.cfgfiles;
	chmod 544 ${USR_LIBLPP_TMPD}/${PREI};
	if [ ${rtyp} = "E" ]
	then
	    cp ${USRDEINSTAL} ${USR_LIBLPP_TMPD}/${DEINSTAL};
	    chmod 544 ${USR_LIBLPP_TMPD}/${DEINSTAL};
	else
	    cp ${DEINSTAL} ${USR_LIBLPP_TMPD}/${DEINSTAL};
	    chmod 544 ${USR_LIBLPP_TMPD}/${DEINSTAL};
	fi
	cp ${PKGINSTDIR}/copyright ${USR_LIBLPP_TMPD}/${FSETNAM}.copyright;

	USR_LIBLPP_MEM="${FSETNAM}.al ${FSETNAM}.inventory\
                	${FSETNAM}.size ${FSETNAM}.copyright\
                	${INSTAL} ${CLEANUP} ${DEINSTAL} \
                	${REJECT} ${UPDATE} ${PREI} ${FSETNAM}.cfgfiles";
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

# Do not create the toc file unless you are the automated build user (bmachine).
# This file is only needed to install our package from a CD-ROM. (WR PROD00004186)

whoami=`whoami`
if [ "${whoami}" = "bmachine" ]
then
    TOCFILE=".toc"
    # Running inutoc requires root privileges.  We rely on sudo to obtain them.
    # The local directory can be mounted over NFS, in which case the root ends up being 'nobody'.
    # Because of this, we pre-create the .toc file and grant access to everyone so that inutoc can do its job.
    touch ${TOCFILE}
    chmod a+w ${TOCFILE}
    sudo inutoc .
    # Change the permissions back, mostly for cleanliness.
    chmod 644 ${TOCFILE}
fi
ls -l  ${TOCFILE} ${FSETNAM}
if [ "${skiptaring}" -eq 0 ]
then
# <<< TODO: make generic name as per TDMF3.2.1:
    TARFILENAME="${PRODUCTNAME}-`uname -s``uname -v``uname -r`-${VERSION}-${BUILDLVL}.${HWTYPE}.tar.Z"
    tar cvf - ${TOCFILE} ${FSETNAM} | compress >../../../../../${TARFILENAME}
    cd ../../../../../
    ls -l ${TARFILENAME}
fi
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
         ${USRDEINSTAL} ${ROOTDEINSTAL} \
         ${UPDATE} ${PREI} ${POSTI} ${CONFIG} ${UNCONFIG} \
	 ${SHAREAL} ${SHAREINVENT} \
         ${SIZEINFO} ${STUBBY} ${USR_LIBLPP_TMPD} ${ROOT_LIBLPP_TMPD}";


# rm -rf ${CLEANUP};

exit 0;
