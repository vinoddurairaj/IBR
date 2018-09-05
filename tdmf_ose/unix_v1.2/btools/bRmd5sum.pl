#!/usr/bin/perl -Ipms
#######################################
#####################################
#
#  perl -w ./bRmd5sum.pl
#
#    gather / report md5sum against  :
#         ($RFW)($TWIP)ver.$Bbnum.gold.iso 
#
#    output to $(rfx)($tuip)SUM  :  pms/bRmd5sum.pm
#
#####################################
#######################################
 
#######################################
#####################################
# vars  /  pms   /  use
#####################################
#######################################

eval 'require bRglobalvars';
eval 'require bRtreepath';
eval 'require bPWD';

our $COMver;
our $RFXver;
our $TUIPver;
our $Bbnum;

our $RFXbuildtree;
our $TUIPbuildtree;

our $rfxSUM;
our $tuipSUM;

#######################################
#####################################
# STDOUT  / STDERR
#####################################
#######################################

open (STDOUT, ">logs/md5sum.txt");
open (STDERR, ">>&STDOUT");

#######################################
#####################################
#  exec statements
#####################################
#######################################

print "\n\nSTART  :  bRmd5sum.pl\n\n";

unlink "pms/bRmd5sum.pm";

#######################################
#####################################
#  log values 
#####################################
#######################################

print "\n\nCOMver             =  $COMver\n";
print "\nRFXver             =  $RFXver\n";
print "\nTUIPver            =  $TUIPver\n";
print "\nBbnum              =  $Bbnum\n\n";
print "\nRFXbuildtree       =  $RFXbuildtree\n\n";
print "\nTUIPbuildtree      =  $TUIPbuildtree\n\n";

#######################################
#####################################
#  sub establishPWD to enable var $CWD with pwd
#####################################
#######################################

establishPWD();

print "\nsub establishPWD returned  :  $CWD\n\n";

#######################################
#####################################
#  RFX generate md5sum check sum
#####################################
#######################################

print "\n$RFXver create check sum routine\n\n";

chdir "$RFXbuildtree" || die "chdir death  :  $RFXbuildtree  :  $!\n";

system qq(pwd);

createCS ($RFXver);

print "\nreturn rfxSUM  =  $rfxSUM\n";

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
#  TUIP generate md5sum check sum 
#####################################
#######################################

print "\n\n$TUIPver check sum routine\n\n";

chdir "$TUIPbuildtree" || die "chdir death  :  $TUIPbuildtree  :  $!\n";;

system qq(pwd);

createCS ($TUIPver);

print "\nreturn tuipSUM  =  $tuipSUM\n";

#######################################
#####################################
#  return to origin $CWD
#####################################
#######################################

chdir "$CWD"  || die "chdir death  :  $CWD  :  $!\n";
print "\n";
system qq(pwd);

#######################################
#######################################
#####################################
#  format (strip trailing filename)  $rfxSUM  :  $tuipSUM
#####################################
#######################################

$rfxSUM=~s/\s+.*$//;
print "\nstripped  rfxSUM  =  $rfxSUM\n";;

$tuipSUM=~s/\s+.*$//;
print "\nstripped tuipSUM  =  $tuipSUM\n";;
chomp($rfxSUM);
chomp($tuipSUM);

#######################################
#####################################
#  dump $(rfx)($tuip)SUM to pms/bRmd5sums.pm 
#####################################
#######################################

open(FHsum,">pms/bRmd5sum.pm");

print FHsum "\$rfxSUM=\"$rfxSUM\"\;";
print FHsum "\n";
print FHsum "\$tuipSUM=\"$tuipSUM\"\;";

close(FHsum);

print "\nlog contents pms/bRmd5sum.pm\n";
system qq(cat pms/bRmd5sum.pm);
print "\n";

system qq(cvs commit -m "$Bbnum  :  autobuild commit " pms/bRmd5sum.pm);

#######################################
#######################################

print "\n\nEND  :   bRmd5sum.pl\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);

#######################################
#####################################
#  sub createCS 
#####################################
#######################################

sub createCS {

   $valueIn = shift;

   print "\n\nvalueIn  =  $valueIn\n\n";

   if ("$valueIn" =~ m/$RFXver/ )  {
      print "\nRFX checksum generation\n";
      $rfxSUM=`md5sum $RFXver.$Bbnum.gold.iso`;
      print "\nrfxSUM  =  $rfxSUM\n";
    }
   if ("$valueIn" =~ m/$TUIPver/ ) {
      print "\nTUIP checksum generation\n";
      $tuipSUM=`md5sum $TUIPver.$Bbnum.gold.iso`;
      print "\ntuipSUM  =  $tuipSUM\n";
   }

}

