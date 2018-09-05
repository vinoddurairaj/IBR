#!/usr/bin/perl -Ipms -Ivalidation
#######################################
#####################################
#
#  perl -w ./bRtarDRIVER.pl
#
#    tar DELIVERABLES (naked)  
#
#    stage to dmsbuilds.sanjose.ibm.com
#
#    $RFXver.$Bbnum.{OSname}.driver.tar
#    $TUIPver.$Bbnum.{OSname}.driver.tar
#
#####################################
#######################################
 
#######################################
#####################################
# vars  /  pm's 
#####################################
#######################################

eval 'require bRglobalvars';
eval 'require bRbuildmachinelist';

our $COMver;
our $RFXver;
our $TUIPver;
our $Bbnum;
our $branchdir;

our $aix433;
our $aix51;
our $aix52;
our $aix53;
our $aix61;
our $hp11;
our $hp11i;
our $hp1123PA;
our $hp1123IPF;
our $linuxRHAS;
our $RHAS4xia64;
our $RHAS4x32;
our $RHAS4x64;
our $solaris7;
our $solaris8;
our $solaris9;
our $solaris10;
our $suse9xia64;
our $suse9xx86;
our $suse9xx64;
our $zRHAS4xia64;

our $AIXrfx="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/installp/mklpp/rel/root/dtc.rte/usr/lib";
our $AIXtuip="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/installp/mklpp/rel/root/dtc.rte/usr/lib";
our $HPrfx="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install/pkg/usr";
our $HPtuip="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install/pkg/usr";
our $SOLrfx="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install/pkg/usr";
our $SOLtuip="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install/pkg/usr";
our $LNXrfx="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install/pkg/";
our $LNXtuip="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install/pkg/";
our $zLNXrfxRH4x="/localwork/bmachine/dev/RH4x/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install/pkg";
our $zLNXtuipRH4x="/localwork/bmachine/dev/RH4x/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install/pkg";
our $zLNXrfxRH5x="/localwork/bmachine/dev/RH5x/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install/pkg";
our $zLNXtuipRH5x="/localwork/bmachine/dev/RH5x/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install/pkg";

our $eXsol="echo directory exists $SOLrfx";
our $eXsolt="echo directory exists $SOLtuip";
our $eXhp="echo directory exists $HPrfx";
our $eXhpt="echo directory exists $HPtuip";
our $eXaix="echo directory exists $AIXrfx";
our $eXaixt="echo directory exists $AIXtuip";
our $eXlnx="echo directory exists $LNXrfx";
our $eXlnxt="echo directory exists $LNXtuip";
our $eXzlnxRH4x="echo directory exists $zLNXrfxRH4x";
our $eXzlnxtRH4x="echo directory exists $zLNXtuipRH4x";
our $eXzlnxRH5x="echo directory exists $zLNXrfxRH5x";
our $eXzlnxtRH5x="echo directory exists $zLNXtuipRH5x";

our $chkSOL="if [ -d $SOLrfx ]; then $eXsol ; else echo exit ; exit; fi";
our $chkSOLt="if [ -d $SOLtuip ]; then $eXsolt ; else echo exit ; exit; fi";
our $chkHP="if [ -d $HPrfx ]; then $eXhp ; else echo exit ; exit; fi";
our $chkHPt="if [ -d $HPtuip ]; then $eXhpt ; else echo exit ; exit; fi";
our $chkAIX="if [ -d $AIXrfx ]; then $eXaix ; else echo exit ; exit; fi";
our $chkAIXt="if [ -d $AIXtuip ]; then $eXaixt ; else echo exit ; exit; fi";
our $chkLNX="if [ -d $LNXrfx ]; then $eXlnx ; else echo exit ; exit; fi";
our $chkLNXt="if [ -d $LNXtuip ]; then $eXlnxt ; else echo exit ; exit; fi";
our $chkzLNXRH4x="if [ -d $zLNXrfxRH4x ]; then $eXzlnxRH4x ; else echo exit ; exit; fi";
our $chkzLNXtRH4x="if [ -d $zLNXtuipRH4x ]; then $eXzlnxtRH4x ; else echo exit ; exit; fi";
our $chkzLNXRH5x="if [ -d $zLNXrfxRH5x ]; then $eXzlnxRH5x ; else echo exit ; exit; fi";
our $chkzLNXtRH5x="if [ -d $zLNXtuipRH5x ]; then $eXzlnxtRH5x ; else echo exit ; exit; fi";

our $pro=". ./.profile";
#our $RFXbinsExposed="/builds/Replicator/UNIX/$RFXver/$Bbnum/binsExposed";
our $RFXdriversExposed="/Builds/Replicator/UNIX/$RFXver/$Bbnum/driversExposed";
our $TUIPdriversExposed="'/Builds/TDMFUNIX\(IP\)'/$TUIPver/$Bbnum/driversExposed";
our $UID="-u bmachine -p moose2";

#######################################
#####################################
# STDOUT  / STDERR
#####################################
#######################################

open (STDOUT, ">logs/tardriver.txt");
open (STDERR, ">&STDOUT");

#######################################
#####################################
#  exec statements
#####################################
#######################################

print "\n\nSTART  :  bRtarDRIVER.pl\n\n";

#######################################
#####################################
# log values
#####################################
#######################################

print "\nCOMver    =  $COMver\n\n";
print "\nRFXver    =  $RFXver\n\n";
print "\nTUIPver   =  $TUIPver\n\n";
print "\nBbnum     =  $Bbnum\n\n";

system qq(date);

print "\n\n";

print "\nAIXrfx    =\n$AIXrfx\n\n";
print "\nAIXtuip   =\n$AIXtuip\n\n";
print "\nHPrfx     =\n$HPrfx\n\n";
print "\nHPtuip    =\n$HPtuip\n\n";
print "\nSOLrfx    =\n$SOLrfx\n\n";
print "\nSOLtuip   =\n$SOLtuip\n\n";
print "\nLNXrfx    =\n$LNXrfx\n\n";
print "\nLNXtuip   =\n$LNXtuip\n\n";

#######################################
#####################################
#  cd ../builds/$Bbnum  :  RFX 
#####################################
#######################################

chdir "../builds/$Bbnum" || die "chdir ../$Bbnum death  :  $!\n";

mkdir "nakedDrivers";
chdir "nakedDrivers";

system qq(pwd);
print "\n\n";

#######################################
#####################################
#  RFX   :  solaris
#####################################
#######################################

