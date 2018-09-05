#!/usr/local/bin/perl -w
##################################
###############################
#
#  perl -w bRparlog.pl
#
#	parse build log $RFWver.$Bbnum.txt
#
#	produce formatted email distribution notification
#	
###############################
#################################

#################################
###############################
# globals 
###############################
#################################

require "bRglobalvars.pm";
require "bRcvstag.pm";
require "bRcvslasttag.pm";
$CVSTAG="$CVSTAG";
$LASTCVSTAG="$LASTCVSTAG";
$buildnumber="$buildnumber";
$RFWver="$RFWver";
$Bbnum="$Bbnum";

open (STDOUT, ">PARSELOG.txt");
open (STDERR, ">>&STDOUT");

print "\n\n\nSTART  :  bRparlog.pl\n\n\n";

##############################
#  grab date / time :  print in notification
##############################

$datepackage='bDATE';

eval "require $datepackage";

get_time();

print "\ntime_out = \n$time_out[1] :  $time_out[0];\n";

$timeX="$time_out[1]:$time_out[0]";

get_date();

print "\ndate_out = \n$date_out[1] :  $date_out[0]:  $date_out[2];\n";

$dateX="$date_out[1]/$date_out[0]/$date_out[2]";

chomp ($timeX);
chomp ($dateX);

##################################
################################
#  read logs  :   grab strings
################################
##################################

##################################
################################
#  get installshield setup.inx error status
################################
##################################

open (FHrfw, "<$RFWver.$Bbnum.txt");
@RFWlog=<FHrfw>;
close (FHrfw);

foreach $ez0 (@RFWlog) {
   #print "\nFOREACH  :  $RFWver.$Bbnum.txt log entry  :  $ez0\n\n";
   if ("$ez0" =~ /setup.inx/ && ("$ez0" =~ /\s[0-9]\serror/ || "$ez0" =~ /[0-9]\serror/ ) ) {
      print "\n\nIF setup.inx found  :  $ez0\n\n";
      push (@errormsg0,"$ez0");
   }
}         

##################################
################################
#
#  devenv  :  .Net  :  requires special parsing  :
#
#	This be the special parsing section  :
#
################################
##################################

#NT Config LOG

open (FHNT,"<..\\NTLOG.txt")  ||  warn "NTLOG.txt not readable  :  $!";
@NTNet=<FHNT>;
close (FHNT);

#print "\n\nNTNet array  =  \n\n@NTNet\n\n  :  END  NTNet array print\n\n";

foreach $eNT (@NTNet) {
   if ( $eNT =~ m/succeeded/ && $eNT =~ m/failed/ && $eNT =~ m/skipped/) {
      print "\nNT Configuration : $eNT\n";
      $NT="NTNet Configuration : $eNT";
     push (@errormsg0,"$NT");
     }
}

#W2K Config LOG

open (FHW2K,"<..\\W2kLOG.txt")  ||  warn "W2kLOG.txt not readable  :  $!";
@WNet=<FHW2K>;
close (FHW2K);

#print "\n\nWNet array  =  \n\n@WNet\n\n  :  END  WNet array print\n\n";

foreach $eW (@WNet) {
   if ( $eW =~ m/succeeded/ && $eW =~ m/failed/ && $eW =~ m/skipped/) {
      print "\nW2K Configuration : $eW\n";
      $W2K="W2K Configuration : $eW";
     push (@errormsg0,"$W2K");
     }
}

##################################
###############################
#  alphabetize return code list 
#
#	ignore lower case vs upper case 
#
#	{ lc($a) cmp lc($b) }  :  this sorts via ignore case.	
#
###############################
#################################

@errormsg0 = sort  { lc($a) cmp lc($b) } @errormsg0;

##################################
###############################
#
#  gather source changes / frequency of changes
#  information "per build"
#
#  add $length to beginning email message
#  append source changes list to end of email message 
#	from scripts bRcvsWR.pl
###############################
#################################

print "\n\nSTART section frequency and source changes\n\n";

system qq(xcopy /K /V /Y ..\\..\\..\\..\\..\\..\\..\\sourcechangelist.txt);

open ( FHX9, "<sourcechangelist.txt" );
@sourcelist=<FHX9>;
close (FHX9);

$i="0";

