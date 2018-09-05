#!/usr/bin/perl -I../pms
############################################
############################################
#  checkOMP.pm
############################################
############################################

sub checkOMP {

print "\n\nSTART  :  bRcheckOMP.pm  :  checkOMP() \n\n";
require "bRglobalvars.pm";
require "bRdevvars.pm";

eval 'require bRstrippedbuildnumber';
eval 'require bPWD';
eval 'require bRompFiles';

#################################################
###############################################
#  bPWD ::  establishPWD
###############################################
#################################################

establishPWD();

print "\n\nCWD  =  $CWD\n\n";

if (%ompFile) {  print "\nIF\n"; }
print "\nstrippedbuildnumber = $stripbuildnum\n";

chdir "../builds/$Bbnum";

system qq(pwd);
                                                                         
   while (my($key,$value) = each(%ompFiles)) {
      print "\nkey = $key  :  value = $value\n";

      if (! -e $key) {
         print "\nIF MISSING  :  $key\n";
         push (@ompStatus,"RFX OMP  :  $key");
      }
   }

print "\nompStatus length = $#ompStatus\n";
print "\nompStatus        =\n@ompStatus\n\n";

chdir "$CWD";

system qq(pwd);

chdir "../builds/tuip/$Bbnum";

system qq(pwd);
                                                                         
   while (my($key,$value) = each(%ompFiles)) {
      print "\nkey = $key  :  value = $value\n";

      if (! -e $key) {
         print "\nIF MISSING  :  $key\n";
         push (@ompStatus,"TUIP OMP  :  $key");
      }
   }

chdir "$CWD";

system qq(pwd);

print "\n\nEND  :  bRcheckOMP.pm  :  checkOMP() \n\n";

return(@ompStatus);

}  # close bracket checkdelstatus 

