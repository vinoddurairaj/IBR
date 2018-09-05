#!/usr/local/bin/perl -w
#####################################
###################################
#
# perl -w -I".." ./bRftppkg.pl
#
#	grab unix agents from packaged intermediate
#	target directories
#
###################################
#####################################

#####################################
###################################
# global vars
###################################
#####################################

require "bRglobalvars.pm";
$branchdir="$branchdir";
#$AIXwa="/home/bmachine/dev/$branchdir/SSM/builds/$Bbnum/SSM";  # need to figure out the $HOME setting ????
$HPwa="/home/bmachine/dev/$branchdir/tdmf_ose/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc/CDROMDIR";
$SOLwa="dev/$branchdir/tdmf_ose/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc";
#$LNXwa="/home/bmachine/dev/$branchdir/SSM/builds/$Bbnum/SSM";

#####################################
###################################
# capture stderr & stdout
###################################
#####################################

open (STDOUT, ">ftpLOG.txt");
open (STDERR, ">>&STDOUT");

print "\n\nSTART  :  bRftppkg.pl\n\n";

#####################################
# get el passwordo
#####################################

open(PSWD,"bRbmachine1.txt");
@psswd=<PSWD>;
close(PSWD);

$passwd=$psswd[0];

#print "\n$passwd & $passwd0\n";

system qq($cd);

print "\n$SOLwa\n";

#	129.212.239.77 		solaris\\7
#	tdmfsol.pkg

#	129.212.102.212 		solaris\\2.6
#	tdmfsol.pkg

#	129.212.102.213  		solaris\\8
#	tdmfsol.pkg

#	129.212.206.7		solaris\\9
#	tdmfsol.pkg


#	129.212.			Linux  Redhat 9
#

#	129.212.			AIX\\3.4.4
#

#	129.212.			AIX 5.1  32 byte 
#
#	129.212.			AIX 5.1  64 byte
#
#	129.212.			AIX 5.2  32 byte
#
#	129.212.			AIX 5.2  64 byte 
#

#	129.212.102.160 		HPUX\\11
#

#	129.212.102.  		HPUX\\11i
#
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

chdir "..\\builds\\$Bbnum\\solaris\\7" || die "chdir  :  died  :  here";

system qq($cd);

use Net::FTP;
   $ftp=Net::FTP->new("129.212.239.77")  or warn "can't connect solaris 7 env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : solaris 7 : error : $@\n";

   $ftp->cwd("$SOLwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("tdmfsol.pkg") or warn "solaris 7 get error : tdmfsol.pkg script: $!";

   $ftp->quit;

############################
# solaris :  2.6 
############################

chdir "..\\2.6";

system qq($cd);

use Net::FTP;
   $ftp=Net::FTP->new("129.212.102.212")  or warn "can't connect solaris 2.6 env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : solaris 2.6 : error : $@\n";

   $ftp->cwd("$SOLwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("tdmfsol.pkg") or warn "solaris 2.6 get error : tdmfsol.pkg script: $!";

   $ftp->quit;

############################
# solaris :  8 
############################

chdir "..\\8";

system qq($cd);

use Net::FTP;
   $ftp=Net::FTP->new("129.212.102.213")  or warn "can't connect solaris 8 env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : solaris 8 : error : $@\n";

   $ftp->cwd("$SOLwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("tdmfsol.pkg") or warn "solaris 8 get error : tdmfsol.pkg script: $!";

   $ftp->quit;

############################
# solaris :  9 
############################

chdir "..\\9";

system qq($cd);

use Net::FTP;
   $ftp=Net::FTP->new("129.212.206.7")  or warn "can't connect solaris 9 env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : solaris 9 : error : $@\n";

   $ftp->cwd("$SOLwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("tdmfsol.pkg") or warn "solaris 9 get error : tdmfsol.pkg script: $!";

   $ftp->quit;

############################
# HP :  11.00 
############################

chdir "..\\..\\HPUX\\11";

system qq($cd);

use Net::FTP;
   $ftp=Net::FTP->new("129.212.102.160")  or warn "can't connect HP 11.00 env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : HP 11.00 : error : $@\n";

   $ftp->cwd("$HPwa");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("sw.tar") or warn "HP 11.00 get error : bRftppkg.pl script: $!";

   $ftp->quit;

print "\n\nEND  :  bRftppkg.pl\n\n";

########################################

close (STDERR);
close (STDOUT);