#our $chkSOL="if [ -d $SOLrfx ]; then echo exists; else echo exit ; exit; fi";

#  RFX   :  solaris 10
our $tarSOL10="sudo tar cvf $RFXver.$Bbnum.sol10.driver.tar *";
our $sshSOL10="ssh $solaris10 -l bmachine";
#our $SOL10="$sshSOL10 \"$checkD ; cd dev ; pwd \"";
our $SOL10="$sshSOL10 \"$chkSOL ; cd $SOLrfx ;  pwd ; $tarSOL10; ls -la *.tar\"";

print "\nRFX  :  exec  :  $SOL10\n";
system qq($SOL10);
system qq(sftp bmachine\@$solaris10:$SOLrfx/$RFXver.$Bbnum.sol10.driver.tar);

print "\n";

#  RFX   :  solaris 9
our $tarSOL9="sudo tar cvf $RFXver.$Bbnum.sol9.driver.tar *";
our $sshSOL9="ssh $solaris9 -l bmachine";
our $SOL9="$sshSOL9 \"$chkSOL ; cd $SOLrfx ;  pwd ; $tarSOL9; ls -la *.tar\"";

print "\nRFX  :  exec  :  $SOL9\n";
system qq($SOL9);
system qq(sftp bmachine\@$solaris9:$SOLrfx/$RFXver.$Bbnum.sol9.driver.tar);

print "\n";

#  RFX   :  solaris 8
our $tarSOL8="sudo tar cvf $RFXver.$Bbnum.sol8.driver.tar *";
our $sshSOL8="rsh $solaris8 -l bmachine";
our $SOL8="$sshSOL8 \"$chkSOL ; cd $SOLrfx ;  pwd ; $tarSOL8; ls -la *.tar\"";

print "\nRFX  :  exec  :  $SOL8\n";
system qq($SOL8);
system qq(ncftpget $UID $solaris8 . $SOLrfx/$RFXver.$Bbnum.sol8.driver.tar);

print "\n";

#  RFX   :  solaris 7
our $tarSOL7="sudo tar cvf $RFXver.$Bbnum.sol7.driver.tar *";
our $sshSOL7="rsh $solaris7 -l bmachine";
our $SOL7="$sshSOL7 \"$chkSOL ; cd $SOLrfx ;  pwd ; $tarSOL7; ls -la *.tar\"";

print "\nRFX  :  exec  :  $SOL7\n";
system qq($SOL7);
system qq(ncftpget $UID $solaris7 . $SOLrfx/$RFXver.$Bbnum.sol7.driver.tar);

print "\n";

#######################################
#####################################
#  RFX   :  hpux
#####################################
#######################################

#  RFX   :  hpux 11 
our $tarHP11="sudo tar cvf $RFXver.$Bbnum.hpux11.driver.tar *";
our $sshHP11="rsh $hp11 -l bmachine";
our $HP11="$sshHP11 \"$chkHP ; cd $HPrfx ;  pwd ; $tarHP11; ls -la *.tar\"";

print "\nRFX  :  exec  :  $HP11\n";
system qq($HP11);
system qq(ncftpget $UID $hp11 . $HPrfx/$RFXver.$Bbnum.hpux11.driver.tar);
#system qq(sftp bmachine\@$hp11:$HPrfx/$RFXver.$Bbnum.hpux11.driver.tar);

print "\n";

#  RFX   :  hpux 11i 
our $tarHP11i="sudo tar cvf $RFXver.$Bbnum.hpux11i.driver.tar *";
our $sshHP11i="rsh $hp11i -l bmachine";
our $HP11i="$sshHP11i \"$chkHP ; cd $HPrfx ;  pwd ; $tarHP11i; ls -la *.tar\"";

print "\nRFX  :  exec  :  $HP11i\n";
system qq($HP11i);
system qq(ncftpget $UID $hp11i . $HPrfx/$RFXver.$Bbnum.hpux11i.driver.tar);
#system qq(sftp bmachine\@$hp11i:$HPrfx/$RFXver.$Bbnum.hpux11i.driver.tar);

print "\n";

#  RFX   :  hpux 1123 ia64
our $tarHP1123ia64="sudo tar cvf $RFXver.$Bbnum.hpux1123ia64.driver.tar *";
our $sshHP1123ia64="ssh $hp1123IPF -l bmachine";
our $HP1123ia64="$sshHP1123ia64 \"$pro ; $chkHP ; cd $HPrfx ;  pwd ; $tarHP1123ia64; ls -la *.tar\"";

print "\nRFX  :  exec  :  $HP1123ia64\n";
system qq($HP1123ia64);
system qq(sftp bmachine\@$hp1123IPF:$HPrfx/$RFXver.$Bbnum.hpux1123ia64.driver.tar);

print "\n";

#  RFX   :  hpux 1123 pa
our $tarHP1123pa="sudo tar cvf $RFXver.$Bbnum.hpux1123parisc.driver.tar *";
our $sshHP1123pa="ssh $hp1123PA -l bmachine";
our $HP1123pa="$sshHP1123pa \"$pro ; $chkHP ; cd $HPrfx ;  pwd ; $tarHP1123pa; ls -la *.tar\"";

print "\nRFX  :  exec  :  $HP1123pa\n";
system qq($HP1123pa);
system qq(sftp bmachine\@$hp1123PA:$HPrfx/$RFXver.$Bbnum.hpux1123parisc.driver.tar);

print "\n";

#  RFX   :  hpux 1131 ia64
our $tarHP1131ia64="sudo tar cvf $RFXver.$Bbnum.hpux1131ia64.driver.tar *";
our $sshHP1131ia64="ssh $hp1131IPF -l bmachine";
our $HP1131ia64="$sshHP1131ia64 \"$pro ; $chkHP ; cd $HPrfx ;  pwd ; $tarHP1131ia64; ls -la *.tar\"";

print "\nRFX  :  exec  :  $HP1131ia64\n";
system qq($HP1131ia64);
system qq(sftp bmachine\@$hp1131IPF:$HPrfx/$RFXver.$Bbnum.hpux1131ia64.driver.tar);

print "\n";

#  RFX   :  hpux 1131 pa
our $tarHP1131pa="sudo tar cvf $RFXver.$Bbnum.hpux1131parisc.driver.tar *";
our $sshHP1131pa="ssh $hp1131PA -l bmachine";
our $HP1131pa="$sshHP1131pa \"$pro ; $chkHP ; cd $HPrfx ;  pwd ; $tarHP1131pa; ls -la *.tar\"";

