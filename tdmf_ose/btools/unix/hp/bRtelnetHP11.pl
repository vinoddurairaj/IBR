#!/usr/local/bin/perl -w
##############################
############################
#
#  perl -w -I..\.. bRtelnetHP11.pl
#
#	telnet to HP11 build machine 
#	make  :  build it
#	make  :  package
#	ready for ftp 
#
############################
##############################

##############################
############################
# vars
############################
##############################

require "bRglobalvars.pm";

$branchdir="$branchdir";
$workarea="/home/bmachine/dev/$branchdir/tdmf_ose/btools/unix";
$workarea0="/home/bmachine/dev/$branchdir/tdmf_ose/builds/$Bbnum/tdmf_ose/btools/unix/hp";
$workarea1="/home/bmachine/dev/$branchdir/tdmf_ose/builds/";
$userid="bmachine";
$buildnumber="$buildnumber";
$Bbnum="$Bbnum";

##############################
############################
#
# STDERR / STDOUT to HP11.txt
#
############################
##############################

open (STDOUT, ">HP11.txt");
open (STDERR, ">>&STDOUT");

##############################
############################
# grab password
############################
##############################

open(PSWD,"..\\..\\bRbmachine1.txt");
@psswd=<PSWD>;
close(PSWD);

$passwd=$psswd[0];

#print "\n\npasswd : $passwd\n\n";

#telnet test

print "\n\nSTART : bRtelnetHP11.pl\n\n";

#####################################
###################################
#  telnet call to bRgetcode.pl
###################################
#####################################

use Net::Telnet ();
      $t = new Net::Telnet (Timeout => undef, 
      #$t = new Net::Telnet (Timeout => undef, 
      Prompt => '/voyager/');  # works with tcsh
      $t->open("129.212.102.160");
      $t->login($userid, $passwd);
      #@getcode = $t->cmd( "pwd ; ls -la  ");
      #@getcode = $t->cmd( "cd $workarea ; pwd ; ls -la  ");
      @getcode = $t->cmd( "cd $workarea ; nohup perl -I../.. bRgetcode.pl ");
      print "\nbRtelnetHP11.pl  :  getcode array :\n@getcode";

#####################################
###################################
#  telnet call to bRcreateHP11.pl
###################################
#####################################

# setting Timeout to undef   
# can always change back to undef, but, if it hangs, it never comes back...
use Net::Telnet ();
      $t = new Net::Telnet (Timeout => undef, 
      #$t = new Net::Telnet (Timeout => undef, 
      Prompt => '/voyager/');  # works with tcsh
      $t->open("129.212.102.160");
      $t->login($userid, $passwd);
      #@create = $t->cmd( "cd $workarea ; pwd ; ls -la  ");
      @create = $t->cmd( "cd $workarea0 ; nohup perl -I../.. bRcreateHP.pl ; cd .. ; cat makeHP.txt ");
      print "\nbRtelnetHP11.pl  :  create array :\n@create";

#####################################
###################################
#  telnet call rm -rf  :  $Bbnum -1  (buildnumber minus 1) 
###################################
#####################################

subtract1 ();

print "\n\nBminus = $Bminus\n\n";

# setting Timeout to undef   
# can always change back to undef, but, if it hangs, it never comes back...
use Net::Telnet ();
      $t = new Net::Telnet (Timeout => undef, 
      #$t = new Net::Telnet (Timeout => undef, 
      Prompt => '/voyager/');  # works with tcsh
      $t->open("129.212.102.160");
      $t->login($userid, $passwd);
      #@rmrf = $t->cmd( "cd $workarea ; pwd ; ls -la  ");
      @rmrf = $t->cmd( "cd $workarea1 ; rm -rf $Bminus  ");
      print "\nbRtelnetHP11.pl  :  rmrf array :\n@rmrf";

print "\n\nEND :  bRtelnetHP11.pl\n\n";

#####################################
###################################
#  subroutine  :  subtract1  "subtract 1 from buildnumber, but,
#	keep prefixed 0's " 
###################################
#####################################

sub subtract1 {

   $minus1= $buildnumber - 1;

   if ( $minus1 !~ /(\d)(\d)/ ) {
      $minus1 =~ s/$minus1/00000$minus1/;
      print "\n2\n";
   } 

   elsif ( $minus1 !~ /(\d)(\d)(\d)/ ) {
      $minus1 =~ s/$minus1/0000$minus1/;
      print "\n3\n";
   } 

   elsif ( $minus1 !~ /(\d)(\d)(\d)(\d)/ ) {
      $minus1 =~ s/$minus1/000$minus1/;
      print "\n4\n";
   } 
   
   elsif ( $minus1 !~ /(\d)(\d)(\d)(\d)(\d)/ ) {
      $minus1 =~ s/$minus1/00$minus1/;
      print "\n5\n";
   }

   elsif ( $minus1 !~ /(\d)(\d)(\d)(\d)(\d)(\d)/ ) {
      $minus1 =~ s/$minus1/0$minus1/;
      print "\n6\n";
   }
   else {
      print "\n\n unbelieveables minus1 is 6 digits :  $minus1 \n";
   }
   
   print "\n\nminus1 = $minus1\n\n";

   $Bminus="B$minus1";

   print "\n\nBminus = $Bminus\n\n";

}  # subtract1 sub close bracket

###################################
###################################

close (STDERR);
close (STDOUT);

#examples :  how to set timeout to indefinite time
# capture outputs Dump_log and Output_log functions

# get into the $t object created via the telnet module

#$t = new Net::Telnet (Timeout => undef,        
#      Dump_Log => "SOL7dump.txt",
#      Output_Log => "SOL7output.txt",

#      $last= $t->lastline;
#      $line0= $t->lastline($line);
#      $timed_out= $t->timed_out;
#      $eof= $t->eof;
#      $msg = $t->errmsg;	
#      print @create;
#      print "\nmsg = $msg\n";
#      print "\neof = $eof\n";
#      print "\ntimed_out = $timed_out\n";
#      print "\nlast = $last\n";
#      print "\nline0 = $line0\n";

#set in the timeout => field with prompt, etc.
# Errmode => "return",
#      Errmode => "die",
#      Input_Log => "S7in.txt",
#      Output_Log => "out.txt",
#      Option_Log => "S7option.txt",
#      Dump_Log => "S7dump.txt",
   
