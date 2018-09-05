#!/bin/sh
# using  pkginstall files as template, 
# generate liblpp.a FILESETNAME.inventory file.

# use me...
Usg() {
echo "Usage: $0 -r <release fileset> -f <liblpp file type> [[-t <list type>]...] [-o] [-g] [-m] [-s <part>]"
echo " where <release fileset> is one of:"
echo "   E - executables"
echo "   D - documentation"
echo " where <prototype file>: "
echo "   pkgproto(1) prototype file"
echo " where <liblpp file type>: "
echo "   A - apply list"
echo "   I - inventory"
echo " where <list type type>: "
echo "   f - files list"
echo "   l - symlinks list"
echo "   d - directories list"
echo "   -o list owner (-A only)"
echo "   -g list group (-A only)"
echo "   -m list modes (-A only)"
echo "   -s list only in fileset partition <part>"
echo " where <part> : "
echo "   r - root partition"
echo "   u - usr partition"
echo "   s - share partition"
}

# package type from argv
PROTOFIL=""
rtyp=""
ftyp=""
fspart=""
ldirs="false"
lfils="false"
lsyml="false"
lall="true"
own="false"
grp="false"
mod="false"
part="false"
while getopts ogmt:f:r:s: name
do
	case ${name} in
	r)	rtyp="$OPTARG";;
	f)	ftyp="$OPTARG";;
	s)	part="true";fspart="$OPTARG";;
	t)	ltyp="$OPTARG";
		case ${ltyp} in
			d)	ldirs="true"; lall="false";;
			f)	lfils="true"; lall="false";;
			l)	lsyml="true"; lall="false";;
			?)	Usg; exit 2;;
		esac
		;;
	o)	own="true";;
	g)	grp="true";;
	m)	mod="true";;
	?)	Usg; exit 2;;
	esac
done
if [ "xx${ftyp}" = "xx" ]
then
	Usg;
	exit 1;
fi
if [ "xx${rtyp}" = "xx" ]
then
	Usg;
	exit 1;
fi
# default is to list all types
if [ "${lall}" = "true" ]
then
	ldirs="true";
	lfils="true";
	lsyml="true";
fi

# where to look for the manifest
TOP=../../..
PKGINSTDIR=${TOP}/pkg.install
if [ "${rtyp}" = "E" ]
then
	PROTOFIL=${PKGINSTDIR}/prototype.src
else
	PROTOFIL=${PKGINSTDIR}/prototype.doc
fi

# pick off directories, regular files, and symlinks.
# pattern any absolute pkginstall directory names for
# subsequent postprocessing.
# resolve product name and revision.
PRODUCT=`./PRODUCT`
CAPQ=`echo ${PRODUCT} | tr '[a-z]' '[A-Z]'`
FTDREV=`./FTDREV`
PKGNAME=`./PKGNAM`
Q=${PKGNAME};

if [ "${rtyp}" = "E" ]
then
	FSETNAM=`./FSETNAM`
else
	FSETNAM=`./FSETNAM`.doc
fi

class="apply,inventory,${FSETNAM}"

SEDCMD=./product.sed
rm -f ${SEDCMD}
cat 1> ${SEDCMD} 2>&1 <<EOSEDCMD
/%Q%/s//${PRODUCT}/g
/%CAPQ%/s//${CAPQ}/g
/%REV%/s//${FTDREV}/g
/%PKGNM%/s//${PKGNAME}/g
EOSEDCMD

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
SEDCMD_MAPPART=./MAP.sed;
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

