#!/usr/bin/local/perl -w
###################################
#################################
# bRMain.pl
#
#	initial script
#	hostname == bmachine12
#
#   to run a build, but first check "if source has changed to determine"
#       to build, or not to build (no source change, no build), 
#       with number incremention, cvs commits etc.,
#       perl -w bRMain.pl -s [ -f ]
#
#   usage statement 
#	perl -w bRMain.pl -u
#
#################################
###################################

###################################
#################################
# global var file !
#################################
###################################

require "bRglobalvars.pm";
$branchdir="$branchdir"; 
$RFWver="$RFWver";       

#################################
###################################

print "\nSTART :  \tbRMain.pl\n\n";

####################################
##################################
#
# if -f : force build to run, even if source changes are NOT found
#		need to create change list during manually run builds.....
#
##################################
####################################

use Getopt::Long;

my %options;
GetOptions (\%options, 
  'sourcechange',	          	# -s or -sourcechange # check if source changes occurred
   'force',			# -f or -force # launch rsh touch command
  'usage',			# usage explanation
);

&usage if $options{usage}; # if -u supplied, will egress prog after usage flash

###################################
#################################
# set $realbuild var to 1 if defined, required to enter if condition
#################################
###################################

my $sourcechange = $options{sourcechange}	? 1 : '';      #COND ? THEN : ELSE
my $force = $options{force}	? 1 : '';  		#COND ? THEN : ELSE
					   	#pg 105-6

###################################
#################################
# grab date array : used to exit build :
#	 on this date, no build run.....
#################################
###################################

$datepackage='bDATE';

eval "require $datepackage";

@datevar=get_date();

print "\nDATE = $datevar[1]/$datevar[0]/$datevar[2]\n\n";

###################################
#################################
# run bRrdiff.pl, if source change occurred since last tag setting
#     run build, if no changes, exit.....
#################################
###################################

open (STDOUT, ">BUILDMAIN.txt");
open (STDERR, ">>BUILDMAIN.txt");

print "\n\nStart  :  bRMain.pl  :  LOG   :  BUILD.txt\n\n";

###############################
#############################
# cvs update : local dir btools only
#############################
###############################

system qq(cvs update);

###############################
#############################
# run bRrdiff.pl ( -f if supplied via command line....)
#############################
###############################

if ($sourcechange && $force ) {     # force build, no exit
				# and pass -f to bRrdiff.pl (to force)
system qq (perl bRrdiff.pl -f);

   if ($? == "0" ) {
      print FHX0 "\nbRrdiff.pl returned  : \trc = $?\n";
      print FHX0 "\n\nrunning the build .....\n\n";
   }
   else {
      print FHX0 "\nbRrdiff.pl returned = : \trc = $?\n";
      print FHX0 "\nbRrdiff.pl with -f should NEVER be in this else\n";
      print FHX0 "\nbRrdiff.pl error\n";
      print FHX0 "\nno exit :  continuing with build\n";
   }
}    #if close bracket

elsif ($sourcechange ) {	# default for nightly build

system qq (perl bRrdiff.pl);

   if ($? == "0" ) {
      print "\nbRrdiff.pl returned  : \trc = $?\n";
      print "\n\nrunning the build .....\n\n";
   }
   else {
      print FHX0 "\nbRrdiff.pl returned = : \trc = $?\n";
      $DATEFORMAT = "$datevar[1]/$datevar[0]/$datevar[2]";
      system qq( blat.exe sourcechangelist.txt -p iconnection -subject "$RFWver  :  no source change  :  no build  :  $DATEFORMAT"
		-to weplicatordev\@softek.com
		-cc "jdoll\@softek.com");

      print "\n\nNO build ..... NO source changes .....\n\n";
      exit 0;
   }
}    #elsif close bracket

else { 
      print  "\nbRrdiff.pl -s and / or -f args not supplied, error :  no build\n";
      exit 1;
}

###################################
#################################
#   build scripts :  required to build Replicator....
#################################
###################################

if ("$buildmachinename" =~ m/$ENV{COMPUTERNAME}/ ) {

      print "\nhostname matched \=\~ $ENV{COMPUTERNAME}\n";
      print "\n$buildmachinename\n";

open (STDOUT, ">BUILDMAIN.txt");
open (STDERR, ">>BUILDMAIN.txt");

      system qq(bRcvsupdate.pl);	# overwrites build log
          print "\nbRcvsupdate.pl rc = $?\n";

      system qq(bRbuildnuminc.pl );	#sound decision
          print "\nbRbuildnuminc.pl rc = $?\n";

     system qq(bRcvstag.pl);		# capture source level with tag first
          print "\nbRcvstag.pl rc = $?\n";

    system qq(bRcheckoutTAG.pl);	#checkout to builds\\$Bbnum
          print "\nbRcheckoutTAG.pl rc = $?\n";

#      system qq(bRtextpipe.pl);	#var subs to tree builds\\$Bbnum
          print "\nbRtextpipe.pl rc = $?\n";

#      system qq(bRsub_rc.pl);		#var subs  *.rc to tree builds\\$Bbnum
          print "\nbRsub_rc.pl rc = $?\n";

      system qq(bRMainTag.pl);	#run from builds\\$Bbnum
          print "\nbRMainTag.pl rc = $?\n";

      system qq(bRdelpreviousbuild.pl);		#chdir builds\\$Bbnum
          print "\nbRdelpreviousbuilds.pl rc = $?\n";

      system qq(bRcapture_env_2k.pl);
          print "\nbRcapture_env_2k.pl rc = $?\n";

      system qq(bRcvscommit.pl);
          print "\nbRcvscommit.pl rc = $?\n";

      exit (8);

close (STDERR);
close (STDOUT);


}

else {
      $subject="$ENV{COMPUTERNAME}  \:  launch of bRMain.pl failed";
      system qq(touch wrong.machine);
      system qq(blat.exe wrong.machine -p iconnection -subject "$subject" -to "jdoll\@softek.com");
      unlink "wrong.machine";
}

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
bRMain.pl usage :

bRMain.pl -s [-f] -u 

-s : check for source change to determine to build or not to build.

-f : force build to run, even if source changes are NOT found
	need to create change list during manually run builds.....

-u :  usage

END

exit(8);

}  		#close bracket sub usage
