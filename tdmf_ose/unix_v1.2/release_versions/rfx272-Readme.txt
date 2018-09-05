This is the Release to General Availability of 
Softek Replicator for UNIX* Release Version 2.7.2.0 Build 16080 2012-12-07
  AIX 5.3 6.1 7.1
  HP-UX 11.23 11.31
  Solaris 9 10
  RedHat Linux Enterprise 4 5 6 
  SuSe Linux Enterprise 10 11

The Softek Data Mobility Console is a companion release available in
the data-mobility-console directory.

Documents in pdf format, zipped		(Newer and more PDFs than CDROM)
 5756946 2012-12-07 16:03:00 RFX272.docs.zip

Debugcapture routines to gather logs and configuration data.
These can be used before installing Replicator as a system inventory.
The routines may be newer than those in the released Replicator packages.
./debugcapture

The installation CDROM, as an ISO image, gzip compressed.
 82620958 2012-12-07 16:02:59 RFX272.B16080.P89.gold.iso.gz
The uncompressed files from the iso, as a gzip tar
 82610768 2012-12-07 16:02:59 RFX272.B16080.P89.gold.iso-all-files.tgz

All of the Linux RPMs, in one gzip tar.
 10218436 2012-12-10 20:50:21 RFX272.B16080-rpm_files.tgz
Compressed AIX, HP-UX, and Solaris installation packages in one compressed file.
 65996854 2012-11-28 17:45:48 RFX272.B16080-gzip_files.tgz

Subdirectories contain the individual platform compressed packages.
	HPUX Itanium *ia64*
	Linux Itanium supplied upon request
gzip/
 12605496 2012-11-28 17:41:44 RFX272.B16080-AIX.tgz
 12285197 2012-11-28 17:40:52 RFX272.B16080-HPUX-1123ia64.depot.gz
  9307327 2012-11-28 17:41:00 RFX272.B16080-HPUX-1123pa.depot.gz
 11473780 2012-11-28 17:41:38 RFX272.B16080-HPUX-1131ia64.depot.gz
  9612454 2012-11-28 17:40:58 RFX272.B16080-HPUX-1131pa.depot.gz
 11336036 2012-11-28 17:45:48 RFX272.B16080-solaris.pkg.gz

rpm/
 5058540 2012-11-28 17:47:34 Replicator-2.7.2.0-16080.i686.rpm
 5238631 2012-11-28 17:44:11 Replicator-2.7.2.0-16080.x86_64.rpm
List of Linux Kernels directly supported for Replicator
    1428 2012-12-10 20:50:21 rfx-linux-drivers.txt

++++++++++++++++++++++++++++++++++++++++++++++++++++
 7385280 2013-04-05 20:28:01 RFX272psmig.Patch01.tgz
The architecture of the pstore has changed with release 2.7.2

If 2.7.2 is installed as an upgrade to some earlier version, the pstore must
either be re-initialized, or the enclosed binaries may be used to convert the
old pstore to the new format.

If the pstore is initialized, a Full or Checksum Refresh is required to
achieve Normal Mode.  If the pstore is converted using this new utility,
tracking bits are saved, and a Smart Refresh should quickly return the group
to Normal Mode.
++++++++++++++++++++++++++++++++++++++++++++++++++++

md5sum * */*
642332cf9d6655dac3a6adb9634adbe3  RFX272psmig.Patch01.tgz
24aea7030e7127329c662464fa06599b  RFX272.B16080-gzip_files.tgz
257cfd51f9cd88448ba58ac4df907dd0  RFX272.B16080.P89.gold.iso-all-files.tgz
a734b245506697829faa2de39373dfc8  RFX272.B16080.P89.gold.iso.gz
f04b06797de6a7be5c65be7d033589d5  RFX272.B16080-rpm_files.tgz
cc4d89145e87ce8038b3da259566f2dd  RFX272.docs.zip
c93d2ef3d26c347e9ce02deba2c7e968  gzip/RFX272.B16080-AIX.tgz
577d7a3d68d336b64652e7fa7b87e917  gzip/RFX272.B16080-HPUX-1123ia64.depot.gz
c937cca71eaa309c98c28ff19cf86e2a  gzip/RFX272.B16080-HPUX-1123pa.depot.gz
a8e58ab020eb2548d3e49b44980bd798  gzip/RFX272.B16080-HPUX-1131ia64.depot.gz
729a3de15afd33d324b702f30253a87a  gzip/RFX272.B16080-HPUX-1131pa.depot.gz
eb057dd1781d7ff967c22fe178fe74f8  gzip/RFX272.B16080-solaris.pkg.gz
b109ebd4556467c0df5d3b6887f1a989  rpm/Replicator-2.7.2.0-16080.i686.rpm
70ad9465781f8f862e0bc4f4f00e5538  rpm/Replicator-2.7.2.0-16080.x86_64.rpm

DMSopen@us.ibm.com
Mon Dec 10 19:18:27 EST 2012 - General Availability
Fri Apr  5 20:38:37 EDT 2013 - Added RFX272psmig.Patch01