# list here AIX extras to be included
AIXTRAS=./AIXSUPPLS.${FSETNAM}
if [ "${rtyp}" = "E" ]
then
# executables fileset files
cat 1> ${AIXTRAS} 2>&1 << EOAIXTRAS
d none /var/opt/%PKGNM%/journal 0755 bin bin
f none /etc/init.d/%PKGNM%-startdriver 0555 bin bin
f none /etc/init.d/%PKGNM%-rcedit 0555 bin bin
f none /%USRDIR%/usr/lib/methods/%PKGNM%.cat 0444 bin bin
f none /%USRDIR%/usr/lib/methods/%PKGNM%.add 0555 bin bin
f none /%USRDIR%/usr/lib/methods/%PKGNM%.get 0555 bin bin
f none /%USRDIR%/usr/lib/methods/%PKGNM%.delete 0555 bin bin
f none /%USRDIR%/usr/lib/methods/cfg%PKGNM% 0555 bin bin
f none /%USRDIR%/usr/lib/methods/ucfg%PKGNM% 0555 bin bin
f none /%USRDIR%/usr/lib/methods/def%PKGNM% 0555 bin bin
f none /%USRDIR%/usr/lib/methods/udef%PKGNM% 0555 bin bin
EOAIXTRAS
else
# documentation fileset files
cat 1> ${AIXTRAS} 2>&1 << EOAIXTRAS
f none %PKGNM%/doc/pdf/AIX.pdf 0755 bin bin
f none %PKGNM%/doc/html/cconf4.htm 0644 root bin
f none %PKGNM%/doc/html/install.htm 0644 root bin
f none %PKGNM%/doc/html/images/configa2.gif 0644 root bin
f none %PKGNM%/doc/html/images/install0.gif 0644 root bin
f none %PKGNM%/doc/html/images/ad_ops0.gif 0644 root bin
f none %PKGNM%/doc/html/images/ad_opsa1.gif 0644 root bin
f none %PKGNM%/doc/html/images/ad_opsa2.gif 0644 root bin
f none %PKGNM%/doc/html/images/ad_opsa3.gif 0644 root bin
f none %PKGNM%/doc/html/images/instala1.gif 0644 root bin
f none %PKGNM%/doc/html/images/instala2.gif 0644 root bin
EOAIXTRAS
fi

AIXCL=./AIXCL.${FSETNAM}
# list here prototype files to exclude
if [ "${rtyp}" = "E" ]
then
# executables fileset files
cat 1> ${AIXCL} 2>&1 << EOAIXCL
EOAIXCL
else
# documentation fileset files
cat 1> ${AIXCL} 2>&1 << EOAIXCL
migf.gif
solaris.pdf
installi.htm
install9.htm
ad_opsa.gif
configa2.gif
configa4.gif
configa7.gif
configb3.gif
installa.gif
installing.ps
EOAIXCL
fi

cat ${AIXTRAS} ${PROTOFIL} |\
	egrep -v -f ${AIXCL} |\
	egrep "(^d|^f|^l)" |\
