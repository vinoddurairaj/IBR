#!/usr/bin/perl -Ipms
#######################################
#####################################
#
#  perl -w ./bRrefineGold.pl
#
#    create $RFXver.$Bbnum.refineGold.iso
#           $TUIPver.$Bbnum.refineGold.iso
#
#	builds/$Bbnum/imageRefineGold
#
#####################################
#######################################
 
#######################################
#####################################
# vars  /  pms   /  use
#####################################
#######################################

eval 'require bRglobalvars';
eval 'require bRrefineGold';
eval 'require bRtreepath';
eval 'require bPWD';

our $COMver;
our $RFXver;
our $TUIPver;
our $Bbnum;

our @RFXrefineGold;
our @TUIPrefineGold;
our @common;

our $CWD;
our $RFXbuildtree;
our $TUIPbuildtree;

#######################################
#####################################
# STDOUT  / STDERR
#####################################
#######################################

#open (STDOUT, ">logs/refinegold.txt");
#open (STDERR, ">&STDOUT");

#######################################
#####################################
#  exec statements
#####################################
#######################################

print "\n\nSTART  :  bRrefineGold.pl\n\n";

#######################################
#####################################
#  log values 
#####################################
#######################################

print "\n\nCOMver     =  $COMver\n";
print "\nRFXver     =  $RFXver\n";
print "\nTUIPver    =  $TUIPver\n";
print "\nBbnum      =  $Bbnum\n\n";

print "\nRFXbuildtree   =  $RFXbuildtree\n";
print "\nTUIPbuildtree  =  $TUIPbuildtree\n\n";

system qq(pwd);
print "\n\n";
system qq(date);
print "\n\n";

#######################################
#####################################
#  sub establishPWD to enable var $CWD with pwd
#####################################
#######################################

establishPWD();

print "\nsub establishPWD returned  :  $CWD\n\n";

#######################################
#####################################
#  RFX refine Gold
#####################################
#######################################

print "\n$RFXver refine gold routine\n";

chdir "$RFXbuildtree" || die "chdir death  :  $RFXbuildtree  :  $!\n";

system qq(pwd);

refineGold($RFXver);

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
#  TUIP refine Gold 
#####################################
#######################################

print "\n\n$TUIPver refine gold routine\n";

chdir "$TUIPbuildtree" || die "chdir death  :  $TUIPbuildtree  :  $!\n";;

system qq(pwd);

refineGold ($TUIPver);

#######################################
#######################################

print "\n\nEND  :  bRrefineGold.pl\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);

#######################################
#####################################
#  sub refineGold 
#####################################
#######################################

sub refineGold {

   my $valueIn= shift;

   print "\n\nvalueIn  =  $valueIn\n\n";

   if ("$valueIn" =~ m/$RFXver/ )  { 
      print "\nRFX\n";
      @list=@RFXrefineGold; 
      $PRODUCT="$RFXver";
   }
   if ("$valueIn" =~ m/$TUIPver/ ) { 
      print "\nTUIP\n"; 
      @list=@TUIPrefineGold; 
      $PRODUCT="$TUIPver";
   }

   foreach $ePATHFILE (@list) {
      print "\nePATHFILE  =\n$ePATHFILE\n";
      system qq(tar rvf $PRODUCT.$Bbnum.refineGold.tar $ePATHFILE);
   }
   foreach $eCOMMON (@common) {
      print "\neCOMMON  =\n$eCOMMON\n";
      system qq(tar rvf $PRODUCT.$Bbnum.refineGold.tar $eCOMMON);
   }
   
   mkdir "refinegoldimage";
   chdir "refinegoldimage";
   system qq(pwd);
   system qq(tar xvf ../$PRODUCT.$Bbnum.refineGold.tar);
   chdir "..";
   system qq(pwd);
   our $PATCHver;
   our $CDLABEL="$PRODUCT-$PATCHver-$Bbnum";
   our $UID="-uid 0 -gid 0 -file-mode 555";
   our $op0="-f -hide-rr-moved -J -R -hfs -max-iso9660-filename";
   our $op1="-L -o $PRODUCT.$Bbnum.refinegold.iso  -U -v -V $CDLABEL $UID refinegoldimage";
   our $mki="mkisofs $op0 $op1";
   system qq($mki);
   undef(@list);
   undef($PRODUCT);

}
