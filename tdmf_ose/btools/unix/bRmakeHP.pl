#!/bin/perl
##################################################
################################################
#
#  ./bRmakeHP.pl  
#
#	run development supplied  :  hp1100.bldcmd  (which runs make)
#
#################################################
##################################################

###################################################
#  STDERR & STDOUT 
###################################################

open (STDOUT,">makeHP.txt");
open (STDERR,">&STDOUT");


$OSlevel=qx| uname -rs |;
chomp ($OSlevel);

print "\n\nSTART bRmake.pl  :  $OSlevel\n\n";

###################################################
#  Call hp1100.bldcmd  :  from checked out tag work area 
###################################################

chdir "../../unix_v1.2/ftdsrc";

system qq(pwd);

system qq(hp1100.bldcmd 210 tdmf);

print "\n\nbRmake : $OSlevel  :  hp1100.bldcmd  :  rc = $?\n\n";

print "\n\nEND bRmake.pl  :  $OSlevel \n\n";

close (STDERR);
close (STDOUT);

