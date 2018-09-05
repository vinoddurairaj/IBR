#!/usr/bin/perl -I../pms
#######################################
#####################################
#
#  perl -w bRcreateISO.pl
#
# 	create ISO images
#
#####################################
#######################################

#######################################
#####################################
#  vars  /  pms  /  use 
#####################################
#######################################

eval 'require bRglobalvars';
eval 'require bRtreepath';
eval 'require bPWD';

our $RFXver;
our $TUIPver;
our $COMver;
our $Bbnum;

our $CWD;
our $RFXbuildtree;
our $TUIPbuildtree;

our $valueIn;

our $CDLABELR="$RFXver-$PATCHver-$Bbnum";
our $CDLABELT="$TUIPver-$PATCHver-$Bbnum";
our $UID="-uid 0 -gid 0 -file-mode 555";
our $op0="-f -hide-rr-moved -J -R -hfs -max-iso9660-filename";
our $op1R="-L -o $RFXver.$Bbnum.gold.iso  -U -v -V $CDLABELR $UID image";
our $op1T="-L -o $TUIPver.$Bbnum.gold.iso -U -v -V $CDLABELT $UID image";
our $mkiR="mkisofs $op0 $op1R";
our $mkiT="mkisofs $op0 $op1T";

#######################################
#####################################
#  STDOUT  /  STDERR 
#####################################
#######################################

open (STDOUT, ">../logs/createiso.txt");
open (STDERR, ">\&STDOUT");

#######################################
#####################################
#  exec statements 
#####################################
#######################################

print "\n\nSTART : bRcreateISO.pl\n\n";

#######################################
#####################################
#  logs values 
#####################################
#######################################

print "\nCOMver         =  $COMver\n";
print "\nBbnum          =  $Bbnum\n";
print "\nRFXbuildtree   =  $RFXbuildtree\n";
print "\nTUIPbuildtree  =  $TUIPbuildtree\n";
print "\nRFXver         =  $RFXver\n";
print "\nTUIPver        =  $TUIPver\n\n";

#######################################
#####################################
#  sub establishPWD to enable var $CWD with pwd
#####################################
#######################################

establishPWD();

print "\nsub establishPWD returned  :  $CWD\n\n";

#######################################
#####################################
#  RFX create iso
#####################################
#######################################

print "\n$RFXver create iso\n";

chdir "../$RFXbuildtree" || die "chdir death  :  $RFXbuildtree  :  $!\n";

system qq(pwd);

cIso ($RFXver);

#######################################
#####################################
#  return to origin $CWD
#####################################
#######################################

chdir "$CWD"  || die "chdir death  :  $CWD  :  $!\n";
print "\n";
system qq(pwd);

#######################################
#####################################
#  TUIP create iso 
#####################################
#######################################

print "\n\n$TUIPver create iso\n";

chdir "../$TUIPbuildtree" || die "chdir death  :  $TUIPbuildtree  :  $!\n";;

system qq(pwd);

cIso ($TUIPver);

#######################################
#####################################
#  sub cIso
#####################################
#######################################

sub cIso {

   $valueIn = shift;

   print "\n\nvalueIn  =  $valueIn\n\n";

   if ("$valueIn" =~ m/$RFXver/ )  {
      print "\n\n$RFXver  :    mkisofs build\n\n";
      print "\nmkiR  =\n\n$mkiR\n\n";
      system qq($mkiR);
   }
   if ("$valueIn" =~ m/$TUIPver/ ) {
      print "\n\n$TUIPver  :   mkisofs build\n\n";
      print "\nmkiT  =\n\n$mkiT\n\n";
      system qq($mkiT);
   }

}

##########################################################
##########################################################

print "\n\nEND  :  bRcreateISO.pl\n\n";

##########################################################
##########################################################

close (STDERR);
close (STDOUT);

