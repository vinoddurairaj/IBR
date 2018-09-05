#!/usr/bin/perl -Ipms -Ivalidation

#  perl -w bIcallPathFileSize.pl
#
#    test call to %PathFileSize via bIPathFileSize.pm

require "bIPathFileSize.pm";
#eval 'require bIPathFileSize';

if (%PathFileSize) { print "\nIF\n"; }


