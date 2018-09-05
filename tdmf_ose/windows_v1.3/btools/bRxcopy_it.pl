#!/usr/bin/perl -w
##################################
################################
#
# bTxcopy_it.pl
#
#	modified version of bTcreateinstallstruct.pl   :  P. Go.
#
#	files are missing  :  renamed  :  etc
################################
##################################

use Getopt::Long;

my $OEM = "TDMF";

GetOptions(
		"oemname=s"  => \$OEM,
		"usage"      => sub {usage()}
	);

##################################
################################
#  STDOUT & STDERR  :  globals
################################
##################################

open(STDOUT, ">xcopy_it.txt");
open(STDERR, ">&STDOUT");

require "bRglobalvars.pm";
$cd="$cd";

print "\n\nSTART :   bRxcopy_it.pl\n\n";

chdir "..\\ftdsrc";

system qq ($cd);

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
		["TDMF\\W2K_Release\\DtcTdmfDbUpgrade.dll",       "\"Installshield\\TDMF_OSE\\Setup Files\\Compressed Files\\Language Independent\\OS Independent\""],
		#["TDMF\\W2K_Release\\TdmfDbUpgrade.dll",       "\"Installshield\\TDMF_OSE\\Setup Files\\Compressed Files\\Language Independent\\OS Independent\""],

		["TDMF\\W2K_Release\\DtcTDMFInstall.dll",         "\"Installshield\\TDMF_OSE\\Setup Files\\Compressed Files\\Language Independent\\OS Independent\""],
		#["TDMF\\W2K_Release\\TDMFInstall.dll",         "\"Installshield\\TDMF_OSE\\Setup Files\\Compressed Files\\Language Independent\\OS Independent\""],

		#missing
		#["TDMF\\W2K_Release\\DtcLogMsg.dll",           "TDMF\\Collector"],
		["TDMF\\W2K_Release\\DtcCollector.exe",        "TDMF\\Collector"],

		["TDMF\\W2K_Release\\DtcCommonGui.exe",                   "TDMF\\GUI"],

		["TDMF\\W2K_Release\\DtcObjects.dll",                     "TDMF\\GUI"],

		["gui\\TDMFCommonGui\\ContextMenus.reg",                   "TDMF\\GUI"],
		#["TDMF\\W2K_Release\\ContextMenus.reg",                   "TDMF\\GUI"],

		["gui\\TdmfCommonGui\\ChartFX\\Cfx4DtcLang.dll",  "TDMF\\GUI"],
		["gui\\TdmfCommonGui\\ChartFX\\Cfx4032.dll",      "TDMF\\GUI"],
		["gui\\TdmfCommonGui\\ChartFX\\Cfx4032.ocx",      "TDMF\\GUI"],
		["gui\\TdmfCommonGui\\ChartFX\\Cfx4Data.dll",     "TDMF\\GUI"],
		["gui\\TdmfCommonGui\\ChartFX\\Cfx4Ole.dll",      "TDMF\\GUI"],
		["gui\\TdmfCommonGui\\ChartFX\\SfxBar.dll",       "TDMF\\GUI"],
		["gui\\TdmfCommonGui\\ChartFX\\SfxXMLData.dll",   "TDMF\\GUI"],

		#rename
		["TDMF\\W2K_Release\\dtcSFresDLL.dll",                          "TDMF\\GUI"],
		#["TDMF\\W2K_Release\\RBRes.dll",                          "TDMF\\GUI"],

		["TDMF\\Release\\dtchostinfo.exe",        "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\Release\\dtckillbackfresh.exe",   "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\Release\\dtcinfo.exe",            "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\Release\\dtckillpmd.exe",         "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\Release\\dtcdebugcapture.exe",    "TDMF\\ReplicationServer\\nt4"],

		#missing
		#["TDMF\\Release\\dtcLogMsg.dll",          "TDMF\\ReplicationServer\\nt4"],

		["TDMF\\Release\\dtcinit.exe",            "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\Release\\dtcset.exe",             "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\Release\\dtcstop.exe",            "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\Release\\dtclaunchpmd.exe",       "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\Release\\dtcreco.exe",            "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\Release\\dtcstart.exe",           "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\Release\\dtckillrmd.exe",         "TDMF\\ReplicationServer\\nt4"],

		["TDMF\\Release\\dtcReplServer.exe",     "TDMF\\ReplicationServer\\nt4"],
		#["TDMF\\Release\\dtc_ReplServer.exe",     "TDMF\\ReplicationServer\\nt4"],

		["TDMF\\Release\\dtctrace.exe",           "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\Release\\dtclaunchrefresh.exe",   "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\Release\\dtclaunchbackfresh.exe", "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\Release\\dtclicinfo.exe",         "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\Release\\dtccheckpoint.exe",      "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\Release\\dtckillrefresh.exe",     "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\Release\\dtcPerf.dll",            "TDMF\\ReplicationServer\\nt4"],
		["TDMF\\Release\\dtcoverride.exe",        "TDMF\\ReplicationServer\\nt4"],
		["cp_post_on_000.bat",                "TDMF\\ReplicationServer\\nt4"],
		["cp_pre_on_000.bat",                 "TDMF\\ReplicationServer\\nt4"],
		["cp_pre_off_000.bat",                "TDMF\\ReplicationServer\\nt4"],
		["dtcboot.bat",                       "TDMF\\ReplicationServer\\nt4"],
		["lib\\libftd\\errors.MSG",           "TDMF\\ReplicationServer\\nt4"],

		["TDMF\\Release\\DtcBlock.sys",           "TDMF\\ReplicationServer\\nt4"],

		["TDMF\\W2K_Release\\dtchostinfo.exe",        "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K_Release\\dtckillbackfresh.exe",   "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K_Release\\dtcinfo.exe",            "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K_Release\\dtckillpmd.exe",         "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K_Release\\dtcdebugcapture.exe",    "TDMF\\ReplicationServer\\win2000"],

		#missing
		#["TDMF\\W2K_Release\\dtcLogMsg.dll",          "TDMF\\ReplicationServer\\win2000"],
		
		["TDMF\\W2K_Release\\dtcinit.exe",            "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K_Release\\dtcset.exe",             "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K_Release\\dtcstop.exe",            "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K_Release\\dtclaunchpmd.exe",       "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K_Release\\dtcreco.exe",            "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K_Release\\dtcstart.exe",           "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K_Release\\dtckillrmd.exe",         "TDMF\\ReplicationServer\\win2000"],
		
		["TDMF\\W2K_Release\\dtcReplServer.exe",     "TDMF\\ReplicationServer\\win2000"],
		#["TDMF\\W2K_Release\\dtc_ReplServer.exe",     "TDMF\\ReplicationServer\\win2000"],
		
		["TDMF\\W2K_Release\\dtctrace.exe",           "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K_Release\\dtclaunchrefresh.exe",   "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K_Release\\dtclaunchbackfresh.exe", "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K_Release\\dtclicinfo.exe",         "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K_Release\\dtcanalyzer.exe",        "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K_Release\\dtccheckpoint.exe",      "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K_Release\\dtckillrefresh.exe",     "TDMF\\ReplicationServer\\win2000"],
		["TDMF\\W2K_Release\\dtcPerf.dll",            "TDMF\\ReplicationServer\\win2000"],

		["TDMF\\W2K_Release\\dtcoverride.exe",        "TDMF\\ReplicationServer\\win2000"],

		["cp_post_on_000.bat",                "TDMF\\ReplicationServer\\win2000"],
		["cp_pre_on_000.bat",                 "TDMF\\ReplicationServer\\win2000"],
		["cp_pre_off_000.bat",                "TDMF\\ReplicationServer\\win2000"],
		["dtcboot.bat",                       "TDMF\\ReplicationServer\\win2000"],
		["lib\\libftd\\errors.MSG",           "TDMF\\ReplicationServer\\win2000"],

		["TDMF\\W2K_Release\\DtcBlock.sys",          "TDMF\\ReplicationServer\\win2000"]

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

