#!/usr/bin/perl -Ipms
#######################################
#####################################
#
#  perl -w ./bRtarImage.pl
#
#    create product image tar archives
#    create unique  image tar archives
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

open (STDOUT, ">logs/tarimage.txt");
open (STDERR, ">&STDOUT");

#######################################
#####################################
#  exec statements 
#####################################
#######################################

print "\n\nSTART : bRtarImage.pl\n\n";

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

print "\n$RFXver create tar image routine\n\n";

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

print "\n\n$TUIPver tar images routine\n\n";

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

   system qq( tar cvf $PRODUCT.$Bbnum.temp.tar AIX doc HPUX Linux Redist solaris );

   #create image directory and tar archive  *ALL*

   system qq(mkdir -p image/Softek/$PRODUCTname);

   chdir "image/Softek/$PRODUCTname" || die "chdir death  :  $PRODUCTname  :  $!";

   system qq(pwd);

   system qq( tar xvf ../../../$PRODUCT.$Bbnum.temp.tar );

   chdir "../..";
 
   system qq(pwd);

   system qq(sudo chown -R root:root *);

   system qq( tar cvf ../$PRODUCT.$Bbnum.ALL.tar Softek/$PRODUCTname/AIX/* Softek/$PRODUCTname/doc Softek/$PRODUCTname/HPUX Softek/$PRODUCTname/Linux Softek/$PRODUCTname/Redist Softek/$PRODUCTname/solaris );

   # tar each OS  separately

   system qq( tar cvf ../$PRODUCT.$Bbnum.solaris.tar Softek/$PRODUCTname/solaris/* );
   system qq( tar cvf ../$PRODUCT.$Bbnum.aix.tar Softek/$PRODUCTname/AIX/* );
   system qq( tar cvf ../$PRODUCT.$Bbnum.hpux.tar Softek/$PRODUCTname/HPUX/* );
   system qq( tar cvf ../$PRODUCT.$Bbnum.linux.tar Softek/$PRODUCTname/Linux/* );
   system qq( tar cvf ../$PRODUCT.$Bbnum.Redist.tar Softek/$PRODUCTname/Redist/* );

chdir "..";

system qq(pwd);

unlink "$PRODUCT.$Bbnum.temp.tar";

}

#######################################
#######################################

print "\n\nEND  :  bRtarImage.pl\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);