print "\nRFX  :  exec  :  $HP1131pa\n";
system qq($HP1131pa);
system qq(sftp bmachine\@$hp1131PA:$HPrfx/$RFXver.$Bbnum.hpux1131parisc.driver.tar);

print "\n";

#######################################
#####################################
#  RFX   :  aix 
#####################################
#######################################

#  RFX   :  aix 433
our $tarAIX433="sudo tar cvf $RFXver.$Bbnum.aix433.driver.tar *";
our $sshAIX433="rsh $aix433 -l bmachine";
our $AIX433="$sshAIX433 \"$chkAIX ; cd $AIXrfx ;  pwd ; $tarAIX433; ls -la *.tar\"";

print "\nRFX  :  exec  :  $AIX433\n";
system qq($AIX433);
system qq(ncftpget $UID $aix433 . $AIXrfx/$RFXver.$Bbnum.aix433.driver.tar);
#system qq(sftp bmachine\@$aix51:$AIXrfx/$RFXver.$Bbnum.aix433.driver.tar);

print "\n";

#  RFX   :  aix 51
our $tarAIX51="sudo tar cvf $RFXver.$Bbnum.aix51.driver.tar *";
our $sshAIX51="ssh $aix51 -l bmachine";
our $AIX51="$sshAIX51 \"$chkAIX ; cd $AIXrfx ;  pwd ; $tarAIX51; ls -la *.tar\"";

print "\nRFX  :  exec  :  $AIX51\n";
system qq($AIX51);
system qq(sftp bmachine\@$aix51:$AIXrfx/$RFXver.$Bbnum.aix51.driver.tar);

print "\n";

#  RFX   :  aix 52
our $tarAIX52="sudo tar cvf $RFXver.$Bbnum.aix52.driver.tar *";
our $sshAIX52="rsh $aix52 -l bmachine";
our $AIX52="$sshAIX52 \"$chkAIX ; cd $AIXrfx ;  pwd ; $tarAIX52; ls -la *.tar\"";

print "\nRFX  :  exec  :  $AIX52\n";
system qq($AIX52);
#system qq(ncftpget $UID $aix52 . $AIXrfx/$RFXver.$Bbnum.aix52.driver.tar);
system qq(sftp bmachine\@$aix52:$AIXrfx/$RFXver.$Bbnum.aix52.driver.tar);

print "\n";

#  RFX   :  aix 53
our $tarAIX53="sudo tar cvf $RFXver.$Bbnum.aix53.driver.tar *";
our $sshAIX53="rsh $aix53 -l bmachine";
our $AIX53="$sshAIX53 \"$chkAIX ; cd $AIXrfx ;  pwd ; $tarAIX53; ls -la *.tar\"";

print "\nRFX  :  exec  :  $AIX53\n";
system qq($AIX53);
system qq(ncftpget $UID $aix53 . $AIXrfx/$RFXver.$Bbnum.aix53.driver.tar);
#system qq(sftp bmachine\@$aix53:$AIXrfx/$RFXver.$Bbnum.aix53.driver.tar);

print "\n";

#  RFX   :  aix 61
our $tarAIX61="sudo tar cvf $RFXver.$Bbnum.aix61.driver.tar *";
our $sshAIX61="rsh $aix61 -l bmachine";
our $AIX61="$sshAIX61 \"$chkAIX ; cd $AIXrfx ;  pwd ; $tarAIX61; ls -la *.tar\"";

print "\nRFX  :  exec  :  $AIX61\n";
system qq($AIX61);
system qq(ncftpget $UID $aix61 . $AIXrfx/$RFXver.$Bbnum.aix61.driver.tar);
#system qq(sftp bmachine\@$aix61:$AIXrfx/$RFXver.$Bbnum.aix61.driver.tar);

print "\n";

#######################################
#####################################
#  RFX   :  redhat 
#####################################
#######################################

#  RFX   :  RedHat 4x 32
our $tarRH4x32="sudo tar cvf $RFXver.$Bbnum.RedHat4x32.driver.tar etc/*";
our $sshRH4x32="ssh $RHAS4x32 -l bmachine";
our $RH4x32="$sshRH4x32 \"$chkLNX ; cd $LNXrfx ;  pwd ; $tarRH4x32; ls -la *.tar\"";

print "\nRFX  :  exec  :  $RH4x32\n";
system qq($RH4x32);
system qq(sftp bmachine\@$RHAS4x32:$LNXrfx/$RFXver.$Bbnum.RedHat4x32.driver.tar);

print "\n";

#  RFX   :  RedHat 4x 64
our $tarRH4x64="sudo tar cvf $RFXver.$Bbnum.RedHat4x64.driver.tar etc/*";
our $sshRH4x64="ssh $RHAS4x64 -l bmachine";
our $RH4x64="$sshRH4x64 \"$chkLNX ; cd $LNXrfx ;  pwd ; $tarRH4x64 ; ls -la *.tar\"";

print "\nRFX  :  exec  :  $RH4x64\n";
system qq($RH4x64);
system qq(sftp bmachine\@$RHAS4x64:$LNXrfx/$RFXver.$Bbnum.RedHat4x64.driver.tar);

print "\n";

#  RFX   :  RedHat 4x ia64
our $tarRH4xia64="sudo tar cvf $RFXver.$Bbnum.RedHat4xia64.driver.tar etc/*";
our $sshRH4xia64="ssh $RHAS4xia64 -l bmachine";
our $RH4xia64="$sshRH4xia64 \"$chkLNX ; cd $LNXrfx ;  pwd ; $tarRH4xia64 ; ls -la *.tar\"";

print "\nRFX  :  exec  :  $RH4xia64\n";
system qq($RH4xia64);
system qq(sftp bmachine\@$RHAS4xia64:$LNXrfx/$RFXver.$Bbnum.RedHat4xia64.driver.tar);

print "\n";

#  RFX   :  RedHat 5x x86
our $tarRH5xx86="sudo tar cvf $RFXver.$Bbnum.RedHat5xx86.driver.tar etc/*";
our $sshRH5xx86="ssh $RHAS5xx86 -l bmachine";
our $RH5xx86="$sshRH5xx86 \"$chkLNX ; cd $LNXrfx ;  pwd ; $tarRH5xx86; ls -la *.tar\"";

