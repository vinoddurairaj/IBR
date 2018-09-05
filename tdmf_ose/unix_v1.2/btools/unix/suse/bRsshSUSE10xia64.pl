#!/usr/bin/perl -I../../pms
#####################################
###################################
#
#  perl -w -I../../pms bRsshSUSE10xia64.pl
#
#    ssh to SUSE10x SP4 ia64 bit build machine 
#    make  :  build it
#    make  :  package
#    ready for ftp 
#
###################################
#####################################

#####################################
###################################
# vars  /  pms  /  use
###################################
#####################################

eval 'require bRglobalvars';
eval 'require bRbuildmachinelist';
eval 'require bRstrippedbuildnumber';
eval 'require bRsshVARS';
eval 'require bRlogValues';

our $COMver;
our $RFXver;
our $TUIPver;
our $Bbnum;

our $buildnumber;
our $PATCHver;
our $PRODtype;

our $branchdir;
our $stripbuildnum;
our $suse10xia64;

our $btoolsPath;
our $buildTreePath;
our $buildTreePathTuip;
our $buildRootPath;
our $buildRootPathTuip;
our $source;
our $rfxBLDCMD;
our $tuipBLDCMD;

$RSH="ssh $suse10xia64 -l bmachine";

$RSHcvsupdate="$RSH \"$source ; cd $btoolsPath ; pwd ; cvs update ; echo \"cvs update rc = $?\"\""; 

$RSHrfxgetcode="$RSH \" $source ; cd $btoolsPath/unix ; pwd ;  perl bRgetcode.pl ; echo \"bRgetcode.pl rc = $?\"\""; 

$RSHtuipgetcode="$RSH \" $source ; cd $btoolsPath/unix ; pwd ;  perl bIgetcode.pl ; echo \"bRgetcode.pl rc = $?\"\""; 

#$RSHrfxbldcmd="$RSH \"$source ; cd $buildTreePath ; pwd ; sudo $rfxBLDCMD  ; echo \"SuSE 10x SP4 ia64  :  bldcmd rc = $?\"\""; 
$RSHrfxbldcmd="$RSH \"$source ; cd $buildTreePath ; pwd ; $rfxBLDCMD  ; echo \"SuSE 10x SP4 ia64  :  bldcmd rc = $?\"\""; 

#$RSHtuipbldcmd="$RSH \"$source ; cd $buildTreePathTuip ; pwd ; sudo $tuipBLDCMD  ; echo \"SuSE 10x SP4 ia64  :  bldcmd rc = $?\"\""; 
$RSHtuipbldcmd="$RSH \"$source ; cd $buildTreePathTuip ; pwd ; $tuipBLDCMD  ; echo \"SuSE 10x SP4 ia64  :  bldcmd rc = $?\"\""; 

#####################################
###################################
#  STDERR / STDOUT
###################################
#####################################

open (STDOUT, ">SUSE10xia64.txt");
open (STDERR, ">&STDOUT");

#####################################
###################################
#  exec statements
###################################
#####################################

print "\n\nSTART : bRrshSUSE10xia64.pl\n\n";

print "\n$RFXver / $TUIPver  ::::  $Bbnum\n\n";

print "\nsuse10xia64       =  $suse10xia64\n\n";

#####################################
###################################
#  log values
###################################
#####################################

logvalues();

#####################################
###################################
#  $RSHcvsupdate    :  common
#  $RSHrfxgetcode   :  RFX  
#  $RSHtuipgetcode  :  TUIP 
#  $RSHrfxbldcmd    :  RFX 
#  $RSHtuipbldcmd   :  TUIP 
###################################
#####################################

system qq($RSHcvsupdate);

system qq($RSHrfxgetcode);

system qq($RSHtuipgetcode);

system qq($RSHrfxbldcmd);

system qq($RSHtuipbldcmd);

#####################################
###################################
#  $RSH3  :   remove previous build tree 
###################################
#####################################

subtract1 ();

print "\n\nafter subtract1 ()  :  Bminus = $Bminus\n\n";

if ($Bminus) {

   #our $RSHrfxRm="$RSH \"$source ; cd $buildRootPath ; pwd ; sudo \\rm -rf $Bminus ;  echo \"rm -rf $Bminus rc = $?\"\"";
   our $RSHrfxRm="$RSH \"$source ; cd $buildRootPath ; pwd ; \\rm -rf $Bminus ;  echo \"rm -rf $Bminus rc = $?\"\"";

   #our $RSHtuipRm="$RSH \"$source ; cd $buildRootPathTuip ; pwd ; sudo \\rm -rf $Bminus ;  echo \"rm -rf $Bminus rc = $?\"\"";
   our $RSHtuipRm="$RSH \"$source ; cd $buildRootPathTuip ; pwd ; \\rm -rf $Bminus ;  echo \"rm -rf $Bminus rc = $?\"\"";
   system qq($RSHrfxRm);
   system qq($RSHtuipRm);
}
else {
   print "\nBminus = $Bminus  <<==  NULL  :  error   :  exiting\n";
   exit();
}

#####################################
#####################################

print "\n\nEND :  bRsshSUSE10xia64.pl\n\n";

#####################################
#####################################

close (STDERR);
close (STDOUT);

#####################################
###################################
#  subroutine  :  subtract1  "subtract 1 from buildnumber, but,
#	keep prefixed 0's " 
###################################
#####################################

sub subtract1 {

   $minus1= $buildnumber - 1;

   print "\n\nafter subtract $minus1\n\n";

   if ( $minus1 !~ /(\d)(\d)/ ) {
      $minus1 =~ s/$minus1/0000$minus1/;
      print "\n2\n";
   } 

   elsif ( $minus1 !~ /(\d)(\d)(\d)/ ) {
      $minus1 =~ s/$minus1/000$minus1/;
      print "\n3\n";
   } 

   elsif ( $minus1 !~ /(\d)(\d)(\d)(\d)/ ) {
      $minus1 =~ s/$minus1/00$minus1/;
      print "\n4\n";
   } 

   elsif ( $minus1 !~ /(\d)(\d)(\d)(\d)(\d)/ ) {
      $minus1 =~ s/$minus1/0$minus1/;
      print "\n4\n";
   } 
      
   print "\n\nminus1 = $minus1\n\n";

   $Bminus="B$minus1";

   print "\n\nBminus = $Bminus\n\n";

}  # subtract1 sub close bracket

