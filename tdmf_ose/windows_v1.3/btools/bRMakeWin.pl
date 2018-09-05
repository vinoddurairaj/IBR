#!/usr/bin/perl -w
#use strict;

##################################
################################
#
# bmakeWin.pl
#
#	calls to msdev projects,
#	or makefile project for the 
#	2000 / NT environment
#
#
################################
##################################

require "bRglobalvars.pm";
$Bbnum="$Bbnum";

use Getopt::Long;

my $Basedir = "C:\\_SOURCES_";
my $Debug   = 0;
my $BuildOptions = "";

GetOptions(
		"basedir=s"  => \$Basedir,
		"debug"      => \$Debug,
		"rebuildall" => sub {$BuildOptions = "/REBUILD /NORECURSE"},
		"usage"      => sub {usage()}
	);

##################################
#
# STDERR / STDOUT to BUILDLOG.txt
#
##################################

unlink "BUILDLOG.txt";
open (STDOUT, ">RFWLOG.$Bbnum.txt");
open (STDERR, ">>&STDOUT");

##################################

print "\n\nstart bmakeWin.pl\n\n";

chdir "..\\ftdsrc";
system qq("cd");

####################################
##################################
#
#Error Var $errvar : set to project name
# 		if error encountered during compile
#		start with null value
#
##################################
####################################

my $errvar="";

