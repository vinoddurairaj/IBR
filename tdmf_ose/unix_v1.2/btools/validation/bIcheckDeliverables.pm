#!/usr/bin/perl -I../pms
############################################
############################################
#  checkdelstatus  :  check deliverables status
############################################
############################################

sub checkdelstatustuip {

print "\n\nSTART  :  bIcheck_deliverables.pm  :  checkdelstatustuip() \n\n";

require "bRglobalvars.pm";
require "bRdevvars.pm";

eval 'require bIstrippedbuildnumber';

eval 'require bIPathFileSize';

#
if (%PathFileSize) {  print "\nIF\n"; }
print "\nstrippedbuildnumber = $stripbuildnum\n";
#exit();

chdir "../builds/tuip/$Bbnum";

system qq(pwd);
                                                                         
   while (my($key,$value) = each(%PathFileSize)) {
      print "\nkey = $key  :  value = $value\n";

      if (! -e $key) {
         print "\nIF MISSING  :  $key\n";
         push (@missingdeliverablestuip,"$key");
      }
   }

print "\nmissingdeliverables length = $#missingdeliverablestuip\n";
print "\nmissingdeliverables = \n@missingdeliverablestuip\n";
foreach $file (@missingdeliverablestuip) { print "\n$file"; }

print "\n\nEND  :  bIcheck_deliverables.pm  :  checkdelstatus() \n\n";

chdir "../../../btools";

system qq(pwd);

return(@missingdeliverablestuip);
#return(@date_out,@date_out0,@FILELIST,@foundlist,@missingdeliverables);
#return(@date_out,@date_out0);  #  <<== syntax works to return 2 array list  save example

}  # close bracket checkdelstatus 

