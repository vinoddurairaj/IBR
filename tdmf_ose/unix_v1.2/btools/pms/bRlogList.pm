#!/usr/bin/perl -I..
################################################
################################################
#  bRlogList.pm
#
#    list of build logs per Plat / OS
################################################
################################################


our @LOGS=qw(unix/aix/AIX51.txt
   unix/aix/AIX52.txt
   unix/aix/AIX53.txt
   unix/aix/AIX61.txt
   unix/hp/HP11i.txt
   unix/hp/HP1123PA.txt
   unix/hp/HP1123IPF.txt
   unix/hp/HP1131IPF.txt
   unix/hp/HP1131PA.txt
   unix/redhat/RedHatAS4x32.txt
   unix/redhat/RedHatAS4x64.txt
   unix/redhat/RedHatAS4xia64.txt
   unix/redhat/RedHatAS5xx64.txt
   unix/redhat/RedHatAS5xx86.txt
   unix/redhat/RedHatAS5xia64.txt
   unix/suse/SUSE9xia64.txt
   unix/suse/SUSE9xx64.txt
   unix/suse/SUSE9xx86.txt
   unix/suse/SUSE10xx64.txt
   unix/suse/SUSE10xx86.txt
   unix/suse/SUSE10xia64.txt
   unix/suse/SUSE11xx64.txt
   unix/suse/SUSE11xx86.txt
   unix/suse/SUSE11xia64.txt
   unix/sol/SOL7.txt
   unix/sol/SOL8.txt
   unix/sol/SOL9.txt
   unix/sol/SOL10.txt
);

#DEACTIVATED
#   unix/zLinux/RedHat4x/RH4xs390x64.txt
#   unix/zLinux/RedHat5x/RH5xs390x64.txt
#   unix/zLinux/SuSE9x/SuSE9xs390x64.txt
#   unix/zLinux/SuSE10x/SuSE10xs390x64.txt

@genLOGS=qw(logs/rdiff.txt
   logs/RFXBUILDLOG.txt
   logs/buildnuminc.txt
   logs/setGMT.txt
   logs/makeVARS.txt
   logs/cvstag.txt
   logs/ftppkg.txt
   logs/ftppkgtuip.txt
   logs/cp3rdparty.txt
   logs/cpdocs.txt
   logs/tarimage.txt
   logs/createiso.txt
   logs/parlog.txt
   notification.txt
);


