#!/usr/bin/local/perl -w
###################################
#################################
# bRMain.pl
#
#	initial script
#	determine if real build
#	is -r provided && hostname == bmachine6
#
#   to run a build, but first check "if source has changed to determine"
#       to build, or not to build (no source change, no build), 
#       with number incremention, cvs commits etc.,
#       perl -w bRMain.pl 
#
#   to force a  build, full, with number incremention, cvs commits etc.,
#       perl -w bRMain.pl -f
#
#################################
###################################

###################################
#################################
# global var file !
#################################
###################################

require "bRglobalvars.pm";
$branchdir="$branchdir"; # stop occasional annoying warning message
$RFUver="$RFUver";        # same issue

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
   'force',			# -f or -force # launch rsh touch command
);

#&usage if $options{usage}; # if -u supplied, will egress prog after usage flash

###################################
#################################
# set $realbuild var to 1 if defined, required to enter if condition
#################################
###################################

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

open (FHX0, ">RFUBUILD.txt");

###############################
# cvs update : local dir btools only : assurance our scripts are up-to-date
###############################

system qq(cvs update);

###############################
# run bRrdiff.pl ( -f if supplied via command line....)
###############################

if ($force ) {     			# force build, no exit
				# and pass -f to bRrdiff.pl (to force)
   system qq (perl -w bRrdiff.pl -f);

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

else {	# default for nightly build  #check for source changes :  no change : no build

   system qq (perl -w bRrdiff.pl);

      if ($? == "0" ) {
         print FHX0 "\nbRrdiff.pl returned  : \trc = $?\n";
         print FHX0 "\n\nrunning the build .....\n\n";
      }
      else {
         print FHX0 "\nbRrdiff.pl returned = : \trc = $?\n";
         $DATEFORMAT = "$datevar[1]/$datevar[0]/$datevar[2]";
         system qq( blat.exe sourcechangelist.txt -p exchange -subject "$RFUver : no build tonight : $DATEFORMAT"
			-to jdd00\@softek.fujitsu.com
			-cc "makebono\@softek.fujitsu.com,
			jdd00\@softek.fujitsu.com");

         print FHX0 "\n\nNO build ..... NO source changes .....\n\n";
         exit 0;
      }
}    #else close bracket

#else { 
#      print FHX0 "\nbRrdiff.pl  :  -f args not supplied, error :  no build\n";
#      exit 1;
#}

close (FHX0);

###################################
#################################
#   build scripts :  required to build tdmf_ose....
#################################
###################################

if ("$buildmachinename" =~ m/$ENV{COMPUTERNAME}/ ) {

   print "\ncomputer name matched \=\~ $ENV{COMPUTERNAME}\n";
   print "\n$buildmachinename\n";

   #open (STDOUT, ">BUILDMAIN.txt");
   #open (STDERR, ">>BUILDMAIN.txt");

      system qq(bRbuildnuminc.pl -b);	#questionable decision
          print "\nbRbuildnuminc.pl rc = $?\n";

      system qq(bRcvstag.pl);		# capture source level with tag first
          print "\nbRcvstag.pl rc = $?\n";

      system qq(bRdirs.pl);		# capture source level with tag first
          print "\nbRdirs.pl rc = $?\n";

      system qq(bRMainUnix.pl);	#checkout to builds\\$Bbnum
          print "\nbRMainUnix.pl rc = $?\n";

      system qq(bRftppkg.pl);	#var subs to tree builds\\$Bbnum
          print "\nbRftppkg.pl rc = $?\n";

#      system qq(bRMainTag.pl);	#run from builds\\$Bbnum
#          print "\nbRMainTag.pl rc = $?\n";

#      system qq(bRcapture_env_2k.pl);
#          print "\nbRcapture_env_2k.pl rc = $?\n";

#      system qq(bRcvscommit.pl);
#          print "\nbRcvscommit.pl rc = $?\n";

      exit (8);

#close (STDERR);
#close (STDOUT);


}

else {
      $subject="$ENV{COMPUTERNAME}  \:  launch of bRMain.pl failed";
      system qq(touch wrong.machine);
      system qq(blat.exe wrong.machine -p exchange -subject "$subject" -to "jdoll\@softek.fujitsu.com");
      unlink "wrong.machine";
}

#####################################
###################################
#
# sub routines
#
###################################
#####################################

#sub usage {

#print "\nin usage sub\n";

#print <<END;
#bRMain.pl usage :

#bRMain.pl -f 

#-f : force build to run, even if source changes are NOT found
#	need to create change list during manually run builds.....

#-u :  usage

#END

#exit(8);

#}  		#close bracket sub usage
