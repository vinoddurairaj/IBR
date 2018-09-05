#!/usr/bin/perl -w
######################################
####################################
#
# ./bRpkzip_ALL.pl 
#
#	create $RFWver.$Bbnum.ALL.zip image
#
#	OEM images  :
#	create $RFWver.$Bbnum.ALL.SF.zip image
#
####################################
######################################

require "bRglobalvars.pm";
$Bbnum="$Bbnum";
$RFWver="$RFWver";
$dirs7="$dirs7";
$dirs8="$dirs8";
$dirs11="$dirs11";


####################################
#####################################

####################################
##################################
# grab STDERR, STDOUT
##################################
####################################

open (STDOUT, ">>$RFWver.$Bbnum.txt");
open (STDERR, "\&STDOUT");

print "\n\nSTART  :  bRpkzip_ALL.pl\n\n\n";

system qq($cd);

chdir "..\\ftdsrc\\Installshield\\TDMF_OSE\\Media\\New CD-ROM Media\\Disk Images\\Disk1" || die "bRpkzip_ALL.zip  :  errors(1)";

system qq($cd);

####################################
##################################
# copy  :  docs / help files 
##################################
####################################

mkdir "Documentation";
mkdir "Help Files";

system qq(xcopy /K /V /Y \\TechPubs\\$RFWver\\Documentation\\*.pdf Documentation);
system qq(xcopy /K /V /Y "\\TechPubs\\$RFWver\\Help Files\\*.chm" "Help Files");

####################################
##################################
# pkzip25 ALL  :  installshield installation deliverables 
##################################
####################################

system qq(pkzip25.exe -add -directories -exclude=CVS* $dirs8\\$RFWver.$Bbnum.ALL.zip *);

print "\n\npkzip25.exe  :  ALL .zip  :  install deliverables rc = $?\n\n"; 

####################################
##################################
# OEM IMAGES   
##################################
####################################

chdir "$dirs8";

system qq($cd);

chdir "OEM\\StoneFly\\tdmf_ose\\windows_v1.3\\ftdsrc\\Installshield\\TDMF_OSE\\Media\\New CD-ROM Media\\Disk Images\\Disk1" || die "bRpkzip_ALL.zip  :  errors(StoneFly)";

system qq($cd);

#copy only Help Files  :  docs not delivered to OEM StoneFly

mkdir "Help Files";
system qq(xcopy /K /V /Y "\\TechPubs\\$RFWver\\Help Files\\*.chm" "Help Files");

system qq(pkzip25.exe -add -directories -exclude=CVS* $dirs11\\$RFWver.$Bbnum.ALL.SF.zip *);

print "\n\npkzip25.exe  :  ALL .zip  :  OEM install deliverables  :  StoneFly rc = $?\n\n"; 

print "\n\n\tEND \tscript bRpkzip_ALL.pl\n\n";

####################################
####################################

close (STDERR);
close (STDOUT);
