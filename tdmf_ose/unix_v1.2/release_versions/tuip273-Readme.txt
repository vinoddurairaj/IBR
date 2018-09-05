This is the Release to General Availability of 
Softek TDMF(IP) for UNIX* Release Version 2.7.3.0 Build 17047 2013-12-02
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
   5780530 2013-12-02 08:19:18 TUIP273.docs.zip 

Debugcapture routines to gather logs and configuration data.
These can be used before installing TDMF-IP as a system inventory.
The routines may be newer than those in the released TDMF-IP packages.
./debugcapture

The installation CDROM, as an ISO image, gzip compressed.
  84118937 2013-12-02 08:20:55 TUIP273.B17047.P6.gold.iso.gz 
The uncompressed files from the iso, as a gzip tar
  84090003 2013-12-02 08:19:00 TUIP273.B17047.P6.gold.iso-all-files.tgz 

All of the Linux RPMs, in one gzip tar.
  10672315 2013-12-03 12:26:00 TUIP273.B17047-rpm_files.tgz 

Compressed AIX, HP-UX, and Solaris installation packages in one compressed file.
  66986355 2013-11-19 09:37:00 TUIP273.B17047-gzip_files.tgz 

Subdirectories contain the individual platform compressed packages.
	HPUX Itanium *ia64*
	Linux Itanium supplied upon request
./gzip:
  12958412 2013-11-19 09:33:26 TUIP273.B17047-AIX.tgz 
  12596792 2013-11-19 09:34:38 TUIP273.B17047-HPUX-1123ia64.depot.gz 
   9476272 2013-11-19 09:31:04 TUIP273.B17047-HPUX-1123pa.depot.gz 
  11754853 2013-11-19 09:37:24 TUIP273.B17047-HPUX-1131ia64.depot.gz 
   9783280 2013-11-19 09:30:56 TUIP273.B17047-HPUX-1131pa.depot.gz 
  11059490 2013-11-19 09:32:14 TUIP273.B17047-solaris.pkg.gz 

./rpm:
      1539 2013-12-03 12:26:40 rfx-linux-drivers.txt 
   5277484 2013-11-19 09:45:13 TDMFIP-2.7.3.0-17047.i686.rpm 
   5478323 2013-11-19 09:38:51 TDMFIP-2.7.3.0-17047.x86_64.rpm 

Batch check of md5sums:
	tr -d "\015" < tuip273-Readme.txt  |md5sum --check -
b05c149e6be3f0a0be5cbae0e5e1cbed  TUIP273.B17047-gzip_files.tgz
20b00878d768b9b2cc924c1cc0af10a0  TUIP273.B17047.P6.gold.iso-all-files.tgz
a1f3f06759ec236f4d14f089b8682031  TUIP273.B17047.P6.gold.iso.gz
01215b4f757c2785a5191ea97dbaedae  TUIP273.B17047-rpm_files.tgz
61c576e9d7eb2295701af9fa19dd1043  TUIP273.B17047-zip_files.zip
e441260af48be58ed38570f96ccbd8ed  TUIP273.docs.zip
efd6b1bdaf1f18a571f0d4f6cf387b83  gzip/TUIP273.B17047-AIX.tgz
b05bafebf156be4089f7d60871d14dd3  gzip/TUIP273.B17047-HPUX-1123ia64.depot.gz
0a699a5610e50bd6249498246e90cb06  gzip/TUIP273.B17047-HPUX-1123pa.depot.gz
666679587e18e10afc4a8b85f9f9a733  gzip/TUIP273.B17047-HPUX-1131ia64.depot.gz
18f353edd103840290a528a7546305d9  gzip/TUIP273.B17047-HPUX-1131pa.depot.gz
b25d3689eb5f93d6801575598e30a09d  gzip/TUIP273.B17047-solaris.pkg.gz
6c7e50ba59ebe602f8b7139509648d97  rpm/rfx-linux-drivers.txt
a77763daf5f85f5da54b0c292f76700e  rpm/TDMFIP-2.7.3.0-17047.i686.rpm
2df74b4e9b5ea34c9952abf51d620a36  rpm/TDMFIP-2.7.3.0-17047.x86_64.rpm
d84cda38db1c86ada8ca83bc21d6ee4c  zip/TUIP273.B17047-AIX.zip
35ceeeb270f20032d516a9648b5860e9  zip/TUIP273.B17047-HPUX-1123ia64.zip
079c895dcaaa926cc95a89e8cc929f5b  zip/TUIP273.B17047-HPUX-1123pa.zip
1cbdeb20957f949517ee5ed746a9d59e  zip/TUIP273.B17047-HPUX-1131ia64.zip
a1085cff14337563f38e8d9e18ffef0c  zip/TUIP273.B17047-HPUX-1131pa.zip
ca6cc44770bdeb0f512d98dd267117da  zip/TUIP273.B17047-solaris.zip

DMSopen@us.ibm.com
Tue Dec  3 13:49:09 PST 2013 - Release to General Availability
