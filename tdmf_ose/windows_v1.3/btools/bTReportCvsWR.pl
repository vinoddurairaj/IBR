#!/usr/bin/local/perl -w
use strict;

###############################
#############################
#
# bTReportCvsWR.pl
#
#	grab WR info from "commited files" and
#	report WR list per build
#
#############################
###############################

use Getopt::Long;

####################################
# Read info from BuildInfo.dat file

my $Version   = "210";
my $BuildNo   = 0;

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
}
close(FILE);

$Version =~ s/\.//g;
$BuildNo--;

my $PreviousTag = "V${Version}B${BuildNo}";

####################################
# Read args

GetOptions(
		"Tag=s" => \$PreviousTag,
		"usage" => sub {usage()}
	);

####################################
# Extract basedir

open(TMP, "cd|");
<TMP> =~ /(^.+)\\tdmf_ose\\windows_v1.3\\btools/i;
my $Basedir = $1;
close(TMP);

####################################

open (STDOUT, ">WRsReport.txt");
open (STDERR, ">>&STDOUT");

print "Previous Tag:  $PreviousTag \n\n";

my @Files;
my @list= qx(cvs -Q rdiff -s -f -r $PreviousTag tdmf_ose/windows_v1.3);

foreach my $File (@list)
{
	if ($File =~ m/^\s*File\s+(.*)\s+(changed from revision|is remove|is new).*$/)
	{
		push (@Files, $1);
	}
}

foreach my $Filename (@Files)
{
	print "\nProcessing file: $Filename\n\n";

	my $OldFileVersion;
	my $CurrentFileVersion;

	my @CvsStatus = qx(cvs status \"$Basedir\\$Filename\");
	foreach my $Line (@CvsStatus)
	{
		if ( $Line =~ /^\s+Working revision:\s+(.*)\s*$/i)
		{
			$CurrentFileVersion = $1;
		}
	}

	my @CvsLogH = qx(cvs log -h \"$Basedir\\$Filename\");
	my $PrevLine;
	foreach my $Line (@CvsLogH)
	{
		if ($Line =~ /^\s+$PreviousTag:\s+(.*)\s*$/)
		{
			if ($PrevLine =~ /^\s+\w+:\s+(.*)\s*$/)
			{
				$OldFileVersion = $1;
			}
		}
		$PrevLine = $Line;
	}

	#print "\tOld File Version = $OldFileVersion\n";
	#print "\tCurrent File Version = $CurrentFileVersion\n";

	my @CvsLog = qx(cvs log -N -r$OldFileVersion:$CurrentFileVersion \"$Basedir\\$Filename\");

	my $Output = 0;
	foreach my $Line (@CvsLog)
	{
		if (!$Output && $Line =~ /^description:/i)
		{
			$Output = 1;
		}
		if ($Output)
		{
			if ($Line !~ /^=+$/)
			{
				print "\t$Line";
			}
		}
	}
}

close (STDERR);
close (STDOUT);


#####################################################################
# Subs
#####################################################################

sub usage
{
	print "Usage: bTReportCvsWR.pl [options]\n\n";
	print "    --Tag <Previous Tag>\n";
	print "    --usage\n";

	exit(0);
}
