#call bRcheck_deliverables.pm package module
#   must be .. directory

print "\nSTART here\n";
$checkmissing='bRcheck_deliverables';
eval "require $checkmissing";
checkdelstatus();
#print "\nmissingdeliverables =\n$missingdeliverables[0]\n";
#print "\nmissingdeliverables =\n@missingdeliverables\n";
#print "\ndate_out  = \n$date_out[0] :  $date_out[1]:  $date_out[2]\n";
print "\ndate_out0  = \n$date_out0[0] :  $date_out0[1]:  $date_out0[2]\n";
#print "\nFILELIST =\n@FILELIST\n";
#print "\nfoundlist =\n@foundlist\n";
print "\nmissingdeliverables =\n@missingdeliverables\n";

print "\nEND here\n";

