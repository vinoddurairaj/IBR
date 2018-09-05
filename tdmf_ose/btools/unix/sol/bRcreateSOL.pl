#!/usr/local/bin/perl
###############################################
#############################################
#
#  perl -w ./bRcreateSOL.pl
#
#	control script for solaris builds
#
#############################################
#############################################

#############################################
# get global vars, cvs commit occured to role $buildnumber on 2000 machine
#	hence, $buildnumber should represent appropriate number
#############################################

require "bRglobalvars.pm";
$Bbnum="$Bbnum";
$pkgtrans="pkgtrans -s . ../tdmfsol.pkg all";

# no STDERR || STDOUT capture  :  use telnet to capture

print "\n\nSTART bRcreateSOL.pl\n\n"; 

#############################################
# get to root of Replication project directory
############################################

system qq(pwd);

#############################################
# call the scripts required to build unix agent
#############################################

chdir "../";

system qq(pwd);

system qq(nohup perl bRmake.pl);

print "\n\nbRmake.pl rc = $?\n\n";

chdir "../../unix_v1.2/ftdsrc/pkg.install";

system qq(pwd);

system qq($pkgtrans);

print "\n\nEND bRcreateSOL.pl\n\n"; 

#############################################

close (STDERR);
close (STDOUT);

