#!/usr/bin/perl -w
########################################
######################################
#  perl -w ./bDATE.pm
#
#      call eval 'require bDATE' && associated functions
#
#     TEST script 
######################################
########################################

print "\n\nSTART   :  bDATE.pl\n\n";

eval 'require bDATE';

get_GMT_date();

print "\nGMTSECOND  =  $GMTSECOND\n";
print "\n\nGMTMINUTE  =  $GMTMINUTE\n";
print "\nGMTHOUR  =  $GMTHOUR\n";
print "\nGMTDAY  =  $GMTDAY\n";
print "\nGMTMONTH  =  $GMTMONTH\n";
print "\nGMTYEAR  =  $GMTYEAR\n";

print "\n\nEND   :  bDATE.pl\n\n";

