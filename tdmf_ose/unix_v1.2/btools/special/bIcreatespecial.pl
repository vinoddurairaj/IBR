#!/usr/bin/perl -Ipms
#######################################
#####################################
#
#  perl -w ./bIcreatespecial.pl
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
#  vars  /  pms  /  use
#####################################
#######################################

eval require 'bRglobalvars';

our $Bbnum;
our $TUIPver;
our $PATCHver;

our $previousimage="TUIP252.B02041.2.tar";

our $CDLABEL="$TUIPver-$PATCHver-$Bbnum";

our $UID="-uid 0 -gid 0 -file-mode 555"; 
our $op0="-f -hide-rr-moved -J -R -hfs -max-iso9660-filename";
our $op1="-L -o $TUIPver.$Bbnum.polybin.iso -U -v -V $CDLABEL $UID specialimage";
our $mki="mkisofs $op0 $op1";

#######################################
#####################################
#  previous build image tar archive must reside on local machine
#	use btools/special to hold previous tar image
#	$previousimage above is THAT image
#####################################
#######################################

#######################################
#####################################
#  STDOUT  /  STDERR  #not used  :  captured via calling script
#####################################
#######################################

#open (STDOUT, ">logs/specialimagetuip.txt");
#open (STDERR, ">>&STDOUT");

print "\n\nSTART  :  bIcreatespecial.pl\n\n";

#######################################
#####################################
#  previous build image tar archive must reside on local machine
#	use btools/special to hold previous tar image
#####################################
#######################################

chdir "../../builds/tuip/$Bbnum";

system qq(pwd);

@dirs = qw(specialimage
	specialimage/Softek
	specialimage/Softek/TDMFIP
	specialimage/Softek/TDMFIP/doc
);

foreach $eD (@dirs) {  mkdir "$eD"; }

chdir "specialimage";

system qq(pwd);

#system qq(ls -l ../../../../btools/special/$previousimage);
system qq(tar xvf ../../../../btools/special/$previousimage);
system qq(tar xvf ../$TUIPver.$Bbnum.linux.tar);
system qq(tar xvf ../$TUIPver.$Bbnum.Redist.tar);
system qq(tar xvf ../$TUIPver.$Bbnum.hpux.tar Softek/TDMFIP/HPUX/1131ipf/1131ipf.depot);
exit();
#######################################
#####################################
# docs  :  assume we have latest ????  
#####################################
#######################################

chdir "..";

system qq(pwd);

system qq(sudo cp doc/*.pdf ./specialimage/Softek/TDMFIP/doc);

#######################################
#####################################
#  mkisofs  
#####################################
#######################################

system qq($mki);

#######################################
#######################################

print "\n\nEND  :  bIcreatespecial.pl\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);
