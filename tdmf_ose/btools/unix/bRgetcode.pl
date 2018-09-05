#!/usr/bin/perl -w
###############################################
#############################################
#
#  perl -w -I".." ./bRgetcode.pl
#
#	cvs update from ../ to get bRglobalvars.pm &
#		bRcvstag.pm
#
#	mkdir tdmf_ose/builds/$Bbnum
#
#	chdir tdmf_ose/builds/$Bbnum
#
#	cvs checkout -r $CVSTAG
#
#	build directory should be set for rest of project
#
#############################################
#############################################

#############################################
# get global vars, cvs commit occured to role $buildnumber on 2000machine
#	hence, $buildnumber should represent appropriate number
#############################################

#############################################
# grab the updated package pm files, etc.
############################################

chdir "..";   	#need to be in root of btools ==>> bRglobalvars.pm 

system qq(pwd);

system qq(cvs update);

#############################################
#############################################
# put globals after update or we regress 1 number o
#############################################
#############################################

require "bRglobalvars.pm";
require "bRcvstag.pm";
$Bbnum="$Bbnum";
$builds="builds";
$SUBPROJECT="$SUBPROJECT";

print "\n\nRFU :  build level $Bbnum\n\n";

print "\n\nSTART bRgetcode.pl\n\n"; 

chomp ($CVSTAG);

print "\n\nCVSTAG = \t$CVSTAG\n\n";

print "\nCVSROOT = $CVSROOT\n";
chomp ($CVSROOT);

#############################################
# get to root of tdmf_ose project directory
############################################

chdir "../";

system qq(pwd);

if ( ! -d "$builds" ) {
    print "\nIF :  mkdir $builds\n";
    system qq(mkdir "$builds");
}

chdir "builds";

system qq(pwd);

system qq(mkdir "$Bbnum");

chdir "$Bbnum";

system qq(pwd);

#############################################
# cvs checkout tag  (set via windows build) :  in bRcvstag.pm
#############################################

#$CVSROOT is set in bRglobalvars.pm #  not picking up $ENV{CVSROOT}

system qq(cvs -d $CVSROOT checkout -r $CVSTAG tdmf_ose/$SUBPROJECT tdmf_ose/btools );
#system qq(cvs -d $CVSROOT checkout -r $CVSTAG tdmf_ose/$SUBPROJECT tdmf_ose/btools );

print "\n\ncvs checkout -r $CVSTAG rc = $? \n\n";

print "\n\nEND bRgetcode.pl\n\n"; 

#############################################

close (STDERR);
close (STDOUT);

