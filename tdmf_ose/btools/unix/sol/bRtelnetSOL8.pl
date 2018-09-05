#!/usr/local/bin/perl -w
##############################
############################
#
#  perl -w -I..\.. bRtelnetSOL8.pl
#
#	telnet to SOL8 build machine 
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
$workarea="/export/home/bmachine/dev/$branchdir/tdmf_ose/btools/unix";
$workarea0="/export/home/bmachine/dev/$branchdir/tdmf_ose/builds/$Bbnum/tdmf_ose/btools/unix/sol";
$workarea1="/export/home/bmachine/dev/$branchdir/tdmf_ose/builds/";
$userid="bmachine";
$buildnumber="$buildnumber";
$Bbnum="$Bbnum";

##############################
############################
#
# STDERR / STDOUT to SOL8.txt
#
############################
##############################

open (STDOUT, ">SOL8.txt");
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

print "\n\nSTART : bRtelnetSOL8.pl\n\n";

#####################################
###################################
#  telnet call to bRgetcode.pl
###################################
#####################################

use Net::Telnet ();
      $t = new Net::Telnet (Timeout => undef, 
      #$t = new Net::Telnet (Timeout => undef, 
      Prompt => '/tdmfsunb/');  # works with tcsh
      #Prompt => '/[a-z]/');  # works with tcsh
      #Prompt => '/\$ $/');  # works with tcsh
      #Prompt => '/$/');  # works with tcsh
      $t->open("129.212.102.213");
      $t->login($userid, $passwd);
      #@getcode = $t->cmd( "cd $workarea ; pwd ; ls ");
      @getcode = $t->cmd( "cd $workarea ; nohup perl -I../.. bRgetcode.pl ; ");
      print "\nbRtelnetSOL8.pl  :  getcode array :\n@getcode";

#####################################
###################################
#  telnet call to bRcreateSOL8.pl
###################################
#####################################

# setting Timeout to undef   
# can always change back to undef, but, if it hangs, it never comes back...
use Net::Telnet ();
      $t = new Net::Telnet (Timeout => undef, 
      #$t = new Net::Telnet (Timeout => undef, 
      Prompt => '/tdmfsunb/');  # works with tcsh
      $t->open("129.212.102.213");
      $t->login($userid, $passwd);
      #@create = $t->cmd( "cd $workarea ; pwd ; ls ");
      @create = $t->cmd( "cd $workarea0 ; nohup perl -I../.. bRcreateSOL.pl ; cd .. ; cat makeLOG.txt ");
      print "\nbRtelnetSOL8.pl  :  create array :\n@create";

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
      Prompt => '/tdmfsunb/');  # works with tcsh
      $t->open("129.212.102.213");
      $t->login($userid, $passwd);
      #@rmrf = $t->cmd( "cd $workarea ; pwd ; ls ");
      @rmrf = $t->cmd( "cd $workarea1 ; rm -rf $Bminus  ");
      print "\nbRtelnetSOL8.pl  :  rmrf array :\n@rmrf";

print "\n\nEND :  bRtelnetSOL8.pl\n\n";

#####################################
###################################
#  subroutine  :  subtract1  "subtract 1 from buildnumber, but,
#	keep prefixed 0's 
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

   
