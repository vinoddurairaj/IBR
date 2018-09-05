#include <stdafx.h>
#include <Windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <malloc.h>
#include <lmerr.h>			// for GetErrorText()


#define IOCTL_STORAGE_GET_DEVICE_NUMBER				CTL_CODE(IOCTL_STORAGE_BASE, 0x0420, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VOLUME_QUERY_VOLUME_NUMBER			CTL_CODE(IOCTL_VOLUME_BASE, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)
//
// The following IOCTL is supported by mounted devices.
//

#define IOCTL_MOUNTDEV_QUERY_DEVICE_NAME			CTL_CODE(MOUNTDEVCONTROLTYPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define MOUNTDEVCONTROLTYPE  ((ULONG) 'M')
#define IOCTL_MOUNTDEV_QUERY_UNIQUE_ID              CTL_CODE(MOUNTDEVCONTROLTYPE, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME    CTL_CODE(MOUNTDEVCONTROLTYPE, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)


#define DEVICE_HARDDISK_VOLUME_STRING				"\\Device\\HarddiskVolume%d"
#define DEVICE_HARDDISK_PARTITION					L"\\Device\\Harddisk%d\\Partition%d"
#define MAXLEN										128;
#define FILENAME									"C:\\config.txt";
#define DRIVE_LETTER_STR							"\\\\.\\"
#define SIGNATURE_UNIQUE_ID							"volume(%08x-%I64x-%I64x)"
#define	PHYSICALDRIVE								"\\\\.\\PHYSICALDRIVE%x"
#define JUNK										L"Junk"

typedef struct _VOLUME_NUMBER {
    ULONG   VolumeNumber;
    WCHAR   VolumeManagerName[8];
} VOLUME_NUMBER, *PVOLUME_NUMBER;

typedef struct _MOUNTDEV_UNIQUE_ID {
    USHORT  UniqueIdLength;
    UCHAR   UniqueId[1];
} MOUNTDEV_UNIQUE_ID, *PMOUNTDEV_UNIQUE_ID;

typedef struct _MOUNTDEV_SUGGESTED_LINK_NAME {
    BOOLEAN UseOnlyIfThereAreNoOtherLinks;
    USHORT  NameLength;
    WCHAR   Name[1];
} MOUNTDEV_SUGGESTED_LINK_NAME, *PMOUNTDEV_SUGGESTED_LINK_NAME;

typedef struct _MOUNTDEV_NAME {
    USHORT  NameLength;
    WCHAR   Name[1];
} MOUNTDEV_NAME, *PMOUNTDEV_NAME;