print "\nRFX  :  exec  :  $RH5xx86\n";
system qq($RH5xx86);
system qq(sftp bmachine\@$RHAS5xx86:$LNXrfx/$RFXver.$Bbnum.RedHat5xx86.driver.tar);

print "\n";

#  RFX   :  RedHat 5x 64
our $tarRH5xx64="sudo tar cvf $RFXver.$Bbnum.RedHat5xx64.driver.tar etc/*";
our $sshRH5xx64="ssh $RHAS5xx64 -l bmachine";
our $RH5xx64="$sshRH5xx64 \"$chkLNX ; cd $LNXrfx ;  pwd ; $tarRH5xx64 ; ls -la *.tar\"";

print "\nRFX  :  exec  :  $RH5xx64\n";
system qq($RH5xx64);
system qq(sftp bmachine\@$RHAS5xx64:$LNXrfx/$RFXver.$Bbnum.RedHat5xx64.driver.tar);

print "\n";

#  RFX   :  RedHat 5x ia64
our $tarRH5xia64="sudo tar cvf $RFXver.$Bbnum.RedHat5xia64.driver.tar etc/*";
our $sshRH5xia64="ssh $RHAS5xia64 -l bmachine";
our $RH5xia64="$sshRH5xia64 \"$chkLNX ; cd $LNXrfx ;  pwd ; $tarRH5xia64 ; ls -la *.tar\"";

print "\nRFX  :  exec  :  $RH5xia64\n";
system qq($RH5xia64);
system qq(sftp bmachine\@$RHAS5xia64:$LNXrfx/$RFXver.$Bbnum.RedHat5xia64.driver.tar);

print "\n";

#######################################
#####################################
#  RFX   :  SuSE
#####################################
#######################################

#  RFX   :  SuSE 9x ia64
our $tarSuSE9xia64="sudo tar cvf $RFXver.$Bbnum.SuSE9xia64.driver.tar etc/*";
our $sshSuSE9xia64="ssh $suse9xia64 -l bmachine";
our $SuSE9xia64="$sshSuSE9xia64 \"$chkLNX ; cd $LNXrfx ;  pwd ; $tarSuSE9xia64 ; ls -la *.tar\"";

print "\nRFX  :  exec  :  $SuSE9xia64\n";
system qq($SuSE9xia64);
system qq(sftp bmachine\@$suse9xia64:$LNXrfx/$RFXver.$Bbnum.SuSE9xia64.driver.tar);

print "\n";

#  RFX   :  SuSE 9x i386
our $tarSuSE9xi386="sudo tar cvf $RFXver.$Bbnum.SuSE9xi386.driver.tar etc/*";
our $sshSuSE9xi386="ssh $suse9xx86 -l bmachine";
our $SuSE9xi386="$sshSuSE9xi386 \"$chkLNX ; cd $LNXrfx ;  pwd ; $tarSuSE9xi386 ; ls -la *.tar\"";

print "\nRFX  :  exec  :  $SuSE9xi386\n";
system qq($SuSE9xi386);
system qq(sftp bmachine\@$suse9xx86:$LNXrfx/$RFXver.$Bbnum.SuSE9xi386.driver.tar);

print "\n";

#  RFX   :  SuSE 9x x64
our $tarSuSE9xx64="sudo tar cvf $RFXver.$Bbnum.SuSE9xx64.driver.tar etc/*";
our $sshSuSE9xx64="ssh $suse9xx64 -l bmachine";
our $SuSE9xx64="$sshSuSE9xx64 \"$chkLNX ; cd $LNXrfx ;  pwd ; $tarSuSE9xx64 ; ls -la *.tar\"";

print "\nRFX  :  exec  :  $SuSE9xx64\n";
system qq($SuSE9xx64);
system qq(sftp bmachine\@$suse9xx64:$LNXrfx/$RFXver.$Bbnum.SuSE9xx64.driver.tar);

print "\n";

#  RFX   :  SuSE 10x ia64
our $tarSuSE10xia64="sudo tar cvf $RFXver.$Bbnum.SuSE10xia64.driver.tar etc/*";
our $sshSuSE10xia64="ssh $suse10xia64 -l bmachine";
our $SuSE10xia64="$sshSuSE10xia64 \"$chkLNX ; cd $LNXrfx ;  pwd ; $tarSuSE10xia64 ; ls -la *.tar\"";

print "\nRFX  :  exec  :  $SuSE10xia64\n";
system qq($SuSE10xia64);
system qq(sftp bmachine\@$suse10xia64:$LNXrfx/$RFXver.$Bbnum.SuSE10xia64.driver.tar);

print "\n";

#  RFX   :  SuSE 10x i386
our $tarSuSE10xi386="sudo tar cvf $RFXver.$Bbnum.SuSE10xi386.driver.tar etc/*";
our $sshSuSE10xi386="ssh $suse10xx86 -l bmachine";
our $SuSE10xi386="$sshSuSE10xi386 \"$chkLNX ; cd $LNXrfx ;  pwd ; $tarSuSE10xi386 ; ls -la *.tar\"";

print "\nRFX  :  exec  :  $SuSE10xi386\n";
system qq($SuSE10xi386);
system qq(sftp bmachine\@$suse10xx86:$LNXrfx/$RFXver.$Bbnum.SuSE10xi386.driver.tar);

print "\n";

#  RFX   :  SuSE 10x x64
our $tarSuSE10xx64="sudo tar cvf $RFXver.$Bbnum.SuSE10xx64.driver.tar etc/*";
our $sshSuSE10xx64="ssh $suse10xx64 -l bmachine";
our $SuSE10xx64="$sshSuSE10xx64 \"$chkLNX ; cd $LNXrfx ;  pwd ; $tarSuSE10xx64 ; ls -la *.tar\"";

print "\nRFX  :  exec  :  $SuSE10xx64\n";
system qq($SuSE10xx64);
system qq(sftp bmachine\@$suse10xx64:$LNXrfx/$RFXver.$Bbnum.SuSE10xx64.driver.tar);

print "\n";

#  RFX   :  SuSE 11x ia64
#our $tarSuSE11xia64="sudo tar cvf $RFXver.$Bbnum.SuSE11xia64.driver.tar etc/*";
#our $sshSuSE11xia64="ssh $suse11xia64 -l bmachine";
#our $SuSE11xia64="$sshSuSE11xia64 \"$chkLNX ; cd $LNXrfx ;  pwd ; $tarSuSE11xia64 ; ls -la *.tar\"";

