#!/usr/bin/perl -Ipms
################################
##############################
# 
#   perl -w ./bRcvsWR.pl
#
#	determine source mods  : current tag to $previouscvstag 
#
#
#	must be run from tdmf_ose/unix_v1.2/builds/$Bbnum/checkout
#
###############################
################################

################################
##############################
#   vars  /  pm's  /  use
##############################
################################

eval 'require bRglobalvars';
eval 'require bRpreviouscvstag';
eval 'require bRcvstag';
eval 'require bRpreviousbuildGMT';
eval 'require bRbuildGMT';

our $COMver;
our $RFXver;
our $TUIPver;
our $Bbnum;

our $previouscvstag;

our $previousbuildGMT;
our $previousbuildGMTYEAR;
our $previousbuildGMTMONTH;
our $previousbuildGMTDAY;
our $previousGMT="$previousbuildGMTYEAR-$previousbuildGMTMONTH-$previousbuildGMTDAY";

our $currentbuildGMT;
our $currentbuildGMTYEAR;
our $currentbuildGMTMONTH;
our $currentbuildGMTDAY;
our $currentGMT="$currentbuildGMTYEAR-$currentbuildGMTMONTH-$currentbuildGMTDAY";

our $CVSROOT;
our $cvs="cvs -d $CVSROOT";

########################################
######################################
# STDOUT  /  STDERR
######################################
########################################

open (STDOUT, ">logs/WRlog.txt");
open (STDERR, ">\&STDOUT");

########################################
######################################
#  exec statements 
######################################
########################################

print "\n\nSTART :   bRcvsWR.pl\n\n";

system qq(pwd);
print "\n";

########################################
######################################
#  log values
######################################
########################################

print "\nCOMver              =  $COMver\n";
print "\nTUIPver             =  $TUIPver\n";
print "\nRFXver              =  $RFXver\n";
print "\nBbnum               =  $Bbnum\n";
print "\npreviousbuildGMT    =  $previousbuildGMT\n";
print "\npreviousGMT         =  $previousGMT\n";
print "\ncurrentbuildGMT     =  $currentbuildGMT\n";
print "\ncurrentGMT          =  $currentGMT\n";
print "\n\npreviouscvstag      =  $previouscvstag\n";
print "\nCVSTAG              =  $CVSTAG\n\n";

#exit();

########################################
######################################
#  cvs rdiff tag to tag  
#
#    must be run from root of cvs workarea
######################################
########################################

#chdir "../builds/$Bbnum/checkout" || die "chdir to ../builds/$Bbnum/checkout failed  :  $!";
chdir "../../..";
system qq(pwd);

print "\n\n qx ! $cvs -Q rdiff -s -r $CVSTAG -r $previouscvstag tdmf_ose/unix_v1.2 ! \n\n";
@list= qx ! $cvs -Q rdiff -s -r $CVSTAG -r $previouscvstag tdmf_ose/unix_v1.2 !;

print "\nsource change list : beginning :  \n@list\n";

#exit();

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
##################################
#  process each file via $cvs log -N -d"date GMT > lastbuilddate GMT"  : 
##################################
####################################

chomp (@keepparsing);

@srcchangelist=@keepparsing;

system qq(pwd);

