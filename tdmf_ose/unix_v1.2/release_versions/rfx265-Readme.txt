This is the Release to General Availability of 
	Softek Replicator for Unix version 2.6.1.0 B08204	2008-06-13
for AIX 4.x, 5.x, HP-UX, RHEL 4, and 
	Softek Replicator for Unix version 2.6.2.0 B09063	2008-09-18
for AIX 6.1, and
	Softek Replicator for Unix version 2.6.2.0 B09229	2009-02-02
for SuSE9 on Intel and AMD, and
	Softek Replicator for Unix version 2.6.3.0 B11029	2008-10-29
for Solaris 7,8,9 and
	Softek Replicator for Unix version 2.6.4.0 B11159	2009-04-25
for zLinux SuSE 9, SuSE 10, RHEL 4, and x86_64 SuSE 10, RHEL5
	Softek Replicator for Unix version 2.6.4.0 B11160	2009-07-22
for SuSE 10 32 bit, RHEL5 32 bit, and
	Softek Replicator Release Version 2.6.3.0 Build 10030	2009-07-31
for Solaris 10, and
	Softek Replicator for UNIX* Release Version 2.6.5.0 Build 12243	2010-03-23
for SuSE 11

	Softek Replicator Version 2.5.3.0 Build 7055	2007-02-26
remains supported for RHEL 3 only as an ftp download only.

The Softek Data Mobility Console is a companion release available in
the data-mobility-console directory.

Documents in pdf format, zipped		(Newer and more PDFs than CDROM)
  Documents are maintained separately for Solaris, version 2.6.3
  verses other unix, as 2.6.5
 24267762 2010-03-23 12:32:32 RFX265.docs.zip

./doc:
Administrator's Guide for AIX, HP-UX, Linux, and Solaris 
 8545320 2010-03-22 17:44:08 2.6.5/repunix26_install_user_r16.pdf
 3598905 2008-11-04 12:06:07 2.6.3/repunix26_admin_guide_r7.pdf
 5437833 2009-04-23 16:48:11 dmcwin11_install_user_r3.pdf
Technical Information Bulletins
	TIB 1 - Courier Transport Method
 1854993 2010-01-26 15:21:10 REP-W31TIB-002.pdf
Installation Guide for AIX, HP-UX, Linux, and Solaris 
 8545320 2010-03-22 17:44:08 2.6.5/repunix26_install_user_r16.pdf
 2326280 2008-11-04 12:05:09 2.6.3/repunix26_install_guide_r9.pdf
Messages Guide for AIX, HP-UX, Linux, and Solaris 
 1359405 2010-03-22 09:01:48 2.6.5/repunix26_messages_guide_r16.pdf
 1401551 2008-11-04 12:06:28 2.6.3/repunix26_messages_guide_r5.pdf
Planning and Implementation Guide
 3437504 2009-05-12 17:38:43 2.6.5/repunix26_planning_implement_r15.pdf
 3424704 2008-11-04 12:05:28 2.6.3/repunix26_planning_implement_r8.pdf
Quick Reference Card for Unix
  215184 2010-03-22 11:22:46 2.6.5/repunix26_quickref_card_r16.pdf
  196318 2008-11-03 15:14:12 2.6.3/repunix26_quickref_card_r6.pdf
  191027 2009-04-23 17:26:07 dmcwin11_quickref_card_r2.pdf
Software Installation and Release Notes
  947408 2009-02-11 14:11:58 2.6.3/repunix26_release_notes_r14.pdf
  834121 2009-12-01 10:58:11 dmcwin11_release_notes_r4.pdf
List of Linux Kernels supported for Replicator
 11021 2010-03-23 12:32:31 rfx-linux-drivers.txt
Supportability Matrices for Open Systems and Mainframe Products
 1426595 2010-02-18 13:01:32 SupportabilityMatricesforOpenSystemsandMainframeProducts.pdf

