#!/usr/bin/perl -Ipms
#######################################
#####################################
#
#  perl -w ./bRftpBuildServer.pl
#
#    ftp deliverables /  logs to build server 
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
eval 'require bRlogList';
eval 'require bPWD';

our $Bbnum;
our $COMver;
our $RFXver;
our $TUIPver;

our $CWD;
our @LOGS;
our @genLOGS;

our $bPSWDF="../../btools/bmachine.txt";
our $btoolsPATH="../../btools";
our $RFXtarget="/builds/Replicator/UNIX/$RFXver/$Bbnum";
our $TUIPtarget="/'builds/TDMFUNIX\(IP\)'/$TUIPver/$Bbnum";

our $passPATH;
our $PRODUCT;
our $TARGET;

#######################################
#####################################
#  STDOUT  /  STDERR
#####################################
#######################################

open (STDOUT, ">logs/ftpbuildserver.txt");
open (STDERR, ">&STDOUT");

#######################################
#####################################
#  exec statements
#####################################
#######################################

print "\n\nSTART  :  bRftpBuild.Server.pl\n\n";

#######################################
#####################################
#  log values 
#####################################
#######################################

print "\n\nCOMver       =  $COMver\n";
print "\nRFXver         =  $RFXver\n";
print "\nTUIPver        =  $TUIPver\n";
print "\nBbnum          =  $Bbnum\n";
print "\nRFXbuildtree   =  $RFXbuildtree\n";
print "\nTUIPbuildtree  =  $TUIPbuildtree\n";
print "\nRFXtarget      =  $RFXtarget\n";
print "\nTUIPtarget     =  $TUIPtarget\n\n";

#######################################
#####################################
#  sub establishPWD to enable $CWD with pwd 
#####################################
#######################################

establishPWD();

print "\nsub establishPWD returned  :  $CWD\n\n";

#######################################
#####################################
#  RFX ncftpput
#####################################
#######################################

print "\n$RFXver ncftp routine output\n\n";

chdir "$RFXbuildtree"  || die "chdir death  :  $RFXbuildtree  :  $!\n";

system qq(pwd);

ncFTP($RFXver);

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
#  TUIP ncftpput
#####################################
#######################################

print "\n$TUIPver ncftp routine output\n\n";

chdir "$TUIPbuildtree"  || die "chdir death  :  $TUIPbuildtree  :  $!\n";

system qq(pwd);

ncFTP($TUIPver);

#######################################
#######################################

print "\n\nEND  :  bRftpBuildServer.pl\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);

#######################################
#####################################
#  sub ncFTP 
#####################################
#######################################

sub ncFTP {

  $valueIn = shift;

   print "\n\nvalueIn  =  $valueIn\n\n";

   if ("$valueIn" =~ m/$RFXver/ )  { 
      $PRODUCT="$RFXver";
      $passPATH="$bPSWDF";
      $logPATH="$btoolsPATH";
      $TARGET="$RFXtarget";

   }
   if ("$valueIn" =~ m/$TUIPver/ ) {
      $PRODUCT="$TUIPver";
      $passPATH="../$bPSWDF";
      $logPATH="../$btoolsPATH";
      $TARGET="$TUIPtarget";

   }
   print "\nncftp routine via product  :  $valueIn\n\n";
   print "\nPRODUCT    =  $PRODUCT\n";
   print "\npassPATH   =  $passPATH\n";
   print "\nTARGET     =  $TARGET\n";  
   print "\nlogPATH    =  $logPATH\n\n";  

   foreach $eGEN (@genLOGS) {
      if (-e "$logPATH\/$eGEN" ) { 
         print "logPATH/eGEN  =  $logPATH\/$eGEN\n";
         system qq(cp "$logPATH\/$eGEN" .);
      }
   }

   foreach $eLOG (@LOGS) {
      if ($PRODUCT =~ /$RFXver/ ) {
         $eRLOG="$logPATH/$eLOG";
         print "\neRLOG  =  $eRLOG\n";
         if (-e "$eRLOG" && "$eRLOG" !~ m/tuip\.txt/) {
            print "\nexists  :  $eRLOG\n";
            system qq(ls -la "$eRLOG");
            print "\n\n";
            system qq(cp "$eRLOG" .);
         }
      }
      if ($PRODUCT =~ /$TUIPver/ ) {
         $eTLOG="$logPATH/$eLOG";
         print "\neTLOG  :  $eTLOG\n";
         if (-e "$eTLOG") {
            print "\nexists  :  $eTLOG\n";
            system qq(ls -la "$eTLOG");
            print "\n\n";
            system qq(cp "$eTLOG" .);
         }
      }

   }

system qq(ncftpput -f $passPATH -m $TARGET/LOGS  *.txt);
system qq(ncftpput -f $passPATH -m $TARGET *.iso );
system qq(ncftpput -f $passPATH -m $TARGET/binArchive *aix.tar);
system qq(ncftpput -f $passPATH -m $TARGET/binArchive *ALL.tar);
system qq(ncftpput -f $passPATH -m $TARGET/binArchive *hpux.tar);
system qq(ncftpput -f $passPATH -m $TARGET/binArchive *linux.tar);
system qq(ncftpput -f $passPATH -m $TARGET/binArchive *Redist.tar);
system qq(ncftpput -f $passPATH -m $TARGET/binArchive *refineGold.tar);
system qq(ncftpput -f $passPATH -m $TARGET/binArchive *solaris.tar);
system qq(ncftpput -f $passPATH -m $TARGET/BAD *BAD.tar);
system qq(ncftpput -f $passPATH -m $TARGET/OMP *OMP.tar);

print "\n\n";
}


