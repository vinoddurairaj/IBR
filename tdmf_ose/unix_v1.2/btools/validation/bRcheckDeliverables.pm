#!/usr/bin/perl -I../pms
############################################
############################################
#  checkdelstatus  :  check deliverables status
############################################
############################################

sub checkdelstatus {

print "\n\nSTART  :  bRcheck_deliverables.pm  :  checkdelstatus() \n\n";
require "bRglobalvars.pm";
require "bRdevvars.pm";

eval 'require bRstrippedbuildnumber';

eval 'require bRPathFileSize';

#
if (%PathFileSize) {  print "\nIF\n"; }
print "\nstrippedbuildnumber = $stripbuildnum\n";
#exit();

chdir "../builds/$Bbnum";

system qq(pwd);
                                                                         
   while (my($key,$value) = each(%PathFileSizeRFX)) {
      print "\nkey = $key  :  value = $value\n";

      if (! -e $key) {
         print "\nIF MISSING  :  $key\n";
         push (@missingdeliverables,"$key");
      }
   }

print "\nmissingdeliverables length = $#missingdeliverables\n";
print "\nmissingdeliverables = \n@missingdeliverables\n";
foreach $file (@missingdeliverables) { print "\n$file"; }

print "\n\nEND  :  bRcheck_deliverables.pm  :  checkdelstatus() \n\n";

chdir "../../btools";

system qq(pwd);

return(@missingdeliverables);
#return(@date_out,@date_out0,@FILELIST,@foundlist,@missingdeliverables);
#return(@date_out,@date_out0);  #  <<== syntax works to return 2 array list  save example

}  # close bracket checkdelstatus 

