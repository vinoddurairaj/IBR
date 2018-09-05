#!/usr/bin/perl -I../pms
#################################################
###############################################
#  bIPathFileSizeSpecial.pm
#
#	create and customize vars to allow 
#	processing for valid special image status
###############################################
#################################################

#$SOLver="2.5.2.0";  # replaced with $PRODver  :  no testo

%PathFileSizeSpecial= ("./AIX/4.3.3/dtc.rte" => 20200000,
"./AIX/5.1/dtc.rte" =>  20400000,
"./AIX/5.2/dtc.rte" =>  20400000,
"./AIX/5.3/dtc.rte" =>  20400000,
"./doc/administration_unix.pdf" => 2120000,
"./doc/installation_unix.pdf" => 900000,
"./doc/messages_unix.pdf" => 654000,
"./doc/planning_implementation_unix.pdf" => 2454000,
"./HPUX/11/11.depot" => 12200000,
"./HPUX/11i/11i.depot" => 16600000,
"./HPUX/1123pa/1123pa.depot" => 18200000,
"./HPUX/1123ipf/1123ipf.depot" => 27840000,
"./HPUX/1131ipf/1131ipf.depot" => 27840000,
"./Linux/RedHat/AS3x32/Replicator-$PRODver-$stripbuildnum.i386.rpm" => 2920000,
"./Linux/RedHat/AS4x32/Replicator-$PRODver-$stripbuildnum.i386.rpm" => 2920000,
"./Linux/RedHat/AS3x64/Replicator-$PRODver-$stripbuildnum.x86_64.rpm" => 2780000,
"./Linux/RedHat/AS4x64/Replicator-$PRODver-$stripbuildnum.x86_64.rpm" => 2780000,
"./Redist/RedHat/RPM/blt-2.4u-8.i386.rpm" => 320000,
"./solaris/7/SFTKdtc.$PRODver.pkg" => 12900000,
"./solaris/8/SFTKdtc.$PRODver.pkg" => 12920000,
"./solaris/9/SFTKdtc.$PRODver.pkg" => 12940000,
"./solaris/10/SFTKdtc.$PRODver.pkg"=> 12940000);