#print "\nRFX  :  exec  :  $SuSE11xia64\n";
#system qq($SuSE11xia64);
#system qq(sftp bmachine\@$suse11xia64:$LNXrfx/$RFXver.$Bbnum.SuSE11xia64.driver.tar);

#print "\n";

#  RFX   :  SuSE 11x i386
our $tarSuSE11xi386="tar cvf $RFXver.$Bbnum.SuSE11xi386.driver.tar etc/*";
our $sshSuSE11xi386="ssh $suse11xx86 -l bmachine";
our $SuSE11xi386="$sshSuSE11xi386 \"$chkLNX ; cd $LNXrfx ;  pwd ; $tarSuSE11xi386 ; ls -la *.tar\"";

print "\nRFX  :  exec  :  $SuSE11xi386\n";
system qq($SuSE11xi386);
system qq(sftp bmachine\@$suse11xx86:$LNXrfx/$RFXver.$Bbnum.SuSE11xi386.driver.tar);

print "\n";

#  RFX   :  SuSE 11x x64
our $tarSuSE11xx64="tar cvf $RFXver.$Bbnum.SuSE11xx64.driver.tar etc/*";
our $sshSuSE11xx64="ssh $suse11xx64 -l bmachine";
our $SuSE11xx64="$sshSuSE11xx64 \"$chkLNX ; cd $LNXrfx ;  pwd ; $tarSuSE11xx64 ; ls -la *.tar\"";

print "\nRFX  :  exec  :  $SuSE11xx64\n";
system qq($SuSE11xx64);
system qq(sftp bmachine\@$suse11xx64:$LNXrfx/$RFXver.$Bbnum.SuSE11xx64.driver.tar);

print "\n";

#######################################
#####################################
#  RFX   :  redhat  :  zLinux 
#####################################
#######################################

#  RFX   :  RedHat 4x s390x  :  zLinux
#our $tarRH4xs390x="sudo tar cvf $RFXver.$Bbnum.RedHat4xs390x.driver.tar etc/*";
#our $sshRH4xs390x="ssh $zRH4xs390x64 -l bmachine";
#our $RH4xs390x="$sshRH4xs390x \"$chkzLNXRH4x ; cd $zLNXrfxRH4x ;  pwd ; $tarRH4xs390x ; ls -la *.tar\"";

#print "\nRFX  :  exec  :  $RH4xs390x\n";
#system qq($RH4xs390x);
#system qq(sftp bmachine\@$zRH4xs390x64:$zLNXrfxRH4x/$RFXver.$Bbnum.RedHat4xs390x.driver.tar);

#print "\n";

#  RFX   :  RedHat 5x s390x  :  zLinux
#our $tarRH5xs390x="sudo tar cvf $RFXver.$Bbnum.RedHat5xs390x.driver.tar etc/*";
#our $sshRH5xs390x="ssh $zRH5xs390x64 -l bmachine";
#our $RH5xs390x="$sshRH5xs390x \"$chkzLNXRH5x ; cd $zLNXrfxRH5x ;  pwd ; $tarRH5xs390x ; ls -la *.tar\"";

#print "\nRFX  :  exec  :  $RH5xs390x\n";
#system qq($RH5xs390x);
#system qq(sftp bmachine\@$zRH5xs390x64:$zLNXrfxRH5x/$RFXver.$Bbnum.RedHat5xs390x.driver.tar);

#print "\n";

#######################################
#####################################
#  ncftpput *.tar . dmsbuilds/builds/Replicator/UNIX/$RFXver/$Bbnum/exposedBins 
#####################################
#######################################

system qq(ncftpput -f ../../../btools/bmachine.txt -m $RFXdriversExposed *.tar);

#######################################
#####################################
#  cd ../tuip/$Bbnum  :  TUIP 
#####################################
#######################################

chdir "../../tuip/$Bbnum" || die "chdir ../tuip/$Bbnum death  :  $!\n";

mkdir "nakedDrivers";
chdir "nakedDrivers";

system qq(pwd);
print "\n\n";

#######################################
#####################################
#  TUIP   :  solaris
#####################################
#######################################

#  TUIP   :  solaris 10
our $tarSOL10t="sudo tar cvf $TUIPver.$Bbnum.sol10.driver.tar *";
our $sshSOL10t="ssh $solaris10 -l bmachine";
our $SOL10t="$sshSOL10t \"$chkSOLt ; cd $SOLtuip ;  pwd ; $tarSOL10t; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $SOL10t\n";
system qq($SOL10t);
system qq(sftp bmachine\@$solaris10:$SOLtuip/$TUIPver.$Bbnum.sol10.driver.tar);

print "\n";

#  TUIP   :  solaris 9
our $tarSOL9t="sudo tar cvf $TUIPver.$Bbnum.sol9.driver.tar *";
our $sshSOL9t="ssh $solaris9 -l bmachine";
our $SOL9t="$sshSOL9t \"$chkSOLt ; cd $SOLtuip ;  pwd ; $tarSOL9t; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $SOL9t\n";
system qq($SOL9t);
system qq(sftp bmachine\@$solaris9:$SOLtuip/$TUIPver.$Bbnum.sol9.driver.tar);

print "\n";

#  TUIP   :  solaris 8
our $tarSOL8t="sudo tar cvf $TUIPver.$Bbnum.sol8.driver.tar *";
our $sshSOL8t="rsh $solaris8 -l bmachine";
our $SOL8t="$sshSOL8t \"$chkSOLt ; cd $SOLtuip ;  pwd ; $tarSOL8t; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $SOL8t\n";
system qq($SOL8t);
system qq(ncftpget $UID $solaris8 . $SOLtuip/$TUIPver.$Bbnum.sol8.driver.tar);

print "\n";

#  TUIP   :  solaris 7
our $tarSOL7t="sudo tar cvf $TUIPver.$Bbnum.sol7.driver.tar *";
our $sshSOL7t="rsh $solaris7 -l bmachine";
our $SOL7t="$sshSOL7t \"$chkSOLt ; cd $SOLtuip ;  pwd ; $tarSOL7t; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $SOL7t\n";
system qq($SOL7t);
system qq(ncftpget $UID $solaris7 . $SOLtuip/$TUIPver.$Bbnum.sol7.driver.tar);

print "\n";

#######################################
#####################################
#  TUIP   :  hpux
#####################################
#######################################

