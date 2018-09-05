#!/usr/local/bin/perl -w
##############################
############################
#
#  perl -w bRtelnetRM.pl
#
#	telnet to bmachine8
#	RedHat linux 8.x 
#	rm -rf /u01/bmachine/iso/$RFWver/$Bbnum
#
#	just the $Bbnum directory
#
############################
##############################

##############################
############################
# vars
############################
##############################

require "bRglobalvars.pm";
$workarea="/u01/bmachine/iso/$RFWver";
$userid="bmachine";
$RFWver="$RFWver";
$Bbnum="$Bbnum";

##############################
############################
#
# STDERR / STDOUT to isoLOG.txt
#
############################
##############################

open (STDOUT, ">>isoLOG.txt");
open (STDERR, ">>&STDOUT");

##############################
############################
# grab password
############################
##############################

open(PSWD,"..\\bRbmachine.txt");
@psswd=<PSWD>;
close(PSWD);

$passwd=$psswd[0];

#print "\n\npasswd : $passwd\n\n";

#telnet test

print "\n\nSTART : bRtelnetRM.pl\n\n";

# eval :  trap error message :  if sent ==> sets into $@

eval {

use Net::Telnet ();
   $t = new Net::Telnet (Timeout => 500,        
   #$t = new Net::Telnet (Timeout => undef,        
   Prompt => '/bmachine8/');  # works with bash & tcsh
   #Prompt => '/\$ $/');  # works with sh
   $t->open("129.212.206.131");
   $t->login($userid, $passwd);
      #@create = $t->cmd( "cd $workarea ; pwd ; ls -la");
      @create = $t->cmd( "cd $workarea; pwd ; ls -la ; rm -rf $Bbnum");
      #print "\ntelnet cmd : rc = $?\n";  # no value to script....
      print @create;

};   #eval close bracket

if ($@) {
    print "\n\nLINUX :  bRtelnetRM.pl :  errors : rc = $@\n\n";
    undef ($@);
}

print "\n\nEND : bRtelnetRM.pl\n\n";

####################################
####################################

close (STDERR);
close (STDOUT);

