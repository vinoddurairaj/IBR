#!/usr/bin/perl -Ipms
#####################################
###################################
#
# perl -w -I".." ./bRftpOBADpl
#
#	grab RFX  && TUIP BAD packages
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
our $suse11xia64;
our $suse11xx64;
our $suse11xx86;
our $RHAS5xia64;
our $RHAS5xx64;
our $RHAS5xx86;

our $scriptInitialDirectory;

our $LWORK="/localwork/bmachine";

# generic ftp locations and product name
our $AIXwa;
our $HPwa;
our $SOLwa;
our $LNXwa;
our $SUSEwa;
our $RedHatProduct;	# because the RedHat products are named "Replicator-" and "TDMFIP-" (as opposed to the others, which are the same)

# bRftppkg.pl - RFX ftp locations
#our $rfxAIXwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/installp/mklpp/rel/dist/CD-ROM/usr/sys/inst.images";
#our $rfxHPwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/CDROMDIR";
#our $rfxSOLwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install";
#our $rfxLNXwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install";
our $rfxLNXwa686="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/i686";
our $rfxLNXwaia64="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/ia64";
#our $rfxLNXRH4xwas390x="$LWORK/dev/RH4x/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/s390x";
#our $rfxLNXRH5xwas390x="$LWORK/dev/RH5x/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/s390x";
our $rfxLNXwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/x86_64";
#our $rfxSUSEwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install";
our $rfxSUSEwa686="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/i686";
our $rfxSUSEwaia64="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/ia64";
#our $rfxSUSE9xwas390x="$LWORK/dev/SuSE9x/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/s390x";
#our $rfxSUSE10xwas390x="$LWORK/dev/SuSE10x/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/s390x";
our $rfxSUSEwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/x86_64";
our $rfxInitialDirectory = "../builds/$Bbnum/BAD";
our $rfxRedHatProduct = "Replicator-BAD";
#our $rfxRedHatProduct = "OMP";

#bIftppkg.pl - TUIP ftp locations
#our $tuipAIXwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/installp/mklpp/rel/dist/CD-ROM/usr/sys/inst.images";
#our $tuipHPwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/CDROMDIR";
#our $tuipSOLwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install";
our $tuipLNXwa686="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/i686";
our $tuipLNXwaia64="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/ia64";
#our $tuipLNXRH4xwas390x="$LWORK/dev/RH4x/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/s390x";
#our $tuipLNXRH5xwas390x="$LWORK/dev/RH5x/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/s390x";
our $tuipLNXwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/x86_64";
our $tuipSUSEwa686="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/i686";
our $tuipSUSEwaia64="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/ia64";
#our $tuipSUSE9xwas390x="$LWORK/dev/SuSE9x/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/s390x";
#our $tuipSUSE10xwas390x="$LWORK/dev/SuSE10x/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/s390x";
#our $tuipSUSEwas390x="$LWORK/dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/s390x";
our $tuipSUSEwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/x86_64";
our $tuipInitialDirectory = "../builds/tuip/$Bbnum/BAD";
our $tuipRedHatProduct = "TDMFIP-BAD";
#our $tuipRedHatProduct = "OMP";

#predeclare sub  :  add $ to real call to doUploads too
sub doUploads ($);

#####################################
###################################
# STDOUT  /  STDERR
###################################
#####################################

open (STDOUT, ">logs/ftpbad.txt");
open (STDERR, ">&STDOUT");

print "\n\nSTART  :  bRftpbad.pl\n\n";

#####################################
###################################
# get el passwordo
###################################
#####################################

#open(PSWD,"bRbmachine1.txt");
#@psswd=<PSWD>;
#close(PSWD);

#$passwd=$psswd[0];

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
#$AIXwa = $rfxAIXwa; 
#$HPwa = $rfxHPwa;
#$SOLwa = $rfxSOLwa;
$LNXwa686 = $rfxLNXwa686;
$LNXwaia64 = $rfxLNXwaia64;
#$LNXRH4xwas390x = $rfxLNXRH4xwas390x;
#$LNXRH5xwas390x = $rfxLNXRH5xwas390x;
$LNXwa = $rfxLNXwa;
$SUSEwa686 = $rfxSUSEwa686;
$SUSEwaia64 = $rfxSUSEwaia64;
#$SUSE9xwas390x = $rfxSUSE9xwas390x;
#$SUSE10xwas390x = $rfxSUSE10xwas390x;
$SUSEwa = $rfxSUSEwa;
$RedHatProduct = $rfxRedHatProduct;