#  TUIP   :  hpux 11 
our $tarHP11t="sudo tar cvf $TUIPver.$Bbnum.hpux11.driver.tar *";
our $sshHP11t="rsh $hp11 -l bmachine";
our $HP11t="$sshHP11t \"$chkHPt ; cd $HPtuip ;  pwd ; $tarHP11t; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $HP11t\n";
system qq($HP11t);
system qq(ncftpget $UID $hp11 . $HPtuip/$TUIPver.$Bbnum.hpux11.driver.tar);
#system qq(sftp bmachine\@$hp11:$HPtuip/$TUIPver.$Bbnum.hpux11.driver.tar);

print "\n";

#  TUIP   :  hpux 11i 
our $tarHP11iT="sudo tar cvf $TUIPver.$Bbnum.hpux11i.driver.tar *";
our $sshHP11iT="rsh $hp11i -l bmachine";
our $HP11iT="$sshHP11iT \"$chkHPt ; cd $HPtuip ;  pwd ; $tarHP11iT; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $HP11iT\n";
system qq($HP11iT);
system qq(ncftpget $UID $hp11i . $HPtuip/$TUIPver.$Bbnum.hpux11i.driver.tar);
#system qq(sftp bmachine\@$hp11i:$HPtuip/$TUIPver.$Bbnum.hpux11i.driver.tar);

print "\n";

#  TUIP   :  hpux 1123 ia64
our $tarHP1123ia64t="sudo tar cvf $TUIPver.$Bbnum.hpux1123ia64.driver.tar *";
our $sshHP1123ia64t="ssh $hp1123IPF -l bmachine";
our $HP1123ia64t="$sshHP1123ia64t \"$pro ; $chkHPt ; cd $HPtuip ;  pwd ; $tarHP1123ia64t; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $HP1123ia64t\n";
system qq($HP1123ia64t);
system qq(sftp bmachine\@$hp1123IPF:$HPtuip/$TUIPver.$Bbnum.hpux1123ia64.driver.tar);

print "\n";

#  TUIP   :  hpux 1123 pa
our $tarHP1123pat="sudo tar cvf $TUIPver.$Bbnum.hpux1123parisc.driver.tar *";
our $sshHP1123pat="ssh $hp1123PA -l bmachine";
our $HP1123pat="$sshHP1123pat \"$pro ; $chkHPt; cd $HPtuip ;  pwd ; $tarHP1123pat; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $HP1123pat\n";
system qq($HP1123pat);
system qq(sftp bmachine\@$hp1123PA:$HPtuip/$TUIPver.$Bbnum.hpux1123parisc.driver.tar);

print "\n";

#  TUIP   :  hpux 1131 ia64
our $tarHP1131ia64t="sudo tar cvf $TUIPver.$Bbnum.hpux1131ia64.driver.tar *";
our $sshHP1131ia64t="ssh $hp1131IPF -l bmachine";
our $HP1131ia64t="$sshHP1131ia64t \"$pro ; $chkHPt ; cd $HPtuip ;  pwd ; $tarHP1131ia64t; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $HP1131ia64t\n";
system qq($HP1131ia64t);
system qq(sftp bmachine\@$hp1131IPF:$HPtuip/$TUIPver.$Bbnum.hpux1131ia64.driver.tar);

print "\n";

#  TUIP   :  hpux 1131 pa
our $tarHP1131pat="sudo tar cvf $TUIPver.$Bbnum.hpux1131parisc.driver.tar *";
our $sshHP1131pat="ssh $hp1131PA -l bmachine";
our $HP1131pat="$sshHP1131pat \"$pro ; $chkHPt ; cd $HPtuip ;  pwd ; $tarHP1131pat; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $HP1131pat\n";
system qq($HP1131pat);
system qq(sftp bmachine\@$hp1131PA:$HPtuip/$TUIPver.$Bbnum.hpux1131parisc.driver.tar);

print "\n";

#######################################
#####################################
#  TUIP   :  aix 
#####################################
#######################################

#  TUIP   :  aix 433
our $tarAIX433t="sudo tar cvf $TUIPver.$Bbnum.aix433.driver.tar *";
our $sshAIX433t="rsh $aix433 -l bmachine";
our $AIX433t="$sshAIX433t \"$chkAIXt ; cd $AIXtuip ;  pwd ; $tarAIX433t; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $AIX433t\n";
system qq($AIX433t);
system qq(ncftpget $UID $aix433 . $AIXtuip/$TUIPver.$Bbnum.aix433.driver.tar);
#system qq(sftp bmachine\@$aix51:$AIXtuip/$TUIPver.$Bbnum.aix433.driver.tar);

print "\n";

#  TUIP   :  aix 51
our $tarAIX51t="sudo tar cvf $TUIPver.$Bbnum.aix51.driver.tar *";
our $sshAIX51t="ssh $aix51 -l bmachine";
our $AIX51t="$sshAIX51t \"$chkAIXt ; cd $AIXtuip ;  pwd ; $tarAIX51t; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $AIX51t\n";
system qq($AIX51t);
system qq(sftp bmachine\@$aix51:$AIXtuip/$TUIPver.$Bbnum.aix51.driver.tar);

print "\n";

#  TUIP   :  aix 52
our $tarAIX52t="sudo tar cvf $TUIPver.$Bbnum.aix52.driver.tar *";
our $sshAIX52t="rsh $aix52 -l bmachine";
our $AIX52t="$sshAIX52t \"$chkAIXt ; cd $AIXtuip ;  pwd ; $tarAIX52t; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $AIX52t\n";
system qq($AIX52t);
#system qq(ncftpget $UID $aix52 . $AIXtuip/$TUIPver.$Bbnum.aix52.driver.tar);
system qq(sftp bmachine\@$aix52:$AIXtuip/$TUIPver.$Bbnum.aix52.driver.tar);

print "\n";

#  TUIP   :  aix 53
our $tarAIX53t="sudo tar cvf $TUIPver.$Bbnum.aix53.driver.tar *";
our $sshAIX53t="rsh $aix53 -l bmachine";
our $AIX53t="$sshAIX53t \"$chkAIXt ; cd $AIXtuip ;  pwd ; $tarAIX53t; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $AIX53t\n";
system qq($AIX53t);
system qq(ncftpget $UID $aix53 . $AIXtuip/$TUIPver.$Bbnum.aix53.driver.tar);
#system qq(sftp bmachine\@$aix53:$AIXtuip/$TUIPver.$Bbnum.aix53.driver.tar);

print "\n";

