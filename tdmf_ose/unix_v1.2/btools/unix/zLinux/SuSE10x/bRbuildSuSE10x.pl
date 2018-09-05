#!/usr/bin/perl -I../../../pms -I../../..
#############################################
###########################################
#
#  perl -c bRbuildSuSE10x.pl
#
#    build SuSE10x zLinux env via Raleigh
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
    #$bminus=~s/^/0/;
our $Bminus="B$bminus" ;

our $BLDCMDR="sudo ./bldcmd brand=$PRODtype  build=$stripbuildnum fix=$PATCHver";
our $BLDCMDT="sudo ./bldcmd brand=$PRODtypetuip build=$stripbuildnum fix=$PATCHver";

#############################################
###########################################
#  STDOUT  /  STDOUT
###########################################
#############################################

#open (STDOUT,">buildSuSE10x.txt");
#open (STDERR,">&STDOUT");

#############################################
###########################################
#  exec statements
###########################################
#############################################

print "\n\nSTART  :  bRbuildSuSE10x.pl\n\n";

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
#  RFX build 
###########################################
#############################################

print "\nRFX zLinux build  :  $Bbnum\n";

chdir "../..";
system qq(pwd);
print "\n\n";
system qq(perl bRgetcodezLinux.pl);
#system qq(ls -l bRgetcodezLinux.pl);

chdir "$CWD" || die "what happen  :  $CWD  :  $!";
system qq(pwd);
print "\n\n";

chdir "$RFXbldPath/$tdmfSrcPath" || die "now what  :  $!";
system qq(pwd);
print "\n\n";

system qq(ls -la bldcmd);
print "\n\nexec  :   $BLDCMDR\n\n"; 
system qq($BLDCMDR);

#############################################
###########################################
#  TUIP build 
###########################################
#############################################

print "\nTUIP zLinux build  :  $Bbnum\n";

chdir "$CWD";
system qq(pwd);
print "\n\n";

chdir "../..";
system qq(pwd);
print "\n\n";
system qq(perl bIgetcodezLinux.pl);

chdir "$CWD";
system qq(pwd);
print "\n\n";

chdir "$TUIPbldPath/$tdmfSrcPath";
system qq(pwd);
print "\n\n";

system qq(ls -la bldcmd);
print "\n\nexec  :   $BLDCMDT\n\n"; 
system qq($BLDCMDT);

#############################################
###########################################
#  RFX remove previous build 
###########################################
#############################################

chdir "$CWD";
system qq(pwd);
print "\n\n";

chdir "$RFXbldPath";
chdir "..";
system qq(pwd);
print "\nRFX rm -rf $Bminus\n";

print "\n\n";
system qq(sudo rm -rf $Bminus);
system qq(ls -la);

#############################################
###########################################
#  TUIP remove previous build 
###########################################
#############################################

chdir "$CWD";
system qq(pwd);
print "\n\n";

chdir "$TUIPbldPath";
chdir "..";
system qq(pwd);
print "\nTUIP rm -rf $Bminus\n";

print "\n\n";
system qq(sudo rm -rf $Bminus);
system qq(ls -la);

#############################################
#############################################

print "\n\nEND  :  bRbuildSuSE10x.pl\n\n";

#############################################
#############################################

close (STDERR);
close (STDOUT);

