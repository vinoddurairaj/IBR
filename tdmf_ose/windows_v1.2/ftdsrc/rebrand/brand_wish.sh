#!/bin/ksh

#
# rebrand wish(1) binary string editor
#
# works by looking at the default brand of 
# the wish(1) binary for strings that need
# to be modified for OEM environments and 
# their offsets.
#
# uses string offsets to compute 
# virtual addresses for use with adb(1).
# adb(1) commands are built and applied to copites
#
# reeeeeal tender...
#
# this will not work, of course, if the replacement
# strings are longer than the originals they're 
# to match.
#
# for fun, wrote this to work on the platform
# where each binary was compiled. this made for
# strange HP-UX and Solaris cases. HP-UX, naturally,
# is the ugliest.
#
# turns out that the AIX case will work on all
# of the others, but thought it better to work
# it out so that there wasn't that little dependency.

# platform
OSTYP=`uname -s`;

# where adb(1) lives
ADBPATH=/bin/adb
if [ ! -f ${ADBPATH} ]
then
	echo "No ${ADBPATH} for ${OSTYP}";
	echo "Use $0 on some OS that supports adb\(1\).";
	exit 1;
fi

# where sources and destinations live
WISHDIR="../libexec/wish";

#
# default brand, direct sales version (dtc)
#
QNAME=dtc;
CAPQNAME=DTC;
OEMNAME=LGTOdtc;
brand="DTCBRAND"; 

#
# all known brands...
# add yours here and below as needed...
#
ALLBRANDS="DTCBRAND MTIBRAND STKBRAND";

#
# usage
#
usg="\
Usage: $0 -b <brandname> \\n\
  where <brandname> is one of: \\n\
  ${ALLBRANDS}";

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
					DTCBRAND) 
						echo "$0: No need to rebrand ${brand}";exit 0;;
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
# whether default
#
if [ ${brand} = "DTCBRAND" ]
then
	echo "$0: No need to rebrand ${brand}";
	exit 0;
fi

# ascii character map

# Decimal - Character
# |  0 NUL|  1 SOH|  2 STX|  3 ETX|  4 EOT|  5 ENQ|  6 ACK|  7 BEL|
# |  8 BS |  9 HT | 10 NL | 11 VT | 12 NP | 13 CR | 14 SO | 15 SI |
# | 16 DLE| 17 DC1| 18 DC2| 19 DC3| 20 DC4| 21 NAK| 22 SYN| 23 ETB|
# | 24 CAN| 25 EM | 26 SUB| 27 ESC| 28 FS | 29 GS | 30 RS | 31 US |
# | 32 SP | 33  ! | 34  " | 35  # | 36  $ | 37  % | 38  & | 39  ' |
# | 40  ( | 41  ) | 42  * | 43  + | 44  , | 45  - | 46  . | 47  / |
# | 48  0 | 49  1 | 50  2 | 51  3 | 52  4 | 53  5 | 54  6 | 55  7 |
# | 56  8 | 57  9 | 58  : | 59  ; | 60  < | 61  = | 62  > | 63  ? |
# | 64  @ | 65  A | 66  B | 67  C | 68  D | 69  E | 70  F | 71  G |
# | 72  H | 73  I | 74  J | 75  K | 76  L | 77  M | 78  N | 79  O |
# | 80  P | 81  Q | 82  R | 83  S | 84  T | 85  U | 86  V | 87  W |
# | 88  X | 89  Y | 90  Z | 91  [ | 92  \ | 93  ] | 94  ^ | 95  _ |
# | 96  ` | 97  a | 98  b | 99  c |100  d |101  e |102  f |103  g |
# |104  h |105  i |106  j |107  k |108  l |109  m |110  n |111  o |
# |112  p |113  q |114  r |115  s |116  t |117  u |118  v |119  w |
# |120  x |121  y |122  z |123  { |124  | |125  } |126  ~ |127 DEL|

# ascii char code array for adb(1) command builder.
ascii="!\"#\$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_\`abcdefghijklmnopqrstuvwxyz";


# map strings from -> to according to OS type
case ${OSTYP} in

SunOS)

# unedited and edited wish(1) binaries
SRCWISH="${WISHDIR}/dtcwish.SunOS.sparc";
DSTWISH="${WISHDIR}/${QNAME}wish.SunOS.sparc";

# strings to replace
strs="
TCL_LIBRARY=/opt/LGTOdtc/lib/tcl8.0 TCL_LIBRARY=/opt/${OEMNAME}/lib/tcl8.0\n
TK_LIBRARY=/opt/LGTOdtc/lib/tk8.0 TK_LIBRARY=/opt/${OEMNAME}/lib/tk8.0\n
TIX_LIBRARY=/opt/LGTOdtc/lib/tix4.1 TIX_LIBRARY=/opt/${OEMNAME}/lib/tix4.1\n
BLT_LIBRARY=/opt/LGTOdtc/lib/blt2.4 BLT_LIBRARY=/opt/${OEMNAME}/lib/blt2.4";
;;