my @Workspace = (
		["LogMsg",  "msdev tdmf.dsw /MAKE \"LogMsg",  "Win32 Release\"", "Win32 Debug\""],
		
		["libsock", "msdev tdmf.dsw /MAKE \"libsock", "Win32 Release\"", "Win32 Debug\""],
		["libcomp", "msdev tdmf.dsw /MAKE \"libcomp", "Win32 Release\"", "Win32 Debug\""],
		["liberr",  "msdev tdmf.dsw /MAKE \"liberr",  "Win32 Release\"", "Win32 Debug\""],
		["libmngt", "msdev tdmf.dsw /MAKE \"libmngt", "Win32 Release\"", "Win32 Debug\""],
		["liblic",  "msdev tdmf.dsw /MAKE \"liblic",  "Win32 Release\"", "Win32 Debug\""],
		["libftd",  "msdev tdmf.dsw /MAKE \"libftd",  "Win32 Release\"", "Win32 Debug\""],
		["libutil", "msdev tdmf.dsw /MAKE \"libutil", "Win32 Release\"", "Win32 Debug\""],
		["liblst",  "msdev tdmf.dsw /MAKE \"liblst",  "Win32 Release\"", "Win32 Debug\""],
		["libmd5",  "msdev tdmf.dsw /MAKE \"libmd5",  "Win32 Release\"", "Win32 Debug\""],
		["LibLic2", "msdev tdmf.dsw /MAKE \"LibLic2", "Win32 Release\"", "Win32 Debug\""],
		["libproc", "msdev tdmf.dsw /MAKE \"libproc", "Win32 Release\"", "Win32 Debug\""],
		["libDB",   "msdev tdmf.dsw /MAKE \"libDB",   "Win32 Release\"", "Win32 Debug\""],
		["libResMgr", "msdev tdmf.dsw /MAKE \"libResMgr", "Win32 Release\"", "Win32 Debug\""],
		
#		["license",         "msdev tdmf.dsw /MAKE \"license",        "Win32 Release\"", "Win32 Debug\""],
		["licinfo",         "msdev tdmf.dsw /MAKE \"licinfo",        "Win32 Release\"", "Win32 Debug\""],
		["hostinfo",        "msdev tdmf.dsw /MAKE \"hostinfo",       "Win32 Release\"", "Win32 Debug\""],
		["createkey",       "msdev tdmf.dsw /MAKE \"createkey",      "Win32 Release\"", "Win32 Debug\""],
		["override",        "msdev tdmf.dsw /MAKE \"override",       "Win32 Release\"", "Win32 Debug\""],
		["checkpoint",      "msdev tdmf.dsw /MAKE \"checkpoint",     "Win32 Release\"", "Win32 Debug\""],
		["debugcapture",    "msdev tdmf.dsw /MAKE \"debugcapture",   "Win32 Release\"", "Win32 Debug\""],
		["info",            "msdev tdmf.dsw /MAKE \"info",           "Win32 Release\"", "Win32 Debug\""],
		["init",            "msdev tdmf.dsw /MAKE \"init",           "Win32 Release\"", "Win32 Debug\""],
		["killbackfresh",   "msdev tdmf.dsw /MAKE \"killbackfresh",  "Win32 Release\"", "Win32 Debug\""],
		["killpmd",         "msdev tdmf.dsw /MAKE \"killpmd",        "Win32 Release\"", "Win32 Debug\""],
		["killrefresh",     "msdev tdmf.dsw /MAKE \"killrefresh",    "Win32 Release\"", "Win32 Debug\""],
		["killrmd",         "msdev tdmf.dsw /MAKE \"killrmd",        "Win32 Release\"", "Win32 Debug\""],
		["launchbackfresh", "msdev tdmf.dsw /MAKE \"launchbackfresh","Win32 Release\"", "Win32 Debug\""],
		["launchpmd",       "msdev tdmf.dsw /MAKE \"launchpmd",      "Win32 Release\"", "Win32 Debug\""],
		["launchrefresh",   "msdev tdmf.dsw /MAKE \"launchrefresh",  "Win32 Release\"", "Win32 Debug\""],
		["Perf",            "msdev tdmf.dsw /MAKE \"Perf",           "Win32 Release\"", "Win32 Debug\""],
		["reco",            "msdev tdmf.dsw /MAKE \"reco",           "Win32 Release\"", "Win32 Debug\""],
		["set",             "msdev tdmf.dsw /MAKE \"set",            "Win32 Release\"", "Win32 Debug\""],
		["start",           "msdev tdmf.dsw /MAKE \"start",          "Win32 Release\"", "Win32 Debug\""],
		["stop",            "msdev tdmf.dsw /MAKE \"stop",           "Win32 Release\"", "Win32 Debug\""],
		["trace",           "msdev tdmf.dsw /MAKE \"trace",          "Win32 Release\"", "Win32 Debug\""],

		["ReplServer",    "msdev tdmf.dsw /MAKE \"ReplServer",    "Win32 Release\"", "Win32 Debug\""],
		
		["TDMFInstall",   "msdev tdmf.dsw /MAKE \"TDMFInstall",   "Win32 Release\"", "Win32 Debug\""],
		["TdmfDbUpgrade", "msdev tdmf.dsw /MAKE \"TdmfDbUpgrade", "Win32 Release\"", "Win32 Debug\""],
		
		["Collector",     "msdev tdmf.dsw /MAKE \"Collector",     "Win32 Release\"", "Win32 Debug\""],

		["TDMFObjects",   "msdev tdmf.dsw /MAKE \"TDMFObjects",   "Win32 Release\"", "Win32 Debug\""],
		["TdmfCommonGui", "msdev tdmf.dsw /MAKE \"TdmfCommonGui", "Win32 Release\"", "Win32 Debug\""],
		["SFResDll",      "msdev tdmf.dsw /MAKE \"SFResDll",      "Win32 Release\"", "Win32 Debug\""],
		["SoftekRes",     "msdev tdmf.dsw /MAKE \"SoftekRes",     "Win32 Release\"", "Win32 Debug\""],
#		["Cfx4TDMFLang",  "msdev tdmf.dsw /MAKE \"Cfx4Lang",      "Win32 Release\"", "Win32 Debug\""],
		
		["driver_W2K",       "msdev tdmf.dsw /MAKE \"driver_W2K",       "Win32 Release\"", "Win32 Debug\""],
		["TDMFANALYZER.sys", "msdev tdmf.dsw /MAKE \"TDMFANALYZER.sys", "Win32 Release\"", "Win32 Debug\""],
		["tdmfanalyzer",     "msdev tdmf.dsw /MAKE \"tdmfanalyzer",     "Win32 Release\"", "Win32 Debug\""]
	);

