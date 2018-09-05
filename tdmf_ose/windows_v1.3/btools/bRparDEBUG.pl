#!/usr/local/bin/perl -w
##################################
###############################
#
#  perl -w bRparDEBUG.pl
#
#	parse build DEBUG logs
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

open (STDOUT, ">parseDEBUG.txt");
open (STDERR, ">>&STDOUT");

print "\n\n\nSTART  :  bRparDEBUG.pl\n\n\n";

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
#
#  devenv  :  .Net  :  requires special parsing  :
#
#	This be the special parsing section  :
#
################################
##################################

#NT Debug Config LOG

open (FHNT,"<..\\NTDebug.txt")  ||  warn "NTDebug.txt not readable  :  $!";
@NTdebug=<FHNT>;
close (FHNT);

#print "\n\nNTNet array  =  \n\n@NTNet\n\n  :  END  NTNet array print\n\n";

foreach $eNT (@NTdebug) {
   if ( $eNT =~ m/succeeded/ && $eNT =~ m/failed/ && $eNT =~ m/skipped/) {
      print "\nNT Debug Configuration : $eNT\n";
      $NT="NTDebug Configuration : $eNT";
     push (@errormsg0,"$NT");
     }
}

#W2K_Debug Config LOG

open (FHW2K,"<..\\W2KDebug.txt")  ||  warn "W2KDebug.txt not readable  :  $!";
@W2KDebug=<FHW2K>;
close (FHW2K);

#print "\n\nWNet array  =  \n\n@WNet\n\n  :  END  WNet array print\n\n";

foreach $eW (@W2KDebug) {
   if ( $eW =~ m/succeeded/ && $eW =~ m/failed/ && $eW =~ m/skipped/) {
      print "\nW2K Debug Configuration : $eW\n";
      $W2K="W2K Debug Configuration : $eW";
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
#  ok, take @errormsg0 and dump to DEBUGbuild.txt
#	to email to "appropriate" list via script bRemail.pl 
#
###############################
#################################

chomp (@errormsg0);

open (ERRMSG0, ">DEBUGbuild.txt");

#################################
###############################
#
# add build output location to DEBUGbuild.txt
#
#	should be 129.212.65.20
#	/builds/Replicator/Windows/$RFWver/$Bbnum
#
#################################
#################################

print ERRMSG0 "\n$RFWver DEBUG BUILD $buildnumber Date  :  $dateX\tTime  :  $timeX\n";
print ERRMSG0 "\n$RFWver DEBUG BUILD $buildnumber DELIVERABLES :\tAVAILABLE AT : \n";
print ERRMSG0 "\nhttp server (SVL) : \t\t\thttp://129.212.65.20/builds/Replicator/Windows/$RFWver/$Bbnum\n";
print ERRMSG0 "\nftp (Sunnyvale) : \t\t\tftp 129.212.65.20\n";
print ERRMSG0 "\nzip  image  : \t\t\t\t$RFWver.$Bbnum.DEBUG.zip\n";
print ERRMSG0 "\nCVS tag   : \t\t\t\t$CVSTAG\n";

#dump projects with rc > 0 above  : or print nothing if SUCCEED is achieved

#  Return Code Print Section

print ERRMSG0 "\n\n=======================$Bbnum======================\n\n";
print ERRMSG0 "\n$RFWver :  Return Codes\n\n";

foreach $elem4 (@errormsg0) {
   print ERRMSG0 "$elem4\n";
   #print "\nERRMSG : $elem4\n";

}  # foreach close bracket


print ERRMSG0 "\n\n=====================$Bbnum===================\n";

close (ERRMSG0);

print "\nEnd bRparDEBUG.pl\n";

########################################
########################################

close (STDOUT);
close (STDERR);
