#!/usr/bin/local/perl -w
###################################
#################################
# 
#  perl -w -I.. /bRrecreateiso.SF.pl
#
#	StoneFly rebrand image
#
#	recreate QA'ed iso image with refreshed 
#	(final  ????) docs
#
#	usage  :
#	perl -w -I.. bRrecreateiso.SF.pl -u
#
#################################
###################################

###################################
#################################
#   required inputs 
#	bRrecreateiso.SF.pl -b -i -t
#
#	All Mandatory
#
#	-b	buildnumber  :  must be B#####
#
#	-i	ISO image revision  :  $RFWver.$Bbnum.#.SF.iso
#
#	-t	Documentation tag  :  $RFWver+$DATE+$Bbnum+FinalDocs(#)
#				ONLY SUPPLY FinalDocs(#)
#				eg.  :  FinalDocs0
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
$RFWver="$RFWver";

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
   print "\n\ninitial checks passed  :  continue recreate StoneFly iso  :  Bbnum  :  $Bbnum  :  iso revision level  :  $isorev\n";
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

open (STDOUT, ">recreateiso.SF.txt");
open (STDERR, ">>&STDOUT");

print "\n\nSTART  :  bRrecreateiso.SF.pl\n\n";

###################################
#################################
# grab $RFWver.$Bbnum.ALL.SFzip :  from build server
#################################
###################################

system qq(ncftpget -f ..\\bmachine.txt . /builds/Replicator/Windows/$RFWver/$Bbnum/$RFWver.$Bbnum.ALL.SF.zip);
print "\n\nncftpget  :  $RFWver.$Bbnum.ALL.SF.zip  :  rc = $?\n\n";

if (-e "$RFWver.$Bbnum.ALL.SF.zip") {
   print "\n\n$RFWver.$Bbnum.ALL.SF.zip exists  :  continuing recreate iso process\n\n";
}
else  {
   print "\n\n$RFWver.$Bbnum.ALL.SF.zip does not exist  :  exiting (8)\n\n";
   exit(8);
}

###################################
#################################
# copy FINAL docs locally into dir structure  
#################################
###################################

#  We only deliver Help Files with StoneFly brand
#if (-d "Documentation" ) {print "\nDocumentation exists\n"; system qq (del /F /Q Documentation\\*);} 
#else {mkdir "Documentation";}
if (-d "Help Files" ) {print "\nHelp Files exists\n";system qq (del /F /Q "Help Files\\*");} 
else {mkdir "Help Files";}

#system qq(xcopy /K /V /Y \\TechPubs\\$RFWver\\Documentation\\*.pdf Documentation);
#print "\n\nxcopy Documentation  :  rc = $?\n";

system qq(xcopy /K /V /Y \\TechPubs\\$RFWver\\StoneFlyHelpFile\\*.chm "Help Files");
print "\n\nxcopy Help Files  :  rc = $?\n";

###################################
#################################
# delete BAD .pdf and .chm (or html) docs from  :  $RFWver.$Bbnum.ALL.SF.zip
#################################
###################################

#system qq(pkzip25 -del $RFWver.$Bbnum.ALL.SF.zip *.pdf);
#print "\n\npkzip25 -del  Documentation\\*.pdf  :  rc = $?\n";

system qq(pkzip25 -del $RFWver.$Bbnum.ALL.SF.zip *.chm);
print "\n\npkzip25 -del  Help Files\\*.chm  :  rc = $?\n";

###################################
#################################
# update docs into  :  $RFWver.$Bbnum.ALL.SF.zip
#################################
###################################

#system qq(pkzip25.exe -add -directories -exclude=CVS* $RFWver.$Bbnum.ALL.SF.zip Documentation\\*.pdf);
#print "\n\npkzip25 -add  Documentation\\*.pdf  :  rc = $?\n";

system qq(pkzip25.exe -add -directories -exclude=CVS* $RFWver.$Bbnum.ALL.SF.zip "Help Files\\*.chm");
print "\n\npkzip25 -add  Help Files\\*.chm  :  rc = $?\n";

###############################
#############################
#  rename iso to supplied -i revision  ( $isorev )  ....
#############################
###############################

rename "$RFWver.$Bbnum.ALL.SF.zip", "$RFWver.$Bbnum.ALL.SF.$isorev.zip";

###############################
#############################
#  recreate new iso  ....
#############################
###############################

#  ncftpput  :  image to iso build machine 

system qq(ncftpput -f bmachineRiso.txt -m /u01/bmachine/iso/recreate/RFW/SF $RFWver.$Bbnum.ALL.SF.$isorev.zip);
print "\n\nncftpput  :  recreate iso  :  rc = $?\n";

#  telnet

