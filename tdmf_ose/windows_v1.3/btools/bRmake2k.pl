#!/usr/bin/perl -w
######################################
####################################
#
# ./bRmake2k.pl 
#
#	make Release config deliverables
#
#
####################################
######################################

require "bRglobalvars.pm";
$Bbnum="$Bbnum";
$RFWver="$RFWver";
$COMPILERPATH="$COMPILERPATH";

####################################
#####################################

####################################
##################################
# grab STDERR, STDOUT
##################################
####################################

open (STDOUT, ">$RFWver.$Bbnum.txt");
open (STDERR, "\&STDOUT");

print "\n\nSTART \tscript bRmake2k.pl\n\n";

print "\n\nCOMPILER called  : \n$COMPILER\n$COMPILER\n\n";

chdir "..\\";

system qq($cd);

####################################
##################################
#  tdmf.sln  ( Release config  :  NT)
##################################
####################################

system qq($cd);

system qq($COMPILER tdmf.sln /BUILD Release /out NTLOG.txt );  # will default to local folder, in theory...

print "\ntdmf.sln : Release : rc = $?\n\n";

####################################
##################################
#  tdmf.sln  ( W2k_Release config  :  Win 2000)
##################################
####################################

system qq($cd);

system qq($COMPILER tdmf.sln /BUILD W2K_Release /out W2kLOG.txt );  # will default to local folder, in theory...

print "\ntdmf.sln : W2K_Release : rc = $?\n\n";

print "\n\n\tEND \tscript bRmake2k.pl\n\n";

####################################
####################################

close (STDERR);
close (STDOUT);
