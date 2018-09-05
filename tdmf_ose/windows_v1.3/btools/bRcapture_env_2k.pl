#!/usr/local/bin/perl -w
##################################
################################
#
# perl -w bRcapture_env_2k.pl
#
#   capture %ENV vars from bmachine
#	commit file "2K_env.txt" via bSMcvscommit.pl
#
################################
##################################

open (FH0, ">2K_env.txt");

while (($key, $value) = each %ENV) {
   #print "$key=$value\n";
   print FH0 "$key=$value\n";
}

close (FH0);
