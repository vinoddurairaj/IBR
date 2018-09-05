#!/usr/local/bin/perl -w
###################################
#################################
#
#  perl -w .\bRprebuild_OEM.pl
#
#	Build any OEM required releases (eg.  :  StoneFly)
#
#################################
###################################

###################################
#################################
#  vars && STDERR :  STDOUT
#################################
###################################

require "bRglobalvars.pm";
require "bRcvstag.pm";
$branchdir="$branchdir";
$CVSTAG="$CVSTAG";
$Bbnum="$Bbnum";
$RFWver="$RFWver";

open (STDOUT, ">prebuildOEM.txt");
open (STDERR, ">>&STDOUT");

print "\n\nSTART  :   bRprebuild_OEM.pl\n\n";

###################################
#################################
#
#  read Build.bat  :  create new OEM specific Build(OEM).bat
#
#	put proper dir path per installshield usage
#
#################################
###################################

open (FHBB, "<Build.bat");
@buildbat=<FHBB>;
close (FHBB);

foreach $ebb (@buildbat) {
   #print "\nFOREACH  :  $ebb\n";
   if ( "$ebb" =~ /builds/ &&  "$ebb" =~ /windows_v1.3/ ) {
      #print "\n\nIF  :  $ebb\n\n";
      $ebb =~ s/B\d+\\tdmf_ose/$Bbnum\\tdmf_ose\\OEM\\Stonefly\\tdmf_ose/;
      print "\n\nIF AFTER  :  $ebb\n\n";
      push (@OEMstonefly,"$ebb");
   }
   else  {
     push (@OEMstonefly,"$ebb");
   } 
}

open (FHsfbb,">BUILD_SF.bat");
print FHsfbb @OEMstonefly;
close (FHsfbb);

system ($cd);

###################################
#################################
#
#  grab files from installshield directory  :
#
#	ftdsrc\\Installshield\\TDMF_OSE\\Setup Files\\Compressed Files\\Language Independent\\OS Independent	
#
#################################
###################################

$ISfiles=qq("Installshield\\TDMF_OSE\\Setup Files\\Compressed Files\\Language Independent\\OS Independent");

chdir "..\\ftdsrc";

system qq (pkzip25 -add -directories ..\\..\\$RFWver.IS7x_SF.zip $ISfiles\\*); 

###################################
#################################
#
#  create new OEM work area
#
#	checkout only the installshield scripts
#	use *RFWexe.$Bbnum.zip for the deliverables
#		Deliverables from zip are same for Softek or OEM releases
#
#################################
###################################

chdir "..\\..";

system ($cd);

mkdir "OEM";
mkdir "OEM\\StoneFly";

system ($cd);

chdir "OEM\\StoneFly";

system ($cd);

###################################
#################################
#  cvs checkout -r $CVSTAG  ( only installshield project required )
#################################
###################################

system qq( cvs checkout -r $CVSTAG tdmf_ose/windows_v1.3/ftdsrc/Installshield );

print "\n\nOEM  :  StoneFly  :  cvs checkout -r $CVSTAG  :  rc = $?\n\n";

chdir "tdmf_ose\\windows_v1.3\\ftdsrc";

system ($cd);

###################################
#################################
#  pkzip25 -extract -directories $RFWver.$Bbnum.RFWexe.zip
#
#	deliverables via VC 7 build process
#################################
###################################

system qq(pkzip25 -extract -directories ..\\..\\..\\..\\..\\$RFWver.IS7x_SF.zip);

mkdir "TDMF";

chdir "TDMF";

system ($cd);

system qq (pkzip25 -extract -directories ..\\..\\..\\..\\..\\..\\$RFWver.$Bbnum.RFWexe.zip );

print "\n\nOEM  :  StoneFly  :  pkzip25 -extract DELIVERABLES  :  rc = $?\n\n";

chdir "..\\";

system ($cd);

###################################
#################################
#  setup specific *.ipr, copy appropriate dll's :  help files, etc
#################################
###################################

print "\n\nSFResDll.dll: \t";
system "copy /Y TDMF\\Release\\dtcSFResDll.dll TDMF\\GUI\\RBRes.dll";

print "\n\nStoneFly.ico: \t";
system "copy /Y \\TDMF_OSE\\UninstallIcon\\StoneFly.ico TDMF\\UninstallIcon\\dtc.ico";

print "\n\ndtc.ico: \t";
system "copy /Y TDMF\\UninstallIcon\\dtc.ico \"Installshield\\TDMF_OSE\\Setup Files\\Uncompressed Files\\Language Independent\\OS Independent\\dtc.ico\"";

print "\n\nReicon DtcCommonGui.exe: \t";
system "reicon TDMF\\UninstallIcon\\dtc.ico TDMF\\GUI\\DtcCommonGui.exe #128";

print "\n\nReicon DtcAnalyzer.exe: \t";
system "reicon TDMF\\UninstallIcon\\dtc.ico TDMF\\ReplicationServer\\win2000\\DtcAnalyzer.exe #101";

print "\n\nModifyIPR.exe: \t";
system "ModifyIPR.exe Installshield\\TDMF_OSE\\TDMF_OSE.ipr Installshield\\TDMF_OSE\\StoneFlyChangesToIpr.txt";

print "\n\nrebrandable.h: \t";
system "copy /Y \"Installshield\\TDMF_OSE\\Script Files\\StoneFlyrebrandable.h\" \"Installshield\\TDMF_OSE\\Script Files\\rebrandable.h\"";

###################################
###################################

print "\n\n\tEND  :  bRprebuild_OEM.pl\n\n";

###################################
###################################

close (STDERR);
close (STDOUT);
