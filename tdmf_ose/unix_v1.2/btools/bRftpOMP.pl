#!/usr/bin/perl -Ipms
#####################################
###################################
#
# perl -w -I".." ./bRftpOMP.pl
#
#	grab RFX  && TUIP OMP packages
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
#our $suse11xia64;
our $suse11xx64;
our $suse11xx86;
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
#our $rfxAIXwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/installp/mklpp/rel/dist/CD-ROM/usr/sys/inst.images";
#our $rfxHPwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/CDROMDIR";
#our $rfxSOLwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install";
#our $rfxLNXwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install";
our $rfxLNXwa686="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/i686";
our $rfxLNXwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/x86_64";
#our $rfxSUSEwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install";
our $rfxSUSEwa686="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/i686";
our $rfxSUSEwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/x86_64";
our $rfxInitialDirectory = "../builds/$Bbnum/OMP";
#our $rfxRedHatProduct = "Replicator";
our $rfxRedHatProduct = "OMP";

#bIftppkg.pl - TUIP ftp locations
#our $tuipAIXwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/installp/mklpp/rel/dist/CD-ROM/usr/sys/inst.images";
#our $tuipHPwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/CDROMDIR";
#our $tuipSOLwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/pkg.install";
our $tuipLNXwa686="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/i686";
our $tuipLNXwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/x86_64";
our $tuipSUSEwa686="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/i686";
our $tuipSUSEwa="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/buildroot/RPMS/x86_64";
our $tuipInitialDirectory = "../builds/tuip/$Bbnum/OMP";
#our $tuipRedHatProduct = "TDMFIP";
our $tuipRedHatProduct = "OMP";

#predeclare sub  :  add $ to real call to doUploads too
sub doUploads ($);

#####################################
###################################
# STDOUT  /  STDERR
###################################
#####################################

open (STDOUT, ">logs/ftpomp.txt");
open (STDERR, ">&STDOUT");

print "\n\nSTART  :  bRftpOMP.pl\n\n";

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
$LNXwa = $rfxLNXwa;
$SUSEwa686 = $rfxSUSEwa686;
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
$LNXwa = $tuipLNXwa;
$SUSEwa686 = $tuipSUSEwa686;
$SUSEwa = $tuipSUSEwa;
$RedHatProduct = $tuipRedHatProduct;

doUploads($tuipInitialDirectory);

###################################################
###################################################

print "\n\nEND  :  bRftpOMP.pl\n\n";

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

     ############################
     #  SUSE 9x x64  deliverable....
     ############################

     chdir "../../../SuSE/9x/x86_64";
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
# subroutine(s)
#################################################
###################################################

############################
#  RedHatAS4x32 
############################

sub RHAS4x32 {
   
print "\n\nRHAS4x32  :  sftp sys call \n";
print "\n\n$RHAS4x32\n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
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
#  RedHat 5x x64 
############################

sub RHAS5xx64 {
print "\n\nRHAS5x64  :  sftp sys call \n";
print "\n\n$RHAS5xx64\n";
print "\n\n\$LNXwa     =   $LNXwa \n\n";
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

