This is the Release to General Availability of Replicator for Unix 2.5.2
	Softek Replicator  Version 2.5.2.0 Build 6046
and Redhat Linux AS/ES 4 only:
	Softek Replicator  Version 2.5.1.0 Build 6536

The Softek Common Console Version 2.5.1 is a companion release 
available in the common-console-only directory.

An ISO image of the 2.5.2 installation CDROM, and the iso gzip
	  283836416 Oct 13 08:55 RFX252.B06046.1.iso
	   97085614 Oct 13 08:55 RFX252.B06046.1.iso.gz
An ISO image of the 2.5.1 installation CDROM, and the iso gzip
	  15552512 Sep 14 12:42 RFX251.B06536.1.iso
	  11321640 Sep 14 12:42 RFX251.B06536.1.iso.gz

Documents in pdf format, zipped		(Newer and more PDFs than CDROM)
	   5844586 Oct 13 11:21 unixdocs.zip

./doc:
Administrator's Guide for AIX, HP-UX, Linux, and Solaris 
	   2604699 Oct 10 10:16 repunix25_admin_guide_r3.pdf
Installation Guide for AIX, HP-UX, Linux, and Solaris 
	   1239595 Oct 13 08:40 repunix25_install_guide_r4.pdf
Messages Guide for AIX, HP-UX, Linux, and Solaris 
	    844516 Oct 10 10:10 repunix25_messages_guide_r3.pdf
Planning and Implementation Guide
	   2924265 Oct 10 10:33 repunix25_planning_implement_r3.pdf
Quick Reference Card for Unix
	    359732 Oct 10 10:45 repunix25_quick_reference_r3.pdf
Software Installation and Release Notes
	    383092 Oct 13 11:21 repunix25_release_notes_r4.pdf

Debugcapture routines to gather logs and configuration data.
These can be used before installing Replicator as a system inventory.
The routines may be newer than those in the released Replicator packages.

./debugcapture
	     17965 Oct 11 14:45 debugcapture-AIX
	     19181 Oct 11 14:45 debugcapture-hpux
	     12757 Oct 11 14:45 debugcapture-Linux
	     18076 Oct 11 14:45 debugcapture-solaris

The Linux release is only in the rpm format.
AIX, HP-UX and Solaris releases are the same packages in zip and gzip archives.
These subdirectories contain the individual platform packages.

gzip:
	   7548070 Oct 10 11:21 AIX-4.3.3.tgz
	   7670441 Oct 10 11:21 AIX-5.1.tgz
	   7665477 Oct 10 11:21 AIX-5.2.tgz
	   7773100 Oct 10 11:21 AIX-5.3.tgz
	   9381414 Oct 10 11:21 HPUX-1123ipf.depot.gz
	   8023113 Oct 10 11:21 HPUX-1123pa.depot.gz
	   7448949 Oct 10 11:21 HPUX-11.depot.gz
	   7604167 Oct 10 11:21 HPUX-11i.depot.gz
	   5764127 Oct 10 11:21 solaris-10.pkg.gz
	   5725203 Oct 10 11:21 solaris-2.6.pkg.gz
	   5750211 Oct 10 11:21 solaris-7.pkg.gz
	   5753568 Oct 10 11:21 solaris-8.pkg.gz
	   5777966 Oct 10 11:21 solaris-9.pkg.gz

zip:
	   7548728 Oct 10 11:21 AIX-4.3.3.zip
	   7670609 Oct 10 11:21 AIX-5.1.zip
	   7664316 Oct 10 11:21 AIX-5.2.zip
	   7774926 Oct 10 11:21 AIX-5.3.zip
	   9381540 Oct 10 11:21 HPUX-1123ipf.zip
	   8023238 Oct 10 11:21 HPUX-1123pa.zip
	   7604289 Oct 10 11:21 HPUX-11i.zip
	   7449070 Oct 10 11:21 HPUX-11.zip
	   5764259 Oct 10 11:21 solaris-10.zip
	   5725335 Oct 10 11:21 solaris-2.6.zip
	   5750343 Oct 10 11:21 solaris-7.zip
	   5753700 Oct 10 11:21 solaris-8.zip
	   5778098 Oct 10 11:21 solaris-9.zip

rpm:
	   2987455 Sep  7 20:37 AS4x32-Replicator-2.5.1.0-6536.i386.rpm
	   2890008 Sep  7 20:37 AS4x64-Replicator-2.5.1.0-6536.x86_64.rpm
	redistributed pre-requisite:
	    394151 Sep  7 20:37 blt-2.4u-8.i386.rpm

cksum * */*
2122748038  15552512 RFX251.B06536.1.iso
3769052220  11321640 RFX251.B06536.1.iso.gz
 596926263 283836416 RFX252.B06046.1.iso
3002006326  97085614 RFX252.B06046.1.iso.gz
2526756016   5844586 unixdocs.zip

gzip
 955784669 7548070 AIX-4.3.3.tgz
3546066921 7670441 AIX-5.1.tgz
1700490781 7665477 AIX-5.2.tgz
3989035118 7773100 AIX-5.3.tgz
 669864751 9381414 HPUX-1123ipf.depot.gz
 49993079 8023113 HPUX-1123pa.depot.gz
596061657 7448949 HPUX-11.depot.gz
2658104024 7604167 HPUX-11i.depot.gz
3208586226 5764127 solaris-10.pkg.gz
2445644230 5725203 solaris-2.6.pkg.gz
743934644 5750211 solaris-7.pkg.gz
3019227233 5753568 solaris-8.pkg.gz
2230492741 5777966 solaris-9.pkg.gz

rpm
737769001 2987455 AS4x32-Replicator-2.5.1.0-6536.i386.rpm
496642813 2890008 AS4x64-Replicator-2.5.1.0-6536.x86_64.rpm
326190438 394151 blt-2.4u-8.i386.rpm

zip
1430487413 7548728 AIX-4.3.3.zip
1823651351 7670609 AIX-5.1.zip
1529637016 7664316 AIX-5.2.zip
1557023225 7774926 AIX-5.3.zip
152049298 9381540 HPUX-1123ipf.zip
1858790044 8023238 HPUX-1123pa.zip
492227393 7604289 HPUX-11i.zip
314168610 7449070 HPUX-11.zip
3775894884 5764259 solaris-10.zip
1481446362 5725335 solaris-2.6.zip
2451160809 5750343 solaris-7.zip
2671630713 5753700 solaris-8.zip
2498011273 5778098 solaris-9.zip

CDold@Softek.com 
Fri Oct 13 20:46:57 PDT 2006
