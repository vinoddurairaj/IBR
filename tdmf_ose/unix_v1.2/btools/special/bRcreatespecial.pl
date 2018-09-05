#!/usr/bin/perl -Ipms
#######################################
#####################################
#
#  perl -w ./bRcreatespecial.pl
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
#  vars   /  pms  / use
#####################################
#######################################

eval 'require bRglobalvars';

our $Bbnum;
our $RFXver;
our $PATCHver;

our $previousimage="RFX252.B06046.1.tar";

our $CDLABEL="$RFXver-$PATCHver-$Bbnum";

our $UID="-uid 0 -gid 0 -file-mode 555"; 
our $op0="-f -hide-rr-moved -J -R -hfs -max-iso9660-filename";
our $op1="-L -o $RFXver.$Bbnum.polybin.iso -U -v -V $CDLABEL $UID specialimage";
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
#  STDOUT  /  STDERR  #not used  :  capture via calling script 
#####################################
#######################################

#open (STDOUT, ">logsspecialimage.txt");
#open (STDERR, ">>&STDOUT");

print "\n\nSTART  :  bRcreatespecial.pl\n\n";

#######################################
#####################################
#  previous build image tar archive must reside on local machine
#	use btools/special to hold previous tar image
#####################################
#######################################

chdir "../../builds/$Bbnum";

system qq(pwd);

@dirs = qw(specialimage
	specialimage/Softek
	specialimage/Softek/Replicator
	specialimage/Softek/Replicator/doc
);

foreach $eD (@dirs) {  mkdir "$eD"; }

chdir "specialimage";

system qq(pwd);

#RFX 253  :  rebuild linux only  :  all others are RFX 252 B06046
#system qq(ls -l ../../../btools/special/$previousimage);
system qq(tar xvf ../../../btools/special/$previousimage);
system qq(tar xvf ../$RFXver.$Bbnum.linux.tar);
system qq(tar xvf ../$RFXver.$Bbnum.Redist.tar);
system qq(tar xvf ../$RFXver.$Bbnum.hpux.tar Softek/Replicator/HPUX/1131ipf/1131ipf.depot);

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

print "\n\nEND  :  bRcreatespecial.pl\n\n";

close (STDERR);
close (STDOUT);
