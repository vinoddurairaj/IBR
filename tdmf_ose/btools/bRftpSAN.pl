#!/usr/local/bin/perl -w
#######################################
#####################################
#
#  perl -w ./bRftpSAN.pl
#
#	ftp deliverables to SAN server  : 
#	 
#	zip archive  :  $RFUver.$Bbnum.ALL.zip	
#	iso  :  $RFUver.$Bbnum.iso
#	logs, etc.
#
#####################################
#######################################
 
#######################################
#####################################
# get global vars 
#####################################
#######################################

require "bRglobalvars.pm";
$Bbnum="$Bbnum";
$RFUver="$RFUver";

#######################################
#####################################
# capture stderr stdout
#####################################
#######################################

open (STDOUT, ">ftpSAN.txt");
open (STDERR, ">&STDOUT");

print "\n\nSTART : \t bRftpSAN.pl\n\n";

#######################################
#####################################
#   ncftpput  :  -m attempts to mkdir remote dest dirs
#####################################
#######################################

chdir "..\\builds";  

system qq($cd);
system qq($cd);

system qq(ncftpput -f ..\\btools\\bmachine.txt -m /builds/TDMF-OSE/$RFUver/$Bbnum *.zip *.iso *.txt);

print "\n\n$RFUver  :  ftpSAN *.zip *.iso  :  rc = $?\n\n";

print "\n\nEND bRftpSAN.pl\n\n";

close (STDERR);
close (STDOUT);
