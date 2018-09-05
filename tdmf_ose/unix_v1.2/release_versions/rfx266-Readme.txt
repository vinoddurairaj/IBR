This is the Release to General Availability of 
	Softek Replicator Version 2.5.3.0 Build 7055    	2007-02-26
for RHEL 3, and
	Softek Replicator for Unix version 2.6.1.0 B08204	2008-06-13
for RHEL 4 itanium, and 
	Softek Replicator for Unix version 2.6.2.0 B09229	2009-02-02
for SuSE9, and
	Softek Replicator for Unix version 2.6.4.0 B11159	2009-04-25
for RHEL 5 x86_64, and
	Softek Replicator for Unix version 2.6.4.0 B11160	2009-07-22
for RHEL 5 32 bit, and
	Softek Replicator for Unix version 2.6.4.0 B11168	2010-07-13
for RHEL 4, SuSE 10, and
	Softek Replicator for UNIX Version 2.6.5.0 B12243	2010-03-23
for SuSE 11, and
	Softek Replicator for UNIX Version 2.6.6.0 B13058	2010-05-21
for HP-UX, and Solaris, and
	Softek Replicator for UNIX Version 2.6.6.0 B13058	2010-08-31
for AIX


The Softek Data Mobility Console is a companion release available in
the data-mobility-console directory.

Documents in pdf format, zipped		(Newer and more PDFs than CDROM)
 15628395 2010-09-07 15:55:16 RFX266.docs.zip

./doc:
Installation and Administrator's Guide for AIX, HP-UX, Linux, and Solaris 
 7613792 2010-06-08 15:19:26 repunix26_install_reference_r18.pdf
 9297364 2010-05-12 23:28:47 dmcwin12_install_user_r1.pdf
Technical Information Bulletins
Messages Guide for AIX, HP-UX, Linux, and Solaris 
 2117302 2010-06-17 11:05:26 repunix26_messages_guide_r18.pdf
Planning and Implementation Guide
Quick Reference Card for Unix
  225354 2010-06-08 15:16:27 repunix26_quickref_card_r18.pdf
  175003 2010-05-12 22:01:48 dmcwin12_quickref_card_r1.pdf
Software Installation and Release Notes
 930177 2010-09-07 15:55:15 repunix26_release_notes_r18.pdf
 843378 2010-09-01 13:15:13 dmcwin12_release_notes_r2.pdf
List of Linux Kernels supported for Replicator
  14762 2010-07-29 02:43:16 rfx-linux-drivers.txt
Supportability Matrices for Open Systems and Mainframe Products
1401542 2010-04-14 12:39:48 SupportabilityMatricesforOpenSystemsandMainframeProducts.pdf

Debugcapture routines to gather logs and configuration data.
These can be used before installing Replicator as a system inventory.
The routines may be newer than those in the released Replicator packages.
./debugcapture

The Linux releases are in the rpm format.
AIX, HP-UX and Solaris releases are compressed to gzip archives.

Installation packages, stored as compressed archives
  42321461 2010-07-13 12:35:28 RFX265.B12243_rpm_files.tgz
 107417382 2010-08-31 16:40:02 RFX266.B13058_B13059_gzip_files.tgz
 108371766 2010-08-31 16:40:02 RFX266.B13058_B13059_zip_files.zip

Subdirectories contain the individual platform packages.
	HPUX Itanium *ia64.depot.gz
	Redhat/SuSE Itanium *ia64.rpm
gzip/
  8465742 2010-08-31 16:40:02 RFX266.B13059-AIX-5.1.tgz
  8657941 2010-08-31 16:37:42 RFX266.B13059-AIX-5.2.tgz
  8699807 2010-08-31 16:37:22 RFX266.B13059-AIX-5.3.tgz
  8569147 2010-08-31 16:37:26 RFX266.B13059-AIX-6.1.tgz
 10522978 2010-05-21 11:27:54 RFX266.B13058-HPUX-1123ia64.depot.gz
  8355024 2010-05-21 11:27:54 RFX266.B13058-HPUX-1123pa.depot.gz
  9888143 2010-05-21 11:29:58 RFX266.B13058-HPUX-1131ia64.depot.gz
  8663852 2010-05-21 11:27:58 RFX266.B13058-HPUX-1131pa.depot.gz
  7723131 2010-05-21 11:28:34 RFX266.B13058-HPUX-11i.depot.gz
  9644223 2010-05-21 11:32:26 RFX266.B13058-solaris-10.pkg.gz
  9573310 2010-05-21 11:31:56 RFX266.B13058-solaris-8.pkg.gz
  9608215 2010-05-21 11:32:16 RFX266.B13058-solaris-9.pkg.gz
