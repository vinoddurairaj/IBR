#call bRcheck_special.pm package module
#   must be .. directory

print "\nSTART  :  bRcheck_special.pl\n";
#$checkmissing='bRcheck_special';
#eval "require $checkmissing";
eval 'require bRcheck_special';
checkspecial();
print "\nmissingspecial =\n@missingspecial\n";

print "\nEND  :  bRcheck_special.pl\n";

