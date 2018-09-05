#!/usr/bin/local/perl -w
###################################
#################################
# 
#  perl -w -I.. /bRrecreateiso.pl
#
#	recreate QA'ed iso image with refreshed 
#	(final  ????) docs
#
#	usage  :
#	perl -w -I.. bRrecreateiso.pl -u
#
#################################
###################################

###################################
#################################
#   required inputs 
#	bRrecreateiso.pl -b -i -t
#
#	All Mandatory
#
#	-b	buildnumber  :  must be B#####
#
#	-i	ISO image revision  :  $RFXver.$Bbnum.#.iso
#
#	-t	Documentation tag  :  $RFXer+$DATE+$Bbnum+FinalDocs(#)
#				ONLY SUPPLY FinalDocs(#)
#				eg.  :  FinalDocs0
#				not implemented
#
#################################
###################################

###################################
#################################
# global var files !
#################################
###################################

require "bRglobalvars.pm";
$branchdir="$branchdir"; 
$RFXver="$RFXver";

####################################
##################################
#
#  check that options are supplied
#
##################################
####################################

my $Bbnum;
my $isorev;

use Getopt::Long;

GetOptions(
   "bbnum=s" => \$Bbnum,
   "isorev=i" => \$isorev,
   "usage"     => sub {usage()}
);

#BREAKDOWN  :     "Bbnum=i" => \$Bbnum  :  i = INTEGER
#BREAKDOWN  :     "tag=s" => \$tag  :  s = STRING
#  "tag=s" => \$tag  # Not used, but, if it was, here is the =s"=> as example  :  "tag=s" => \$tag  :  need a my $tag = ""; too
#  $bbnum is a string as we start the "number" with B!

####################################
##################################
#
#  check that variable have values ||  ==>>  flash usage()  exit();
#
##################################
####################################

if (defined ($Bbnum) && defined ($isorev) ) {
   print "\n\ninitial checks passed  :  continue recreate iso  :  Bbnum  :  $Bbnum  :  iso revision level  :  $isorev\n";
}
else {
   print "\n\nexit  :  usage displayed  :  must supply -b B##### and -i #\n\n";
   usage();
}

###################################
#################################
# STDOUT  :  STDERR
#################################
###################################

open (STDOUT, ">recreateiso.txt");
open (STDERR, ">>&STDOUT");

print "\n\nSTART  :  bRrecreateiso.pl\n\n";

###################################
#################################
# grab $RFXver.$Bbnum.ALL.tar :  from build server
#################################
###################################

system qq(ncftpget -f ../bmachine.txt . /builds/Replicator/UNIX/$RFXver/$Bbnum/$RFXver.$Bbnum.ALL.tar);
print "\n\nncftpget  :  $RFXver.$Bbnum.ALL.tar  :  rc = $?\n\n";

if (-e "$RFXver.$Bbnum.ALL.tar") {
   print "\n\n$RFXver.$Bbnum.ALL.tar exists  :  continuing recreate iso process\n\n";
}
else  {
   print "\n\n$RFXver.$Bbnum.ALL.tar does not exist  :  exiting (8)\n\n";
   exit(8);
}

###################################
#################################
# unlink $RFXver.$Bbnum.ALL.tar 
#################################
###################################

system qq(tar xvf $RFXver.$Bbnum.ALL.tar);

###################################
#################################
# copy FINAL docs locally into dir structure  
#################################
###################################

