#!/usr/bin/perl -Ipms -Ivalidation
#call bRcheck_deliverables.pm package module
#   must be .. directory

print "\n\nSTART  :  bRcheck_filesize.pl\n\n";
eval 'require bRcheckSize';
checksize();

print "\nvalidatemissing length =  $#validatemissing\n";
print "\nwarnfilesize length =  $#warnfilesize\n";

print "\nEND  :  bRcheck_filesize.pl\n";

