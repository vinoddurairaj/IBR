#
# awk -f vars.awk file|- SUB=value...
#
# Whatever substitutions you want done you specify the values as parameters AFTER
# the file to be processed
#

function printError( message )
{
    print "ERROR: " message | "cat >&2"
    exit 1;
}

BEGIN \
{
    "date +%Y" | getline end_year
    for(i=2; i < ARGC; i++)
    {
	nsubs=split( ARGV[i], sub_variable, "=" );
	if (nsubs != 2)
	{
	    exit 1;
	}
	substitute[sub_variable[1]]=sub_variable[2];
	delete ARGV[i];
    }
    if ("PRODUCTBRAND" in ENVIRON) {
	productbrand=ENVIRON["PRODUCTBRAND"]
    } else {
	printError("vars.awk: Missing PRODUCTBRAND from the environment.");
        exit 1
    }
    if ("PKGIFYARCH" in ENVIRON) {
	architecture=tolower(ENVIRON["PKGIFYARCH"])
    } else {
	printError("vars.awk: Missing PKGIFYARCH from the environment.");
    }
    if ("PKGIFYNUMVERS" in ENVIRON) {
	numericversion=tolower(ENVIRON["PKGIFYNUMVERS"])
    } else {
	printError("vars.awk: Missing PKGIFYNUMVERS from the environment.");
    }
    if ("DISTRIBUTION" in ENVIRON) {
	distribution=ENVIRON["DISTRIBUTION"]
    } else {
	distribution="NOTSET"
    }
    if ("DISTRELEASE" in ENVIRON) {
	distrelease=ENVIRON["DISTRELEASE"]
    } else {
	distrelease="NOTSET"
    }
    
    print_line=1
    in_if=0
}

/^[ \t]*%[iI][fF][bB][rR][aA][nN][dD]/ {
    if (NF < 3) printError( "Invalid %ifbrand" )
    value=tolower($3)
    if ($2 == "==") {
	print_line = productbrand == value
    } else if ($2 == "!=" || $2 == "<>" ) {
	print_line = productbrand != value
    } else {
	printError( "Invalid %ifbrand conditional" )
    }
    in_if++
    next
}

/^[ \t]*%[iI][fF][dD][iI][sS][tT][rR][iI][bB][uU][tT][iI][oO][nN]/ {
    if (NF < 3) printError( "Invalid %ifdistribution" )
    value=tolower($3)
    if ($2 == "==") {
	print_line = distribution == value
    } else if ($2 == "!=" || $2 == "<>" ) {
	print_line = distribution != value
    } else {
	printError( "Invalid %ifdistribution conditional" )
    }
    in_if++
    next
}

/^[ \t]*%[iI][fF][dD][iI][sS][tT][rR][eE][lL][eE][aA][sS][eE]/ {
    if (NF < 3) printError( "Invalid %ifdistrelease" )
    if ($3 !~ /[0-9]+/) printError( "Invalid %ifdistrelease release value" );
    if ($2 == "==") {
	for (i = 3; i <= NF; i++)
	{
	    print_line = distrelease == $i
	    if (print_line)
	    {
		break;
	    }
	}
    } else if ($2 == "!=" || $2 == "<>" ) {
	for (i = 3; i <= NF; i++)
	{
	    print_line = distrelease != $i
	    if (!print_line)
	    {
		break;
	    }
	}
	print_line = distrelease != $3
    } else if (NF == 3 && $2 == "<=") {
	print_line = distrelease <= $3
    } else if (NF == 3 && $2 == ">=") {
	print_line = distrelease >= $3
    } else if (NF == 3 && $2 == "<") {
	print_line = distrelease < $3
    } else if (NF == 3 && $2 == ">") {
	print_line = distrelease > $3
    } else {
	printError( "Invalid %ifdistrelease conditional" )
    }
    in_if++
    next
}

/^[ \t]*%[iI][fF][aA][rR][cC][hH]/ {
    if (NF < 3) printError( "Invalid %ifarch" )
    if ($2 == "==") {
	for (i = 3; i <= NF; i++)
	{
	    value=tolower($i)
	    print_line = architecture == value
	    if (print_line)
	    {
		break;
	    }
	}
    } else if ($2 == "!=" || $2 == "<>" ) {
	for (i = 3; i <= NF; i++)
	{
	    value=tolower($i)
	    print_line = architecture != value
	    if (!print_line)
	    {
		break;
	    }
	}
    } else {
	printError( "Invalid %ifarch conditional" );
    }
    in_if++
    next
}

