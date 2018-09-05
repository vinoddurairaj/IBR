#!/usr/local/bin/perl -w
##############################
############################
#
#  perl -w bRcreateISORM.pl
#
#	create ISO image of final built product
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
# STDERR / STDOUT to isoLOG.txt
#
############################
##############################

open (STDOUT, ">createisoLOG.txt");
open (STDERR, ">>&STDOUT");

##############################
############################
# call them scripts
############################
##############################

print "\n\nSTART : bRcreateISO.pl\n\n";

chdir "iso";

system qq($cd);

system qq(perl -I.. bRftpALL.pl);

print "\n\nbRftpALL.pl  :  rc = $?\n\n";

system qq(perl -I.. bRtelnetISO.pl);

print "\n\nbRtelnetISO.pl  :  rc = $?\n\n";

system qq(perl -I.. bRtelnetISO_OEM.pl);

print "\n\nbRtelnetISO.pl_OEM  :  rc = $?\n\n";

system qq(perl -I.. bRftpISO.pl);

print "\n\nbRftpISO.pl  :  rc = $?\n\n";

system qq(perl -I.. bRtelnetRM.pl);

print "\n\nbRtelnetRM.pl  :  rc = $?\n\n";

##############################
############################
# dump above logs to this script log for www site
############################
##############################

open (FH0, "isoLOG.txt");
@logs=<FH0>;
close (FH0);

print "\n\nISO LOGS from ISO image creation process  :  \n\nBELOW\n\n@logs\n\n";

system qq($cd);

print "\n\nEND : bRcreateISO.pl\n\n";
