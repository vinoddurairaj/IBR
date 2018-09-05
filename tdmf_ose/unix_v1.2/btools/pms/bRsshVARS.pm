#!/usr/bin/perl -I../pms
##########################################
##########################################
#  bRsshVARS.pm
#
#    variables for btools/unix/OS/*ssh* scripts
##########################################
##########################################

eval 'require bRglobalvars';
eval 'require bRstrippedbuildnumber';

our $rfxBLDCMD="./bldcmd  brand=$PRODtype     build=$stripbuildnum fix=$PATCHver";
our $tuipBLDCMD="./bldcmd brand=$PRODtypetuip build=$stripbuildnum fix=$PATCHver";

our $btoolsPath="dev/$branchdir/tdmf_ose/unix_v1.2/btools";
our $RH4xPath="dev/RH4x/$branchdir/tdmf_ose/unix_v1.2/btools";
our $RH5xPath="dev/RH5x/$branchdir/tdmf_ose/unix_v1.2/btools";
our $SuSE9xPath="dev/SuSE9x/$branchdir/tdmf_ose/unix_v1.2/btools";
our $SuSE10xPath="dev/SuSE10x/$branchdir/tdmf_ose/unix_v1.2/btools";

our $buildTreePath="dev/$branchdir/tdmf_ose/unix_v1.2/builds/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc";
our $buildTreePathTuip="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip/$Bbnum/tdmf_ose/unix_v1.2/ftdsrc";

our $buildRootPath="dev/$branchdir/tdmf_ose/unix_v1.2/builds/";
our $buildRootPathTuip="dev/$branchdir/tdmf_ose/unix_v1.2/builds/tuip";

our $pro=". ./.profile";
our $source="source .bashrc";

