#!/usr/bin/perl -I../pms 
##################################################
################################################
#  bRpatchDef.pm
#
#    lists variables definitions patch artifacts requirements
#
#  go to $patchRevision to define patch tar archive name
#
#  NonRoot  :  files contained in the nonroot.tar archives.
#              files will likely be installed at path
#              defaultInstDir/SFTKdtc/bin
#              AIX is exception, "usr" fileset
#
#  Root  :  files contained in root.tar archives.  files
#           to install are installed from filesytem root (/)
#           directory 
#
#  To configure core artifacts  ==>>  go to list @coreFiles
#
#    these might be install scripts, readme files....
#
#    @corefiles=("path/to/file/required", "etc/etc");
#
#
#  To configure package artifacts  ==>>  
#             go to list @currentPackagePatches
#
#    these must be exclusively Tech Pubs....
#
#    @currentPackagePatches=("path/to/file/required","etc/etc");
#
#  To configure built artifacts  ==>>  go to list @patchMain
#
#     (we will use aix53 for example)
#     to include nonroot files  :  nonOn
#     to exclude nonroot files  :  nonOff
#
#     to include root files  :  rootOn
#     to exclude root files  :  rootOff
#
#     to determine nonroot / root On / Off   :  
#     look for required artifacts in  :
#     nonroot ==>>  nakedBins/PRODver.Bbnum.aix53.nonroot.tar
#     root    ==>>  nakedbins/PRODver.Bnum.aix53.root.tar
#
#     NOTE  :  root is likely drivers plus binary files
#              or scripts that are installed in the root
#              file system (and possibly a second install
#              of a file included in the binary install
#              path ) (AIX is exception as the driver files
#              appear to be installed / located with the
#              nonroot files...)
#
#     go to list  :  OSversionNonrootFile  : 
#        @aix53NonrootFile
#        add files w/ path to this list
#
#     go to list  :  OSversionRootFile  : 
#        @aix53RootFile
#        add files w/ path to this list
#
################################################
##################################################

##################################################
################################################
#  @coreFiles   
#
#    list should include (eg) readme, install scripts
#    (not built binaries);
#
################################################
##################################################

#examples no freak out please
#path implies these examples are in local
#directory tdmf_ose/unix_v1.2/ftdsrc/patches
@coreFiles=("ReadMe",
);

#examples  save for syntax  
#path implies these examples are in local
#directory tdmf_ose/unix_v1.2/ftdsrc/patches
#@coreFiles=("ReadMe",
#   "RFXreadMe",
#   "TDMFIPreadMe",
#   "installPatch",
#   "uninstallPatch",
#);

##################################################
################################################
#  @currentPackagePatchesRFX
#  @currentPackagePatchesTDMFIP
#
#   default NO docs  ==>> comment out the lists
#        as the scripts check for $#list > -1
#
#    list exclusive to TechPubs
#    (not built binaries);
#
#    there should be no other consideration on the
#        package side than them docs
#
#  NOTE  :  first implementation we noticed that
#           RFX / TDMFIP docs names / docs themselves
#           are inconsistent, hence 2 lists required 
#
################################################
##################################################

#list the files only.  the auto script will
#  collect these from TCworkarea/PRODUCTdocs dirs

#@currentPackagePatchesRFX=("RFXReleaseNotes.pdf",
#);

#@currentPackagePatchesTDMFIP=("ReleaseNotes.pdf",
#);


##################################################
################################################
#  name patch tar 
################################################
##################################################

$patchRevision="$RFXver.Patch01.tar";


##################################################
################################################
#  $OSverBinTarName
#  $OSverDriverTarName
#
#    shorten our long string in patchMain
################################################
##################################################

