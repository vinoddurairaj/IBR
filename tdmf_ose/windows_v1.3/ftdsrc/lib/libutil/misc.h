#ifndef _MISC_H_
#define _MISC_H_

/*
 * misc.h -- miscellaneous routines
 *
 * (c) Copyright 1999 Legato Systems, Inc. All Rights Reserved
 *
 */

/* function prototypes */
extern int stringcompare_addr(void *s1, void *s2);
extern int stringcompare_const(const void *s1, const void *s2);
extern int stringcompare(void *s1, void *s2);
extern int getbufline(char **buffer, char **key, char **value, char delim);
extern int buf_all_zero(char *buf, int buflen);

#if defined(_WINDOWS)

#ifdef __cplusplus
extern "C"{ 
#endif

#define VOLUME_LOCK_TIMEOUT		10000		// 10 Seconds
#define VOLUME_LOCK_RETRIES		20

//
// NTFS volume information
//
// Mike Pollett
//typedef struct {
//	LARGE_INTEGER    	SerialNumber;
//	LARGE_INTEGER    	NumberOfSectors;
//	LARGE_INTEGER    	TotalClusters;
//	LARGE_INTEGER    	FreeClusters;
//	LARGE_INTEGER    	Reserved;
//	DWORD    			BytesPerSector;
//	DWORD    			BytesPerCluster;
//	DWORD    			BytesPerMFTRecord;
//	DWORD    			ClustersPerMFTRecord;
//	LARGE_INTEGER    	MFTLength;
//	LARGE_INTEGER    	MFTStart;
//	LARGE_INTEGER    	MFTMirrorStart;
//	LARGE_INTEGER    	MFTZoneStart;
//	LARGE_INTEGER    	MFTZoneEnd;
//} NTFS_VOLUME_DATA_BUFFER, *PNTFS_VOLUME_DATA_BUFFER;

typedef struct {
	ULONGLONG					cluster;
	ULONGLONG					num;
    LIST_ENTRY ListEntry;
} CLUSTER_GROUP, *PCLUSTER_GROUP;

#define IS_MAGIC(a,b)		(*(int*)(a)==*(int*)(b))
#define IS_NTFS_VOLUME(a)	IS_MAGIC((a)+3,"NTFS")
#define IS_FAT16_VOLUME(a)	IS_MAGIC((a)+3,"MSDOS5.0")
#pragma pack(1)

typedef struct  _FAT16_BOOT_SECTOR
{
    UCHAR       bsJump[3];          // x86 jmp instruction, checked by FS
    CCHAR       bsOemName[8];       // OEM name of formatter
    USHORT      bsBytesPerSec;      // Bytes per Sector
    UCHAR       bsSecPerClus;       // Sectors per Cluster
    USHORT      bsResSectors;       // Reserved Sectors
    UCHAR       bsFATs;             // Number of FATs
    USHORT      bsRootDirEnts;      // Number of Root Dir Entries
    USHORT      bsSectors;          // Number of Sectors
    UCHAR       bsMedia;            // Media type
    USHORT      bsFATsecs;          // Number of FAT sectors
    USHORT      bsSecPerTrack;      // Sectors per Track
    USHORT      bsHeads;            // Number of Heads
    ULONG       bsHiddenSecs;       // Hidden Sectors 
    ULONG       bsHugeSectors;      // Number of Sectors if > 32 MB size
    UCHAR       bsDriveNumber;      // Drive Number
    UCHAR       bsReserved1;        // Reserved
    UCHAR       bsBootSignature;    // New Format Boot Signature
    ULONG       bsVolumeID;         // VolumeID
    CCHAR       bsLabel[11];        // Label
    CCHAR       bsFileSystemType[8];// File System Type - FAT12 or FAT16
    CCHAR       bsReserved2[448];   // Reserved
    UCHAR       bsSig2[2];          // Originial Boot Signature - 0x55, 0xAA
}   FAT16_BOOT_SECTOR, *PFAT16_BOOT_SECTOR;

typedef struct  _NTFS_BOOT_SECTOR
{
    UCHAR       bsJump[3];          // x86 jmp instruction, checked by FS
    CCHAR       bsOemName[8];       // OEM name of formatter
    USHORT      bsBytesPerSec;      // Bytes per Sector
    UCHAR       bsSecPerClus;       // Sectors per Cluster
    USHORT      bsResSectors;       // Reserved Sectors
    UCHAR       bsAlways00;
    USHORT      bsAlways01;
    USHORT      bsSectors;          // Number of Sectors
    UCHAR       bsMedia;            // Media type
    USHORT      bsAlways02;
    USHORT      bsSecPerTrack;      // Sectors per Track
    USHORT      bsHeads;            // Number of Heads
    ULONG       bsHiddenSecs;       // Hidden Sectors 
    ULONG       bsNotUsed0;
    ULONG       bsNotUsed1;
    ULONGLONG	bsTotalSectors;
    ULONGLONG	bsMFT;
    ULONGLONG	bsMFTMirror;
	ULONG		bsClusterPerFileRecord;
	ULONG		bsClusterPerIndexBlock;
    ULONGLONG    bsVolumeID;         // VolumeID
	ULONG		bsChecksum;
    CCHAR       bsReserved2[426];   // Reserved
    UCHAR       bsSig2[2];          // Originial Boot Signature - 0x55, 0xAA
}   NTFS_BOOT_SECTOR, *PNTFS_BOOT_SECTOR;

#pragma pack()

extern BOOL GetVolumePartitionInfo(HANDLE hVolume, PPARTITION_INFORMATION ppi);
extern BOOL GetVolumeDiskGeometry(HANDLE hVolume, DISK_GEOMETRY *geo);
extern BOOL CloseVolume(HANDLE hVolume);
extern BOOL LockVolume(HANDLE hVolume);
extern BOOL UnLockVolume(HANDLE hVolume);
extern BOOL DismountVolume(HANDLE hVolume);
extern HANDLE OpenAVolume(char *cDriveLetter, DWORD dwAccessFlags);
extern HANDLE OpenAVolumeMountPoint(char *szVolumeName, DWORD dwAccessFlags);
extern BOOL MountVolume(HANDLE hVolume);
extern BOOL CheckVolumeMounted(HANDLE hVolume);
extern HANDLE DismountAVolume(char *cDriveLetter);
extern HANDLE AttachAVolume(char *cDriveLetter);
extern BOOL DeleteAVolume(char *cDriveLetter);
extern BOOL DeleteAMountPointVolume(char *szMountPoint);
extern BOOL CreateAVolume(char *cDriveLetter, char * NTPath);
extern BOOL CreateAMountPointVolume(char *szVolMntPnt, char *szDeviceName);
extern HANDLE LockAVolume(char *cDriveLetter);
extern HANDLE LockAMountPointVolume(char *szMountPoint);
extern BOOL sync(char *cDriveLetter);

extern BOOLEAN GetNTFSInfo( char *cDriveLetter, PNTFS_VOLUME_DATA_BUFFER VolumeInfo, char *szError ); 
extern BOOL GetVolumeUsedClusters( HANDLE hVolume, PLIST_ENTRY pUsedBitmapList, 
                                    char *szError );
extern BOOL GetVolumeFreeClusters( HANDLE hVolume, PLIST_ENTRY pFreeBitmapList, 
                                    char *szError );
extern BOOL GetFileUsedClusters( int drive, char *argument, PLIST_ENTRY pUsedBitmapList, 
                                    char *szError );


extern long disksize(char *fn);

// 
// STEVE added for dynamic disk detection
//
extern unsigned int IsDiskBasic(char *szDrive);

//
// SVG 30-05-03
// +++
extern long fdisksize(char * szDir, HANDLE fd);
// ---
// volume2.c 
extern void getDiskSigAndInfo(char *szDir, char *szDiskInfo, int iGroupID);
extern BOOL getMntPtSigAndInfo(char *szDir, char *szDiskInfo, int iGroupID);
extern unsigned long getDeviceNameSymbolicLink( char *lpDeviceName, char *lpTargetPath, unsigned long ucchMax );
#ifdef __cplusplus 
}
#endif

#endif /* _WINDOWS */

#endif

