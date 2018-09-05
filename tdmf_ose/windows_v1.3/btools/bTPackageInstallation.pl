#!/usr/bin/perl -w
use strict;

#####################################################################
#####################################################################
#
# btPackageInstallation.pl
#
#####################################################################
#####################################################################

use Getopt::Long;

my $OEM = "TDMF";
GetOptions(
		"oemname=s"   => \$OEM,
		"usage"       => sub {usage()}
	);

####################################
# Extract basedir

open(TMP, "cd|");
<TMP> =~ /(^.+)\\tdmf_ose\\windows_v1.3\\btools/i;
my $Basedir = $1;
close(TMP);

####################################
# Extract Buildno and CVSBranch

my $Version   = "210";
my $BuildNo   = 0;
my $CVSBranch = "trunk";

open(FILE, "<BuildInfo.dat") or die "Missing BuildInfo.dat";
while(<FILE>)
{
	if (/^.*PRODUCTVERSION.*=\s*(\d+)\.(\d+)\.(\d+)\s*.*$/)
	{
		$Version = "$1$2$3";
	}
	if (/^.*BUILD.*=\s*(\d+)\s*.*$/)
	{
		$BuildNo = $1;
	}
	if (/^.*BRANCH.*=.*\"([\w\s]*)\".*$/)
	{
		$CVSBranch = $1;
	}
}
close(FILE);

####################################

print "START : \tscript btPackageInstallation.pl\n\n";
print "$Basedir\n\n";

####################################

if (CreateInstallationPackage() == 0)
{
	PublishPackage();
}

####################################

print "\n\n\nEND : \tscript btPackageInstallation.pl\n\n\n\n";


#####################################################################
# Subs
#####################################################################

sub usage
{
	print "Usage: btPackageInstallation [options]\n\n";
	print "    --oemname <TDMF |StoneFly> (default = TDMF)\n";
	print "    --usage\n";

	exit(0);
}


#####################################################################
# Create Installation package

sub CreateInstallationPackage
{
	my $RC = 0;

	system("bTCreateInstallStruct.pl -o \"$OEM\"");
	$RC = $?;

	if ($RC == 0)
	{
		system("InstallShieldBuild.bat");
		$RC = $?;
	}

	return $RC;
}

#####################################################################
# Copy package to public folder

sub PublishPackage
{
	my $DstDir;

	mkdir "C:\\TDMF_OSE_BUILDS";
	$DstDir = "C:\\TDMF_OSE_BUILDS";
	if ($CVSBranch ne "trunk")
	{
		mkdir "$DstDir\\$CVSBranch";
		$DstDir = "$DstDir\\$CVSBranch";
	}
	if ($OEM ne "TDMF")
	{
		mkdir "$DstDir\\$OEM";
		$DstDir = "$DstDir\\$OEM";
	}
	mkdir "$DstDir\\$BuildNo";
	$DstDir = "$DstDir\\$BuildNo";

	print "Build $BuildNo: \t";
	system "xcopy \"$Basedir\\tdmf_ose\\windows_v1.3\\ftdsrc\\Installshield\\TDMF_OSE\\Media\\New CD-ROM Media\\Disk Images\\Disk1\\*.*\" $DstDir /I /E /Y /EXCLUDE:$Basedir\\tdmf_ose\\windows_v1.3\\btools\\ExcludeFiles.lst";
	if ($? != 0) 
	{
		print "Error: Can't copy new build.\n";
	}

	# Copy Help and Doc files
	print "Help and Doc files: \t";
	mkdir "$DstDir\\Help Files";
	mkdir "$DstDir\\Documentation";

	if ($OEM eq "TDMF")
	{
		system "copy C:\\TDMF_OSE\\Doc\\Replicator.chm \"$DstDir\\Help Files\"";
		if ($? != 0)
		{
			print "Error: Can't copy Help file.\n";
		}
	}
	elsif ($OEM eq "StoneFly") # StoneFly Help file
	{
		system "copy C:\\TDMF_OSE\\Doc\\Replicator.chm \"$DstDir\\Help Files\"";
		if ($? != 0)
		{
			print "Error: Can't copy Help file.\n";
		}
	}
	
	if ($OEM eq "TDMF")
	{
		system "copy \"C:\\TDMF_OSE\\Doc\\Documentation\\*.*\" \"$DstDir\\Documentation\"";
		if ($? != 0)
		{
			print "Error: Can't copy Doc files.\n";
		}
	}
	else # StoneFly Doc files
	{
	}

	# Build target name (for zip file and iso image)
	my $TargetName = "RFW$Version.B$BuildNo";
	if ($OEM ne "TDMF")
	{
		$TargetName = "$TargetName.$OEM";
	}

	# Zip files RFW$Version.B$BuildNo.$OEM.zip
	system "\"c:\\Program files\\WinZip\\wzzip\" -r -p C:\\TDMF_OSE_BUILDS\\Zip\\$TargetName.zip $DstDir\\*.*";
	if ($? != 0) 
	{
		print "Error: Can't create zip file.\n";
	}

	# Create ISO image
	my $VolumeId = "RFW$Version.B$BuildNo";
	system "C:\\TDMF_OSE\\Tools\\mkisofs.exe -J -l -V $VolumeId -v -o \"C:\\TDMF_OSE_BUILDS\\ISO images\\$TargetName.iso\" $DstDir";
	if ($? != 0) 
	{
		print "Error: Can't create ISO image.\n";
	}
}