doUploads($rfxInitialDirectory);

# reset working directory as if script were restarted
chdir "$scriptInitialDirectory" || die "chdir : died : here";
system qq(pwd);

# do uploads for TUIP
#$AIXwa = $tuipAIXwa; 
#$HPwa = $tuipHPwa;
#$SOLwa = $tuipSOLwa;
$LNXwa686 = $tuipLNXwa686;
$LNXwaia64 = $tuipLNXwaia64;
#$LNXRH4xwas390x = $tuipLNXRH4xwas390x;
#$LNXRH5xwas390x = $tuipLNXRH5xwas390x;
$LNXwa = $tuipLNXwa;
$SUSEwa686 = $tuipSUSEwa686;
$SUSEwaia64 = $tuipSUSEwaia64;
#$SUSE9xwas390x = $tuipSUSE9xwas390x;
#$SUSE10xwas390x = $tuipSUSE10xwas390x;
$SUSEwa = $tuipSUSEwa;
$RedHatProduct = $tuipRedHatProduct;

doUploads($tuipInitialDirectory);

###################################################
###################################################

print "\n\nEND  :  bRftpBAD.pl\n\n";

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

sub doUploads($) {

     my $directoryPrefix = $_[0];
     print "\nENTER: subroutine began: doUploads($directoryPrefix)\n\n";
 
     ############################
     #  RedHat AS4x32  :  ftp deliverable from AS 4x 32 build machine.... 
     ############################

     chdir "$directoryPrefix" || die "chdir  :  died  :  here";
 
     chdir "RedHat/4x/x86_32";
     system qq(pwd);

     $ipaddr="$RHAS4x32";

     RHAS4x32();

     undef($ipaddr);

     ############################
     #  RedHat AS4x64
     ############################

     chdir "../x86_64";
     system qq(pwd);

     $ipaddr="$RHAS4x64";

     RHAS4x64();

     undef($ipaddr);

     ############################
     #  RedHat AS4xia64
     ############################

     chdir "../ia64";
     system qq(pwd);

     $ipaddr="$RHAS4xia64";

     RHAS4xia64();

     undef($ipaddr);

     ############################
     #  RedHat AS5 x64
     ############################

     chdir "../../5x/x86_64";
     system qq(pwd);

     $ipaddr="$RHAS5xx64";

     RHAS5xx64();

     undef($ipaddr);

     ############################
     #  RedHat AS5x x86  (i686)
     ############################

     chdir "../x86_32";
     system qq(pwd);

     $ipaddr="$RHAS5xx86";

     RHAS5xx86();

     undef($ipaddr);

     ############################
     #  RedHat AS5x ia64
     ############################

     chdir "../ia64";
     system qq(pwd);

     $ipaddr="$RHAS5xia64";

     RHAS5xia64();

     undef($ipaddr);

     ############################
     #  SUSE 9x x64
     ############################

     chdir "../../../SuSE/9x/x86_64";
     system qq(pwd);

     $ipaddr="$suse9xx64";

     suse9xx64();

     undef($ipaddr);

     ############################
     #  SUSE 9x x86
     ############################

     chdir "../x86_32";
     system qq(pwd);

     $ipaddr="$suse9xx86";

     suse9xx86();

     undef($ipaddr);

     ############################
     #  SUSE 9x ia64
     ############################

     chdir "../ia64";
     system qq(pwd);

     $ipaddr="$suse9xia64";

     suse9xia64();

     undef($ipaddr);

     ############################
     #  SUSE 10x x64
     ############################

     chdir "../../10x/x86_64";
     system qq(pwd);

     $ipaddr="$suse10xx64";

     suse10xx64();

     undef($ipaddr);

     ############################
     #  SUSE 10x x86
     ############################

     chdir "../x86_32";
     system qq(pwd);

     $ipaddr="$suse10xx86";

     suse10xx86();

     undef($ipaddr);

     ############################
     #  SUSE 10x ia64
     ############################

     chdir "../ia64";
     system qq(pwd);

     $ipaddr="$suse10xia64";

     suse10xia64();

     undef($ipaddr);

     ############################
     #  SUSE 11x x64
     ############################

     chdir "../../11x/x86_64";
     system qq(pwd);

     $ipaddr="$suse11xx64";

     suse11xx64();

     undef($ipaddr);

     ############################
     #  SUSE 11x x86
     ############################

     chdir "../x86_32";
     system qq(pwd);

     $ipaddr="$suse11xx86";

     suse11xx86();

     undef($ipaddr);

     ############################
     #  SUSE 11x ia64
     ############################

     #chdir "../ia64";
     #system qq(pwd);

     #$ipaddr="$suse11xia64";

     #suse11xia64();

     #undef($ipaddr);

     #print "\nEND: subroutine completed: doUpload($directoryPrefix)\n\n";
}


