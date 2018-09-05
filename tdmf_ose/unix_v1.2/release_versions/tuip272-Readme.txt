This is the Release to General Availability of 
Softek TDMF(IP) for UNIX* Release Version 2.7.2.0 Build 16080 2012-12-07
  AIX 5.3 6.1 7.1
  HP-UX 11.23 11.31
  Solaris 9 10
  RedHat Linux Enterprise 4 5 6 
  SuSe Linux Enterprise 10 11

Some recent older platforms, supported under TDMF-IP 2.6, are available with
limited support in the gzip/older_platforms and rpm/older_platforms directories. 
AIX-5.1 AIX-5.2 HPUX-11i RedHat-3 RedHat-4-Itanium solaris-7 solaris-8 SuSE-9

The Softek Data Mobility Console is a companion release available in
the data-mobility-console directory.

Documents in pdf format, zipped		(Newer and more PDFs than CDROM)
  5702109 2012-12-07 16:03:00 TUIP272.docs.zip

Debugcapture routines to gather logs and configuration data.
These can be used before installing TDMF-IP as a system inventory.
The routines may be newer than those in the released TDMF-IP packages.
./debugcapture

The installation CDROM, as an ISO image, gzip compressed.
 82563082 2012-12-07 16:03:00 TUIP272.B16080.P89.gold.iso.gz
The uncompressed files from the iso, as a gzip tar
 82543794 2012-12-07 16:03:00 TUIP272.B16080.P89.gold.iso-all-files.tgz

All of the Linux RPMs, in one gzip tar.
 10218052 2012-12-10 20:57:00 TUIP272.B16080-rpm_files.tgz
Compressed AIX, HP-UX, and Solaris installation packages in one compressed file.
 65986193 2012-11-28 17:51:52 TUIP272.B16080-gzip_files.tgz

Subdirectories contain the individual platform compressed packages.
	HPUX Itanium *ia64*
	Linux Itanium supplied upon request
./gzip:
total 65164
 12605309 2012-11-28 17:43:58 TUIP272.B16080-AIX.tgz
 12285932 2012-11-28 17:42:20 TUIP272.B16080-HPUX-1123ia64.depot.gz
  9304058 2012-11-28 17:42:18 TUIP272.B16080-HPUX-1123pa.depot.gz
 11472152 2012-11-28 17:43:40 TUIP272.B16080-HPUX-1131ia64.depot.gz
  9612348 2012-11-28 17:42:16 TUIP272.B16080-HPUX-1131pa.depot.gz
 11335789 2012-11-28 17:51:52 TUIP272.B16080-solaris.pkg.gz
     older_platforms/

./rpm:
total 10096
     118 2012-12-10 20:57:00 rpm-drivers-check.txt
 5057223 2012-11-28 17:54:06 TDMFIP-2.7.2.0-16080.i686.rpm
 5238985 2012-11-28 17:48:22 TDMFIP-2.7.2.0-16080.x86_64.rpm
    1420 2012-12-10 20:57:00 tuip-linux-drivers.txt
    older_platforms/

++++++++++++++++++++++++++++++++++++++++++++++++++++
 7385260 2013-04-05 20:29:07 TUIP272psmig.Patch01.tgz
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
af31af1cc7e65205eb644a762c8ade29  TUIP272psmig.Patch01.tgz
1ba586e46d929a23f91f6785a53f4549  TUIP272.B16080-gzip_files.tgz
219c5ba809f3a4733e23441e62e83fea  TUIP272.B16080.P89.gold.iso-all-files.tgz
4022ac231dc3b99cd85c547dc1327387  TUIP272.B16080.P89.gold.iso.gz
1c43d87a021721dc23ade2721102a1f4  TUIP272.B16080-rpm_files.tgz
1a8040bf55391b871c647842e5cc48de  TUIP272.docs.zip
8342eb51e93a622bc077245fc0602aff  gzip/TUIP272.B16080-AIX.tgz
572b2c0bbc1cbfffbe617ef05e0c541b  gzip/TUIP272.B16080-HPUX-1123ia64.depot.gz
3a2fd996a811276245cf4054675419f6  gzip/TUIP272.B16080-HPUX-1123pa.depot.gz
ff8ac79127dadb988b673441a6d28939  gzip/TUIP272.B16080-HPUX-1131ia64.depot.gz
fd3f0bca680c7d689d5c063f87e24f6a  gzip/TUIP272.B16080-HPUX-1131pa.depot.gz
4b9ba7bf5f2beb28ea0a5a28d3dbfc07  gzip/TUIP272.B16080-solaris.pkg.gz
00494deae6a7a16242ee413a8f65e2ba  rpm/TDMFIP-2.7.2.0-16080.i686.rpm
35128a2332da0e7ce6ffda9afa1d22f5  rpm/TDMFIP-2.7.2.0-16080.x86_64.rpm

DMSopen@us.ibm.com
Tue Dec 11 12:09:04 EST 2012 - Release to General Availability
Tue Apr  2 14:25:41 EDT 2013 - typographical error edits to this file.
Fri Apr  5 20:41:29 EDT 2013 - Added TUIP272psmig.Patch01
