#!/bin/sh

# compose an lpp_name file for the package.
# build a file of sed(1) commands to apply
# to a template lpp_name...

# use me...
Usg() {
echo "Usage: $0 -r <release fileset> -p <package type> [-c <content type> ...]"
echo " where <content type> is one of:"
echo "   R - content includes a Root part"
echo "   U - content includes a Usr part"
echo "   S - content includes a Share part"
echo " there must be a content type flag for each content type included"
echo " where <release fileset> is one of:"
echo "   E - executables"
echo "   D - documentation"
echo " where <package type>: "
echo " Indicates whether this is an installation  "
echo " update package and what type. The values are:  "
echo "   I -Installation   "
echo "   S -Single update   "
echo "   SR -Single update, required   "
echo "   ML -Maintenance level update "
echo " The following types are valid for Version  "
echo " 3.2-formatted update packages only:  "
echo "   G -Single update, required   "
echo "   M -Maintenance packaging update   "
echo "   MC -Cumulative packaging update "
echo "   ME -Enhancement packaging update "
}

# package type from argv
ptyp=""
rtyp=""
ctyp=""
rootpart="false"
usrpart="false"
sharepart="false"
while getopts c:r:p: name
do
	case ${name} in
	c)	ctyp="$OPTARG";
		case ${ctyp} in
		R)	rootpart="true";;
		U)	usrpart="true";;
		S)	sharepart="true";;
		?)	Usg; exit 2;;
		esac
		;;
	r)	rtyp="$OPTARG";;
	p)	pflag=1;
		ptyp="$OPTARG";;
	?)	Usg; exit 2;;
	esac
done
if [ "xx${rtyp}" = "xx" ]
then
	Usg;
	exit 1;
fi
if [ "xx${ptyp}" = "xx" ]
then
	Usg;
	exit 1;
fi
# validate ptyp
case ${ptyp} in
	I|S|SR|ML)	;;
	G|M|MC|ME)	echo "Installation for AIX Version 3.2 Not Supported"; 
			exit 2;;
	*)		Usg; exit 2;;
esac
case ${ptyp} in
	I)	ptypnam="Installation";;
	S)	ptypnam="Single Update";;
	SR)	ptypnam="Single Update, Required";;
	ML)	ptypnam="Maintenance Level Update";;
esac
echo "Making Distribution Package Type ${ptypnam}"

# which release type
if [ "${rtyp}" = "E" ]
then
	PACKAGE_NAME=`./FSETNAM`;
else
	PACKAGE_NAME=`./FSETNAM`.doc;
fi

# boilerplate for lpp_name
LPP_NAME_TMPL=./LPP_NAME.tmpl

# sed commands file
SED_CMDS_FILE=./tmpl2lpp_name.sed
rm -f ${SED_CMDS_FILE}

# whether we use iFOR/LS
IFORLS="false"

#  Field Name   Format      Separator Description
#  1. Format    Integer     White     Indicates the release level of installp
#                           space     for which this package was built. The
#                                     values are: 1 -AIX Version 3.1, 3 -
#                                     Version 3.2, 4 -AIX Version 4.1
FORMAT_TYPE=4
echo /@@FORMAT@@/s//${FORMAT_TYPE}/ 1>> ${SED_CMDS_FILE} 2>&1


#  Field Name   Format      Separator Description
#  2. Platform  Character   White     Indicates the platform for which this
#                           space     package was built. The only available
#                                     value is R.
PLATFORM_TYPE=R
echo /@@PLATFORM@@/s//${PLATFORM_TYPE}/ 1>> ${SED_CMDS_FILE} 2>&1

#  Field Name   Format      Separator Description
#  3. Package   Character   White     Indicates whether this is an installation
#  Type                     space     or update package and what type. The
#                                     values are: I -Installation, S -Single
#                                     update, SR -Single update, required, ML
#                                     -Maintenance level update
#                                     The following types are valid for Version
#                                     3.2-formatted update packages only: G
#                                     -Single update, required, M -Maintenance
#                                     packaging update, MC -Cumulative packaging
#                                     update, ME -Enhancement packaging update
PACKAGE_TYPE=${ptyp}
echo /@@PACKAGE_TYPE@@/s//${PACKAGE_TYPE}/ 1>> ${SED_CMDS_FILE} 2>&1

# 
#  Field Name   Format      Separator Description
#  4. Package   Character   White     The name of the software package
#  Name                     space     (PackageName).
#               {           New line  Indicates the beginning of the repeatable
#                                     sections of fileset-specific data.
# 
echo /@@PACKAGE_NAME@@/s//${PACKAGE_NAME}/ 1>> ${SED_CMDS_FILE} 2>&1

