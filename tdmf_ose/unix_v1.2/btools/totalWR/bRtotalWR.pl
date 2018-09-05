#!/usr/local/bin/perl -w
################################
##############################
# 
#   perl -w ./bXtotalWR.pl
#
#	determine source differences between initial build tag and
#	and current build tag
#
#	group list by WR number
#
#	must be run from root of cvs workarea 
#
###############################
################################

################################
##############################
# global  vars 
##############################
################################

#manual set vars per start of list
$LASTCVSTAG="RFXHEAD+20051115+B04400";
$lastGMT="2005-11-15";

require "bRglobalvars.pm";
require "bRcvstag.pm";
require "bRlastbuildGMTdate.pm";
$Bbnum="$Bbnum";
$RFXver="$RFXver";
$CVSROOT="$CVSROOT";
$cvs="cvs -d $CVSROOT";

########################################
######################################
# STDERR, STDOUT
######################################
########################################

open (STDOUT, ">WRtotal.txt");
open (STDERR, ">\&STDOUT");

print "\n\nSTART :   bRtotalWR.pl\n\n";

system qq(pwd);

########################################
######################################
# set $currentGMT 
######################################
########################################

eval "require 'bDATE.pm'";
@gmtdate=get_GMT_date();
$currentGMT="$gmtdate[1]-$gmtdate[0]-$gmtdate[2]";
print "\n\ncurrentGMT  =  $currentGMT\n\n";

########################################
######################################
# begin processing 
######################################
########################################
#must be run from root of cvs workarea

chdir "../../../.." || die "chdir to ../../../.. failed  :  $!";

system qq(pwd);

print "\n\n qx ! $cvs -Q rdiff -s -r $CVSTAG -r $LASTCVSTAG tdmf_ose/unix_v1.2 ! \n\n";
@list= qx ! $cvs -Q rdiff -s -r $CVSTAG -r $LASTCVSTAG tdmf_ose/unix_v1.2 !;

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
#  process each file via $cvs log -N -d"date GMT > lastbuilddateGMT"  : 
#
#	produce printable list for change list
####################################
####################################

chomp (@keepparsing);

#########################################
# new adds
#########################################

@srcchangelist=@keepparsing;

system qq(pwd);

foreach $esrc0 (@srcchangelist) {
   undef(@revlist);
   undef(@parseforWR);
   #undef(@endofarray);
   print "\n\nparseforrev= qx| $cvs log $branchid -N -d$currentGMT 23:59 GMT > $lastGMT 00:00 GMT \"$esrc0\" \n\n";
   #@parseforrev= qx| $cvs log $branchid -N -d"$currentGMT 23:59 GMT > $lastGMT 00:00 GMT" \"$esrc0\" |;
   @parseforrev=qx|$cvs log $branchid -N -d"$currentGMT 23:59 GMT > $lastGMT 00:00 GMT" $esrc0|;
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
                 push (@finalchangelist,"$esrc0 $filerev $parseforWR[$i] @endofarray "); 
                 print "\n\n\nTO  :  $esrc0  :  $filerev  :  $parseforWR[$i]  :  @endofarray\n\n\n";
                 undef($parseforWR[$i]);
                 undef(@endofarray);
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

system qq(pwd);
cheatremove();
sub cheatremove {
foreach $esrc0 (@trackbtools) {
   undef(@btoolsrevlist);
   undef(@parsebtools);
   print "\n\nFOREACH  :  btools  :  esrc0  ;  $esrc0\n\n"; 
   print "\n\nbtoolsrev= qx| $cvs log $branchid -N -d$currentGMT 23:59  GMT > $lastGMT 00:00 GMT  \"$esrc0\" \n\n";
   @btoolsrev= qx| $cvs log $branchid -N -d"$currentGMT 23:59  GMT > $lastGMT 00:00 GMT" $esrc0 |;
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
         $esrc0 =~ s/^(\s+)//g;
         $esrc0 =~ s/^\n//g;
         $esrc0 =~ s/^(\s+)//g;
         print "\nbtoolsfilerev =\n$btoolsfilerev\n";
         print "\nesrc0 =\n$esrc0\n";
         chomp ($btoolsfilerev);
         chomp ($btoolsfilerev);
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
                 chomp ($parsebtools[$i]);
                 push (@btoolsarray,"$esrc0 $btoolsfilerev $parsebtools[$i] "); 
            }  # if close bracket
        }        # for close bracket
        
      }  # third foreach close bracket
}        # first foreach close bracket
} #sub cheatremove close bracket
  #do not need to run btools during test phase
#########################################
#  end new adds  :  btools
#########################################

@printarray=@finalchangelist;

#chomp(@printarray);
#chomp(@btoolsarray);

print "\n\nbtoolsarray length =:\n$#btoolsarray\n\n";

print "\n\nprintarray length =:\n$#printarray\n\n";

#chomp(@printarray);
#chomp(@btoolsarray);

# dump changes to sourcechangelist.txt  and btoolschangelist.txt  :  respectively

