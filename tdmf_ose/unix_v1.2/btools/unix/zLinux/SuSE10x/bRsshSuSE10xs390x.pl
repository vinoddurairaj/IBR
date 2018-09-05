#!/usr/bin/perl -I../../../pms -I../../.. 
#####################################
###################################
#
#  perl -I../../.. bRsshSuSE10xs390.pl
#
#    ssh to SuSE10x 6 s390x (zLinux via rtp)
#    make  :  build it
#    make  :  package
#    ftp from rtp to NSJ dmsbuilds  
#        (network issue in other direction
#    ftp from dmsbuilds to local
#        package to product image
#
###################################
#####################################

#####################################
###################################
#  vars  /  pms  / use
###################################
#####################################

require "bRglobalvars.pm";
require "bRbuildmachinelist.pm";
require "bRsshVARS.pm";
require "bRstrippedbuildnumber.pm";
require "bRlogValues.pm";

our $buildnumber;
our $SuSE10xPath;
our $LWORK="/localwork/bmachine";
our $btoolsPathZ="$LWORK/$SuSE10xPath/unix/zLinux/SuSE10x";
our $zsuse10xs390x64;

our $SSH="ssh -l bmachine $zsuse10xs390x64";

our $SSH0="$SSH \"$source ; uname -a ; cd $btoolsPathZ ; pwd ; cd ../../.. ; pwd ; cvs update ; echo \"cvs update rc = $?\"\""; 
our $SSH1="$SSH \"$source ; cd $btoolsPathZ ; pwd ; perl bRbuildSuSE10x.pl\""; 
our $SSH2="$SSH \"$source ; cd $btoolsPathZ ; pwd ; perl bRpostBuildSuSE10x.pl\""; 

#print "\n\nSSH0  :\n$SSH0\n\n";
#print "\n\nSSH2  :\n$SSH2\n\n";
#exit();

#####################################
###################################
# STDERR / STDOUT
###################################
#####################################

unlink "SuSE10xs390x64.txt";
open (STDOUT, ">SuSE10xs390x64.txt");
open (STDERR, ">&STDOUT");

#####################################
###################################
#  exec statements
###################################
#####################################

print "\n\nSTART : bRsshSuSE10xs390x.pl\n\n";

#####################################
###################################
#  log values
###################################
#####################################

logvalues();

print "\n\nSuSE10xPath        =  $SuSE10xPath\n\n";

#####################################
###################################
#  rsh call to cvs update remote btools directory
###################################
#####################################

      system qq($SSH0);
      print "\ncvs update rc = $? \n";

#####################################
###################################
#  ssh  :  exec remote bRbuildSuSE10x.pl  :  cvs checkout / ./bldcmd
###################################
#####################################

    system qq($SSH1);
    print "\nbRbuildSuSE10x.pl  :  SSH2  : rc = $?\n\n";

#####################################
###################################
#  ssh  :  exec remote bRpostBuildSuSE10x.pl  :  cvs checkout / ./bldcmd
###################################
#####################################

    system qq($SSH2);
    print "\nbRpostBuildSuSE10x.pl  :  SSH2  : rc = $?\n\n";

#####################################
###################################
#  ftp RTP deliverables to local from dmsbuilds 
###################################
#####################################

system qq(perl bRftpRTP.pl);

#####################################
#####################################

print "\n\nEND :  bRsshSuSE10xs390x.pl\n\n";

#####################################
###################################
#  subroutine  :  subtract1  "subtract 1 from buildnumber, but,
#	keep prefixed 0's " 
###################################
#####################################

sub subtract1 {

   $minus1= $buildnumber - 1;

   print "\n\nafter subtract $minus1\n\n";

   if ( $minus1 !~ /(\d)(\d)/ ) {
      $minus1 =~ s/$minus1/0000$minus1/;
      print "\n2\n";
   } 

   elsif ( $minus1 !~ /(\d)(\d)(\d)/ ) {
      $minus1 =~ s/$minus1/000$minus1/;
      print "\n3\n";
   } 

   elsif ( $minus1 !~ /(\d)(\d)(\d)(\d)/ ) {
      $minus1 =~ s/$minus1/00$minus1/;
      print "\n4\n";
   } 

   elsif ( $minus1 !~ /(\d)(\d)(\d)(\d)(\d)/ ) {
      $minus1 =~ s/$minus1/0$minus1/;
      print "\n4\n";
   } 
      
   print "\n\nminus1 = $minus1\n\n";

   $Bminus="B$minus1";

   print "\n\nBminus = $Bminus\n\n";

}  # subtract1 sub close bracket

###################################
###################################

close (STDERR);
close (STDOUT);
