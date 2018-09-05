#!/usr/bin/perl -Ipms
#####################################
###################################
#
# perl -w ./bIftppkg.pl
#
#	grab unix agents from packaged intermediate
#	target directories
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
eval 'require bRdevvars';
eval "require bIstrippedbuildnumber";

our $COMver;
our $Bbnum;

our $AIXwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/installp/mklpp/rel/dist/CD-ROM/usr/sys/inst.images";
our $HPwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/CDROMDIR";
our $SOLwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install";
our $LNXwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install";
our $SUSEwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/UNKNOWNpath/pkg.install";

our $aix51;
our $aix52;
our $aix53;
our $aix61;
our $hp11i;
our $hp1123PA;
our $hp1123IPF;
our $hp1131IPF;
our $hp1131PA;
our $RHAS4x32;
our $RHAS4x64;
our $RHAS4xia64;
our $solaris7;
our $solaris8;
our $solaris9;
our $solaris10;
our $suse9xia64;

#####################################
###################################
# STDERR  /  STDOUT
###################################
#####################################

open (STDOUT, ">logs/ftppkgtuip.txt");
open (STDERR, ">&STDOUT");

print "\n\nSTART  :  bIftppkg.pl\n\n";

#####################################
###################################
# get el passwordo
###################################
#####################################

open(PSWD,"bRbmachine1.txt");
@psswd=<PSWD>;
close(PSWD);

$passwd=$psswd[0];

#print "\n$passwd & $passwd0\n";

#####################################
###################################
#  log values 
###################################
#####################################

print "\n\nCOMver  =  $COMver\n";
print "\nBbnum     =  $Bbnum\n";

system qq(pwd);

######################################
####################################
#
# ftp to source of package build machines  
#
####################################
######################################

############################
# solaris :  7
############################

chdir "../builds/tuip/$Bbnum/solaris/7" || die "chdir ../build/tuip/$Bbnum  :  $! ";
system qq(pwd);
 
$ipaddr="$solaris7";

pingit($ipaddr);

if ("$rc" == "0" ) {
   print "\n$rc\n";
   solaris7();
}
else {
   print "\n$ipaddr  :  sub pingit error  :  $rc\n";
}

undef($ipaddr);

############################
# solaris8 
############################

chdir "../8";
system qq(pwd);

$ipaddr="$solaris8";

pingit($ipaddr);

if ("$rc" == "0" ) {
   print "\n$rc\n";
   solaris8();
}
else {
   print "\n$ipaddr  :  sub pingit error  :  $rc\n";
}

undef($ipaddr);

############################
# solaris9 
############################

chdir "../9";
system qq(pwd);

$ipaddr="$solaris9";

pingit($ipaddr);

if ("$rc" == "0" ) {
   print "\n$rc\n";
   solaris9();
}
else {
   print "\n$ipaddr  :  sub pingit error  :  $rc\n";
}

undef($ipaddr);

############################
# solaris10
############################

chdir "../10";
system qq(pwd);

$ipaddr="$solaris10";

pingit($ipaddr);

if ("$rc" == "0" ) {
   print "\n$rc\n";
   solaris10();
}
else {
   print "\n$ipaddr  :  sub pingit error  :  $rc\n";
}

undef($ipaddr);

############################
# hp11i
############################

chdir "../../HPUX/11i";

$ipaddr="$hp11i";

pingit($ipaddr);

if ("$rc" == "0" ) {
   print "\n$rc\n";
   hp11i();
}
else {
   print "\n$ipaddr  :  sub pingit error  :  $rc\n";
}

undef($ipaddr);

############################
# hp1123pa
############################

chdir "../1123pa";

$ipaddr="$hp1123PA";

#pingit($ipaddr);

#if ("$rc" == "0" ) {
#   print "\n$rc\n";
   hp1123pa();
#}
#else {
#   print "\n$ipaddr  :  sub pingit error  :  $rc\n";
#}

undef($ipaddr);

############################
# hp1123ipf
############################

chdir "../1123ipf";

$ipaddr="$hp1123IPF";

#pingit($ipaddr);

