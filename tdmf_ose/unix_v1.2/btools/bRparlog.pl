#!/usr/bin/perl -Ipms -Ivalidation
#######################################
#####################################
#
#  perl -w ./bRparlog.pl
#
#    parse logs for deviant build messages
#    create build notification
#    create build log
#
#####################################
#######################################
 
#######################################
#####################################
#  vars  /  pms  /  use 
#####################################
#######################################

eval 'require bRglobalvars';
eval 'require bRcvstag';
eval 'require bRpreviouscvstag';
eval 'require bRbuildGMT';

eval 'require bRstrippedbuildnumber';
eval 'require bRcheckDeliverables';
eval 'require bIcheckDeliverables';
eval 'require bRcheckSize';
eval 'require bIcheckSize';
eval 'require bRcheckSpecial';
eval 'require bIcheckSpecial';
eval 'require bRcheckOMP';
eval 'require bRcheckBAD';
eval 'require bRlogList';
eval 'require bRmd5sum';

require Text::Wrapper;

if (-e "pms/bRpreviousbuildGMT.pm" ) { eval 'require bRpreviousbuildGMT' }
else {our $previousbuildGMT="00:00";}
if (-e "pms/bRbuildGMT.pm" ) { eval 'require bRbuildGMT' }
else {our $currentbuildGMT="00:00";}

our $COMver;
our $RFXver;
our $TUIPver;
our $Bbnum;
our $PATCHver;

our $stripbuildnum;

our $currentbuildGMTYEAR;
our $currentbuildGMTMONTH;
our $currentbuildGMTDAY;
our $currentbuildGMT;
our $previousbuildGMTYEAR;
our $previousbuildGMTMONTH;
our $previousbuildGMTDAY;
our $previousbuildGMT;
our $CBdate="$currentbuildGMTYEAR $currentbuildGMTMONTH $currentbuildGMTDAY";
our $CBtime="$currentbuildGMT";

our $CVSTAG;
our $previouscvstag;

our $rfxSUM;
our $tuipSUM;

our @LOGS;
our @RFXBUILD;
our @UNSTABLE;
our @bldcmdList;
our @warnfilesize;
our @warnfilesizetuip;
our @validatemissing;
our @validatemissingtuip;
our @missingdeliverables;
our @missingdeliverablestuip;
our @ompStatus;

#######################################
#####################################
#  STDOUT  /  STDERR
#####################################
#######################################

open (STDOUT, ">logs/parlog.txt");
open (STDERR, ">&STDOUT");

#######################################
#####################################
#  exec statements 
#####################################
#######################################

print "\n\nSTART  :  bRparlog.pl\n\n";

#######################################
#####################################
#  log values 
#####################################
#######################################

print "\nCOMver                  =  $COMver\n";
print "\nRFXver                  =  $RFXver\n";
print "\nTUIPver                 =  $TUIPver\n";
print "\nBbnum                   =  $Bbnum\n";
print "\nrfxSUM                  =  $rfxSUM\n";
print "\ntuipSUM                 =  $tuipSUM\n";
print "\ncurrentbuildGMTYEAD     =  $currentbuildGMTYEAR\n";
print "\ncurrentbuildGMTMONTH    =  $currentbuildGMTMONTH\n";
print "\ncurrentbuildGMTDAY      =  $currentbuildGMTDAY\n";
print "\ncurrentbuildGMT         =  $currentbuildGMT\n";
print "\nCBdate                  =  $CBdate\n";
print "\nCBtime                  =  $CBtime\n\n";
print "\npreviousbuildGMTYEAD    =  $previousbuildGMTYEAR\n";
print "\npreviousbuildGMTMONTH   =  $previousbuildGMTMONTH\n";
print "\npreviousbuildGMTDAY     =  $previousbuildGMTDAY\n";
print "\npreviousbuildGMT        =  $previousbuildGMT\n";
print "\n\nCVSTAG (current)      =  $CVSTAG\n";
print "\npreviouscvstag          =  $previouscvstag\n";
print "\nstripbuildnum           =  $stripbuildnum\n\n";

print "\nLOGS  =\n@LOGS\n";

#######################################O
#####################################
# @LOGS list used to create @RFXBUILD 
#####################################
#######################################

foreach $e0 (@LOGS) {
   print "\nLOGFILE  =  $e0\n";
   system qq(ls -l $e0);
   print "\n\n";
   open (FHLOG,"<$e0");
   @logarray=<FHLOG>;
   close (FHLOG); 
   foreach $ePUSH (@logarray) {
      push (@RFXBUILD,"$ePUSH");
   }
   undef (@logarray); 
}
#######################################
#####################################
# create RFXBUILDLOG.txt (all PLAT / OS build logs) 
#####################################
#######################################

