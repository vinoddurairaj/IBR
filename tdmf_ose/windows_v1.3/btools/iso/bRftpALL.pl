#!/usr/local/bin/perl -w
###################################
#################################
#
#  perl -w -I.. ./bRftpALL.pl
#
#	ftp $RFWver.$Bbnum.ALL.zip to linux machine
#
#	create iso image script
#
#################################
###################################

require "bRglobalvars.pm";

open (STDOUT, ">isoLOG.txt");
open (STDERR, ">>&STDOUT");

print "\n\nSTART bRftpALL.pl\n\n";

#######################################
#####################################
# grab password for bmachine account on fokker
#######################################
#######################################

open(PSWD,"..\\bRbmachine.txt");
@psswd=<PSWD>;
close(PSWD);

$passwd=$psswd[0];

#print "\n\n$passwd\n\n";

chdir "..\\..\\..\\";

system qq($cd);
system qq($cd);

######################################
####################################
# ftpALL subroutine
####################################
######################################

ftpALL();

sub ftpALL {

use Net::FTP;
$ftp=Net::FTP->new("129.212.206.131")	or die "can't connect  : $@\n";

$ftp->login("bmachine","$passwd")	or die "could not login : $@\n";

$ftp->cwd("/u01/bmachine");

$ftp->mkdir("iso");

$ftp->cwd("iso");

$ftp->mkdir("$RFWver");

$ftp->cwd("$RFWver");

$ftp->mkdir("$Bbnum");

$ftp->cwd("$Bbnum");

$ftp->binary;

$ftp->put("$RFWver.$Bbnum.ALL.zip");

$ftp->mkdir("OEM");

$ftp->cwd("OEM");

$ftp->mkdir("SF");

$ftp->cwd("SF");

$ftp->put("$RFWver.$Bbnum.ALL.SF.zip");

$ftp->quit;

print "end ftp:  bye\n";
# end bftp sub routine
}

print "\n\nEND bRftpALL.pl\n\n";

######################################
######################################

close (STDERR);
close (STDOUT);
