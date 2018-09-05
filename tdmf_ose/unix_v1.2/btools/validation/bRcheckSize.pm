#!/usr/bin/perl -I../pms
###########################################
#########################################
#  bRcheckSize.pm 
#
#		diff deliverable  list / size list against
#		known good list of files / size
#########################################
###########################################

###########################################
#########################################
#  globals
#########################################
###########################################

require "bRglobalvars.pm";
require "bRdevvars.pm";

eval 'require bRPathFileSize';

###########################################
#########################################
#  sub  :  checksize 
#########################################
###########################################

sub checksize {

   print "\n\nSTART  :  bRcheck_size.pm\n\n";

   $i="0";

   chdir "../builds/$Bbnum";

   system qq(pwd);
 
   while (my($key,$value) = each(%PathFileSizeRFX)) {
      print "\nkey = $key  :  value = $value\n";

      if ( -e $key) {
         print "\nFOUND  :  $key\n";
         push (@FOUND,"\n$key");
         $currentsize= (stat("$key"))[7];
         print "\nvalue = $value  :  currentsize = $currentsize\n";
         if ("$value" > "$currentsize") {
            push (@warnfilesize,"$key  :  file size = $currentsize  :  expected size > $value");
         }
      }
      else {
         push (@validatemissing,"$key");
      }
   }

   print "\nFOUND length  =  $#FOUND\n";
   print "\nvalidatemissing length  =  $#validatemissing\n";
   print "\nvalidatemissing (-1)  =  @validatemissing\n";
   print "\nwarnfilesize length  =  $#warnfilesize\n";
   print "\nwarnfilesize  = @warnfilesize\n";

   print "\n\nEND  :  bRcheck_size.pm\n\n";

   chdir "../../btools";   
   system qq(pwd);
   return (@warnfilesize,@validatemissing);

}  # close bracket checksize()