typedef struct DEVICE_INFO
{
	// Disk number for reference in WMI
    ULONG					DiskNumber;

	// Physical Device name or WMI Instance Name
    //UNICODE_STRING			PhysicalDeviceName;
    //WCHAR					PhysicalDeviceNameBuffer[DISKPERF_MAXSTR];

	BOOLEAN					bValidName;	// TRUE means all follwing BOOLEAN values are TRUE

	//		IOCTL_STORAGE_GET_DEVICE_NUMBER to retrieve this info, 
	// -	if disk is PhysDisk type then IOCTL_STORAGE_GET_DEVICE_NUMBER succeeds and 
	//		we store name string \\Device\HardDisk(n)\\Partition(n) in DiskPartitionName
	// -	if disk is not PhyDisk then IOCTL_STORAGE_GET_DEVICE_NUMBER fails
	//		We use IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, to get device name string and that string 
	//		we store in DiskVolumeName.
	//		It also uses IOCTL_VOLUME_QUERY_VOLUME_NUMBER to get VOLUME_NUMBER information

    // Use to keep track of Volume info from ntddvol.h
    WCHAR					StorageManagerName[8];		// L"PhysDisk", L"LogiDisk" else the value from VOLUME_NUMBER
														// PhyDisk means "\\Device\\Harddisk%d\\Partition%d",
														// LogiDisk means "\\Device\HarDiskVolume%d

	// IOCTL_STORAGE_GET_DEVICE_NUMBER to retrieve this info, Example : \\Device\HardDisk(n)\\Partition(n)
	// if disk is PhysDisk type then we get following info successfully.
	BOOLEAN					bStorage_device_Number;		// TRUE means Storage_device_Number is valid
	STORAGE_DEVICE_NUMBER	StorageDeviceNumber;		// values retrieved from IOCTL_STORAGE_GET_DEVICE_NUMBER
	WCHAR					DiskPartitionName[128];		// Stores \\Device\HardDisk(n)\\Partition(n)

	// IOCTL_VOLUME_QUERY_VOLUME_NUMBER to retrieve HarddiskVolume number and its volumename like logdisk, etc..
	// if disk is LogiDisk type then we get following info successfully.
	BOOLEAN					bVolumeNumber;			// TRUE means VolumeNumber is valid
	VOLUME_NUMBER			VolumeNumber;			// IOCTL_VOLUME_QUERY_VOLUME_NUMBER
	
	// IOCTL_MOUNTDEV_QUERY_DEVICE_NAME used to retrieve \Device\HarddiskVolume1 into DiskVolumeName...
	BOOLEAN					bDiskVolumeName;		// TRUE means DiskVolumeName has valid value
	WCHAR					DiskVolumeName[128];	// IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returns string in DiskVolumeName	
													// Example : \Device\HarddiskVolume1 or \DosDevices\D or "\DosDevices\E:\FilesysD\mnt
	// following fields are supported only >= Win2k OS 
	// IOCTL_MOUNTDEV_QUERY_UNIQUE_ID used to retrieve Volume/Disk Unique Persistence ID, 
	// It Contains the unique volume ID. The format for unique volume names is "\??\Volume{GUID}\", 
	// where GUID is a globally unique identifier that identifies the volume.
	BOOLEAN							bUniqueVolumeId;	// TRUE means UniqueIdInfo Fields values are valid
	PMOUNTDEV_UNIQUE_ID				UniqueIdInfo;		// Example: For instance, drive letter "D" must be represented in this manner: "\DosDevices\D:". 
	
	// following fields are supported only >= Win2k OS 
	// IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME is Optional returns Drive letter (if Drive Letter is persistent across boot) or
	// suggest Drive Letter Dos Sym,bolic Name.
	BOOLEAN							bSuggestedDriveLetter;	// TRUE means All Following Fields values are valid
	PMOUNTDEV_SUGGESTED_LINK_NAME	SuggestedDriveLinkName;	// Example: For instance, drive letter "D" must be represented in this manner: "\DosDevices\D:". 

	// Store Customize Signature GUID which gets used as alternate Volume GUID only if bUniqueVolumeID is not valid
	// Format is "volume(nnnnnnnn-nnnnnnnnnnnnnnnn-nnnnnnnnnnnnnnnn)"	: volume(Disksignature-StartingOffset-SizeInBytes)
	// Our customize alternate Disk Signature based Unique ID for Volume (Raw Disk/ Disk Partition)
	BOOLEAN			bSignatureUniqueVolumeId;	// TRUE means SignatureUniqueIdLength and SignatureUniqueId has valid values
	USHORT			SignatureUniqueIdLength;
    UCHAR			SignatureUniqueId[128];			// 128 is enough, if requires bump up this value.

	BOOLEAN			IsVolumeFtVolume; // TRUE means StorageManagerName == FTDISK && NumberOfDiskExtents > 0
	//PDEVICE_OBJECT	pRawDiskDevice;	// Pointer to RAW Disk device of current partition or disk object

	DWORD			Signature;			// signature
	LARGE_INTEGER   StartingOffset;		// Starting Offset of Partition, if its RAW Disk than value is 0 
    LARGE_INTEGER   PartitionLength;	// Size of partition, if its RAW Disk than value is 0

} DEVICE_INFO, *PDEVICE_INFO;

int InitializeHandle(char DriveLetter);
DEVICE_INFO SymbolicLinkInfo( char DriveLetter /* char* SymLink1, char* SymLink2, PUCHAR SymLink3*/ );
int	sftk_GenerateSignatureGuid( PDEVICE_INFO	devinfo, HANDLE handle );
int sftk_GetVolumeDiskExtents( PDEVICE_INFO	devinfo, HANDLE handle);
VOID DisplayErrorText();

