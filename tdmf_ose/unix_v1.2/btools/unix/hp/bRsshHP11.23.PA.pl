#!/usr/bin/perl -I../../pms
#####################################
###################################
#
#  perl -w -I../../pms bRsshHP11.23.PA.pl
#
#    ssh to HP1123PA build machine 
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
our $hp1123PA;

our $btoolsPath;
our $buildTreePath;
our $buildTreePathTuip;
our $buildRootPath;
our $buildRootPathTuip;
our $pro;
our $rfxBLDCMD;
our $tuipBLDCMD;

$SSH="ssh $hp1123PA -l bmachine";

$SSHcvsupdate="$SSH \"$pro ; cd $btoolsPath ; pwd ; cvs update ; echo \"cvs update rc = $?\"\""; 

$SSHrfxgetcode="$SSH \" $pro ; cd $btoolsPath/unix ; pwd ;  perl bRgetcode.pl ; echo \"bRgetcode.pl rc = $?\"\""; 

$SSHtuipgetcode="$SSH \" $pro ; cd $btoolsPath/unix ; pwd ;  perl bIgetcode.pl ; echo \"bRgetcode.pl rc = $?\"\""; 

#$SSHrfxbldcmd="$SSH \"$pro ; cd $buildTreePath ; pwd ; sudo $rfxBLDCMD  ; echo \"HP1123PA  :  bldcmd rc = $?\"\""; 
$SSHrfxbldcmd="$SSH \"$pro ; cd $buildTreePath ; pwd ; $rfxBLDCMD  ; echo \"HP1123PA  :  bldcmd rc = $?\"\""; 

#$SSHtuipbldcmd="$SSH \"$pro ; cd $buildTreePathTuip ; pwd ; sudo $tuipBLDCMD  ; echo \"HP1123PA  :  bldcmd rc = $?\"\""; 
$SSHtuipbldcmd="$SSH \"$pro ; cd $buildTreePathTuip ; pwd ; $tuipBLDCMD  ; echo \"HP1123PA  :  bldcmd rc = $?\"\""; 

#####################################
###################################
#  STDERR / STDOUT
###################################
#####################################

open (STDOUT, ">HP1123PA.txt");
open (STDERR, ">\&STDOUT");

#####################################
###################################
#  exec statements
###################################
#####################################

print "\n\nSTART : bRsshHP11.23.PA.pl\n\n";

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
print "\npro            =   $pro\n";
print "\nhp1123PA      =   $hp1123PA\n";

print "\nbtoolsPath     =   $btoolsPath\n";

print "\nbuildTreePath           =\n\n$buildTreePath\n";
print "\nbuildTreePathtuip       =\n\n$buildTreePathTuip\n";

print "\nbuildRootPath           =\n\n$buildRootPath\n";
print "\nbuildRootPathTuip       =\n\n$buildRootPathTuip\n";

print "\n\nSSH                   =\n\n$SSH\n";
print "\n\nSSHcvsupdate          =\n\n$SSHcvsupdate\n";
print "\n\nSSHrfxgetcode         =\n\n$SSHrfxgetcode\n";
print "\n\nSSHtuipgetcode        =\n\n$SSHtuipgetcode\n";
print "\n\nSSHrfxbldcmd          =\n\n$SSHrfxbldcmd\n";
print "\n\nSSHtuipbldcmd         =\n\n$SSHtuipbldcmd\n\n\n";

print "\n===================================================================\n";
print "===================================================================\n\n";

#####################################
###################################
#  $SSHcvsupdate    :  common
#  $SSHrfxgetcode   :  RFX  
#  $SSHtuipgetcode  :  TUIP 
#  $SSHrfxbldcmd    :  RFX 
#  $SSHtuipbldcmd   :  TUIP 
###################################
#####################################

system qq($SSHcvsupdate);

system qq($SSHrfxgetcode);

system qq($SSHtuipgetcode);

system qq($SSHrfxbldcmd);

system qq($SSHtuipbldcmd);

#####################################
###################################
#  $SSH3  :   remove previous build tree 
###################################
#####################################

subtract1 ();

print "\n\nafter subtract1 ()  :  Bminus = $Bminus\n\n";

if ($Bminus) {

   #our $SSHrfxRm="$SSH \"$pro ; cd $buildRootPath ; pwd ; sudo \\rm -rf $Bminus ;  echo \"rm -rf $Bminus rc = $?\"\"";
   our $SSHrfxRm="$SSH \"$pro ; cd $buildRootPath ; pwd ; \\rm -rf $Bminus ;  echo \"rm -rf $Bminus rc = $?\"\"";

   #our $SSHtuipRm="$SSH \"$pro ; cd $buildRootPathTuip ; pwd ; sudo \\rm -rf $Bminus ;  echo \"rm -rf $Bminus rc = $?\"\"";
   our $SSHtuipRm="$SSH \"$pro ; cd $buildRootPathTuip ; pwd ; \\rm -rf $Bminus ;  echo \"rm -rf $Bminus rc = $?\"\"";
   system qq($SSHrfxRm);
   system qq($SSHtuipRm);
}
else {
   print "\nBminus = $Bminus  <<==  NULL  :  error   :  exiting\n";
   exit();
}

#####################################
#####################################

print "\n\nEND :  bRsshHP11.23.PA.pl\n\n";

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

