#!/usr/local/bin/perl -w
#######################################
#####################################
#
#  perl -w ./bRis7x.pl
#
#	create RFW installation package
#
#	create OEM installation package  
#		must run after bRprebuild_OEM.pl
#
#####################################
#######################################

#######################################
#####################################
# global vars
#####################################
#######################################

require "bRglobalvars.pm";
$Bbnum="$Bbnum";
$RFWver="$RFWver";

#######################################
#####################################
# capture stderr stdout 
#####################################
#######################################

open (STDOUT, ">>$RFWver.$Bbnum.txt");
open (STDERR, ">>&STDOUT");

print "\n\nSTART bRis7x.pl\n\n";

#######################################
#####################################
#
#  update "Build.bat" (ide generated script) with correct
#	build directory
#
#	match upon B\d\d\d\d\  substitute with $Bbnum	
#
#	must change the Build.bat template at branch point
#
#####################################
#######################################

open (FH1, "<Build.bat") || die "$RFWver :  pro 7x :  installshield build.bat  :  error(s)  occurred  :   $! ";
@buildbat=<FH1>;
close (FH1);

foreach $eb0 (@buildbat ) {
   #print "\nFOREACH : $eb0\n";

   if ("$eb0" =~ m/tdmf_ose/ && "$eb0" =~ m/B\d\d\d\d\d/ ) {
      print "\nIF : eb0 : $eb0\n";
      $eb0=~ s/B\d\d\d\d\d/$Bbnum/;
      #print "\nIF : AFTER : eb0 : $eb0\n";
      push (@pushbat,"$eb0");
   }
   else {
      #print "\nelse : eb0 : $eb0\n";
      push (@pushbat,"$eb0");
   }  
}

open (FH2X, ">Build.bat");
print FH2X @pushbat;
close (FH2X);

#######################################
#####################################
#
#   run build.bat (provided by installshield ide, executes compile and isbuild)
#
#####################################
#######################################

system qq(Build.bat);

#######################################
#####################################
#
#   OEM installation builds	
#
#   	run build_SF.bat (StoneFly)
#
#	build_SF.bat  :  created via bRprebuild_OEM.pl
#
#####################################
#######################################

print "\n\nOEM SECTION  :  bRis7x.pl  : FIRST\n\n";

system qq(BUILD_SF.bat);

print "\n\nOEM SECTION  :  bRis7x.pl  :  LAST\n\n";

print "\n\nEND bRis7x.pl\n\n";

#####################################
#####################################

close (STDERR);
close (STDOUT);
