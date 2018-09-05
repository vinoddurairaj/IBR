#!/usr/bin/local/perl -w
use strict;

###################################
#################################
# bTMain.pl
#
#	initial script
#
#   to get usage statement 
#	perl -w bMain.pl -u
#
# Working dir -> must be ...\bTools\
# CVS.exe -> in the path
# CVSROOT -> well defined
# CVS password must have been entered once
#
# The first time:
# 1- Checkout the branch
# 2- Modify the $Branch, $Version and $BuildNo in BuildInfo.dat file
# 3- Commit changes
# 4- run this script
# 
#################################
###################################

use Getopt::Long;

my $Rebuild = 0;
my $DontIncBuildNo = 0;

GetOptions(
		"rebuild"    => \$Rebuild,
		"nobuildInc" => \$DontIncBuildNo,
		"usage"      => sub {usage()}
	);

###################################

open(STDOUT, ">BUILD.txt");
open(STDERR, ">&STDOUT");

print "\nSTART : \tscript bTMain.pl\n\n";

#################################
# grab date array : used to exit build : on this date, no build run.....
#################################

my $datepackage='bDATE';
eval "require $datepackage";

my @datevar=get_date();

print "\nDATE = $datevar[1]/$datevar[0]/$datevar[2]\n\n";

#################################
# run bTUpdateBTools.pl, to update btools directory.
# Update of this script has to be done manually.
#################################

system qq(cvs update);

#################################
# BuildInfo.dat
#################################

my $Version = "1.3.0";
my $BuildNo = 0;
my $Branch  = "trunk";

# Read info from BuildInfo.dat file

open(FILE, "<BuildInfo.dat") or die "Missing BuildInfo.dat";
while(<FILE>)
{
	if (/^.*PRODUCTVERSION.*=\s*([\d,.]+)\s*.*$/)
	{
		$Version = $1;
	}
	if (/^.*BUILD.*=\s*(\d+)\s*.*$/)
	{
		$BuildNo = $1;
	}
	if (/^.*BRANCH.*=.*\"([\w\s]*)\".*$/)
	{
		$Branch = $1;
	}
}
close(FILE);

if (! $DontIncBuildNo)
{
	# Increment build number
	$BuildNo++;
	print "\nCreate Build# $BuildNo ...\n";

	# save back info
	open(FILE, ">BuildInfo.dat") or die "Can't create file BuildInfo.dat";
	print FILE "PRODUCTVERSION = $Version\n";
	print FILE "BUILD          = $BuildNo\n";
	if ($Branch ne "trunk")
	{
		print FILE "BRANCH         = \"$Branch\"\n";
	}
	close(FILE);

	system qq(cvs commit -m "Build number = $BuildNo");
}

#################################
# If it's not a forced rebuild, check for diff.
#################################

if (!$Rebuild)
{
	#################################
	# run bTRDiff.pl, Check if source change occurred since last tag setting
	#################################

	##system qq (perl bRdiff.pl);
	$? = 0;  ## TODO 

	print "\nbRdiff.pl returned  : \trc = $?\n";

	if ($? == "0" ) {

		$Rebuild = 1;
	}
}

if ($Rebuild)
{
	print "\n\nrunning the build .....\n\n";

	system("bTMakeBuild.pl -v \"$Version\" -b $BuildNo -c \"$Branch\"");
}
else {
	print "\n\nNO build ..... NO source changes .....\n\n";
}

print "\nEND : \tscript bTMain.pl\n\n";

close(STDOUT);
close(STDERR);

###################################
#
# sub routines
#
#####################################

sub usage {

print <<END;
bMain.pl usage :

bMain.pl [-b -r -u -n]

-r :  force a rebuild (even if sources have not changed).

-n :  No build No increment (not a real new build)

-u :  usage

END

exit(8);

}