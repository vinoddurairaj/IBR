#!/usr/bin/perl -I../../pms
#####################################
###################################
#
#  perl -w -I../../pms bRrshSOL10.pl
#
#    rsh to SOL10 build machine 
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

our $COMver;
our $RFXver;
our $TUIPver;
our $Bbnum;

our $buildnumber;
our $PATCHver;
our $PRODtype;

our $branchdir;
our $stripbuildnum;
our $solaris10;

our $btoolsPath;
our $buildTreePath;
our $buildTreePathTuip;
our $buildRootPath;
our $buildRootPathTuip;
our $source;
our $rfxBLDCMD;
our $tuipBLDCMD;

$RSH="ssh $solaris10 -l bmachine";

$RSHcvsupdate="$RSH \"$source ; cd $btoolsPath ; pwd ; cvs update ; echo \"cvs update rc = $?\"\""; 

$RSHrfxgetcode="$RSH \" $source ; cd $btoolsPath/unix ; pwd ;  perl bRgetcode.pl ; echo \"bRgetcode.pl rc = $?\"\""; 

$RSHtuipgetcode="$RSH \" $source ; cd $btoolsPath/unix ; pwd ;  perl bIgetcode.pl ; echo \"bRgetcode.pl rc = $?\"\""; 

#$RSHrfxbldcmd="$RSH \"$source ; cd $buildTreePath ; pwd ; sudo $rfxBLDCMD  ; echo \"SOL10  :  bldcmd rc = $?\"\""; 
$RSHrfxbldcmd="$RSH \"$source ; cd $buildTreePath ; pwd ; $rfxBLDCMD  ; echo \"SOL10  :  bldcmd rc = $?\"\""; 

#$RSHtuipbldcmd="$RSH \"$source ; cd $buildTreePathTuip ; pwd ; sudo $tuipBLDCMD  ; echo \"SOL10  :  bldcmd rc = $?\"\""; 
$RSHtuipbldcmd="$RSH \"$source ; cd $buildTreePathTuip ; pwd ; $tuipBLDCMD  ; echo \"SOL10  :  bldcmd rc = $?\"\""; 

#####################################
###################################
#  STDERR / STDOUT
###################################
#####################################

open (STDOUT, ">SOL10.txt");
open (STDERR, ">\&STDOUT");

#####################################
###################################
#  exec statements
###################################
#####################################

print "\n\nSTART : bRrshSOL10.pl\n\n";

print "\n$RFXver / $TUIPver  ::::  $Bbnum\n\n";

#####################################
###################################
#  log values
###################################
#####################################

print "\n===================================================================\n";
print "\n$Bbnum variable values log  :\n";
print "\n===================================================================\n";

print "\nrfxBLDCMD      =   $rfxBLDCMD\n";;
print "\ntuipBLDCMD     =   $tuipBLDCMD\n";;

print "\nCOMver         =   $COMver\n";
print "\nRFXver         =   $RFXver\n";
print "\nTUIPver        =   $TUIPver\n";
print "\nBbnum          =   $Bbnum\n";

print "\nbuildnumber    =   $buildnumber\n";
print "\nstripbuildnum  =   $stripbuildnum\n";
print "\nPATCHver       =   $PATCHver\n";
print "\nPRODtype       =   $PRODtype\n";

print "\nbranchdir      =   $branchdir\n";
print "\nsource         =   $source\n";
print "\nsolaris10      =   $solaris10\n";

print "\nbtoolsPath     =   $btoolsPath\n";

print "\nbuildTreePath           =\n\n$buildTreePath\n";
print "\nbuildTreePathtuip       =\n\n$buildTreePathTuip\n";

print "\nbuildRootPath           =\n\n$buildRootPath\n";
print "\nbuildRootPathTuip       =\n\n$buildRootPathTuip\n";

print "\n\nRSH                   =\n\n$RSH\n";
print "\n\nRSHcvsupdate          =\n\n$RSHcvsupdate\n";
print "\n\nRSHrfxgetcode         =\n\n$RSHrfxgetcode\n";
print "\n\nRSHtuipgetcode        =\n\n$RSHtuipgetcode\n";
print "\n\nRSHrfxbldcmd          =\n\n$RSHrfxbldcmd\n";
print "\n\nRSHtuipbldcmd         =\n\n$RSHtuipbldcmd\n\n\n";

print "\n===================================================================\n";
print "===================================================================\n\n";

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

print "\n\nEND :  bRrshSOL10.pl\n\n";

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

