#!/usr/bin/perl -Ipms
#######################################
#####################################
#
#  perl -w ./bRtarBAD.pl
#
#    create BAD image tar archives
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

our $DOCpath;
our $valueIn;

#######################################
#####################################
#  STDOUT  /  STDERR 
#####################################
#######################################

open (STDOUT, ">logs/tarbad.txt");
open (STDERR, ">&STDOUT");

#######################################
#####################################
#  exec statements 
#####################################
#######################################

print "\n\nSTART : bRtarbad.pl\n\n";

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
#  RFX tar cvf deliverables 
#####################################
#######################################

print "\n$RFXver create tar BAD routine\n\n";

chdir "$RFXbuildtree" || die "chdir death  :  $RFXbuildtree  :  $!\n";

system qq(pwd);

tarImages ($RFXver);

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
#  TUIP  cvf deliverables
#####################################
#######################################

print "\n\n$TUIPver tar BAD routine\n\n";

chdir "$TUIPbuildtree" || die "chdir death  :  $TUIPbuildtree  :  $!\n";;

system qq(pwd);

tarImages ($TUIPver);

#######################################
#####################################
#  sub tarImages
#####################################
#######################################

sub tarImages {

   $valueIn = shift;

   print "\n\nvalueIn  =  $valueIn\n\n";

   if ("$valueIn" =~ m/$RFXver/ )  { 
      $PRODUCT="$RFXver";
      $PRODUCTname="Replicator";
    }
   if ("$valueIn" =~ m/$TUIPver/ ) {
      $PRODUCT="$TUIPver";
      $PRODUCTname="TDMFIP";
   }

   print "\n\n$valueIn  :  PRODUCT  =  $PRODUCT\n";
   print "\n\nPRODUCTname           =  $PRODUCTname\n";

   system qq(pwd);

   system qq( tar cvf $PRODUCT.$Bbnum.BAD.tar BAD );

}

#######################################
#######################################

print "\n\nEND  :  bRtarBAD.pl\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);
