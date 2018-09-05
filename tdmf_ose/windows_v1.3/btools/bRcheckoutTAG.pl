#!/usr/bin/perl -w
########################################
######################################
#
# 	perl -w ./bRcheckoutTAG.pl
#
#	create nightly build directory and all source via :
#
#	cvs checkout -r $CVSTAG projects list
#
#	$CVSTAG created via bRcvstag.pl
#	pulled from require "bRcvstag.pm";
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

########################################
######################################
# STDERR, STDOUT
######################################
########################################

open (STDOUT, ">CHECKOUTTAG.txt");
open (STDERR, "\&STDOUT");

########################################
######################################
# 
######################################
########################################

print "\n\nSTART script bRcheckoutTAG.pl\n\n";

chdir "..\\";

system qq($cd);

###############################
# mkdirs		(if neccessary)
###############################

if ( -d "builds") {
    print "\n$cd/builds exists\n";
}
else {
    mkdir "builds";
}

chdir "builds";

system qq($cd);

if ( -d "$Bbnum") {
    print "\n$cd/$Bbnum exists\n";
}
else {
    mkdir "$Bbnum";
}

chdir "$Bbnum";

system qq($cd);

#system qq(cvs checkout -r $CVSTAG admintools tools);  # test with this proj ok....
#testing ok if tag exists on these projects......or create one.....

system qq(cvs checkout -r $CVSTAG tdmf_ose/windows_v1.3);

if ($? == "0" ) {
   print "\n\n\n\tcvs checkout -r $CVSTAG rc = $?\n\n\n";
   }
else {
   print "\n\n\n\tcvs checkout -r $CVSTAG rc = $?\n\n\n";
}

system qq($cd);

# need to close STDERR, STDOUT to blat.exe me the log
# to review status per build, email log with rc.

print "\n\n\nEND script bRcheckoutTAG.pl\n\n\n";

close (STDERR);
close (STDOUT);

