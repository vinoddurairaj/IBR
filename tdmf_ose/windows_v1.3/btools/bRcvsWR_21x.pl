#!/usr/local/bin/perl -w
################################
##############################
# 
#   perl -w ./bRcvsWR_21x.pl
#
#	determine source differences between last build tag and
#	and current build tag
#
#	NOTE  :  temporary implementation to help development
#		know when source changes have occurred.
#	
#	fixing missed source file issue using :
#
#		tag to tag change list creation
#
#	must run from directory  :
#	tdmf_ose\\builds\\$Bbnum\\tdmf_ose\\windows\\btools
#
###############################
################################

################################
##############################
# global  vars 
##############################
################################

require "bRlastbuildtag_21x.pm";
require "bRglobalvars.pm";
#require "bRcvslasttag.pm";
#require "bRcvstag.pm";
#$Bbnum="$Bbnum";
#$RFWver="$RFWver";
$LASTBUILDTAG="$LASTBUILDTAG";

########################################
######################################
# STDERR, STDOUT
######################################
########################################

open (STDOUT, ">WRlog_21x.txt");
open (STDERR, "\&STDOUT");

print "\n\nSTART :   bRcvsWR_21x.pl :  script : cvs : to determine source changes\n\n";

#system qq($cd);

#must be run from root of cvs workarea

chdir "..\\..\\..";

system qq($cd);

print "\n\n qx ! cvs -Q rdiff -s -r V210GA -r $LASTBUILDTAG tdmf_ose/windows_v1.3/ftdsrc ! \n\n";
@list= qx ! cvs -Q rdiff -s -r V210GA -r $LASTBUILDTAG tdmf_ose/windows_v1.3/ftdsrc !;

# for testing  :  quick return
#@list= qx ! cvs -Q rdiff -s -r $CVSTAG -r $LASTCVSTAG SSM/btools !;

print "\nsource change list : beginning :  \n@list\n";

foreach $e0 (@list) {
   #print "\nFOREACH : e0 :  $e0\n";
   if ( "$e0" =~ m/^File / || "$e0" =~ m/^\ File/ ) {
      $e0 =~ s/^File\ //g || $e0 =~ s/^\ File\ //g; 
      $e0=~ s/changed\ from\ revision(.*)$//g;  #match string and everything to end of line
      $e0=~ s/is\ removed(.*)$//g;		  #match string and everything to end of line
      $e0=~ s/is\ new(.*)$//g;		  #match string and everything to end of line
      #print "\nIF : $e0\n";
      #$e0 =~ s/^\ //g; 
      push (@keepparsing, "$e0");
      }
}

print "\n\nkeepparsing = :\n@keepparsing\n\n"; 

# track btools entries separately

$i="0";

for ($i=0 ; $keepparsing[$i] ; $i++) {
      print "\nFOREACH : index : keepparsing[$i] : $keepparsing[$i]\n";
      if ( $keepparsing[$i] =~ m/btools\// ) {                     
         print "\nIF :  btools match : $keepparsing[$i]\n";
         push (@trackbtools,"$keepparsing[$i]"); 
         splice ( @keepparsing, $i, 1);
         $i=$i - 1;
         print "\n\ni after subtract - 1 = :  $i\n";   
      } 

}

print "\n\nkeepparsing after splice = :\n@keepparsing\n\n"; 

print "\n\ntrackbtools list =\n@trackbtools\n\n"; 

print "\n\nkeepparsing length = : $#keepparsing\n";

print "\n\nkeepparsing array AFTER = : @keepparsing\n";

####################################
####################################
#  process each file via cvs status  : 
#
#	produce printable list for change list
####################################
####################################

our $fileversion;  #need declaration outside foreach scope...

chomp (@keepparsing);

foreach $filename (@keepparsing) {

   chomp ($filename);

   print "\nprocessing file :\n$filename\n\n";
   print "|$filename|";


   $filename=~s/^\ //g;	#remove space at beginning of string

   @cvsstatus=`cvs status "$filename"`;  

   # grab "$fileversion" to process via cvs log -N -r$fileversion $filename

   foreach $ex1 (@cvsstatus) {
      #print "\nex1 = $ex1\n";
      if ( "$ex1" =~ m/Working\ revision/i) {
         print "\nIF fileversion  : cvstatus :  $ex1\n";
         $ex1 =~ s/Working\ revision\://g;
         print "\nIF  fileversion  :  HERE  :  0 : $ex1\n";
         $ex1 =~ s/\s+//g;
         print "\nIF fileversion  :  HERE  :  2 : $ex1\n";
         $fileversion="$ex1";
      }   # if close bracket

   }      #  fileversion foreach close bracket

   print "\nfilename = $filename\n\n";
   print "\nfileversion = $fileversion\n\n";

   #  process each file thru cvs log & fileversion & filename

   @cvslog=`cvs log -N -r$fileversion "$filename"`;

   chomp (@cvslog);
   chomp ($filename);
   chomp ($fileversion);
   #create / add to the list with $filename and $fileversion
   push (@printarray, "$filename\t");
   push (@printarray, "$fileversion\t");

   print "\ncvslog length = $#cvslog\n";

   #  via cvs log  :  match string date  :  string after date is the log message
   $i="0";
   foreach $ex10 (@cvslog ) {
   print "\nex10 : $ex10\n";
   print "FOREACH cvslog array  :  $cvslog[$i]\n\n"; 
   $i++;
   print "\ni = $i\n";
      if ( "$ex10" =~ /date\:/) {
         print "\nIF: date ?? : $ex10\n";
         print "\nPUSH NEXT INDEX to  :  \@printarray :  \n$cvslog[$i]\n\n"; 
         push (@printarray, "$cvslog[$i]\n");
      }  #local if close bracket
   }   # local foreach close bracket

}      #Main foreach  :  close bracket

