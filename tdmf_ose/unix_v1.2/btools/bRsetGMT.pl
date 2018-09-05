#!/usr/bin/perl -Ipms
#######################################
#####################################
#
#  perl -w ./bRsetGMT.pl
#
#    create and capture  : 
#        bRbuildGMT.pm          :  current  build time / date
#        bRpreviousbuildGMT.pm  :  previous build time / date
#
#####################################
#######################################
 
#######################################
#####################################
# vars  /  pms  /  use
#####################################
#######################################

use strict;

our $currentbuildGMTYEAR;
our $currentbuildGMTMONTH;
our $currentbuildGMTDAY;
our $currentbuildGMT;
our $previousbuildGMTYEAR;
our $previousbuildGMTMONTH;
our $previousbuildGMTDAY;
our $previousbuildGMT;
our $GMTMINUTE;
our $GMTHOUR;
our $GMTYEAR;
our $GMTMONTH;
our $GMTDAY;

eval 'require bRglobalvars';
eval 'require bRbuildGMT';
eval 'require bDATE';

our $COMver;
our $Bbnum;

#######################################
#####################################
#  STDERR  /  STDOUT
#####################################
#######################################

open (STDOUT, ">logs/setGMT.txt");
open (STDERR, ">&STDOUT");

#######################################
#####################################
#  exec statements
#####################################
#######################################

print "\n\nSTART  :  bRsetGMT.pl\n\n";

#######################################
#####################################
#  log values
#####################################
#######################################

print "\n\nCOMver  =  $COMver\n\n";
print "\n\nBbnum   =  $Bbnum\n\n";

#######################################
#####################################
#  set previous build GMT Time to pms/bRpreviousbuildGMT.pm  
#####################################
#######################################

$previousbuildGMTYEAR="$currentbuildGMTYEAR";
$previousbuildGMTMONTH="$currentbuildGMTMONTH";
$previousbuildGMTDAY="$currentbuildGMTDAY";
$previousbuildGMT="$currentbuildGMT";

print "\npreviousbuildGMTYEAR   =  $previousbuildGMTYEAR\n";
print "\npreviousbuildGMTMONTH  =  $previousbuildGMTMONTH\n";
print "\npreviousbuildGMTDAY    =  $previousbuildGMTDAY\n";
print "\npreviousbuildGMT       =  $previousbuildGMT\n";

open(FHprevious,">pms/bRpreviousbuildGMT.pm");
print FHprevious "\$previousbuildGMTYEAR=\"$previousbuildGMTYEAR\"\;\n";
print FHprevious "\$previousbuildGMTMONTH=\"$previousbuildGMTMONTH\"\;\n";
print FHprevious "\$previousbuildGMTDAY=\"$previousbuildGMTDAY\"\;\n";
print FHprevious "\$previousbuildGMT=\"$previousbuildGMT\"\;";
close (FHprevious);

#######################################
#####################################
#  sub get_GMT_date  
#    bDATE.pm  
#####################################
#######################################

get_GMT_date();

print "\ncurrent GMTMINUTE    :  $GMTMINUTE\n";
print "\ncurrent GMTHOUR      :  $GMTHOUR\n";
print "\ncurrent GMTYEAR      :  $GMTYEAR\n";
print "\ncurrent GMTMONTH     :  $GMTMONTH\n";
print "\ncurrent GMTDAY       :  $GMTDAY\n";

#######################################
#####################################
#  set current GMT Time  to  pms/bRbuildGMT.pm  
#####################################
#######################################

$currentbuildGMT="$GMTHOUR:$GMTMINUTE";

open(FHcurrent,">pms/bRbuildGMT.pm");
print FHcurrent "\$currentbuildGMTYEAR=\"$GMTYEAR\"\;\n";
print FHcurrent "\$currentbuildGMTMONTH=\"$GMTMONTH\"\;\n";
print FHcurrent "\$currentbuildGMTDAY=\"$GMTDAY\"\;\n";
print FHcurrent "\$currentbuildGMT=\"$GMTHOUR:$GMTMINUTE\"\;";
close (FHcurrent);

#######################################
#####################################
#  cvs commit   
#####################################
#######################################

system qq(cvs commit -m "$Bbnum  :  autobuild commit" pms/bRbuildGMT.pm);
system qq(cvs commit -m "$Bbnum  :  autobuild commit" pms/bRpreviousbuildGMT.pm);

#######################################
#######################################

print "\n\nEND  :  bRsetGMT.pl\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);