#  Field Name   Format      Separator Description
#  5. Fileset   Character   White     The complete name of the fileset. This
#  name                     space     field begins the heading information for
#                                     the fileset or fileset update.
# 
if [ "${rtyp}" = "E" ]
then
        FILESET_NAME=`./FSETNAM`;
else
        FILESET_NAME=`./FSETNAM`.doc;
fi
echo /@@FILESET_NAME@@/s//${FILESET_NAME}/ 1>> ${SED_CMDS_FILE} 2>&1

#  Field Name   Format      Separator Description
#  6. Level     Shown in    White     The level of the fileset to be installed.
#               Description space     The format is:
#               column                Version.Release.ModificationLevel.FixLevel
#                                     .FixID should be appended for Version
#                                     3.2-formatted updates only.
# 
VERSION_LEVEL=`./VERSLVL`
RELEASE_LEVEL=`./RELLVL`
MODIFICATION_LEVEL=`./MODLVL`
FIX_LEVEL=`./FXLVL`
LEVEL=${VERSION_LEVEL}.${RELEASE_LEVEL}.${MODIFICATION_LEVEL}.${FIX_LEVEL}

echo /@@LEVEL@@/s//${LEVEL}/ 1>> ${SED_CMDS_FILE} 2>&1

#  Field Name   Format      Separator Description
#  7. Diskette  Integer     White     Indicates the diskette volume number where
#  Volume                   space     the fileset is located if shipped on
#                                     diskette.
# 
DISKETTE_VOLUME=1
echo /@@DISKETTE_VOLUME@@/s//${DISKETTE_VOLUME}/ 1>> ${SED_CMDS_FILE} 2>&1

#  Field Name   Format      Separator Description
#  8. Bosboot   Character   White     Indicates whether a bosboot is needed
#                           space     following the installation. The values
#                                     are: N -Do not invoke bosboot, b -Invoke
#                                     bosboot
BOSBOOT=N
echo /@@BOSBOOT@@/s//${BOSBOOT}/ 1>> ${SED_CMDS_FILE} 2>&1

# 
#  Field Name   Format      Separator Description
#  9. Content   Character   White     Indicates the parts included in the
#                           space     fileset or fileset update. The values are:
#                                     B -usr and root part, H -share part, U
#                                     -usr part only
if [ "${rootpart}" = "true" -a "${usrpart}" = "true" ]
then
	CONTENT=B
fi
if [ "${rootpart}" = "false" -a "${usrpart}" = "true" ]
then
	CONTENT=U
fi
if [ "${sharepart}" = "true" ]
then
	CONTENT=H
fi
echo /@@CONTENT@@/s//${CONTENT}/ 1>> ${SED_CMDS_FILE} 2>&1

# 
#  Field Name   Format      Separator Description
#  10. Language Character   White     Not used.
#                           space
# 
LANGUAGE=en_US
echo /@@LANGUAGE@@/s//${LANGUAGE}/ 1>> ${SED_CMDS_FILE} 2>&1

#  Field Name   Format      Separator Description
#  11.          Character   # or new  Fileset description.
#  Description              line
# 
DESCRIPTION=`./PKGDESC`
echo /@@DESCRIPTION@@/s//${DESCRIPTION}/ 1>> ${SED_CMDS_FILE} 2>&1

#  Field Name   Format      Separator Description
#  12. Comments Character   New line  (Optional) Additional comments.
#               [           New line  Indicates the beginning of the body of the
#                                     fileset information.
# 
COMMENT="" # no comment
echo /@@COMMENT@@/s//${COMMENT}/ 1>> ${SED_CMDS_FILE} 2>&1

#  Field Name   Format      Separator Description
#  13.          Described   New line  (Optional) Installation dependencies the
#  Requisite    following             fileset has on other filesets and fileset
#  information  table                 updates. See the section following this
#                                     table for detailed description.
#               %           New line  Indicates the separation between requisite
#                                     and size information.
# 
if [ "${ptyp}" = "I" ]
then
	# for a new install should not be prerequisites...
	REQUISITE_INFO="" 
else
	# TBD:
	echo "NEED REQUISITE_INFO for Installation Type ${ptypnam}.";
	exit 1;
fi
if [ "xx${REQUISITE_INFO}" = "xx" ]
then
	echo /@@REQUISITE_INFO@@/d 1>> ${SED_CMDS_FILE} 2>&1
else
	echo /@@REQUISITE_INFO@@/s//${REQUISITE_INFO}/ 1>> ${SED_CMDS_FILE} 2>&1
fi

