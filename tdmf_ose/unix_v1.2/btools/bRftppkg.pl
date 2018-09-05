#!/usr/bin/perl -Ipms
#####################################
###################################
#
# perl -w -I".." ./bRftppkg.pl
#
#	grab RFX  && TUIP packages from intermediate
#	target directories
#
#	merged with old bIftppkg.pl
#	grab unix agents from packaged intermediate dirs too
#
###################################
#####################################

#####################################
###################################
#  vars  /  pms  /  use
###################################
#####################################

eval 'require bRglobalvars';
eval 'require bRbuildmachinelist';
eval 'require bRdevvars';
eval 'require bRstrippedbuildnumber';

our $COMver;
our $Bbnum;
our $branchdir;

our $aix51;
our $aix52;
our $aix53;
our $aix61;
our $hp11i;
our $hp1123PA;
our $hp1123IPF;
our $hp1131IPF;
our $hp1131PA;
our $RHAS64;
our $RHAS4x64;
our $RHAS4xia64;
our $solaris7;
our $solaris8;
our $solaris9;
our $solaris10;
our $suse9xia64;
our $suse9xx64;
our $suse9xx86;
our $suse10xia64;
our $suse10xx64;
our $suse10xx86;
our $RHAS5xia64;
our $RHAS5xx64;
our $RHAS5xx86;

our $scriptInitialDirectory;

# generic ftp locations and product name
our $AIXwa;
our $HPwa;
our $SOLwa;
our $LNXwa;
our $SUSEwa;
our $RedHatProduct;	# because the RedHat products are named "Replicator-" and "TDMFIP-" (as opposed to the others, which are the same)

# bRftppkg.pl - RFX ftp locations
our $rfxAIXwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/installp/mklpp/rel/dist/CD-ROM/usr/sys/inst.images";
our $rfxHPwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/CDROMDIR";
our $rfxSOLwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install";
our $rfxLNXwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS";
our $rfxSUSEwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS";
our $rfxInitialDirectory = "../builds/$Bbnum";
our $rfxRedHatProduct = "Replicator";

#bIftppkg.pl - TUIP ftp locations
our $tuipAIXwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/installp/mklpp/rel/dist/CD-ROM/usr/sys/inst.images";
our $tuipHPwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/CDROMDIR";
our $tuipSOLwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install";
our $tuipLNXwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS";
our $tuipSUSEwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS";
our $tuipInitialDirectory = "../builds/tuip/$Bbnum";
our $tuipRedHatProduct = "TDMFIP";

#predeclare sub  :  add $ to real call to doUploads too
sub doUploads ($);

#####################################
###################################
# STDOUT  /  STDERR
###################################
#####################################

#open (STDOUT, ">logs/ftppkg.txt");
open (STDOUT, ">logs/ftppkg.txt");
open (STDERR, ">&STDOUT");

print "\n\nSTART  :  bRftppkg.pl\n\n";

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

print "\n\nCOMver  = $COMver\n";
print "\nBbnum     = $Bbnum\n\n";

system qq(pwd);

use Cwd; # get working directory at start of script
$scriptInitialDirectory = cwd();

######################################
####################################
#
# ftp to source of package build machines  
#
####################################
######################################

# do uploads for RFX
$AIXwa = $rfxAIXwa; 
$HPwa = $rfxHPwa;
$SOLwa = $rfxSOLwa;
$LNXwa = $rfxLNXwa;
$SUSEwa = $rfxSUSEwa;
$RedHatProduct = $rfxRedHatProduct;

doUploads($rfxInitialDirectory);

# reset working directory as if script were restarted
chdir "$scriptInitialDirectory" || die "chdir : died : here";
system qq(pwd);

# do uploads for TUIP
$AIXwa = $tuipAIXwa; 
$HPwa = $tuipHPwa;
$SOLwa = $tuipSOLwa;
$LNXwa = $tuipLNXwa;
$SUSEwa = $tuipSUSEwa;
$RedHatProduct = $tuipRedHatProduct;

doUploads($tuipInitialDirectory);

###################################################
###################################################

print "\n\nEND  :  bRftppkg.pl\n\n";

###################################################
###################################################

close (STDERR);
close (STDOUT);

################################################################
#############################################################
# doUploads()
# 	update the directories that need have the deliverables
# 	before calling the actual ftp actions (subroutines such as 
# 	solaris7(), aix52(), etc);  This way, only one set of 
# 	functions are used.
#############################################################
################################################################

