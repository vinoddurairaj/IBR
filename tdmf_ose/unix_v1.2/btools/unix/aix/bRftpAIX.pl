#!/usr/local/bin/perl -w
#####################################
###################################
#
# perl -w -I".." ./bRftpAIX.pl
#
#	grab AIX deliverables intermediate target directories
#
###################################
#####################################

#####################################
###################################
# global vars
###################################
#####################################

require "bRglobalvars.pm";
require "bRbuildmachinelist.pm";
require "bRdevvars.pm";
$branchdir="$branchdir";
$AIXwa0="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc";
$AIXwa1="installp/mklpp/rel/dist/CD-ROM/usr/sys/inst.images";
$aixbuild="$aixbuild";

#####################################
###################################
# run sub machinename before STDOUT / STDERR to
#	set logfile name to OS for this script 
###################################
#####################################

machinename();

if ($UNAME =~ /bmaix43/ ) {
   $LOGFILE="ftp43.txt";
}
elsif ($UNAME =~ /uabmach1/ ) {
   $LOGFILE="ftp51.txt";
}
elsif ($UNAME =~ /bmaix52/ ) {
   $LOGFILE="ftp52.txt";
}
else {
   #to be here  :  we (scripts) went wrong somewhere
   $LOGFILE="ftpERROR.txt";
}

#####################################
###################################
# capture stderr & stdout
###################################
#####################################

open (STDOUT, ">$LOGFILE");
open (STDERR, ">>&STDOUT");

print "\n\nSTART  :  bRftpAIX.pl\n\n";

print "\n\nUNAME = $UNAME  :  build level $Bbnum\n\n";

#####################################
# get el passwordo
#####################################

open(PSWD,"../../bRbmachine1.txt");
@psswd=<PSWD>;
close(PSWD);

$passwd=$psswd[0];

#print "\n$passwd & $passwd0\n";

system qq(pwd);

######################################
####################################
#  identify machine
####################################
######################################

print "\n\nUNAME = $UNAME\n\n";

######################################
####################################
#
# ftp to source of package build machines  
#
####################################
######################################

#4.3
if ($UNAME =~ /bmaix43/ ) {
   chdir "../../../builds/$Bbnum/AIX/4.3.3" || die "chdir  :  $UNAME  :  here";
   system qq(pwd);

   pingit($aixbuild);  #returns $rc

   if ("$rc" == "0" ) {
      print "\n$rc\n";
      aixget();
   }
   else {
      print "\n$aixbuild  :   $UNAME  :  sub pingit error  :  $rc\n";
   }
}

#5.1
if ($UNAME =~ /uabmach1/ ) {
   chdir "../../../builds/$Bbnum/AIX/5.1" || die "chdir  :  $UNAME  :  here";
   system qq(pwd);

   pingit($aixbuild);  #returns $rc

   if ("$rc" == "0" ) {
      print "\n$rc\n";
      aixget();
   }
   else {
      print "\n$aixbuild  :   $UNAME  :  sub pingit error  :  $rc\n";
   }
}

#5.2
if ($UNAME =~ /bmaix52/ ) {
   chdir "../../../builds/$Bbnum/AIX/5.2" || die "chdir  :  $UNAME  :  here";
   system qq(pwd);

   pingit($aixbuild);  #returns $rc

   if ("$rc" == "0" ) {
      print "\n$rc\n";
      aixget();
   }
   else {
      print "\n$aixbuild  :   $UNAME  :  sub pingit error  :  $rc\n";
   }
}

###################################################
###################################################
print "\n\nEND  :  bRftpAIX.pl\n\n";

########################################

close (STDERR);
close (STDOUT);

###################################################
#################################################
# subroutine(s)
#################################################
###################################################

sub aixget {
use Net::FTP;
   $ftp=Net::FTP->new("$aixbuild")  or warn "can't connect $UNAME env : $@\n";

   $ftp->login("bmachine","$passwd")	or warn "could not login : $UNAME : error : $@\n";

   $ftp->cwd("$AIXwa0/$AIXwa1");
   $DIR=$ftp->pwd ();
   print "\nDIR dir :  $DIR\n";

   $ftp->binary;

   $ftp->get("dtc.rte") or warn " $UNAME get error : bRftppkg.pl script: $!";
#   $ftp->get("dtc.rte.doc") or warn "$UNAME get error : bRftppkg.pl script: $!";
   $ftp->ascii;

   $ftp->get(".toc") or warn " $UNAME get error : bRftppkg.pl script: $!";

   $ftp->quit;

}

###################################################
#################################################
# pingit()  :  validate machine is alive or exit as it bombs ftp program 
#################################################
###################################################

sub pingit {

   #adding in rsh ls too  ==>> ping can ping a valid
   #running interface  :  machine may not indded be up / running
   #and this will break the perl ftp module

   print "\n\nENTER sub pingit : machine  :  $aixbuild\n\n";

   @pingit=`ping -c 1 $aixbuild`;
   $pingRC="$?";
   @rshit = system qq(rsh $aixbuild -l bmachine ls);
      
   $rshRC="$rshit[0]";     #syntax  :  $rshRC="@rshit";  works too.
   			   #using $array[0] syntax for certainty as array should
			   #contain only 1 element  :  the return code, hopefully

   if ( "$pingRC" == "0" && "$rshRC" == "0" ) {
      print "\nping rc = $pingRC  :  rsh rc = $rshRC\n";
      $rc="0";
      print "\n\nEXIT sub pingit : machine  :  $aixbuild\n\n";
      return $rc;
   }
   else {
      print "\nping rc = $pingRC  :  rsh rc = $rshRC\n";
      print "\n\nEXIT sub pingit : machine  :  $aixbuild\n\n";
      $rc="1";
      return $rc;
   }

}  # sub pingit close bracket

sub machinename {
   $UNAME=qx| rsh $aixbuild -l bmachine \"uname -n\"|;
   return $UNAME;
}