AIX)

# unedited and edited wish(1) binaries
SRCWISH="${WISHDIR}/dtcwish.AIX.rs6000";
DSTWISH="${WISHDIR}/${QNAME}wish.AIX.rs6000";

# strings to replace
strs="
/usr/dtc/lib /usr/${QNAME}/lib\n
/usr/dtc/lib/tcl8.0 /usr/${QNAME}/lib/tcl8.0\n
/usr/dtc/lib/tix4.1 /usr/${QNAME}/lib/tix4.1\n
/usr/dtc/lib/tix4.1 /usr/${QNAME}/lib/tix4.1\n
/usr/dtc/lib/blt2.4 /usr/${QNAME}/lib/blt2.4\n
/usr/dtc/lib:/usr/dtc/lib:/usr/dtc/lib:/usr/lib:/lib /usr/${QNAME}/lib:/usr/${QNAME}/lib:/usr/${QNAME}/lib:/usr/lib:/lib";;

HP-UX)

# unedited and edited wish(1) binaries
SRCWISH="${WISHDIR}/dtcwish.HP-UX.parisc";
DSTWISH="${WISHDIR}/${QNAME}wish.HP-UX.parisc";

# strings to replace
strs="
TCL_LIBRARY=/opt/LGTOdtc/lib/tcl8.0 TCL_LIBRARY=/opt/${OEMNAME}${QNAME}/lib/tcl8.0\n
TK_LIBRARY=/opt/LGTOdtc/lib/tk8.0 TK_LIBRARY=/opt/${OEMNAME}${QNAME}/lib/tk8.0\n
TIX_LIBRARY=/opt/LGTOdtc/lib/tix4.1 TIX_LIBRARY=/opt/${OEMNAME}${QNAME}/lib/tix4.1\n
BLT_LIBRARY=/opt/LGTOdtc/lib/blt2.4 BLT_LIBRARY=/opt/${OEMNAME}${QNAME}/lib/blt2.4"
;;

*) 
	echo "Unsupported OS type: ${OSTYP}";exit 1;;

esac


#
# find/replace instances of target string...
#

# where we're doin' what we're doin'...
echo "${SRCWISH} --> ${DSTWISH}";

# make a copy to apply adb(1) commands to...
rm -f ${DSTWISH}
cp ${SRCWISH} ${DSTWISH}


#
# away we go...
#
echo ${strs} |
while read tgt new
do

# commands strings filename
adbcmdsfil="/tmp/$$.adbcmds";
echo " ${tgt} --> ${new}";

###########################################################################

# SunOS  vers

# in SunOS, <addr>?v <0xbb> doesn't work as advertised,
# so use <addr>?w <0xwwww> rather. beware of this.
# also, need to determine virtual addresses from the
# physical addresses reported by strings(1).
# fairly straightforward.

###########################################################################
if [ "${OSTYP}" = "SunOS" ]
then

# funny thing about SunOS...
# get the base address of read only data
# need to relocate this section
eval `dump -h ${SRCWISH} | grep ".rodata1" |\
	 awk '{printf "readdr=%s; offset=%s", substr($4,3), substr($5,3)}'`;
readdr=`echo ${readdr} | tr '[a-z]' '[A-Z]'`;
offset=`echo ${offset} | tr '[a-z]' '[A-Z]'`;
reloff=`echo 16o 16i ${readdr} ${offset} - p q | dc`

