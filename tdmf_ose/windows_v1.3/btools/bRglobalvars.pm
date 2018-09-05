###################################
#################################
#
#  bRglobalvars.pm
#
#	package module  :  global vars   :  require bRglobalvars
#
#################################
###################################

$buildnumber="06999";

$cd="cd";			# for pwd  :  system qq($cd)

$buildmachinename="BMACHINE12";   # prevent build runs on "dev" machines

# XX needs to be CHANGED TO RELEASE LEVEL.

# P Go scripts vars     
$Version="2.5.0";
$Branch="trunk";
#$BuildNo = 0;

$RFWver="RFWHEAD";		# trunk
#$RFWver="RFW220";		# branch

$branchdir="trunk"; 		 # trunk
#$branchdir="branch\\$RFWver"; 	 # installshield build.bat dependency

$branchid="-b";                      # trunk / HEAD  -b = default branch ==>> HEAD!
#$branchid="-rV210GA";             # -rBRANCH  <<== no space, hopefully

$branch_rdiff="-f";			        # for bRrdiff.pl (cvs script :  source changes)
#$branch_rdiff="-r $RFWver_p";                  # branch setting
				        #  -f = force head, trunk, hopefully

$REBUILD_OR_ALL="";   		# Set to NULL # test build 0648
#$REBUILD_OR_ALL="/REBUILD";   # Set to /REBUILD
		      # or /ALL
#.NET Compiler path
$COMPILER=qq("C:\\Program Files\\Microsoft Visual Studio .NET 2003\\Common7\\IDE\\devenv");
$COMPILERPATH=qq("C:\\Program Files\\Microsoft Visual Studio .NET 2003\\Common7\\IDE");

#CVSROOT var
$CVSROOT=":pserver:bmachine\@bmachine0.sanjose.ibm.com:/cvs2/sunnyvale";

$drive="e:\\";	# what drive to build machine

$Bbnum="B$buildnumber";

$dirs2="..\\..";
$dirs3="..\\..\\..";
$dirs4="..\\..\\..\\..";
$dirs5="..\\..\\..\\..\\..";
$dirs6="..\\..\\..\\..\\..\\..";
$dirs7="..\\..\\..\\..\\..\\..\\..";
$dirs8="..\\..\\..\\..\\..\\..\\..\\..";
$dirs9="..\\..\\..\\..\\..\\..\\..\\..\\..";
$dirs10="..\\..\\..\\..\\..\\..\\..\\..\\..\\..";
$dirs11="..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..";