our $aix53B="$RFXver.$Bbnum.aix53.nonroot.tar";
our $aix53D="$RFXver.$Bbnum.aix53.root.tar";
our $h23ia64B="$RFXver.$Bbnum.hpux1123ia64.nonroot.tar";
our $h23ia64D="$RFXver.$Bbnum.hpux1123ia64.root.tar";
our $h23paB="$RFXver.$Bbnum.hpux1123parisc.nonroot.tar";
our $h23paD="$RFXver.$Bbnum.hpux1123parisc.root.tar";
our $h31ia64B="$RFXver.$Bbnum.hpux1131ia64.nonroot.tar";
our $h31ia64D="$RFXver.$Bbnum.hpux1131ia64.root.tar";
our $h31paB="$RFXver.$Bbnum.hpux1131parisc.nonroot.tar";
our $h31paD="$RFXver.$Bbnum.hpux1131parisc.root.tar";
our $sol10B="$RFXver.$Bbnum.sol10.nonroot.tar";
our $sol10D="$RFXver.$Bbnum.sol10.root.tar";
our $sol9B="$RFXver.$Bbnum.sol9.nonroot.tar";
our $sol9D="$RFXver.$Bbnum.sol9.root.tar";
our $r5xi686B="$RFXver.$Bbnum.RedHat5xx86.nonroot.tar";
our $r5xi686D="$RFXver.$Bbnum.RedHat5xx86.root.tar";
our $r5xx64B="$RFXver.$Bbnum.RedHat5xx64.nonroot.tar";
our $r5xx64D="$RFXver.$Bbnum.RedHat5xx64.root.tar";
our $r6xi686B="$RFXver.$Bbnum.RedHat6xx86.nonroot.tar";
our $r6xi686D="$RFXver.$Bbnum.RedHat6xx86.root.tar";
our $r6xx64B="$RFXver.$Bbnum.RedHat6xx64.nonroot.tar";
our $r6xx64D="$RFXver.$Bbnum.RedHat6xx64.root.tar";
our $s10xi686B="$RFXver.$Bbnum.SuSE10xi386.nonroot.tar";
our $s10xi686D="$RFXver.$Bbnum.SuSE10xi386.root.tar";
our $s10xx64B="$RFXver.$Bbnum.SuSE10xx64.nonroot.tar";
our $s10xx64D="$RFXver.$Bbnum.SuSE10xx64.root.tar";
our $s11xi686B="$RFXver.$Bbnum.SuSE11xi386.nonroot.tar";
our $s11xi686D="$RFXver.$Bbnum.SuSE11xi386.root.tar";
our $s11xx64B="$RFXver.$Bbnum.SuSE11xx64.nonroot.tar";
our $s11xx64D="$RFXver.$Bbnum.SuSE11xx64.root.tar";

##################################################
################################################
#  @patchMain
#
#    double array containing data and too include or not
#    too include in patch
#
#    index[0]  =  OSverNonRootFile (array name w/o @)
#    index[1]  =  OSverRootFile (array name w/o @)
#    index[2]  =  [ nonOn / nonOff ] (toggle)
#    index[3]  =  [ rootOn / rootOff ] (toggle)
#    index[4]  =  OSver.nonroot.tar (name) 
#    index[5]  =  OSver.root.tar (name) 
#    index[6]  =  NA 
#
#
################################################
##################################################
 
@patchMain = (
["aix53NonrootFile","aix53RootFile","nonOn","rootOff","$aix53B","$aix53D"],

["hp23ia64NonrootFile","hp23ia64RootFile","nonOn","rootOff","$h23ia64B","$h23ia64D"],

["hp23paNonrootFile","hp23paRootFile","nonOn","rootOff","$h23paB","$h23paD"],

["hp31ia64NonrootFile","hp31ia64RootFile","nonOn","rootOff","$h31ia64B","$h31ia64D"],

["hp31paNonrootFile","hp31paRootFile","nonOn","rootOff","$h31paB","$h31paD"],

["sol10NonrootFile","sol10RootFile","nonOff","rootOff","$sol10B","$sol10D"],

["sol9NonrootFile","sol9RootFile","nonOn","rootOff","$sol9B","$sol9D"],

["red5xi686NonrootFile","red5xi686RootFile","nonOn","rootOff","$r5xi686B","$r5xi686D"],

["red5xx64NonrootFile","red5xx64RootFile","nonOn","rootOff","$r5xx64B","$r5xx64D"],

["red6xi686NonrootFile","red6xi686RootFile","nonOff","rootOff","$r6xi686B","$r6xi686D"],

["red6xx64NonrootFile","red6xx64RootFile","nonOff","rootOff","$r6xx64B","$r6xx64D"],

["suse10xi686NonrootFile","suse10xi686RootFile","nonOff","rootOff","$s10xi686B","$s10xi686D"],

["suse10xx64NonrootFile","suse10xx64RootFile","nonOff","rootOff","$s10xx64B","$s10xx64D"],

["suse11xi686NonrootFile","suse11xi686RootFile","nonOff","rootOff","$s11xi686B","$s11xi686D"],

["suse11xx64NonrootFile","suse11xx64RootFile","nonOff","rootOff","$s11xx64B","$s11xx64D"],

);

