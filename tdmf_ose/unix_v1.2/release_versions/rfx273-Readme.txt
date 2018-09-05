This is the Release to General Availability of 
Softek Replicator for UNIX* Release Version 2.7.3.0 Build 17047 2013-12-02
  AIX 5.3 6.1 7.1
  HP-UX 11.23 11.31
  Solaris 9 10
  RedHat Linux Enterprise 4 5 6 
  SuSe Linux Enterprise 10 11

The Softek Data Mobility Console is a companion release available in
the data-mobility-console directory.

Documents in pdf format, zipped		(Newer and more PDFs than CDROM)
   5776976 2013-12-02 08:19:18 RFX273.docs.zip 

Debugcapture routines to gather logs and configuration data.
These can be used before installing Replicator as a system inventory.
The routines may be newer than those in the released Replicator packages.
./debugcapture

The installation CDROM, as an ISO image, gzip compressed.
  84124315 2013-12-02 08:20:51 RFX273.B17047.P6.gold.iso.gz 
The uncompressed files from the iso, as a gzip tar
  84104095 2013-12-02 08:19:00 RFX273.B17047.P6.gold.iso-all-files.tgz 

All of the Linux RPMs, in one gzip tar.
  10673469 2013-12-03 12:30:00 RFX273.B17047-rpm_files.tgz 
Compressed AIX, HP-UX, and Solaris installation packages in one compressed file.
  66992269 2013-11-19 09:32:00 RFX273.B17047-gzip_files.tgz 

Subdirectories contain the individual platform compressed packages.
	HPUX Itanium *ia64*
	Linux Itanium supplied upon request
gzip:
 12960023 2013-11-19 12:30:52 RFX273.B17047-AIX.tgz
 12600216 2013-11-19 12:31:14 RFX273.B17047-HPUX-1123ia64.depot.gz
  9476738 2013-11-19 12:29:44 RFX273.B17047-HPUX-1123pa.depot.gz
 11755474 2013-11-19 12:32:30 RFX273.B17047-HPUX-1131ia64.depot.gz
  9789516 2013-11-19 12:29:38 RFX273.B17047-HPUX-1131pa.depot.gz
 11056871 2013-11-19 12:30:20 RFX273.B17047-solaris.pkg.gz

rpm:
 5281488 2013-11-19 12:37:24 Replicator-2.7.3.0-17047.i686.rpm
 5478727 2013-11-19 12:34:11 Replicator-2.7.3.0-17047.x86_64.rpm
    1547 2013-12-03 15:30:46 rfx-linux-drivers.txt

zip:
 12961642 2013-11-19 12:30:52 RFX273.B17047-AIX.zip
 12600360 2013-11-19 12:31:14 RFX273.B17047-HPUX-1123ia64.zip
  9476881 2013-11-19 12:29:44 RFX273.B17047-HPUX-1123pa.zip
 11755618 2013-11-19 12:32:30 RFX273.B17047-HPUX-1131ia64.zip
  9789659 2013-11-19 12:29:38 RFX273.B17047-HPUX-1131pa.zip
 11057021 2013-11-19 12:30:20 RFX273.B17047-solaris.zip

Batch check of md5sums:
	tr -d "\015" < rfx273-Readme.txt |md5sum --check -
22490fe2840c6e00c5360f57c2166175  RFX273.B17047-gzip_files.tgz
b55d5d758c3e33749f391945e4ade76b  RFX273.B17047.P6.gold.iso-all-files.tgz
bbd21da28d41a3b9a33adc4108df3492  RFX273.B17047.P6.gold.iso.gz
0743987e080dd354c36949a1b0bb7338  RFX273.B17047-rpm_files.tgz
e3de23943c3b0db30a51e2bb402466db  RFX273.B17047-zip_files.zip
6592f37a0f00be75857b3d1e3aec3ad6  RFX273.docs.zip
0535c232e537e3e94836bfff3139ce0c  gzip/RFX273.B17047-AIX.tgz
dcfaf971f548b42edc4ed02a938dcf19  gzip/RFX273.B17047-HPUX-1123ia64.depot.gz
ebd408e29243476ead61563938e90245  gzip/RFX273.B17047-HPUX-1123pa.depot.gz
bba416e6faa797592e6990b9eb823481  gzip/RFX273.B17047-HPUX-1131ia64.depot.gz
732da1fa3e8525c4b03ac01b1d27ea84  gzip/RFX273.B17047-HPUX-1131pa.depot.gz
071b49def09a07dc15d30019ded106cc  gzip/RFX273.B17047-solaris.pkg.gz
c7366e1940c82cecd6224549ac4b5152  rpm/Replicator-2.7.3.0-17047.i686.rpm
cc29b02cebeb5186852581b51a31c15b  rpm/Replicator-2.7.3.0-17047.x86_64.rpm
c84b0e80e9a93d1e4c899ed2fb0668d2  rpm/rfx-linux-drivers.txt
787cea4d3a558e88a773232e0ea31103  zip/RFX273.B17047-AIX.zip
f2f734afd42b888b6d09719d0adf389a  zip/RFX273.B17047-HPUX-1123ia64.zip
57d0c3ad0047c3d84beafcbb1ee6d833  zip/RFX273.B17047-HPUX-1123pa.zip
5f08f07a9c84724615d181b617d92d68  zip/RFX273.B17047-HPUX-1131ia64.zip
f039ee6c999acf13931620c36d24b3db  zip/RFX273.B17047-HPUX-1131pa.zip
275397d2013aef88a584ab821dc4cfb8  zip/RFX273.B17047-solaris.zip

DMSopen@us.ibm.com
Thu Dec  5 13:16:26 EST 2013 - General Availability

