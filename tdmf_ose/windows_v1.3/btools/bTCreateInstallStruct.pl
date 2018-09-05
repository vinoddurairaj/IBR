#!/usr/bin/perl -w
use strict;

##################################
################################
#
# bTCreateInstallStruct.pl
#
#
################################
##################################

use Getopt::Long;

my $OEM = "TDMF";

GetOptions(
		"oemname=s"  => \$OEM,
		"usage"      => sub {usage()}
	);

##################################
open(STDOUT, ">CreateInstallStruct.txt");
open(STDERR, ">&STDOUT");


print "START :   bTCreateInstallStruct.pl\n\n";

chdir "..\\ftdsrc";

##################################
# Create clean target directories

print "Cleanup and create directories\n\n";
system("rmdir /S /Q TDMF\\Collector");
system("rmdir /S /Q TDMF\\Gui");
system("rmdir /S /Q TDMF\\ReplicationServer");
system("rmdir /S /Q TDMF\\MSDE2KSP3");
system("rmdir /S /Q TDMF\\UninstallIcon");

mkdir "TDMF\\Collector";
mkdir "TDMF\\Gui";
mkdir "TDMF\\ReplicationServer";
mkdir "TDMF\\ReplicationServer\\win2000";
mkdir "TDMF\\ReplicationServer\\nt4";
mkdir "TDMF\\MSDE2KSP3";
mkdir "TDMF\\UninstallIcon";
mkdir "Installshield\\TDMF_OSE\\Setup Files";
mkdir "Installshield\\TDMF_OSE\\Setup Files\\Compressed Files";
mkdir "Installshield\\TDMF_OSE\\Setup Files\\Compressed Files\\Language Independent";
mkdir "Installshield\\TDMF_OSE\\Setup Files\\Compressed Files\\Language Independent\\OS Independent";

print "\nCleanup done!\n\n";

##################################
# Copy files

my @Files = (
		["TDMF\\W2K\\TdmfDbUpgrade.dll",       "\"Installshield\\TDMF_OSE\\Setup Files\\Compressed Files\\Language Independent\\OS Independent\""],
		["TDMF\\W2K\\TDMFInstall.dll",         "\"Installshield\\TDMF_OSE\\Setup Files\\Compressed Files\\Language Independent\\OS Independent\""],

		["TDMF\\W2K\\DtcLogMsg.dll",           "TDMF\\Collector"],
		["TDMF\\W2K\\DtcCollector.exe",        "TDMF\\Collector"],

		["TDMF\\W2K\\DtcCommonGui.exe",                   "TDMF\\GUI"],
		["TDMF\\W2K\\DtcObjects.dll",                     "TDMF\\GUI"],
		["TDMF\\W2K\\ContextMenus.reg",                   "TDMF\\GUI"],
		["gui\\TdmfCommonGui\\ChartFX\\Cfx4DtcLang.dll",  "TDMF\\GUI"],
		["gui\\TdmfCommonGui\\ChartFX\\Cfx4032.dll",      "TDMF\\GUI"],
		["gui\\TdmfCommonGui\\ChartFX\\Cfx4032.ocx",      "TDMF\\GUI"],
		["gui\\TdmfCommonGui\\ChartFX\\Cfx4Data.dll",     "TDMF\\GUI"],
		["gui\\TdmfCommonGui\\ChartFX\\Cfx4Ole.dll",      "TDMF\\GUI"],
		["gui\\TdmfCommonGui\\ChartFX\\SfxBar.dll",       "TDMF\\GUI"],
		["gui\\TdmfCommonGui\\ChartFX\\SfxXMLData.dll",   "TDMF\\GUI"],
		["TDMF\\W2K\\RBRes.dll",                          "TDMF\\GUI"],

		["TDMF\\NT4\\dtchostinfo.exe",        "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtckillbackfresh.exe",   "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtcinfo.exe",            "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtckillpmd.exe",         "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtcdebugcapture.exe",    "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtcLogMsg.dll",          "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtcinit.exe",            "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtcset.exe",             "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtcstop.exe",            "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtclaunchpmd.exe",       "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtcreco.exe",            "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtcstart.exe",           "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtckillrmd.exe",         "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtc_ReplServer.exe",     "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtctrace.exe",           "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtclaunchrefresh.exe",   "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtclaunchbackfresh.exe", "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtclicinfo.exe",         "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtccheckpoint.exe",      "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtckillrefresh.exe",     "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtcPerf.dll",            "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\dtcoverride.exe",        "TDMF\\ReplicationServer\\nt4"],
		["cp_post_on_000.bat",                "TDMF\\ReplicationServer\\nt4"],
		["cp_pre_on_000.bat",                 "TDMF\\ReplicationServer\\nt4"],
		["cp_pre_off_000.bat",                "TDMF\\ReplicationServer\\nt4"],
		["dtcboot.bat",                       "TDMF\\ReplicationServer\\nt4"],
		["lib\\libftd\\errors.MSG",           "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\NT4\\DtcBlock.sys",           "TDMF\\ReplicationServer\\nt4"],

		["TDMF\\W2K\\dtchostinfo.exe",        "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtckillbackfresh.exe",   "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtcinfo.exe",            "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtckillpmd.exe",         "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtcdebugcapture.exe",    "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtcLogMsg.dll",          "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtcinit.exe",            "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtcset.exe",             "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtcstop.exe",            "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtclaunchpmd.exe",       "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtcreco.exe",            "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtcstart.exe",           "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtckillrmd.exe",         "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtc_ReplServer.exe",     "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtctrace.exe",           "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtclaunchrefresh.exe",   "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtclaunchbackfresh.exe", "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtclicinfo.exe",         "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtcanalyzer.exe",        "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtccheckpoint.exe",      "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtckillrefresh.exe",     "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtcPerf.dll",            "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\dtcoverride.exe",        "TDMF\\ReplicationServer\\win2000"],
		["cp_post_on_000.bat",                "TDMF\\ReplicationServer\\win2000"],
		["cp_pre_on_000.bat",                 "TDMF\\ReplicationServer\\win2000"],
		["cp_pre_off_000.bat",                "TDMF\\ReplicationServer\\win2000"],
		["dtcboot.bat",                       "TDMF\\ReplicationServer\\win2000"],
		["lib\\libftd\\errors.MSG",           "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K\\DtcBlock.sys",          "TDMF\\ReplicationServer\\win2000"]
	);


