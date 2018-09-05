#!/usr/bin/perl -w
######################################
####################################
#
# ./bRbuildnumberinc.pl (to cvs commit the change)
#
#	Build Number Incrementor!
#
#	cvs commit bRglobalvars.pm before tagging....
#
####################################
######################################

#####################################
####################################
#  globals
####################################
#####################################

require "bRglobalvars.pm";

####################################
##################################
# STDOUT :  STDERR
##################################
####################################

open (STDOUT, ">buildnumincLOG.txt");
open (STDERR, ">>&STDOUT");

print "\n\nSTART  :  bRbuildnuminc.pl\n\n";

# increment build number by 1

$buildnumberinc=++$buildnumber;

print "\n$buildnumberinc current build number level\n";

open (NEWBN, "<bRglobalvars.pm");
@getbnum=<NEWBN>;
close (NEWBN);

open (WNEWBN, ">bRglobalvars.pm");

# match build level in global variable file, and replace
# with incremented number for use elsewhere.

foreach $elem (@getbnum) {
   if ($elem !~ m/\$buildnumber/) {
      #print "IF $elem\n";
      print WNEWBN "$elem";
   }
   else {
      #print "\nELSE $elem \n";
      print $elem if $elem =~ s/(\d+)/$buildnumberinc/;
      print WNEWBN "$elem";
   }
}

close (WNEWBN);

#commit new build number -- need it for unix and others
#	real early in our process.

system qq(cvs commit -m "Build run  :  $buildnumber" bRglobalvars.pm);

print "\n\n\t end script bRbuildnuminc.pl\n\n";

close (STDERR);
close (STDOUT);
