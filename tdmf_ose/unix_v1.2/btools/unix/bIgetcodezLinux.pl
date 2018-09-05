#!/usr/bin/perl -I../pms -I..
###############################################
#############################################
#
#  perl -w -I".." ./bIgetcodezLinux.pl
#
#    zLinux script only  :  CVSROOT issue
#      strictly for Raleigh work around
#
#    cvs update from ../ to get bRglobalvars.pm & bRcvstag.pm
#
#    mkdir tdmf_ose/unix_v1.2/tuip/$Bbnum[/MACHINElevel]
#
#    chdir tdmf_ose/unix_v1.2/tuip/builds/$Bbnum[/MACHINElevel]
#
#    cvs checkout -r $CVSTAG
#
#    build directory should be set for rest of project
#
#############################################
###############################################

###############################################
#############################################
#  Add paths to $ENV{PATH} as required  :  workaround, update .profile in $HOME
#############################################
###############################################

#$ENV{PATH}="$ENV{PATH}:/usr/local/bin:.";

###############################################
#############################################
#  vars  /  pms  /  use
#############################################
###############################################

eval 'require bRglobalvars';
eval 'require bRcvstag';

our $Bbnum;
our $builds="builds";
#our $CVSROOT=':pserver:bmachine@9.29.86.20:/cvs2/sunnyvale';
our $CVSROOT=':pserver:bmachine@bmachine0.sanjose.ibm.com:/cvs2/sunnyvale';
chomp ($CVSTAG);
chomp ($CVSROOT);

###############################################
#############################################
#  exec statements
#############################################
###############################################

print "\n\nSTART  :   bIgetcodezLinux.pl\n\n"; 

###############################################
#############################################
#  log values
#############################################
###############################################

print "\n\nTUIP :  build level $Bbnum\n\n";

print "\n\nTUIP :  hostname  =   $ENV{HOSTNAME}\n\n";

print "\n\nCVSTAG  =  $CVSTAG\n\n";

print "\nCVSROOT = $CVSROOT\n";

###############################################
#############################################
# get to root of tdmf_ose project directory
##############################################
################################################

chdir "../..";

system qq(pwd);

if ( ! -d "$builds" ) {
    print "\nIF :  mkdir $builds\n";
    system qq(mkdir "$builds");
}

chdir "builds";

system qq(pwd);

system qq(mkdir "tuip");

chdir "tuip";

system qq(pwd);

system qq(mkdir "$Bbnum");

chdir "$Bbnum";

system qq(pwd);

#################################################
###############################################
#  determine machine  :  create extra build dir
#    except tdmrb01
###############################################
#################################################

#if ($ENV{HOSTNAME} =~ m/tdmrb01/ ) {
#   print "\ntdmrb01  :  no extra subdir created\n";
#}
#elsif ($ENV{HOSTNAME} =~ m/tdmrb02/ ) {
#   print "\nRedHat5x  :  mkdir tdmrb02\n";
#   mkdir "RedHat5x";
#   chdir "RedHat5x";
#   system qq(pwd)
#}

#################################################
###############################################
# cvs checkout tag  (set via windows build) :  in bRcvstag.pm
###############################################
#################################################

system qq(cvs -d $CVSROOT export -r $CVSTAG tdmf_ose/unix_v1.2 tdmf_ose/btools );

print "\n\ncvs checkout -r $CVSTAG rc = $? \n\n";

#################################################
#################################################

print "\n\nEND  :  bIgetcodezLinux.pl\n\n"; 

#################################################
#################################################

close (STDERR);
close (STDOUT);

