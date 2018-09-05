#!/usr/bin/perl -w
######################################
####################################
#
# ./bRpkzip_DEBUG.pl 
#
#	create $RFWver.$Bbnum.DEBUG.zip image
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

open (STDOUT, ">>$RFWver.$Bbnum.DEBUG.txt");
open (STDERR, "\&STDOUT");

print "\n\nSTART  :  bRpkzip_DEBUG.pl\n\n\n";

system qq($cd);

chdir "..\\ftdsrc\\TDMF" || die "bRpkzip_DEBUG.zip  :  errors(1)";

system qq($cd);

###################################
##################################
# pkzip25 DEBUG  :  deliverables 
##################################
####################################

system qq(pkzip25.exe -add -directories ..\\..\\..\\$RFWver.$Bbnum.DEBUG.zip Debug\\* W2K_Debug\\*);

print "\n\npkzip25.exe  :  DEBUG .zip  :  deliverables rc = $?\n\n"; 

print "\n\n\tEND \tscript bRpkzip_DEBUG.pl\n\n";

####################################
####################################

close (STDERR);
close (STDOUT);
