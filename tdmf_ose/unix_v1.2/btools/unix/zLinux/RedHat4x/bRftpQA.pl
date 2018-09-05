#!/usr/bin/perl -I../../../pms -I../../..
###################################
#################################
#
#  ./bRftpQA.pl
#
#    ftp deliverables to Ireland QA server
#
#    9.29.90.85  :  dnas200
#
#    use 9.29 due to IBM side of network    
#
#################################
###################################

#######################################
#####################################
# vars  /  pm's  /  use
#######################################
#######################################

eval 'require bRglobalvars';

use Net::FTP;

our $COMver;
our $RFXver;
our $TUIPver;
our $Bbnum;
our $buildnumber;

#############################################
###########################################
#  STDOUT  /  STDOUT
###########################################
#############################################

open (STDOUT,">ftpqa.txt");
open (STDERR,">&STDOUT");

#############################################
###########################################
#  exec statements
###########################################
#############################################

print "\n\nSTART  :  bRftpQA.pl\n\n";

#############################################
###########################################
#  log values 
###########################################
#############################################

print "\n\nCOMver          =   $COMver\n";
print "\nRFXver          =   $RFXver\n";
print "\nTUIPver         =   $TUIPver\n";
print "\nBbnum           =   $Bbnum\n";
print "\nbuildnumber     =   $buildnumber\n\n";

system qq(uname -a);
print "\n\n";
system qq(date);
print "\n\n";

#############################################
###########################################
#  establish PWD 
###########################################
#############################################

$CWD=`pwd`;
print "\nCWD          =   $CWD\n\n";
chomp($CWD);

chdir "/work/builds/$Bbnum"   || die "chdir death /work/builds/$Bbnum  :  $!";

system qq(pwd);

#######################################
#####################################
# ftp  :  required deliverables
#######################################
#######################################

$ftp=Net::FTP->new("9.29.90.85", Debug => 1) or die "can't connect : $@\n";
$ftp->login("ftpuser","softek")        or die "could not login  : $@\n";

$ftp->binary;
$ftp->put("$RFXver.$Bbnum.zLinuxRH4x.iso");
$ftp->binary;
$ftp->put("$TUIPver.$Bbnum.zLinuxRH4x.iso");
#$ftp->binary;
#$ftp->put("RedHat5x/$RFXver.$Bbnum.zLinuxRH5x.iso");
#$ftp->binary;
#$ftp->put("RedHat5x/$TUIPver.$Bbnum.zLinuxRH5x.iso");
#$ftp->binary;
#$ftp->put("SLES9x/$RFXver.$Bbnum.zLinuxSLES9x.iso");
#$ftp->binary;
#$ftp->put("SLES9x/$TUIPver.$Bbnum.zLinuxSLES9x.iso");
#$ftp->binary;
#$ftp->put("SLES10x/$RFXver.$Bbnum.zLinuxSLES10x.iso");
#$ftp->binary;
#$ftp->put("SLES10x/$TUIPver.$Bbnum.zLinuxSLES10x.iso");

$ftp->quit;

#######################################
#######################################

print "\n\nEND  :  bRftpQA\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);

