//====================================================================
//
// Diskkey.c
//
//====================================================================

#pragma pack(1)

//----------------------------------------------------------------------
// 
// Enumerated types
//
//----------------------------------------------------------------------

//
// Partition types
//
typedef enum {
	Mirror,
	Stripe,
	ParityStripe,
	VolumeSet,
	NonFT,
	WholeDisk
} PARTITION_TYPE;


//
// Fault-tolerant partition states
//
typedef enum {
	Healthy,
	Orphaned,
	Regenerating,
	Initializing,
	SyncWithCopy
} FT_PARTITION_STATE; 

//
// Fault-tolerant volume states - from ddk\inc\ntddft.h
//
typedef enum _FT_STATE {

    FtStateOk,      // set is complete
    FtHasOrphan,    // set is running, but one member is turned off
    FtDisabled,     // set has been turned off - it cannot be used
    FtRegenerating, // member of set is being generated from check data
    FtInitializing, // set is being initialized
    FtCheckParity,  // check data is not in sync
    FtNoCheckData   // parity has not been initialized

} FT_STATE, *PFT_STATE;


//----------------------------------------------------------------------
//
// Non-Fault tolerant disk information
//
//----------------------------------------------------------------------

//
// Non-FT disk partition information 
//
typedef struct {
	
	//
	// Is the partition a member of an FT group?
	//

	PARTITION_TYPE		FtType;

	//
	// State if member of an FT group
	//

	FT_PARTITION_STATE	FtState;

	//
	// Starting offset and length of partition on the disk
	//

	LARGE_INTEGER    	StartingOffset;
	LARGE_INTEGER    	Length;

	//
	// Length of the FT-used portion of the partition. This always appears
	// to be 0, regardless of the length.
	//

	LARGE_INTEGER    	FtLength;
	ULONG	   			Unused[2];		// for alignment

	//
	// Drive letter if user has assigned one
	//

	CHAR    			DriveLetter;

	//
	// 1 if the user has assigned a drive letter
	//

	CHAR    			AssignDriveLetter;

	//
	// Partition number
	//

	USHORT    			LogicalNumber;

	//
	// Which FT-group the partition belongs to
	//

	USHORT    			FtGroup;

	//
	// Which member in the group 
	//

	USHORT    			FtMember;

	//
	// Unknown
	// 

	UCHAR    			Unknown;
	UCHAR    			Unused1[3]; 
} DISK_PARTITION, *PDISK_PARTITION;

//
// Header that preceeds partition information
//
typedef struct {

	//
	// Signature Disk Administrator has written
	// to the disk
	//

	ULONG				Signature;

	//
	// Start of an array of DISK_PARTITION
	//

	PVOID				Partitions; 
} PARTITION_HEADER, *PPARTITION_HEADER;

//
// Information for a disk
//
typedef struct {
	USHORT    			NumberOfPartitions;
	USHORT    			ReservedShort;
	PVOID				PartitionHeader;	 // PARTITION_HEADER
} DISK_DESCRIPTION, *PDISK_DESCRIPTION;


//
// Non-FT disk information header
//
typedef struct {
	USHORT    			NumberOfDisks;
	USHORT    			ReservedShort;
	DISK_DESCRIPTION    Disks[1];
} DISK_REGISTRY, *PDISK_REGISTRY;


//----------------------------------------------------------------------
//
// Fault-tolerant information
//
//----------------------------------------------------------------------

//
// Fault-tolerant group member definition - from ddk\inc\ntddft.h
//
typedef struct _FT_MEMBER_DESCRIPTION {

    USHORT        State;
    USHORT        ReservedShort;
    ULONG         Signature;
    ULONG         OffsetToPartitionInfo;   // offset to the partition's 
										   // info in the disk info buffer
    ULONG         LogicalNumber;           // identifies disk+partition of member

} FT_MEMBER_DESCRIPTION, *PFT_MEMBER_DESCRIPTION;


//
// Fault-tolerant group definition - from ddk\inc\ntddft.h
//
typedef struct _FT_DESCRIPTION {

    USHORT			NumberOfMembers;
    USHORT			Type;
    ULONG			Reserved;

    //
    // The volume state will indicate which member is being regenerated
    //

    FT_STATE		FtVolumeState;

    //
    // Array for NumberOfMembers
    //

    FT_MEMBER_DESCRIPTION FtMemberDescription[1];

} FT_DESCRIPTION, *PFT_DESCRIPTION;


//
// Fault-tolerant information header - from ddk\inc\ntddft.h
//
typedef struct _FT_REGISTRY {

    USHORT NumberOfComponents;
    USHORT ReservedShort;

    //
    // An array of NumberOfComponents
    //

    FT_DESCRIPTION FtDescription[1];

} FT_REGISTRY, *PFT_REGISTRY;



//----------------------------------------------------------------------
//
// Header for the DISK information value - from ddk\inc\ntddft.h
// 
//----------------------------------------------------------------------
typedef struct _DISK_CONFIG_HEADER {

    ULONG Version;
    ULONG CheckSum;
    BOOLEAN DirtyShutdown;
    UCHAR Reserved[3];
    ULONG DiskInformationOffset;
    ULONG DiskInformationSize;
    ULONG FtInformationOffset;
    ULONG FtInformationSize;
    ULONG FtStripeWidth;
    ULONG FtPoolSize;
    ULONG NameOffset;  // Reserved for future.
    ULONG NameSize;    // Reserved for future.

} DISK_CONFIG_HEADER, *PDISK_CONFIG_HEADER;


#pragma pack()
