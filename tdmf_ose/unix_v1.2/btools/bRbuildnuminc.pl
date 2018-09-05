#!/usr/bin/perl -Ipms
######################################
####################################
#
# perl -w ./bRbuildnumberinc.pl
#
#	Build Number Incrementor
#
#	cvs commit bRglobalvars.pm before cvs tag routine
#
####################################
######################################

#####################################
####################################
#  vars  /  pms  /  use
####################################
#####################################

eval 'require bRglobalvars';

our $buildnumber;

####################################
##################################
# STDOUT  /  STDERR
##################################
####################################

open (STDOUT, ">logs/buildnuminc.txt");
open (STDERR, ">\&STDOUT");

####################################
##################################
#  exec statements 
##################################
####################################

print "\n\nSTART  :  bRbuildnuminc.pl\n\n";

####################################
##################################
#  increment build number by 1
#    log values 
##################################
####################################

$buildnumberinc=++$buildnumber;

print "\n\ncurrent build number  :  $buildnumberinc \n";

####################################
##################################
#  update bRglobalvars.pm
##################################
####################################

open (NEWBN, "<pms/bRglobalvars.pm");
@getbnum=<NEWBN>;
close (NEWBN);

open (WNEWBN, ">pms/bRglobalvars.pm");

foreach $eBN (@getbnum) {
   if ($eBN !~ m/\$buildnumber/) {
      #print "IF $eBN\n";
      print WNEWBN "$eBN";
   }
   else {
      #print "\nELSE $eBN\n";
      print $eBN if $eBN =~ s/(\d+)/$buildnumberinc/;
      print WNEWBN "$eBN";
   }
}

close (WNEWBN);

####################################
##################################
#  create $stripbuildnum
#    no padded zero's 
##################################
####################################

$stripbuildnum="$buildnumberinc";

print "\nstripbuildnum before prefix check :  $stripbuildnum\n";

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

print "\nstripbuildnum after  :  $stripbuildnum\n";

####################################
##################################
#  create pms/b(R)(I)strippedbuildnumber.pm 
##################################
####################################

open (FHstrip,">pms/bRstrippedbuildnumber.pm");

print FHstrip "\$stripbuildnum=\"$stripbuildnum\"\;";;

close (FHstrip);

open (FHistrip,">pms/bIstrippedbuildnumber.pm");

print FHistrip "\$stripbuildnum=\"$stripbuildnum\"\;";;

close (FHistrip);

####################################
##################################
#  create pms/b(R)(I)strippedbuildnumber.pm 
##################################
####################################

system qq(cvs commit -m "B$buildnumber  :  auto build commit" pms/bRglobalvars.pm pms/bRstrippedbuildnumber.pm pms/bIstrippedbuildnumber.pm);

####################################
####################################

print "\n\nEND  :  bRbuildnuminc.pl\n\n";

####################################
####################################

close (STDERR);
close (STDOUT);
