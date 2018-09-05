#!/usr/bin/perl -Ipms
#######################################
#####################################
#
#  perl -w ./bRspecialimage.pl
#
#	create product image of combined build components
#		from current build and previous build. 
#	 
#	Mgmt must specify current components and previous components to 
#		package and deliver
#
#####################################
#######################################
 
#######################################
#####################################
# vars  /  pms  /  use 
#####################################
#######################################

eval 'require bRglobalvars';

$COMver;
$Bbnum;
$RFXbuildtree;
$TUIPbuildtree;
$RFXver;
$TUIPver;

#######################################
#####################################
#  previous build image tar archive must reside on local machine
#	use btools/special to hold previous tar image
#	$previousimage above is THAT image
#####################################
#######################################

#######################################
#####################################
# STDOUT / STDERR 
#####################################
#######################################

open (STDOUT, ">logs/specialimage.txt");
open (STDERR, ">>&STDOUT");

#######################################
#####################################
#  exec statements 
#####################################
#######################################

print "\n\nSTART  :  bRspecialimage.pl\n\n";

#######################################
#####################################
#  log values 
#####################################
#######################################

print "\nCOMver         =  $COMver\n";
print "\nBbnum          =  $Bbnum\n";
print "\nRFXbuildtree   =  $RFXbuildtree\n";
print "\nTUIPbuildtree  =  $TUIPbuildtree\n";
print "\nRFXver         =  $RFXver\n";
print "\nTUIPver        =  $TUIPver\n\n";

#######################################
#####################################
#  previous build image tar archive must reside on local machine
#	use btools/special to hold previous tar image
#####################################
#######################################

chdir "special";

system qq(pwd);

#######################################
#####################################
#  sys call to special/b(I)(R)createspecial.pl
#####################################
#######################################

system qq(perl -I.. bRcreatespecial.pl);
system qq(perl -I.. bIcreatespecial.pl);

#######################################
#######################################

print "\n\nEND  :  bRspecialimage.pl\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);