##################################################
################################################
#  @OSverFileList
#
#    above is generic for testing.
#   
#    we should have lists similar to these examples  :
#
#    @AIX53files
#    @SOL10files
#    @HPUX1123ia64files
#    @HPUX1123pafileList
#
#  Itterate  :  nonroot.tar  ==>> binary
#                  root.tar  ==>> drivers plus binaries
#                                 installed in root path
#
#
#  example  :
#
#    @aix53NonrootFile=(
#       "path/to/binFile/in/tar/aix53/name.a",
#       "path/to/binFile/in/tar/aix53/name.o"
#    );
#
#  looks like most bin files are in root of the tar....
#  need to verify that one.  yes sir.  lets check the
#  driver tars...nope, those driver tars have directories...
#
#  Note  :  current values are set for test of mechanism
################################################
##################################################

#AIX use nonroot / root tar entries from the paths
#   in those archives (paths should be proper in archives)
#   some hacmp scripts are in the root tar ...
@aix53NonrootFile=(
   "usr/dtc/bin/dtcpsmigrate272",
   "usr/dtc/bin/dtcstart",
   "usr/dtc/bin/dtclimitsize",
   "usr/dtc/bin/dtcexpand",
);

#NOTE  :  aix seems to have the driver included in the 
          #nonroot archive  
          #some hacmp scripts are in the root tar ...
@aix53RootFile=(
   "etc/dtc/hacmp/cl_activate_dtc",
   "etc/dtc/hacmp/cl_deactivate_dtc",
);

@hp23ia64NonrootFile=(
   "SFTKdtc/bin/dtcpsmigrate272",
   "SFTKdtc/bin/dtcstart",
   "SFTKdtc/bin/dtclimitsize",
   "SFTKdtc/bin/dtcexpand",
);

@hp23ia64RootFile=(
   "usr/conf/mod/dtc",
);

@hp23paNonrootFile=(
   "SFTKdtc/bin/dtcpsmigrate272",
   "SFTKdtc/bin/dtcstart",
   "SFTKdtc/bin/dtclimitsize",
   "SFTKdtc/bin/dtcexpand",
);

@hp23paRootFile=(
   "usr/conf/mod/dtc",
);

@hp31ia64NonrootFile=(
   "SFTKdtc/bin/dtcpsmigrate272",
   "SFTKdtc/bin/dtcstart",
   "SFTKdtc/bin/dtclimitsize",
   "SFTKdtc/bin/dtcexpand",
);

@hp31ia64RootFile=(
   "usr/conf/mod/dtc",
);

@hp31paNonrootFile=(
   "SFTKdtc/bin/dtcpsmigrate272",
   "SFTKdtc/bin/dtcstart",
   "SFTKdtc/bin/dtclimitsize",
   "SFTKdtc/bin/dtcexpand",
);

@hp31paRootFile=(
   "usr/conf/mod/dtc",
);

@sol10NonrootFile=(
   "SFTKdtc/bin/dtcpsmigrate272",
   "SFTKdtc/bin/dtcstart",
   "SFTKdtc/bin/dtclimitsize",
   "SFTKdtc/bin/dtcexpand",
);

@sol10RootFile=(
   "usr/kernel/drv/dtc",
   "usr/kernel/drv/sparcv9/dtc",
);

@sol9NonrootFile=(
   "SFTKdtc/bin/dtcpsmigrate272",
   "SFTKdtc/bin/dtcstart",
   "SFTKdtc/bin/dtclimitsize",
   "SFTKdtc/bin/dtcexpand",
);

@sol9RootFile=(
   "usr/kernel/drv/dtc",
   "usr/kernel/drv/sparcv9/dtc",
);