if (-e "logs/RFXBUILDLOG.txt" ) { unlink "logs/RFXBUILDLOG.txt"; }

open (FHRFX,">logs/RFXBUILDLOG.txt");
print "\n\nRFXBUILD length = $#RFXBUILD\n\n";
print FHRFX @RFXBUILD;
close (FHRFX);

#######################################
#####################################
#  parse for bldcmd  &&  "rc =" 
#####################################
#######################################

foreach $eRC (@RFXBUILD) {
   chomp($eRC);  #required to parse uniq / sort
   #print "\neRC = $eRC\n";
   if ("$eRC" =~ /bldcmd/ && "$eRC" =~ /rc\ =/ ) {
      print "\n\nIF  :  $eRC\n\n";
      push (@bldcmdList,"$eRC\n");
   }
   elsif ("$eRC" =~ /gmake\[[0-9]\]/ && "$eRC" =~ /Error\ [0-9]/ ) {
      print "\nelsif  :  add to \@UNSTABLE  =~  $eRC\n";
      push (@UNSTABLE,"$eRC\n");
   }
   elsif ("$eRC" =~ /gmake/ && "$eRC" =~ /Error\ [1-9]/ ) {
      print "\nelsif  :  add to \@UNSTABLE  =~  $eRC\n";
      push (@UNSTABLE,"$eRC\n");
   }
}

#######################################
#####################################
#   check for known error strings routine
#####################################
#######################################

foreach $eCKRC (@RFXBUILD) {
   if ("$eCKRC" =~ m/error\!/ && "$eCKRC" =~ m/blocksize/ && "$eCKRC" =~ m/Tar/i) {
      print "\nknown error routine match  :\n$eCKRC\n";
      push (@UNSTABLE,"$eCKRC\n");
   }
   if ("$eCKRC" =~ m/ERROR/ && "$eCKRC" =~ m/expected\ depot/ && "$eCKRC" =~ m/not\ exist/i) {
      print "\nknown error routine match  :\n$eCKRC\n";
      push (@UNSTABLE,"$eCKRC\n");
   }

   if ("$eCKRC" =~ m/ERROR/ && "$eCKRC" =~ m/Cannot\ access/ && "$eCKRC" =~ m/source/i) {
      print "\nknown error routine match  :\n$eCKRC\n";
      push (@UNSTABLE,"$eCKRC\n");
   }
   if ("$eCKRC" =~ m/ERROR/ && "$eCKRC" =~ m/Cannot\ read/ && "$eCKRC" =~ m/depot/i) {
      print "\nknown error routine match  :\n$eCKRC\n";
      push (@UNSTABLE,"$eCKRC\n");
   }
}

#######################################
#####################################
#  validation routine  :  bRcheckOMP.pm 
#  returns  :  @ompStatus
#####################################
#######################################

checkOMP();

if ($#ompStatus > -1 ) {
    foreach $eOMP (@ompStatus) {
      push (@UNSTABLE,"missing  :  $eOMP\n");
    }
}

#######################################
#####################################
#  validation routine  :  bRcheckBAD.pm
#  returns  :  @badStatus
#####################################
#######################################

checkBAD();

if ($#badStatus > -1 ) {
    foreach $eBAD (@badStatus) {
      push (@UNSTABLE,"missing  :  $eBAD\n");
    }
}

#######################################
#####################################
#  validation routine  :  b(I)(R)checkDeliverables.pm 
#
#  returns  :  @missingdeliverables  :  @missingdeliverablestuip
#####################################
#######################################

checkdelstatus();

checkdelstatustuip();

if ($#missingdeliverables > -1 ) {
    foreach $eMD (@missingdeliverables) {
      push (@UNSTABLE,"RFX missing  :  $eMD\n");
    }
}
if ($#missingdeliverablestuip > -1 ) {
    foreach $eMDT (@missingdeliverablestuip) {
      push (@UNSTABLE,"TUIP missing  :  $eMDT\n");
    }
}

#######################################
#####################################
#  validation routine  :  b(I)(R)checkSize.pm 
#
#  returns  :  @warnfilesize  :  @validatemissing
#  returns  :  @warnfilesizetuip  :  @validatemissingtuip
#####################################
#######################################

checksize();

checksizetuip();

#test values  :  clean out when implementation successful
#@validatemissingtuip=qq(VDT  :  yo);
#@validatemissing=qq(VD   :  we are);
#@warnfilesizetuip=qq(WFST  :  under );
#@warnfilesize=qq(WFS  :  water );