sub doUploads($)
{
     my $directoryPrefix = $_[0];
     print "\nENTER: subroutine began: doUploads($directoryPrefix)";
     ############################
     # solaris :  7
     ############################

     chdir "$directoryPrefix/solaris/7" || die "chdir  :  died  :  here";
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

     chdir "../../Linux/RedHat/4x/x86_32";
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

     chdir "../x86_64";
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
     #  RedHat AS4x64 ia64  :  ftp deliverable from AS 4x 64 build machine.... 
     ############################

     chdir "../ia64";
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
     #  RedHat AS5 x64
     ############################

     chdir "../../5x/x86_64";
     system qq(pwd);

     $ipaddr="$RHAS5xx64";

     pingit($ipaddr);

     if ("$rc" == "0" ) {
	print "\n$rc\n";
	RHAS5xx64();
     }
     else {
	print "\n$ipaddr  :  sub pingit error  :  $rc\n";
     }

     undef($ipaddr);

     ############################
     #  RedHat AS5x x86  (i686)
     ############################

     chdir "../x86_32";
     system qq(pwd);

     $ipaddr="$RHAS5xx86";

     pingit($ipaddr);

     if ("$rc" == "0" ) {
	print "\n$rc\n";
	RHAS5xx86();
     }
     else {
	print "\n$ipaddr  :  sub pingit error  :  $rc\n";
     }

     undef($ipaddr);

     ############################
     #  RedHat AS5x ia64
     ############################

     chdir "../ia64";
     system qq(pwd);

     $ipaddr="$RHAS5xia64";

     pingit($ipaddr);

     if ("$rc" == "0" ) {
	print "\n$rc\n";
	RHAS5xia64();
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
     #  SUSE 9x ia64  deliverable....
     ############################

     chdir "../../Linux/SuSE/9x/ia64";
     system qq(pwd);

     $ipaddr="$suse9xia64";

     #pingit($ipaddr);

     if ("$rc" == "0" ) {
	print "\n$rc\n";
	suse9xia64();
     }
     else {
	print "\n$ipaddr  :  sub pingit error  :  $rc\n";
     }

     undef($ipaddr);

     ############################
     #  SUSE 9x x64  deliverable....
     ############################

     chdir "../x86_64";
     system qq(pwd);

     $ipaddr="$suse9xx64";

     #pingit($ipaddr);

     if ("$rc" == "0" ) {
	print "\n$rc\n";
	suse9xx64();
     }
     else {
	print "\n$ipaddr  :  sub pingit error  :  $rc\n";
     }

     undef($ipaddr);

     ############################
     #  SUSE 9x x86  deliverable....
     ############################

     chdir "../x86_32";
     system qq(pwd);

     $ipaddr="$suse9xx86";

     #pingit($ipaddr);

     if ("$rc" == "0" ) {
	print "\n$rc\n";
	suse9xx86();
     }
     else {
	print "\n$ipaddr  :  sub pingit error  :  $rc\n";
     }

     undef($ipaddr);

     ############################
     #  SUSE 10x x64  deliverable....
     ############################

     chdir "../../10x/x86_64";
     system qq(pwd);

     $ipaddr="$suse10xx64";

     #pingit($ipaddr);

     if ("$rc" == "0" ) {
	print "\n$rc\n";
	suse10xx64();
     }
     else {
	print "\n$ipaddr  :  sub pingit error  :  $rc\n";
     }

     undef($ipaddr);

     ############################
     #  SUSE 10x ia64  deliverable....
     ############################

     chdir "../ia64";
     system qq(pwd);

     $ipaddr="$suse10xia64";

     pingit($ipaddr);

     if ("$rc" == "0" ) {
	print "\n$rc\n";
	suse10xia64();
     }
     else {
	print "\n$ipaddr  :  sub pingit error  :  $rc\n";
     }

     undef($ipaddr);


     ############################
     #  SUSE 10x x86  deliverable....
     ############################

     chdir "../x86_32";
     system qq(pwd);

     $ipaddr="$suse10xx86";

     #pingit($ipaddr);

     if ("$rc" == "0" ) {
	print "\n$rc\n";
	suse10xx86();
     }
     else {
	print "\n$ipaddr  :  sub pingit error  :  $rc\n";
     }

     undef($ipaddr);

     ############################
     #  SUSE 11x x64  deliverable....
     ############################

     chdir "../../11x/x86_64";
     system qq(pwd);

     $ipaddr="$suse11xx64";

     #pingit($ipaddr);

     if ("$rc" == "0" ) {
	print "\n$rc\n";
	suse11xx64();
     }
     else {
	print "\n$ipaddr  :  sub pingit error  :  $rc\n";
     }

     undef($ipaddr);

     ############################
     #  SUSE 11x ia64  deliverable....
     ############################

     #chdir "../ia64";
     #system qq(pwd);

     #$ipaddr="$suse11xia64";

     #pingit($ipaddr);

     #if ("$rc" == "0" ) {
	#print "\n$rc\n";
	#suse11xia64();
     #}
     #else {
	#print "\n$ipaddr  :  sub pingit error  :  $rc\n";
     #}

     #undef($ipaddr);

     ############################
     #  SUSE 11x x86  deliverable....
     ############################

     chdir "../x86_32";
     system qq(pwd);

     $ipaddr="$suse11xx86";

     #pingit($ipaddr);

     if ("$rc" == "0" ) {
	print "\n$rc\n";
	suse11xx86();
     }
     else {
	print "\n$ipaddr  :  sub pingit error  :  $rc\n";
     }

     undef($ipaddr);

     print "\nEND: subroutine completed: doUpload($directoryPrefix)\n\n";
}





###################################################
#################################################




###################################################
#################################################
# subroutine(s)
#################################################
###################################################

############################
# solaris 7 
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
# solaris 8 
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
# solaris 9 
############################

sub solaris9 {

print "\n\nsolaris9  :  sftp sys call \n";
print "\n\n$solaris9\n";
print "\n\n\$SOLwa     =   $SOLwa \n";
print "\n\nSFTKdtc.\$PRODver.pkg     =   SFTKdtc.$PRODver.pkg\n\n";

system qq(sftp bmachine\@$solaris9:$SOLwa/SFTKdtc.$PRODver.pkg);

}

############################
# solaris 10 
############################

sub solaris10 {

print "\n\nsolaris10  :  sftp sys call \n";
print "\n\n$solaris10\n";
print "\n\n\$SOLwa     =   $SOLwa \n";
print "\n\nSFTKdtc.\$PRODver.pkg     =   SFTKdtc.$PRODver.pkg\n\n";

system qq(sftp bmachine\@$solaris10:$SOLwa/SFTKdtc.$PRODver.pkg);

}

############################
# HP11i 
############################

sub hp11i{
use Net::FTP;
   $ftp=Net::FTP->new("$hp11i")  or warn "can't connect HP 11i env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : HP 11i : error : $@\n";

   $ftp->cwd("$HPwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("HP-UX-$PRODver-$stripbuildnum.depot") or warn "HP 11i get error : bRftppkg.pl script: $!";

   $ftp->quit;

# rename to proper filename
rename "HP-UX-$PRODver-$stripbuildnum.depot","11i.depot";

}

############################
# HP1123pa
############################

sub hp1123pa {

print "\n\nhp1123PA  :  sftp sys call \n";
print "\n\n$hp1123PA\n";
print "\n\n\$HPwa     =   $HPwa \n\n";
print "\n\nHP-U-\$PRODver-$stripbuildnum.depot  \t=\nHP-UX-$PRODver-$stripbuildnum.depot\n\n";

system qq(sftp bmachine\@$hp1123PA:$HPwa/HP-UX-$PRODver-$stripbuildnum.depot);

rename "HP-UX-$PRODver-$stripbuildnum.depot","1123pa.depot";

}

############################
# HP1123ipf
############################

sub hp1123ipf {

print "\n\nhp1123IPF  :  sftp sys call \n";
print "\n\n$hp1123IPF\n";
print "\n\n\$HPwa     =   $HPwa \n\n";
print "\n\nHP-UX-\$PRODver-$stripbuildnum.depot  \t=\nHP-UX-$PRODver-$stripbuildnum.depot\n\n";

system qq(sftp bmachine\@$hp1123IPF:$HPwa/HP-UX-$PRODver-$stripbuildnum.depot);

rename "HP-UX-$PRODver-$stripbuildnum.depot","1123ipf.depot";

}

############################
# HP1131ipf
############################

sub hp1131ipf {

print "\n\nhp1131IPF  :  sftp sys call \n";
print "\n\n$hp1131IPF\n";
print "\n\n\$HPwa     =   $HPwa \n\n";
print "\n\nHP-U-\$PRODver-$stripbuildnum.depot  \t=\nHP-UX-$PRODver-$stripbuildnum.depot\n\n";

system qq(sftp bmachine\@$hp1131IPF:$HPwa/HP-UX-$PRODver-$stripbuildnum.depot);

rename "HP-UX-$PRODver-$stripbuildnum.depot","1131ipf.depot";

}

############################
# HP1131pa
############################

sub hp1131pa {

print "\n\nhp1131PA  :  sftp sys call \n";
print "\n\n$hp1131PA\n";
print "\n\n\$HPwa     =   $HPwa \n\n";
print "\n\nHP-UX-\$PRODver-$stripbuildnum.depot  \t=\nHP-UX-$PRODver-$stripbuildnum.depot\n\n";

system qq(sftp bmachine\@$hp1131PA:$HPwa/HP-UX-$PRODver-$stripbuildnum.depot);

rename "HP-UX-$PRODver-$stripbuildnum.depot","1131pa.depot";

}

############################
#  RedHatAS4x32 
############################

sub RHAS4x32 {
   
print "\n\nRHAS4x32  :  sftp sys call \n";
print "\n\n$RHAS4x32\n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.i686.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.i686.rpm\n\n";

system qq(sftp bmachine\@$RHAS4x32:$LNXwa/i686/$RedHatProduct-$PRODver-$stripbuildnum.i686.rpm);


}

############################
#  RedHatAS4x64 
############################

sub RHAS4x64 {
   
print "\n\nRHAS4x64  :  sftp sys call \n";
print "\n\n$RHAS4x64\n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.x86_64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm\n\n";

system qq(sftp bmachine\@$RHAS4x64:$LNXwa/x86_64/$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm);

}

############################
#  RedHatAS4xia64 
############################

sub RHAS4xia64 {
   
print "\n\nRHAS4xia64  :  sftp sys call \n";
print "\n\n$RHAS4xia64\n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.ia64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm\n\n";

system qq(sftp bmachine\@$RHAS4xia64:$LNXwa/ia64/$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm);

}

############################
#  RedHat 5x ia64 
############################

sub RHAS5xia64 {

print "\n\nRHAS5xia64  :  sftp sys call \n";
print "\n\n$RHAS5xia64\n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.ia64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm\n\n";

system qq(sftp bmachine\@$RHAS5xia64:$LNXwa/ia64/$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm);
}

############################
#  RedHat 5x x64 
############################

sub RHAS5xx64 {
print "\n\nRHAS5x64  :  sftp sys call \n";
print "\n\n$RHAS5xx64\n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.x86_64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm\n\n";

system qq(sftp bmachine\@$RHAS5xx64:$LNXwa/x86_64/$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm);

}

############################
#  RedHat 5x x86 
############################

sub RHAS5xx86 {

print "\n\nRHAS5xx86  :  sftp sys call \n";
print "\n\n$RHAS5xx86\n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.i686.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.i686.rpm\n\n";

system qq(sftp bmachine\@$RHAS5xx86:$LNXwa/i686/$RedHatProduct-$PRODver-$stripbuildnum.i686.rpm);
   
}

############################
# AIX51  
############################

sub aix51 {

print "\n\naix51  :  sftp sys call \n";
print "\n\n$aix51\n";
print "\n\n\$AIXwa     =   $AIXwa \n\n";
print "\n\ndtc.rte\n\n";
print "\n\n.toc\n\n";

system qq(sftp bmachine\@$aix51:$AIXwa/dtc.rte);
sleep 10;
#system qq(sftp bmachine\@$aix51:$AIXwa/.toc);

}

############################
# AIX52 
############################

sub aix52 {

print "\n\naix52  :  sftp sys call \n";
print "\n\n$aix52\n";
print "\n\n\$AIXwa     =   $AIXwa \n\n";
print "\n\ndtc.rte\n\n";
print "\n\n.toc\n\n";

system qq(sftp bmachine\@$aix52:$AIXwa/dtc.rte);
sleep 10;
#system qq(sftp bmachine\@$aix52:$AIXwa/.toc);

}

############################
# AIX53 
############################

sub aix53 {
   
use Net::FTP;
   $ftp=Net::FTP->new("$aix53")  or warn "can't connect aix53 env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : aix53 : error : $@\n";

   $ftp->cwd("$AIXwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("dtc.rte") or warn "aix53 get error : bRftppkg.pl script: $!";

   $ftp->ascii;
                                                                                
   #$ftp->get(".toc") or warn "aix53 get error : bRftppkg.pl script: $!";

   $ftp->quit;

}

############################
# AIX61 
############################

sub aix61 {
   
use Net::FTP;
   $ftp=Net::FTP->new("$aix61")  or warn "can't connect aix61 env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : aix61 : error : $@\n";

   $ftp->cwd("$AIXwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("dtc.rte") or warn "aix61 get error : bRftppkg.pl script: $!";

   $ftp->ascii;
                                                                                
   #$ftp->get(".toc") or warn "aix61 get error : bRftppkg.pl script: $!";

   $ftp->quit;

}

############################
# SuSE9x ia64 
############################

sub suse9xia64 {

print "\n\nsuse9xia64  :  sftp sys call \n";
print "\n\n$suse9xia64\n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.ia64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm\n\n";

system qq(sftp bmachine\@$suse9xia64:$LNXwa/ia64/$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm);

}

############################
# SuSE9x x64 
############################

sub suse9xx64 {
   
print "\n\nsuse9xx64  :  sftp sys call \n";
print "\n\n$suse9xx64\n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.x86_64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm\n\n";

system qq(sftp bmachine\@$suse9xx64:$LNXwa/x86_64/$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm);

}

############################
# SuSE9x x86
############################

sub suse9xx86 {

print "\n\nsuse9xx86  :  sftp sys call \n";
print "\n\n$suse9xx86\n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.i686.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.i686.rpm\n\n";

system qq(sftp bmachine\@$suse9xx86:$LNXwa/i686/$RedHatProduct-$PRODver-$stripbuildnum.i686.rpm);

}

############################
# SuSE10x ia64
############################

sub suse10xia64 {
   
print "\n\nsuse10xia64  :  sftp sys call \n";
print "\n\n$suse10xia64\n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.ia64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm\n\n";

system qq(sftp bmachine\@$suse10xia64:$LNXwa/ia64/$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm);

}

############################
# SuSE10x x64
############################

sub suse10xx64 {

print "\n\n$suse10xx64  :  sftp sys call \n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.x86_64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm\n\n";

system qq(sftp bmachine\@$suse10xx64:$LNXwa/x86_64/$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm);

}

############################
# SuSE10x x86
############################

sub suse10xx86 {
   
print "\n\n$suse10xx86  :  sftp sys call \n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.i686.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.i686.rpm\n\n";

system qq(sftp bmachine\@$suse10xx86:$LNXwa/i686/$RedHatProduct-$PRODver-$stripbuildnum.i686.rpm);

}

############################
# SuSE11x ia64
############################

sub suse11xia64 {
   
print "\n\nsuse11xia64  :  sftp sys call \n";
print "\n\n$suse11xia64\n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.ia64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm\n\n";

system qq(sftp bmachine\@$suse11xia64:$LNXwa/ia64/$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm);

}

############################
# SuSE11x x64
############################

sub suse11xx64 {

print "\n\n$suse10xx64  :  sftp sys call \n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.x86_64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm\n\n";

system qq(sftp bmachine\@$suse11xx64:$LNXwa/x86_64/$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm);

}

############################
# SuSE11x x86
############################

sub suse11xx86 {
   
print "\n\n$suse11xx86  :  sftp sys call \n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.i686.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.i686.rpm\n\n";

system qq(sftp bmachine\@$suse11xx86:$LNXwa/i686/$RedHatProduct-$PRODver-$stripbuildnum.i686.rpm);

}

###################################################
###################################################

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

   if ("$ipaddr" =~ /sjbrhlas3m2/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   elsif ("$ipaddr" =~ /sjbrhlas3m3/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   elsif ("$ipaddr" =~ /sji641hs/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   elsif ("$ipaddr" =~ /sji641ht/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   elsif ("$ipaddr" =~ /sji641hv/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   elsif ("$ipaddr" =~ /sjbrhlas4m3/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   elsif ("$ipaddr" =~ /sjbrhlas4m2/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   elsif ("$ipaddr" =~ /sjbrhlas4m1/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   elsif ("$ipaddr" =~ /sjbsolv10m1/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   elsif ("$ipaddr" =~ /sjx64chp/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   elsif ("$ipaddr" =~ /sjx64chr/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   elsif ("$ipaddr" =~ /sjx64lxd/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   elsif ("$ipaddr" =~ /sjx64lxb/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   elsif ("$ipaddr" =~ /sjx64lxc/) {
      @sshit = system qq(ssh -q -l bmachine $ipaddr ls);
      $rshRC=$sshit[0];
   }
   elsif ("$ipaddr" =~ /sjx64lxa/) {
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

