#!/usr/bin/perl -I../../../pms -I../../..
#############################################
###########################################
#
#  perl -c bRcpRPM.pl
#
#    local cp RFX / TUIP *.rpm to /work/builds/B##### 
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

our $PRODtype;
our $PRODtypetuip;

our $RFXbldPath="../../../../builds/$Bbnum";
our $TUIPbldPath="../../../../builds/tuip/$Bbnum";
our $tdmfSrcPath="tdmf_ose/unix_v1.2/ftdsrc";

our  $stripbuildnum="$buildnumber";
     $stripbuildnum=~s/^0//;

our $bminus=$stripbuildnum -1;
    $bminus=~s/^/0/;
our $Bminus="B$bminus" ;

our $BLDCMDR="sudo ./bldcmd brand=$PRODtype  build=$stripbuildnum fix=$PATCHver";
our $BLDCMDT="sudo ./bldcmd brand=$PRODtypetuip build=$stripbuildnum fix=$PATCHver";

#############################################
###########################################
#  STDOUT  /  STDOUT
###########################################
#############################################

open (STDOUT,">cprpm.txt");
open (STDERR,">&STDOUT");

#############################################
###########################################
#  exec statements
###########################################
#############################################

print "\n\nSTART  :  bRcpRPM.pl\n\n";

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

print "\nPRODtype        =   $PRODtype\n";
print "\nPRODtypetuip    =   $PRODtypetuip\n";

print "\nRFXbldPath      =   $RFXbldPath\n";
print "\nTUIPbldPath     =   $TUIPbldPath\n";
print "\ntdmfSrcPath     =   $tdmfSrcPath\n";

print "\nRFX BLDCMD      =   $BLDCMDR\n";
print "\nTUIP BLDCMD     =   $BLDCMDT\n";

print "\nstripbuildnumber  =  $stripbuildnum\n";
print "\nBminus            =  $Bminus\n\n\n";

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

print "\nRFX zLinux cp RPM  :  $Bbnum\n";

mkdir "/work/builds/";
mkdir "/work/builds/$Bbnum";
mkdir "/work/builds/$Bbnum/RH4x";
if (! -d "/work/builds/$Bbnum/RH4x") {
   print "\n/work/builds/$Bbnum/RH4x does not exist\n";
   print "\nexiting cp RPM routine\n";
   exit();
}

#############################################
###########################################
#  RFX build 
###########################################
#############################################

chdir "$RFXbldPath" || die "chdir $RFXbldPath death  :  $!";
system qq(pwd);
print "\n\n";

system qq(cp $tdmfSrcPath/pkg.install/Rep*.rpm /work/builds/$Bbnum/RH4x);

#############################################
###########################################
#  TUIP build 
###########################################
#############################################

print "\nTUIP zLinux cp RPM  :  $Bbnum\n";

chdir "$CWD";
system qq(pwd);
print "\n\n";

chdir "$TUIPbldPath";
system qq(pwd);
print "\n\n";

system qq(cp $tdmfSrcPath/pkg.install/TDMF*.rpm /work/builds/$Bbnum/RH4x);

chdir "$CWD";
system qq(pwd);
print "\n\n";

#############################################
#############################################

print "\n\nEND  :  bRcpRPM.pl\n\n";

#############################################
#############################################

close (STDERR);
close (STDOUT);

