#!/usr/local/bin/perl
###############################################
#############################################
#
#  perl -w ./bRcreateHP.pl
#
#	control script for hp builds
#
#############################################
#############################################

#############################################
# get global vars, cvs commit occured to role $buildnumber on 2000 machine
#	hence, $buildnumber should represent appropriate number
#############################################

require "bRglobalvars.pm";
$Bbnum="$Bbnum";

# no STDERR || STDOUT capture  :  use telnet to capture

print "\n\nSTART bRcreateHP.pl\n\n"; 

#############################################
# get to root of Replication project directory
############################################

system qq(pwd);

#############################################
# call the scripts required to build unix agent
#############################################

chdir "../";

system qq(pwd);

system qq(nohup perl bRmakeHP.pl);

print "\n\nbRmakeHP.pl rc = $?\n\n";

system qq(pwd);

print "\n\nEND bRcreateHP.pl\n\n"; 

#############################################

close (STDERR);
close (STDOUT);

