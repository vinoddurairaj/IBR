#!/usr/bin/perl -w
##################################################
################################################
#
#  perl -w ./bRzmachineStatus.pl
#
#    non-robust effort to possibly take the surprise
#    element out of this production machine going
#    offline / online without notification to our teams
################################################
##################################################

##################################################
################################################
#  vars  /  pms  /  use
################################################
##################################################

our @dfResults;
our @STATUS;

use Net::SMTP;

our $U34="/srcpool/tdmf/bldenv/s390x/lib/modules/2.6.9-34.EL/build/.config";
our $U42="/srcpool/tdmf/bldenv/s390x/lib/modules/2.6.9-42.EL/build/.config";
our $U55="/srcpool/tdmf/bldenv/s390x/lib/modules/2.6.9-55.EL/build/.config";
our $U67="/srcpool/tdmf/bldenv/s390x/lib/modules/2.6.9-67.EL/build/.config";

$ENV{SUBJECT}="";
$ENV{BUILDid}="jdoll\@us.ibm.com";
our $accept="0";
our $fault="0";

##################################################
################################################
#  STDOUT  /  STDERR
################################################
##################################################

open (STDOUT,">zmachinestatus.txt");
open (STDERR,">&STDOUT");

##################################################
################################################
#  initial machine status report inputs
################################################
##################################################

$currentD=`date`;

push (@STATUS,"\ntdmrb01.rtp.raleigh.ibm.com machine status report\n");
push (@STATUS,"\n$currentD\n");

print "\n\n$currentD\n\n";

##################################################
################################################
#   observations as results of df -k
#    gather results to array list
################################################
##################################################

@dfResults=`df -k`;

print "\nlength dfResults  =  $#dfResults\n";

#print "\ndfResults array   =\n\n@dfResults\n";

##################################################
################################################
#  process @dfResults  :  expected && unexpected
################################################
##################################################

foreach $eDF (@dfResults) {
   if ("$eDF" =~ m/99\%/ || "$eDF" =~ m/100\%/  ) {
   #if ( "$eDF" =~ m/100\%/  ) {
      push (@STATUS,"\n$eDF");
   }
   if ("$eDF" =~ m/\/vmosg\/work/ ) {
      push (@STATUS,"\n$eDF");
   }
}

push (@STATUS,"\n\n");

##################################################
################################################
#  test for existence of files 
################################################
##################################################

if (! -e "$U34" ) {
   push(@STATUS,"MISSING  :\n$U34\n\n");
}
if (! -e "$U42" ) {
   push(@STATUS,"MISSING  :\n$U42\n\n");
}
if (! -e "$U55" ) {
   push(@STATUS,"MISSING  :\n$U55\n\n");
}
if (! -e "$U67" ) {
   push(@STATUS,"MISSING  :\n$U67\n\n");
}

print "\nSTATUS length  =  $#STATUS\n";
print "\nSTATUS array   =\n@STATUS\n";

##################################################
################################################
#  parse @STATUS  :  set subject via results 
################################################
##################################################

foreach $eSV (@STATUS) {
   if ("$eSV" =~ m/99\%/ || "$eSV" =~ m/100\%/) {
     $fault++;
   }
   if ("$eSV" =~ m/MISSING/) {
     $fault++;
   }
   if ("$eSV" =~ m/vmosg\/work/) {
     $accept++;
   }

}
if ($accept > "0" && $fault == "0" ) {
   $ENV{SUBJECT}="tdmrb01 STABLE";
}
else {
   $ENV{SUBJECT}="tdmrb01 FAULT reported";
}

print "\nfault         =  $fault\n";
print "\naccept        =  $accept\n";
print "\nENV{SUBJECT}  =  $ENV{SUBJECT}\n\n";

##################################################
################################################
#  email status daily
################################################
##################################################

$smtp = Net::SMTP->new('NA.relay.ibm.com');

$smtp->mail($ENV{BUILDid});
$smtp->to('jdoll@us.ibm.com','ajoncic@us.ibm.com','vsubrama@us.ibm.com','saravanp@us.ibm.com');
$smtp->data();
$smtp->datasend("Subject: $ENV{SUBJECT}\n");  # must have e o l "\n"
$smtp->datasend("@STATUS\n");

$smtp->dataend();
$smtp->quit;

##################################################
##################################################

close (STDERR);
close (STDOUT);