my @NTWorkspace = (
		["LogMsg",  "msdev tdmf.dsw /MAKE \"LogMsg",  "Win32 Release\"", "Win32 Debug\""],
		
		["libsock", "msdev tdmf.dsw /MAKE \"libsock", "Win32 Release\"", "Win32 Debug\""],
		["libcomp", "msdev tdmf.dsw /MAKE \"libcomp", "Win32 Release\"", "Win32 Debug\""],
		["liberr",  "msdev tdmf.dsw /MAKE \"liberr",  "Win32 Release\"", "Win32 Debug\""],
		["libmngt", "msdev tdmf.dsw /MAKE \"libmngt", "Win32 Release\"", "Win32 Debug\""],
		["liblic",  "msdev tdmf.dsw /MAKE \"liblic",  "Win32 Release\"", "Win32 Debug\""],
		["libftd",  "msdev tdmf.dsw /MAKE \"libftd",  "Win32 Release\"", "Win32 Debug\""],
		["libutil", "msdev tdmf.dsw /MAKE \"libutil", "Win32 Release\"", "Win32 Debug\""],
		["liblst",  "msdev tdmf.dsw /MAKE \"liblst",  "Win32 Release\"", "Win32 Debug\""],
		["libmd5",  "msdev tdmf.dsw /MAKE \"libmd5",  "Win32 Release\"", "Win32 Debug\""],
		["LibLic2", "msdev tdmf.dsw /MAKE \"LibLic2", "Win32 Release\"", "Win32 Debug\""],
		["libproc", "msdev tdmf.dsw /MAKE \"libproc", "Win32 Release\"", "Win32 Debug\""],
		["libDB",   "msdev tdmf.dsw /MAKE \"libDB",   "Win32 Release\"", "Win32 Debug\""],
		["libResMgr", "msdev tdmf.dsw /MAKE \"libResMgr", "Win32 Release\"", "Win32 Debug\""],

#		["license",         "msdev tdmf.dsw /MAKE \"license",        "Win32 Release\"", "Win32 Debug\""],
		["licinfo",         "msdev tdmf.dsw /MAKE \"licinfo",        "Win32 Release\"", "Win32 Debug\""],
		["hostinfo",        "msdev tdmf.dsw /MAKE \"hostinfo",       "Win32 Release\"", "Win32 Debug\""],
		["createkey",       "msdev tdmf.dsw /MAKE \"createkey",      "Win32 Release\"", "Win32 Debug\""],
		["override",        "msdev tdmf.dsw /MAKE \"override",       "Win32 Release\"", "Win32 Debug\""],
		["checkpoint",      "msdev tdmf.dsw /MAKE \"checkpoint",     "Win32 Release\"", "Win32 Debug\""],
		["debugcapture",    "msdev tdmf.dsw /MAKE \"debugcapture",   "Win32 Release\"", "Win32 Debug\""],
		["info",            "msdev tdmf.dsw /MAKE \"info",           "Win32 Release\"", "Win32 Debug\""],
		["init",            "msdev tdmf.dsw /MAKE \"init",           "Win32 Release\"", "Win32 Debug\""],
		["killbackfresh",   "msdev tdmf.dsw /MAKE \"killbackfresh",  "Win32 Release\"", "Win32 Debug\""],
		["killpmd",         "msdev tdmf.dsw /MAKE \"killpmd",        "Win32 Release\"", "Win32 Debug\""],
		["killrefresh",     "msdev tdmf.dsw /MAKE \"killrefresh",    "Win32 Release\"", "Win32 Debug\""],
		["killrmd",         "msdev tdmf.dsw /MAKE \"killrmd",        "Win32 Release\"", "Win32 Debug\""],
		["launchbackfresh", "msdev tdmf.dsw /MAKE \"launchbackfresh","Win32 Release\"", "Win32 Debug\""],
		["launchpmd",       "msdev tdmf.dsw /MAKE \"launchpmd",      "Win32 Release\"", "Win32 Debug\""],
		["launchrefresh",   "msdev tdmf.dsw /MAKE \"launchrefresh",  "Win32 Release\"", "Win32 Debug\""],
		["Perf",            "msdev tdmf.dsw /MAKE \"Perf",           "Win32 Release\"", "Win32 Debug\""],
		["reco",            "msdev tdmf.dsw /MAKE \"reco",           "Win32 Release\"", "Win32 Debug\""],
		["set",             "msdev tdmf.dsw /MAKE \"set",            "Win32 Release\"", "Win32 Debug\""],
		["start",           "msdev tdmf.dsw /MAKE \"start",          "Win32 Release\"", "Win32 Debug\""],
		["stop",            "msdev tdmf.dsw /MAKE \"stop",           "Win32 Release\"", "Win32 Debug\""],
		["trace",           "msdev tdmf.dsw /MAKE \"trace",          "Win32 Release\"", "Win32 Debug\""],

		["ReplServer",    "msdev tdmf.dsw /MAKE \"ReplServer",    "Win32 Release\"", "Win32 Debug\""],

		["driver_NT4", "msdev tdmf.dsw /MAKE \"driver_NT4", "Win32 Release\"", "Win32 Debug\""]
	);

####################################
# Build W2k
####################################

SetEnv("W2K");

for my $Project (@Workspace) {
     
	print "\n\nbmakeWin.pl :  START : @$Project[0]\n\n\n";

	$errvar=@$Project[0];

	my $Cmd = @$Project[1] . " - " . ($Debug ? @$Project[3] : @$Project[2]) . " " . $BuildOptions;
	system qq($Cmd);

	if ($? != 0) {

		print "\n$errvar\n";
		print "\n\trc = $? \n";
		errorvar();
	}  # if end bracket
}

####################################
# Build NT
####################################

