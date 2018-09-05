#!/usr/bin/perl -w
######################################
####################################
#
# ./bRpkzip_DDK.pl 
#
#	unzip $RFWver.DDK.zip image
#		to build directory
#
####################################
######################################

require "bRglobalvars.pm";
$Bbnum="$Bbnum";
$RFWver="$RFWver";

####################################
#####################################

####################################
##################################
# grab STDERR, STDOUT
##################################
####################################

open (STDOUT, ">DDKzipLOG.txt");
open (STDERR, "\&STDOUT");

print "\n\nSTART  :  bRpkzip_DDK.pl\n\n\n";

system qq($cd);

chdir "..\\" || die "bRpkzip_DDK.zip  :  errors(1)";

system qq($cd);

####################################
##################################
# pkzip25 -extract -directories E:\\buildDDK\\$RFWver\\$RFWver.NETDDK.zip  
##################################
####################################

system qq(pkzip25 -extract -directories E:\\buildDDK\\$RFWver\\$RFWver.NETDDK.zip);

print "\n\npkzip25.exe  :  extract NETDDK  :  rc = $?\n\n"; 

print "\n\n\tEND \tscript bRpkzip_DDK.pl\n\n";

####################################
####################################

close (STDERR);
close (STDOUT);
