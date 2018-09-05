#!/usr/bin/perl -I../../../pms -I../../..
###################################
#################################
#
#  ./bRftpTUIP.pl
#
#       ftp deliverables to San Jose server
#
#       http://dmsbuilds.sanjose.ibm.com/builds/TDMFUNIX(IP)/$TUIPver/$Bbnum/zLinux
#
#################################
###################################

#######################################
#####################################
# vars  /  pms  /  use
#######################################
#######################################

eval 'require bRglobalvars';
eval 'require bRbuildmachinelist';

use Net::FTP;

our $COMver;
our $RFXver;
our $TUIPver;
our $Bbnum;
our $buildnumber;

our $dmsbuilds;

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

open (STDOUT,">ftptuip.txt");
open (STDERR,">&STDOUT");

#############################################
###########################################
#  exec statements
###########################################
#############################################

print "\n\nSTART  :  bRftpTUIP.pl\n\n";

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
print "\ndmsbuilds       =   $dmsbuilds\n";

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

chdir "/work/builds/$Bbnum/RH5x"   || die "chdir death /work/builds/$Bbnum/RH5x  :  $!";

system qq(pwd);

#######################################
#####################################
# ftp  :  required deliverables
#######################################
#######################################

$ftp=Net::FTP->new("$dmsbuilds", Debug => 1) or die "can't connect : $@\n";
$ftp->login("bmachine","moose2")        or die "could not login  : $@\n";

$ftp->cwd("builds/TDMFUNIX(IP)");
$ftp->mkdir("$TUIPver");
$ftp->cwd("$TUIPver");
$ftp->mkdir("$Bbnum");
$ftp->cwd("$Bbnum");

$ftp->mkdir("zLinux");
$ftp->mkdir("zLinux/RedHat5x");
$ftp->mkdir("zLinux/RedHat5x/logs");
$ftp->cwd("zLinux/RedHat5x/logs");

$ftp->ascii;
$ftp->put("cprpm.txt");
$ftp->ascii;
$ftp->put("isoZ.txt");
$ftp->ascii;
$ftp->put("tarcvf.txt");
$ftp->cwd("..");
$ftp->binary;
$ftp->put("$TUIPver.$Bbnum.zlinuxRH5x.tar");

$ftp->quit;

#######################################
#######################################

print "\n\nEND  :  bRftpTUIP\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);