#if ("$rc" == "0" ) {
#   print "\n$rc\n";
   hp1123ipf();
#}
#else {
#   print "\n$ipaddr  :  sub pingit error  :  $rc\n";
#}

undef($ipaddr);

############################
# hp1131ipf
############################

chdir "../1131ipf";

$ipaddr="$hp1131IPF";

#pingit($ipaddr);

#if ("$rc" == "0" ) {
#   print "\n$rc\n";
   hp1131ipf();
#}
#else {
#   print "\n$ipaddr  :  sub pingit error  :  $rc\n";
#}

undef($ipaddr);

############################
# hp1131pa
############################

chdir "../1131pa";

$ipaddr="$hp1131PA";

#pingit($ipaddr);

#if ("$rc" == "0" ) {
#   print "\n$rc\n";
   hp1131pa();
#}
#else {
#   print "\n$ipaddr  :  sub pingit error  :  $rc\n";
#}

undef($ipaddr);

############################
#  RedHat AS4x32  :  ftp deliverable from AS 4x 32 build machine.... 
############################

chdir "../../Linux/RedHat/AS4x32";
system qq(pwd);

$ipaddr="$RHAS4x32";

pingit($ipaddr);

if ("$rc" == "0" ) {
   print "\n$rc\n";
   RHAS4x32();
}
else {
   print "\n$ipaddr  :  sub pingit error  :  $rc\n";
}

undef($ipaddr);

############################
#  RedHat AS4x64  :  ftp deliverable from AS 4x 64 build machine.... 
############################

chdir "../AS4x64";
system qq(pwd);

$ipaddr="$RHAS4x64";

pingit($ipaddr);

if ("$rc" == "0" ) {
   print "\n$rc\n";
   RHAS4x64();
}
else {
   print "\n$ipaddr  :  sub pingit error  :  $rc\n";
}

undef($ipaddr);

############################
#  RedHat AS4xia64  :  ftp deliverable from AS 4x ia 64 build machine.... 
############################

chdir "ia64";
system qq(pwd);

$ipaddr="$RHAS4xia64";

pingit($ipaddr);

if ("$rc" == "0" ) {
   print "\n$rc\n";
   RHAS4xia64();
}
else {
   print "\n$ipaddr  :  sub pingit error  :  $rc\n";
}

undef($ipaddr);

############################
#  AIX 5.1  :  deliverable.... 
############################

chdir "../../../../AIX/5.1";
system qq(pwd);

$ipaddr="$aix51";

pingit($ipaddr);

if ("$rc" == "0" ) {
   print "\n$rc\n";
   aix51();
}
else {
   print "\n$ipaddr  :  sub pingit error  :  $rc\n";
}

undef($ipaddr);

############################
#  AIX 5.2  :  deliverable.... 
############################

chdir "../5.2";
system qq(pwd);

$ipaddr="$aix52";

pingit($ipaddr);

if ("$rc" == "0" ) {
   print "\n$rc\n";
   aix52();
}
else {
   print "\n$ipaddr  :  sub pingit error  :  $rc\n";
}

undef($ipaddr);

############################
#  AIX 5.3  :  deliverable.... 
############################

chdir "../5.3";
system qq(pwd);

$ipaddr="$aix53";

pingit($ipaddr);

if ("$rc" == "0" ) {
   print "\n$rc\n";
   aix53();
}
else {
   print "\n$ipaddr  :  sub pingit error  :  $rc\n";
}

undef($ipaddr);

############################
#  AIX 6.1  :  deliverable.... 
############################

chdir "../6.1";
system qq(pwd);

$ipaddr="$aix61";

pingit($ipaddr);

if ("$rc" == "0" ) {
   print "\n$rc\n";
   aix61();
}
else {
   print "\n$ipaddr  :  sub pingit error  :  $rc\n";
}

undef($ipaddr);

############################
#  SUSE 9x 64  deliverable....
############################

chdir "../../Linux/SuSE/9x64";
system qq(pwd);

$ipaddr="$suse9xia64";

#pingit($ipaddr);

