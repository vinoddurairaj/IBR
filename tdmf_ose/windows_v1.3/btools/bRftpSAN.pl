#!/usr/local/bin/perl -w
###################################
#################################
#
#  ./bRftpSAN.pl
#
#	ftp deliverables to Sunnyvale SAN server
#
#	\\\\129.212.65.20\\builds\\Replicator\\Windows\\$RFWver\\$Bbnum
#
#	ftp \\builds\\Replicator\\Windows\\$RFWver\\$Bbnum
#
#################################
###################################

require "bRglobalvars.pm";
$RFWver="$RFWver";
$file0="$RFWver.$Bbnum.ALL.zip";
$file1="$RFWver.$Bbnum.RFWexe.zip";
$file2="$RFWver.$Bbnum.iso";
$file3="$RFWver.$Bbnum.StoneFly.iso";
$TXTfiles="x*.txt R*.txt c*.txt D*.txt";  # can not use b*.txt  =>> sends password file to server
$TXTfiles0="N*.txt W*.txt P*.txt p*.txt e*.txt E*.txt";  # can not use b*.txt  =>> sends password file to server

open (STDOUT, ">ftpBUILDS.txt");
open (STDERR, ">>&STDOUT");

print "\n\nSTART bRftpSAN.pl\n\n";

#######################################
#####################################
# grab password for bmachine account on fokker
#######################################
#######################################

chdir "..\\..\\";

system qq($cd);
system qq($cd);

# setup var to cd toooo.

######################################
####################################
# ftp to SAN Server  :  Sunnyvale Build Server
####################################
######################################

ftpSAN();

sub ftpSAN {

system qq(touch bmachine.txt);
system qq(xcopy /K /V /Y windows_v1.3\\btools\\bmachine.txt);

system qq(touch EMAILDIST.txt);
system qq(xcopy /K /V /Y windows_v1.3\\btools\\EMAILDIST.txt);

system qq(touch $RFWver.$Bbnum.txt);
system qq(xcopy /K /V /Y windows_v1.3\\btools\\$RFWver.$Bbnum.txt);

system qq(touch xcopy_it.txt);
system qq(xcopy /K /V /Y windows_v1.3\\btools\\xcopy_it.txt);

system qq(touch createisoLOG.txt);
system qq(xcopy /K /V /Y windows_v1.3\\btools\\createisoLOG.txt);

system qq(touch DDKzipLOG.txt);
system qq(xcopy /K /V /Y windows_v1.3\\btools\\DDKzipLOG.txt);

system qq(touch prebuildOEM.txt);
system qq(xcopy /K /V /Y windows_v1.3\\btools\\prebuildOEM.txt);

system qq(touch PARSELOG.txt);
system qq(xcopy /K /V /Y windows_v1.3\\btools\\PARSELOG.txt);

system qq(touch NTLOG.txt);
system qq(xcopy /K /V /Y windows_v1.3\\NTLOG.txt);

system qq(touch W2kLOG.txt);
system qq(xcopy /K /V /Y windows_v1.3\\W2kLOG.txt);

#######################################
#####################################
# ncftput  :  required deliverables
#######################################
#######################################

system qq(ncftpput -f bmachine.txt -m /builds/Replicator/Windows/$RFWver/$Bbnum $file0 $file1 $file2 $file3  $TXTfiles $TXTfiles0);

print "end ftpSAN :  bye\n";
}	# end ftpSAN 

print "\n\nEND bRftpSAN.pl\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);


