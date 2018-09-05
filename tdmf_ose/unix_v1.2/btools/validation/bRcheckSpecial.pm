#!/usr/bin/perl -I../pms
############################################
############################################
#  checkdelstatus  :  check deliverables status
############################################
############################################

sub checkspecial {

print "\n\nSTART  :  bRcheck_special.pm \n\n";
require "bRglobalvars.pm";
require "bRdevvars.pm";

eval 'require bRPathFileSizeSpecial';

$SPECIALPATH="./specialimage/Softek/Replicator";

chdir "../builds/$Bbnum";

############################################
##########################################
# strip build number
##########################################
############################################

$stripbuildnum="$buildnumber";
 
print "\n$stripbuildnum\n";
 
if ( $stripbuildnum =~ /^0000\d+/ ) {
   print "\nIF 4 :  $stripbuildnum\n";
   $stripbuildnum =~ s/^0000//;
   print "\nAFTER  :  $stripbuildnum\n";
}
elsif ( $stripbuildnum =~ /^000\d+/ ) {
   print "\nELSIF 3  :  $stripbuildnum\n";
   $stripbuildnum =~ s/^000//;
   print "\nAFTER  :  $stripbuildnum\n";
}
elsif ( $stripbuildnum =~ /^00\d+/ ) {
   print "\nELSIF 2  :  $stripbuildnum\n";
   $stripbuildnum =~ s/^00//;
   print "\nAFTER  :  $stripbuildnum\n";
}
elsif ( $stripbuildnum =~ /^0\d+/ ) {
   print "\nELSIF 2  :  $stripbuildnum\n";
   $stripbuildnum =~ s/^0//;
   print "\nAFTER  :  $stripbuildnum\n";
}

system qq(pwd);

system qq(pwd);

#######################################
#####################################
#  %bRPathFileSizeSpecial check 
#####################################
#######################################

   while (my($key,$value) = each(%PathFileSizeSpecial)) {
      print "\nkey = $key  :  value = $value\n";

      if ( ! -e "./$SPECIALPATH/$key") {
         print "\nIF MISSING  :  ./$SPECIALPATH/$key\n";
         push (@missingspecial,"$SPECIALPATH/$key");
      }
   }


print "\nmissingspecial length = $#missingpecial\n";
print "\nmissingspecial = \n@missingspecial\n";
foreach $file (@missingspecial) { print "\n$file"; }

print "\n\nEND  :  bRcheck_special.pm\n\n";

chdir "../../btools";

system qq(pwd);

return(@missingspecial);
#return(@date_out,@date_out0,@FILELIST,@foundlist,@missingdeliverables);
#return(@date_out,@date_out0);  #  <<== syntax works to return 2 array list  save example

}  # close bracket checkdelstatus 