if ("$rc" == "0" ) {
   print "\n$rc\n";
   #suse9xia64();
}
else {
   print "\n$ipaddr  :  sub pingit error  :  $rc\n";
}

undef($ipaddr);

###################################################
###################################################
print "\n\nEND  :  bIftppkg.pl\n\n";

###################################################
###################################################

close (STDERR);
close (STDOUT);

###################################################
#################################################
# subroutine(s)
#################################################
###################################################

############################
# solaris :  7 
############################

sub solaris7 {
use Net::FTP;
   $ftp=Net::FTP->new("$solaris7")  or warn "can't connect solaris 7 env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : solaris 7 : error : $@\n";

   $ftp->cwd("$SOLwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("SFTKdtc.$PRODver.pkg") or warn "solaris 7 get error : tdmfsol.pkg script: $!";

   $ftp->quit;
}

############################
# solaris :  8 
############################

sub solaris8 {

use Net::FTP;
   $ftp=Net::FTP->new("$solaris8")  or warn "can't connect solaris 8 env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : solaris 8 : error : $@\n";

   $ftp->cwd("$SOLwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("SFTKdtc.$PRODver.pkg") or warn "solaris 8 get error : tdmfsol.pkg script: $!";

   $ftp->quit;

}

############################
# solaris :  9 
############################

sub solaris9 {
use Net::FTP;
   $ftp=Net::FTP->new("$solaris9")  or warn "can't connect solaris 9 env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : solaris 9 : error : $@\n";

   $ftp->cwd("$SOLwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("SFTKdtc.$PRODver.pkg") or warn "solaris 9 get error : tdmfsol.pkg script: $!";

   $ftp->quit;

}

############################
# solaris :  10 
############################

sub solaris10 {
use Net::FTP;
   $ftp=Net::FTP->new("$solaris10")  or warn "can't connect solaris 10 env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : solaris 10 : error : $@\n";

   $ftp->cwd("$SOLwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("SFTKdtc.$PRODver.pkg") or warn "solaris 10 get error : tdmfsol.pkg script: $!";

   $ftp->quit;

}

############################
# HP :  11i 
############################

sub hp11i{
use Net::FTP;
   $ftp=Net::FTP->new("$hp11i")  or warn "can't connect HP 11i env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : HP 11i : error : $@\n";

   $ftp->cwd("$HPwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("HP-UX-$PRODver-$stripbuildnum.depot") or warn "HP 11i get error : bIftppkg.pl script: $!";

   $ftp->quit;

# rename to proper filename
rename "HP-UX-$PRODver-$stripbuildnum.depot","11i.depot";

}

############################
# HP :  1123pa
############################

sub hp1123pa {
use Net::FTP;
   $ftp=Net::FTP->new("$hp1123PA")  or warn "can't connect HP 1123pa env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : HP 1123pa : error : $@\n";
   #$ftp->login("root","moose2")	or warn "could not login : HP 1123pa : error : $@\n";
   #$HPwa_workaround="/RFX/bmachine/dev/trunk/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/CDROMDIR";

   $ftp->cwd("$HPwa");
   #$ftp->cwd("$HPwa_workaround");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   print "\n\n\nGOHERE  :  HP-UX-$PRODver-$stripbuildnum.depot\n\n\n";
   #sleep 5;
   $ftp->get("HP-UX-$PRODver-$stripbuildnum.depot") or warn "HP 1123pa get error : bIftppkg.pl script: $!";

   $ftp->quit;

   #rename to proper filename
   rename "HP-UX-$PRODver-$stripbuildnum.depot","1123pa.depot";

}

############################
# HP :  1123ipf
############################

sub hp1123ipf {
use Net::FTP;
   $ftp=Net::FTP->new("$hp1123IPF")  or warn "can't connect HP 1123 ipf env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : HP 1123 ipf : error : $@\n";

   $ftp->cwd("$HPwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   print "\n\n\nGOHERE  :  HP-UX-$PRODver-$stripbuildnum.depot\n\n\n";
   #sleep 5;
   $ftp->get("HP-UX-$PRODver-$stripbuildnum.depot") or warn "HP 1123ipf get error : bIftppkg.pl script: $!";

   $ftp->quit;

   #rename to proper filename
   rename "HP-UX-$PRODver-$stripbuildnum.depot","1123ipf.depot";

}

############################
# HP :  1131ipf
############################

sub hp1131ipf {
use Net::FTP;
   $ftp=Net::FTP->new("$hp1131IPF")  or warn "can't connect HP 1131 ipf env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : HP 1131 ipf : error : $@\n";

   $ftp->cwd("$HPwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   print "\n\n\nGOHERE  :  HP-UX-$PRODver-$stripbuildnum.depot\n\n\n";
   #sleep 5;
   $ftp->get("HP-UX-$PRODver-$stripbuildnum.depot") or warn "HP 1131ipf get error : bIftppkg.pl script: $!";

   $ftp->quit;

   #rename to proper filename
   rename "HP-UX-$PRODver-$stripbuildnum.depot","1131ipf.depot";

}

############################
# HP :  1131pa
############################

sub hp1131pa {
use Net::FTP;
   $ftp=Net::FTP->new("$hp1131PA")  or warn "can't connect HP 1131 pa env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : HP 1131 pa : error : $@\n";

   $ftp->cwd("$HPwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   print "\n\n\nGOHERE  :  HP-UX-$PRODver-$stripbuildnum.depot\n\n\n";
   #sleep 5;
   $ftp->get("HP-UX-$PRODver-$stripbuildnum.depot") or warn "HP 1131pa get error : bIftppkg.pl script: $!";

   $ftp->quit;

   #rename to proper filename
   rename "HP-UX-$PRODver-$stripbuildnum.depot","1131pa.depot";

}

############################
#  RedHatAS4x32  :  get it .... 
############################

sub RHAS4x32 {
   
use Net::FTP;
   $ftp=Net::FTP->new("$RHAS4x32")  or warn "can't connect LINUX RH ADV Srv 4 32 env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : LINUX RH ADV Srv 4 32 : error : $@\n";

   $ftp->cwd("$LNXwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("TDMFIP-$PRODver-$stripbuildnum.i386.rpm") or warn "RHAS 4 32 get error : bIftppkg.pl script: $!";

   $ftp->quit;

}

############################
#  RedHatAS4x64  :  get it .... 
############################

sub RHAS4x64 {
   
use Net::FTP;
   $ftp=Net::FTP->new("$RHAS4x64")  or warn "can't connect LINUX RH ADV Srv 4 64 env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : LINUX RH ADV Srv 4 64 : error : $@\n";

   $ftp->cwd("$LNXwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("TDMFIP-$PRODver-$stripbuildnum.x86_64.rpm") or warn "RHAS 4 64 get error : bIftppkg.pl script: $!";

   $ftp->quit;

}

############################
#  RedHatAS4xia64  :  get it .... 
############################

sub RHAS4xia64 {
   
use Net::FTP;
   $ftp=Net::FTP->new("$RHAS4xia64")  or warn "can't connect LINUX RH ADV Srv 4x ia64 env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : LINUX RH ADV Srv 4x ia64 : error : $@\n";

   $ftp->cwd("$LNXwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("TDMFIP-$PRODver-$stripbuildnum.ia64.rpm") or warn "RHAS 4x ia64 get error : bIftppkg.pl script: $!";

   $ftp->quit;

}

############################
# AIX51  :  ftp deliverable from aix build machine.... 
############################

sub aix51 {
   
use Net::FTP;
   $ftp=Net::FTP->new("$aix51")  or warn "can't connect aix51 env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : aix51 : error : $@\n";

   $ftp->cwd("$AIXwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("dtc.rte") or warn "aix51 get error : bIftppkg.pl script: $!";

   $ftp->ascii;
                                                                                
   $ftp->get(".toc") or warn "aix51 get error : bIftppkg.pl script: $!";

   $ftp->quit;

}

############################
# AIX52  :  ftp deliverable from aix build machine.... 
############################

sub aix52 {
   
use Net::FTP;
   $ftp=Net::FTP->new("$aix52")  or warn "can't connect aix52 env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : aix52 : error : $@\n";

   $ftp->cwd("$AIXwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("dtc.rte") or warn "aix52 get error : bIftppkg.pl script: $!";

   $ftp->ascii;
                                                                                
   $ftp->get(".toc") or warn "aix52 get error : bIftppkg.pl script: $!";

   $ftp->quit;

}

############################
# AIX53  :  ftp deliverable from aix build machine.... 
############################

sub aix53 {
   
use Net::FTP;
   $ftp=Net::FTP->new("$aix53")  or warn "can't connect aix53 env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : aix53 : error : $@\n";

   $ftp->cwd("$AIXwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("dtc.rte") or warn "aix53 get error : bIftppkg.pl script: $!";

   $ftp->ascii;
                                                                                
   $ftp->get(".toc") or warn "aix53 get error : bIftppkg.pl script: $!";

   $ftp->quit;

}

############################
# AIX61  :  ftp deliverable from aix build machine.... 
############################

sub aix61 {
   
use Net::FTP;
   $ftp=Net::FTP->new("$aix61")  or warn "can't connect aix61 env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : aix61 : error : $@\n";

   $ftp->cwd("$AIXwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("dtc.rte") or warn "aix61 get error : bIftppkg.pl script: $!";

   $ftp->ascii;
                                                                                
   $ftp->get(".toc") or warn "aix61 get error : bIftppkg.pl script: $!";

   $ftp->quit;

}

############################
# SuSE9xia64  :  ftp deliverable from aix build machine....
############################

sub suse9xia64 {

use Net::FTP;
   $ftp=Net::FTP->new("$suse9xia64")  or warn "can't connect suse9x ia64 env : $@\n";

   $ftp->login("bmachine","$passwd")    or warn "could not login : suse9x ia64 : error : $@\n";

   $ftp->cwd("$SUSEwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("PACKAGENAME") or warn "SUSE9x64 get error : bRftppkg.pl script: $!";

   $ftp->quit;

}

###################################################
#################################################
# pingit()  :  validate machine is alive or exit as it bombs ftp program 
#################################################
###################################################

sub pingit {

   #adding in rsh ls too  ==>> ping can ping a valid
   #running interface  :  machine may not indded be up / running
   #and this will break the perl ftp module

   print "\n\nENTER sub pingit : machine  :  $ipaddr\n\n";

   @pingit=`ping -c 1 $ipaddr`;
   $pingRC="$?";

   #if ("$ipaddr" =~ /206.141/) {
   if ("$ipaddr" =~ /sjbrhlas32m2/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   #elsif ("$ipaddr" =~ /206.147/) {
   elsif ("$ipaddr" =~ /sjbrhlas3m3/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   #elsif ("$ipaddr" =~ /206.68/) {
   elsif ("$ipaddr" =~ /sjbrhlas4m3/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   #elsif ("$ipaddr" =~ /206.43/) {
   elsif ("$ipaddr" =~ /sjbrhlas4m2/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   #elsif ("$ipaddr" =~ /206.43/) {
   elsif ("$ipaddr" =~ /sjbrhlas4m1/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   #elsif ("$ipaddr" =~ /206.142/) {
   elsif ("$ipaddr" =~ /sjbsolv10m1/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   else {
      @rshit = system qq(rsh $ipaddr -l bmachine ls);
      
      $rshRC="$rshit[0]";     #syntax  :  $rshRC="@rshit";  works too.
   			   #using $array[0] syntax for certainty as array should
			   #contain only 1 element  :  the return code, hopefully
   }

   if ( "$pingRC" == "0" && "$rshRC" == "0" ) {
      print "\nping rc = $pingRC  :  rsh rc = $rshRC\n";
      $rc="0";
      print "\n\nEXIT sub pingit : machine  :  $ipaddr\n\n";
      return $rc;
   }
   else {
      print "\nping rc = $pingRC  :  rsh rc = $rshRC\n";
      print "\n\nEXIT sub pingit : machine  :  $ipaddr\n\n";
      $rc="1";
      return $rc;
   }

}  # sub pingit close bracket