Debugcapture routines to gather logs and configuration data.
These can be used before installing Replicator as a system inventory.
The routines may be newer than those in the released Replicator packages.
./debugcapture

The Linux releases are in the rpm format.
AIX, HP-UX and Solaris releases are compressed to gzip archives.

Installation packages, stored as compressed archives
  53484849 2010-03-23 12:32:31 RFX265.B12243_rpm_files.tgz	Linux
 126589613 2009-07-31 12:49:08 RFX264.B10030_gzip_files.tgz	non-Linux

Subdirectories contain the individual platform packages.
	HPUX PA-RISC *pa.depot.gz
	HPUX Itanium *ipf.depot.gz
gzip:
   8273849 2008-06-13 09:35:48 RFX261.B08204-AIX-4.3.3.tgz
   8397299 2008-06-13 09:35:50 RFX261.B08204-AIX-5.1.tgz
   8589721 2008-06-13 09:35:52 RFX261.B08204-AIX-5.2.tgz
   8624117 2008-06-13 09:35:54 RFX261.B08204-AIX-5.3.tgz
   8524693 2008-09-18 10:06:30 RFX262.B09063-AIX-6.1.tgz
  10235321 2008-06-13 09:35:38 RFX261.B08204-HPUX-1123ipf.depot.gz
   8774765 2008-06-13 09:35:34 RFX261.B08204-HPUX-1123pa.depot.gz
   9624442 2008-06-13 09:35:40 RFX261.B08204-HPUX-1131ipf.depot.gz
   9105338 2008-06-13 09:35:42 RFX261.B08204-HPUX-1131pa.depot.gz
   8215988 2008-06-13 09:35:32 RFX261.B08204-HPUX-11i.depot.gz
   9516715 2008-10-29 13:31:36 RFX263.B10029-solaris-7.pkg.gz
   9894743 2008-10-29 13:31:40 RFX263.B10029-solaris-8.pkg.gz
   9925804 2008-10-29 13:31:44 RFX263.B10029-solaris-9.pkg.gz
   9975281 2009-07-31 12:49:08 RFX263.B10030-solaris-10.pkg.gz
rpm:
 3805525 2009-04-25 05:27:54 RedHat-5x-s390x-Replicator-2.6.4.0-11159.s390x.rpm
 3504604 2009-07-22 13:48:59 RedHat-5x-x86_32-Replicator-2.6.4.0-11160.i686.rpm
 3418384 2009-04-25 06:09:20 RedHat-5x-x86_64-Replicator-2.6.4.0-11159.x86_64.rpm
 3356411 2007-02-26 13:55:28 RedHat-AS3x32-Replicator-2.5.3.0-7055.i386.rpm
 3182748 2007-02-26 13:55:28 RedHat-AS3x64-Replicator-2.5.3.0-7055.x86_64.rpm
 3588159 2008-06-13 12:35:43 RedHat-AS4x32-Replicator-2.6.1.0-8204.i386.rpm
 5187769 2008-06-13 12:35:44 RedHat-AS4x64-ia64-Replicator-2.6.1.0-8204.ia64.rpm
 3494313 2008-06-13 12:35:44 RedHat-AS4x64-Replicator-2.6.1.0-8204.x86_64.rpm
 3570825 2008-06-13 12:36:59 RedHat-AS4x64-s390x-Replicator-2.6.1.0-8204.s390x.rpm
 2476322 2009-04-25 05:42:43 SuSE-10x-s390x-Replicator-2.6.4.0-11159.s390x.rpm
 2196849 2009-07-22 13:50:07 SuSE-10x-x86_32-Replicator-2.6.4.0-11160.i686.rpm
 2189744 2009-04-25 06:10:29 SuSE-10x-x86_64-Replicator-2.6.4.0-11159.x86_64.rpm
 3964402 2009-02-04 13:24:02 SuSE-9x-ia64-Replicator-2.6.2.0-9229.ia64.rpm
 2307371 2009-04-25 05:36:40 SuSE-9x-s390x-Replicator-2.6.4.0-11159.s390x.rpm
 2122329 2009-02-04 13:24:08 SuSE-9x-x64-Replicator-2.6.2.0-9229.x86_64.rpm
 2343415 2009-02-04 13:24:15 SuSE-9x-x86-Replicator-2.6.2.0-9229.i386.rpm
  394151 2008-06-13 12:36:32 blt-2.4u-8.i386.rpm

