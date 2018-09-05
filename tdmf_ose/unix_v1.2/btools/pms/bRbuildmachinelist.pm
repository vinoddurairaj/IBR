################################
##############################
#
#  bRbuildmachinelist.pm
#
#	match each build machine hostname to a variable
#
##############################
################################

#solaris
$solaris7="sjbsolv27m1";                    #9.43.90.28
$solaris8="sjbsolv28m1";                    #9.43.90.29
$solaris9="sjbsolv29m1";                    #9.43.90.32
$solaris10="sjbsolv10m1";                   #9.43.90.35

#hp
$hp11="sjbhp11v0m2";                        #9.43.90.21
$hp11i="sjbhp11v1m2";                       #9.43.90.23
$hp1123IPF="sjbhp11v2m1";                   #9.43.90.26
$hp1123PA="sjbhp11v2m2";                    #9.43.90.24
$hp1131IPF="sjbhp11v3m1";                   #9.43.90.22
$hp1131PA="sjbhp11v3m2";                    #9.43.90.25

#aix
$aix433="sjbimax43m1";                      #9.43.90.20
$aix51="sjbimax51m1";                       #9.43.90.19
$aix52="sjbimax52m1";                       #9.43.90.17
$aix53="sjbimax53m1";                       #9.43.90.18
$aix61="sjbimax61m1";                       #9.43.90.16

# RedHat
$linuxRHAS="sjbrhlas3m2";                    #9.43.90.31
$RHAS64="sjbrhlas3m3";                       #9.43.90.38

#RedHat 4x
$RHAS4xia64="sjbrhlas4m1";                   #9.43.90.33
$RHAS4x64="sjbrhlas4m2";                     #9.43.90.37
$RHAS4x32="sjbrhlas4m3";                     #9.43.90.39

#RedHat 5x SP#  ia64
$RHAS5xia64="sji641ht.sanjose.ibm.com";     #9.43.90.46
#RedHat 5x SP#  x64
$RHAS5xx64="sjx64chr.sanjose.ibm.com";      #9.43.90.48
#RedHat 5x SP#  x86
$RHAS5xx86="sjx64chp.sanjose.ibm.com";      #9.43.90.36

# zLinux RedHat s390 (x)
$zRH4xs390x64="tdmrb01.rtp.raleigh.ibm.com";  #9.42.142.119
$zRH5xs390x64="tdmrb02.rtp.raleigh.ibm.com";  #9.42.142.119

# SuSE
#SuSE9x SP4 ia64
$suse9xia64="sji641hv.sanjose.ibm.com";     #9.43.90.44 
#SuSE9x SP4 x86
$suse9xx86="sjx64lxa.sanjose.ibm.com";      #9.43.90.49 
#SuSE9x SP4 x64
$suse9xx64="sjx64lxc.sanjose.ibm.com";      #9.43.90.52 
#SuSE10x SP# ia64
$suse10xia64="sji641hs.sanjose.ibm.com";    #9.43.90.45 
#SuSE10x SP# x86
$suse10xx86="sjx64lxb.sanjose.ibm.com";     #9.43.90.51 
#SuSE10x SP# x64
$suse10xx64="sjx64lxd.sanjose.ibm.com";     #9.43.90.53 
#SuSE11x SP# ia64
$suse11xia64="sji64UNKNOWN.sanjose.ibm.com";    #9.43.90.45 
#SuSE11x SP# x86
$suse11xx86="sjx86suse11.sanjose.ibm.com";      #9.43.90.81 
#SuSE11x SP# x64
$suse11xx64="sjx64suse11.sanjose.ibm.com";      #9.43.90.82 

# zLinux SuSE s390 (x)
$zsuse9xs390x64="tdmsb01.rtp.raleigh.ibm.com";
$zsuse10xs390x64="tdmsb02.rtp.raleigh.ibm.com";



#other build infrastructure machines
$dmsbuilds="dmsbuilds.sanjose.ibm.com";
#bmachine0     9.43.90.40  :  cvs
#bmlinux0      9.43.90.30
#sjbrhles4m2   9.43.90.34   :  /srcpool (anaconda)
#sjbmscripts   9.43.90.36