#telnet iso vars  ==>> leave here so they are defined....
$workarea="/u01/bmachine/iso/recreate/RFW/SF";
$userid="bmachine";
$passwd="moose2";
$mdi="mkdir image";
$cdi="cd image";
$UZ="unzip ../$RFWver.$Bbnum.ALL.SF.$isorev.zip";
$cdd="cd ..";
$CDLABEL="$RFWver-$Bbnum.$isorev-OEM";
$UID="-uid 0 -gid 0 -file-mode 555";    # root and 1 setting for os gid  r-x on executables
$op0="-f -hide-rr-moved -J -R -hfs -max-iso9660-filename";
$op1="-L -o $RFWver.$Bbnum.$isorev.StoneFly.iso -U -v -V $CDLABEL $UID image";
$mki="mkisofs $op0 $op1";

eval {

use Net::Telnet ();
   $t = new Net::Telnet (Timeout => 1000,        
   #$t = new Net::Telnet (Timeout => undef,        
   Prompt => '/bmachine8/');  # works with bash & tcsh
   #Prompt => '/\$ $/');  # works with sh
   $t->open("129.212.206.131");
   $t->login($userid, $passwd);
      #@create = $t->cmd( "cd $workarea ; pwd ; ls -la");
      @create = $t->cmd( "mkdir -p $workarea; cd $workarea; $mdi; $cdi; $UZ; $cdd; $mki");
      #print "\ntelnet cmd : rc = $?\n";  # no value to script....
      print @create;

};   #eval close bracket

if ($@) {
    print "\n\nbRrecreateiso.SF.pl :  errors : rc = $@\n\n";
    undef ($@);
}
print "\n\n";
#  ncftpget iso

system qq(ncftpget -f bmachineRiso.txt . /u01/bmachine/iso/recreate/RFW/SF/$RFWver.$Bbnum.$isorev.StoneFly.iso);
print "\n\nncftpget  :  iso build server  :  rc = $?\n";

#  ncftpput  :  image to iso build machine 
system qq(ncftpput -f ..\\bmachine.txt /builds/Replicator/Windows/$RFWver/$Bbnum $RFWver.$Bbnum.$isorev.StoneFly.iso);
print "\n\nncftpput  :  recreate SAN  build server  :  rc = $?\n";

###############################
#############################
#  cleanup iso build machine images ....
#############################
###############################

eval {

use Net::Telnet ();
   $t = new Net::Telnet (Timeout => 500,        
   #$t = new Net::Telnet (Timeout => undef,        
   Prompt => '/bmachine8/');  # works with bash & tcsh
   #Prompt => '/\$ $/');  # works with sh
   $t->open("129.212.206.131");
   $t->login($userid, $passwd);
      #@cleanup = $t->cmd( "cd $workarea ; pwd ; ls -la");
      @cleanup = $t->cmd( "cd $workarea; pwd ; ls -la ; rm *.iso ; rm *.zip ; rm -rf image");
      #print "\ntelnet cmd : rc = $?\n";  # no value to script....
      print @cleanup;

};   #eval close bracket

if ($@) {
   print "\n\ntelnet  :  rm iso build machine errors : rc = $@\n\n";
   undef ($@);
}

###############################
#############################
#  create isochangenotification.txt  :  email to distribution ....
#############################
###############################

unlink "isochangenotification.txt";

open (FHRECREATE,">isochangenotification.txt");

print FHRECREATE "\n========================================\n";
print FHRECREATE "\n$RFWver  :  iso change notification\n";
print FHRECREATE "\n========================================\n";
print FHRECREATE "\nRecreated ISO Image for build level  :  $Bbnum  :  available at  :\n";
print FHRECREATE "\nSunnyvale Build Server  :  129.212.65.20\n";
print FHRECREATE "\ndirectory  :  \\builds\\Replicator\\Windows\\$RFWver\\$Bbnum\n";
print FHRECREATE "\niso image  :  $RFWver.$Bbnum.$isorev.StoneFly.iso\n";
print FHRECREATE "\n========================================\n";
print FHRECREATE "\nDocumentation / Help Files  :  Updated ....\n";
print FHRECREATE "\n========================================\n";

close (FHRECREATE);

system qq(blat.exe isochangenotification.txt -p iconnection -subject 
	"$RFWver $Bbnum StoneFly ISO Recreation Notification"
                -to "replicationdev\@softek.com");

########################################
########################################

print "\n\n\nEND  :  bRcreateiso.SF.pl\n\n\n";

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
bRrecreateiso.SF.pl usage :

bRrecreateiso.SF.pl -b -i -u 

-b  :  Build Number to recreate OEM StoneFly iso image  
	-b B#####
	Image must be available on build server and in \$RFWver per branch

-i  :   iso revision level   :  \$RFWver.\$Bbnum.\$isorev.SF.iso
	-i #
	Revision level anticipated  ==>> must be unique.

-u  :  usage

/docs/\$Product  ==>>  cvs tag source to GA build level source

Help Files must be updated to appropriate build machine source locations.

END

exit(8);

}  		#close bracket sub usage
