#!/usr/bin/perl -I../../../pms -I../../..
###################################
#################################
#
#  ./bRtarCVF.pl
#
#    tar cvf $RFXver.$Bbnum.zlinuxSuSE9x.tar 
#    tar cvf $TUIPver.$Bbnum.zlinuxSuSE9x.tar 
#
#################################
###################################

#######################################
#####################################
# vars  /  pm's  /  use
#######################################
#######################################

eval 'require bRglobalvars';

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

open (STDOUT,">tarcvf.txt");
open (STDERR,">&STDOUT");

#############################################
###########################################
#  exec statements
###########################################
#############################################

print "\n\nSTART  :  bRtarCVF.pl\n\n";

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

chdir "/work/builds/$Bbnum/SuSE9x"   || die "chdir death /work/builds/$Bbnum/SuSE9x  :  $!";

system qq(pwd);

$RFXname=`ls Rep*.rpm`;
$TUIPname=`ls TDM*.rpm`;

chomp($RFXname);
chomp($TUIPname);

print "\n$RFXname   =  $RFXname\n";
print "\n$TUIPname  =  $TUIPname\n";

system qq(tar cvf $RFXver.$Bbnum.zlinuxSuSE9x.tar  $RFXname);
system qq(tar cvf $TUIPver.$Bbnum.zlinuxSuSE9x.tar $TUIPname);

#######################################
#######################################

print "\n\nEND  :  bRtarCVF\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);

