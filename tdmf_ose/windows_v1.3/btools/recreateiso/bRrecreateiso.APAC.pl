#!/usr/bin/local/perl -w
###################################
#################################
# 
#  perl -w -I.. /bRrecreateiso.APAC.pl
#
#	TDMF APAC rebrand image
#
#	recreate QA'ed iso image with refreshed 
#	(final  ????) docs
#
#	usage  :
#	perl -w -I.. bRrecreateiso.APAC.pl -u
#
#################################
###################################

###################################
#################################
#   required inputs 
#	bRrecreateiso.APAC.pl -b -i -t
#
#	All Mandatory
#
#	-b	buildnumber  :  must be B#####
#
#	-i	ISO image revision  :  $RFWver.$Bbnum.#.TDMF.iso
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
   print "\n\ninitial checks passed  :  continue recreate TDMF APAC iso  :  Bbnum  :  $Bbnum  :  iso revision level  :  $isorev\n";
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

open (STDOUT, ">recreateiso.APAC.txt");
open (STDERR, ">>&STDOUT");

print "\n\nSTART  :  bRrecreateiso.APAC.pl\n\n";

###################################
#################################
# grab $RFWver.$Bbnum.ALL.TDMF.zip :  from build server
#################################
###################################

system qq(ncftpget -f ..\\bmachine.txt . /builds/Replicator/Windows/$RFWver/$Bbnum/$RFWver.$Bbnum.ALL.TDMF.zip);
print "\n\nncftpget  :  $RFWver.$Bbnum.ALL.TDMF.zip  :  rc = $?\n\n";

if (-e "$RFWver.$Bbnum.ALL.TDMF.zip") {
   print "\n\n$RFWver.$Bbnum.ALL.TDMF.zip exists  :  continuing recreate iso process\n\n";
}
else  {
   print "\n\n$RFWver.$Bbnum.ALL.TDMF.zip does not exist  :  exiting (8)\n\n";
   exit(8);
}

###################################
#################################
# copy FINAL docs locally into dir structure  
#################################
###################################

#  We only deliver Help Files with TDMF APAC rebrand
if (-d "Documentation" ) {print "\nDocumentation exists\n"; system qq (del /F /Q Documentation\\*);} 
else {mkdir "Documentation";}
if (-d "Help Files" ) {print "\nHelp Files exists\n";system qq (del /F /Q "Help Files\\*");} 
else {mkdir "Help Files";}

system qq(xcopy /K /V /Y \\TechPubs\\$RFWver\\APACDocumentation\\*.pdf Documentation);
print "\n\nxcopy Documentation  :  rc = $?\n";

system qq(xcopy /K /V /Y \\TechPubs\\$RFWver\\APACHelpFiles\\*.chm "Help Files");
print "\n\nxcopy Help Files  :  rc = $?\n";

###################################
#################################
# delete BAD .pdf and .chm (or html) docs from  :  $RFWver.$Bbnum.ALL.TDMF.zip
#################################
###################################

system qq(pkzip25 -del $RFWver.$Bbnum.ALL.TDMF.zip *.pdf);
print "\n\npkzip25 -del  Documentation\\*.pdf  :  rc = $?\n";

system qq(pkzip25 -del $RFWver.$Bbnum.ALL.TDMF.zip *.chm);
print "\n\npkzip25 -del  Help Files\\*.chm  :  rc = $?\n";

###################################
#################################
# update docs into  :  $RFWver.$Bbnum.ALL.TDMF.zip
#################################
###################################

system qq(pkzip25.exe -add -directories -exclude=CVS* $RFWver.$Bbnum.ALL.TDMF.zip Documentation\\*.pdf);
print "\n\npkzip25 -add  Documentation\\*.pdf  :  rc = $?\n";

system qq(pkzip25.exe -add -directories -exclude=CVS* $RFWver.$Bbnum.ALL.TDMF.zip "Help Files\\*.chm");
print "\n\npkzip25 -add  Help Files\\*.chm  :  rc = $?\n";

###############################
#############################
#  rename iso to supplied -i revision  ( $isorev )  ....
#############################
###############################

rename "$RFWver.$Bbnum.ALL.TDMF.zip", "$RFWver.$Bbnum.ALL.TDMF.$isorev.zip";

###############################
#############################
#  recreate new iso  ....
#############################
###############################

#  ncftpput  :  image to iso build machine 

system qq(ncftpput -f bmachineRiso.txt -m /u01/bmachine/iso/recreate/RFW/TDMF $RFWver.$Bbnum.ALL.TDMF.$isorev.zip);
print "\n\nncftpput  :  recreate iso  :  rc = $?\n";

#  telnet

#telnet iso vars  ==>> leave here so they are defined....
$workarea="/u01/bmachine/iso/recreate/RFW/TDMF";
$userid="bmachine";
$passwd="moose2";
$mdi="mkdir image";
$cdi="cd image";
$UZ="unzip ../$RFWver.$Bbnum.ALL.TDMF.$isorev.zip";
$cdd="cd ..";
$CDLABEL="$RFWver-$Bbnum.$isorev-OEM";
$UID="-uid 0 -gid 0 -file-mode 555";    # root and 1 setting for os gid  r-x on executables
$op0="-f -hide-rr-moved -J -R -hfs -max-iso9660-filename";
$op1="-L -o $RFWver.$Bbnum.$isorev.TDMF.iso -U -v -V $CDLABEL $UID image";
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
    print "\n\nbRrecreateiso.APAC.pl :  errors : rc = $@\n\n";
    undef ($@);
}
print "\n\n";
#  ncftpget iso

system qq(ncftpget -f bmachineRiso.txt . /u01/bmachine/iso/recreate/RFW/TDMF/$RFWver.$Bbnum.$isorev.TDMF.iso);
print "\n\nncftpget  :  iso build server  :  rc = $?\n";

#  ncftpput  :  image to iso build machine 
system qq(ncftpput -f ..\\bmachine.txt /builds/Replicator/Windows/$RFWver/$Bbnum $RFWver.$Bbnum.$isorev.TDMF.iso);
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
print FHRECREATE "\niso image  :  $RFWver.$Bbnum.$isorev.TDMF.iso\n";
print FHRECREATE "\n========================================\n";
print FHRECREATE "\nDocumentation / Help Files  :  Updated ....\n";
print FHRECREATE "\n========================================\n";

close (FHRECREATE);

system qq(blat.exe isochangenotification.txt -p iconnection -subject 
	"$RFWver $Bbnum APAC ISO Recreation Notification"
                -to "replicationdev\@softek.com");

########################################
########################################

print "\n\n\nEND  :  bRcreateiso.APAC.pl\n\n\n";

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
bRrecreateiso.APAC.pl usage :

bRrecreateiso.APAC.pl -b -i -u 

-b  :  Build Number to recreate OEM TDMF APAC iso image  
	-b B#####
	Image must be available on build server and in \$RFWver per branch

-i  :   iso revision level   :  \$RFWver.\$Bbnum.\$isorev.TDMF.iso
	-i #
	Revision level anticipated  ==>> must be unique.

-u  :  usage

/docs/\$Product  ==>>  cvs tag source to GA build level source

Documents must be updated to appropriate build machine source locations.
Help Files must be updated to appropriate build machine source locations.

END

exit(8);

}  		#close bracket sub usage