while read type ignore obj mode owner group
do

	# get src name, and in case of symlink, target.
	case ${type} in
		d) otype="DIRECTORY";
		   srcobj=${obj};;
		f) otype="FILE";
		   srcobj=${obj};;
		l) otype="SYMLINK";
		   tgtobj=`echo ${obj} | awk -F= '{print $1}'`;
		   srcobj=`echo ${obj} | awk -F= '{print $2}'`;;
	esac

	# chk if src obj is relative or absolute path names.
	# if relative, and rooted in %PKGNM% pkginstall would
	# be relative to /opt, so map this to %OPTDIR%
	echo ${srcobj} | egrep "(^/)" 1> /dev/null 2>&1
	if [ $? -ne 0 ]
	then
		# pattern relative pathname
		srcobj=`echo "%OPTDIR%/${srcobj}" | sed -f ${SEDCMD}`
	else
		# pattern absolute pathname
		srcobj=`echo ${srcobj} |\
		sed -f ${SEDCMD} |\
		sed -e '\@^/etc/opt/%%Q%%@ s@@%ETCOPTDIR%@' \
		    -e '\@^/etc/opt@d' \
		    -e '\@^/etc/opt@ s@@%ETCOPTDIR%@' \
		    -e '\@^/var/opt@ s@@%VAROPTDIR%@' \
		    -e '\@^/etc/init.d@ s@@%ETCINITDDIR%@' \
		    -e '\@^/etc/rcS.d@ s@@%ETCRSDDIR%@' \
		    -e '\@^/etc/rc3.d@ s@@%ETCR3DDIR%@' \
		    -e '\@^/usr/kernel/drv@ s@@%USRKERNELDRVDIR%@' \
		    -e '\@^/usr/sbin@ s@@%USRSBINDIR%@'`
	fi

	# dont output holes or 
	# anything that failed to 
	# pattern
	if [  "${srcobj}xx" = "xx" ]
	then
		continue;
	fi
	echo ${srcobj} | egrep "(%)" 1> /dev/null 2>&1
	if [ $? -ne 0 ]
	then
		continue;
	fi
	

	if [ "${type}" = "l" ]
	then
		echo ${tgtobj} | egrep "(^/)" 1> /dev/null 2>&1
		if [ $? -ne 0 ]
		then
			# pattern relative pathname
			tgtobj=`echo "%OPTDIR%/${tgtobj}" | sed -f ${SEDCMD}`
		else
			# pattern absolute pathname
			tgtobj=`echo ${tgtobj} |\
			sed -f ${SEDCMD} |\
			sed -e '\@^/etc/opt/%%Q%%@ s@@%ETCOPTDIR%@' \
			    -e '\@^/etc/opt@d' \
			    -e '\@^/var/opt@ s@@%VAROPTDIR%@' \
			    -e '\@^/etc/init.d@ s@@%ETCINITDDIR%@' \
			    -e '\@^/etc/rcS.d@ s@@%ETCRSDDIR%@' \
			    -e '\@^/etc/rc3.d@ s@@%ETCR3DDIR%@' \
			    -e '\@^/usr/kernel/drv@ s@@%USRKERNELDRVDIR%@' \
			    -e '\@^/usr/sbin@ s@@%USRSBINDIR%@'`
		fi
	fi

	# whether to output, according to type.
	if [ "${type}" = "l" -a "${lsyml}" = "false" ]
	then
		continue;
	fi
	# whether to output, according to type.
	if [ "${type}" = "f" -a "${lfils}" = "false" ]
	then
		continue;
	fi
	# whether to output, according to type.
	if [ "${type}" = "d" -a "${ldirs}" = "false" ]
	then
		continue;
	fi

	if [ "${part}" = "true" ]
	then
		# sort out which fileset partition 
		# this obj belongs to.
		whichpart=`echo ${srcobj} | sed -f ${SEDCMD_MAPPART}`
		echo ${whichpart} | grep %USRPART% 1> /dev/null 2>&1
		if [ $? -eq 0 ]
		then
			objpart="u"
		fi
		echo ${whichpart} | grep %ROOTPART% 1> /dev/null 2>&1
		if [ $? -eq 0 ]
		then
			objpart="r"
		fi
		echo ${whichpart} | grep %SHAREPART% 1> /dev/null 2>&1
		if [ $? -eq 0 ]
		then
			objpart="s"
		fi
		if [ "${fspart}" != "${objpart}" ]
		then
			continue
		fi
	fi

	# output an apply list entry, or an inventory stanza.
	if [ "${ftyp}" = "I" ]
	then
		echo ""
		if [ "${type}" = "l" ]
		then
			echo "${tgtobj}:"
			echo "  target = ${srcobj}"
		else
			echo "${srcobj}:"
		fi
		echo "  type = ${otype}"
		echo "  owner = ${owner}"
		echo "  group = ${group}"
		echo "  mode = ${mode}"
		echo "  class = ${class}"
		echo ""
	else
		# additional fields?
		if [ ${own} = "true" ]
		then
			srcobj="${srcobj} ${owner}"
		fi
		if [ ${grp} = "true" ]
		then
			srcobj="${srcobj} ${group}"
		fi
		if [ ${mod} = "true" ]
		then
			srcobj="${srcobj} ${mode}"
		fi
		echo "${srcobj}"
	fi
done
rm -f ${SEDCMD} ${AIXTRAS} ${AIXCL}
exit 0;
