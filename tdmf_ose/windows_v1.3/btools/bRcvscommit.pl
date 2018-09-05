!#/usr/local/bin/perl -w
#####################################
#
#  bRcvscommit.pl
#
#	commit changes to files changed during build
#
#####################################

require "bRglobalvars.pm";
$RFWver="$RFWver";

open (STDOUT, ">CVSCOMMITLOG.txt");
open (STDERR, ">>&STDOUT");

print "\n\n start bRcvscommit.pl\n\n";

###############################
#############################
# xcopy any commit required files from builds\\$Bbnum
#   created via a $TAG which prevents commits,
#   to our local directory for commits, if required.
#############################
###############################

system qq(xcopy /K /V /Y ..\\builds\\$Bbnum\\tdmf_ose\\windows_v1.3\\btools\\emaildist.txt);
system qq(xcopy /K /V /Y ..\\builds\\$Bbnum\\tdmf_ose\\windows_v1.3\\btools\\bRsuc_fail.pm);
system qq(xcopy /K /V /Y ..\\builds\\$Bbnum\\tdmf_ose\\windows_v1.3\\btools\\btoolschangelist.txt);
system qq(xcopy /K /V /Y ..\\builds\\$Bbnum\\tdmf_ose\\windows_v1.3\\btools\\sourcechangelist.txt);
system qq(xcopy /K /V /Y ..\\builds\\$Bbnum\\tdmf_ose\\windows_v1.3\\btools\\bRlastbuildGMTdate.pm);

system qq($cd);
system qq($cd);

# place required cvs commit files into hash
# list, and process each file.  Add relative path
# as required.  Numbers are the keys, files are values....
# some of these entries are committed previously,
# leaving here as an insurance policy....

%commitfiles = (
"1", "bRglobalvars.pm",
"2", "sourcechangelist.txt",    		# watch the no comma,
"3", "2K_env.txt",	        		# comma required issue here
"4", "emaildist21x.txt",		# comma required issue here
"5", "btoolschangelist.txt",		# comma required issue here
"6", "bRlastbuildGMTdate.pm",	# comma required issue here
"7", "bRsuc_fail.pm");		# last entry, no comma....
#"7", "addfilehere");

$buildnumber="$buildnumber";  #stop the annoying warning message....

#################################
###############################
#
# process hash values from %commitfiles list
#
#	must run thru cmd.exe 
#
###############################
#################################

foreach $value (values %commitfiles) {
   system qq(cvs commit -m "$RFWver $Bbnum commit via bRcvscommit.pl : buildcommitstring" $value);
   print "\n hash foreach : $value \n";
   system qq(ls -l $value);
}

print "\n\n\t end bRcvscommit.pl\n\n";

close (STDERR);
close (STDOUT);
