#!/usr/bin/perl -Ipms
#####################################################
###################################################
#  perl -Ipms bRnobuildemail.pl
#
#	send NO build notification to required audience
#
###################################################
#####################################################

#####################################################
###################################################
#  vars / pm's 
###################################################
#####################################################

eval 'require bRglobalvars';
eval 'require bRsucFail';

our $COMver;
our $RFXver;
our $TUIPver;
our $Bbnum;

#####################################################
###################################################
#  STDOUT  /  STDERR
###################################################
#####################################################

open (STDOUT,">logs/nobuildemail.txt");
open (STDERR,">&STDOUT");

#####################################################
###################################################
#  exec statements 
###################################################
#####################################################

print "\n\nSTART  :  bRnobuildemail.pl\n\n\n";

#####################################################
###################################################
#  log values 
###################################################
#####################################################

print "\nCOMver    =  $COMver\n";
print "\nRFXver    =  $RFXver\n";
print "\nTUIPver   =  $TUIPver\n";
print "\nBbnum     =  $Bbnum\n";

####################################################
##################################################
#  create empty array with notification.txt contents
##################################################
####################################################

open (FHnotification,"<nobuild.txt");
@nobuild=<FHnotification>;
close (FHnotification);

####################################################
##################################################
#  set $ENV{BUILD} = subject
##################################################
####################################################

chomp ($todaydate=`date +%D+%H%M`);
$ENV{NOBUILD}="$COMver.$todaydate  :  build skipped";
$ENV{BUILDid}="jdoll\@us.ibm.com";

print "\n\n$todaydate\n\n";
print "\n\n$ENV{NOBUILD}\n\n";
print "\n\n$ENV{BUILDid}\n\n";

####################################################
##################################################
#  use perl smtp module
##################################################
####################################################

use Net::SMTP;

    $smtp = Net::SMTP->new('na.relay.ibm.com');

    $smtp->mail($ENV{BUILDid});
    #$smtp->mail($ENV{USER});  #FROM print message  :  original
    #$smtp->mail($COMver.$Bbnum);
    $smtp->to('paulclou@ca.ibm.com','jacquesv@ca.ibm.com','proulxm@ca.ibm.com','tomegan@us.ibm.com','robertgubbins@ie.ibm.com','damienmonks@ie.ibm.com','pierreb@ca.ibm.com','mvadnais@ca.ibm.com','jhenthor@us.ibm.com','lucykung@us.ibm.com','smousav@us.ibm.com','jdoll@us.ibm.com');
    #$smtp->to('jdoll@us.ibm.com');
    #$smtp->subject('yo hoo');  #<<== syntax error
    $smtp->data();
    #$smtp->datasend("To: RFX Dev"); 
    #$smtp->datasend("A simple test message\n");
    $smtp->datasend("Subject: $ENV{NOBUILD}\n");  #<<== must have end of line "\n" or everything ends up in subject line
    $smtp->datasend("@nobuild\n");
    $smtp->dataend();
    $smtp->quit;

####################################################
####################################################

print "\n\nEND  :  bRemail.pl\n\n\n";

####################################################
####################################################

close (STDERR);
close (STDOUT);

