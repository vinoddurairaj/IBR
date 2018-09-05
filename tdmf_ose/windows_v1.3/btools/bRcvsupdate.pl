#!/usr/local/bin/perl -w
###################################
#################################
#
#  perl -w .\bRcvsupdate.pl
#
#	cvs update Replicator source tree
#
#################################
###################################

require "bRglobalvars.pm";

open (STDOUT, ">cvsupdateLOG.txt");
open (STDERR, ">>&STDOUT");

print "\n\nbegin script bRcvsupdate.pl\n\n";

$buildnumber="$buildnumber";
$incnum=++$buildnumber;

print "\n\n\tBUILD $incnum \n\n\n";

chdir "..\\..";  #should be in \\dev\\$branchdir\\tdmf_ose

system ($cd);

system qq(cvs update -d windows_v1.3);

if ( $? == "0" ) {
   print "\n\ncvs update -d rc = $? \n\n";
}
else {
   print "\n\ncvs update -d may have errors rc = $? \n\n";
}

system ($cd);

print "\n\n\t end script bRcvsupdate.pl\n\n";

close (STDERR);
close (STDOUT);
