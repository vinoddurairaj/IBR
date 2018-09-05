#!/usr/bin/perl -w
######################################
####################################
#
# ./bRbuildnumberinc.pl 
#
#	Build Number incrementor script
#
####################################
######################################

require "bRglobalvars.pm";


open (STDOUT, ">buildnumincLOG.txt");
open (STDERR, ">>&STDOUT");

print "\n\nSTART  :  script bRbuildnuminc.pl\n\n";

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

system qq(cvs commit -m "Build $buildnumber" bRglobalvars.pm);

system qq(cvs update);  #  should not hurt anything....might help....

print "\n\n\t end script bRbuildnuminc.pl\n\n";

close (STDERR);
close (STDOUT);
