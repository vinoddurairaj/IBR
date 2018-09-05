#!/usr/local/bin/perl -w
#######################################
#####################################
#
#  perl -w ./tDATE.pl
#
#	test script for bDATE.pm module
#
#####################################
#######################################
 
#######################################
#####################################
# capture stderr stdout
#####################################
#######################################

#open (STDOUT, ">DATELOG.txt");
#open (STDERR, ">>&STDOUT");

print "\n\nSTART : \tDATE.pl\n\n";

$datepackage='bDATE';

eval "require $datepackage";

#######################################
#####################################
#  get_date()
#####################################
#######################################

@datevar=get_date();

print "\n\nDATE :\t$datevar[1]/$datevar[0]/$datevar[2]\n\n";

#######################################
#####################################
#  get_time()
#####################################
#######################################

@timevar=get_time();

print "\n\nTIME :\t$timevar[1]:$timevar[0]\n\n";

#######################################
#####################################
#  get_GMT_date()
#####################################
#######################################

@gmtdate=get_GMT_date();

print "\n\nGMT  :\t$gmtdate[1]/$gmtdate[0]/$gmtdate[2]\n\n";

#######################################
#######################################

print "\n\nEND tDATE.pl\n\n";

close (STDERR);
close (STDOUT);
