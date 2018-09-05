#!/usr/bin/perl -w
use strict;

#####################################################################
#####################################################################
#
# bTReplaceVersionInfo.pl
#
#####################################################################
#####################################################################

sub SetVersionInfo($$$);
sub SetBuildDefine($$$);


my $ProductVersion  = "1.3.0";  # overridable
my $BuildNo	        = 318;      # overridable
my $FileVersion     = "1,0,0,1";
my $ProductName     = "Softek Replicator";
my $CompanyName     = "Softek Technology Corporation";
my $LegalCopyright  = "Copyright 2001-2003";
my $LegalTrademarks = "";

#######################################

use Getopt::Long;

GetOptions(
		"version=s" => \$ProductVersion,
		"buildno=i" => \$BuildNo,
		"usage"     => sub {usage()}
	);

##################################

print "START :   bTReplaceVersionInfo.pl\n\n";

#######################################
# Replace Version Info in .rc files

open(TMP, "dir ..\\ftdsrc\\*.rc /S /B|");
while (<TMP>)
{
	chop; # remove last char: '\n'
	SetVersionInfo $_, $ProductVersion, $BuildNo;
}
close(TMP);

#######################################
# Replace Defined Values in .cl files

open(TMP, "dir ..\\ftdsrc\\*.cl /S /B|");
while (<TMP>)
{
	chop; # remove last char: '\n'
	SetBuildDefine $_, $ProductVersion, $BuildNo;
}
close(TMP);

#######################################

print "\n\n\n\tEND :   script bTReplaceVersionInfo.pl\n\n\n\n";


#####################################################################
# Subs
#####################################################################

sub usage
{
	print "Usage: bmakeWin [options]\n\n";
	print "    --version(product)\n";
	print "    --buildno\n";
	print "    --usage\n";

	exit(0);
}

sub SetVersionInfo($$$)
{
	my $File           = shift;
	my $ProductVersion = shift;
	my $BuildNo        = shift;
	my $Modif = 0;
	my $Error = 0;

	local $_;
	local $?;
	local $.;

	unlink "RcFile.tmp";
	open(TMPFILE, ">RcFile.tmp") or print "Unable to open Tmp file.\n";

	open(FILE, "$File") or $Error = 1;
	
	if ($Error)
	{
		print "Unable to open $File !\n";
	}
	else
	{
		while (<FILE>)
		{
			if (/^ +FILEVERSION/)
			{
				print TMPFILE " FILEVERSION $FileVersion\n";
				$Modif = 1;
			}
			elsif (/^ +PRODUCTVERSION/)
			{
				my $ProductVersionForRC = $ProductVersion;
				$ProductVersionForRC =~ s/,/\./g;
				print TMPFILE " PRODUCTVERSION $ProductVersionForRC\n";
				$Modif = 1;
			}
			elsif (/^ +VALUE \"CompanyName\"/)
			{
				print TMPFILE "            VALUE \"CompanyName\", \"$CompanyName\\0\"\n";
				$Modif = 1;
			}
			elsif (/^ +VALUE \"FileVersion\"/)
			{
				print TMPFILE "            VALUE \"FileVersion\", \"$FileVersion\\0\"\n";
				$Modif = 1;
			}
            elsif (/^ +VALUE \"ProductVersion\"/)
			{
				print TMPFILE "            VALUE \"ProductVersion\", \"$ProductVersion\\0\"\n";
				$Modif = 1;
			}
			elsif (/^ +VALUE \"SpecialBuild\"/)
			{
				print TMPFILE "            VALUE \"SpecialBuild\", \"$BuildNo\\0\"\n";
				$Modif = 1;
			}
			elsif (/^ +VALUE \"LegalCopyright\"/)
			{
				print TMPFILE "            VALUE \"LegalCopyright\", \"$LegalCopyright\\0\"\n";
				$Modif = 1;
			}
			elsif (/^ +VALUE \"LegalTrademarks\"/)
			{
				print TMPFILE "            VALUE \"LegalTrademarks\", \"$LegalTrademarks\\0\"\n";
				$Modif = 1;
			}
			elsif (/^ +VALUE \"ProductName\"/)
			{
				print TMPFILE "            VALUE \"ProductName\", \"$ProductName\\0\"\n";
				$Modif = 1;
			}
			else
			{
				print TMPFILE $_;
			}
		}
		close(FILE);
	}

	close(TMPFILE);

	if ($Modif)
	{
		#print "Modif \"$File\"\n";
		rename "RcFile.tmp", $File or print "Unable to replace $File.\n";
	}

	unlink "RcFile.tmp";
}


sub SetBuildDefine($$$)
{
	my $File           = shift;
	my $ProductVersion = shift;
	my $BuildNo        = shift;
	my $Error = 0;

	local $_;
	local $?;
	local $.;

	unlink "ClFile.tmp";
	open(TMPFILE, ">ClFile.tmp") or print "Unable to open Tmp file.\n";

	open(FILE, "$File") or $Error = 1;
	
	if ($Error)
	{
		print "Unable to open $File !\n";
	}
	else
	{
		while (<FILE>)
		{
			s/(^.*\/D WVERSION=L\\\"\")(\d+.\d+.\d+ Build \d*)(\"\\\".*$)/$1$ProductVersion Build $BuildNo$3/;
			s/(^.*\/D VERSION=\\\"\")(\d+.\d+.\d+ Build \d*)(\"\\\".*$)/$1$ProductVersion Build $BuildNo$3/;
			print TMPFILE $_;
		}
		close(FILE);
	}

	close(TMPFILE);

	#print "Modif \"$File\"\n";
	rename "ClFile.tmp", $File or print "Unable to replace $File.\n";
	unlink "ClFile.tmp";
}

__END__


====================
Typical Version Info
====================

VS_VERSION_INFO VERSIONINFO
 FILEVERSION 1,3,17,0
 PRODUCTVERSION 1,3,0,0
 FILEFLAGSMASK 0x3fL
#ifdef _DEBUG
 FILEFLAGS 0x1L
#else
 FILEFLAGS 0x0L
#endif
 FILEOS 0x4L
 FILETYPE 0x2L
 FILESUBTYPE 0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904b0"
        BEGIN
            VALUE "Comments", "\0"
            VALUE "CompanyName", "\0"
            VALUE "FileDescription", "TDMFObjects Module\0"
            VALUE "FileVersion", "1, 3, 17, 0\0"
            VALUE "InternalName", "TDMFObjects\0"
            VALUE "LegalCopyright", "Copyright 2002\0"
            VALUE "LegalTrademarks", "\0"
            VALUE "OLESelfRegister", "\0"
            VALUE "OriginalFilename", "TDMFObjects.DLL\0"
            VALUE "PrivateBuild", "\0"
            VALUE "ProductName", "TDMFObjects Module\0"
            VALUE "ProductVersion", "1, 3\0"
            VALUE "SpecialBuild", "\0"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x409, 1200
    END
END

====================
Typical .cl files
====================

/D SFTK /D WPRODUCTNAME=L\""TDMF"\" /D WVERSION=L\""1.3.21 Build "\" /D WQNM=L\""tdmf"\" /D VERSION=\""1.3.21 Build "
\" /D QNM=\""tdmf"\"  /D OEMNAME=\""Fujitsu Softek"\"
/D PRODUCTNAME=\""TDMF"\" /D MASTERNAME=\""TDMFServer"\" /D DRIVERNAME=\""TDMFblock"\"
/D COMPANYNAME=\""Fujitsu Softek"\"