if ($#validatemissingtuip > -1 ) {
   foreach $eVDT (@validatemissingtuip) {
      chomp ($eVDT);
      push (@UNSTABLE,"TUIP validate missing  :  $eVDT\n");
   }
}
if ($#validatemissing > -1 ) {
   foreach $eVD (@validatemissing) {
      push (@UNSTABLE,"RFX validate missing  :  $eVD\n");
   }
}
if ($#warnfilesizetuip > -1 ) {
   foreach $eWFST (@warnfilesizetuip) {
      push (@UNSTABLE,"TUIP file size warning  :\n$eWFST\n");
   }
}
if ($#warnfilesize > -1 ) {
   foreach $eWFS (@warnfilesize) {
      push (@UNSTABLE,"RFX file size warning  :\n$eWFS\n");
   }
}

print "\nbldcmdList        =  @bldcmdList\n";
print "\nUNSTABLE          =  @UNSTABLE\n";
print "\nUNSTABLE Length   =  $#UNSTABLE\n";

#######################################
#####################################
#  @UNSTABLE  :  uniq sort routine 
#####################################
#######################################

if (-e "uniq.temp" ) {unlink "uniq.temp";}
if (-e "uniq.list" ) {unlink "uniq.list";}

if ( $#UNSTABLE > -1) {
   open (FHu,">uniq.temp");
   print FHu @UNSTABLE;
   close (FHu);

   undef (@UNSTABLE);

   print "\nbefore cat uniq.temp\n";
   sleep 4;
   system qq(cat uniq.temp );
   print "\nafter cat uniq.temp\n";
   system qq(cat uniq.temp | uniq > uniq.list );

   open (FHuniq,"<uniq.list");
   @UNSTABLE=<FHuniq>;
   close (FHuniq);

   @UNSTABLE = sort  { lc($a) cmp lc($b) } @UNSTABLE;

   print "\n\nUNSTABLE uniq sort parse result  :\n\n@UNSTABLE\n\n";

}

#######################################
#####################################
#  @bldcmdList  :  sort routine 
#####################################
#######################################

@bldcmdList = sort  { lc($a) cmp lc($b) } @bldcmdList;

print "\nbldcmdList sort parse results  :\n\n@bldcmdList\n\n";

################################
###############################
#
#  gather source changes / frequency of changes
#  information "per build"
#
#  add $length to beginning email message
#  append source changes list to end of email message
#       from scripts bRcvsWR.pl
###############################
#################################

print "\n\nenter frequency / source changes\n\n";

open ( FHSOURCE, "<sourcechangelist.txt" );
@sourcelist=<FHSOURCE>;
close (FHSOURCE);

$i="0";

for ($i=0 ; $sourcelist[$i] ; $i++) {
   print "\nFOR : sourcelist[$i] : $sourcelist[$i]\n";
   if ( $sourcelist[$i] !~ m/./ ) {
      print "\nIF :  splice newline match : $sourcelist[$i]\n";
      splice ( @sourcelist, $i, 1);
      $i=$i - 1;
      print "\n\nsourcechangelist : i after subtract - 1 = :  $i\n";
   }

}

$sourcelistlength=$#sourcelist + 1;

print "\nsourcelist array = \n @sourcelist\n";
print "\nsourcelistlenght array length = $sourcelistlength\n";

chomp (@sourcelist);

# adding in btools tracking, but, will keep it separate

open ( FHBTOOLS, "<btoolschangelist.txt" );
@btoolslist=<FHBTOOLS>;
close (FHBTOOLS);

$i="0";

for ($i=0 ; $btoolslist[$i] ; $i++) {
   print "\nFOR : btoolslist[$i] : $btoolslist[$i]\n";
   if ( $btoolslist[$i] !~ m/./ ) {
      print "\nIF :  splice newline match : $btoolslist[$i]\n";
      splice ( @btoolslist, $i, 1);
      $i=$i - 1;
      print "\n\nbtoolslist : i after subtract - 1 = :  $i\n";
   }

}

$btoolslistlength=$#btoolslist + 1;

print "\nbtoolslist array = \n @btoolslist\n";
print "\nbtoolslistlength array length = $btoolslistlength\n";

chomp (@sourcelist);

#################################
###############################
#  set $bldstatus  : capture in pms/bRsucFail.pm
##############################
################################

if ($#UNSTABLE > -1 ) {
   $bldstatus="UNSTABLE BUILD STATUS";
}
else {
   $bldstatus="STABLE BUILD ACHIEVED";
}

print "\n\n$bldstatus\n\n";

open (FHstat,">pms/bRsucFail.pm");

print FHstat "\$SUCCEED_FAILURE=\"$bldstatus\"\;";

close (FHstat);

system qq(cvs commit -m "$Bbnum  :  autobuild commit" pms/bRsucFail.pm);

#######################################
#####################################
#  create notification.txt 
#####################################
#######################################

open (NOTI, ">notification.txt");
 