#  TUIP   :  aix 61
our $tarAIX61t="sudo tar cvf $TUIPver.$Bbnum.aix61.driver.tar *";
our $sshAIX61t="rsh $aix61 -l bmachine";
our $AIX61t="$sshAIX61t \"$chkAIXt ; cd $AIXtuip ;  pwd ; $tarAIX61t; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $AIX61t\n";
system qq($AIX61t);
system qq(ncftpget $UID $aix61 . $AIXtuip/$TUIPver.$Bbnum.aix61.driver.tar);
#system qq(sftp bmachine\@$aix51:$AIXtuip/$TUIPver.$Bbnum.aix61.driver.tar);

print "\n";

#######################################
#####################################
#  TUIP   :  redhat 
#####################################
#######################################

#  TUIP   :  RedHat 4x 32
our $tarRH4x32t="sudo tar cvf $TUIPver.$Bbnum.RedHat4x32.driver.tar etc/*";
our $sshRH4x32t="ssh $RHAS4x32 -l bmachine";
our $RH4x32t="$sshRH4x32t \"$chkLNXt ; cd $LNXtuip ;  pwd ; $tarRH4x32t; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $RH4x32t\n";
system qq($RH4x32t);
system qq(sftp bmachine\@$RHAS4x32:$LNXtuip/$TUIPver.$Bbnum.RedHat4x32.driver.tar);

print "\n";

#  TUIP   :  RedHat 4x 64
our $tarRH4x64t="sudo tar cvf $TUIPver.$Bbnum.RedHat4x64.driver.tar etc/*";
our $sshRH4x64t="ssh $RHAS4x64 -l bmachine";
our $RH4x64t="$sshRH4x64t \"$chkLNXt ; cd $LNXtuip ;  pwd ; $tarRH4x64t ; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $RH4x64t\n";
system qq($RH4x64t);
system qq(sftp bmachine\@$RHAS4x64:$LNXtuip/$TUIPver.$Bbnum.RedHat4x64.driver.tar);

print "\n";

#  TUIP   :  RedHat 4x ia64
our $tarRH4xia64t="sudo tar cvf $TUIPver.$Bbnum.RedHat4xia64.driver.tar etc/*";
our $sshRH4xia64t="ssh $RHAS4xia64 -l bmachine";
our $RH4xia64t="$sshRH4xia64t \"$chkLNXt ; cd $LNXtuip ;  pwd ; $tarRH4xia64t ; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $RH4xia64t\n";
system qq($RH4xia64t);
system qq(sftp bmachine\@$RHAS4xia64:$LNXtuip/$TUIPver.$Bbnum.RedHat4xia64.driver.tar);

print "\n";

#  TUIP   :  RedHat 5x x86
our $tarRH5xx86t="sudo tar cvf $TUIPver.$Bbnum.RedHat5xx86.driver.tar etc/*";
our $sshRH5xx86t="ssh $RHAS5xx86 -l bmachine";
our $RH5xx86t="$sshRH5xx86t \"$chkLNXt ; cd $LNXtuip ;  pwd ; $tarRH5xx86t; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $RH5xx86t\n";
system qq($RH5xx86t);
system qq(sftp bmachine\@$RHAS5xx86:$LNXtuip/$TUIPver.$Bbnum.RedHat5xx86.driver.tar);

print "\n";

#  TUIP   :  RedHat 5x 64
our $tarRH5xx64t="sudo tar cvf $TUIPver.$Bbnum.RedHat5xx64.driver.tar etc/*";
our $sshRH5xx64t="ssh $RHAS5xx64 -l bmachine";
our $RH5xx64t="$sshRH5xx64t \"$chkLNXt ; cd $LNXtuip ;  pwd ; $tarRH5xx64t ; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $RH5xx64t\n";
system qq($RH5xx64t);
system qq(sftp bmachine\@$RHAS5xx64:$LNXtuip/$TUIPver.$Bbnum.RedHat5xx64.driver.tar);

print "\n";

#  TUIP   :  RedHat 5x ia64
our $tarRH5xia64t="sudo tar cvf $TUIPver.$Bbnum.RedHat5xia64.driver.tar etc/*";
our $sshRH5xia64t="ssh $RHAS5xia64 -l bmachine";
our $RH5xia64t="$sshRH5xia64t \"$chkLNXt ; cd $LNXtuip ;  pwd ; $tarRH5xia64t ; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $RH5xia64t\n";
system qq($RH5xia64t);
system qq(sftp bmachine\@$RHAS5xia64:$LNXtuip/$TUIPver.$Bbnum.RedHat5xia64.driver.tar);

print "\n";

#######################################
#####################################
#  TUIP   :  SuSE
#####################################
#######################################

#  TUIP   :  SuSE 9x ia64
our $tarSuSE9xia64t="sudo tar cvf $TUIPver.$Bbnum.SuSE9xia64.driver.tar etc/*";
our $sshSuSE9xia64t="ssh $suse9xia64 -l bmachine";
our $SuSE9xia64t="$sshSuSE9xia64t \"$chkLNX ; cd $LNXtuip ;  pwd ; $tarSuSE9xia64t ; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $SuSE9xia64t\n";
system qq($SuSE9xia64t);
system qq(sftp bmachine\@$suse9xia64:$LNXtuip/$TUIPver.$Bbnum.SuSE9xia64.driver.tar);

print "\n";

#  TUIP   :  SuSE 9x i386
our $tarSuSE9xi386t="sudo tar cvf $TUIPver.$Bbnum.SuSE9xi386.driver.tar etc/*";
our $sshSuSE9xi386t="ssh $suse9xx86 -l bmachine";
our $SuSE9xi386t="$sshSuSE9xi386 \"$chkLNX ; cd $LNXtuip ;  pwd ; $tarSuSE9xi386t ; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $SuSE9xi386t\n";
system qq($SuSE9xi386t);
system qq(sftp bmachine\@$suse9xx86:$LNXtuip/$TUIPver.$Bbnum.SuSE9xi386.driver.tar);

print "\n";

#  TUIP   :  SuSE 9x x64
our $tarSuSE9xx64t="sudo tar cvf $TUIPver.$Bbnum.SuSE9xx64.driver.tar etc/*";
our $sshSuSE9xx64t="ssh $suse9xx64 -l bmachine";
our $SuSE9xx64t="$sshSuSE9xx64t \"$chkLNX ; cd $LNXtuip ;  pwd ; $tarSuSE9xx64t ; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $SuSE9xx64t\n";
system qq($SuSE9xx64t);
system qq(sftp bmachine\@$suse9xx64:$LNXtuip/$TUIPver.$Bbnum.SuSE9xx64.driver.tar);

