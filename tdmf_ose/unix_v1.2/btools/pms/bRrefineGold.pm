#!/usr/bin/perl -I.. -I. -I../validation
##########################################
########################################
#  bRrefineGold.pm
#
#    paths for refineGold.iso image
#    
#    lists  :  @RFXrefineGold
#              @TUIPrefineGold 
#              @common
########################################
##########################################

eval 'require bRglobalvars';
eval 'require bRstrippedbuildnumber';
eval 'require bRdevvars';

@RFXrefineGold=("./AIX/5.1/dtc.rte",
   "./HPUX/11i/11i.depot",
   "./Linux/SuSE/10x/x86_64/Replicator-$PRODver-$stripbuildnum.x86_64.rpm",
   "./Linux/SuSE/9x/x86_32/Replicator-$PRODver-$stripbuildnum.i386.rpm",
   "./Linux/RedHat/4x/ia64/Replicator-$PRODver-$stripbuildnum.ia64.rpm",
   "./solaris/10/SFTKdtc.$PRODver.pkg",
);

#DEACTIVATED
#   "./Linux/SuSE/9x/s390x/Replicator-$PRODver-$stripbuildnum.s390x.rpm",
#   "./Linux/SuSE/10x/s390x/Replicator-$PRODver-$stripbuildnum.s390x.rpm",


@TUIPrefineGold=("./AIX/5.1/dtc.rte",
   "./HPUX/11i/11i.depot",
   "./Linux/SuSE/10x/x86_64/TDMFIP-$PRODver-$stripbuildnum.x86_64.rpm",
   "./Linux/SuSE/9x/x86_32/TDMFIP-$PRODver-$stripbuildnum.i386.rpm",
   "./Linux/RedHat/4x/ia64/TDMFIP-$PRODver-$stripbuildnum.ia64.rpm",
   "./solaris/10/SFTKdtc.$PRODver.pkg",
);

#DEACTIVATED
#   "./Linux/SuSE/9x/s390x/TDMFIP-$PRODver-$stripbuildnum.s390x.rpm",
#   "./Linux/SuSE/10x/s390x/TDMFIP-$PRODver-$stripbuildnum.s390x.rpm",

@common=("doc/*",
   "Redist/*",
);

