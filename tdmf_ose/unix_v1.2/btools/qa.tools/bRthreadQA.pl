#!/usr/bin/perl -I../pms
###################################
#################################
#
#  perl -w ./bRthreadQA.pl
#
#   detach  :  no join  /  wait
#
#   perl58 required for thread module
#
#################################
###################################

###################################
#################################
#  vars  /  pms  /  use
#################################
###################################

eval 'require bRglobalvars';

use threads;

###################################
#################################
#  STDOUT  /  STDERR
#################################
###################################

open (STDOUT, ">../logs/threadQA.txt");
open (STDERR, ">\&STDOUT");

###################################
#################################
#  exec statements
#################################
###################################

print "\n\nSTART  :  bRthreadQA.pl\n\n\n";

###################################
#################################
#  thread statements ($thread# = async { block };
#
#   threads  :
#      $threadQA
#
#################################
###################################

$q="0";
$r="0";
$t="0";

my $threadR = async { while ( "$q" < 1 ) {
   print "\nq = $q\n";
   system qq(perl bRftpQA.pl);
      $q++;
      print "\nq  =  $q\n";
   } };

my $threadT = async { while ( "$q" < 1 ) {
   print "\nt = $t\n";
   system qq(perl bIftpQA.pl);
      $t++;
      print "\nt  =  $t\n";
   } };

my $threadTA = async { while ( "$r" < 1 ) {
   print "\nr = $r\n";
   system qq(perl -c bRftpQA.pl);
      $r++;
      print "\nr  =  $r\n";
   } };

###################################
#################################
#  thread#-> join();
#
#   forces the threads to wait for each other before
#   moving to next statement
#
#  thread#-> detach();
#   will not join or wait  ==>>
#   detach script "should continue to execute"
#################################
###################################

$threadR->detach();
$threadT->detach();
$threadTA->join();

###################################
###################################

print "\n\nEND  :  bRthreadQA.pl\n\n";

###################################
###################################

close (STDERR);
close (STDOUT);

