/**************************************************************************************

Module Name: sftk_ps.h   
Author Name: Parag sanghvi
Description: Define all Pstore File layout structures and APIS 
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#ifndef _SFTK_PS_H_
#define _SFTK_PS_H_

#pragma pack(push,1)

/*********************************************************************************************
Pstore File format :

Offset	SizeInFile		Struct_Size		Total_Used_fields_size		Struct 	
 0		512				49				49							SFTK_PS_HDR (included SFTK_PS_LG)

 512	1024			871				871							SFTK_PS_DEV strcuture for device 1
 1536	LRDB size		LRDB size		LRDB size (8K)				Lrdb Bitmap Raw Memory
 9728	HRDB size		HRDB size		HRDB size (128K)			Hrdb Bitmap Raw Memory
	
 140800	 1024			867				867							SFTK_PS_DEV strcuture for device 2 
 .....
 .....
 .....
In other words, following is Pstore file layout: max_dev and max_group is always 1024 !!!

  		SFTK_PS_HDR	(This also includes SFTK_PS_LG)

		SFTK_PS_DEV	(Fixed size of Device info Structure for device 1 )
		SFTK_BITMAP_REGION_HDR for LRDB (Fixed size 512 bytes)
		LRDB Bitmap	(Variable size Raw memory stored LRDB Bitmaps of device 1 SFTK_PS_DEV)
		SFTK_BITMAP_REGION_HDR for HRDB (Fixed size 512 bytes)
		HRDB Bitmap	(Variable size Raw memory stored HRDB Bitmaps of device 1 SFTK_PS_DEV)

		SFTK_PS_DEV	(Fixed size of Device info Structure for device 2)
		SFTK_BITMAP_REGION_HDR for LRDB (Fixed size 512 bytes)
		LRDB Bitmap	(Variable size Raw memory stored LRDB Bitmaps of device 2 SFTK_PS_DEV)
		SFTK_BITMAP_REGION_HDR for HRDB (Fixed size 512 bytes)
		HRDB Bitmap	(Variable size Raw memory stored HRDB Bitmaps of device 2 SFTK_PS_DEV)

		..........
		..........
		..........

		SFTK_PS_DEV	(Fixed size of Device info Structure for device n)
		LRDB Bitmap	(Variable size Raw memory stored LRDB Bitmaps of device n SFTK_PS_DEV)
		HRDB Bitmap	(Variable size Raw memory stored LRDB Bitmaps of device n SFTK_PS_DEV)

  Minimum size of Pstore file is : sizeof(SFTK_PS_HDR) 

*********************************************************************************************/

// Predeclaration of external defined structure 

// macro definations used specifically for Pstore and in its APIs
#define SFTK_PS_VERSION_1_MAGIC			0xBADF00D1		// keeping same as old Magic num...
#define SFTK_PS_MAJOR_VER				0x1
#define SFTK_PS_MINOR_VER				0x0
#define SFTK_PS_EXTRA_VER				0x0

#define SFTK_PS_HDR_BSIZE				1024			// 1024 bytes

#define OFFSET_SFTK_PS_HDR												0						// starting offset in bytes
#define OFFSET_FIRST_SFTK_PS_DEV(blockaligned_psHdrsize)				blockaligned_psHdrsize	// starting offset in bytes
#define OFFSET_SFTK_LRDB(ps_dev_offset, blockaligned_psDevsize)			(ps_dev_offset + blockaligned_psDevsize) // starting offset in bytes
#define OFFSET_SFTK_HRDB(lrdb_offset, lrdbsize)							(lrdb_offset + lrdbsize)				// starting offset in bytes
#define TOTAL_PS_DEV_REGION_SIZE(psDevSize, lrdbSize, hrdbSize)			(psDevSize + lrdbSize + hrdbSize)				// Total PS_Dev Region size
#define OFFSET_NEXT_SFTK_PS_DEV(ps_dev_Soffset, total_psDevRegionSize)	(ps_dev_Soffset + total_psDevRegionSize)	// Next Ps dev Region


// #define ALIGN(X) ((X) & 0xFFFFFFF8) // round down to next 4k boundary
// #define BOUND(X) ALIGN((X)+BLKS-1)	// round up to next 4k boundary

#define	DEV_BSHIFT		9		// 2^9 = 512
#define SECTOR_SIZE		512		// 0x200

