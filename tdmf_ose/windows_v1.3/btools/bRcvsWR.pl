#!/usr/local/bin/perl -w
################################
##############################
# 
#   perl -w ./bRcvsWR.pl
#
#	determine source differences between last build tag and
#	and current build tag
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

require "bRglobalvars.pm";
require "bRcvslasttag.pm";
require "bRcvstag.pm";
require "bRlastbuildGMTdate.pm";
$Bbnum="$Bbnum";
$RFWver="$RFWver";
$lastbuildGMTdate="$lastbuildYEAR-$lastbuildMONTH-$lastbuildDAY";
$lastGMT="$lastbuildGMTdate";

########################################
######################################
# STDERR, STDOUT
######################################
########################################

open (STDOUT, ">WRlog.txt");
open (STDERR, "\&STDOUT");

print "\n\nSTART :   bRcvsWR.pl :  script : cvs : to determine source changes\n\n";

system qq($cd);

print "\n\$lastbuildYEAR-\$lastbuildMONTH-\$lastbuildDAY = $lastbuildYEAR-$lastbuildMONTH-$lastbuildDAY\n\n";

########################################
######################################
# get GMT date
######################################
########################################

$datepackage='bDATE';

eval "require $datepackage";

@gmtdate=get_GMT_date();

print "\n\nGMT  :\t$gmtdate[1]-$gmtdate[0]-$gmtdate[2]\n\n";

#cvs seems to like this date format  ==>> use it....
$currentGMT="$gmtdate[1]-$gmtdate[0]-$gmtdate[2]";

if ($#gmtdate > -1) {

   print "\n\ngmtdatelength =  $#gmtdate\n\n";
   open (FHlast,">bRlastbuildGMTdate.pm");
   print FHlast "\$lastbuildYEAR=\"$gmtdate[1]\"\;\n";
   print FHlast "\$lastbuildMONTH=\"$gmtdate[0]\"\;\n";
   print FHlast "\$lastbuildDAY=\"$gmtdate[2]\"\;\n";
   print FHlast "1\;";    # or returns non-true value

}
else {
   print "\ngmtdatelength =  $#gmtdate\n\n";
   print "\ncall to bDATE.pm  :  sub get_GMT_date  :  errors occured\n";
}

print "\n\nsaved  : lastbuildGMTdate = $lastbuildGMTdate  :  to bRlastbuildGMTdate.pm\n\n";

#exit();

#####################################
###################################
#  start parsing processes
###################################
#####################################

#script must be run from root of cvs workarea

chdir "..\\..\\..\\..\\..\\..\\..";
#chdir "..\\..\\..";

system qq($cd);

print "\n\n qx ! cvs -Q rdiff -s -r $CVSTAG -r $LASTCVSTAG tdmf_ose/windows_v1.3 ! \n\n";
@list= qx ! cvs -Q rdiff -s -r $CVSTAG -r $LASTCVSTAG SSM tdmf_ose/windows_v1.3 !;

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

chomp (@keepparsing);

#########################################
# new adds
#########################################

@srcchangelist=@keepparsing;

system qq($cd);

foreach $esrc0 (@srcchangelist) {
   undef(@revlist);
   undef(@parseforWR);

   print "\n\nparseforrev= qx|cvs log $branchid -N -d$currentGMT 23:59 GMT > $lastGMT 00:00 GMT \"$esrc0\" \n\n";

   @parseforrev= qx|cvs log $branchid -N -d"$currentGMT 23:59 GMT > $lastGMT 00:00 GMT" \"$esrc0\" |;

   #print "\nparseforrev = \n@parseforrev";

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
         print "\nfilerev =\n$filerev\n";
        @parseforWR= qx|cvs log -N -r$filerev \"$esrc0\" |;
        print "\nparseforWR =\n@parseforWR\n";
        #sleep 2;
chomp ($filerev);
         $i="0";
         for ($i = 0; $parseforWR[$i] ; $i++ ) {
             if ( "$parseforWR[$i]" =~ /^date:/ ) {
                 print "\ndate: match $parseforWR[$i]\n";
                 $i++;
                 print "\nafter $i inc  :  $parseforWR[$i]\n\n";
                 push (@finalchangelist,"$esrc0\t$filerev\t $parseforWR[$i]"); 
            }  # if close bracket
       }        # for close bracket
     }  # third foreach close bracket

}        # first foreach close bracket

#########################################
# end new adds
#########################################

#}      #Main foreach  :  close bracket

print "\n\nfinalchangelist  =\n@finalchangelist\n";    

#######################################
#######################################
#  same routine / file processing as above  :
#
#	except  :  process build script changes  :  btools
#######################################
#######################################

chomp (@trackbtools);

#########################################
# new adds  :  btools
#########################################

system qq($cd);

foreach $esrc0 (@trackbtools) {
   undef(@btoolsrevlist);
   undef(@parsebtools);

   print "\n\nFOREACH  :  btools  :  esrc0  ;  $esrc0\n\n"; 

   print "\n\nbtoolsrev= qx|cvs log $branchid -N -d$currentGMT 23:59  GMT > $lastGMT 00:00 GMT  \"$esrc0\" \n\n";

   @btoolsrev= qx|cvs log $branchid -N -d"$currentGMT 23:59  GMT > $lastGMT 00:00 GMT " \"$esrc0\" |;

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
        @parsebtools= qx|cvs log -N -r$btoolsfilerev \"$esrc0\" |;
        print "\nparsebtools =\n@parsebtools\n";
        #sleep 2;
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

#########################################
#  end new adds  :  btools
#########################################

print "\n\nbtoolsarray length =:\n$#btoolsarray\n\n";

@printarray=@finalchangelist;

print "\n\nprintarray length =:\n$#printarray\n\n";

#chomp(@printarray);
#chomp(@btoolsarray);

# dump changes to sourcechangelist.txt  and btoolschangelist.txt  :  respectively

#chdir "tdmf_ose\\windows_v1.3\\builds\\$Bbnum\\tdmf_ose\\windows_v1.3\\btools";

system qq($cd);

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