print "\n\nprintarray =\n@printarray\n\n";

#######################################
#######################################
#  same routine / file processing as above  :
#
#	except  :  process build script changes  :  btools
#######################################
#######################################

print "\n\ntrackbtools length =\n$#trackbtools\n\n"; 

our $btoolsversion;  #need declaration outside foreach scope...

chomp (@trackbtools);

foreach $btoolsname (@trackbtools) {

   chomp ($btoolsname);

   print "\nbtools process file :\n$btoolsname\n\n";
   print "|$btoolsname|";

   $btoolsname=~s/^\ //g;	#remove space at beginning of string

   @btoolsstatus=`cvs status "$btoolsname"`;  

   # grab "$btoolsversion" to process via cvs log -N -r$btoolsversion $btoolsname

   foreach $eb1 (@btoolsstatus) {
      #print "\neb1 = $eb1\n";
      if ( "$eb1" =~ m/Working\ revision/i) {
         print "\nIF btoolsversion  : cvstatus :  $eb1\n";
         $eb1 =~ s/Working\ revision\://g;
         print "\nIF  btoolsversion  :  HERE  :  0 : $eb1\n";
         $eb1 =~ s/\s+//g;
         print "\nIF btoolsversion  :  HERE  :  2 : $eb1\n";
         $btoolsversion="$eb1";
      }   # btoolsversion  if  :  close bracket

   }      #  btoolsversion foreach close bracket

   print "\nbtoolsname = $btoolsname\n\n";
   print "\nbtoolsversion = $btoolsversion\n\n";

   #  process each file thru cvs log & btoolsversion & btoolsname

   @btoolslog=`cvs log -N -r$btoolsversion "$btoolsname"`;

   chomp (@btoolslog);
   chomp ($btoolsname);
   chomp ($btoolsversion);
   #create / add to the list with $btoolsname and $btoolsversion
   push (@btoolsarray, "$btoolsname\t");
   push (@btoolsarray, "$btoolsversion\t");

   print "\ncvslog length = $#cvslog\n";

   #  via cvs log  :  match string date  :  string after date is the log message
   $i="0";
   foreach $ex10 (@btoolslog ) {
   print "\nex10 : $ex10\n";
   print "FOREACH btoolslog array  :  $btoolslog[$i]\n\n"; 
   $i++;
   print "\ni = $i\n";
      if ( "$ex10" =~ /date\:/) {
         print "\nIF: date ?? : $ex10\n";
         print "\nPUSH NEXT INDEX to  :  \@btoolsarray :  \n$btoolslog[$i]\n\n"; 
         push (@btoolsarray, "$btoolslog[$i]\n");
      }  #local if close bracket
   }   # local foreach close bracket

}      #Main  :  btools  :  foreach  :  close bracket

print "\n\nbtoolsarray length =:\n$#btoolsarray\n\n";

print "\n\nprintarray length =:\n$#printarray\n\n";

#chomp(@printarray);
#chomp(@btoolsarray);

# dump changes to sourcechangelist.txt  and btoolschangelist.txt  :  respectively

#chdir "SSM\\builds\\$Bbnum\\SSM\\btools";
#chdir "SSM\\builds\\$Bbnum\\SSM\\btools";

system qq($cd);

# print results to respective files  :  read via bSMparlog.pl

print "\n\nprintarray length = $#printarray\n\n";

open (FHcc,">sourcechangelist.txt");

foreach $CC (@printarray) {
   print FHcc "$CC";
   print "CC = $CC";
}

close (FHcc);

open (FHbt,">btoolschangelist.txt");

foreach $BT (@btoolsarray) {
   print FHbt "$BT";
   print "BT = $BT";
}

close (FHbt);

############################################
############################################

print "\n\nEND :  bRcvsWR.pl\n\n";
