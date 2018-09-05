#!/usr/local/bin/perl -w
##############################
############################
#
#  perl -w bRMainUnix.pl
#
#	run telnets to build tdmf_open  :
#	aix solaris hp linux
#
############################
##############################

##############################
############################
# vars
############################
##############################

require "bRglobalvars.pm";

##############################
############################
#
# STDERR / STDOUT to UNIXLOG.txt
#
############################
##############################

open (STDOUT, ">UNIXLOG.txt");
open (STDERR, ">>&STDOUT");

#run telnet's 

print "\n\nSTART : bRMainUnix.pl\n\n";

chdir "unix\\sol";

system qq($cd);

system qq(perl -w -I..\\.. bRtelnetSOL7.pl);
print "\n\nbRtelnetSOL7.pl rc = $?\n\n";

system qq(perl -w -I..\\.. bRtelnetSOL6.pl);
print "\n\nbRtelnetSOL6.pl rc = $?\n\n";

system qq(perl -w -I..\\.. bRtelnetSOL8.pl);
print "\n\nbRtelnetSOL8.pl rc = $?\n\n";

system qq(perl -w -I..\\.. bRtelnetSOL9.pl);
print "\n\nbRtelnetSOL9.pl rc = $?\n\n";

chdir "..\\hp";

system qq($cd);

system qq(perl -w -I..\\.. bRtelnetHP11.pl);
print "\n\nbRtelnetHP11.pl rc = $?\n\n";

system qq(perl -w -I..\\.. bRtelnetHP11i.pl);
print "\n\nbRtelnetHP11i.pl rc = $?\n\n";

#system qq(perl -w -I.. bRtelnetAIX.pl);
#print "\n\nbRtelnetAIX.pl rc = $?\n\n";

#system qq(perl -w -I.. bRtelnetHP.pl);
#print "\n\nbRMtelnetHP.pl rc = $?\n\n";

#system qq(perl -w -I.. bRtelnetLNX.pl);
#print "\n\nbRtelnetLNX.pl rc = $?\n\n";

print "\n\nEND :  bRMainUnix.pl\n\n";

close (STDERR);
close (STDOUT);
