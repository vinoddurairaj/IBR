#call bIcheck_special.pm package module
#   must be .. directory

print "\nSTART  :  bIcheck_special.pl\n";
#$checkmissing='bIcheck_special';
#eval "require $checkmissing";
eval 'require bIcheck_special';
checkspecialtuip();
print "\nmissingspecial =\n@missingspecialtuip\n";

print "\nEND  :  bIcheck_special.pl\n";

