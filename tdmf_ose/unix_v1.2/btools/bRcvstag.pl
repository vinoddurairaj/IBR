#!/usr/bin/perl -Ipms
########################################
######################################
#
#  perl -w ./bRcvstag.pl
#
#     create build tag  :  per build
#
#     tag nomeclature : $COMver.YYYYMMDD.B####
#
#     $COMver represents the source trunk / branch level
#         YYYYMMDD represents the date (current)
#         B####    represents the build level number
#
#    GMT date derived via bRsetGMT.pl via pms/bRbuildGMT.pm
#
#    NOTE  :  for Linux, we are checking out 2 workareas  :
#
#         (1)  builds/export/$Bbnum  <<==  cvs export
#              required due to CVS/Entries files ending up in 
#              package if we use checkout
#
#         (2)  builds/checkout/$Bbnum  <<==  cvs checkout
#              used for cvs reports, etc, that require a workarea
#
######################################
########################################

########################################
######################################
# vars  /  pms  /  use
######################################
########################################

eval 'require bRglobalvars';
eval 'require bRcvstag';
eval 'require bRcvsprojects';
eval 'require bRpreviouscvstag';
eval 'require bRbuildGMT';
eval 'require bRpreviousbuildGMT';

our $CVSTAGKEEP="$CVSTAG";  #set var to previous tag  :  clean value
our $CVSTAG="\$CVSTAG";
our $previouscvstag;

our $COMver;
our $Bbnum;
our $projects;

our $currentbuildGMTYEAR;
our $currentbuildGMTMONTH;
our $currentbuildGMTDAY;

our $currentTagGMTDate="$currentbuildGMTYEAR$currentbuildGMTMONTH$currentbuildGMTDAY";
our $currentTag="$COMver+$currentTagGMTDate+$Bbnum";

########################################
######################################
# STDERR  /  STDOUT
######################################
########################################

open (STDOUT, ">logs/cvstag.txt");
open (STDERR, ">&STDOUT");

########################################
######################################
#  exec statements 
######################################
########################################

print "\n\nSTART  :  bRcvstag.pl\n\n";

########################################
######################################
#  log values 
######################################
########################################

print "\nCOMVer             =  $COMver\n";
print "\nBbnum              =  $Bbnum\n";
print "\ncurrentTagGMTDate  =  $currentTagGMTDate\n\n";
print "\ncurrentTag         =  $currentTag\n\n";

########################################
######################################
# keep the previous TAG for build report via bRparselog.pl
######################################
########################################

open (FHX,">pms/bRpreviouscvstag.pm");
print FHX "\$previouscvstag=\"$CVSTAGKEEP\"\;"; 
close (FHX);

########################################
######################################
# capture current build tag
######################################
########################################

open (FH1, ">pms/bRcvstag.pm");
print FH1 "$CVSTAG=\"$currentTag\"\;";
print "\nbRcvstag.pm : $CVSTAG=$currentTag\n";
close (FH1);

########################################
######################################
# commit modified *.pm's 
######################################
########################################

system qq(cvs commit -m "$Bbnum  :  auto build commit" pms/bRcvstag.pm pms/bRpreviouscvstag.pm );

print "\n\ncvs commit :  bRcvstag.pm & bRpreviouscvstag.pm  :  rc = $?\n\n";

########################################
######################################
#  cvs update -d   :  grab latest source
######################################
########################################

chdir "../../..";

system qq(pwd);

system qq(cvs update -d $projects);

########################################
######################################
#  create tag  :  tag trees 
######################################
########################################

$TAGIT="cvs -d $CVSROOT tag $currentTag $projects";
print "\nTAGIT = $TAGIT\n";

chomp ($TAGIT);

system qq($TAGIT);

print "\n\ncvs tag ftdsrc  :  rc = $?\n\n";

########################################
######################################
#  checkout tag  :  export tag
######################################
########################################

chdir "tdmf_ose/unix_v1.2";

system qq(pwd);

if (! -d "builds") {
   mkdir "builds";
}

chdir "builds";

system qq(pwd);

mkdir "$Bbnum";

chdir "$Bbnum";

system qq(pwd);

mkdir "export";

mkdir "checkout";

chdir "export";

system qq(pwd);

system qq(cvs -d $CVSROOT export -r $currentTag tdmf_ose/unix_v1.2);

chdir "../checkout";

system qq(pwd);

system qq(cvs -d $CVSROOT checkout -r $currentTag tdmf_ose/unix_v1.2);

########################################
########################################

print "\n\n\nEND  :  bRcvstag.pl\n\n\n";

########################################
########################################

close (STDERR);
close (STDOUT);

