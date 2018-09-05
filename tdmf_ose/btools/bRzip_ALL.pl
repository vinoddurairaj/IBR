#!/usr/local/bin/perl -w
#######################################
#####################################
#
#  perl -w ./bRzip_ALL.pl
#
#	zip delivers from product image dir structure  : 
#	 
#	..\\builds\\$Bbnum  
#
#	creates zip archive  :  $RFUver.$Bbnum.ALL.zip	
#
#####################################
#######################################
 
#######################################
#####################################
# get global vars 
#####################################
#######################################

require "bRglobalvars.pm";
$Bbnum="$Bbnum";
$RFUver="$RFUver";

#######################################
#####################################
# capture stderr stdout
#####################################
#######################################

open (STDOUT, ">zipALL.txt");
open (STDERR, ">&STDOUT");

print "\n\nSTART : \t bRzip_ALL.pl\n\n";

#######################################
#####################################
#   pkzip25 -add -dirs *
#####################################
#######################################

chdir "..\\builds\\$Bbnum";  

system qq($cd);
system qq($cd);

system qq(pkzip25 -add -directories ..\\$RFUver.$Bbnum.ALL.zip *);

print "\n\nEND bRzip_ALL.pl\n\n";

close (STDERR);
close (STDOUT);
