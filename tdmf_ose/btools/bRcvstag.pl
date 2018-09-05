#!/usr/bin/perl -w
########################################
######################################
#
# 	perl -w ./bRcvstag.pl
#
#	create build tag for tdmf_ose & related projects
#
#	tag nomeclature : $RFUver.YYYYMMDD.B####
#
#		$RFUver represents the source trunk / branch level
#		YYYYMMDD represents the date (current)
#		B####    represents the build level number
#
#
#	to format date, we are calling bDATE.pm module
#	sub get_date() returns date in YYYYMMDD format
#
######################################
########################################

########################################
######################################
# grab globals  : 
#  need the packaged global vars at this point in
#  script to get the appropriate label info
#  at the correct time
######################################
########################################

require "bRglobalvars.pm";
require "bRcvstag.pm";
require "bRcvslasttag.pm";
$CVSTAGKEEP="$CVSTAG";  #set var with last tag  :  clear value : next var
$CVSTAG="\$CVSTAG";
$LASTCVSTAG="$LASTCVSTAG";
$buildnumber="$buildnumber";
$SUBPROJECT="$SUBPROJECT";
$workarea="/BUILDS/usr/bmachine/dev/$branchdir";
$userid="bmachine";
$branchdir="$branchdir";
$branchNAME="$branchNAME";
$RFU="tdmf_ose/unix_v1.2";
$CO="cvs checkout -r $branchNAME $RFU";

########################################
######################################
# clear rc = $? log file  :  attempt to get "clean results"
######################################
########################################

unlink "TAGrc.txt";

########################################
######################################
# grab passwd for telnet
######################################
########################################

open(PSWD,"bRbmachine1.txt");
@psswd=<PSWD>;
close(PSWD);

$passwd=$psswd[0];

#print "\n\npasswd : $passwd\n\n";

########################################
######################################
# STDERR, STDOUT
######################################
########################################

open (STDOUT, ">CVSTAG.txt");
open (STDERR, "\&STDOUT");

########################################
######################################
# label stored as var in bVglobalvars.pm
#	grab, manipulate, recreate file....
# 
#	need to "commit" changed file
######################################
########################################

print "\n\nSTART  :  bRcvstag.pl\n\n";

# load bDATE module and call sub get_date

$datepackage='bDATE';

eval "require $datepackage";

@datevar=get_date();

# ensure format of the new tag

print "\nDATE FORMAT = $datevar[1]$datevar[0]$datevar[2]\n\n";
print "\nFULL TAG = \n\t$RFUver+$datevar[1]$datevar[0]$datevar[2]+$Bbnum\n\n";
print "\nFULL TAG = \n\t$RFUver+$datevar[1]$datevar[0]$datevar[2]+$Bbnum\n\n";

############################
##########################
# keep the last TAG for print via bRparselog.pl
##########################
############################

open (FHX,">bRcvslasttag.pm");

print FHX "\$LASTCVSTAG=\"$CVSTAGKEEP\"\;"; 

close (FHX);

############################
##########################
# capture the TAG
##########################
############################

open (FH1, ">bRcvstag.pm");

print FH1 "$CVSTAG=\"$RFUver+$datevar[1]$datevar[0]$datevar[2]+$Bbnum\"\;";
print "bRcvstag.pm : $CVSTAG=$RFUver+$datevar[1]$datevar[0]$datevar[2]+$Bbnum";

close (FH1);

############################
##########################
# commit tag .pm so that they are in the NEW build directory, oops
##########################
############################

system qq(cvs commit -m "Build $buildnumber commit via bRcvstag.pl :  buildcommitstring" bRcvstag.pm bRcvslasttag.pm);

print "\n\ncvs commit :  bRcvstag.pm & bRcvslasttag.pm :  rc = $?\n\n";

############################
##########################
# telnet to David  :  unix tag process required 
#
#	files with same name except case issue
#		foo.c =~ foo.C
##########################
############################

system qq($cd);

print "\n\n";

############################
##########################
#
#  but first  :  to ensure a clean source tree :  
#
#	cvs checkout -r $branchNAME tdmf_ose/unix_v1.2
#
#	should provide better results than cvs update -d
##########################
############################

