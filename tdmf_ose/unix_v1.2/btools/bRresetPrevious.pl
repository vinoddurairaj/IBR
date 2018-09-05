#!/usr/bin/perl -Ipms 
###################################################
#################################################
#  perl -w ./bRresetPrevious.pl
#
#    reset build values to previous build values
#    if failure / missing components are reported
#    in current build
#################################################
###################################################

###################################################
#################################################
#  vars  /  pms  / use
#################################################
###################################################

use strict;

eval 'require bRglobalvars';
eval 'require bRsucFail';
eval 'require bRpreviouscvstag';
eval 'require bRpreviousbuildGMT';

our $SUCCEED_FAILURE;
our $COMver;
our $RFXver;
our $TUIPver;
our $Bbnum;
our $reset;

our $previouscvstag;
our $previousbuildGMTYEAR;
our $previousbuildGMTMONTH;
our $previousbuildGMTDAY;
our $previousbuildGMT;

our $cvsmess="$Bbnum  :  autobuild commit  :  reset due to build status UNSTABLE";

###################################################
#################################################
#  STDOUT  /  STDERR
#################################################
###################################################

open (STDOUT,">logs/resetprevious.txt");
open (STDERR,">>&STDOUT");

###################################################
#################################################
#  exec statements
#################################################
###################################################

print "\n\nSTART  :  bRresetPrevious.pl\n\n";

###################################################
#################################################
#  log values
#################################################
###################################################

print "\n\nCOMver  =  $COMver\n";
print "\nRFXver    =  $RFXver\n";
print "\nTUIPver   =  $TUIPver\n";
print "\nBbnum     =  $Bbnum\n\n";

system qq(pwd);
print "\n\n";

chdir "pms";
system qq(pwd);
print "\n\n";

###################################################
#################################################
#  print previous variables  :  debug validation
#################################################
###################################################

print "\npreviouscvstag         =  $previouscvstag\n";
print "\npreviousbuildGMTYEAR   =  $previousbuildGMTYEAR\n";
print "\npreviousbuildGMTMONTH  =  $previousbuildGMTMONTH\n";
print "\npreviousbuildGMTDAY    =  $previousbuildGMTDAY\n";
print "\npreviousbuildGMT       =  $previousbuildGMT\n";

###################################################
#################################################
#  if $SUCCEDD_FAILURE matches  =~  UNSTABLE string 
#################################################
###################################################

if ("$SUCCEED_FAILURE" =~ m/UNSTABLE/ ) {
   $reset="1";
   print "\nIF  :  SUCCEED_FAILURE = $SUCCEED_FAILURE\n";
}

###################################################
#################################################
#  if $reset = 1  :  reset 
#################################################
###################################################

if ($reset == "1" ) {
   open (FHcvs,">bRcvstag.pm");
   print FHcvs "\$CVSTAG=\"$previouscvstag\"\;"; 
   print       "\n\$CVSTAG=\"$previouscvstag\"\;"; 
   close (FHcvs);
   open (FHgmt,">bRbuildGMT.pm");
   print FHgmt "\$currentbuildGMTYEAR=\"$previousbuildGMTYEAR\"\;\n"; 
   print       "\n\$currentbuildGMTYEAR=\"$previousbuildGMTYEAR\"\;\n"; 
   print FHgmt "\$currentbuildGMTMONTH=\"$previousbuildGMTMONTH\"\;\n"; 
   print       "\n\$currentbuildGMTMONTH=\"$previousbuildGMTMONTH\"\;\n"; 
   print FHgmt "\$currentbuildGMTDAY=\"$previousbuildGMTDAY\"\;\n";
   print       "\n\$currentbuildGMTDAY=\"$previousbuildGMTDAY\"\;\n"; 
   print FHgmt "\$currentbuildGMT=\"$previousbuildGMT\"\;\n";
   print       "\n\$currentbuildGMT=\"$previousbuildGMT\"\;\n"; 
   close (FHgmt);
   system qq(cvs commit -m "$cvsmess" bRcvstag.pm);
   system qq(cvs commit -m "$cvsmess" bRbuildGMT.pm);
}

###################################################
###################################################

print "\n\nEND  :  bRresetPrevious.pl\n\n";

###################################################
###################################################

close(STDERR);
close(STDOUT);