print NOTI "\n$COMver BUILD $Bbnum Build Status     :  $bldstatus\n";
print NOTI "\n$COMver BUILD $Bbnum GMT Date         :  $CBdate\n";
print NOTI "\n$COMver BUILD $Bbnum GMT Time         :  $CBtime\n";
print NOTI "\n$COMver Patch  /  Fix Version         :  $PATCHver\n";
print NOTI "\n$COMver BUILD DELIVERABLES AVAILABLE  :\n";
print NOTI "\nNSJ  :  http://dmsbuilds.sanjose.ibm.com/builds/Replicator/UNIX/$RFXver/$Bbnum\n";
print NOTI "\nNSJ  :  http://dmsbuilds.sanjose.ibm.com/builds/TDMFUNIX(IP)/$TUIPver/$Bbnum\n";
print NOTI "\n$RFXver  GOLD iso image          :  $RFXver.$Bbnum.gold.iso\n";
print NOTI "\n$RFXver  GOLD md5sum             :  $rfxSUM\n";
print NOTI "\n$TUIPver GOLD iso image          :  $TUIPver.$Bbnum.gold.iso\n";
print NOTI "\n$TUIPver GOLD md5sum             :  $tuipSUM\n";
print NOTI "\n# of corecode files changed      =  $sourcelistlength\n";
print NOTI "\n# of build scripts changed       =  $btoolslistlength\n";
print NOTI "\nCVS tag (current)                :  $CVSTAG\n";
print NOTI "\nSource mod list created via cvs tag to tag diff  :\n";
print NOTI "\n$CVSTAG     to     $previouscvstag\n";
print NOTI "\ncurrent  build tag GMT time  :  $currentbuildGMT\n";
print NOTI "\nprevious build tag GMT time  :  $previousbuildGMT\n"; 

#  if @UNSTABLE >  -1  :
#    report in this section
 
print "\n\n\@UNSTABLE Length = $#UNSTABLE\n\n";
if ("$#UNSTABLE" > "-1" ) {
print NOTI "\n\n=========$Bbnum  :  UNSTABLE  Report==============\n\n";
 
   foreach $eSTABu (@UNSTABLE) {
      print NOTI "$eSTABu\n";
      #print NOTI "$eSTABu\n";
  }
}

#  Return Code Print Section
#  commented out  :  not valid results 
#print NOTI "\n=======================$Bbnum======================\n\n";
#print NOTI "\n$RFXver :  Return Codes\n\n";
 
#foreach $eCODES (@bldcmdList) {
#   print NOTI "$eCODES\n";
   #print "\nNOTI  : $eCODES\n";
 
#}  # foreach close bracket
 
# Source change lists  :  core code  &  btools  :  respectively
# core code
 
print NOTI "\n\n=====================$Bbnum===================\n";
print NOTI "\nSource File    File Rev     Developer    WR / Message \n";
print NOTI "\n=====================$Bbnum===================\n\n";
 
foreach $elemCC0 (@sourcelist) {
  $wrapper = Text::Wrapper->new(columns => 72, body_start => '          ');
   print NOTI $wrapper->wrap($elemCC0);
   print NOTI "\n";
   print $wrapper->wrap($elemCC0);
}
 
# btools
 
print NOTI "\n\n\n=====================$Bbnum===================\n";
print NOTI "\nBuild Script  :  Environment Changes  :  btools etc.\n";
print NOTI "\nSource File :    File Rev :     WR / Message list :     $Bbnum\n";
print NOTI "\n=====================$Bbnum===================\n\n";
 
foreach $elemBT (@btoolslist) {
print NOTI "$elemBT";
print "\nbtools : $elemBT\n";
}
 
close (NOTI);

#######################################
#######################################

print "\n\nEND bRparlog.pl\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);

#######################################
#####################################
#SAVE incase we need to implement in future
#####################################
#######################################

#######################################
#####################################
#  special polybin build required ? 
#####################################
#######################################
#if ($SPECIAL) { print NOTI "\n$SPECIAL\n"; }
#if ($SPECIALtuip) { print NOTI "\n$SPECIALtuip\n"; }

#if (-e "../builds/$Bbnum/$RFXver.$Bbnum.polybin.iso") {
#   checkspecial();
#   $SPECIAL="\nSPECIAL BUILD PACKAGE AVAILABLE  :  $RFXver.$Bbnum.polybin.iso\n";
#   print "\nmissingspecial length = $#missingspecial\n";
#}

#if (-e "../builds/tuip/$Bbnum/$TUIPver.$Bbnum.polybin.iso") {
#   checkspecialtuip();
#   $SPECIALtuip="\nSPECIAL TUIP BUILD PACKAGE AVAILABLE  :  $TUIPver.$Bbnum.polybin.iso\n";
#   print "\nmissingspecial length = $#missingspecialtuip\n";
#}