#  Field Name   Format      Separator Description
#  14. Size     Described   New line  Size requirements for the fileset by
#  information  later in              directory. See Size Information Section
#               this chapter          later in this article for detailed
#                                     description.
#               %           New line  Indicates the separation between size and
#                                     supersede information.
# 
SIZEINFOCMD=./mksizeinfo.sh
SIZEINFO=./SIZEINFO.${PACKAGE_NAME}
rm -f ${SIZEINFO}
${SIZEINFOCMD} -r ${rtyp}  1> ${SIZEINFO} 2>&1
sizcnt=`wc -l ${SIZEINFO}`
echo "/@@SIZE_INFO@@/a\\" 1>> ${SED_CMDS_FILE} 2>&1
cnt=1
cat ${SIZEINFO} |\
while read dir siz
do
        if [ ${cnt} -ne ${sizcnt} ]
        then
                echo "${dir} ${siz}\\" 1>> ${SED_CMDS_FILE} 2>&1
        else
                echo "${dir} ${siz}" 1>> ${SED_CMDS_FILE} 2>&1
        fi
	cnt=`expr ${cnt} + 1`
done
echo "/@@SIZE_INFO@@/d" 1>> ${SED_CMDS_FILE} 2>&1
# rm -f ${SIZEINFO}



#  Field Name   Format      Separator Description
#  15.          Described   New line  (Optional) Information on what the fileset
#  Supersede    later in              replaces. This field should exist in
#  information  this chapter          Version 3.2-formatted updates only. See
#                                     Supersede Information Section later in
#                                     this article for detailed description.
#               %           New line  Indicates the separation between supersede
#                                     and licensing information.
# 
SUPERSEDE_INFO="" # TBD
if [ "xx${SUPERSEDE_INFO}" = "xx" ]
then
        echo /@@SUPERSEDE_INFO@@/d 1>> ${SED_CMDS_FILE} 2>&1
else
        echo /@@SUPERSEDE_INFO@@/s//${SUPRESEDE_INFO}/ 1>> ${SED_CMDS_FILE} 2>&1
fi

if [ ${IFORLS} = "true" ]
then
#  Field Name   Format      Separator Description
#  16. iFOR/LS  Character   New line  (Optional) The vendor ID registered for
#  vendor ID                          iFOR/LS use as the owner of the product.
#                                     This field is used if the product uses
#                                     iFOR/LS license passwords. This field
#                                     should exist only if the fileset uses
#                                     iFOR/LS license passwords.
# 
IFORLS_VENDORID="" # TBD
echo /@@IFORLS_VENDORID@@/s//${IFORLS_VENDORID}/ 1>> ${SED_CMDS_FILE} 2>&1

#  Field Name   Format      Separator Description
#  17. iFOR/LS  Character   New line  The ID registered for iFOR/LS use as the
#  product ID                         ID of the product. This field should exist
#                                     only if the iFOR/LS vendor ID is present.
# 
IFORLS_PRODUCTID="" # TBD
echo /@@IFORLS_PRODUCTID@@/s//${IFORLS_PRODUCTID}/ 1>> ${SED_CMDS_FILE} 2>&1

#  Field Name   Format      Separator Description
#  18. iFOR/LS  Character   New line  The value registered for iFOR/LS use as
#  product                            the version of the product. This field
#  version                            should exist if the iFOR/LS product ID is
#                                     present.
#               %           New line  Indicates the separation between licensing
#                                     and fix information.
# 
IFORLS_VERSION="" # TBD
echo /@@IFORLS_VERSION@@/s//${IFORLS_VERSION}/ 1>> ${SED_CMDS_FILE} 2>&1
else # [ ${IFORLS} = "true" ]
        echo /@@IFORLS_VENDORID@@/d 1>> ${SED_CMDS_FILE} 2>&1
        echo /@@IFORLS_PRODUCTID@@/d 1>> ${SED_CMDS_FILE} 2>&1
        echo /@@IFORLS_VERSION@@/d 1>> ${SED_CMDS_FILE} 2>&1
fi   # [ ${IFORLS} = "true" ]

#  Field Name   Format      Separator Description
#  19. Fix      Described   New line  Information regarding the fixes contained
#  information  later in              in the fileset update. See Fix Information
#               article               Sectionlater in this article for detailed
#                                     description.
#               ]           New line  Indicates the end of the body of the
#                                     fileset information.
#               }           New line  Indicates the end of the repeatable
#                                     sections of fileset-specific information.
if [ "${ptyp}" != "I" ]
then
	FIX_INFO="" # TBD
	echo /@@FIX_INFO@@/s//${FIX_INFO}/ 1>> ${SED_CMDS_FILE} 2>&1
else
	echo /@@FIX_INFO@@/d 1>> ${SED_CMDS_FILE} 2>&1
fi

# generate lpp_name
LPP_NAME=./lpp_name
cat ${LPP_NAME_TMPL} | sed -f ${SED_CMDS_FILE} 1>  ${LPP_NAME} 2>&1
rm -f ${SED_CMDS_FILE}

exit 0
