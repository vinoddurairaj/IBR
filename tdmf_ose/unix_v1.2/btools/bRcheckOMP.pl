#!/usr/bin/perl -Ipms -Ivalidation
#call bRcheck_OMP.pm package module
#   must be .. directory

our @ompStatus;

print "\nSTART  :  bRcheckOMP.pl\n";

eval 'require bRcheckOMP';
checkOMP();
print "\nompStatus  =\n@ompStatus\n";

print "\nEND   ;  bRcheckOMP.pl\n";