my $Error = 0;

for my $File (@Files)
{
	print "@$File[0]: \t";

	my $Cmd = "copy @$File[0] @$File[1]";
	system qq($Cmd);

	if ($? != 0) 
	{
		$Error = 1;
	}

	print "\n";
}


##################################
# Copy local machine files (not in cvs)

print "MSDE2KSP3: \t";
system "xcopy C:\\TDMF_OSE\\MSDE2KSP3 TDMF\\MSDE2KSP3 /I /E /Y";
if ($? != 0) 
{
	$Error = 1;
}
print "\n";

print "dtc.ico: \t";
system "copy C:\\TDMF_OSE\\UninstallIcon\\sftk.ico TDMF\\UninstallIcon\\dtc.ico";
if ($? != 0) 
{
	$Error = 1;
}
print "\n";

print "pstore.txt: \t";
system "copy C:\\TDMF_OSE\\ReplicationServer\\nt4\\pstore.txt TDMF\\ReplicationServer\\nt4";
if ($? != 0) 
{
	$Error = 1;
}
print "\n";

####################################
# Re-branding

if ($OEM =~ /StoneFly/i)
{
	print "SFResDll.dll: \t";
	system("copy /Y TDMF\\W2K\\SFResDll.dll TDMF\\GUI\\RBRes.dll");
	if ($? != 0)
	{
		$Error = 1;
	}
	print "\n";

	print "StoneFly.ico: \t";
	system "copy /Y C:\\TDMF_OSE\\UninstallIcon\\StoneFly.ico TDMF\\UninstallIcon\\dtc.ico";
	if ($? != 0)
	{
		$Error = 1;
	}
	print "\n";

	print "dtc.ico: \t";
	system "copy /Y TDMF\\UninstallIcon\\dtc.ico \"Installshield\\TDMF_OSE\\Setup Files\\Uncompressed Files\\Language Independent\\OS Independent\\dtc.ico\"";
		if ($? != 0) 
	{
		$Error = 1;
	}
	print "\n";

	# re-icon win executables
	print "Reicon DtcCommonGui.exe: \t";
	system "reicon TDMF\\UninstallIcon\\dtc.ico TDMF\\GUI\\DtcCommonGui.exe #128";
	if ($? != 0)
	{
		$Error = 1;
	}
	print "\n";

	print "Reicon Analyzer.exe: \t";
	system "reicon TDMF\\UninstallIcon\\dtc.ico TDMF\\ReplicationServer\\win2000\\DtcAnalyzer.exe #101";
	if ($? != 0)
	{
		$Error = 1;
	}
	print "\n";

	print "ModifyIPR.exe: \t";
	system "ModifyIPR.exe Installshield\\TDMF_OSE\\TDMF_OSE.ipr Installshield\\TDMF_OSE\\StoneFlyChangesToIpr.txt";
	if ($? != 0)
	{
		$Error = 1;
	}
	print "\n";

	print "rebrandable.h: \t";
	system "copy /Y \"Installshield\\TDMF_OSE\\Script Files\\StoneFlyrebrandable.h\" \"Installshield\\TDMF_OSE\\Script Files\\rebrandable.h\"";
	if ($? != 0)
	{
		$Error = 1;
	}
	print "\n";
}

####################################

if ($Error)
{
	print "\nWarning:   Copying errors occurred\n";
}

####################################

print "\n\nEND :   bTCreateInstallStruct.pl\n\n\n\n";

close(STDOUT);
close(STDERR);

exit($Error);

####################################

sub usage
{
	print "Usage: bTCreateInstallStruct [options]\n\n";
	print "    --oemname <TDMF |StoneFly> (default = TDMF)\n";
	print "    --usage\n";

	exit(0);
}
