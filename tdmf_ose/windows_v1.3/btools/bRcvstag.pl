#!/usr/bin/perl -w
########################################
######################################
#
# 	perl -w ./bRcvstag.pl
#
#	create nightly build tag for Replicator 
#
#	tag nomeclature :  $RFWver.YYYYMMDD.B####
#
#		$RFWver represents version  
#		YYYYMMDD represents the date (current)
#		B####    represents the build level number
#
#	to format date, we are calling bDATE.pm module
#	sub get_date() returns date in YYYYMMDD format
#
######################################
########################################

########################################
######################################
# grab globals  : 
#  need the packaged global vars at this point in
#  script to get the appropriate label info
#  at the correct time
######################################
########################################

require "bRglobalvars.pm";
require "bRcvstag.pm";
require "bRcvslasttag.pm";
$CVSTAGKEEP="$CVSTAG";
$CVSTAG="\$CVSTAG";
$LASTCVSTAG="$LASTCVSTAG";
$buildnumber="$buildnumber";

########################################
######################################
# STDERR, STDOUT
######################################
########################################

open (STDOUT, ">CVSTAG.txt");
open (STDERR, "\&STDOUT");

########################################
######################################
# label stored as var in bVglobalvars.pm
#	grab, manipulate, recreate file....
# 
#	need to "commit" changed file
######################################
########################################

print "\n\nSTART script bRCVSTAG.pl\n\n";

# load bDATE module and call sub get_date

$datepackage='bDATE';

eval "require $datepackage";

@datevar=get_date();

# ensure format of the new tag

print "\nDATE FORMAT = $datevar[1]$datevar[0]$datevar[2]\n\n";
print "\nFULL TAG = \n\t$RFWver+$datevar[1]$datevar[0]$datevar[2]+$Bbnum\n\n";
print "\nFULL TAG = \n\t$RFWver+$datevar[1]$datevar[0]$datevar[2]+$Bbnum\n\n";

############################
# keep the last TAG for print via bRparselog.pl
############################

open (FHX,">bRcvslasttag.pm");

print FHX "\$LASTCVSTAG=\"$CVSTAGKEEP\"\;"; 

close (FHX);

############################
# capture the TAG
############################

open (FH1, ">bRcvstag.pm");

print FH1 "$CVSTAG=\"$RFWver+$datevar[1]$datevar[0]$datevar[2]+$Bbnum\"\;";
print "bRcvstag.pm : $CVSTAG=$RFWver+$datevar[1]$datevar[0]$datevar[2]+$Bbnum";

close (FH1);

############################
# commit tag .pm so that they are in the NEW build directory, oops
############################

system qq(cvs commit -m "Build $buildnumber commit via bRcvstag.pl :  buildcommitstring" bRcvstag.pm bRcvslasttag.pm);

print "\n\ncvs commit :  bRcvstag & bRcvslasttag.pm :  rc = $?\n\n";

############################
# get to root of required projects
############################

chdir "..\\..\\..";

system qq($cd);

print "\n\n";

#for testing, use the first entry....or second....

#system qq(cvs tag $RFWver+$datevar[1]$datevar[0]$datevar[2]+$Bbnum admintools tools);  # test with this proj ok....
#system qq(cvs tag $RFWver+$datevar[1]$datevar[0]$datevar[2]+$Bbnum+test00 admintools/misc tools);  # test with this proj ok....
system qq(cvs tag $RFWver+$datevar[1]$datevar[0]$datevar[2]+$Bbnum tdmf_ose/windows_v1.3 );

if ($? == "0" ) {
   print "\n\n\n\tcvs tag rc = $?\n\n\n";
   open (FH0, ">tdmf_ose\\windows_v1.3\\btools\\cvslasttag.txt");
   print FH0 "$RFWver+$datevar[1]$datevar[0]$datevar[2]+$Bbnum";
   close (FH0); 
}
else {
   print "\n\n\n\tcvs tag rc = $?\n\n\n";
   print "\n\ncvslasttag.txt : NOT updated via build $Bbnum :  be advised\n\n";
}

chdir "tdmf_ose\\windows_v1.3\\btools";

print "\n\n";

system qq($cd);

# need to close STDERR, STDOUT to blat.exe me the log
# to review status per build, email log with rc.

print "\n\n\nEND script bRcvstag.pl\n\n\n";

close (STDERR);
close (STDOUT);

system qq(blat.exe CVSTAG.txt  -p iconnection -subject "$RFWver build $Bbnum : cvs tag Log"
	-to jdoll\@softek.com);

