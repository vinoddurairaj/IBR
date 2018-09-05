#!/usr/local/bin/perl -w
##############################
############################
#
#  perl -w bRtelnetISO_APAC.pl
#
#	telnet to bmachine8
#	RedHat linux 8.x 
#
#	create iso image  :  APAC TDMF deliverable  
#
############################
##############################

##############################
############################
# vars
############################
##############################

require "bRglobalvars.pm";

$workarea="/u01/bmachine/iso/$RFWver/$Bbnum/AP";
$userid="bmachine";
$mdi="mkdir image";
$cdi="cd image";
$UZ="unzip ../$RFWver.$Bbnum.ALL.TDMF.zip";
$cdd="cd ..";
$CDLABEL="$RFWver-$Bbnum-OEM";
$UID="-uid 0 -gid 0 -file-mode 555";    # root and 1 setting for os gid  r-x on executables
$op0="-f -hide-rr-moved -J -R -hfs -max-iso9660-filename";
$op1="-L -o $RFWver.$Bbnum.TDMF.iso -U -v -V $CDLABEL $UID image";
$mki="mkisofs $op0 $op1";

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

print "\n\nSTART : bRtelnetISO_APAC.pl\n\n";

# eval :  trap error message :  if sent ==> sets into $@

eval {

#   can change to undef, but, if it hangs, it never comes back...
use Net::Telnet ();
   $t = new Net::Telnet (Timeout => 1000,        
   #$t = new Net::Telnet (Timeout => undef,        
   Prompt => '/bmachine8/');  # works with bash & tcsh
   #Prompt => '/\$ $/');  # works with sh
   $t->open("129.212.206.131");
   $t->login($userid, $passwd);
      #@create = $t->cmd( "cd $workarea ; pwd ; ls -la");
      @create = $t->cmd( "cd $workarea; pwd; $mdi; $cdi; $UZ; $cdd; $mki");
      #print "\ntelnet cmd : rc = $?\n";  # no value to script....
      print @create;

};   #eval close bracket

if ($@) {
    print "\n\nLINUX :  bRtelnetISO_APAC.pl :  errors : rc = $@\n\n";
    undef ($@);
}

print "\n\nEND : bRtelnetISO_APAC.pl\n\n";

##############################
##############################

close (STDERR);
close (STDOUT);

#print "\nworkarea :  $workarea\n\n";
#print "\nmdi :  $mdi\n";
#print "\ncdi :  $cdi\n";
#print "\nUZ :  $UZ\n";
#print "\nmki :  $mki\n";