SetEnv("NT");

for my $NTProject (@NTWorkspace) {
     
	print "\n\nbmakeWin.pl :  START : @$NTProject[0]\n\n\n";

	$errvar=@$NTProject[0];

	my $Cmd = @$NTProject[1] . " - " . ($Debug ? @$NTProject[3] : @$NTProject[2]) . " /REBUILD /NORECURSE";
	system qq($Cmd);

	if ($? != 0) {

		print "\n$errvar\n";
		print "\n\trc = $? \n";
		errorvar();
	}  # if end bracket
}

####################################

print "\n\n\n\tEND :   script bmakeWin.pl\n\n\n\n";

close (STDERR);
close (STDOUT);

####################################
#
#  sub errorvar
#
#  if above system call are rc !=0, 
#  then this sub is called, and value
#  of $errorvar is printed to build log file
#  Error string will be parsed at end of build
#  	process.
#
####################################

sub errorvar {

print "\n$errvar may have had compile errors\n";
print "\n$errvar error reported from bmakeWin.pl\n";

$errvar="";

}

sub SetEnv {

	my $arg = shift;

	if ($arg =~ /NT/i)
	{
		$ENV{TOS}   = "NT4";
		$ENV{CL}	= "\@$Basedir\\tdmf_ose\\windows_v1.3\\ftdsrc\\tdmf_nt.cl";
	}
	else
	{
		$ENV{TOS}   = "W2K";
		$ENV{CL}	= "\@$Basedir\\tdmf_ose\\windows_v1.3\\ftdsrc\\tdmf.cl";
	}

	$ENV{BASEDIR}		= "C:\\NTDDK";
	$ENV{TOP}			= "$Basedir\\tdmf_ose\\windows_v1.3\\ftdsrc";

	$ENV{PRODUCTNAME}	= "TDMF";
	$ENV{CMDPREFIX}     = "dtc";
	$ENV{LIBLICDBG}		= "lib\\liblic\\Debug\\liblic.lib";
	$ENV{LIBLIC2DBG}	= "lib\\liblic2\\Debug\\liblic2.lib";
	$ENV{LIBMD5DBG}		= "lib\\libmd5\\Debug\\libmd5.lib";
	$ENV{LIBFTDDBG}		= "lib\\libftd\\Debug\\libftd.lib";
	$ENV{LIBSOCKDBG}	= "lib\\libsock\\Debug\\libsock.lib";
	$ENV{LIBCOMPDBG}	= "lib\\libcomp\\Debug\\libcomp.lib";
	$ENV{LIBLSTDBG}		= "lib\\liblst\\Debug\\liblst.lib";
	$ENV{LIBPROCDBG}	= "lib\\libproc\\Debug\\libproc.lib";
	$ENV{LIBUTILDBG}	= "lib\\libutil\\Debug\\libutil.lib";
	$ENV{LIBERRDBG}		= "lib\\liberr\\Debug\\liberr.lib";
	$ENV{LIBMNGTDBG}	= "lib\\libmngt\\Debug\\libmngt.lib";
	$ENV{LIBDBDBG}		= "lib\\libdb\\Debug\\libdb.lib";
	$ENV{LIBLICREL}		= "lib\\liblic\\Release\\liblic.lib";
	$ENV{LIBLIC2REL}	= "lib\\liblic2\\Release\\liblic2.lib";
	$ENV{LIBMD5REL}		= "lib\\libmd5\\Release\\libmd5.lib";
	$ENV{LIBFTDREL}		= "lib\\libftd\\Release\\libftd.lib";
	$ENV{LIBSOCKREL}	= "lib\\libsock\\Release\\libsock.lib";
	$ENV{LIBCOMPREL}	= "lib\\libcomp\\Release\\libcomp.lib";
	$ENV{LIBLSTREL}		= "lib\\liblst\\Release\\liblst.lib";
	$ENV{LIBPROCREL}	= "lib\\libproc\\Release\\libproc.lib";
	$ENV{LIBUTILREL}	= "lib\\libutil\\Release\\libutil.lib";
	$ENV{LIBERRREL}		= "lib\\liberr\\Release\\liberr.lib";
	$ENV{LIBMNGTREL}	= "lib\\libmngt\\Release\\libmngt.lib";
	$ENV{LIBDBREL}		= "lib\\libdb\\Release\\libdb.lib";
}

sub usage
{
	print "Usage: bmakeWin [options]\n\n";
	print "    --basedir (default = \"C:\\_SOURCES_\")\n";
	print "    --debug\n";
	print "    --rebuildall\n";
	print "    --usage\n";

	exit(0);
}