print "\n";

#  TUIP   :  SuSE 10x ia64
our $tarSuSE10xia64t="sudo tar cvf $TUIPver.$Bbnum.SuSE10xia64.driver.tar etc/*";
our $sshSuSE10xia64t="ssh $suse10xia64 -l bmachine";
our $SuSE10xia64t="$sshSuSE10xia64t \"$chkLNX ; cd $LNXtuip ;  pwd ; $tarSuSE10xia64t ; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $SuSE10xia64t\n";
system qq($SuSE10xia64t);
system qq(sftp bmachine\@$suse10xia64:$LNXtuip/$TUIPver.$Bbnum.SuSE10xia64.driver.tar);

print "\n";

#  TUIP   :  SuSE 10x i386
our $tarSuSE10xi386t="sudo tar cvf $TUIPver.$Bbnum.SuSE10xi386.driver.tar etc/*";
our $sshSuSE10xi386t="ssh $suse10xx86 -l bmachine";
our $SuSE10xi386t="$sshSuSE10xi386 \"$chkLNX ; cd $LNXtuip ;  pwd ; $tarSuSE10xi386t ; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $SuSE10xi386t\n";
system qq($SuSE10xi386t);
system qq(sftp bmachine\@$suse10xx86:$LNXtuip/$TUIPver.$Bbnum.SuSE10xi386.driver.tar);

print "\n";

#  TUIP   :  SuSE 10x x64
our $tarSuSE10xx64t="sudo tar cvf $TUIPver.$Bbnum.SuSE10xx64.driver.tar etc/*";
our $sshSuSE10xx64t="ssh $suse10xx64 -l bmachine";
our $SuSE10xx64t="$sshSuSE10xx64t \"$chkLNX ; cd $LNXtuip ;  pwd ; $tarSuSE10xx64t ; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $SuSE10xx64t\n";
system qq($SuSE10xx64t);
system qq(sftp bmachine\@$suse10xx64:$LNXtuip/$TUIPver.$Bbnum.SuSE10xx64.driver.tar);

print "\n";

#  TUIP   :  SuSE 11x ia64
#our $tarSuSE11xia64t="sudo tar cvf $TUIPver.$Bbnum.SuSE11xia64.driver.tar etc/*";
#our $sshSuSE11xia64t="ssh $suse11xia64 -l bmachine";
#our $SuSE11xia64t="$sshSuSE11xia64t \"$chkLNX ; cd $LNXtuip ;  pwd ; $tarSuSE11xia64t ; ls -la *.tar\"";

#print "\nTUIP  :  exec  :  $SuSE11xia64t\n";
#system qq($SuSE11xia64t);
#system qq(sftp bmachine\@$suse11xia64:$LNXtuip/$TUIPver.$Bbnum.SuSE11xia64.driver.tar);

#print "\n";

#  TUIP   :  SuSE 11x i386
our $tarSuSE11xi386t="tar cvf $TUIPver.$Bbnum.SuSE11xi386.driver.tar etc/*";
our $sshSuSE11xi386t="ssh $suse11xx86 -l bmachine";
our $SuSE11xi386t="$sshSuSE11xi386 \"$chkLNX ; cd $LNXtuip ;  pwd ; $tarSuSE11xi386t ; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $SuSE11xi386t\n";
system qq($SuSE11xi386t);
system qq(sftp bmachine\@$suse11xx86:$LNXtuip/$TUIPver.$Bbnum.SuSE11xi386.driver.tar);

print "\n";

#  TUIP   :  SuSE 11x x64
our $tarSuSE11xx64t="tar cvf $TUIPver.$Bbnum.SuSE11xx64.driver.tar etc/*";
our $sshSuSE11xx64t="ssh $suse11xx64 -l bmachine";
our $SuSE11xx64t="$sshSuSE11xx64t \"$chkLNX ; cd $LNXtuip ;  pwd ; $tarSuSE11xx64t ; ls -la *.tar\"";

print "\nTUIP  :  exec  :  $SuSE11xx64t\n";
system qq($SuSE11xx64t);
system qq(sftp bmachine\@$suse11xx64:$LNXtuip/$TUIPver.$Bbnum.SuSE11xx64.driver.tar);

print "\n";

#######################################
#####################################
#  TUIP   :  redhat  :  zLinux 
#####################################
#######################################

#  TUIP   :  RedHat 4x s390x  :  zLinux
#our $tarRH4xs390xt="sudo tar cvf $TUIPver.$Bbnum.RedHat4xs390x.driver.tar etc/*";
#our $sshRH4xs390xt="ssh $zRH4xs390x64 -l bmachine";
#our $RH4xs390xt="$sshRH4xs390xt \"$chkzLNXtRH4x ; cd $zLNXtuipRH4x ;  pwd ; $tarRH4xs390xt ; ls -la *.tar\"";

#print "\nTUIP  :  exec  :  $RH4xs390xt\n";
#system qq($RH4xs390xt);
#system qq(sftp bmachine\@$zRH4xs390x64:$zLNXtuipRH4x/$TUIPver.$Bbnum.RedHat4xs390x.driver.tar);

#print "\n";

#  TUIP   :  RedHat 5x s390x  :  zLinux
#our $tarRH5xs390xt="sudo tar cvf $TUIPver.$Bbnum.RedHat5xs390x.driver.tar etc/*";
#our $sshRH5xs390xt="ssh $zRH5xs390x64 -l bmachine";
#our $RH5xs390xt="$sshRH5xs390xt \"$chkzLNXtRH5x ; cd $zLNXtuipRH5x ;  pwd ; $tarRH5xs390xt ; ls -la *.tar\"";

#print "\nTUIP  :  exec  :  $RH5xs390xt\n";
#system qq($RH5xs390xt);
#system qq(sftp bmachine\@$zRH5xs390x64:$zLNXtuipRH5x/$TUIPver.$Bbnum.RedHat5xs390x.driver.tar);

#print "\n";

#######################################
#####################################
#  ncftpput *.tar . dmsbuilds/builds/Replicator/UNIX/$TUIPver/$Bbnum/exposedBins 
#####################################
#######################################

system qq(ncftpput -f ../../../../btools/bmachine.txt -m $TUIPdriversExposed *.tar);

#######################################
#######################################

print "\n\nEND  :   bRtarDRIVER.pl\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);