chdir "tdmf_ose/unix_v1.2/btools/totalWR";

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
#  create list of WR's sorted by source change to WR
#
#	format  :  WR   List of changes to the WR ....
############################################
############################################

chomp(@groupWRs=(@printarray));

chomp (@btoolsarray);
foreach $eBTOOLS (@btoolsarray) {
   chomp($eBTOOLS);
   print "\n\neBTOOLS  =  $eBTOOLS\n\n";
   #if ("$eBTOOLS" !~ /buildcommitstring/g || ("$eBTOOLS" !~ /Build/g && "$eBTOOLS" !~ /\srun/g) ) {
   if ("$eBTOOLS" =~ /buildcommitstring/g ) {
      print "\nIF  :  here $eBTOOLS !~ /buildcommitstring/g\n";
      push (@btoolsLINES,"$eBTOOLS ");
   }
   elsif ("$eBTOOLS" =~ /Build/g && "$eBTOOLS" =~ /\s+run/g ) {
      print "\nIF  :  here $eBTOOLS !~ /Build run/g\n";
      push (@btoolsLINES,"$eBTOOLS ");
   }
   elsif ("$eBTOOLS" =~ /build/g && "$eBTOOLS" =~ /\s+commit/g ) {
      print "\nIF  :  here $eBTOOLS !~ /Build run/g\n";
      push (@btoolsLINES,"$eBTOOLS ");
   }
   else {
      print "\nELSEHERE  : $eBTOOLS\n\n";
      push (@groupWRs,"$eBTOOLS");
   }
}


#chomp(@groupWRs=`cat sourcechangelist.txt`);

foreach $eLIST (@groupWRs) {
   $eLIST     =~  s/(WR\:\s+)(\d+){0,5}/WR$2/ig;  #WR: colon space consistency
   print "\n\neLIST  :  $eLIST\n";
   $eLIST     =~  s/(WR\s+)(\d+){0,5}/WR$2/ig;  #WR space consistency
   #$eLIST     =~  s/(wr)(\d+){0,5}/WR$2/g;  #WR space consistency
   $eLIST     =~  s/(wr)(\d+)/WR$2/g;  #WR space consistency
   $eLIST     =~  s/(MR\s+)(\d+){0,5}/WR$2/ig;  #MR used  :  great....
   $eLIST     =~  s/$/ /;  #add space to end of line
   $eLIST     =~  /^(.*?)\s+(\d+.*?)\s+(.*)$/;
   $one = "$1"; $two = "$2" ; $three = "$3";
   #if ( $three =~ /\D/ || $three =~ /\d/ ) {
   if ( $three =~ /^WR/ || $three =~ /^WR.*\d|^WR.*\D/ || $three =~ /WR(\d+){5}/ || $three =~ /WR\s+(\d+){5}/  ) {
      #print "IF  :  \n1 = $one 2 = $two 3 = $three\n";
      push (@needsort,"\n$three $one $two");
   }
   elsif ( $three =~ /WR\:\s+(\d+){5}/ || $three =~ /\s+(\d+){5}\s+/ || $three =~ /MR\s+(\d+){5}/ ) {
      #print "IF  :  \n1 = $one 2 = $two 3 = $three\n";
      push (@needsort,"\n$three $one $two");
   }
   elsif ( $three =~ /^wr(\d+){5}/ || $three =~ /wr.*(\d+){5}/ || $three =~ /^\d\d\d\d\d/ ) {
      #print "IF  :  \n1 = $one 2 = $two 3 = $three\n";
      push (@needsort,"\n$three $one $two");
   }
   elsif ( $one =~ /btools/ ) {
      #print "IF  :  \n1 = $one 2 = $two 3 = $three\n";
      push (@needsort,"\nbtools NO WR  :  $three $one $two");
   }
   else {
      #print "ELSE  :  \n1 = $one 2 = $two\n";
      push (@needsort,"\nNO WR  :  $three $one $two");
   }
   undef($one); undef($two); undef($three);
}

@sorted = sort (@needsort);
print "\nsorted =\n@sorted\n";

open (FHtotal,">WRtotallist.txt");
#print FHtotal "\n@sorted\n";
$k="0";
print FHtotal "\n########################################################\n";
print FHtotal "\n########################################################\n";
print FHtotal "\n\nCVS tag rdiff $CVSTAG to $LASTCVSTAG \n\n";
print FHtotal "\n########################################################\n";
print FHtotal "\n########################################################\n";

foreach $eLONGLINE (@sorted) {
   print FHtotal "\n$eLONGLINE";
   #print FHtotal "\n$k  :  $eLONGLINE";
   $k++;
}
close (FHtotal);
############################################
##########################################
# ncftp WRtotallist.txt to build server
##########################################
############################################

system qq(ncftpput -f ../bmachine.txt -m /builds/Replicator/UNIX/$RFXver/WRlist WRtotallist.txt);

#unlink "bmachine.txt";

############################################
############################################

print "\n\nEND :  bRtotalWR.pl\n\n";

############################################
############################################

close(STDERR);
close(STDOUT);
