#!/usr/bin/local/perl -w
###################################
#################################
# perl -w ./bRdelpreviousbuild.pl
#
#	delete the previous build image :  disk space issue
#	with new build process.
#
#################################
###################################

###################################
#################################
# global var file
#################################
###################################

require "bRglobalvars.pm";
$buildnumber="$buildnumber";
$RFWver="$RFWver";

#################################
###################################

open (STDOUT, ">DELPREVIOUSBUILD.txt");
open (STDERR, ">>STDOUT");

print "\nSTART :  \tbRdelpreviousbuild.pl\n\n";

###################################
#################################
#  need to subtract 1 from current build :  new vars , etc....
#################################
###################################

$lastbuild=$buildnumber - 1;

print "\nlastbuild = $lastbuild\n";

#$lastbuild="0000";   # testing...
#$lastbuild="00000";   # testing...
#$lastbuild="000";   # testing...

print "\nlastbuild = $lastbuild\n";

if ("$lastbuild" =~ m/(\d\d\d\d\d)/ ) {
     print "\n\nIF = $lastbuild\n";
}
else {
    $lastbuild =~ s/$lastbuild/0$lastbuild/;
    print "\n\nELSE = $lastbuild\n";
}

$BlastBuild="B$lastbuild";

print "\n$BlastBuild = $BlastBuild\n";

chdir "..\\builds\\$BlastBuild" || die "chdir to ..\\builds\\$BlastBuild error : $!";

system qq($cd);
system qq($cd);

# make sure non-writeable issue is not present
system qq (attrib /S -R * );

print "\nRFW : $BlastBuild :  attrib -R (recurse) : rc = $?\n";

# rmdir is being called thru cygwin, no good, attempting rm -rf
#system qq(rmdir /S tdmf_ose  );
system qq( rm -rf  tdmf_ose);

$returncode="$?";

print "\nreturncode = $returncode\n";

print "\nRFW : $BlastBuild :  rm -rf source files (recurse) : rc = $returncode\n";

print "\nEND :  \tbRdelpreviousbuild.pl\n\n";

system qq( touch blat.empty.file );

system qq(blat.exe blat.empty.file -p iconnection -subject "$RFWver : $BlastBuild : rm -rf source tree : rc = $returncode"
			-to jdoll\@softek.com);


close (STDERR);
close (STDOUT);
