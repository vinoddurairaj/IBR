#!/usr/bin/perl -Ipms -Ivalidation
#call bIcheck_deliverables.pm package module
#   must be .. directory
our @missingdeliverablestuip;

print "\nSTART  check deliverables tuip\n";

eval 'require bIcheckDeliverables';
checkdelstatustuip();

print "\nmissingdeliverablestuip =\n@missingdeliverablestuip\n";

print "\nEND  check deliverables tuip\n";

