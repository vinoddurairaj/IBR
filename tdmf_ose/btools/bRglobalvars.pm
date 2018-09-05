###################################
#################################
#
#  bRglobalvars.pm
#
#	package module  :  global vars   :  require bRglobalvars
#
#################################
###################################

$buildnumber="000001";

$cd="cd";			# for pwd  :  system qq($cd)

#$buildmachinename="BMACHINE9";
$buildmachinename="BMACHINE1";   #testing

# XX needs to be CHANGED TO RELEASE LEVEL.

$RFUver="RFU129";		# trunk
#$RFUver="FRU210";		# trunk
#$RFUver="RFU210ESP";	# branch. archive names, etc.

$cvsworkspace="tdmf_ose";

$branchdir="trunk"; 		 # for unix trunk
#$branchdir="branch/T0P210GA"; # for unix branch

$branch_rdiff="-f";			        # for bSMrdiff.pl (cvs script :  source changes)
#$branch_rdiff="-r RFU210_ESP0_p";           # branch setting
				        #  -f = force head, trunk, hopefully

$branchNAME="HEAD";		#HEAD blivoviously
#$branchNAME="RFU210GA_p";	# branch-o-rama

$REBUILD_OR_ALL="";   		# Set to NULL # test build 0648
#$REBUILD_OR_ALL="/REBUILD";   # Set to /REBUILD
		      # or /ALL

$CVSROOT=":pserver:bmachine\@129.212.239.125:/cvs2/sunnyvale";

$intertarg="intertarg";
$Bbnum="B$buildnumber";

$cvsexport="-D -f"; 		#trunk setting
#$cvsexport="-r RFU210ESP2_p";	# ESP2 branch
#$cvsexport="-r RFU210XXXGA_p";	# GA branch

$SUBPROJECT="unix_v1.2";		#due to the version numbers in the source tree

$dirs2="..\\..\\";
$dirs3="..\\..\\..\\";
$dirs4="..\\..\\..\\..\\";
$dirs5="..\\..\\..\\..\\..\\";
$dirs6="..\\..\\..\\..\\..\\..\\";
$dirs7="..\\..\\..\\..\\..\\..\\..\\";
$dirs8="..\\..\\..\\..\\..\\..\\..\\..\\";
$dirs9="..\\..\\..\\..\\..\\..\\..\\..\\..\\";
$dirs10="..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\";
