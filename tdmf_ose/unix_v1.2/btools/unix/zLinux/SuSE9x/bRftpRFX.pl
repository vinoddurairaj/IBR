#!/usr/bin/perl -I../../../pms -I../../..
###################################
#################################
#
#  ./bRftpRFX.pl
#
#       ftp deliverables to San Jose server
#
#       http://dmsbuilds.sanjose.ibm.com/builds/Replicator/UNIX/$RFXver/$Bbnum/zLinux
#
#################################
###################################

#######################################
#####################################
# vars  /  pm's  /  use
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

open (STDOUT,">ftprfx.txt");
open (STDERR,">&STDOUT");

#############################################
###########################################
#  exec statements
###########################################
#############################################

print "\n\nSTART  :  bRftpRFX.pl\n\n";

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

chdir "/work/builds/$Bbnum/SuSE9x"   || die "chdir death /work/builds/$Bbnum/SuSE9x  :  $!";

system qq(pwd);

#######################################
#####################################
# ftp  :  required deliverables
#######################################
#######################################

$ftp=Net::FTP->new("$dmsbuilds", Debug => 1) or die "can't connect : $@\n";
$ftp->login("bmachine","moose2")        or die "could not login  : $@\n";

$ftp->cwd("builds/Replicator/UNIX");
$ftp->mkdir("$RFXver");
$ftp->cwd("$RFXver");
$ftp->mkdir("$Bbnum");
$ftp->cwd("$Bbnum");

$ftp->mkdir("zLinux");
$ftp->mkdir("zLinux/SuSE9x");
$ftp->mkdir("zLinux/SuSE9x/logs");
$ftp->cwd("zLinux/SuSE9x/logs");

$ftp->ascii;
$ftp->put("cprpm.txt");
$ftp->ascii;
$ftp->put("isoZ.txt");
$ftp->ascii;
$ftp->put("tarcvf.txt");
$ftp->cwd("..");
$ftp->binary;
$ftp->put("$RFXver.$Bbnum.zlinuxSuSE9x.tar");

$ftp->quit;

#######################################
#######################################

print "\n\nEND  :  bRftpRFX\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);

