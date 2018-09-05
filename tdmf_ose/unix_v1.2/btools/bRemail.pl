#!/usr/bin/perl -Ipms
#####################################################
###################################################
#  perl -w bRemail.pl
#
#	distribute notification.txt
#
###################################################
#####################################################

#####################################################
###################################################
#  vars  /  pms  /  use 
###################################################
#####################################################

eval 'require bRglobalvars';
eval 'require bRsucFail';

our $RFXver;
our $Bbnum;
our $COMver;

our $SUCCEED_FAILURE;

#####################################################
###################################################
#  STDOUT  /  STDERR 
###################################################
#####################################################

open (STDOUT,">logs/email.txt");
open (STDERR,">&STDOUT");

#####################################################
###################################################
#  exec statements 
###################################################
#####################################################

print "\n\nSTART  :  bRemail.pl\n\n\n";

#####################################################
###################################################
#  log values 
###################################################
#####################################################

print "\n\nRFXver         =  $RFXver\n";
print "\nBbnum            =  $Bbnum\n";
print "\nCOMver           =  $COMver\n";
print "\nSUCCEDD_FAILURE  =   $SUCCEED_FAILURE\n";

####################################################
##################################################
#  create array with notification.txt contents
##################################################
####################################################

open (FHnotification,"<notification.txt");
@noti=<FHnotification>;
close (FHnotification);

####################################################
##################################################
#  set $ENV{BUILD} = $COMver.$Bbnum.$SUCCEED_FAILURE 
##################################################
####################################################

$ENV{BUILD}="$COMver.$Bbnum.$SUCCEED_FAILURE";
$ENV{BUILDid}="jdoll\@us.ibm.com";

print "\n\nENV{BUILD}      =  $ENV{BUILD}\n\n";
print "\n\nENV{BUILDid}    =  $ENV{BUILDid}\n\n";

####################################################
##################################################
#  perl smtp module
##################################################
####################################################

use Net::SMTP;

    #$smtp = Net::SMTP->new('na.relay.ibm.com');
    $smtp = Net::SMTP->new('localhost');

    $smtp->mail($ENV{BUILDid});
    #$smtp->mail($ENV{USER});  #FROM print message  :  original
    #$smtp->mail($RFXver.$Bbnum);
    $smtp->to('kodjod@ca.ibm.com','TRbosworth@us.ibm.com','bmusolff@us.ibm.com','jacquesv@ca.ibm.com', 'jhenthor@us.ibm.com','paulclou@ca.ibm.com','proulxm@ca.ibm.com','robert.kilduff@ie.ibm.com','tomegan@us.ibm.com','mvadnais@ca.ibm.com','tomegan@us.ibm.com','robertgubbins@ie.ibm.com','damienmonks@ie.ibm.com','pellswo@us.ibm.com','lucykung@us.ibm.com','jdoll@us.ibm.com');
    #$smtp->to('jdoll@us.ibm.com');
    #$smtp->subject('yo hoo');  #<<== syntax error
    $smtp->data();
    #$smtp->datasend("To: RFX Dev"); 
    #$smtp->datasend("A simple test message\n");
    $smtp->datasend("Subject:  $ENV{BUILD}\n");  #<<== must have end of line "\n" or everything ends up in subject line
    $smtp->datasend("@noti\n");
    $smtp->dataend();
    $smtp->quit;

####################################################
####################################################

print "\n\nEND  :  bRemail.pl\n\n\n";

####################################################
####################################################

close (STDERR);
close (STDOUT);

