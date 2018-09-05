#!/usr/bin/perl  -Ipms
###############################################
#############################################
#
#  perl -w ./bRcallzLinux.pl
#
#	call ssh scripts
#
###########################################
#############################################

#############################################
###########################################
#  vars  /  pms  /  use
###########################################
#############################################

eval 'require bRglobalvars';

use threads;

eval $COMver;
eval $Bbnum;

#############################################
###########################################
#   STDOUT / STDERR
###########################################
#############################################

open (STDOUT, ">logs/callzlinux.txt");
open (STDERR, ">&STDOUT");

#############################################
###########################################
#  exec statements 
###########################################
#############################################

print "\n\nSTART  :  bRcallzLinux.pl\n\n"; 

#############################################
###########################################
#  log values 
###########################################
#############################################

print "\n\nCOMver  =  $COMver\n\n";
print "\nBbnum     =  $Bbnum\n\n";

chdir "unix/zLinux";
$PWD=qx|pwd|;
print "$PWD";

#############################################
###########################################
#  define thread  control vars / values
###########################################
#############################################

print "\npre nonparallel build message\n\n";

chdir "RedHat4x" || die "chdir death Redhat4x  :  $!\n";
system qq(pwd);
print "\n\n";
system qq(ls bRsshRH4xs390x.pl);
system qq(perl bRsshRH4xs390x.pl);
chdir "../RedHat5x" || die "chdir death Redhat5x  :  $!\n";
system qq(pwd);
print "\n\n";
system qq(ls bRsshRH5xs390x.pl);
system qq(perl bRsshRH5xs390x.pl);
chdir "../SuSE9x" || die "chdir death SuSE9x  :  $!\n";
system qq(pwd);
print "\n\n";
system qq(ls bRsshSuSE9xs390x.pl);
system qq(perl bRsshSuSE9xs390x.pl);
chdir "../SuSE10x" || die "chdir death SuSE10x  :  $!\n";
system qq(pwd);
print "\n\n";
system qq(ls bRsshSuSE10xs390x.pl);
system qq(perl bRsshSuSE10xs390x.pl);

print "\n\npost nonparallel build message\n\n";
exit();

#############################################
###########################################
#  ssh scripts 
#	add thread statements 2009 01 06
###########################################
#############################################

my $threadRH4x =  threads->create(sub { chdir "RedHat4x";
   print "\nRedhat 4x\n";
   system qq(pwd);
   print "\n\n";
   system qq(perl bRsshRH4xs390x.pl);
   system qq(ls bRsshRH4xs390x.pl);
   print "\nRedHat 4x ssh rc  =  $?\n";
   });

my $threadRH5x = threads->create(sub { chdir "../RedHat5x";
   print "\nRedhat 5x\n";
   system qq(pwd);
   print "\n\n";
   system qq(perl bRsshRH5xs390x.pl);
   system qq(ls bRsshRH5xs390x.pl);
   print "\nRedHat 5x ssh rc  =  $?\n";
   });

my $threadSuSE9x = threads->create(sub { chdir "../SuSE9x";
   print "\nSuSE 9x\n";
   system qq(pwd);
   print "\n\n";
   system qq(perl bRsshSuSE9xs390x.pl);
   system qq(ls bRsshSuSE9xs390x.pl);
   print "\nSuSE 9x ssh rc  =  $?\n";
   });

my $threadSuSE10x = threads->create(sub { chdir "../SuSE10x";
   print "\nSuSE 10x\n";
   system qq(pwd);
   print "\n\n";
   system qq(perl bRsshSuSE10xs390x.pl);
   system qq(ls bRsshSuSE10xs390x.pl);
   print "\nSuSE 10x ssh rc  =  $?\n";
   });

#############################################
###########################################
#  wait for threads to complete  :  join
###########################################
#############################################

$threadRH4x->join();
$threadRH5x->join();
$threadSuSE9x->join();
$threadSuSE10x->join();

###########################################
###########################################

print "\n\nEND  :  bRcallzLinux.pl\n\n"; 

###########################################
###########################################

close (STDERR);
close (STDOUT);

