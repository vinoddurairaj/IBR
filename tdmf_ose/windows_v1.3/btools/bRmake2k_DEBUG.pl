#!/usr/bin/perl -w
######################################
####################################
#
# ./bRmake2k_DEBUG.pl 
#
#	make DEBUG config deliverables
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

open (STDOUT, ">$RFWver.$Bbnum.DEBUG.txt");
open (STDERR, "\&STDOUT");

print "\n\nSTART \tscript bRmake2k_DEBUG.pl\n\n";

print "\n\nCOMPILER called  : \n$COMPILER\n$COMPILER\n\n";

chdir "..\\";

system qq($cd);

####################################
##################################
#  tdmf.sln  ( DEBUG config  :  NT)
##################################
####################################

system qq($cd);

system qq($COMPILER tdmf.sln /BUILD Debug /out NTDebug.txt );  # will default to local folder, in theory...

print "\ntdmf.sln : Debug : rc = $?\n\n";

####################################
##################################
#  tdmf.sln  ( W2k_Debug config  :  Win 2000)
##################################
####################################

system qq($cd);

system qq($COMPILER tdmf.sln /BUILD W2K_Debug /out W2KDebug.txt );  # will default to local folder, in theory...

print "\ntdmf.sln : W2K_Debug : rc = $?\n\n";

print "\n\n\tEND \tscript bRmake2k_Debug.pl\n\n";

####################################
####################################

close (STDERR);
close (STDOUT);
