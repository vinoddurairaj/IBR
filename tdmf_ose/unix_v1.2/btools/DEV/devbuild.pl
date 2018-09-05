#!/usr/bin/perl -I.. -I../pms
###########################################
#########################################
#  perl -w ./devbuild.pl
#
#    provide builds on given plat / os
#    in same workarea
#########################################
###########################################

###########################################
#########################################
# vars  /  pms  /  use 
#########################################
###########################################

eval 'require bRbuildmachinelist';
eval 'require devLogs';

our @devLogs;
our @LOGS;
our @errors;

use Getopt::Long;

our $solaris;
our $aix;
our $zlinux;
our $hp;
our $linux;
our $uid;

###########################################
#########################################
# getopts  cmd line
#########################################
###########################################

GetOptions(
   'solaris=s'      => \$solaris,
   'aix=s'          => \$aix,
   'zlinux=s'       => \$zlinux,
   'linux=s'        => \$linux,
   'hp=s'           => \$hp,
   'workarea=s'     => \$workarea,
   'id=s'           => \$uid,
   'usage'          => \$usage,
) or usage();

###########################################
#########################################
#  check for defined $workarea or usage()
#
#   ( or usage is set); 
#########################################
###########################################

if (! $workarea) {
    print "\nworkarea path not entered\n";
    usage();
}
if (! $uid) {
    print "\nuid not entered\n";
    usage();
}
else {
   print "\nworkarea path  =  $workarea\n";
   print "\nuid            =  $uid\n";
}
if (! $solaris && ! $aix && ! $zlinux && ! $hp && ! $linux ) {
   usage();
}
if ($usage) {
   usage();
}

###########################################
#########################################
#  STDOUT / STDERR  pg 752 
#########################################
###########################################

open SAVEOUT, ">&STDOUT";
open SAVEERR, ">&STDERR";
select STDERR; $| =1;
select STDOUT; $| =1;

###########################################
#########################################
#  clean logs 
#########################################
###########################################

chomp (@devLogs);
foreach $devlog (@devLogs) {
   #print "\ndevlog = $devlog\n";
   if (-e "$devlog") {
      system qq(rm $devlog);
   }
}
if (-e "buildErrors.txt") {
   system qq(rm buildErrors.txt);
}

###########################################
#########################################
#   build required plat / OS per Developer inputs
#########################################
###########################################

our $rmtBldCmd="cd $workarea;  pwd ; sudo ./bldcmd >";