// #define	BLOCK_ALIGN(a)  ( ((a >> DEV_BSHIFT) + 1) << DEV_BSHIFT )
// #define PAGE_SIZE 0x1000
// #define PAGE_ALIGN(Va) ((PVOID)((ULONG_PTR)(Va) & ~(PAGE_SIZE - 1)))
// #define SECTOR_ALIGN(size) ((PVOID)((ULONG_PTR)(size) & ~(SECTOR_SIZE - 1)))

//
// BLOCK_ALIGN_UP(size) macro will return value which SECTOR ALIGN upwards
// Example: BLOCK_ALIGN_UP(520) will get returned 1024 bytes
//			BLOCK_ALIGN_UP(512) will get returned 512 bytes
//
#define BLOCK_ALIGN_UP(size) ( (size % SECTOR_SIZE)?size + (SECTOR_SIZE - (size % SECTOR_SIZE)):size )


// structure definations
typedef struct SFTK_PS_LG
{
	ULONG	LGroupNumber;		// Logical group number supplied from Service
	LONG    PrevState;				// Saving Prev state, need it for protocol...SFTK_MODE_FULL_REFRESH, etc... defined in ftdio.h file
	LONG	state;				// SFTK_MODE_NORMAL, etc... defined in ftdio.h file
	BOOLEAN	bInconsistantData;		// At new LG create, this becomes TRUE, it remains FALSE till first time Smart Refresh gets started.
												// create time LG, State mode default  get sets to Tracking mode.
	ULONG	UserChangedToTrackingMode;	// TRUE if user IOCTL has changed the state to tracking mode.
													// in this case Driver does not change tracking mode to smart refresh mode
													// till user sends other atte change IOCTL.
	ULONG	TotalNumDevices;	// Number of Logical Group is opened (by service means in used..)...

	ULONG	statsize;			// statesize 
	ULONG	sync_depth;			// used to delay src disk I/O if sedondary Comit Write gets slow down
    ULONG	sync_timeout;		// used to hold incoming IO Completion Irp fot specified seconds delay, 
    ULONG	iodelay;			// incoming IO goes to disk, sync_timout was used to delay Completion of 
								// IRP which is completed already but don't pass back to caller.
	SFTK_ROLE		Role;
	SFTK_SECONDARY	Secondary;

	// TODO : Add other information requires during Boottime
	// TODO : LG based Cache Manager info, Socket Information, etc...

	// CHAR	*statbuf;	// Not Used, just copy buffer from and to service using IOCTL.....
    // LONG    statsize;	

} SFTK_PS_LG, *PSFTK_PS_LG;	// size = 7 * 4 = 28 bytes

typedef struct SFTK_PS_HDR
{
	ULONG		MagicNum;			// = SFTK_PS_VERSION_1_MAGIC = 0xBADF00D1
	ULONG		MajorVer;			// Store Major version of Pstore file
	ULONG		MinorVer;			// Store Minor version of Pstore file
	ULONG		ExtraVer;			// Store extra version of Pstore file if requires
	ULONG		SizeInbAllign;		// Total size of cuurent struct (This is Block alligned size stored on File)

	BOOLEAN		ValidLastShutdown;	// TRUE means Last Shutdown, Pstore file is Valid.
	ULONGLONG	Checksum;			// Current structure's checksum, All fileds used to calc checksum

	SFTK_PS_LG	LgInfo;						// size = 19 * 4 = 76 bytes 

	ULONG		LastOffsetOfPstoreFile;	// This Pstore file offset where new Device gets added
    
} SFTK_PS_HDR, *PSFTK_PS_HDR;	// size = 8*4 + 1 + 28 = 61 bytes

// Device information along with Bitmap
typedef struct SFTK_PS_BITMAP
{
	BOOLEAN		ValidBitmap;			// TRUE means Bitmap is valid for use, this is used during BootTime 
	ULONGLONG	Checksum;				// Checksum Of bitmap memory on file......TODO : Do we need this ???
	
	ULONG		Sectors_per_bit;		// 1 bit = Sectors_per_bit number of sectors, Value * 512 = Chunksize in bytes
	ULONG		TotalNumOfBits;			// Total number of bits used for bitmap
	ULONG		BitmapSize;				// total Size of Buffer used for bitmap.IN BYTES
	ULONG		BitmapSizeBlockAlign;	// size of Bitmap buffer Sector alligned stored on Pstore File

	ULONG		len32;					// For BackWard Compatibility... to build ftd_dev_info_t  during boot time...

} SFTK_PS_BITMAP, *PSFTK_PS_BITMAP;	// size = 7 * 4 + 1 = 29 bytes

