#!/usr/bin/perl -I../pms
#######################################
#####################################
#
#  perl -w ./bRftpQA.pl
#
#   product images to dnas200 
#
#####################################
#######################################
 
#######################################
#####################################
# vars  /  pms  /  use
#####################################
#######################################

eval 'require bRglobalvars';

our $Bbnum;
our $RFXver;

use Net::FTP;

#######################################
#####################################
# STDOUT  /  STDERR
#####################################
#######################################

open (STDOUT, ">../logs/ftpQA.txt");
open (STDERR, ">\&STDOUT");

#######################################
#####################################
# exec statements
#####################################
#######################################

print "\n\nSTART  :  bRftpQA.pl\n\n";

#######################################
#####################################
#  get to build built tree
#####################################
#######################################

chdir "../../builds/$Bbnum"  ||  die "chdir death  :  $!";

system qq(pwd);

#######################################
#####################################
# ftp
#
#   $RFXver.$Bbnum.iso
#####################################
#######################################

$ftp=Net::FTP->new("dnas200")	or die "can't connect : $@\n";
$ftp->login("ftpuser","softek")	or die "could not login  : $@\n";
#$ftp->binary;
#$ftp->put("$RFXver.$Bbnum.polybin.iso");
$ftp->cwd("buildxfer");
$ftp->binary;
$ftp->put("$RFXver.$Bbnum.gold.iso");
$ftp->quit;

#######################################
#######################################

print "\n\nEND  :  bRftpQA.pl\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);

