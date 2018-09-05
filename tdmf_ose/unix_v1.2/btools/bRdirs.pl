#!/usr/bin/perl -Ipms
#######################################
#####################################
#
#  perl -w ./bRdirs.pl
#
#	directories required to create build && image 
#
#	builds/$Bbnum  :  build workarea
#
#####################################
#######################################
 
#######################################
#####################################
# vars  /  pms   /  use
#####################################
#######################################

eval 'require bRglobalvars';

our $COMver;
our $RFXver;
our $TUIPver;
our $Bbnum;

#######################################
#####################################
# array  :  Dirs
#####################################
#######################################

@Dirs= qw(AIX
   AIX/5.1
   AIX/5.2
   AIX/5.3
   AIX/6.1
   doc
   HPUX
   HPUX/11i
   HPUX/1123pa
   HPUX/1123ipf
   HPUX/1131ipf
   HPUX/1131pa
   Redist
   Redist/RedHat
   Redist/RedHat/RPM
   solaris
   solaris/7
   solaris/8
   solaris/9
   solaris/10
   Linux
   OMP
   OMP/SuSE
   OMP/SuSE/9x
   OMP/SuSE/9x/x86_32
   OMP/SuSE/9x/x86_64
   OMP/SuSE/10x
   OMP/SuSE/10x/x86_32
   OMP/SuSE/10x/x86_64
   OMP/SuSE/11x
   OMP/SuSE/11x/x86_32
   OMP/SuSE/11x/x86_64
   OMP/RedHat
   OMP/RedHat/4x
   OMP/RedHat/4x/x86_32
   OMP/RedHat/4x/x86_64
   OMP/RedHat/5x
   OMP/RedHat/5x/x86_32
   OMP/RedHat/5x/x86_64
   BAD
   BAD/SuSE
   BAD/SuSE/9x
   BAD/SuSE/9x/x86_32
   BAD/SuSE/9x/x86_64
   BAD/SuSE/9x/ia64
   BAD/SuSE/10x
   BAD/SuSE/10x/x86_32
   BAD/SuSE/10x/x86_64
   BAD/SuSE/10x/ia64
   BAD/SuSE/11x
   BAD/SuSE/11x/x86_32
   BAD/SuSE/11x/x86_64
   BAD/SuSE/11x/ia64
   BAD/RedHat
   BAD/RedHat/4x
   BAD/RedHat/4x/x86_32
   BAD/RedHat/4x/x86_64
   BAD/RedHat/4x/ia64
   BAD/RedHat/4x/s390x
   BAD/RedHat/5x
   BAD/RedHat/5x/x86_32
   BAD/RedHat/5x/x86_64
   BAD/RedHat/5x/ia64
   BAD/RedHat/5x/s390x
   Linux/SuSE
   Linux/SuSE/9x
   Linux/SuSE/9x/ia64
   Linux/SuSE/9x/x86_32
   Linux/SuSE/9x/x86_64
   Linux/SuSE/10x
   Linux/SuSE/10x/ia64
   Linux/SuSE/10x/x86_32
   Linux/SuSE/10x/x86_64
   Linux/SuSE/11x
   Linux/SuSE/11x/x86_32
   Linux/SuSE/11x/x86_64
   Linux/RedHat
   Linux/RedHat/4x
   Linux/RedHat/4x/x86_32
   Linux/RedHat/4x/x86_64
   Linux/RedHat/4x/ia64
   Linux/RedHat/5x
   Linux/RedHat/5x/x86_32
   Linux/RedHat/5x/x86_64
   Linux/RedHat/5x/ia64
);

#DEACTIVATED
#   Linux/SuSE/9x/s390x
#   Linux/SuSE/10x/s390x
#   Linux/RedHat/4x/s390x
#   Linux/RedHat/5x/s390x

#######################################
#####################################
# STDOUT  / STDERR
#####################################
#######################################

open (STDOUT, ">logs/dirs.txt");
open (STDERR, ">&STDOUT");

#######################################
#####################################
#  exec statements
#####################################
#######################################

print "\n\nSTART  :  bRdirs.pl\n\n";

#######################################
#####################################
#  log values 
#####################################
#######################################

print "\n\nCOMver     =  $COMver\n";
print "\nRFXver     =  $RFXver\n";
print "\nTUIPver    =  $TUIPver\n";
print "\nBbnum      =  $Bbnum\n\n";

chdir "..";

system qq(pwd);
print "\n\n";

#######################################
#####################################
#  process directory mkdirs
#####################################
#######################################

if (! -d "builds") {
   mkdir "builds";
}
else {
   print "\ndirectory builds exists\n";
} 

chdir "builds";

system qq(pwd);

mkdir "$Bbnum";

chdir "$Bbnum";

system qq(pwd);

#######################################
#####################################
#  mkdir ../builds/tuip/$Bbnum
#####################################
#######################################

print "\n\nRFX mkdirs execution\n\n";

foreach $edir (@Dirs) {
   print "\nDirs : $edir\n";
   mkdir "$edir";
   print "\n$edir  :  mkdir  :  rc = $?\n";
}

#######################################
#####################################
#  mkdir ../builds/tuip/$Bbnum
#####################################
#######################################

print "\n\nTUIP mkdirs execution\n\n";

chdir "..";
system qq(pwd);
if (! -d "tuip") { mkdir "tuip";}
chdir "tuip";
mkdir "$Bbnum";
chdir "$Bbnum";

foreach $edir (@Dirs) {
   print "\nDirs tuip : $edir\n";
   mkdir "$edir";
   print "\n$edir  :  mkdir  :  rc = $?\n";
}

#######################################
#######################################

print "\n\nEND bRdirs.pl\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);