cksum * */*
2311195807 126589613 RFX264.B10030_gzip_files.tgz
2449507328  53484849 RFX265.B12243_rpm_files.tgz
2176829808  24267762 RFX265.docs.zip
gzip:
2095410487  8273849 RFX261.B08204-AIX-4.3.3.tgz
2170147366  8397299 RFX261.B08204-AIX-5.1.tgz
1972258277  8589721 RFX261.B08204-AIX-5.2.tgz
2605286329  8624117 RFX261.B08204-AIX-5.3.tgz
2288934994  8524693 RFX262.B09063-AIX-6.1.tgz
2380224052 10235321 RFX261.B08204-HPUX-1123ipf.depot.gz
  36924780  8774765 RFX261.B08204-HPUX-1123pa.depot.gz
3536006711  9624442 RFX261.B08204-HPUX-1131ipf.depot.gz
4011327910  9105338 RFX261.B08204-HPUX-1131pa.depot.gz
 213645455  8215988 RFX261.B08204-HPUX-11i.depot.gz
4026943857  9516715 RFX263.B10029-solaris-7.pkg.gz
2812447761  9894743 RFX263.B10029-solaris-8.pkg.gz
2476776916  9925804 RFX263.B10029-solaris-9.pkg.gz
 318617036  9975281 RFX263.B10030-solaris-10.pkg.gz
rpm:
3455205255 3805525 RedHat-5x-s390x-Replicator-2.6.4.0-11159.s390x.rpm
1890164577 3504604 RedHat-5x-x86_32-Replicator-2.6.4.0-11160.i686.rpm
3087138259 3418384 RedHat-5x-x86_64-Replicator-2.6.4.0-11159.x86_64.rpm
4175299237 3356411 RedHat-AS3x32-Replicator-2.5.3.0-7055.i386.rpm
2431940214 3182748 RedHat-AS3x64-Replicator-2.5.3.0-7055.x86_64.rpm
 785150619 3588159 RedHat-AS4x32-Replicator-2.6.1.0-8204.i386.rpm
3829377491 5187769 RedHat-AS4x64-ia64-Replicator-2.6.1.0-8204.ia64.rpm
1494650828 3494313 RedHat-AS4x64-Replicator-2.6.1.0-8204.x86_64.rpm
2722102282 3570825 RedHat-AS4x64-s390x-Replicator-2.6.1.0-8204.s390x.rpm
2108302337 3964402 SuSE-9x-ia64-Replicator-2.6.2.0-9229.ia64.rpm
1934399832 2307371 SuSE-9x-s390x-Replicator-2.6.4.0-11159.s390x.rpm
1457214353 2122329 SuSE-9x-x64-Replicator-2.6.2.0-9229.x86_64.rpm
1594645826 2343415 SuSE-9x-x86-Replicator-2.6.2.0-9229.i386.rpm
3634008756 2476322 SuSE-10x-s390x-Replicator-2.6.4.0-11159.s390x.rpm
1972270517 2196849 SuSE-10x-x86_32-Replicator-2.6.4.0-11160.i686.rpm
3163386074 2189744 SuSE-10x-x86_64-Replicator-2.6.4.0-11159.x86_64.rpm
1974564118 1674853 SuSE-11x-x86_32-Replicator-2.6.5.0-12243.i686.rpm
3592622659 1290790 SuSE-11x-x86_64-Replicator-2.6.5.0-12243.x86_64.rpm

DMSopen@us.ibm.com
Thu Apr  1 15:32:07 EDT 2010 - RFX 2.6.5 General Availability