@red5xi686NonrootFile=(
   "SFTKdtc/bin/dtcpsmigrate272",
   "SFTKdtc/bin/dtcstart",
);

@red5xi686RootFile=(
   "etc/opt/SFTKdtc/driver/linux-2.6.18-128.el5xen.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-92.el5PAE.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-8.el5PAE.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-128.el5PAE.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-8.el5xen.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-92.el5xen.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-53.el5xen.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-164.el5PAE.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-53.el5.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-164.el5.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-164.el5xen.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-8.el5.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-53.el5PAE.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-128.el5.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-92.el5.i686/sftkdtc.ko",
);

@red5xx64NonrootFile=(
   "SFTKdtc/bin/dtcpsmigrate272",
   "SFTKdtc/bin/dtcstart",
);

@red5xx64RootFile=(
   "etc/opt/SFTKdtc/driver/linux-2.6.18-53.el5xen.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-8.el5.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-8.el5xen.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-92.el5xen.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-128.el5.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-164.el5.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-53.el5.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-128.el5xen.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-92.el5.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.18-164.el5xen.x86_64/sftkdtc.ko",
);

@red6xi686NonrootFile=(
   "SFTKdtc/bin/throtd",
   "SFTKdtc/bin/in.pmd",
);

@red6xi686RootFile=(
   "etc/opt/SFTKdtc/driver/linux-2.6.32-71.el6.i686/sftkdtc.ko",
);

@red6xx64NonrootFile=(
   "SFTKdtc/bin/throtd",
   "SFTKdtc/bin/in.pmd",
);

@red6xx64RootFile=(
   "etc/opt/SFTKdtc/driver/linux-2.6.32-71.el6.x86_64/sftkdtc.ko",
);

@suse10xi686NonrootFile=(
   "SFTKdtc/bin/throtd",
   "SFTKdtc/bin/in.pmd",
);

@suse10xi686RootFile=(
   "etc/opt/SFTKdtc/driver/linux-2.6.16.21-0.8-smp.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.54.5-bigsmp.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.21-0.8-default.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.21-vmi.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.54.5-vmipae.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.46-0.12-xen.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.21-xenpae.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.21-xen.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.46-0.12-xenpae.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.21-0.8-bigsmp.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.46-0.12-smp.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.21-smp.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.54.5-xenpae.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.54.5-vmi.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.54.5-xen.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.21-default.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.21-0.8-xenpae.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.54.5-default.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.21-0.8-xen.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.46-0.12-default.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.54.5-smp.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.21-bigsmp.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.46-0.12-bigsmp.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.21-vmipae.i686/sftkdtc.ko",
);

@suse10xx64NonrootFile=(
   "SFTKdtc/bin/throtd",
   "SFTKdtc/bin/in.pmd",
);

@suse10xx64RootFile=(
   "etc/opt/SFTKdtc/driver/linux-2.6.16.46-0.12-xen.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.54.5-default.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.21-xen.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.21-0.8-smp.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.54.5-smp.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.21-0.8-default.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.46-0.12-smp.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.46-0.12-default.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.21-0.8-xen.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.21-smp.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.54.5-xen.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.16.60-0.21-default.x86_64/sftkdtc.ko",
);

@suse11xi686NonrootFile=(
   "SFTKdtc/bin/throtd",
   "SFTKdtc/bin/in.pmd",
);

@suse11xi686RootFile=(
   "etc/opt/SFTKdtc/driver/linux-2.6.27.19-5-pae.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.32.12-0.7-default.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.32.12-0.7-pae.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.32.12-0.7-xen.i686/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.27.19-5-default.i686/sftkdtc.ko",
);


@suse11xx64NonrootFile=(
   "SFTKdtc/bin/throtd",
   "SFTKdtc/bin/in.pmd",
);

@suse11xx64RootFile=(
   "etc/opt/SFTKdtc/driver/linux-2.6.32.12-0.7-trace.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.32.12-0.7-xen.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.27.19-5-default.x86_64/sftkdtc.ko",
   "etc/opt/SFTKdtc/driver/linux-2.6.32.12-0.7-default.x86_64/sftkdtc.ko",
);

