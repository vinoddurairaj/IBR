#!/usr/bin/perl -I..
###################################
#################################
#  bRglobalvars.pm
#    build global vars
#################################
###################################

$buildnumber="13006";

# XX needs to be CHANGED TO RELEASE LEVEL.

$RFX="RFX";		# trunk
$TUIP="TUIP";		# trunk
$Version="HEAD";	# trunk
$RFXver="$RFX$Version";		# trunk
$TUIPver="$TUIP$Version";	# trunk

#$COMver="$RFXver$TUIPver$Version";
$COMver="$RFX$TUIP$Version";

$branchid="-b";                      # trunk -b = default branch
#$branchid="-rRFX210_p";             # -rBRANCH  <<== no space

# os.bldcmd $PATCHver $PRODtype
# example :  sol.bldcmd $PATCHver $PRODtype 
$PATCHver="0";  # upto 3 digits  :  use specific number  :  no padded zero's 
$PRODtype="rep";       #  rep
$PRODtypetuip="tdmf";  #  tdmf
#$PRODver deprecated via vars.pm (generate via ftdsrc/mk/Makefile)
#see bRmakeVARS.pl 
#$PRODver="2.2.0"; #product version  ==>> in deliverables filenames....

#  note  :  $stipbuildnum is generated via RSH and bRstripbuildnum.pm
$stripbuildnum="99999";  #set value  :  overwrite via bRstrippedbuildnum.pm

$branchdir="trunk"; 		 # for unix trunk
#$branchdir="branch/$RFXver";    # for unix branch

$branch_rdiff="-f";		        # bRrdiff.pl dependency
#$branch_rdiff="-r $RFXver_p";          # branch setting
				        #  -f = force head, trunk, hopefully

$CVSROOT=":pserver:bmachine\@bmachine0:/cvs2/sunnyvale";

$Bbnum="B$buildnumber";

$SUBPROJECT="unix_v1.2";		#due to the version numbers in the source tree

