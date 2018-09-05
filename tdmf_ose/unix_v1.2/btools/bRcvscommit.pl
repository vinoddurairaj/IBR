#!/usr/bin/perl -Ipms
#####################################
###################################
#
#  bRcvscommit.pl
#
#	commit modified build scripts, modules
#
###################################
#####################################

#####################################
###################################
#  vars  /  pms  /  use
###################################
#####################################

eval 'require bRglobalvars';

our $COMver;
our $Bbnum;

#####################################
###################################
#  STDOUT  /  STDERR 
###################################
#####################################

open (STDOUT, ">logs/cvscommit.txt");
open (STDERR, ">>&STDOUT");

#####################################
###################################
#  exec statements 
###################################
#####################################

print "\n\nSTART  :  bRcvscommit.pl\n\n";

#####################################
###################################
#  logs values 
###################################
#####################################

print "\n\nCOMver  =  $COMver\n";
print "\nBbnum     =  $Bbnum\n\n";

system qq(pwd);
print "\n\n";

#####################################
###################################
#  hash  :  %commitfiles
#
#    some of these files should have been committed previously
#    in build process.  insurance step
###################################
#####################################

%commitfiles = (
"1", "pms/bRbuildGMT.pm",
"2", "pms/bRpreviousbuildGMT.pm",
"3", "notification.txt",
"4", "pms/bRsucFail.pm",
"5", "pms/bRpreviouscvstag.pm",
"6", "notification.txt",
"7", "pms/bRcvstag.pm");

#####################################
###################################
# process hash %commitfiles 
###################################
#####################################

foreach $value (values %commitfiles) {
   print "\nforeach : $value \n";
   system qq(cvs commit -m "$Bbnum  : autobuild commit" $value);
   system qq(ls -l $value);
}

#####################################
#####################################

print "\n\nEND  :   bSMcvscommit.pl\n\n";

#####################################
#####################################

close (STDERR);
close (STDOUT);