typedef struct SFTK_PS_DEV
{
	ULONG			SizeInbAllign;		// Total size of cuurent struct (This is Block alligned size stored on File)
	ULONGLONG		Checksum;			// Current structure's checksum, All fileds used to calc checksum
	BOOLEAN			Deleted;			// if TRUE means ignore this Dev in LG of pstore file since current Dev is deleted
 
	ULONG           cdev;           
    ULONG           bdev;           
    ULONG           localbdisk;		// same as bdev
    ULONG           localcdisk;		// same as cdev	

	CHAR			Devname[64];      // User supplied information, from ftd_dev_info_t
    CHAR			Vdevname[64];     // User supplied information	from ftd_dev_info_t
	// OS supported Unique ID for Volume (Raw Disk/ Disk Partition)
	BOOLEAN			bUniqueVolumeIdValid;	// TRUE means UniqueIdLength and UniqueId has valid values
	USHORT			UniqueIdLength;
    UCHAR			UniqueId[256];			// 256 is enough, if requires bump up this value.

	// Our customize alternate Disk Signature based Unique ID for Volume (Raw Disk/ Disk Partition)
	BOOLEAN			bSignatureUniqueVolumeIdValid;	// TRUE means SignatureUniqueIdLength and SignatureUniqueId has valid values
	USHORT			SignatureUniqueIdLength;
    UCHAR			SignatureUniqueId[256];			// 256 is enough, if requires bump up this value.

	// Optional - If Device is formatted disk Partition, than associated drive letter symbolik link name info 
	BOOLEAN			bSuggestedDriveLetterLinkValid;	// TRUE means SuggestedDriveLetterLinkLength and SuggestedDriveLetterLink has valid values
	USHORT			SuggestedDriveLetterLinkLength;
    UCHAR			SuggestedDriveLetterLink[128];	// 256 is enough, if requires bump up this value.

	LONG			Flags;				// SFTK_DEV->Flags values goes here,  PNP Removal = // SFTK_DEV_FLAG_SRC_DEVICE_ONLINE, etc..
	ULONG			Disksize;			// Total disk blocks (sectors) in this device (Values in Block)
	ULONG			statesize;
	ULONG			RefreshLastBitIndex;	// this HRDB based bit index value same as SFTK_DEV

	SFTK_PS_BITMAP	Lrdb;			// Lrdb Bitmap storage information
	SFTK_PS_BITMAP	Hrdb;			// Lrdb Bitmap storage information

	ULONG			LrdbOffset;		// Actual starting Offset in bytes (relative to 0 ) for LRDB memory
	ULONG			HrdbOffset;		// Actual starting Offset in bytes (relative to 0) for HRDB memory

	ULONG			TotalSizeOfDevRegion;	// Total size Current Dev region includes 
											// = BLOCK_ALLIGN(SFTK_PS_DEV + LRDB Bitmap + HRDB Bitmap) = 1024 bytes

} SFTK_PS_DEV, *PSFTK_PS_DEV;	// 12*4 + 3 + 3*2 + 64 + 64 + 256 + 256 + 128 + 29 + 29 = 883 bytes

#define HRDB_BITMAP_TIMESTAMP(LrdbBitmapTimeStamp)		((LrdbBitmapTimeStamp) += 1)
#define SFTK_PS_BITMAP_REGION_MAGIC						0xFADF11D1		

// TRUE means valid else FALSE
#define IS_PS_HRDB_BITMAP_VALID(HrdbHdr, LrdbHdr)		((HrdbHdr)->data.TimeStamp.QuadPart >= (LrdbHdr)->data.TimeStamp.QuadPart)


//
// This is Hdr of Bitmap Raw Memory, this is always 512 block size.
// This is used specially to confirm validation of HRDB bitmap 
//
typedef struct SFTK_PS_BITMAP_REGION_HDR 
{
	ULONG	MagicNum;
    union {
        LARGE_INTEGER	TimeStamp;	// TimeStamp Following Bitmap Got Flush
        CHAR dummy[SECTOR_SIZE - sizeof(ULONG)];
    } data;

} SFTK_PS_BITMAP_REGION_HDR, *PSFTK_PS_BITMAP_REGION_HDR;

#pragma pack(pop)

#endif // _SFTK_PS_H_