###################################################
#################################################
# subroutine(s)
#################################################
###################################################

############################
#  RedHatAS4x32 
############################

sub RHAS4x32 {
   
print "\n\nRHAS4x32  :  sftp sys call \n";
print "\n\n$RHAS4x32\n";
print "\n\n\$LNXwa686     =   $LNXwa686 \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.i686.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.i686.rpm\n\n";

system qq(sftp bmachine\@$RHAS4x32:$LNXwa686/$RedHatProduct-$PRODver-$stripbuildnum.i686.rpm);


}

############################
#  RedHatAS4x64 
############################

sub RHAS4x64 {
   
print "\n\nRHAS4x64  :  sftp sys call \n";
print "\n\n$RHAS4x64\n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.x86_64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm\n\n";

system qq(sftp bmachine\@$RHAS4x64:$LNXwa/$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm);

}

############################
#  RedHatAS4xia64 
############################

sub RHAS4xia64 {
   
print "\n\nRHAS4xia64  :  sftp sys call \n";
print "\n\n$RHAS4xia64\n";
print "\n\n\$LNXwaia64     =   $LNXwaia64 \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.ia64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm\n\n";

system qq(sftp bmachine\@$RHAS4xia64:$LNXwaia64/$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm);

}

############################
#  RedHatAS4xs390x
############################

sub RHAS4xs390x {
   
print "\n\nRHAS4xs390x  :  sftp sys call \n";
print "\n\n$zRH4xs390x64\n";
print "\n\n\$LNX4xwas390x     =   $LNXRH4xwas390x \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.s390x.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.s390x.rpm\n\n";

system qq(sftp bmachine\@$zRH4xs390x64:$LNXRH4xwas390x/$RedHatProduct-$PRODver-$stripbuildnum.s390x.rpm);

}

############################
#  RedHat 5x x64 
############################

sub RHAS5xx64 {
print "\n\nRHAS5x64  :  sftp sys call \n";
print "\n\n$RHAS5xx64\n";
print "\n\n\$LNXwa   =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.x86_64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm\n\n";

system qq(sftp bmachine\@$RHAS5xx64:$LNXwa/$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm);   

}

############################
#  RedHat 5x x86 
############################

sub RHAS5xx86 {

print "\n\nRHAS5xx86  :  sftp sys call \n";
print "\n\n$RHAS5xx86\n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.i686.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.i686.rpm\n\n";

system qq(sftp bmachine\@$RHAS5xx86:$LNXwa686/$RedHatProduct-$PRODver-$stripbuildnum.i686.rpm);
   
}

############################
#  RedHat 5x ia64
############################

sub RHAS5xia64 {

print "\n\nRHAS5xia64  :  sftp sys call \n";
print "\n\n$RHAS5xia64\n";
print "\n\n\$LNXwaia64     =   $LNXwaia64\n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.ia64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm\n\n";

system qq(sftp bmachine\@$RHAS5xia64:$LNXwaia64/$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm);
   
}

############################
#  RedHat 5x s390x
############################

sub RHAS5xs390x {

print "\n\nRHAS5xs390x  :  sftp sys call \n";
print "\n\n$zRH5xs390x64\n";
print "\n\n\$LNXRH5xwas390x     =   $LNXRH5xwas390x\n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.s390x.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.s390x.rpm\n\n";

system qq(sftp bmachine\@$zRH5xs390x64:$LNXRH5xwas390x/$RedHatProduct-$PRODver-$stripbuildnum.s390x.rpm);
   
}

