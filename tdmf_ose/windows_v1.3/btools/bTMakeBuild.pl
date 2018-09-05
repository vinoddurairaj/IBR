#!/usr/bin/perl -w
use strict;

#####################################################################
#####################################################################
#
# bTMakeBuild.pl
#
#####################################################################
#####################################################################

use Getopt::Long;

my $Version   = "1,3,0";
my $BuildNo   = 0;
my $CVSBranch = "trunk";

GetOptions(
		"version=s"   => \$Version,
		"buildno=i"   => \$BuildNo,
		"cvsbranch=s" => \$CVSBranch,
		"usage"       => sub {usage()}
	);

#################################
# grab date array

my $datepackage='bDATE';
eval "require $datepackage";

####################################

open(TMP, "cd|");
<TMP> =~ /(^.+)\\tdmf_ose\\windows_v1.3\\btools/i;
my $Basedir = $1;
close(TMP);

####################################

print "START : \tscript bTMakeBuild.pl\n\n";
print "$Basedir\n\n";

####################################

if (GetCleanCopyOfFtdsrc() == 0)
{
	if (SetNewBuildNumber() == 0)
	{
		CvsTagBranch();
		if (CompileOnWindows() == 0)
		{
			system("bTPackageInstallation.pl");
		}
	}
}
else
{
	print "ERROR: Unable to proceed.\n";
}

####################################

print "\n\n\nEND : \tscript bTMakeBuild.pl\n\n\n\n";


#####################################################################
# Subs
#####################################################################

sub usage
{
	print "Usage: bmakeWin [options]\n\n";
	print "    --version\n";
	print "    --buildno\n";
	print "    --cvsbranch\n";
	print "    --usage\n";

	exit(0);
}

#####################################################################
# Delete ftdsrc directory and CVS Checkout

sub GetCleanCopyOfFtdsrc
{
	my $RC = 0;

	print "START : \tGet clean copy of ftdsrc\n\n";

	chdir $Basedir;

	system("rmdir /S /Q tdmf_ose\\windows_v1.3\\ftdsrc");
	my $CvsCmd = "cvs checkout " . (($CVSBranch eq "trunk") ? "" : "-r $CVSBranch") . " tdmf_ose/windows_v1.3/ftdsrc";
	system("$CvsCmd");
	$RC = $?;
	print "\n\t$CvsCmd rc = $RC\n\n";

	chdir "$Basedir\\tdmf_ose\\windows_v1.3\\bTools";

	print "END : \tGet clean copy of ftdsrc\n\n";

	return $RC;
}

#####################################################################
# Increment Build number, change in sources and commit in branch

sub SetNewBuildNumber
{
	my $RC = 0;

	print "START : \tSetNewBuildNumber\n\n";

	system("bTReplaceVersionInfo.pl -v $Version -b $BuildNo");

	chdir $Basedir;
	system("cvs commit -m \"$Version Build $BuildNo commit via bTMakeBuild.pl\" tdmf_ose/windows_v1.3");
	$RC = $?;
	print "\n\tcvs commit -m \"$Version Build $BuildNo commit via bTMakeBuild.pl tdmf_ose/windows_v1.3\"  rc = $RC\n\n";
	chdir "$Basedir\\tdmf_ose\\windows_v1.3\\bTools";

	print "END : \tSetNewBuildNumber\n\n";

	return $RC;
}

#####################################################################
# CVS Tag branch with build number

sub CvsTagBranch
{
	my $RC = 0;

	print "START : \tCvsTagBranch\n\n";

	chdir $Basedir;

	my $VersionText = $Version;
	$VersionText =~ s/\.//g;
	my $CVSNewTag  = "V${VersionText}B${BuildNo}";

	system("cvs tag $CVSNewTag tdmf_ose/windows_v1.3");
	$RC = $?;
	print "\n\tcvs tag $CVSNewTag  rc = $RC\n\n";

	chdir "$Basedir\\tdmf_ose\\windows_v1.3\\bTools";

	print "END : \tCvsTagBranch\n\n";

	return $RC;
}

#####################################################################
# Compile (bTMakeWin.pl)

sub CompileOnWindows
{
	my $RC = 0;

	print "START : \tscript bTMakeWin.pl (see BUILDLOG.txt)\n\n";

	system("bTMakeWin.pl -b $Basedir");
	$RC = $?;

	print "END : \tscript bTMakeWin.pl\n\n";

	return $RC;
}
