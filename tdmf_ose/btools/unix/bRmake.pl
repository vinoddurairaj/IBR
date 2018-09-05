#!/bin/perl
##################################################
################################################
#
#  ./bRmake.pl  
#
#	run development supplied  :  sol.bldcmd  (which runs make)
#
#################################################
##################################################

###################################################
#  STDERR & STDOUT 
###################################################

open (STDOUT,">makeLOG.txt");
open (STDERR,"&STDOUT");

###################################################
#  grab os and version level to for notification / report
###################################################
#save until we have implented bRmakeAIX.pl
# AIX uname -sv required  :  someone had to be different

##if ( $OScheck=qx| uname| =~ m/AIX/ ) {
#   print "\nIF : AIX  :  $OScheck\n";
#   $OSlevel=qx| uname -sv |;
#   chomp ($OSlevel);
#   $build="";
#   chomp ($build);
#   print "\nIF : AIX  :  $OSlevel\n";
#}
#else {

$OSlevel=qx| uname -rs |;
chomp ($OSlevel);

#   print "\nELSE : $OSlevel\n";
#   $build="sol.bldcmd";
#   chomp ($build);
#}

###################################################
#  due to telnet call issue (dies before end of make  :  no clue)
###################################################

print "\n\nSTART bRmake.pl  :  $OSlevel\n\n";

###################################################
#  Call sol.bldcmd  :  from checked out tag work area 
###################################################

chdir "../../unix_v1.2/ftdsrc";

system qq(pwd);

system qq(sol.bldcmd 210 tdmf);

print "\n\nbRmake : $OSlevel  :  $build  :  rc = $?\n\n";

print "\n\nEND bRmake.pl  :  $OSlevel \n\n";

close (STDERR);
close (STDOUT);