rpm/
  394151 2008-06-13 12:36:32 blt-2.4u-8.i386.rpm
 3911010 2010-07-13 12:34:10 RedHat-4x-x86_32-Replicator-2.6.4.0-11168.i686.rpm
 4034961 2010-07-13 12:34:12 RedHat-4x-x86_64-Replicator-2.6.4.0-11168.x86_64.rpm
 3504604 2009-07-22 13:48:59 RedHat-5x-x86_32-Replicator-2.6.4.0-11160.i686.rpm
 3418384 2009-04-25 06:09:20 RedHat-5x-x86_64-Replicator-2.6.4.0-11159.x86_64.rpm
 3356411 2007-02-26 13:55:28 RedHat-AS3x32-Replicator-2.5.3.0-7055.i386.rpm
 3182748 2007-02-26 13:55:28 RedHat-AS3x64-Replicator-2.5.3.0-7055.x86_64.rpm
 5187769 2008-06-13 12:35:44 RedHat-AS4x64-ia64-Replicator-2.6.1.0-8204.ia64.rpm
 2218834 2010-07-13 12:35:28 SuSE-10x-x86_32-Replicator-2.6.4.0-11168.i686.rpm
 2208185 2010-07-13 12:35:26 SuSE-10x-x86_64-Replicator-2.6.4.0-11168.x86_64.rpm
 1674853 2010-03-23 12:32:31 SuSE-11x-x86_32-Replicator-2.6.5.0-12243.i686.rpm
 1290790 2010-03-23 12:32:31 SuSE-11x-x86_64-Replicator-2.6.5.0-12243.x86_64.rpm
 3964402 2009-02-04 13:24:02 SuSE-9x-ia64-Replicator-2.6.2.0-9229.ia64.rpm
 2122329 2009-02-04 13:24:08 SuSE-9x-x64-Replicator-2.6.2.0-9229.x86_64.rpm
 2343415 2009-02-04 13:24:15 SuSE-9x-x86-Replicator-2.6.2.0-9229.i386.rpm
zip/
  8465860 2010-08-31 16:40:02 RFX266.B13059-AIX-5.1.zip
  8656574 2010-08-31 16:37:42 RFX266.B13059-AIX-5.2.zip
  8699039 2010-08-31 16:37:22 RFX266.B13059-AIX-5.3.zip
  8568185 2010-08-31 16:37:26 RFX266.B13059-AIX-6.1.zip
 10523104 2010-05-21 11:27:54 RFX266.B13058-HPUX-1123ia64.zip
  8355149 2010-05-21 11:27:54 RFX266.B13058-HPUX-1123pa.zip
  9888269 2010-05-21 11:29:58 RFX266.B13058-HPUX-1131ia64.zip
  8663977 2010-05-21 11:27:58 RFX266.B13058-HPUX-1131pa.zip
  7723253 2010-05-21 11:28:34 RFX266.B13058-HPUX-11i.zip
  9644355 2010-05-21 11:32:26 RFX266.B13058-solaris-10.zip
  9573442 2010-05-21 11:31:56 RFX266.B13058-solaris-8.zip
  9608347 2010-05-21 11:32:16 RFX266.B13058-solaris-9.zip