strings -a -t x ${SRCWISH} | grep ${tgt} | \
while read off str 
do

	# only exact matches, please...
	if [ "${str}" != "${tgt}" ]
	then
		continue;
	fi

	# offset of target str
	canoff=`echo ${off} | tr '[a-z]' '[A-Z]'`;
	canoff=`echo 16o 16i ${canoff} ${reloff} + p q | dc`;
	decoff=`echo 16i ${canoff} p q | dc `;

	echo ${ascii} ${tgt} ${new} ${decoff} ${str} | 
	awk \
	'{ 
		ascii = $1;
		tgt = $2; 
		new = $3; 
		off = $4;
		str = $5
		tgtlen = length(tgt);
		newlen = length(new);
		strlen = length(str);
		newodd = newlen % 2;

		# characters of target string
		k = 1;
		while (k <= tgtlen ) {
			c[k] = sprintf("%s", substr(tgt, k, 1));
			k = k + 1;
		}

		# characters of replacement string
		k = 1;
		while (k <= newlen ) {
			n[k] = sprintf("%s", substr(new, k, 1));
			k = k + 1;
		}

		# characters of found string
		k = 1;
		while (k <= strlen ) {
			f[k] = sprintf("%s", substr(str, k, 1));
			k = k + 1;
		}

		# first character of target string in found string
		# printf "str: %s\n", str;
		# printf "tgt: %s\n", tgt;
		# printf "new: %s\n", new;
		# printf "off: %x\n", off;
		stroff = index(str, tgt);
		s = substr(str, stroff, tgtlen);
		# printf "stroff: %d\n", stroff;
		# printf "s: %s\n", s;

		# starting offset
		soff = stroff - 1 ;
		strlen = strlen - soff;
		# printf "soff: %d\n", soff;
		# printf "new strlen: %d\n", strlen;


		# zero fill the old string
		k = 1;
		offset = soff + off;
		while ( k <= strlen ) {
			printf "%x?w %02x%02x\n", offset, 0, 0;
			k = k + 2;
			offset = offset + 2;
		}

		# write the new string
		k = 1;
		offset = soff + off;
		while ( k <= newlen ) {
			ccode0 = index(ascii, n[k]) + 32;
			if ( (newodd == 1) && ((k + 2) > newlen)) {
				ccode1 = 0;
			}
			else
				ccode1 = index(ascii, n[k+1]) + 32;
			printf "%x?w %02x%02x\n", offset, ccode0, ccode1;
			k = k + 2;
			offset = offset + 2;
		}
		
}' >> ${adbcmdsfil};

# what we're gonna do...
echo 'appying adb(1) commands:';
cat ${adbcmdsfil} | awk '{printf "  %s\n", $0}'

adb -w ${DSTWISH} < ${adbcmdsfil}

# mop up
rm ${adbcmdsfil};

done # (while read off str do)

###########################################################################

# AIX vers

# AIX is nice, <addr>?v <0xbb> works as advertised,
# and no physical --> virtual translation is necessary.
# works on the HP-UX and Solaris binaries without
# modification.

###########################################################################
elif [ ${OSTYP} = "AIX" ]
then

# non SunOS case, ...

strings -a -t x ${SRCWISH} | egrep ${tgt} | \
while read off str
do

	# only exact matches, please...
	if [ "${str}" != "${tgt}" ]
	then
		continue;
	fi

	# offset of target str
	canoff=`echo ${off} | tr '[a-z]' '[A-Z]'`
	decoff=`echo 16i ${canoff} p q | dc`

	echo ${ascii} ${tgt} ${new} ${decoff} ${str} | 
	awk \
	'{ 
		ascii = $1;
		tgt = $2; 
		new = $3; 
		off = $4;
		str = $5
		tgtlen = length(tgt);
		newlen = length(new);
		strlen = length(str);

		# characters of target string
		k = 1;
		while (k <= tgtlen ) {
			c[k] = sprintf("%s", substr(tgt, k, 1));
			k = k + 1;
		}

		# characters of replacement string
		k = 1;
		while (k <= newlen ) {
			n[k] = sprintf("%s", substr(new, k, 1));
			k = k + 1;
		}

		# characters of found string
		k = 1;
		while (k <= strlen ) {
			f[k] = sprintf("%s", substr(str, k, 1));
			k = k + 1;
		}

		# first character of target string in found string
		# printf "str: %s\n", str;
		# printf "tgt: %s\n", tgt;
		# printf "new: %s\n", new;
		# printf "off: %x\n", off;
		stroff = index(str, tgt);
		s = substr(str, stroff, tgtlen);
		# printf "stroff: %d\n", stroff;
		# printf "s: %s\n", s;

		# starting offset
		soff = stroff - 1 ;
		strlen = strlen - soff;
		# printf "soff: %d\n", soff;
		# printf "new strlen: %d\n", strlen;

		# rewrite the found string
		offset = soff + off;
		# printf "new offset: %d\n", offset;
		k = 1;
		while ( k <= strlen ) {
			if ( k <= newlen ) {
				ccode = index(ascii, n[k]) + 32;
				printf "%x?V %x\n", offset, ccode;
			}
			else {
				printf "%x?V 0\n", offset;
			}
			k = k + 1;
			offset = offset + 1;
		}
		
}' >> ${adbcmdsfil};

# what we're gonna do...
echo 'appying adb(1) commands:';
cat ${adbcmdsfil} | awk '{printf "  %s\n", $0}'

adb -w ${DSTWISH} < ${adbcmdsfil}

# mop up
rm ${adbcmdsfil};

done # (while read off str do)

###########################################################################

# HP-UX vers

# HP-UX adb(1) barely works anything like as advertised.
# <addr>?W <0xwwwwwww> works as advertised,
# though physical --> virtual translation is necessary,
# and headachy fudges are required to boot...