system qq($cd);

print "\n\ncvs check out and tree rm  :  rm first, then checkout a fresh tree";

use Net::Telnet ();
     $t = new Net::Telnet (Timeout => 1200, 
      #Errmode => "return",
      Input_Log => "CHECKOUT.txt",
      Prompt => '/david/');  # works with tcsh
      $t->open("129.212.239.77");
      $t->login($userid, $passwd);
      #@checkout = $t->cmd( "cd $workarea ; pwd ; ls -la  ");
      @checkout = $t->cmd( "cd $workarea ; pwd ; rm -rf $RFU ; $CO ; echo cvs checkout tdmf_ose rc = $? ");
      print @checkout;

############################
##########################
#
#  TAG telnet jobs :  
#
##########################
############################

#for testing, use the first entry...

$TAGIT="cvs tag $RFUver+$datevar[1]$datevar[0]$datevar[2]+$Bbnum tdmf_ose/$SUBPROJECT";
# testing :  use below  :  admintools project
#$TAGIT="cvs tag $RFUver+$datevar[1]$datevar[0]$datevar[2]+$Bbnum admintools";  # test with this proj ok....
print "\nTAGIT = $TAGIT\n";


$TAGIT0="cvs tag $RFUver+$datevar[1]$datevar[0]$datevar[2]+$Bbnum tdmf_ose/btools";
# testing :  use below  :  tools project
#$TAGIT0="cvs tag $RFUver+$datevar[1]$datevar[0]$datevar[2]+$Bbnum tools";  # test with this proj ok....
print "\nTAGIT0 = $TAGIT0\n";

chomp ($TAGIT);
chomp ($TAGIT0);

print "\n\n";

system qq($cd);

#########################
#######################
#telnet  :  perform  $TAGIT
#######################
#########################

use Net::Telnet ();
     $t = new Net::Telnet (Timeout => 1200, 
      #Errmode => "return",
      Input_Log => "TAGIT.txt",
      Prompt => '/david/');  # works with tcsh
      $t->open("129.212.239.77");
      $t->login($userid, $passwd);
      #@create = $t->cmd( "cd $workarea ; pwd ; ls -la  ");
      @create = $t->cmd( "cd $workarea ; pwd ; $TAGIT ; echo cvs tag tdmf_ose rc = $? ");
      print @create;

#########################
#######################
#telnet  :  perform  $TAGIT0
#######################
#########################

use Net::Telnet ();
     $t = new Net::Telnet (Timeout => 1200, 
      #Errmode => "return",
      Input_Log => "TAGIT0.txt",
      Prompt => '/david/');  # works with tcsh
      $t->open("129.212.239.77");
      $t->login($userid, $passwd);
      #@create0 = $t->cmd( "cd $workarea ; pwd ; ls -la  ");
      @create0 = $t->cmd( "cd $workarea ; pwd ; $TAGIT0 ; echo cvs tag btools rc = $?");
      print @create0;

# need to close STDERR, STDOUT to blat.exe me the log
# to review status per build, email log with rc.

print "\n\n\nEND script bRcvstag.pl\n\n\n";

close (STDERR);
close (STDOUT);

##################################
################################
# PARSE CVSTAG.txt :  dump "rc = $?" to TAGrc.txt for status email
################################
##################################

open (FHTAG,"<CVSTAG.txt");
@tagrc=<FHTAG>;
close (FHTAG);

chomp (@tagrc);

foreach $et0 (@tagrc) {
   if ("$et0" =~ m/cvs tag tdmf_ose/  || "$et0" =~ m/cvs tag btools/ || "$et0" =~ m/cvs checkout/) {
      print "\n\nIF  :  match $et0 =~ echo cvs tag\n\n";
      push (@grab_rc,"\n$et0\n");
   }
}

open (FHTAGrc,">TAGrc.txt");

print FHTAGrc "\n$TAGIT0\n";
print FHTAGrc "\n@grab_rc\n";

close (FHTAGrc);

system qq(blat.exe TAGrc.txt  -p exchange -subject "$RFUver  :  $Bbnum : cvs tag Log"
	-to jdoll\@softek.fujitsu.com);