cksum * */*
715642385 42321461 RFX265.B12243_rpm_files.tgz
197177150 107417382 RFX266.B13058_B13059_gzip_files.tgz
1640698096 108371766 RFX266.B13058_B13059_zip_files.zip
 820111717  15628395 RFX266.docs.zip

2926136508  8465742 gzip/RFX266.B13059-AIX-5.1.tgz
 386957924  8657941 gzip/RFX266.B13059-AIX-5.2.tgz
1612991843  8699807 gzip/RFX266.B13059-AIX-5.3.tgz
 257327803  8569147 gzip/RFX266.B13059-AIX-6.1.tgz
 811111380 10522978 gzip/RFX266.B13058-HPUX-1123ia64.depot.gz
 683262239  8355024 gzip/RFX266.B13058-HPUX-1123pa.depot.gz
2216509975  9888143 gzip/RFX266.B13058-HPUX-1131ia64.depot.gz
 330339616  8663852 gzip/RFX266.B13058-HPUX-1131pa.depot.gz
2103580972  7723131 gzip/RFX266.B13058-HPUX-11i.depot.gz
3268631643  9644223 gzip/RFX266.B13058-solaris-10.pkg.gz
4275393076  9573310 gzip/RFX266.B13058-solaris-8.pkg.gz
1792931267  9608215 gzip/RFX266.B13058-solaris-9.pkg.gz
 326190438   394151 rpm/blt-2.4u-8.i386.rpm
1219970840  3911010 rpm/RedHat-4x-x86_32-Replicator-2.6.4.0-11168.i686.rpm
4016239683  4034961 rpm/RedHat-4x-x86_64-Replicator-2.6.4.0-11168.x86_64.rpm
1890164577  3504604 rpm/RedHat-5x-x86_32-Replicator-2.6.4.0-11160.i686.rpm
3087138259  3418384 rpm/RedHat-5x-x86_64-Replicator-2.6.4.0-11159.x86_64.rpm
4175299237  3356411 rpm/RedHat-AS3x32-Replicator-2.5.3.0-7055.i386.rpm
2431940214  3182748 rpm/RedHat-AS3x64-Replicator-2.5.3.0-7055.x86_64.rpm
3829377491  5187769 rpm/RedHat-AS4x64-ia64-Replicator-2.6.1.0-8204.ia64.rpm
 460090132  2218834 rpm/SuSE-10x-x86_32-Replicator-2.6.4.0-11168.i686.rpm
 103442321  2208185 rpm/SuSE-10x-x86_64-Replicator-2.6.4.0-11168.x86_64.rpm
1974564118  1674853 rpm/SuSE-11x-x86_32-Replicator-2.6.5.0-12243.i686.rpm
3592622659  1290790 rpm/SuSE-11x-x86_64-Replicator-2.6.5.0-12243.x86_64.rpm
2108302337  3964402 rpm/SuSE-9x-ia64-Replicator-2.6.2.0-9229.ia64.rpm
1457214353  2122329 rpm/SuSE-9x-x64-Replicator-2.6.2.0-9229.x86_64.rpm
1594645826  2343415 rpm/SuSE-9x-x86-Replicator-2.6.2.0-9229.i386.rpm
2052667671  8465860 zip/RFX266.B13059-AIX-5.1.zip
2471673829  8656574 zip/RFX266.B13059-AIX-5.2.zip
3897929410  8699039 zip/RFX266.B13059-AIX-5.3.zip
 979736249  8568185 zip/RFX266.B13059-AIX-6.1.zip
2689094089 10523104 zip/RFX266.B13058-HPUX-1123ia64.zip
1120640307  8355149 zip/RFX266.B13058-HPUX-1123pa.zip
1303095476  9888269 zip/RFX266.B13058-HPUX-1131ia64.zip
1341562008  8663977 zip/RFX266.B13058-HPUX-1131pa.zip
3138842517  7723253 zip/RFX266.B13058-HPUX-11i.zip
4269323310  9644355 zip/RFX266.B13058-solaris-10.zip
3727935757  9573442 zip/RFX266.B13058-solaris-8.zip
1474392148  9608347 zip/RFX266.B13058-solaris-9.zip

DMSopen@us.ibm.com
Thu May 27 11:38:17 EDT 2010 - RFX 2.6.6 General Availability
Thu Jul 29 02:23:04 EDT 2010 - update of RFX 2.6.4 for Linux
Thu Sep  9 12:06:52 EDT 2010 - Documentation update
Fri Sep 10 20:03:41 EDT 2010 - updated AIX for shared jfslogs
