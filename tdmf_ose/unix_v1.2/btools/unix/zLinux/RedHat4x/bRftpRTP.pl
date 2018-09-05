#!/usr/bin/perl -I../../../pms -I../../..
###################################
#################################
#
#  ./bRftpRTP.pl
#
#       ftp deliverables dmsbuilds to local 
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

our $RFXbldPath="../../../../builds/$Bbnum/Linux/RedHat/4x/s390x";
our $TUIPbldPath="../../../../builds/tuip/$Bbnum/Linux/RedHat/4x/s390x";
our $tdmfSrcPath="tdmf_ose/unix_v1.2/ftdsrc";

our  $stripbuildnum="$buildnumber";
     $stripbuildnum=~s/^0//;

our $bminus=$stripbuildnum -1;
    $bminus=~s/^/0/;
our $Bminus="B$bminus" ;

#############################################
###########################################
#  STDOUT  /  STDOUT
###########################################
#############################################

if ( -e "ftprtp.txt") {unlink "ftprtp.txt";}
open (STDOUT,">ftprtp.txt");
open (STDERR,">&STDOUT");

#############################################
###########################################
#  exec statements
###########################################
#############################################

print "\n\nSTART  :  bRftpRTP.pl\n\n";

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

chdir "$RFXbldPath"   || die "chdir death $RFXbldPath   :  $!";

system qq(pwd);

#######################################
#####################################
# ftp  :  RFX
#######################################
#######################################

$ftp=Net::FTP->new("$dmsbuilds", Debug => 1) or die "can't connect : $@\n";
$ftp->login("bmachine","moose2")        or die "could not login  : $@\n";

$ftp->cwd("builds/Replicator/UNIX/$RFXver/$Bbnum/zLinux/RedHat4x");
$ftp->binary;
$ftp->get("$RFXver.$Bbnum.zlinuxRH4x.tar");

$ftp->quit;

#######################################
#####################################
# tar xvf $RFXver.$Bbnum.zlinuxRH4x.tar && unlink
#####################################
#######################################
print "\n\n";

system qq(tar xvf $RFXver.$Bbnum.zlinuxRH4x.tar);

unlink "$RFXver.$Bbnum.zlinuxRH4x.tar";

print "\n\n";

system qq(pwd);

system qq(ls -la);
print "\n\n";

#######################################
#####################################
#  cd $CWD
#####################################
#######################################

chdir "$CWD"  || die "chdir death $CWD  :  $!\n";
system qq(pwd);
print "\n\n";

#######################################
#####################################
# ftp  :  TUIP 
#######################################
#######################################

chdir "$TUIPbldPath"   || die "chdir death $TUIPbldPath   :  $!";

$ftp=Net::FTP->new("$dmsbuilds", Debug => 1) or die "can't connect : $@\n";
$ftp->login("bmachine","moose2")        or die "could not login  : $@\n";

$ftp->cwd("builds/TDMFUNIX\(IP\)/$TUIPver/$Bbnum/zLinux/RedHat4x");
$ftp->binary;
$ftp->get("$TUIPver.$Bbnum.zlinuxRH4x.tar");

$ftp->quit;

#######################################
#####################################
# tar xvf $TUIPver.$Bbnum.zlinuxRH4x.tar && unlink
#####################################
#######################################

print "\n\n";

system qq(tar xvf $TUIPver.$Bbnum.zlinuxRH4x.tar);

unlink "$TUIPver.$Bbnum.zlinuxRH4x.tar";

print "\n\n";

system qq(pwd);

system qq(ls -la);
print "\n\n";


#######################################
#######################################

print "\n\nEND  :  bRftpRFX\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);