/^[ \t]*%[iI][fF][vV][eE][rR][sS][iI][oO][nN]/ {
    if (NF < 3) printError( "Invalid %ifversion" )
    if ($3 !~ /[0-9]+/) printError( "Invalid %ifversion version value" );
    if ($2 == "==") {
	for (i = 3; i <= NF; i++)
	{
	    print_line = numericversion == $i
	    if (print_line)
	    {
		break;
	    }
	}
    } else if ($2 == "!=" || $2 == "<>") {
	for (i = 3; i <= NF; i++)
	{
	    print_line = numericversion != $i
	    if (!print_line)
	    {
		break;
	    }
	}
    } else if (NF == 3 && $2 == "<=") {
	print_line = numericversion <= $3
    } else if (NF == 3 && $2 == ">=") {
	print_line = numericversion >= $3
    } else if (NF == 3 && $2 == "<") {
	print_line = numericversion < $3
    } else if (NF == 3 && $2 == ">") {
	print_line = numericversion > $3
    } else {
	printError( "Invalid %ifversion conditional" );
    }
    in_if++
    next
}

/^[ \t]*%[eE][nN][dD][iI][fF]/ {
    print_line=1
    in_if--
    next
}

"PKGNM" in substitute && /%PKGNM%/ {
    gsub( /%PKGNM%/, substitute["PKGNM"], $0 )
}
"PRODUCTNAME" in substitute && /%PRODUCTNAME%/ {
    gsub( /%PRODUCTNAME%/, substitute["PRODUCTNAME"], $0 )
}
"PRODUCTNAME_TOKEN" in substitute && /%PRODUCTNAME_TOKEN%/ {
    gsub( /%PRODUCTNAME_TOKEN%/, substitute["PRODUCTNAME_TOKEN"], $0 )
}
"CAPPRODUCTNAME" in substitute && /%CAPPRODUCTNAME%/ {
    gsub( /%CAPPRODUCTNAME%/, substitute["CAPPRODUCTNAME"], $0 )
}
"TECHSUPPORTNAME" in substitute && /%TECHSUPPORTNAME%/ {
    gsub( /%TECHSUPPORTNAME%/, substitute["TECHSUPPORTNAME"], $0 )
}
"COMPANYNAME" in substitute && /%COMPANYNAME%/ {
    gsub( /%COMPANYNAME%/, substitute["COMPANYNAME"], $0 )
}
"COMPANYNAME2" in substitute && /%COMPANYNAME2%/ {
    gsub( /%COMPANYNAME2%/, substitute["COMPANYNAME2"], $0 )
}
"COMPANYNAME3" in substitute && /%COMPANYNAME3%/ {
    gsub( /%COMPANYNAME3%/, substitute["COMPANYNAME3"], $0 )
}
"OSRELEASE" in substitute && /%OSRELEASE%/ {
    gsub( /%OSRELEASE%/, substitute["OSRELEASE"], $0 )
}
"OSRELEASE" in substitute && /%PSFOSRELEASE%/ {
    gsub( /%PSFOSRELEASE%/, substr(substitute["OSRELEASE"],3), $0 )
}
"ARCH" in substitute && /%ARCH%/ {
    gsub( /%ARCH%/, substitute["ARCH"], $0 )
}
"PKGROOTDIR" in substitute && /%PKGROOTDIR%/ {
    gsub( /%PKGROOTDIR%/, substitute["PKGROOTDIR"], $0 )
}
"MODULENAME" in substitute && /%MODULENAME%/ {
    gsub( /%MODULENAME%/, substitute["MODULENAME"], $0 )
}
"FTDLIBEXECDIR" in substitute && /%FTDLIBEXECDIR%/ {
    gsub( /%FTDLIBEXECDIR%/, substitute["FTDLIBEXECDIR"], $0 )
}
"FTDBINDIR" in substitute && /%FTDBINDIR%/ {
    gsub( /%FTDBINDIR%/, substitute["FTDBINDIR"], $0 )
}
"FTDLIBDIR" in substitute && /%FTDLIBDIR%/ {
    gsub( /%FTDLIBDIR%/, substitute["FTDLIBDIR"], $0 )
}
"VERSION" in substitute && /%VERSION%/ {
    gsub( /%VERSION%/, substitute["VERSION"], $0 )
}
"REV" in substitute && /%REV%/ {
    gsub( /%REV%/, substitute["REV"], $0 )
}
"FIXNUM" in substitute && /%FIXNUM%/ {
    gsub( /%FIXNUM%/, substitute["FIXNUM"], $0 )
}
"VERSIONBUILD" in substitute && /%VERSIONBUILD%/ {
    gsub( /%VERSIONBUILD%/, substitute["VERSIONBUILD"], $0 )
}
"BUILDNUM" in substitute && /%BUILDNUM%/ {
    gsub( /%BUILDNUM%/, substitute["BUILDNUM"], $0 )
}
"OEM" in substitute && /%OEM%/ {
    gsub( /%OEM%/, substitute["OEM"], $0 )
}
"Q" in substitute && /%Q%/ {
    gsub( /%Q%/, substitute["Q"], $0 )
}
"PDFQ" in substitute && /%PDFQ%/ {
    gsub( /%PDFQ%/, substitute["PDFQ"], $0 )
}
"CAPQ" in substitute && /%CAPQ%/ {
    gsub( /%CAPQ%/, substitute["CAPQ"], $0 )
}
"QAGN" in substitute && /%QAGN%/ {
    gsub( /%QAGN%/, substitute["QAGN"], $0 )
}
"CAPQAGN" in substitute && /%CAPQAGN%/ {
    gsub( /%CAPQAGN%/, substitute["CAPQAGN"], $0 )
}
"OPTDIR" in substitute && /%OPTDIR%/ {
    gsub( /%OPTDIR%/, substitute["OPTDIR"], $0 )
}
"VAROPTDIR" in substitute && /%VAROPTDIR%/ {
    gsub( /%VAROPTDIR%/, substitute["VAROPTDIR"], $0 )
}
"ETCOPTDIR" in substitute && /%ETCOPTDIR%/ {
    gsub( /%ETCOPTDIR%/, substitute["ETCOPTDIR"], $0 )
}
"ETCINITDDIR" in substitute && /%ETCINITDDIR%/ {
    gsub( /%ETCINITDDIR%/, substitute["ETCINITDDIR"], $0 )
}
"ETCRSDDIR" in substitute && /%ETCRSDDIR%/ {
    gsub( /%ETCRSDDIR%/, substitute["ETCRSDDIR"], $0 )
}
"ETCR2DDIR" in substitute && /%ETCR2DDIR%/ {
    gsub( /%ETCR2DDIR%/, substitute["ETCR2DDIR"], $0 )
}
"ETCR3DDIR" in substitute && /%ETCR3DDIR%/ {
    gsub( /%ETCR3DDIR%/, substitute["ETCR3DDIR"], $0 )
}
"FTDCFGDIR" in substitute && /%FTDCFGDIR%/ {
    gsub( /%FTDCFGDIR%/, substitute["FTDCFGDIR"], $0 )
}
"FTDCONFDIR" in substitute && /%FTDCONFDIR%/ {
    gsub( /%FTDCONFDIR%/, substitute["FTDCONFDIR"], $0 )
}
"FTDVAROPTDIR" in substitute && /%FTDVAROPTDIR%/ {
    gsub( /%FTDVAROPTDIR%/, substitute["FTDVAROPTDIR"], $0 )
}
"OURCFGDIR" in substitute && /%OURCFGDIR%/ {
    gsub( /%OURCFGDIR%/, substitute["OURCFGDIR"], $0 )
}
"USRKERNELDRVDIR" in substitute && /%USRKERNELDRVDIR%/ {
    gsub( /%USRKERNELDRVDIR%/, substitute["USRKERNELDRVDIR"], $0 )
}
"FTDTIXBINPATH" in substitute && /%FTDTIXBINPATH%/ {
    gsub( /%FTDTIXBINPATH%/, substitute["FTDTIXBINPATH"], $0 )
}
"FTDTIXWISH" in substitute && /%FTDTIXWISH%/ {
    gsub( /%FTDTIXWISH%/, substitute["FTDTIXWISH"], $0 )
}
"USRSBINDIR" in substitute && /%USRSBINDIR%/ {
    gsub( /%USRSBINDIR%/, substitute["USRSBINDIR"], $0 )
}
"ROOTED" in substitute && /%ROOTED%/ {
    gsub( /%ROOTED%/, substitute["ROOTED"], $0 )
}
"GROUPNAME" in substitute && /%GROUPNAME%/ {
    gsub( /%GROUPNAME%/, substitute["GROUPNAME"], $0 )
}
"CAPGROUPNAME" in substitute && /%CAPGROUPNAME%/ {
    gsub( /%CAPGROUPNAME%/, substitute["CAPGROUPNAME"], $0 )
}
"GIFPREFIX" in substitute && /%GIFPREFIX%/ {
    gsub( /%GIFPREFIX%/, substitute["GIFPREFIX"], $0 )
}
"COPYRIGHTYEAR" in substitute && /%COPYRIGHTYEAR[ \t]+([0-9]+)[ \t]*%/ {
    while (match( $0, /%COPYRIGHTYEAR[ \t]+[0-9]+[ \t]*%/))
    {
	start_year=substr( $0, RSTART + 14, RLENGTH - 14 - 1 );
	gsub( /[ \t\n]+/, "", start_year );
	years=start_year
	if (expand_copyright_years)
	{
	    for (start_year++; start_year <= end_year; start_year++)
	    {
		years=years ", " start_year;
	    }
	}
	else
	{
	    if (start_year != end_year)
	    {
		years=years "-" end_year
	    }
	}
	sub( /%COPYRIGHTYEAR[ \t]+[0-9]+[ \t]*%/, years, $0)
    }
}
{ if (print_line) print $0 }
END {
    if (in_if) printError("brand.awk: Mismatch %endif");
}