foreach $esrc0 (@srcchangelist) {
   undef(@revlist);
   undef(@parseforWR);
   print "\n\nparseforrev= qx| $cvs log $branchid -N -d$currentGMT $currentbuildGMT GMT > $previousGMT $previousbuildGMT GMT \"$esrc0\" \n\n";
   @parseforrev=qx|$cvs log $branchid -N -d"$currentGMT $currentbuildGMT GMT > $previousGMT $previousbuildGMT GMT" $esrc0|;
   system qq(pwd) ;sleep 1;
   print "\nparseforrev = \n@parseforrev";

   foreach $all0 (@parseforrev)  {
      if ( "$all0" =~ /^revision/ && "$all0" !~ /revisions/ ) {
         print "\nIF  :  $all0\n";
         $all0 =~ s/revision//g;
         $all0 =~ s/^(\s+)//g;    # remove leading space(s)
         print "\nIF after  :  $all0\n";
         push (@revlist,"$all0");
         print "\nrevlist =\n@revlist\n";
      }  # if close bracket

   }     # second foreach close bracket
      foreach $filerev (@revlist) { 
        $filerev =~ s/^(\s+)//g;    # remove leading space(s)
	chomp ($filerev);
        print "\nfilerev =\n$filerev\n";
        #@parseforWR= qx| $cvs log -N -r$filerev \"$esrc0\" |;
        print "\n\nparseforWR= qx| $cvs log -N -r$filerev $esrc0 | \n\n";
        @parseforWR= qx| $cvs log -N -r$filerev $esrc0 |;
        print "\nparseforWR =\n@parseforWR\n";
        system qq(pwd);
        chomp ($filerev);
         $i="0";
         for ($i = 0; $parseforWR[$i] ; $i++ ) {

             #if ( "$parseforWR[$i]" =~ /(\s+author:)\s+(.*?;\s)(.*)$/ ) {
             if ( "$parseforWR[$i]" =~ /(\s+author:)\s+(.*?;\s)(.*)$/ ) {
                 print "\n\n\nauthor: match $parseforWR[$i]\n";
                 print "\n1 = $1\n  :  \n2 = $2\n";
                 $author = "$2";
                 print "\nauthor = $author\n";
                 $author =~ s/\;//g;
                 print "\nauthor = $author\n";
             }

             if ( "$parseforWR[$i]" =~ /^date:/ ) {
                 print "\ndate: match $parseforWR[$i]\n";
                 $i++;
                 print "\nafter $i inc  :  $parseforWR[$i]\n\n";
                 print "\nlength  :  $#parseforWR\n\n";
                 if ( "$i" < "$#parseforWR" ) { $x=$i ; $x++; print "\n\nX = $x\n\n";};
                 #capture trailing elements and add to @finalchangelist
                 @lastelements = @parseforWR[$x .. $#parseforWR];
                 print "\nlastelements range   $x .. $#parseforWR\n\n";
                 print "\nlastelements length  =  $#lastelements\n";
                 print "\nlastelements  =\n@lastelements\n";
                 $j=0;
                 print "\n\nendofarray length  =  $#endofarray\n\n";
                 while ("$j" < $#lastelements) {
                     chomp($lastelements[$j]);
                    print "\nWHILE  :  $lastelements[$j]\n";
                    push (@endofarray,"$lastelements[$j]");
                    $j++;
                 }
                 print "\n\nendofarray  =\n@endofarray\n\n";
                 chomp($parseforWR[$i]);
                 print "\n\n\nOWENS  :  $parseforWR[$i]";
                 push (@finalchangelist,"$esrc0  $filerev $author  $parseforWR[$i]  @endofarray \n");
                 print "\n\n\nTO  :  $esrc0  :  $filerev  :  $author  :  $parseforWR[$i]  :  @endofarray\n\n\n";
                 undef($parseforWR[$i]);
                 undef(@endofarray);
                 undef($author);     
            }  # if close bracket
       }        # for close bracket
     }  # third foreach close bracket

}        # first foreach close bracket

#}      #Main foreach  :  close bracket

print "\n\nfinalchangelist  =\n@finalchangelist\n";    

#######################################
#######################################
#  same routine / file processing as above  :
#
#	exception  :  process build script changes  :  btools
#######################################
#######################################

chomp (@trackbtools);

system qq(pwd);

foreach $esrc0 (@trackbtools) {
   undef(@btoolsrevlist);
   undef(@parsebtools);
   print "\n\nFOREACH  :  btools  :  esrc0  ;  $esrc0\n\n"; 
   print "\n\nbtoolsrev= qx| $cvs log $branchid -N -d$currentGMT $currentbuildGMT  GMT > $previousGMT $previousbuildGMT GMT  \"$esrc0\" \n\n";
   @btoolsrev= qx| $cvs log $branchid -N -d"$currentGMT $currentbuildGMT  GMT > $previousGMT $previousbuildGMT GMT" $esrc0 |;
   #print "\nbtoolsrev = \n@btoolsrev";

   foreach $allbtools (@btoolsrev)  {
      if ( "$allbtools" =~ /^revision/ && "$allbtools" !~ /revisions/ ) {
         print "\nIF  :  $allbtools\n";
         $allbtools =~ s/revision//g;
         $allbtools =~ s/^(\s+)//g;    # remove leading space(s)
         print "\nIF after  :  $allbtools\n";
         push (@btoolsrevlist,"$allbtools");
         print "\nbtoolsrevlist =\n@btoolsrevlist\n";
      }  # if close bracket

   }     # second foreach close bracket

      foreach $btoolsfilerev (@btoolsrevlist) { 
         $btoolsfilerev =~ s/^(\s+)//g;    # remove leading space(s)
         print "\nbtoolsfilerev =\n$btoolsfilerev\n";
         chomp ($btoolsfilerev);
         print "\n\nparsebtools= qx| $cvs log -N -r$btoolsfilerev $esrc0 | \n\n";
         @parsebtools= qx| $cvs log -N -r$btoolsfilerev $esrc0 |;
         print "\nparsebtools =\n@parsebtools\n";
         #sleep 2;
         system qq(pwd);
         chomp ($btoolsfilerev);
         $i="0";
         for ($i = 0; $parsebtools[$i] ; $i++ ) {
             if ( "$parsebtools[$i]" =~ /^date:/ ) {
                 print "\ndate: match $parsebtools[$i]\n";
                 $i++;
                 print "\nafter $i inc  :  $parsebtools[$i]\n\n";
                 push (@btoolsarray,"$esrc0\t$btoolsfilerev\t $parsebtools[$i]"); 
            }  # if close bracket
        }        # for close bracket
        
      }  # third foreach close bracket
}        # first foreach close bracket

@printarray=@finalchangelist;

#chomp(@printarray);
#chomp(@btoolsarray);

print "\n\nbtoolsarray length =:\n$#btoolsarray\n\n";

print "\n\nprintarray length =:\n$#printarray\n\n";

#chomp(@printarray);
#chomp(@btoolsarray);

# dump changes to sourcechangelist.txt  and btoolschangelist.txt  :  respectively

chdir "tdmf_ose/unix_v1.2/btools";

system qq(pwd);

# print results to respective files  :  read via bSMparlog.pl

open (FHcc,">sourcechangelist.tmp.txt");

foreach $CC (@printarray) {
   print FHcc "$CC";
   print "CC = $CC";
}

close (FHcc);

#remove duplicate entries
system qq(sort sourcechangelist.tmp.txt | uniq > sourcechangelist.txt);

open (FHbt,">btoolschangelist.tmp.txt");

foreach $BT (@btoolsarray) {
   print FHbt "$BT";
   print "BT = $BT";
}

close (FHbt);

#remove duplicate entries
system qq(sort btoolschangelist.tmp.txt | uniq > btoolschangelist.txt);

############################################
############################################

print "\n\nEND :  bRcvsWR.pl\n\n";

############################################
############################################

close(STDERR);
close(STDOUT);

