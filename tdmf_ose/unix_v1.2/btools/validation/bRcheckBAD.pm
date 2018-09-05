#!/usr/bin/perl -I../pms
############################################
############################################
#  checkBAD.pm
############################################
############################################

sub checkBAD {

print "\n\nSTART  :  bRcheckBAD.pm  :  checkBAD() \n\n";
require "bRglobalvars.pm";
require "bRdevvars.pm";

eval 'require bRstrippedbuildnumber';
eval 'require bPWD';
eval 'require bRbadFiles';

#################################################
###############################################
#  bPWD ::  establishPWD
###############################################
#################################################

establishPWD();

print "\n\nCWD  =  $CWD\n\n";

if (%badFile) {  print "\nIF\n"; }
print "\nstrippedbuildnumber = $stripbuildnum\n";

chdir "../builds/$Bbnum";

system qq(pwd);
                                                                         
   while (my($key,$value) = each(%badFiles)) {
      print "\nkey = $key  :  value = $value\n";

      if (! -e $key) {
         print "\nIF MISSING  :  $key\n";
         push (@badStatus,"RFX BAD  :  $key");
      }
   }

print "\nbadStatus length = $#badStatus\n";
print "\nbadStatus        =\n@badStatus\n\n";

chdir "$CWD";

system qq(pwd);

chdir "../builds/tuip/$Bbnum";

system qq(pwd);
                                                                         
   while (my($key,$value) = each(%badFiles)) {
      $key =~ s/Replicator/TDMFIP/;
      print "\nkey = $key  :  value = $value\n";
      if (! -e $key) {
         print "\nIF MISSING  :  $key\n";
         push (@badStatus,"TUIP BAD  :  $key");
      }
   }

chdir "$CWD";

system qq(pwd);

print "\n\nEND  :  bRcheckBAD.pm  :  checkBAD() \n\n";

return(@badStatus);

}  # close bracket checkdelstatus 