system "xcopy \\TDMF_OSE\\MSDE2KSP3 TDMF\\MSDE2KSP3 /I /E /Y";
#system "xcopy C:\\TDMF_OSE\\MSDE2KSP3 TDMF\\MSDE2KSP3 /I /E /Y";
if ($? != 0) 
{
	$Error = 1;
}
print "\n";

print "dtc.ico: \t";
system "copy \\TDMF_OSE\\UninstallIcon\\sftk.ico TDMF\\UninstallIcon\\dtc.ico";
#system "copy C:\\TDMF_OSE\\UninstallIcon\\sftk.ico TDMF\\UninstallIcon\\dtc.ico";
if ($? != 0) 
{
	$Error = 1;
}
print "\n";

print "pstore.txt: \t";
system "copy \\TDMF_OSE\\ReplicationServer\\nt4\\pstore.txt TDMF\\ReplicationServer\\nt4";
#system "copy C:\\TDMF_OSE\\ReplicationServer\\nt4\\pstore.txt TDMF\\ReplicationServer\\nt4";
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
	system("copy /Y TDMF\\W2K_Release\\SFResDll.dll TDMF\\GUI\\RBRes.dll");
	if ($? != 0)
	{
		$Error = 1;
	}
	print "\n";

	print "StoneFly.ico: \t";
	system "copy /Y \\TDMF_OSE\\UninstallIcon\\StoneFly.ico TDMF\\UninstallIcon\\dtc.ico";
	#system "copy /Y C:\\TDMF_OSE\\UninstallIcon\\StoneFly.ico TDMF\\UninstallIcon\\dtc.ico";
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

print "\n\nEND :   bRxcopy_it.pl\n\n\n\n";

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
