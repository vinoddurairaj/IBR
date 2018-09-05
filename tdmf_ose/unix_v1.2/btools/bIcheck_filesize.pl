#call bXcheck_deliverables.pm package module
#   must be .. directory

print "\n\nSTART  :  bIcheck_filesize.pl\n\n";
eval 'require bIcheck_size';
checksizetuip();

print "\nvalidatemissing length =  $#validatemissingtuip\n";
print "\nwarnfilesize lenght =  $#warnfilesizetuip\n";

print "\nEND  :  bIcheck_filesize.pl\n";