for ($i=0 ; $sourcelist[$i] ; $i++) {
      print "\nFOR : index : sourcelist[$i] : $sourcelist[$i]\n";
      if ( $sourcelist[$i] !~ m/./ ) {
         print "\nIF :  splice newline match : $sourcelist[$i]\n";
         splice ( @sourcelist, $i, 1);
         $i=$i - 1;
          print "\n\nsourcechangelist : i after subtract - 1 = :  $i\n";
      }

}

$sourcelistlength=$#sourcelist + 1;

print "\nsourcelist array = \n @sourcelist\n";
print "\nsourcelistlenght array length = $sourcelistlength\n";

chomp (@sourcelist);

# adding in btools tracking, but, will keep it separate

system qq(xcopy /K /V /Y ..\\..\\..\\..\\..\\..\\..\\btoolschangelist.txt);

open ( FHBTOOLS, "<btoolschangelist.txt" );
@btoolslist=<FHBTOOLS>;
close (FHBTOOLS);

$i="0";

for ($i=0 ; $btoolslist[$i] ; $i++) {
      print "\nFOR : index : btoolslist[$i] : $btoolslist[$i]\n";
      if ( $btoolslist[$i] !~ m/./ ) {
         print "\nIF :  splice newline match : $btoolslist[$i]\n";
         splice ( @btoolslist, $i, 1);
         $i=$i - 1;
          print "\n\nbtoolslist : i after subtract - 1 = :  $i\n";
      }

}

$btoolslistlength=$#btoolslist + 1;

print "\nbtoolslist array = \n @btoolslist\n";
print "\nbtoolslistlength array length = $btoolslistlength\n";

chomp (@sourcelist);

print "\n\n\tEND section frequency and source changes\n\n";

##################################
###############################
#
#  Success or Failure  :  parsing  
#
#	did we find anything rc != 0
#
#	first implementation  :  2003/10/15  :  see if it works....   
#  
###############################
#################################


print "\n\nSTART  :  Success or Failure  Routine\n\n";

print "\n\nerrormsg0 array =\n@errormsg0\n";

foreach $ei0 (@errormsg0) {

   if ( "$ei0" =~ /NTNet\sConfig/ || "$ei0" =~ /W2K\sConfig/ ) {
      #print "\nIF  :  succeed or failure routine :  $ei0\n";
      if ( "$ei0" =~ /\s0 error\(s\)/ ) {
               print "\nIF error 0 :  $ei0\n";
      }
      elsif ( "$ei0" =~ /setup.inx/ && "$ei0" =~ /\s[0-9] error\(s\)/ ) {
               print "\nELSIF error [0-9] :  $ei0\n";
               push (@SUC_FAIL,"$ei0");
      }
      elsif ( "$ei0" =~ /NTNet\sConfig/ && "$ei0" =~ /\s0\sfailed/ ) {
               print "\nELSIF NTNet  :  rc = 0  :  $ei0\n";
      }
      elsif ( "$ei0" =~ /NTNet\sConfig/ && "$ei0" =~ /[0-9]\sfailed/  || "$ei0" =~ /[1-9][0-9]\sfailed/ ) {
               print "\nELSIF NTNet  :  rc = [1-9]  :  $ei0\n";
               push (@SUC_FAIL,"$ei0");
      }
      elsif ( "$ei0" =~ /W2K\sConfig/ && "$ei0" =~ /\s0\sfailed/ ) {
               print "\nELSIF W2KNet  :  rc = 0  :  $ei0\n";
      }
      elsif ( "$ei0" =~ /W2K\sConfig/ && "$ei0" =~ /[0-9]\sfailed/  || "$ei0" =~ /[1-9][0-9]\sfailed/ ) {
               print "\nELSIF W2KNet  :  rc = [1-9]  :  $ei0\n";
               push (@SUC_FAIL,"$ei0");
      }
   }
}
chomp (@SUC_FAIL);

# remove leading space from element  ....
foreach $eSF0 (@SUC_FAIL) {

   if ( $eSF0 =~ s/^\ // ) {
      print "\nMATCHED space :  $eSF0\n";
      push (@SUC_FAIL0, "$eSF0");
   }
   else {
      print "\nMATCHED space :  $eSF0\n";
      push (@SUC_FAIL0, "$eSF0"); 
   }
}

if ($#SUC_FAIL0 > "-1") {
   chomp($SUCCEED_FAILURE="\"FAILURE\";");
}
else {
   chomp($SUCCEED_FAILURE="\"compiles_succeeded\";");
}

