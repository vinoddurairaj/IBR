#!/usr/bin/perl
######################################################
####################################################
#  perl -w ./bRrunMainCron26.pl
#
#    cron is not working via /home0/bmachine disk / file system
#    work around  :  call this script in cron
#    which will cd to RFXTUIP26x build dir
#    and exec bRMain.pl per nightly standard
#
####################################################
######################################################

#####################################################
###################################################
#  add this file to ~/dev/branch/RFXTUIP26cron
#
#    initiate via cron entry  :
#    10 22 * * cd dev/branch/RFXTUIP26cron ; /usr/bin/perl bRrunMaincron26.pl
###################################################
#####################################################

open (STDOUT,">runMainCron.txt");
open (STDERR,">>&STDOUT");

chdir "/home0/bmachine/dev/branch/RFXTUIP26/tdmf_ose/unix_v1.2/btools";

system qq(pwd);
system qq(pwd);
print "\n\n";
system qq( ls -la bRMain.pl);
unlink "rdiff.txt";
print "\n\n";
system qq(perl bRMain.pl);
#system qq(perl bRrdiff.pl);  # test
system qq( ls -la rdiff.txt);
print "\n\n";
system qq(date);

print "\n\nEND\n";


