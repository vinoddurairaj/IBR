#!/usr/bin/perl -Ipms
###################################
#################################
# bRMain.pl
#
#   to run a build, but first check "if source has changed to determine"
#       to build, or not to build (no source change, no build), 
#       with number incremention, cvs commits etc.,
#       perl -w bRMain.pl 
#
#   to force a  build, full, with number incremention, cvs commits etc.,
#       perl -w bRMain.pl -f
#
#################################
###################################

###################################
#################################
#  vars  /  pms  /  use 
#################################
###################################

eval 'require bRglobalvars';
eval 'require bDATE';

our $branchdir;
our $COMver;

our $GMTMINUTE;
our $GMTHOUR;
our $GMTMONTH;
our $GMTDAY;
our $GMTYEAR;

###################################
#################################
#  exec statements 
#################################
###################################

print "\nSTART  :  bRMain.pl\n\n";

get_GMT_date();

###################################
#################################
#  log values 
#################################
###################################

print "\nbuild GMT Date        :  $GMTMONTH  $GMTDAY  $GMTYEAR\n";
print "\nbuild START GMT Time  :  $GMTMINUTE:$GMTHOUR\n";
print "\nCOMver                =  $COMver\n";
print "\nbranchdir             =  $branchdir\n";

##################################
##################################
#  getOpt  : --force supplied ?
##################################
####################################

use Getopt::Long;

my %options;
GetOptions (\%options, 
   'force',			# -f or --force
);

# set $force var to 1 if defined

my $force = $options{force}	? 1 : '';  		#COND ? THEN : ELSE

###################################
#################################
# cvs update : btools only
#################################
###################################

system qq(cvs update);

###################################
#################################
# run bRrdiff.pl
#    if $force not set to value 
#################################
###################################

open (FHX0, ">logs/RFXMAIN.txt");

if ($force ) {

    print "\nFORCE BUILD SUPPLIED    :  skip rdiff step\n";
    print FHX0 "\n\nrunning the build .....\n\n";
}

else {	# default for nightly build

   system qq(perl bRrdiff.pl);

   if ($? == "0" ) {
      print FHX0 "\nbRrdiff.pl returned  : \trc = $?\n";
      print FHX0 "\n\nrunning the build .....\n\n";
   }
   else {
      print FHX0 "\nbRrdiff.pl returned  : \trc = $?\n";
      print FHX0 "\nskip GMT Date        :  $GMTMONTH  $GMTDAY  $GMTYEAR\n";
      print FHX0 "\nskip GMT Time        :  $GMTMNUTE:$GMTHOUR\n";
      system qq(perl bRnobuildemail.pl);
      print FHX0 "\n\nNO build ..... NO changes reported.....\n\n";
      exit 0;
   }
}

close (FHX0);

###################################
#################################
#  exec build scripts that build product, images, deliverables,
#    CM reports, notifications, etc. 
#################################
###################################

print "\n\nexec status  :  $COMver build\n\n";
system qq(date);
print "\n\n";

open (STDOUT, ">logs/BUILDMAIN.txt");
open (STDERR, ">&STDOUT.txt");

system qq(perl bRbuildnuminc.pl);
    print "\nbRbuildnuminc.pl rc = $?\n";

system qq(perl bRsetGMT.pl);
    print "\nbRsetGMT.pl rc = $?\n";

system qq(perl bRmakeVARS.pl);		# gmake vars.pm  : cvs commit
    print "\nbRmakeVARS.pl rc = $?\n";

system qq(perl bRcvstag.pl);
    print "\nbRcvstag.pl rc = $?\n";

system qq(perl bRdirs.pl);
    print "\nbRdirs.pl rc = $?\n";

system qq(perl bRthreadCALLS.pl);
    print "\nbRthreadCALLS.pl rc = $?\n";

system qq(perl bRftppkg.pl);	# merged bRftppkg.pl and bIftppkg.pl
    print "\nbRftppkg-merge.pl rc = $?\n";

#system qq(perl bIftppkg.pl);	# ditto
#    print "\nbIftppkg.pl rc = $?\n";

system qq(perl bRcp3rdParty.pl);
    print "\nbRcp3rdParty.pl rc = $?\n";

#system qq(perl bIcopy_3rdparty.pl);
    #print "\nbIcopy_3rdparty.pl rc = $?\n";

system qq(perl bRcpDocs.pl);
    print "\nbRcpDocs.pl rc = $?\n";

#system qq(perl bIdocs.pl);
    #print "\nbIdocs.pl rc = $?\n";

system qq(perl bRtarImage.pl);
    print "\nbRtarImage.pl rc = $?\n";

#system qq(perl bItarImage.pl);
    #print "\nbItarImage.pl rc = $?\n";

chdir "iso";
system qq(pwd);

system qq(perl -I.. bRcreateISO.pl);
    print "\nbRcreateISO.pl rc = $?\n";

#system qq(perl -I.. bIcreateISO.pl);
    #print "\nbIcreateISO.pl rc = $?\n";

chdir "..";
system qq(pwd);

system qq(perl bRrefineGold.pl);
    print "\nbRrefineGold.pl rc = $?\n";

system qq(perl bRrefineGold.pl);
    print "\nbRrefineGold.pl rc = $?\n";

system qq(perl bRftpOMP.pl);
    print "\nbRftpOMP.pl rc = $?\n";

system qq(perl bRtarOMP.pl);
    print "\nbRtarOMP.pl rc = $?\n";

system qq(perl bRftpBAD.pl);
    print "\nbRftpBAD.pl rc = $?\n";

system qq(perl bRtarBAD.pl);
    print "\nbRtarBAD.pl rc = $?\n";

system qq(perl bRcvsWR.pl);
    print "\nbRcvsWR.pl rc = $?\n";

system qq(perl bRmd5sum.pl);
    print "\nbRmd5sum.pl rc = $?\n";

system qq(perl bRparlog.pl);
    print "\nbRparlog.pl rc = $?\n";

system qq(perl bRftpBuildServer.pl);
    print "\nbRftpBuildServer.pl rc = $?\n";

#system qq(perl bRftpSAN.pl);
    #print "\nbRftpSAN.pl rc = $?\n";

#system qq(perl bIftpSAN.pl);
    #print "\nbIftpSAN.pl rc = $?\n";

system qq(perl bRtarDELIV.pl);
    print "\nbRtarDELIV.pl rc = $?\n";

system qq(perl bRtarDRIVER.pl);
    print "\nbRtarDRIVER.pl rc = $?\n";

system qq(perl bRemail.pl);
    print "\nbRemail rc = $?\n";

chdir "qa.tools";
system qq(pwd);

#system qq(perl bRthreadQA.pl);
#    print "\nbRthread.pl rc = $?\n";

chdir "..";
system qq(pwd);

system qq(perl bRcvscommit.pl);
    print "\nbRcvscommit.pl rc = $?\n";

#system qq(perl bRresetPrevious.pl);
#    print "\nbRresetPrevious.pl rc = $?\n";

system qq(perl bRrmPreviousBldTree.pl);
    print "\nbRrmPreviousBldTree.pl rc = $?\n";

#chdir "totalWR";
#system qq(pwd);

#system qq(perl -I.. bRtotalWR.pl);
#    print "\nbRtotalWR.pl rc = $?\n";

#chdir "..";
#system qq(pwd);

print "\nEND  :  Build Run  :  $Bbnum\n";

###################################
###################################

print "\n\nEND  :  bRMain.pl\n\n";

###################################
###################################

close (STDERR);
close (STDOUT);