if ($solaris) { 
   if ($solaris =~ m/7/) {
      print "\nbuild solaris 7\n";
      closeSTD();
      open STDOUT, ">sol7.txt";
      open STDERR, ">&STDERR";
      system qq(rsh $solaris7 -l $uid \"cd $workarea;  pwd ; sudo ./bldcmd sol7.txt\");
      system qq(rsh $solaris7 -l $uid \"cd $workarea;  pwd ; sudo grep . sol7.txt");
      closeSTD();
      reopen();
   }
   if ($solaris =~ m/10/) {
      print "\nbuild solaris 10\n";
      closeSTD();
      open STDOUT, ">sol10.txt";
      open STDERR, ">&STDERR";
      system qq(ssh $solaris10 -l $uid \"cd $workarea;  pwd ; sudo ./bldcmd sol10.txt\");
      system qq(ssh $solaris10 -l $uid \"cd $workarea;  pwd ; sudo grep . sol10.txt");
      closeSTD();
      reopen();
   }
}
if ($aix) { 
   if ($aix =~ m/5\.1/) {
   print "\nbuild aix 5.1\n";
      closeSTD();
      open STDOUT, ">aix51.txt";
      open STDERR, ">&STDERR";
      system qq(ssh $aix51 -l $uid \"cd $workarea;  pwd ; sudo ./bldcmd aix51.txt\");
      system qq(ssh $aix51 -l $uid \"cd $workarea;  pwd ; sudo grep . aix51.txt\");
      closeSTD();
      reopen();
   }
   if ($aix =~ m/6\.1/) {
   print "\nbuild aix 6.1\n";
      closeSTD();
      open STDOUT, ">aix61.txt";
      open STDERR, ">&STDERR";
      system qq(rsh $aix61 -l $uid \"cd $workarea;  pwd ; sudo ./bldcmd aix61.txt\");
      system qq(rsh $aix61 -l $uid \"cd $workarea;  pwd ; sudo grep . aix61.txt\");
      closeSTD();
      reopen();
   } 
}
if ($zlinux) { 
   print "\nbuild zlinux\n";
   if ($zlinux =~ m/RH5xs390/) {
   print "\nbuild zlinux RH5x\n";
      closeSTD();
      open STDOUT, ">rh5xs390.txt";
      open STDERR, ">&STDERR";
      system qq(ssh $$zRH5xs390x64 -l $uid \"cd $workarea;  pwd ; sudo ./bldcmd rh5xs390.txt\");
      system qq(ssh $$zRH5xs390x64 -l $uid \"cd $workarea;  pwd ; sudo grep . rh5xs390.txt\");
      closeSTD();
      reopen();
   }
   if ($zlinux =~ m/SuSE9xs390/) {
   print "\nbuild zlinux SuSE9x\n";
      closeSTD();
      open STDOUT, ">suse9x390.txt";
      open STDERR, ">&STDERR";
      system qq(ssh $zsuse10xs390x64 -l $uid \"cd $workarea;  pwd ; sudo ./bldcmd suse9x390.txt\");
      system qq(ssh $zsuse10xs390x64 -l $uid \"cd $workarea;  pwd ; sudo grep . suse9x390.txt\");
      closeSTD();
      reopen();
   }
}
if ($hp) { 
   if ($hp =~ m/11\.11/) {
   print "\nbuild hp 11.11\n";
      closeSTD();
      open STDOUT, ">hp1111.txt";
      open STDERR, ">&STDERR";
      system qq(rsh $hp11i -l $uid \"cd $workarea;  pwd ; sudo ./bldcmd hp1111.txt\");
      system qq(rsh $hp11i -l $uid \"cd $workarea;  pwd ; sudo grep . hp1111.txt\");
      closeSTD();
      reopen();
   }
   if ($hp =~ m/11\.31ia64/) {
   print "\nbuild hp 11.31 ia64\n";
      closeSTD();
      open STDOUT, ">hp1131ia64.txt";
      open STDERR, ">&STDERR";
      system qq(ssh $hp1131IPF -l $uid \"cd $workarea;  pwd ; sudo ./bldcmd hp1131ia64.txt\");
      system qq(ssh $hp1131IPF -l $uid \"cd $workarea;  pwd ; sudo grep . hp1131ia64.txt\");
      closeSTD();
      reopen();
   }
}
if ($linux) { 
   if ($linux =~ m/RH4xx86_32/) {
   print "\nbuild RH4xx86_32\n";
      closeSTD();
      open STDOUT, ">rh4xx86_32.txt";
      open STDERR, ">&STDERR";
      system qq(ssh $RHAS4x32 -l $uid \"cd $workarea;  pwd ; sudo ./bldcmd rh4xx86_32.txt\");
      system qq(ssh $RHAS4x32 -l $uid \"cd $workarea;  pwd ; sudo grep . rh4xx86_32.txt\");
      closeSTD();
      reopen();
   }
   if ($linux =~ m/SuSE10xia64/) {
   print "\nbuild SuSE10x ia64\n";
      closeSTD();
      open STDOUT, ">suse10xia64.txt";
      open STDERR, ">&STDERR";
      system qq(ssh $suse10xia64 -l $uid \"cd $workarea;  pwd ; sudo ./bldcmd suse10xia64.txt\");
      system qq(ssh $suse10xia64 -l $uid \"cd $workarea;  pwd ; sudo grep . suse10xia64.txt\");
      closeSTD();
      reopen();
   }
}

###########################################
#########################################
#  parse logs
#
#    look for compile errors 
#########################################
###########################################

foreach $devlog (@devLogs) {
   #print "\ndevlog = $devlog\n";
   if (-e "$devlog") {
      @LOGS=`cat $devlog`;
   }

  foreach $eLog (@LOGS ) {
      if ( "$eLog" =~ m/gmake/i && "$eLog" =~ m/error/i ) {
         push (@errors,"\n=====================================\n"); 
         push (@errors,"\n$devlog\n\n$eLog"); 
         push (@errors,"\n=====================================\n"); 
      }
      if ( "$eLog" =~ m/Packaging/i && "$eLog" =~ m/not/ && "$eLog" =~ m/successful/ ) {
         push (@errors,"\n=====================================\n"); 
         push (@errors,"\n$devlog\n\n$eLog"); 
         push (@errors,"\n=====================================\n"); 
      }
   }
   undef(@LOGS);
}

if ($#errors > "-1") {
  #print "\nerrors  =  @errors\n";
  open (FHwriteErrors,">buildErrors.txt");
  print FHwriteErrors "\n@errors\n";
  close (FHwriteErrors);
}
if (-e "buildErrors.txt") {
   print "\nbuild errors may have occured\n";
   print "\ncheck log ./buildErrors.txt\n";
}

print "\nEND devbuild.pl\n";
close SAVEERR;
close SAVEOUT;
closeSTD();

###########################################
#########################################
#  subs
#########################################
###########################################

sub usage {
   print "\nusage  : perl devbuilds.pl\n";
   print "-w \"workarea\" -i \"uid\"\n";
   print "[ -s \"7\" or \"10\" or \"7 10\" ] \n";
   print "[ -a \"5.1\" or \"6.1\" or \"5.1 6.1\" ]\n";
   print "[ -z \"RH5xs390\" or \"SuSE9xs390\" ] or\n"; 
   print "[ -z \"RH5xs390 SuSE9xs390\" ]\n"; 
   print "[ -h \"11.11\" or \"11.31ia64\" ]\n"; 
   print "[ -h \"11.11 11.31ia64\" ]\n"; 
   print "[ -l \"RH4xx86_32\" or \"SuSE10xia64\" ] or\n"; 
   print "[ -l linux \"RH4xx86_32 SuSE10xia64\" ]\n"; 
   print "at least 1 of the above must be built...\n";
   print "-u usage\n";
   exit();
}

sub reopen {
   open STDOUT, ">&SAVEOUT";
   open STDERR, ">&SAVEERR";
}
sub closeSTD {
   close STDOUT;
   close STDERR;
}