unlink "bRsuc_fail.pm";

open (FHsfpm,">bRsuc_fail.pm");

print "\n\nprint to bRsuc_fail.pm  :  \$SUCCEED_FAILURE=\"$SUCCEED_FAILURE\"";
print FHsfpm "\$SUCCEED_FAILURE=$SUCCEED_FAILURE";

close (FHsfpm);

##################################
###############################
#
#  ok, take @errormsg0 and dump to EMAILDIST.txt
#	to email to "appropriate" list via script bRemail.pl 
#
###############################
#################################

chomp (@errormsg0);

open (ERRMSG0, ">EMAILDIST.txt");

#################################
###############################
#
# add build output location to ERRMSGDIST.txt
#
#	should be 129.212.65.20
#	/builds/Replicator/Windows/$RFWver/$Bbnum
#
#################################
#################################

print ERRMSG0 "\n$RFWver BUILD $buildnumber Date  :  $dateX\tTime  :  $timeX\n";
print ERRMSG0 "\n$RFWver BUILD $buildnumber DELIVERABLES :\tAVAILABLE AT : \n";
print ERRMSG0 "\nhttp server (SVL) : \t\t\thttp://129.212.65.20/builds/Replicator/Windows/$RFWver/$Bbnum\n";
print ERRMSG0 "\nftp (Sunnyvale) : \t\t\tftp 129.212.65.20\n";
print ERRMSG0 "\ninstall image  : \t\t\t\t$RFWver.$Bbnum.ALL.zip\n";
print ERRMSG0 "\ninstall iso image  : \t\t\t$RFWver.$Bbnum.iso\n";
print ERRMSG0 "\nStoneFly iso image  : \t\t\t$RFWver.$Bbnum.StoneFly.iso\n";
print ERRMSG0 "\nCVS tag   : \t\t\t\t$CVSTAG\n";
print ERRMSG0 "\ncorecode source files changed :   =\t$sourcelistlength\n";
print ERRMSG0 "\nbuild related scripts changed :   =\t$btoolslistlength\n";
print ERRMSG0 "\nSource change list created from cvs tags :\n";
print ERRMSG0 "\n$CVSTAG  :  to  :  $LASTCVSTAG\n";

#  if @SUC_FAIL0 contains even 1 element  :  
#	run this, else, do not add section

print "\n\nSUC_FAIL0 Length = $#SUC_FAIL0\n\n";
if ("$#SUC_FAIL0" > "-1" ) {
print ERRMSG0 "\n\n======$Bbnum  :  Projects / Solutions Reporting  $SUCCEED_FAILURE======\n\n";

   foreach $eSFx0 (@SUC_FAIL0) {
      print ERRMSG0 "$eSFx0\n";
      #print ERRMSG0 "$eSFx0\n";
  }
}

#dump projects with rc > 0 above  : or print nothing if SUCCEED is achieved

#  Return Code Print Section

print ERRMSG0 "\n\n=======================$Bbnum======================\n\n";
print ERRMSG0 "\n$RFWver :  Return Codes\n\n";

foreach $elem4 (@errormsg0) {
   print ERRMSG0 "$elem4\n";
   #print "\nERRMSG : $elem4\n";

}  # foreach close bracket

# Source change lists  :  core code  &  btools  :  respectively

# core code  

print ERRMSG0 "\n\n=====================$Bbnum===================\n";
print ERRMSG0 "\nSource File :    File Rev :     WR / Message list :     $Bbnum\n";
print ERRMSG0 "\n=====================$Bbnum===================\n\n";

foreach $elemCC0 (@sourcelist) {
print ERRMSG0 "$elemCC0\n";
print "\nCore Code : $elemCC0\n";
}

print ERRMSG0 "\n\n=====================$Bbnum================\n\n";

# btools

# core code  

print ERRMSG0 "\n\n=====================$Bbnum===================\n";
print ERRMSG0 "\nBuild Script  :  Environment Changes  :  btools etc.\n";
print ERRMSG0 "\nSource File :    File Rev :     WR / Message list :     $Bbnum\n";
print ERRMSG0 "\n=====================$Bbnum===================\n\n";

foreach $elemBT (@btoolslist) {
print ERRMSG0 "$elemBT";
print "\nbtools : $elemBT\n";
}

close (ERRMSG0);

print "\nEnd bRparlog.pl\n";

########################################
########################################

close (STDOUT);
close (STDERR);