############################
# SuSE9x x64 
############################

sub suse9xx64 {
   
print "\n\nsuse9xx64  :  sftp sys call \n";
print "\n\n$suse9xx64\n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.x86_64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm\n\n";

system qq(sftp bmachine\@$suse9xx64:$LNXwa/$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm);

}

############################
# SuSE9x x86
############################

sub suse9xx86 {

print "\n\nsuse9xx86  :  sftp sys call \n";
print "\n\n$suse9xx86\n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.i686.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.i686.rpm\n\n";

system qq(sftp bmachine\@$suse9xx86:$LNXwa686/$RedHatProduct-$PRODver-$stripbuildnum.i686.rpm);

}

############################
# SuSE9x ia64
############################

sub suse9xia64 {

print "\n\nsuse9xx86  :  sftp sys call \n";
print "\n\n$suse9xia64\n";
print "\n\n\$SuSEwaia64     =   $SUSEwaia64 \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.ia64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm\n\n";

system qq(sftp bmachine\@$suse9xia64:$SUSEwaia64/$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm);

}

############################
# SuSE9x s390x 
############################

sub suse9xs390x {

print "\n\nsuse9xs390x  :  sftp sys call \n";
print "\n\n$zsuse9xs390x64\n";
print "\n\n\$SuSE9xwas390x     =   $SUSE9xwas390x\n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.s390x.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.s390x.rpm\n\n";

system qq(sftp bmachine\@$zsuse9xs390x64:$SUSE9xwas390x/$RedHatProduct-$PRODver-$stripbuildnum.s390x.rpm);

}

############################
# SuSE10x x64
############################

sub suse10xx64 {

print "\n\n$suse10xx64  :  sftp sys call \n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.x86_64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm\n\n";

system qq(sftp bmachine\@$suse10xx64:$LNXwa/$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm);

}

############################
# SuSE10x x86
############################

sub suse10xx86 {
   
print "\n\n$suse10xx86  :  sftp sys call \n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.i686.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm\n\n";

system qq(sftp bmachine\@$suse10xx86:$LNXwa686/$RedHatProduct-$PRODver-$stripbuildnum.i686.rpm);

}

############################
# SuSE10x ia64
############################

sub suse10xia64 {

print "\n\nsuse10ia64  :  sftp sys call \n";
print "\n\n$suse10xia64\n";
print "\n\n\$SUSEaia64     =   $SUSEwaia64 \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.ia64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm\n\n";

system qq(sftp bmachine\@$suse10xia64:$SUSEwaia64/$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm);

}

############################
# SuSE11x x64
############################

sub suse11xx64 {

print "\n\n$suse11xx64  :  sftp sys call \n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.x86_64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm\n\n";

system qq(sftp bmachine\@$suse11xx64:$LNXwa/$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm);

}

############################
# SuSE11x x86
############################

sub suse11xx86 {
   
print "\n\n$suse11xx86  :  sftp sys call \n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.i686.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.x86_64.rpm\n\n";

system qq(sftp bmachine\@$suse11xx86:$LNXwa686/$RedHatProduct-$PRODver-$stripbuildnum.i686.rpm);

}

############################
# SuSE11x ia64
############################

sub suse11xia64 {

print "\n\nsuse11xia64  :  sftp sys call \n";
print "\n\n$suse11xia64\n";
print "\n\n\$SUSEaia64     =   $SUSEwaia64 \n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.ia64.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm\n\n";

system qq(sftp bmachine\@$suse11xia64:$SUSEwaia64/$RedHatProduct-$PRODver-$stripbuildnum.ia64.rpm);

}

############################
# SuSE10x s390x 
############################

sub suse10xs390x {

print "\n\nsuse10xs390x  :  sftp sys call \n";
print "\n\n$zsuse10xs390x64\n";
print "\n\n\$SuSE10xwas390x     =   $SUSE10xwas390x\n\n";
print "\n\n\$RedHatProduct-\$PRODver-\$stripbuildnum.s390x.rpm  \t=\n$RedHatProduct-$PRODver-$stripbuildnum.s390x.rpm\n\n";

system qq(sftp bmachine\@$zsuse10xs390x64:$SUSE10xwas390x/$RedHatProduct-$PRODver-$stripbuildnum.s390x.rpm);

}


###################################################
###################################################