if (-d "Softek/Replicator/doc" ) {print "\ndoc exists\n"; system qq(sudo rm Softek/Replicator/doc/*);} 
else {mkdir -p "Softek/Replicator/doc";}

system qq(sudo cp -p ../../doc/*.pdf Softek/Replicator/doc);
print "\n\ncp *.pdf doc  :  rc = $?\n";

###################################
#################################
# unlink $RFXver.$Bbnum.ALL.tar 
#################################
###################################

unlink "$RFXver.$Bbnum.ALL.tar";

###################################
#################################
# create refreshed (new docs included) *ALL.tar archive
#################################
###################################

system qq(tar cvf $RFXver.$Bbnum.ALL.$isorev.tar Softek/* );

###############################
#############################
# extract new tar archive to image directory....
#############################
###############################

mkdir "image";

chdir "image";

system qq(pwd);

system qq(tar xvf ../$RFXver.$Bbnum.ALL.$isorev.tar);

chdir "..";

###############################
#############################
#  recreate new iso  ....
#############################
###############################

###############################
#iso vars  :   straight from ../iso/bRcreateiso.pl
###############################

$CDLABEL="$RFXver-$PATCHver-$Bbnum";
$UID="-uid 0 -gid 0 -file-mode 555";    # root and 1 setting for os gid  r-x on executables
$op0="-f -hide-rr-moved -J -R -hfs -max-iso9660-filename";
$op1="-L -o $RFXver.$Bbnum.$isorev.iso -U -v -V $CDLABEL $UID image";
$mki="mkisofs $op0 $op1";
$RFXver="$RFXver";
$PATCHver="$PATCHver";

system qq($mki);

###############################
#############################
#  create isochangenotification.txt  :  email to distribution ....
#############################
###############################

unlink "isochangenotification.txt";

open (FHRECREATE,">isochangenotification.txt");

print FHRECREATE "\n========================================\n";
print FHRECREATE "\n$RFXver  :  iso change notification\n";
print FHRECREATE "\n========================================\n";
print FHRECREATE "\nRecreated ISO Image for build level  :  $Bbnum  :  available at  :\n";
print FHRECREATE "\nSunnyvale Build Server  :  dmsbuilds.sanjose.ibm.com\n";
print FHRECREATE "\ndirectory  :  /builds/Replicator/UNIX/$RFXver/$Bbnum\n";
print FHRECREATE "\niso image  :  $RFXver.$Bbnum.$isorev.iso\n";
print FHRECREATE "\n========================================\n";
print FHRECREATE "\nDocumentation / Help Files  :  Updated ....\n";
print FHRECREATE "\n========================================\n";

close (FHRECREATE);

###############################
#############################
#  ncftp deliverables to build server 
#############################
###############################

system qq(ncftpput -f ../bmachine.txt /builds/Replicator/UNIX/$RFXver/$Bbnum $RFXver.$Bbnum.$isorev.iso isochangenotification.txt);
print "\n\nncftpput  :  recreate SAN  build server  :  rc = $?\n";

###############################
#############################
#  email isochangenotification.txt
#############################
###############################

open (FHemail,"<isochangenotification.txt");
@emailnotification=<FHemail>;
close (FHemail);

use Net::SMTP;
   $ENV{BUILDid}="jdoll\@softek.com";
   $ENV{BUILD}="$RFXver.$Bbnum";
   $smtp = Net::SMTP->new('69.4.4.176');
   $smtp->mail($ENV{BUILDid});
#   $smtp->to('jdoll@softek.com');
   $smtp->to('replicationdev@softek.com','jdoll@softek.com');
   $smtp->data();
   $smtp->datasend("Subject:  $RFXver $Bbnum Softek ISO Recreation Notification\n");  #<<== must have end of line "\n" or everything ends up in subject line
   $smtp->datasend("@emailnotification\n");
   $smtp->dataend();
$smtp->quit;

###############################
#############################
#  create isochangenotification.txt  :  email to distribution ....
#############################
###############################

system qq(rm -rf ./Softek/ ./image/);

print "\n\nrm -rf Softek/ image/  :  rc = $?\n\n";

########################################
########################################

print "\n\n\nEND  :  bRcreateiso.pl\n\n\n";

close (STDERR);
close (STDOUT);

#####################################
###################################
#
# sub routines
#
###################################
#####################################

sub usage {

#print "\nin usage sub\n";

print <<END;
bRrecreateiso.pl usage :

bRrecreateiso.pl -b -i -u 

-b  :  Build Number to recreate iso image  
	-b B#####
	Image must be available on build server and in \$RFXver per branch

-i  :   iso revision level   :  \$RFXver.\$Bbnum.\$isorev.iso
	-i #
	Revision level anticipated  ==>> must be unique.

-u  :  usage

END

exit(8);

}  		#close bracket sub usage
