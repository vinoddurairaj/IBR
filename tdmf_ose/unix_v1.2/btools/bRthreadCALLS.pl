#!/usr/bin/perl -Ipms
###################################
#################################
#
#  perl -w ./bRthreadCALLS.pl
#
#	thread sys calls to unix/os/b(R)(I)sshORrshOS.pl files
#
#################################
###################################

###################################
#################################
#  vars  /  pms  /  use
#################################
###################################

eval 'require bRglobalvars';
eval 'require bRlogList';

use threads;

our $COMver;
our $RFXver;
our $TUIPver;
our $Bbnum;

our @LOGS;

###################################
#################################
#  STDOUT  /  STDERR
#################################
###################################

open (STDOUT, ">logs/threadCALLS.txt");
open (STDERR, ">&STDOUT");

###################################
#################################
#  exec statements
#################################
###################################

print "\n\nSTART :  bRthreadCALLS.pl\n\n\n";

###################################
#################################
#  log values
#################################
###################################

print "\nCOMver     =  $COMver\n";
print "\nRFXver     =  $RFXver\n";
print "\nTUIPver    =  $TUIPver\n";
print "\nBbnum      =  $Bbnum\n\n";

system qq(date);
print "\n\n";

###################################
###################################
# unlink prevous build logs
###################################
###################################

foreach $eRMLog ( @ LOGS ) {
   unlink "$eRMLog";
}

# check for success of unlinks via ls -lR unix

system qq(ls -lR unix/);

###################################
#################################
#  control thread statements ($thread# = async { block }; 
#
#	with while loops
#
#	threads  :
#		$threadAIX
#		$threadHP
#		$threadSOL
#		$threadREDHAT
#  OBSOLETE	$threadIA64 OBSOLETE
#		$threadSUSE
#  DEACTIVATED  $threadzLinux
#
#################################
###################################

$i="0";
$j="0";
$k="0";
$m="0";
#$n="0";
#$o="0";
$p="0";

my $threadAIX = async { while  ("$i" < 1) { 
	print "\ni = $i\n";
	system qq( perl bRcallAIX.pl );
	#system qq( perl bIcallAIX.pl );
        print "\n\nAIX complete\n\n";
	$i++;
	print "\ni = $i\n";
	} };

my $threadHP = async { while ( "$j" < 1 ) { 
	print "\nj = $j\n";
	system qq( perl bRcallHP.pl );
        print "\n\nHP complete\n\n";
	$j++;
	print "\nj = $j\n";
	} };

my $threadSOL = async { while ( "$k" < 1 ) { 
	print "\nk = $k\n";
	system qq( perl bRcallSOL.pl );
        print "\n\nSOL complete\n\n";
	$k++;
	print "\nk = $k\n";
	} };

my $threadREDHAT = async { while ( "$m" < 1 ) { 
	print "\nm = $m\n";
	system qq( perl bRcallREDHAT.pl );
        print "\n\nREDHAT complete\n\n";
	$m++;
	print "\nm = $m\n";
	} };

#my $threadIA64 = async { while ( "$n" < 1 ) { 
#	print "\nn = $n\n";
#	system qq( perl bRcallIA64.pl );
#        print "\n\nIA64 complete\n\n";
#	$n++;
#	print "\nn = $n\n";
#	} };

#my $threadZ = async { while ( "$o" < 1 ) { 
#	print "\no = $o\n";
#	system qq( perl bRcallzLinux.pl );
#        print "\n\nzLinux complete\n\n";
#	$o++;
#	print "\no = $o\n";
#	} };

my $threadSUSE = async { while ( "$p" < 1 ) { 
	print "\np = $p\n";
	system qq( perl bRcallSUSE.pl );
        print "\n\nSuSE complete\n\n";
	$p++;
	print "\np = $p\n";
	} };

###################################
#################################
#  thread#-> join(); 
#
#	forces the threads to wait for each other before
# 	moving to next statement :  hopefully
#################################
###################################

$threadAIX->join();
$threadHP->join();
$threadSOL->join();
$threadREDHAT->join();
#$threadIA64->join();
#$threadZ->join();
$threadSUSE->join();

###################################
###################################

print "\n\nEND  :  bRthreadCALL.pl\n\n";

###################################
###################################

close (STDERR);
close (STDOUT);
