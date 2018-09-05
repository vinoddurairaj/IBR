#!/usr/bin/perl -Ipms
#######################################
#####################################
#
#  perl -w ./bRcpDocs.pl
#
#    doc / help file product inclusion requirements
#	 
#    RFX  docs stage location  :
#        tdmf_ose/unix_v1.2/doc 
#    TUIP docs stage location  :
#        tdmf_ose/unix_v1.2/doc/tuip 
#
#####################################
#######################################
 
#######################################
#####################################
#  vars  /  pms  /  use 
#####################################
#######################################

eval 'require bRglobalvars';
eval 'require bRtreepath';
eval 'require bPWD';

our $RFXver;
our $TUIPver;
our $COMver;
our $Bbnum;

our $CWD;
our $RFXbuildtree;
our $TUIPbuildtree;

our $DocTarget="doc";

our $DOCpath;
our $valueIn;

#######################################
#####################################
#  STDOUT  /  STDERR 
#####################################
#######################################

open (STDOUT, ">logs/cpdocs.txt");
open (STDERR, ">>&STDOUT");

#######################################
#####################################
#  exec statements 
#####################################
#######################################

print "\n\nSTART : bRcpdocs.pl\n\n";

#######################################
#####################################
#  logs values 
#####################################
#######################################

print "\nCOMver         =  $COMver\n";
print "\nBbnum          =  $Bbnum\n";
print "\nRFXbuildtree   =  $RFXbuildtree\n";
print "\nTUIPbuildtree  =  $TUIPbuildtree\n";
print "\nRFXver         =  $RFXver\n";
print "\nTUIPver        =  $TUIPver\n\n";

#######################################
#####################################
#  sub establishPWD to enable var $CWD with pwd
#####################################
#######################################

establishPWD();

print "\nsub establishPWD returned  :  $CWD\n\n";

#######################################
#####################################
#  RFX cp Docs
#####################################
#######################################

print "\n$RFXver cp routine\n";

chdir "$RFXbuildtree" || die "chdir death  :  $RFXbuildtree  :  $!\n";

system qq(pwd);

cpDocs ($RFXver);

#######################################
#####################################
#  return to origin $CWD
#####################################
#######################################

chdir "$CWD"  || die "chdir death  :  $CWD  :  $!\n";
print "\n";
system qq(pwd);

#######################################
#####################################
#  TUIP cp Docs
#####################################
#######################################

print "\n\n$TUIPver cp routine\n";

chdir "$TUIPbuildtree" || die "chdir death  :  $TUIPbuildtree  :  $!\n";;

system qq(pwd);

cpDocs ($TUIPver);

#######################################
#####################################
#  sub cpDocs
#####################################
#######################################

sub cpDocs {

   $valueIn = shift;

   print "\n\nvalueIn  =  $valueIn\n\n";

   if ("$valueIn" =~ m/$RFXver/ )  { $DOCpath="../../doc"; }
   if ("$valueIn" =~ m/$TUIPver/ ) { $DOCpath="../../../doc"; }

   print "\n\n$valueIn  :  DOCpath =  $DOCpath\n";

   system qq(cp -p $DOCpath/*.pdf ./$DocTarget);

}

#######################################
#######################################

print "\n\nEND bRcpdocs.pl\n\n";

#######################################
#######################################

close (STDERR);
close (STDOUT);

