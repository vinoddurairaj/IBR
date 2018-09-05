#!/usr/local/bin/perl -w
#######################################
#####################################
#
#  perl -w ./bRdirs.pl
#
#	directories required to create product image 
#	 
#	&  :  miscellaneous mkdirs if required
#
#	builds\\$Bbnum  :  intermediate target for pre-iso
#
#####################################
#######################################
 
#######################################
#####################################
# get global vars 
#####################################
#######################################

require "bRglobalvars.pm";

#######################################
#####################################
# capture stderr stdout
#####################################
#######################################

open (STDOUT, ">mkdirs.txt");
open (STDERR, ">>&STDOUT");

print "\n\nSTART : \t bRdirs.pl\n\n";

#######################################
#####################################
# setup the intertarg directory structure, and B####
#	used via each incremented build
#
#####################################
#######################################


chdir "..\\";

system qq($cd);

$builds="builds";
chomp ($builds);

if (! -d $builds) {
   mkdir "$builds";
   print "\nmkdir $builds\n";
}
else {
   print "\ndir $builds exists\n";
} 

chdir "$builds";

system qq($cd);

mkdir "$Bbnum";

chdir "$Bbnum";

system qq($cd);

#######################################
#####################################
# array  :  mkdirs  :  product image requirements
#####################################
#######################################

@Dirs= qw(AIX
	AIX\\4.3.3
	AIX\\5.1-32bit
	doc
	HPUX
	HPUX\\10.20
	HPUX\\11
	HPUX\\11i
	solaris
	solaris\\2.6
	solaris\\7
	solaris\\8
	solaris\\9
);

foreach $elem3 (@Dirs) {

#	print "\nDirs : $elem3\n";

  if (! -d "$elem3") {
       mkdir qq($elem3);
       print "\nIF : mkdir : $elem3\n";

  }
  else {
       print "\nELSE : Dir exists : $elem3\n";
  }

}  # foreach close bracket $elem3

#####################################
#######################################

print "\n\nEND bRdirs.pl\n\n";

close (STDERR);
close (STDOUT);
