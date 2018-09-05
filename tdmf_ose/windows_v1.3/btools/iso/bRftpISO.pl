#!/usr/local/bin/perl -w
###################################
#################################
#
#  perl -w -I.. ./bRftpISO.pl
#
#	ftp $RFWver.$Bbnum.iso to bmachine12
#
#################################
###################################

require "bRglobalvars.pm";

#######################################
#####################################
# capture STDOUT & STDERR
#######################################
#######################################

open (STDOUT, ">>isoLOG.txt");
open (STDERR, ">>&STDOUT");

print "\n\nSTART bRftpISO.pl\n\n";

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

chdir "..\\..\\..";

system qq($cd);

system qq($cd);

######################################
####################################
# ftpISO subroutine
####################################
######################################

ftpISO();

sub ftpISO {

use Net::FTP;
$ftp=Net::FTP->new("129.212.206.131")	or die "can't connect  : $@\n";

#insert your fokker userid in place of "bmachine"

$ftp->login("bmachine","$passwd")	or die "could not login : $@\n";

$ftp->cwd("/u01/bmachine/iso/$RFWver/$Bbnum")      or warn "ftp get warning  :  $!";

$ftp->binary;

$ftp->get("$RFWver.$Bbnum.iso");

$ftp->cwd("OEM/SF");

$ftp->binary;

$ftp->get("$RFWver.$Bbnum.StoneFly.iso");

$ftp->quit;

print "end ftpISO  :  bye\n";
# end bftp sub routine
}

print "\n\nEND bRftpISO.pl\n\n";

######################################
######################################

close (STDERR);
close (STDOUT);

