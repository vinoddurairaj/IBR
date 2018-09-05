#!$ENV{PERL58} -I. -I../pms
#################################################
###############################################
#
#  bRlogValues.pm
#
#    sub logvalues  :  log values of global vars
#
###############################################
#################################################

#################################################
###############################################
#  vars  /  pms  /  use
###############################################
#################################################

eval 'require bRglobalvars';
eval 'require bRsshVARS';
eval 'require bRstrippedbuildnumber';

#################################################
###############################################
#  sub logvalues
###############################################
#################################################

sub logvalues {

$scriptName="$0";
$scriptName=~s/^.*\\bR/bR/g;

$runTime=`date`;

print "\n\nSTART  :  $scriptName\n\n";

print "\n\n=============================================\n";
print "\n+++++++++++++++++++++++++++++++++++++++++++++\n";
print "\nLOG VALUES  :  $Bbnum\n";
print "\n+++++++++++++++++++++++++++++++++++++++++++++\n";
print "\n=============================================\n\n";

print "\nRFXver           =  $RFXver\n";
print "\nTUIPver          =  $TUIPver\n";
print "\nCOMver           =  $COMver\n";
print "\nbuildnumber      =  $buildnumber\n";
print "\nstripbuildnum    =  $stripbuildnum\n";
print "\nBbnum            =  $Bbnum\n";

print "\nbranchdir        =  $branchdir\n";
print "\nbranchid         =  $branchid\n";
print "\nbranch_rdiff     =  $branch_rdiff\n";
print "\nsource           =  $source\n";
print "\npro              =  $pro\n";
print "\nscriptName       =  $scriptName\n";
print "\nCVSROOT          =  $CVSROOT\n\n";

print "\nrfxBLDCMD            =  $rfxBLDCMD\n";
print "\ntuipBLDCMD           =  $tuipBLDCMD\n\n";
print "\nbtoolsPath           =  $btoolsPath\n\n";
print "\nbuildTreePath        =  $buildTreePath\n";
print "\nbuildTreePathTuip    =  $buildTreePath\n\n";
print "\nbuildRootPath        =  $buildRootPath\n";
print "\nbuildRootPathTuip    =  $buildRootPathTuip\n\nn";

#print "\ndollar 0         =  $0\n";

print "\n\nscript exec local runtime  =  $runTime\n\n";

system qq(pwd);
print "\n\n";

}

sub logcurrentGMT {

eval 'require bSbuildGMT';

print "\ncurrentbuildGMTYEAR     =  $currentbuildGMTYEAR\n";
print "\ncurrentbuildGMTMONTH    =  $currentbuildGMTMONTH\n";
print "\ncurrentbuildGMTDAY      =  $currentbuildGMTDAY\n";
print "\ncurrentbuildGMT         =  $currentbuildGMT\n\n";

}

sub logpreviousGMT {

eval 'require bSpreviousbuildGMT';

print "\npreviousbuildGMTYEAR    =  $previousbuildGMTYEAR\n";
print "\npreviousbuildGMTMONTH   =  $previousbuildGMTMONTH\n";
print "\npreviousbuildGMTDAY     =  $previousbuildGMTDAY\n";
print "\npreviousbuildGMT        =  $previousbuildGMT\n";

}
 
