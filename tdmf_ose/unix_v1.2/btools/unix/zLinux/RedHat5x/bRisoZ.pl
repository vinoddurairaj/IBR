#!/usr/local/bin/perl -I../../../pms -I../../..
######################################################
####################################################
#
#  perl -w bSMisoZ.pl
#
#    create ISO image
#
####################################################
######################################################

######################################################
####################################################
#  vars  /  pms  /  use
####################################################
######################################################

eval 'require bRglobalvars';

our $RFXver;
our $TUIPver;
our $PATCHver;
our $Bbnum;

our $CDLABELR="$RFXver-$PATCHver-$Bbnum";
our $CDLABELT="$TUIPver-$PATCHver-$Bbnum";
our $UID="-uid 0 -gid 0 -file-mode 555";
our $op0="-f -hide-rr-moved -J -R -hfs -max-iso9660-filename";
our $opR="-L -o  $RFXver.$Bbnum.zLinuxRH5x.iso -U -v -V $CDLABELR $UID imageR";
our $opT="-L -o $TUIPver.$Bbnum.zLinuxRH5x.iso -U -v -V $CDLABELT $UID imageT";
our $mkiR="mkisofs $op0 $opR";
our $mkiT="mkisofs $op0 $opT";

######################################################
####################################################
#  STDERR  /  STDOUT
####################################################
######################################################

open (STDOUT, ">isoZ.txt");
open (STDERR, ">&STDOUT");

######################################################
####################################################
#  exec statements 
####################################################
######################################################

print "\n\nSTART  :  bRisoZ.pl\n\n";

######################################################
####################################################
#  log values 
####################################################
######################################################

print "\n\nRFXver             =   $RFXver\n";
print "\nTUIPver            =   $TUIPver\n";
print "\nBbnum              =   $Bbnum\n";

print "\nCDLABELR           =   $CDLABELR\n";
print "\nCDLABELT           =   $CDLABELT\n";
print "\nUID                =   $UID\n";
print "\nop0                =   $op0\n";
print "\nopR                =   $opR\n";
print "\nopT                =   $opT\n";
print "\nmkiR               =   $mkiR\n";
print "\nmkiT               =   $mkiT\n";


chdir "/work/builds/$Bbnum/RH5x"  || die "chdir death /work/builds/$Bbnum/RH5x  :  $!";

system qq(pwd);
print "\n\n";
system qq(ls -l);

print "\n\n";

######################################################
####################################################
#  RFX iso procedure 
####################################################
######################################################

mkdir "imageR";

system qq(cp Rep*.rpm imageR);
system qq(ls -la imageR);

system qq($mkiR);

print "\n\nRFX mkisofs  :  rc = $?\n";

######################################################
####################################################
#  TUIP iso procedure 
####################################################
######################################################

mkdir "imageT";

system qq(cp TDM*.rpm imageT);

system qq($mkiT);

print "\n\nTUIP mkisofs  :  rc = $?\n";

##########################################################
##########################################################

print "\n\nEND : bRisoZ.pl\n\n";

##########################################################
##########################################################

close (STDERR);
close (STDOUT);

