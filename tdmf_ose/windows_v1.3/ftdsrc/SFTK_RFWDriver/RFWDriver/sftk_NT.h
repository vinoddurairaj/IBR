/**************************************************************************************

Module Name: sftk_NT.h   
Author Name: Parag sanghvi
Description: Define all Windows OS based definations
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#ifndef _SFTK_NT_H_
#define _SFTK_NT_H_

#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
// SKbl - SofteK Block
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'lbKS')	
#endif

// 
// TODO FIXME FIXME : For Windows NT Driver, use this macros, defined this compile time... 
// #define NTFOUR		1
//

// 
// Windows driver extra code used following macro definations...
//

// To enable WMI support, define this macro..., 
// TODO : Please do not define this macro, WMI code is not working and required to test and implement this properly
// TODO: only the wraper unfinished code is done for WMI, Its disable 
// #define WMI_IMPLEMENTED	

#define DEVICE_HARDDISK_VOLUME_STRING	"\\Device\\HarddiskVolume%d"
#define DEVICE_HARDDISK_PARTITION		L"\\Device\\Harddisk%d\\Partition%d"

typedef struct DEVICE_INFO
{
	// Disk number for reference in WMI
    ULONG					DiskNumber;

	// Physical Device name or WMI Instance Name
    UNICODE_STRING			PhysicalDeviceName;
    WCHAR					PhysicalDeviceNameBuffer[DISKPERF_MAXSTR];

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
	PDEVICE_OBJECT	pRawDiskDevice;	// Pointer to RAW Disk device of current partition or disk object

	ULONG			Signature;			// signature
	LARGE_INTEGER   StartingOffset;		// Starting Offset of Partition, if its RAW Disk than value is 0 
    LARGE_INTEGER   PartitionLength;	// Size of partition, if its RAW Disk than value is 0

} DEVICE_INFO, *PDEVICE_INFO;

#ifdef _TDIDRIVER

typedef
VOID
(*PDEVICE_UNLOAD) (
    IN struct _DEVICE_OBJECT *DeviceObject
    );

#endif	//_TDIDRIVER

//
// Device Extension
//
typedef struct _DEVICE_EXTENSION 
{
	// Always must be first field
	SFTK_NODE_ID		NodeId;	// NODE_TYPE_FILTER_DEV type node

    // Back pointer to device object
    PDEVICE_OBJECT		DeviceObject;

    // Target Device Object, Belowe Level Device Object, use this for IoCallDriver()..
    PDEVICE_OBJECT		TargetDeviceObject;

    // Physical device object
    PDEVICE_OBJECT		PhysicalDeviceObject;

	// This link list is linked to Anchor SFTK_CONFIG->DevExt_List
	LIST_ENTRY			DevExt_Link;

	// Pointer to Configured SFTK_DEV device, if this device is used for replications
	struct SFTK_DEV 	*Sftk_dev;	

	// Stores all unique information for current device.
	DEVICE_INFO			DeviceInfo;	
	
	// must synchronize paging path notifications
    KEVENT				PagingPathCountEvent;
    ULONG				PagingPathCount;

#ifdef WMI_IMPLEMENTED	
    // Private context for using WmiLib
    WMILIB_CONTEXT WmilibContext;
#endif

#ifdef _TDIDRIVER

   PDEVICE_OBJECT       pDeviceObject;
   BOOLEAN				bSymbolicLinkCreated;
   SESSION_MANAGER		SessionManager;
   SM_INIT_PARAMS		SMInitParams;
   PDRIVER_DISPATCH     MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
   PDEVICE_UNLOAD       DeviceUnload;

#endif //_TDIDRIVER


} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#define DEVICE_EXTENSION_SIZE sizeof(DEVICE_EXTENSION)

#define PROCESSOR_COUNTERS_SIZE FIELD_OFFSET(DISK_PERFORMANCE, QueryTime)

#define FILTER_DEVICE_PROPOGATE_FLAGS            0
#define FILTER_DEVICE_PROPOGATE_CHARACTERISTICS (FILE_REMOVABLE_MEDIA |  \
                                                 FILE_READ_ONLY_DEVICE | \
                                                 FILE_FLOPPY_DISKETTE    \
                                                 )

#define USE_PERF_CTR

#ifdef USE_PERF_CTR
#define SwrGetClock(a, b) (a) = KeQueryPerformanceCounter((b))
#else
#define SwrGetClock(a, b) KeQuerySystemTime(&(a))
#endif

#if DBG

#define SFTK_BreakPoint() DbgBreakPoint()

extern ULONG SwrDebug;

VOID
SwrDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

#define OS_ASSERT( exp ) \
    if (!(exp)) \
        RtlAssert( #exp, __FILE__, __LINE__, NULL )

#define OS_ASSERTMSG( msg, exp ) \
    if (!(exp)) \
        RtlAssert( #exp, __FILE__, __LINE__, msg )

#define DebugPrint(x)   SwrDebugPrint x

/*
#define DebugPrint(DebugPrintLevel, x)   if (SwrDebug & DebugPrintLevel)		
												OS_DbgPrint x
*/

// #define OS_ASSERT(x)	assert x

#define DBG_ERROR			0x00000001
#define DBG_DISPATCH		0x00000002
#define DBG_IOCTL_DISPLAY	0x00000002
#define DBG_COMPLETION		0x00000004
#define DBG_THREAD			0x00000008

#define DBG_OS_APIS			0x00000010
#define DBG_INIT			0x00000020
#define DBG_RDWR			0x00000030

#define DBG_IOCTL			0x00000100
#define DBG_CONFIG			0x00000200
#define DBG_STATE			0x00000400

#if 1 //Veera
#define DBG_SEND			0x00001000
#define DBG_CONNECT			0x00002000
#define DBG_LISTEN			0x00004000
#define DBG_COM				0x00008000
#define DBG_TDI_QUERY		0x00010000
#define DBG_TDI_INIT		0x00020000
#define DBG_TDI_UNINIT		0x00040000
#define DBG_PROTO			0x00080000
#define DBG_RECV			0x00100000
#endif

#define DBG_QUEUE			0x01000000
#define DBG_RTHREAD			0x02000000
#define DBG_MM				0x04000000

#else // Else Release version
#define OS_ASSERT( _exp_ )		{}
#define OS_ASSERTMSG( msg, exp )	{}

#define DebugPrint(x)			{}
#define SFTK_BreakPoint()		{}
#endif

// Windows 2K and NT based API Prototypes

NTSTATUS
SwrSendToNextDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#ifdef WMI_IMPLEMENTED	
NTSTATUS SwrWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SwrQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

NTSTATUS
SwrQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

NTSTATUS
SwrWmiFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    );

#endif // WMI_IMPLEMENTED	

#endif // _SFTK_NT_H_