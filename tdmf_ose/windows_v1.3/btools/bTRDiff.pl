#!/usr/local/bin/perl -w
################################
##############################
# 
#   perl -w ./bRdiff.pl
#
#	determine source differences between last build tag and
#	head of trunk or branch
#
#
#	cvs last build tag retrieved from cvslasttag.txt
#
#	trunk or head revision var should be set in bSMglobalvars.pm
#
###############################
################################

################################
##############################
# vars
##############################
################################

##require "bSMglobalvars.pm";

##$Bbnum="$Bbnum";

########################################
######################################
# STDERR, STDOUT
######################################
########################################

open (STDOUT, ">CVSrdiff.txt");
open (STDERR, "\&STDOUT");

print "\n\nSTART :   bRdiff.pl :  script : cvs : to determine source changes\n\n";

open (FH0, "<cvslasttag.txt");
@tag=<FH0>;
close(FH0);

print "\n\ntag array index 0 : $tag[0]\n\n";
$tag0="$tag[0]";
print "\n\ntag0 variable :  $tag0\n\n";

system qq($cd);

unlink "sourcechangelist.txt";
unlink "btoolschangelist.txt";

chdir "..\\..\\";

system qq($cd);

print "\nbranch_rdiff = : $branch_rdiff\n";

@list= qx ! cvs -Q rdiff -s $branch_rdiff -r $tag0 SSM sideapps toolsets controls FLEXlm openssl !;

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

for ($i=0 ; $keepparsing[$i] ; $i++) {

   print "\nFOREACH 2 : index : keepparsing[$i] : $keepparsing[$i]\n";

      if ( $keepparsing[$i] =~ m/CCMain.rc/ ) {                     

         print "\nIF :  CCMain.rc : match : $keepparsing[$i]\n";
         @CCMain = qx !  cvs status "$keepparsing[$i]" ! ;

         print "\nCCMain array = : @CCMain\n";

         foreach $e2 ( @CCMain ) {
             if ( "$e2" =~ m/Working\ revision/i) {
                 $e2 =~  s/Working\ revision\://g && $e2 =~ s/\s+//g;
                 $fileversion = "$e2"; 
                 print "\nIF fileversion = : $fileversion" 
             }     #if close bracket

         }         #foreach close bracket        
       
         @cvslogout= qx ! cvs log -r$fileversion "$keepparsing[$i]" ! ;

         #print "\ncvslogout = :  \n@cvslogout\n";
         
           foreach $e3 (@cvslogout ) { # parse out current index if it matches : buildcommitstring
           #print "\nFOREACH e3 : $e3\n";
              if ("$e3" =~ m/buildcommitstring/ ) {
                 splice ( @keepparsing, $i, 1);
                 $i=$i - 1;
                print "\n\ni after subtract - 1 = :  $i\n";   
             } # if close bracket
          }    # foreach close bracket

      }       # if close bracket 

}                # foreach close bracket

print "\n\nkeepparsing length = : $#keepparsing\n";

print "\n\nkeepparsing end array = : @keepparsing\n";

open (FH1, ">SSM\\btools\\sourcechangelist.txt");

print FH1 "@keepparsing\n";

close (FH1);

open (FH2, ">SSM\\btools\\btoolschangelist.txt");

print FH2 "@trackbtools\n";

close (FH2);

chdir "SSM\\btools";

#return something, if 0 or higher, run the build as the array contains contents.
#  if -1, then, exit build process, we have, in theory, no source changes.

#splice ( @keepparsing, 0, 1); #test : remove 1 entry, index 0
#undef(@keepparsing);	  #test : undef array....

#inc build number for cvs message
$buildnumber="$buildnumber";
$inc="$buildnumber";
$Bbnum=++$inc;

if ( $#keepparsing > -1 ) {
    print "\nkeepparsing array length = : $#keepparsing\n";
    print "\nexiting with exit code 0, run the build ....source changes should have occurred\n\n";
    system qq(cvs commit -m "Build $Bbnum commit via bSMrdiff.pl :  buildcommitstring" sourcechangelist.txt);
    system qq(cvs commit -m "Build $Bbnum commit via bSMrdiff.pl :  buildcommitstring" btoolschangelist.txt);
    exit (0);
}
else {
    print "\nkeepparsing array length = : $#keepparsing\n";
    print "\nexiting with exit code 1, NO build .... NO source changes should have occurred\n\n";
    exit (1);
}

print "\n\nEND :  bRdiff.pl\n\n";
