#!perl -Ipms
#above  :  chdir .. performed before include of libs (cvs update issue)
###############################################
#############################################
#
#  perl -w -I../pms ./bIgetCodeNoRoot.pl
#
#    cvs update from ../ to get bRglobalvars.pm & bRcvstag.pm
#
#    current build tree path  :
#            ~/dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum
#
#    cvs checkout -r $CVSTAG $projects
#
#############################################
###############################################

###############################################
#############################################
#  cvs update  :  latest pms/* etc 
#############################################
###############################################

chdir "..";

system qq(pwd);

system qq(cvs update);

###############################################
#############################################
#  vars  /  use  /  pms
#############################################
###############################################

eval 'require bRglobalvars';
eval 'require bRcvsprojects';
eval 'require bRcvstag';

our $COMver;
our $RFXver;
our $TUIPver;
our $Bbnum;
our $projects;
our $CVSTAG;
#our $CVSROOT;
#our $CVR="-d $CVSROOT";

###############################################
#############################################
#  exec statements 
#############################################
###############################################

print "\n\nSTART bIgetCodeNoRoot.pl\n\n"; 

###############################################
#############################################
#  log values
#############################################
###############################################

print "\n\nCOMver    =  $COMver\n";
print "\nRFXver      =  $RFXver\n";
print "\nTUIPver     =  $TUIPver\n";
print "\nBbnum       =  $Bbnum\n\n";
print "\nCVSTAG      =  $CVSTAG\n\n";
#print "\nCVSROOT     =  $CVSROOT\n\n";
#print "\nCVR         =  $CVR\n\n";
print "\n\nprojects  =  $projects\n\n";

###############################################
#############################################
#  create build tree structure
#############################################
###############################################

chdir "..";

system qq(pwd);

if ( ! -d "builds" ) {
    print "\nIF :  mkdir builds\n";
    system qq(mkdir "builds");
}

chdir "builds";

if ( ! -d "tuip" ) {
    print "\nIF :  mkdir tuip\n";
    system qq(mkdir "tuip");
}

chdir "tuip";

system qq(pwd);

system qq(mkdir "$Bbnum");

chdir "$Bbnum";

system qq(pwd);

###############################################
#############################################
#  get code  :  level assigned via $CVSTAG 
#############################################
###############################################

#system qq(cvs $CVR export -r $CVSTAG $projects );
system qq(cvs export -r $CVSTAG $projects );

print "\n\ncvs checkout -r $CVSTAG rc = $? \n\n";

###############################################
###############################################

print "\n\nEND  :  bIgetCodeNoRoot.pl\n\n"; 

###############################################
###############################################

close (STDERR);
close (STDOUT);

