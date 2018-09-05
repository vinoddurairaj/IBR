#!/usr/bin/perl -I../../../pms -I../../..
#############################################
###########################################
#
#  perl -c bRcpLOGSpl
#
#    local cp build LOGS to /work/builds/B#####/SuSE10x 
#
#    until we get network issues resolved
# 
###########################################
#############################################

#############################################
###########################################
#  vars  /  pms  /  use
###########################################
#############################################

eval 'require bRglobalvars';

our $COMver;
our $RFXver;
our $TUIPver;
our $Bbnum;
our $buildnumber;
our $branchdir;

#############################################
###########################################
#  STDOUT  /  STDOUT
###########################################
#############################################

open (STDOUT,">cplogs.txt");
open (STDERR,">&STDOUT");

#############################################
###########################################
#  exec statements
###########################################
#############################################

print "\n\nSTART  :  bRcpLOGS.pl\n\n";

#############################################
###########################################
#  log values 
###########################################
#############################################

print "\n\nCOMver          =   $COMver\n";
print "\nRFXver          =   $RFXver\n";
print "\nTUIPver         =   $TUIPver\n";
print "\nBbnum           =   $Bbnum\n";
print "\nbuildnumber     =   $buildnumber\n";
print "\nbranchdir       =   $branchdir\n\n";
print "\nRH5xpath        =   $RH5xpath\n\n";
print "\nSLES9xpath      =   $SLES9xpath\n\n";
print "\nSLES10xpath     =   $SLES10xpath\n\n";

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

#############################################
###########################################
#  setup / cp log 
###########################################
#############################################

print "\n$COMver zLinux cp LOGS  :  $Bbnum\n";

mkdir "/work/builds/";
mkdir "/work/builds/$Bbnum";
mkdir "/work/builds/$Bbnum/SuSE10x";
if (! -d "/work/builds/$Bbnum/SuSE10x") {
   print "\n/work/builds/$Bbnum/SuSE10x does not exist\n";
   print "\nexiting cp RPM routine\n";
   exit();
}

system qq(cp cprpm.txt /work/builds/$Bbnum/SuSE10x);
system qq(cp tarcvf.txt /work/builds/$Bbnum/SuSE10x);
system qq(cp isoZ.txt /work/builds/$Bbnum/SuSE10x);

print "\n\n";

#############################################
#############################################

print "\n\nEND  :  bRcpLOGS.pl\n\n";

#############################################
#############################################

close (STDERR);
close (STDOUT);

