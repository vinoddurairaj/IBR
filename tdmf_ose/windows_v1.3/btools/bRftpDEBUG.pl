#!/usr/local/bin/perl -w
###################################
#################################
#
#  ./bRftpDEBUG.pl
#
#	ftp DEBUG deliverables to Sunnyvale SAN server
#
#	\\\\129.212.65.20\\builds\\Replicator\\Windows\\$RFWver\\$Bbnum
#
#	ftp \\builds\\Replicator\\Windows\\$RFWver\\$Bbnum
#
#################################
###################################

require "bRglobalvars.pm";
$RFWver="$RFWver";
$file0="$RFWver.$Bbnum.DEBUG.zip";
#$file1="$RFWver.$Bbnum.RFWexe.zip";
#$file2="$RFWver.$Bbnum.iso";
#$file3="$RFWver.$Bbnum.StoneFly.iso";
$TXTfiles="parseDEBUG.txt DEBUGbuild.txt R*D*.txt";  # can not use b*.txt  =>> sends password file to server
$TXTfiles0="NTDebug.txt W2KDebug.txt";  # can not use b*.txt  =>> sends password file to server

open (STDOUT, ">ftpDEBUG.txt");
open (STDERR, ">>&STDOUT");

print "\n\nSTART bRftpDEBUG.pl\n\n";

chdir "..\\..\\";

system qq($cd);
system qq($cd);

# setup var to cd toooo.

######################################
####################################
# ftp to SAN Server  :  Sunnyvale Build Server
####################################
######################################

ftpDEBUG();

sub ftpDEBUG {

system qq(touch bmachine.txt);
system qq(xcopy /K /V /Y windows_v1.3\\btools\\bmachine.txt);

system qq(touch $RFWver.$Bbnum.DEBUG.txt);
system qq(xcopy /K /V /Y windows_v1.3\\btools\\$RFWver.$Bbnum.DEBUG.txt);

system qq(touch DEBUGbuild.txt);
system qq(xcopy /K /V /Y windows_v1.3\\btools\\DEBUGbuild.txt);

system qq(touch parseDEBUG.txt);
system qq(xcopy /K /V /Y windows_v1.3\\btools\\parseDEBUG.txt);

system qq(touch NTDebug.txt);
system qq(xcopy /K /V /Y windows_v1.3\\NTDebug.txt);

system qq(touch W2KDebug.txt);
system qq(xcopy /K /V /Y windows_v1.3\\W2KDebug.txt);

#######################################
#####################################
# ncftput  :  required deliverables
#######################################
#######################################

system qq(ncftpput -f bmachine.txt -m /builds/Replicator/Windows/$RFWver/$Bbnum $file0 $TXTfiles $TXTfiles0 );

print "\n\nend ftpDEBUG :  bye\n\n";

}	# end ftpSAN 

print "\n\nEND bRftpSAN.pl\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);


