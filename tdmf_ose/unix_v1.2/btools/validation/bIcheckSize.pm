#!/usr/bin/perl -I../pms
###########################################
#########################################
#  bIcheckSize.pm 
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

eval 'require bIPathFileSize';

###########################################
#########################################
#  sub  :  checksize 
#########################################
###########################################

sub checksizetuip {

   print "\n\nSTART  :  bIcheck_size.pm\n\n";

   $i="0";

   chdir "../builds/tuip/$Bbnum";

   system qq(pwd);
 
   while (my($key,$value) = each(%PathFileSize)) {
      print "\nkey = $key  :  value = $value\n";

      if ( -e $key) {
         print "\nFOUND  :  $key\n";
         push (@FOUND,"\n$key");
         $currentsize= (stat("$key"))[7];
         print "\nvalue = $value  :  currentsize = $currentsize\n";
         if ("$value" > "$currentsize") {
            push (@warnfilesizetuip,"$key  :  file size = $currentsize  :  expected size > $value");
         }
      }
      else {
         push (@validatemissingtuip,"$key");
      }
   }

   print "\nFOUND length  =  $#FOUND\n";
   print "\nvalidatemissing length  =  $#validatemissingtuip\n";
   print "\nvalidatemissing (-1)  =  @validatemissingtuip\n";
   print "\nwarnfilesize length  =  $#warnfilesizetuip\n";
   print "\nwarnfilesize  = @warnfilesizetuip\n";

   chdir "../../../btools";   
   system qq(pwd);

   print "\n\nEND  :  bIcheck_size.pm\n\n";

   return (@warnfilesizetuip,@validatemissingtuip);

}  # close bracket checksize()

