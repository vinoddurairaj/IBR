#!/usr/bin/perl -Ipms
###########################################
#########################################
#
#  perl -w bRrmPreviousBldTree.pl
#
#    remove previous build tree
#
#########################################
###########################################

###########################################
#########################################
# vars  /  pms  /  use
#########################################
###########################################

eval 'require bRglobalvars';

our $Bbnum;
our $buildnumber;

###########################################
#########################################
# STDERR / STDOUT
#########################################
###########################################

open (STDOUT, ">logs/rmpreviousbldtree.txt");
open (STDERR, ">>&STDOUT");

print "\n\nSTART : bRrmPreviousBldTree.pl\n\n";

###########################################
#########################################
#  sudo rm -rf  :  $Bbnum -1  (buildnumber minus 1) 
#########################################
###########################################

subtract1 ();

print "\n\nafter subtract1 ()  :  Bminus = $Bminus\n\n";

chdir "../builds/" || die "chdir death  :  $Bminus ";

system qq(pwd);

if ($Bminus) { system qq(sudo rm -rf $Bminus); }

chdir "tuip";

system qq(pwd);

if ($Bminus) { system qq(sudo rm -rf $Bminus); }

###########################################
###########################################

print "\n\nEND :  bRrmPreviousBldTree.pl\n\n";

###########################################
###########################################

close (STDERR);
close (STDOUT);

###########################################
#########################################
#  subroutine  :  subtract1  "subtract 1 from buildnumber, but,
#	keep prefixed 0's " 
#########################################
###########################################

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

