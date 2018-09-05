#!/usr/bin/perl -I../../../pms -I../../..
#############################################
###########################################
#
#  perl -c bRthreadFTP.pl
#
#    ftp RFX / TUIP *.rpm to Sunnyvale:/builds/PRODUCT/B##### 
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

use threads;

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

open (STDOUT,">threadftp.txt");
open (STDERR,">&STDOUT");

#############################################
###########################################
#  exec statements
###########################################
#############################################

print "\n\nSTART  :  bRthreadFTP.pl\n\n";

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
#  thread && controls 
###########################################
#############################################

print "\nexec scripts via threads  :  no wait  :  $Bbnum\n";

$i="0";
$j="0";
$k="0";
$l="0";

my $threadSJ = async { while  ("$i" < 1) {
        print "\ni = $i\n";
        #called via Main
        #system qq( perl bRftpRFX.pl );
        $i++;
        print "\ni = $i\n";
        } };

my $threadST = async { while  ("$l" < 1) {
        print "\nl = $l\n";
        #called via Main
        #system qq( perl bRftpTUIP.pl );
        $l++;
        print "\nl = $l\n";
        } };

my $threadQX = async { while  ("$k" < 1) {
        print "\nk = $k\n";
        #sleep 370;
        system qq( perl bRftpQA.pl );
        $k++;
        print "\nk = $k\n";
        } };

my $threadTHROW = async { while ( "$j" < 1 ) {
        print "\nj = $j\n";
        $j++;
        print "\nj = $j\n";
        print "\nTHROW away\n";
        } };

#############################################
###########################################
#  thread#-> detach();
#       will not join or wait  ==>> detach scrip
###########################################
#############################################

$threadSJ->detach();
$threadST->detach();
$threadQX->detach();
$threadTHROW->join();

#############################################
#############################################

print "\n\nEND  :  bRthreadFTP.pl\n\n";

#############################################
#############################################

close (STDERR);
close (STDOUT);