###########################################################################

elif [ ${OSTYP} = "HP-UX" ]
then

# funny thing about HP-UX...
# get the base address of read only data
# need to relocate this section
eval `size -v -x ${SRCWISH} | grep '\\$DATA\\$' |\
		awk '{printf "readdr=%s; offset=%s", substr($4,3), substr($3,3)}'`;
readdr=`echo ${readdr} | tr '[a-z]' '[A-Z]'`;
offset=`echo ${offset} | tr '[a-z]' '[A-Z]'`;
reloff=`echo 16o 16i ${readdr} ${offset} - p q | dc`

strings -a -t x ${SRCWISH} | grep ${tgt} | \
while read off str 
do

	# only exact matches, please...
	if [ "${str}" != "${tgt}" ]
	then
		continue;
	fi

	# offset of target str
	canoff=`echo ${off} | tr '[a-z]' '[A-Z]'`;
	canoff=`echo 16o 16i ${canoff} ${reloff} + p q | dc`;
	decoff=`echo 16i ${canoff} p q | dc `;

	echo ${ascii} ${tgt} ${new} ${decoff} ${str} | 
	awk \
	'{ 
		ascii = $1;
		tgt = $2; 
		new = $3; 
		off = $4;
		str = $5
		tgtlen = length(tgt);
		newlen = length(new);
		strlen = length(str);
		newodd = newlen % 4;

		# characters of target string
		k = 1;
		while (k <= tgtlen ) {
			c[k] = sprintf("%s", substr(tgt, k, 1));
			k = k + 1;
		}

		# characters of replacement string
		k = 1;
		while (k <= newlen ) {
			n[k] = sprintf("%s", substr(new, k, 1));
			k = k + 1;
		}

		# characters of found string
		k = 1;
		while (k <= strlen ) {
			f[k] = sprintf("%s", substr(str, k, 1));
			k = k + 1;
		}

		# first character of target string in found string
		# printf "str: %s\n", str;
		# printf "tgt: %s\n", tgt;
		# printf "new: %s\n", new;
		# printf "off: %x\n", off;
		stroff = index(str, tgt);
		s = substr(str, stroff, tgtlen);
		# printf "stroff: %d\n", stroff;
		# printf "s: %s\n", s;

		# starting offset
		soff = stroff - 1 ;
		strlen = strlen - soff;
		# printf "soff: %d\n", soff;
		# printf "new strlen: %d\n", strlen;

		# zero fill the old string 
		k = 1;
		offset = soff + off + 2;
		while ( k <= strlen ) {
			printf "%x?W %02x%02x%02x%02x\n", offset, 0, 0, 0, 0;
			k = k + 4;
			offset = offset + 4;
		}

		# write the new string
		k = 1;
		offset = soff + off + 2;
		while ( k <= newlen ) {
				if ((k + 4) > newlen) {
					if (newodd == 0) {
						ccode0 = index(ascii, n[k]) + 32;
						ccode1 = index(ascii, n[k+1]) + 32;
						ccode2 = index(ascii, n[k+2]) + 32;
						ccode3 = index(ascii, n[k+3]) + 32;
					}
					if (newodd == 1) {
						ccode0 = index(ascii, n[k]) + 32;
						ccode1 = 0;
						ccode2 = 0;
						ccode3 = 0;
					}
					if (newodd == 2) {
						ccode0 = index(ascii, n[k]) + 32;
						ccode1 = index(ascii, n[k+1]) + 32;
						ccode2 = 0;
						ccode3 = 0;
					}
					if (newodd == 3) {
						ccode0 = index(ascii, n[k]) + 32;
						ccode1 = index(ascii, n[k+1]) + 32;
						ccode2 = index(ascii, n[k+2]) + 32;
						ccode3 = 0;
					}
				}
				else {
						ccode0 = index(ascii, n[k]) + 32;
						ccode1 = index(ascii, n[k+1]) + 32;
						ccode2 = index(ascii, n[k+2]) + 32;
						ccode3 = index(ascii, n[k+3]) + 32;
				}
				printf "%x?W %02x%02x%02x%02x\n", 
				       offset, ccode0, ccode1, ccode2, ccode3;
			k = k + 4;
			offset = offset + 4;
		}
		
}' >> ${adbcmdsfil};

# what we're gonna do...
echo 'appying adb(1) commands:';
cat ${adbcmdsfil} | awk '{printf "  %s\n", $0}'

adb -w ${DSTWISH} < ${adbcmdsfil}

# mop up
rm ${adbcmdsfil};

done # (while read off str do)

fi 

###########################################################################

done # (while read tgt new)

exit 0;
