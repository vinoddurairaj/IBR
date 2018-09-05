#!/usr/bin/perl -w
######################################
####################################
#
# ./bRpkzip_exe.pl 
#
#	create $RFWver.$Bbnum.RFWexe.zip image
#
#	contains the exe :  dll :  etc  :  deliverables from installshield 
#		target tree
#
####################################
######################################

require "bRglobalvars.pm";
$Bbnum="$Bbnum";
$RFWver="$RFWver";
$dirs3="$dirs3";

####################################
#####################################

####################################
##################################
# grab STDERR, STDOUT
##################################
####################################

open (STDOUT, ">>$RFWver.$Bbnum.txt");
open (STDERR, "\&STDOUT");

print "\n\nSTART  :  bRpkzip_exe.pl\n\n\n";

system qq($cd);

chdir "..\\ftdsrc\\TDMF" || die "bRpkzip_exe.zip  :  errors(1)";

system qq($cd);

####################################
##################################
# pkzip25 exe  :  installation deliverables  ==>> outside install package 
##################################
####################################

system qq(pkzip25.exe -add -directories -exclude=CVS* $dirs3\\$RFWver.$Bbnum.RFWexe.zip *);

print "\n\npkzip25.exe  :  exe .zip  :  install deliverables rc = $?\n\n"; 

print "\n\n\tEND \tscript bRpkzip_exe.pl\n\n";

####################################
####################################

close (STDERR);
close (STDOUT);
