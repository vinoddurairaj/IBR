#!/usr/bin/perl -Ipms -Ivalidation
#call bRcheck_deliverables.pm package module
#   must be .. directory

our @missingdeliverables;

print "\nSTART here\n";

eval 'require bRcheckDeliverables';
checkdelstatus();
print "\nmissingdeliverables =\n@missingdeliverables\n";

print "\nEND here\n";

