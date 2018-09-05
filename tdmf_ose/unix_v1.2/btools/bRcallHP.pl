#!/usr/bin/perl -Ipms
###############################################
#############################################
#
#  perl -w ./bRcallHP.pl
#
#	call ssh scripts
#
###########################################
#############################################

#############################################
###########################################
#  vars  /  pms  /  use
###########################################
#############################################

eval 'require bRglobalvars';

our $COMver;
our $Bbnum;

#############################################
###########################################
#  STDOUT / STDERR
###########################################
#############################################

open (STDOUT, ">logs/callHP.txt");
open (STDERR, ">>&STDOUT");

#############################################
###########################################
#  exec statements
###########################################
#############################################

print "\n\nSTART bRcallHP.pl\n\n"; 

#############################################
###########################################
#  log values
###########################################
#############################################

print "\n\nCOMver  =  $COMver\n";
print "\nBbnumr    =  $Bbnum\n\n";

chdir "unix/hp";
$PWD=qx|pwd|;
print "$PWD";

#############################################
###########################################
#  build HP
###########################################
#############################################

#system qq(perl -I../.. bRrshHP11.pl);
system qq(perl  bRrshHP11i.pl);
system qq(perl  bRsshHP11.23.PA.pl);
system qq(perl  bRsshHP11.23.IPF.pl);
system qq(perl  bRsshHP11.31.IPF.pl);
system qq(perl  bRsshHP11.31.PA.pl);

###########################################
###########################################

print "\n\nEND bRcallHP.pl\n\n"; 

###########################################
###########################################

close (STDERR);
close (STDOUT);
