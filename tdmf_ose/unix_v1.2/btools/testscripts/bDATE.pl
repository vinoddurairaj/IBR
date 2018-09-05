#!/usr/bin/perl -I../pms
########################################
######################################
#  perl -w ./bDATE.pm
#
#    eval 'require bDATE' && associated functions
#
#    TEST script 
######################################
########################################

our $GMTSECOND;
our $GMTMINUTE;
our $GMTHOUR;
our $GMTDAY;
our $GMTMONTH;
our $GMTYEAR;

print "\n\nSTART   :  bDATE.pl\n\n";

eval 'require bDATE';

get_GMT_date();

print "\nGMTSECOND  =  $GMTSECOND\n";
print "\nGMTMINUTE  =  $GMTMINUTE\n";
print "\nGMTHOUR  =  $GMTHOUR\n";
print "\nGMTDAY  =  $GMTDAY\n";
print "\nGMTMONTH  =  $GMTMONTH\n";
print "\nGMTYEAR  =  $GMTYEAR\n";

print "\n\nEND   :  bDATE.pl\n\n";

