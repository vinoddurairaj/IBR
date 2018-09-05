#!/usr/bin/local/perl -w
###################################
#################################
# bRMainTag.pl
#
#	run the windows build requirements
#
#	built from the TAG source / directory created during each build
#
#################################
###################################

###################################
#################################
# global var file
#################################
###################################

require "bRglobalvars.pm";
$branchdir="$branchdir"; 
$RFWver="$RFWver";       
$Bbnum="$Bbnum";          
$Version="$Version";
$Branch="$Branch";
$drive="$drive";

#################################
###################################

open (STDOUT, ">MAINTAG.txt");
open (STDERR, ">>STDOUT");

print "\n\nSTART :  bRMainTag.pl\n\n";

###################################
#################################
#   Windows :  build scripts :  required to build Replicator....
#################################
###################################

chdir "..\\builds\\$Bbnum\\tdmf_ose\\windows_v1.3\\btools";

system qq($cd);

###################################
#################################
#   system calls
#################################
###################################

#system qq(bRpkzip_DDK.pl);  #  DDK now in CVS ....
system qq(bRMake2k.pl);
system qq(bRxcopy_it.pl);
system qq(bRpkzip_exe.pl);
system qq(bRprebuild_OEM.pl);
system qq(bRis7x.pl);
system qq(bRpkzip_ALL.pl);
system qq(bRcreateISO.pl);
system qq(bRcvsWR.pl);
system qq(bRparlog.pl);
system qq(bRftpSAN.pl);
system qq(bRemail.pl);
system qq(bRmake2k_debug.pl);
system qq(bRparDEBUG.pl);
system qq(bRpkzip_DEBUG.pl);
system qq(bRftpDEBUG.pl);

#print "\nrc = $?\n";

system qq($cd);

print "\n\nEND :  bRMainTag.pl\n\n";

close (STDERR);
close (STDOUT);
