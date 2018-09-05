#!/usr/bin/perl -Ipms
#######################################
#####################################
#
#  perl -w ./bRcp3rdParty.pl
#
#    cp 3rd party deliverables to product image 
#	 
#	builds/$Bbnum       :  RFX  intermediate target for pre-iso
#	builds/tuip/$Bbnum  :  TUIP intermediate target for pre-iso
#
#####################################
#######################################
 
#######################################
#####################################
#  vars   /  pms  /  use
#####################################
#######################################

eval 'require bRglobalvars';
eval 'require bRtreepath';
eval 'require bPWD';

our $COMver;
our $Bbnum;

our $CWD;

our $RFXbuildtree;
our $TUIPbuildtree;
our $RFXver;
our $TUIPver;

our $REDHATRPM="Redist/RedHat/RPM";

our $PARTYpath;
our $valueIn;

#######################################
#####################################
#  @Redist list  :  define required deliverables to process
#####################################
#######################################

our @Redist= ("Redist/RedHat/RPM/blt-2.4u-8.i386.rpm");
chomp (@Redist);

#######################################
#####################################
#  STDOUT  /  STDERR
#####################################
#######################################

open (STDOUT, ">logs/cp3rdparty.txt");
open (STDERR, ">>&STDOUT");

#######################################
#####################################
#  exec statements
#####################################
#######################################

print "\n\nSTART  :  bRcp3rdparty.pl\n\n";

#######################################
#####################################
#  log values
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
#  RFX cp 3rd Party 
#####################################
#######################################

print "\n$RFXver cp routine\n";

chdir "$RFXbuildtree"  || die "chdir death  :  $RFXbuildtree  :  $!\n";

system qq(pwd);

redist ($RFXver);

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
#  TUIP cp 3rd Party 
#####################################
#######################################

print "\n\n$TUIPver cp routine\n";

chdir "$TUIPbuildtree" || die "chdir death  :  $TUIPbuildtree  :  $!\n";

system qq(pwd);

redist ($TUIPver);

#######################################
#####################################
#  sub redist
#####################################
#######################################

sub redist {

   $valueIn = shift;

   print "\n\nvalueIn  =  $valueIn\n\n";

   if ("$valueIn" =~ m/$RFXver/ )  { $PARTYpath="../../3rdparty"; }
   if ("$valueIn" =~ m/$TUIPver/ ) { $PARTYpath="../../../3rdparty"; }

   foreach $eRD (@Redist) {

      print "\nRedist :  cp  :  $eRD\n";
      print "\n\nPARTYpath  =  $PARTYpath\n";

      system qq(ls -l $PARTYpath/$eRD);
      system qq(cp $PARTYpath/$eRD $REDHATRPM);
      system qq(ls -l $REDHATRPM);
   }
}

#####################################
#######################################

print "\n\nEND bRcopy_3rdparty.pl\n\n";

close (STDERR);
close (STDOUT);

