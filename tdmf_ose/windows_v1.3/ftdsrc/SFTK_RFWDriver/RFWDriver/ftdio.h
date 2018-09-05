/*
 * ftdio.h - FTD user/kernel interface
 * 
 * Copyright (c) 2000 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#ifndef FTDIO_H
#define FTDIO_H

#include <sys/types.h>

#pragma pack(push,1)

#define FTD_DRIVER_ERROR_CODE   0xE0000000L
#define FTD_IOCTL_INDEX         0x0800

#ifdef _IOR
#undef _IOR
#endif

#ifdef _IOW
#undef _IOW
#endif

#ifdef _IO
#undef _IO
#endif

#define _IOR(cmd, index, data) CTL_CODE( FILE_DEVICE_UNKNOWN, FTD_IOCTL_INDEX + index,  \
        METHOD_BUFFERED, FILE_READ_ACCESS )
#define _IOW(cmd, index, data)CTL_CODE( FILE_DEVICE_UNKNOWN, FTD_IOCTL_INDEX + index,   \
        METHOD_BUFFERED, FILE_WRITE_ACCESS )
#define _IOWR(cmd, index, data)CTL_CODE( FILE_DEVICE_UNKNOWN, FTD_IOCTL_INDEX + index,  \
        METHOD_BUFFERED, FILE_ANY_ACCESS )
#define _IO(cmd, index)CTL_CODE( FILE_DEVICE_UNKNOWN, FTD_IOCTL_INDEX + index,  \
        METHOD_BUFFERED, FILE_ANY_ACCESS )

#define MAXMIN 0xffffff

#define DATASTAR_MAJIC_NUM  0x0123456789abcdefUL

#define FTD_OFFSET       16

typedef int ftd_unknown_t;      /* FIXME XXX */

#define FTD_CMD 'q'

#define FTD_GET_CONFIG           _IOR(FTD_CMD,  0x1, ftd_unknown_t)
#define FTD_NEW_DEVICE           _IOW(FTD_CMD,  0x2, ftd_dev_info_t)
#define FTD_NEW_LG               _IOW(FTD_CMD,  0x3, ftd_lg_info_t)
#define FTD_DEL_DEVICE           _IOW(FTD_CMD,  0x4, dev_t)
#define FTD_DEL_LG               _IOW(FTD_CMD,  0x5, ftd_state_t)
#define FTD_CTL_CONFIG           _IOW(FTD_CMD,  0x6, ftd_unknown_t)
#define FTD_GET_DEV_STATE_BUFFER _IOWR(FTD_CMD, 0x7, stat_buffer_t)
#define FTD_GET_LG_STATE_BUFFER  _IOWR(FTD_CMD, 0x8, stat_buffer_t)
#define FTD_SEND_LG_MESSAGE      _IOW(FTD_CMD,  0x9, stat_buffer_t)
#define FTD_OLDEST_ENTRIES       _IOWR(FTD_CMD, 0xa, oldest_entries_t)
#define FTD_MIGRATE              _IOW(FTD_CMD,  0xb, migrated_entries_t)
#define FTD_GET_LRDBS            _IOWR(FTD_CMD, 0xc, dirtybit_array_t)
#define FTD_GET_HRDBS            _IOWR(FTD_CMD, 0xd, dirtybit_array_t)
#define FTD_SET_LRDBS            _IOWR(FTD_CMD, 0xe, dirtybit_array_t)
#define FTD_SET_HRDBS            _IOWR(FTD_CMD, 0xf, dirtybit_array_t)
#define FTD_GET_LRDB_INFO        _IOWR(FTD_CMD, 0x11, dirtybit_array_t)
#define FTD_GET_HRDB_INFO        _IOWR(FTD_CMD, 0x12, dirtybit_array_t)
#define FTD_GET_DEVICE_NUMS      _IOWR(FTD_CMD, 0x13, ftd_devnum_t)
#define FTD_SET_DEV_STATE_BUFFER _IOWR(FTD_CMD, 0x14, stat_buffer_t)
#define FTD_SET_LG_STATE_BUFFER  _IOWR(FTD_CMD, 0x15, stat_buffer_t)
#define FTD_START_LG             _IOW(FTD_CMD,  0x16, dev_t)
#define FTD_GET_NUM_DEVICES      _IOWR(FTD_CMD, 0x17, ftd_state_t)
#define FTD_GET_NUM_GROUPS       _IOWR(FTD_CMD, 0x18, int)
#define FTD_GET_DEVICES_INFO     _IOW(FTD_CMD,  0x19, stat_buffer_t)
#define FTD_GET_GROUPS_INFO      _IOW(FTD_CMD,  0x1a, stat_buffer_t)
#define FTD_GET_DEVICE_STATS     _IOW(FTD_CMD,  0x1b, stat_buffer_t)
#define FTD_GET_GROUP_STATS      _IOW(FTD_CMD,  0x1c, stat_buffer_t)
#define FTD_SET_GROUP_STATE      _IOWR(FTD_CMD, 0x1d, ftd_state_t)
#define FTD_UPDATE_DIRTYBITS     _IO(FTD_CMD,   0x1e)
#define FTD_CLEAR_BAB            _IOWR(FTD_CMD, 0x1f, dev_t)
#define FTD_CLEAR_HRDBS          _IOWR(FTD_CMD, 0x20, dev_t)
#define FTD_CLEAR_LRDBS          _IOWR(FTD_CMD, 0x21, dev_t)
#define FTD_GET_GROUP_STATE      _IOWR(FTD_CMD, 0x22, ftd_state_t)
#define FTD_GET_BAB_SIZE         _IOWR(FTD_CMD, 0x23, int)
#define FTD_UPDATE_LRDBS         _IOW(FTD_CMD,  0x24, dev_t)
#define FTD_UPDATE_HRDBS         _IOW(FTD_CMD,  0x25, dev_t)
#define FTD_SET_SYNC_DEPTH       _IOW(FTD_CMD,  0x26, ftd_param_t)
#define FTD_SET_IODELAY          _IOW(FTD_CMD,  0x27, ftd_param_t)
#define FTD_SET_SYNC_TIMEOUT     _IOW(FTD_CMD,  0x28, ftd_param_t)
#define FTD_SET_TRACE_LEVEL      _IOW(FTD_CMD,  0x29, int)
#define FTD_INIT_STOP            _IOW(FTD_CMD,  0x30, dev_t)

#if 1 // PARAG : Added New IOCTLs for new driver 
#define FTD_GET_PERF_INFO		_IOWR(FTD_CMD,  0x31, dev_t)	// array of PERF_INFO

// FTD_CONFIG_BEGIN:
// At Service starting time, Tis IOCTL is the first IOCTL gets send to Driver
// This IOCTL mainly is used to update or synch Pstore file based configuration with
// service Config file at system start.
// This IOCTL must followed with list of above mentioned IOCTLS to confirm LG/Devs etc.
// This IOCTL must also end with FTD_CONFIG_END
#define FTD_CONFIG_BEGIN				_IOWR(FTD_CMD,  0x32, dev_t)	// array of PERF_INFO
#define FTD_CONFIG_END					_IOWR(FTD_CMD,  0x33, dev_t)	// array of PERF_INFO

#define IOCTL_GET_MM_START				_IOWR(FTD_CMD,  0x40, dev_t)	// MM IOCTL
#define IOCTL_SET_MM_DATABASE_MEMORY	_IOWR(FTD_CMD,  0x41, dev_t)	// MM IOCTL
#define IOCTL_MM_INIT_RAW_MEMORY		_IOWR(FTD_CMD,  0x42, dev_t)	// MM IOCTL
#define IOCTL_MM_STOP					_IOWR(FTD_CMD,  0x43, dev_t)	// MM IOCTL
#define IOCTL_MM_CMD					_IOWR(FTD_CMD,  0x44, dev_t)	// MM IOCTL

#define IOCTL_MM_ALLOC_RAW_MEM			_IOWR(FTD_CMD,  0x45, dev_t)	// MM IOCTL
#define IOCTL_MM_FREE_RAW_MEM			_IOWR(FTD_CMD,  0x46, dev_t)	// MM IOCTL

#endif

//The IOCTLS are used to ( Add | Remove | Enable | Disable | Query | Set ) Connection information
#define SFTK_IOCTL_TCP_ADD_CONNECTIONS				_IOWR(FTD_CMD, 0x47, CONNECTION_DETAILS)
#define SFTK_IOCTL_TCP_REMOVE_CONNECTIONS			_IOWR(FTD_CMD, 0x48, CONNECTION_DETAILS)
#define SFTK_IOCTL_TCP_ENABLE_CONNECTIONS			_IOWR(FTD_CMD, 0x49, CONNECTION_DETAILS)
#define SFTK_IOCTL_TCP_DISABLE_CONNECTIONS			_IOWR(FTD_CMD, 0x4A, CONNECTION_DETAILS)
#define SFTK_IOCTL_TCP_QUERY_SM_PERFORMANCE			_IOWR(FTD_CMD, 0x4B, SM_PERFORMANCE_INFO)
#define SFTK_IOCTL_TCP_SET_CONNECTIONS_TUNABLES		_IOWR(FTD_CMD, 0x4C, CONNECTION_TUNABLES)

#define SFTK_IOCTL_START_PMD						_IOWR(FTD_CMD, 0x4D, SM_INIT_PARAMS)
#define SFTK_IOCTL_STOP_PMD							_IO(FTD_CMD, 0x4E)

#define SFTK_IOCTL_START_RMD						_IOWR(FTD_CMD, 0x4F, SM_INIT_PARAMS)
#define SFTK_IOCTL_STOP_RMD							_IO(FTD_CMD, 0x50)

#define SFTK_IOCTL_INIT_PMD							_IOWR(FTD_CMD, 0x51 , SM_INIT_PARAMS)
#define SFTK_IOCTL_INIT_RMD							_IOWR(FTD_CMD, 0x52 , SM_INIT_PARAMS)
#define SFTK_IOCTL_UNINIT_PMD						_IO(FTD_CMD, 0x53)
#define SFTK_IOCTL_UNINIT_RMD						_IO(FTD_CMD, 0x54)

#define SFTK_IOCTL_TCP_SEND_NORMAL_DATA				_IO(FTD_CMD, 0x55)
#define SFTK_IOCTL_TCP_SEND_REFRESH_DATA			_IO(FTD_CMD, 0x56)

#if 1 // PARAG : Added New IOCTLs for new driver 
#define FTD_GET_ALL_STATS_INFO		_IOWR(FTD_CMD,  0x55, dev_t)	// variable size of struct ALL_LG_STATISTICS
#define FTD_GET_LG_STATS_INFO		_IOWR(FTD_CMD,  0x56, dev_t)	// variable size of struct LG_STATISTICS

#define FTD_GET_LG_TUNING_PARAM		_IOWR(FTD_CMD,  0x57, dev_t)	// Fixed size of struct LG_PARAM
#define FTD_SET_LG_TUNING_PARAM		_IOWR(FTD_CMD,  0x58, dev_t)	// Fixed size of struct LG_PARAM

#define FTD_GET_All_ATTACHED_DISKINFO	_IOWR(FTD_CMD,  0x59, dev_t)	// Variable size of struct PATTACHED_DISK_INFO_LIST

// Following Ioctl is only for testign purpose....
#define FTD_RECONFIGURE_FROM_REGISTRY	_IOWR(FTD_CMD,  0x60, dev_t)	// Variable size of struct PATTACHED_DISK_INFO_LIST

#if TARGET_SIDE
#define FTD_IOCTL_FAILOVER					_IOWR(FTD_CMD,  0x61, dev_t)	// Fixed size of struct PFAIL_OVER
#define FTD_IOCTL_ROLLBACK					_IOWR(FTD_CMD,  0x62, dev_t)	// Fixed size of struct PROLL_BACK
#define FTD_IOCTL_COMMIT					_IOWR(FTD_CMD,  0x63, dev_t)	// Fixed size of struct PC

#define FTD_IOCTL_FULLREFRESH_PAUSE_RESUME	_IOWR(FTD_CMD,  0x64, ftd_param_t)	// If the Group is in Full Refresh Mode then we will stop the full refresh 
																				// and close the connections and exit the connect thread. but we will remain
																				// in the FTD_MODE_FULL_REFRESH until we complete the whole Syncing.
																				// Depending on the valuse of ftd_param_t.value  1 - PAUSE , 0 - RESUME
#endif

#endif


#if 1 // MM_SERVICE_ALLOC_SUPPORT

// Shared Memory Map section info
#define SFTK_SHAREDMEMORY_FILENAME				"Sftk_SharedMemory"
#define SFTK_SHAREDMEMORY_USER_PATHNAME			"Sftk_SharedMemory"
#define SFTK_SHAREDMEMORY_KERNEL_PATHNAME	   L"\\BaseNamedObjects\\Sftk_SharedMemory"

// Shared Memory Map Mutex info
#define SFTK_SHAREDMEMORY_EVENT					"Sftk_SharedMemoryEvent"
#define SFTK_SHAREDMEMORY_USER_EVENT			"Sftk_SharedMemoryEvent"
#define SFTK_SHAREDMEMORY_KERNEL_EVENT		   L"\\BaseNamedObjects\\Sftk_SharedMemoryEvent"

#define	SFTK_SHAREDMEMORY_SIZE					(1 * 1024 * 1024)		// 1 MBytes may be more than enough

// New: for Cache/CacheDriverAPI/CacheDriverAPI.cpp (note: no L prefix)
// #define	REG_SYSTEM_SESSION_MEMORY_MANAGER	"System\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\"

#define	REG_SYSTEM_MM_PARAMETERS_ASCII		"System\\CurrentControlSet\\Services\\sftkblk\\Parameters\\"

// "MMSize" key stores value in Percentage relative to system's total Available Phy memory
#define REG_KEY_MM_SIZE_IN_PERCENTAGE			"MMAllocPercentage"		// stores in Percentage of Total Systems Avail Phys mem

// following 2 fields always get set by MM_START() in service
// "AvailablePhyMem" key stores value in Mega Bytes during MM_Start() time, Total systems available memory
#define REG_KEY_MM_AVAILABLE_PHY_MEM			"AvailablePhyMem"	// stores in Mega Bytes

// "AllocatedMMMem key stores value in Mega Bytes of maximum memory MM will used 
#define REG_KEY_MM_ALLOCATED_MEM				"MaxMMUseSize"		// stores in Mega Bytes

// following 2 fields are optional, if exist we use this values otherwise we use default values
// "AllocatedMMMem key stores value in Mega Bytes of maximum memory MM will used 
#define REG_KEY_MM_INIT_RAW_MEM_TO_ALLOCATE		"InitRawMemoryAllocate"		// stores in Mega Bytes

// "AllocatedMMMem key stores value in Mega Bytes of maximum memory MM will used 
#define REG_KEY_MM_INCREMENT_ALLOC_CHUNK_SIZE	"IncrAllocChunkSize"		// stores in Mega Bytes

// "ChunkSizeOfAllocation" key stores value in Bytes, this is number in increment will used to allocate RAW memory as needed...
// #define REG_KEY_MM_CHUNK_SIZE_ALLOCATION		"ChunkSizeOfAllocation"	// stores in bytes


#define MAX_MM_TYPE_ENTRIES		20

//
// Driver Registry Macros
//
#define	REG_SFTK_BLOCK_DIRECT_PARAMETERS			L"SYSTEM\\CurrentControlSet\\Services\\sftkblk\\Parameters"

#define	REG_SFTK_BLOCK_REGISTRY_DIRECT_PARAMETERS	L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\sftkblk\\Parameters"

#define	REG_SFTK_SLASH								L"\\"

#define	REG_SFTK_BLOCK_DRIVER_PARAMETERS		L"sftkblk\\Parameters"



#if TARGET_SIDE
#define	REG_PRIMARY_UNICODE						L"P"
#define	REG_SECONDARY_UNICODE					L"S"

#define	REG_KEY_TotalPLG						L"TotalPLG"					// REG_DWORD
#define	REG_KEY_TotalSLG						L"TotalSLG"					// REG_DWORD

#define	REG_KEY_LGNum_STRING_ROOT				L"sftkblk\\Parameters\\LGNum%S%08d"	// its Key
#define	REG_KEY_LGNum_KEY_ROOT					L"LGNum%S%08d"				// its Key
#else
#define	REG_KEY_TotalLG							L"TotalLG"					// REG_DWORD

#define	REG_KEY_LGNum_STRING_ROOT				L"sftkblk\\Parameters\\LGNum%08d"	// its Key
#define	REG_KEY_LGNum_KEY_ROOT					L"LGNum%08d"				// its Key
#endif

#define	REG_KEY_LGNumber						L"LGNumber"					// REG_DWORD

#if TARGET_SIDE
#define	REG_KEY_CreationRole					L"CreationRole"			// REG_DWORD : stores creation time role, this value never get changed.
#define	REG_KEY_PreviouseRole					L"PreviouseRole"		// REG_DWORD :stores Previouse role
#define	REG_KEY_CurrentRole						L"CurrentRole"			// REG_DWORD :stores Current time role
#define	REG_KEY_FailOver						L"FailOver"				// REG_DWORD :TRUE means Failover is ON 
#define	REG_KEY_JEnable							L"JEnable"				// REG_DWORD :TRUE means Journal is ON 
#define	REG_KEY_JApplyRunning					L"JApplyRunning"		// REG_DWORD :TRUE means Journal Apply running 
#define	REG_KEY_JPath							L"JPath"				// REG_SZ
#endif

#define	REG_KEY_PstoreFile						L"PstoreFile"				// REG_SZ
#define	REG_KEY_LastShutdown					L"LastShutdown"				// REG_DWORD	// TRUE means Last shutdown was successful else failed required full-recovery
#define	REG_KEY_TotalDev						L"TotalDev"					// REG_DWORD

// REG_KEY_LastShutdown gets updated to TRUE each time once Pstotre file is acceblile at service startup time
// During boot, if REG_KEY_LastShutdown is TRUE than Driver configures LG for tracking mode.
// At boot time  first time when Write appears to any Src devices under LG, LastShutdown becomes FALSE
// At service starttime, Pstore file gets opened and boot time write delta gets merged with pstore LRDB and updated
// on pstore file than only this Lastshutdown becomes TRUE till that moment its FALSE
// REG_KEY_LastShutdown FALSE means can't monitor writes at boot time, required LG full recovery again.

#if TARGET_SIDE
#define	REG_KEY_DevNum_STRING_ROOT				L"sftkblk\\Parameters\\LGNum%S%08d\\DevNum%08d"	// its Key
#define	REG_KEY_DevNum_LGNum_KEY_ROOT			L"LGNum%S%08d\\DevNum%08d"	// its Key
#else
#define	REG_KEY_DevNum_STRING_ROOT				L"sftkblk\\Parameters\\LGNum%08d\\DevNum%08d"	// its Key
#define	REG_KEY_DevNum_LGNum_KEY_ROOT			L"LGNum%08d\\DevNum%08d"	// its Key
#endif

#define	REG_KEY_DevInfo							L"DevInfo"					// REG_BIN

#define	REG_KEY_DevNumber						L"DevNumber"				// REG_DWORD
#define	REG_KEY_VolumeNumber					L"VolumeNumber"				// REG_DWORD
#define	REG_KEY_UniqueId						L"UniqueId"					// REG_BIN
#define	REG_KEY_SignatureUniqueId				L"SignatureUniqueId"		// REG_BIN
#define	REG_KEY_DiskVolumeName					L"DiskVolumeName"			// REG_SZ
#define	REG_KEY_VDevName						L"VDevName"					// REG_SZ
#define	REG_KEY_DevName							L"DevName"					// REG_DWORD
#define	REG_KEY_DiskSize						L"DiskSize"					// REG_DWORD
#define	REG_KEY_Lrdbsize						L"Lrdbsize"					// REG_DWORD
#define	REG_KEY_Hrdbsize						L"Hrdbsize"					// REG_DWORD

// Speceifies the Execution State of any thread
typedef enum EXECUTION_STATE
{
	RESUME = 0,
	PAUSE = 1
}EXECUTION_STATE, *PEXECUTION_STATE;

// Role defination enum
typedef enum ROLE_TYPE
{
	PRIMARY = 0,
	SECONDARY
} ROLE_TYPE, *PROLE_TYPE;

// Role defination enum
typedef enum JAPPLY_FILE_TYPE
{
	SR = 0,
	SR_NORMAL,
	NORMAL

} JAPPLY_FILE_TYPE, *PJAPPLY_FILE_TYPE;

#if TARGET_SIDE
// Journal File Format
#define JSR_i_FILENAME_UNICODE	L"J%4d%3d.SRi"	// %4d - LgNum, %3d - Sequence number
#define JSR_c_FILENAME_UNICODE	L"J%4d%3d.SRc"
#define JSN_i_FILENAME_UNICODE	L"J%4d%3d.SNi"
#define JSN_c_FILENAME_UNICODE	L"J%4d%3d.SNc"	

#define JN_c_FILENAME_UNICODE	L"J%4d%3d.Nc"

#define JSR_i_FILENAME_ASCII	"J%4d%3d.SRi"	// %4d - LgNum, %3d - Sequence number
#define JSR_c_FILENAME_ASCII	"J%4d%3d.SRc"
#define JSN_i_FILENAME_ASCII	"J%4d%3d.SNi"
#define JSN_c_FILENAME_ASCII	"J%4d%3d.SNc"	

#define JN_c_FILENAME_ASCII		"J%4d%3d.Nc"

typedef struct FAIL_OVER
{
	ULONG			LgNum;
	ROLE_TYPE		lgCreationRole;	// Passes Lg Creation role information
	BOOLEAN			UseJournal;	// TRUE means Do Failover with Journal else do not use Journal, Primary can go for tracking till failover finishes
//	BOOLEAN			Forced;	// TRUE means must successed else try to do this.
	LARGE_INTEGER	TimeOutForJApplyprocess;	// use in secondary creation role mode, value must be in negative 100 nanoseconds base...
} FAIL_OVER, *PFAIL_OVER;

typedef struct ROLL_BACK
{
	ULONG			LgNum;
	ROLE_TYPE		lgCreationRole;	// Passes Lg Creation role information
} ROLL_BACK, *PROLL_BACK;

typedef struct COMMIT
{
	ULONG			LgNum;
	ROLE_TYPE		lgCreationRole;	// Passes Lg Creation role information
} COMMIT, *PCOMMIT;

#endif // #if TARGET_SIDE

typedef struct REG_DEV_INFO
{
	ULONG		LgNum;
    ULONG		cDev;		// Dev Id used for Protocol to send to secondary and also used other places..

	ULONG		Disksize;				// in Sectors, Actual sizae = disksize * 512
	ULONG		Lrdbsize;
	ULONG		Hrdbsize;
	ULONG		Statsize;

	USHORT		DevNameLength;
	USHORT		VDevNameLength;
	USHORT		strRemoteDeviceNameLength;

	BOOLEAN		bUniqueVolumeIdValid;	// TRUE means UniqueIdLength and UniqueId has valid values
	USHORT		UniqueIdLength;

	BOOLEAN		bSignatureUniqueVolumeIdValid;	// TRUE means SignatureUniqueIdLength and SignatureUniqueId has valid values
	USHORT		SignatureUniqueIdLength;

	BOOLEAN		bSuggestedDriveLetterLinkValid;	// TRUE means SuggestedDriveLetterLinkLength and SuggestedDriveLetterLink has valid values
	USHORT		SuggestedDriveLetterLinkLength;
    /* Followings are type def data in binary 
    CHAR		devname[1];					// devname[256];      
    CHAR		vdevname[1];				// vdevname[256]
	CHAR		strRemoteDeviceName[1]		// strRemoteDeviceName[64];	
	UCHAR		UniqueId[1];					// UniqueId[256];256 is enough, if requires bump up this value.
	UCHAR		SignatureUniqueId[1];			// SignatureUniqueId[128]; 256 is enough, if requires bump up this value.
	UCHAR		SuggestedDriveLetterLink[1];	// SuggestedDriveLetterLink[128];	// 256 is enough, if requires bump up this value.
	*/
} REG_DEV_INFO, *PREG_DEV_INFO;

// Following Structures and enums are used to define commands in shared memory map section.
// These are used to pass information or commands from Driver to Service.
typedef enum SM_COMMAND
{
	CMD_MM_ALLOC = 1,
	CMD_MM_FREE,
	CMD_TERMINATE,
	CMD_NONE
} SM_COMMAND, *PSM_COMMAND;

// These are used to pass Error information of executed commands from service to Driver.
typedef enum SM_STATUS
{
	SM_STATUS_SUCCESS		= 0,
	SM_ERR_INVALID_COMMAND	= 1,
	SM_ERR_MM_REACH_LIMIT,				// No More Memopry can allocate
	SM_ERR_MM_ALLOC_FAILED,				// Memory Allocation Failed !!
	SM_ERR_MM_FREE_FAILED,				// Memory Free Failed !!!
} SM_STATUS, *PSM_STATUS;


// CMD_SUCCESS used for success value elase failed.
#define CMD_SUCCESS			0

#define CMD_NOT_EXECUTED		1
#define CMD_EXECUTE_COMPLETE	2

typedef struct SM_CMD
{
	SM_COMMAND	Command;				// identifies type of MM structure
	UCHAR		Executed;				// TRUE means ITs executed and returned status is valid
	ULONG		InBufferSize;
	PVOID		InBuffer;
	ULONG		OutBufferSize;
	PVOID		OutBuffer;
	ULONG		RetBytes;
	SM_STATUS	Status;					// values from enum SM_STATUS
} SM_CMD, *PSM_CMD;

// Following GET_MM_DATABASE_SIZE structure used with IOCTL_GET_MM_DATABASE_SIZE
// Since Service Mode Does allocation of memory, 
// SizeOfMemoryHold is used to calculate total number of nodes required to allocate memory
typedef struct GET_MM_DATABASE_SIZE
{
	UCHAR	MMIndex;				// identifies type of MM structure
	ULONG	NodeSize;				// fixed size 
	ULONG	PageSizeRepresenting;	// 4K or 64 K based
	BOOLEAN	RawMemory;				// TRUE means this is type used for RAW Memory
} GET_MM_DATABASE_SIZE, *PGET_MM_DATABASE_SIZE;

// IOCTL_GET_MM_DATABASE_SIZE is used to send structure GET_MM_DATABASE_SIZE to driver
typedef struct MM_DATABASE_SIZE_ENTRIES
{
	BOOLEAN		AWEUsed;			// TRUE means AWE will used for memory else VirtualAlloc will be used
	ULONG		PageSize;			// PageSize = System defined page size = 4K or ...
	ULONG		VChunkSize;			// System defined usermode Virtual Alloc Chunksize = 64K or ..
	ULONG		MaxAllocatePhysMemInMB;	// in Mega Bytes
	// IncrementAllocationChunkSize is used to allocate on demand from driver, this memory is max at a time to allocate
	// when driver ask to allocate
	ULONG		IncrementAllocationChunkSizeInMB;

	// Tuning Parameters for Alloc/Free, values either come from registry or from Defualt values
	UCHAR		AllocThreshold;			// in Percentage, Used to alloc new memory from system/Service 
	ULONG		AllocIncrement;			// amount of mem used to alloc in this increment
	ULONG		AllocThresholdTimeout;	// Threshold check time interval In Milliseconds
	ULONG		AllocThresholdCount;	// Optional, Threshold check counter, mey be not needed 

	UCHAR		FreeThreshold;			// in Percentage, Used to Free new memory from system/Service 
	ULONG		FreeIncrement;			// amount of mem used to Free in this increment
	ULONG		FreeThresholdTimeout;	// Threshold check time interval In Milliseconds
	ULONG		FreeThresholdCount;		// Optional, Threshold check counter, mey be not needed 

	ULONG		NumberOfEntries;

	GET_MM_DATABASE_SIZE	MmDb[MAX_MM_TYPE_ENTRIES];
} MM_DATABASE_SIZE_ENTRIES, *PMM_DATABASE_SIZE_ENTRIES;

typedef struct AWE_INFO
{
	PULONG	ArrayPFNs;
	ULONG	NumberOfArray;
} AWE_INFO, *PAWE_INFO;

typedef struct VIRTUAL_MM_INFO
{
	PULONG	Vaddr;			// Point to virtual address
	ULONG	SizeOfMem;		// size in bytes of Vaddr memory
} VIRTUAL_MM_INFO, *PVIRTUAL_MM_INFO;

// Following structure used with IOCTL_SET_MM_DATABASE_MEMORY
typedef struct SET_MM_DATABASE_MEMORY
{
	UCHAR	MMIndex;			// identifies type of MM structure
	ULONG	NodeSize;			// Fixed Size of node
	ULONG	TotalNumberOfNodes;	// Total number of nodes passed in allocated memory or it will get fit this many nodes
	ULONG	NumberOfArray;
	ULONG	ChunkSize;			// in bytes, specify Memory allocation chunksize, for AWE its always PAGE_SIZE = 4K
	ULONG	TotalMemorySize;	// in bytes total memory size passed with this transaction

	// AWE_INFO			AweMem;
	VIRTUAL_MM_INFO		VMem[1];
	
}SET_MM_DATABASE_MEMORY, *PSET_MM_DATABASE_MEMORY;

// Following structure used with IOCTL_MM_INIT_RAW_MEMORY 
// It also used for Dynamic alloc/free using command CMD_MM_ALLOC/CMD_MM_FREE in shared memory map IPC
// comminucation between driver and service
typedef struct SET_MM_RAW_MEMORY
{
	ULONG	NumberOfArray;
	ULONG	ChunkSize;			// in bytes, specify Memory allocation chunksize, for AWE its always PAGE_SIZE = 4K
	ULONG	TotalMemorySize;	// in bytes total memory size passed with this transaction
	PULONG	ArrayOfMem;
}SET_MM_RAW_MEMORY, *PSET_MM_RAW_MEMORY;

// This structure is used to store in Global MM structure in service
typedef struct MM_SET_DATABASE_SIZE_ENTRIES
{
	ULONG						NumberOfEntries;
	PSET_MM_DATABASE_MEMORY		PtrMmDb[MAX_MM_TYPE_ENTRIES];
} MM_SET_DATABASE_SIZE_ENTRIES, *PMM_SET_DATABASE_SIZE_ENTRIES;

#endif



#if	MM_TEST_WINDOWS_SLAB
#define FTD_TEST_MM_MAX_ALLOC		_IOWR(FTD_CMD,  0x34, dev_t)	// Test MM Max Allocation
#define FTD_TEST_GET_MM_ALLOC_INFO	_IOWR(FTD_CMD,  0x35, dev_t)	// Get MM Information

typedef struct MM_LOOKASIDE
{
	USHORT	Depth;
    USHORT	MaximumDepth;
    ULONG	TotalAllocates;
    union {
        ULONG AllocateMisses;
        ULONG AllocateHits;
    };

    ULONG	TotalFrees;
    union {
        ULONG FreeMisses;
        ULONG FreeHits;
    };

    ULONG	PoolType;
    ULONG	Tag;
    ULONG	Size;
    PVOID	Allocate;
    PVOID	Free;
    ULONG	LastTotalAllocates;
    union {
        ULONG LastAllocateMisses;
        ULONG LastAllocateHits;
    };

} MM_LOOKASIDE, *PMM_LOOKASIDE;

typedef struct MM_ANCHOR_INFO
{
	ULONG			NumOfFreeList; // Number of Free nodes exist
	ULONG			NumOfUsedList; // Number of Used nodes exist

	UCHAR			Type;	// Values are MM_TYPE_MM_ENTRY, etc.

	UCHAR			Flag;	// MM_ANCHOR_FLAG_LOOKASIDE_INIT_DONE

	ULONG			FixedSize;
	ULONGLONG		TotalSize;	// In bytes total memory allocated for this MM_Type	
	
	ULONG			MaximumSize; // MaximumSize in bytes can get allocate
	ULONG			MinimumSize; // MaximumSize in bytes can get used 

	ULONG			NumOfNodestoKeep;	// MM_DEFAULT_MAX_NODES_TO_ALLOCATE, user requested number of nodes to allocate and keep
	ULONG			TotalNumAllocated;	// Total Entries allocated, 
	
	MM_LOOKASIDE	Lookaside;
	
	PVOID			Allocate;	// Allocate function pointer storage
    PVOID			Free;		// Free function pointer storage
    
} MM_ANCHOR_INFO, *PMM_ANCHOR_INFO;

#endif

/* 
 * AIX IOC's use high order bits to encode
 * in/out-ness, generating much compiler 
 * grief. our kernel code needs only see 
 * the IOC parms, not the rest of the
 * information encoded in the IOC number.
 */
#define IOC_CST(c) ((c))

typedef unsigned __int64    ftd_uint64_t;
typedef __int64             ftd_int64_t;

#if 0 // PARAG : already defined in sftkprotocol.h file
/* This is the header for a writelog entry */
typedef struct wlheader_s {
    ftd_uint64_t    majicnum;           /* somebody doesn't know how to spell */
    unsigned int    offset;             /* in sectors */
    unsigned int    length;             /* in sectors */
    time_t          timestamp;
    dev_t           dev;                /* device id */
    dev_t           diskdev;            /* disk device id */
    int             flags;              /* flags */
    int             complete;           /* I/O completion flag */
    int             timoid;             /* sync mode timeout id */
#ifdef KERNEL
    void            *group_ptr;         /* internal use only */
    IRP             *bp;                /* IRP to complete on migrate */
#else
    void            *opaque1;           /* Use these and you'll hate life */
    void            *opaque2;           /* and we won't care!!!! */
#endif
} wlheader_t;
#endif

typedef struct disk_stats_s {
    char            devname[64];
    dev_t           localbdisk ; /* block device number */
    unsigned long   localdisksize ;

    int             wlentries ;
    int             wlsectors ;

    ftd_uint64_t    readiocnt;
    ftd_uint64_t    writeiocnt;
    ftd_uint64_t    sectorsread;
    ftd_uint64_t    sectorswritten;
} disk_stats_t ;

typedef struct ftd_stat_s {
    int                   lgnum;
#if TARGET_SIDE
	ROLE_TYPE			lgCreationRole;	// Passes Lg Creation role information
#endif
    int                   connected;
    /* Statistics */
    unsigned long       loadtimesecs ;         /* in seconds */
    unsigned long       loadtimesystics ;        /* in sys ticks */
    int                 wlentries ;
    int                 wlsectors ;
    int                 bab_free;
    int                 bab_used;
    int                 state;
    int                 ndevs;
    unsigned int        sync_depth;
    unsigned int        sync_timeout;
    unsigned int        iodelay;

} ftd_stat_t;


/* Struct for ENTRIES_HAVE_MIGRATED */
typedef struct migrated_entries_s {
    unsigned long   bytes;      /* Number of bytes migrated */
} migrated_entries_t ;

/* info needed to add a logical group */
typedef struct ftd_lg_info_s {
    dev_t        lgdev;
    char         vdevname[256];		// pstore/Sstore file name with complete path
    int          statsize;			// not used anymore though this 

#if TARGET_SIDE
	ROLE_TYPE    lgCreationRole;	// Passes Lg Creation role information
	CHAR         JournalPath[256];	// Passes Journal Directory storage complete path (file name should not be there)
#endif
} ftd_lg_info_t;

typedef struct ftd_devnum_s {
    dev_t b_major;
    dev_t c_major;
} ftd_devnum_t;

#if 1 // PARAG_ADDED
// Unique ID Format from Win2K and above Are :
// "\??\Volume{GUID}\", 
// 
// signature - volume(nnnnnnnn-nnnnnnnnnnnnnnnn-nnnnnnnnnnnnnnnn) = total 50 bytes
// Example : Signature Format: volume(Signature-StartingOffset.QuadPart-PartitionLength.QuadPart)
//
// SuggestedDriveLetterLink- stoires Drive letter (if Drive Letter is persistent across boot) or
// suggest Drive Letter Dos Sym,bolic Name. Example: "\??\D:", 
//
// #define SIGNATURE_UNIQUE_ID		"volume(%08x-%016x-%016x)"	// total 50 bytes
#define SIGNATURE_UNIQUE_ID		"volume(%08X-%016I64X-%016I64X)"	// total 50 bytes

#endif

/* info needed to add a device to a logical group */
typedef struct ftd_dev_info_s {
    int          lgnum;
#if TARGET_SIDE
	ROLE_TYPE    lgCreationRole;	// Passes Lg Creation role information
#endif
    dev_t        localcdev;  /* dev_t for raw disk device */
/* FIXME: I think that we should just be passing in a minor number here wl */
    dev_t        cdev;       /* dev_t for raw ftd disk device */
    dev_t        bdev;       /* dev_t for block ftd disk device */
    unsigned int disksize;		// in secotrs
    int          lrdbsize32; /* number of 32-bit words in LRDB */
    int          hrdbsize32; /* number of 32-bit words in HRDB */
    int          lrdb_res;   /* LRDB resolution */
    int          hrdb_res;   /* HRDB resolution */
    int          lrdb_numbits; /* number of valid bits in LRDB */
    int          hrdb_numbits; /* number of valid bits in HRDB */
    int          statsize;   /* number of bytes in the statistics buffer */
    unsigned int lrdb_offset;   /* Where to place the lrdb in pstore */
    char         devname[256];      /* in only */
    char         vdevname[256];     /* in only */

	// Added the Remote Device Name this will be sent at handshake time
	char strRemoteDeviceName[64];	// Remote Drive Letter from Config File

#if 1 // PARAG_ADDED
	// OS supported Unique ID for Volume (Raw Disk/ Disk Partition)
	BOOLEAN	bUniqueVolumeIdValid;	// TRUE means UniqueIdLength and UniqueId has valid values
	USHORT  UniqueIdLength;
    UCHAR   UniqueId[256];			// 256 is enough, if requires bump up this value.

	// Our customize alternate Disk Signature based Unique ID for Volume (Raw Disk/ Disk Partition)
	BOOLEAN	bSignatureUniqueVolumeIdValid;	// TRUE means SignatureUniqueIdLength and SignatureUniqueId has valid values
	USHORT  SignatureUniqueIdLength;
    UCHAR   SignatureUniqueId[128];			// 256 is enough, if requires bump up this value.

	// Optional - If Device is formatted disk Partition, than associated drive letter symbolik link name info 
	BOOLEAN	bSuggestedDriveLetterLinkValid;	// TRUE means SuggestedDriveLetterLinkLength and SuggestedDriveLetterLink has valid values
	USHORT  SuggestedDriveLetterLinkLength;
    UCHAR   SuggestedDriveLetterLink[128];	// 256 is enough, if requires bump up this value.
#endif

} ftd_dev_info_t;

/* structure for GET_OLDEST_ENTRIES */
typedef struct oldest_entries_s {
    int          *addr;     /* address of buffer */
    unsigned int offset32;  /* offset from tail to begin copying in 32-bit */
    unsigned int len32;     /* length of buffer in 32-bit words */
    unsigned int retlen32;  /* RETURNED: length of data returned */
    int          state;     /* RETURNED: state of the group */
} oldest_entries_t;

/*
 * stat buffer descriptor
 */
typedef struct stat_buffer_t {
    dev_t lg_num;  /* logical group device id */
#if TARGET_SIDE
	ROLE_TYPE    lgCreationRole;	// Passes Lg Creation role information
#endif
    dev_t dev_num; /* device id. used only for GET_DEV_STATS */

#if 1	// PARAG_ADDED
	char *DevId;  // String represents unique SFTK_DEV across all LG's SFTK_DEV	 
				  // Using DevId instead of dev_num to find unique SFTK_DEV in Driver
#endif
    int   len;     /* length in bytes */
    char  *addr;   /* pointer to the buffer */
} stat_buffer_t;

/* FIXME: This needs work. Add more info about each device, especially
   the bitmap info. */
typedef struct dirtybit_array_t {
    int   numdevs;
    int   dblen32;
    int   state_change; /* 1 -> do automatic state change, 0 -> don't */
    dev_t *devs;  /* add more crap here */
    int   *dbbuf; /* the bitmap data */
} dirtybit_array_t;

/* a generic buffer passed to the kernel. Contents determined by context */
typedef struct buffer_desc_s {
    int *addr;  /* a user-mode address */
    int len32;  /* number of 32 bit words in addr */
} buffer_desc_t;

/* used to change the state of a device */
typedef struct ftd_state_s {
    dev_t	lg_num;
#if TARGET_SIDE
	ROLE_TYPE    lgCreationRole;	// Passes Lg Creation role information
#endif
    int		state;
	BOOLEAN	bInconsistantData;		// At new LG create, this becomes TRUE, it remains FALSE till first time Smart Refresh gets started.
									// create time LG, State mode default  get sets to Tracking mode.
} ftd_state_t;

/*
 * The kernel assigned device numbers for the driver
 */
typedef struct ftd_dev_num_s {
    int b_major;
    int c_major;
} ftd_dev_num_t;

typedef struct ftd_param_s {
    int lgnum;
#if TARGET_SIDE
	ROLE_TYPE    lgCreationRole;	// Passes Lg Creation role information
#endif
    int value;
} ftd_param_t;


// Following structure get passed as input paraneter for IOCTL_CONFIG_BEGIN 
typedef struct CONFIG_BEGIN_INFO
{
    ULONG	HostId;				// The HostId that us used to install, uses for PMD to RMD Protocol communications
	CHAR	Version[32];		// The Version of the Product, uses for PMD to RMD Protocol communications
	CHAR	SystemName[64];			// computer name for local system...needed for pstore file access in cluster....
} CONFIG_BEGIN_INFO, *PCONFIG_BEGIN_INFO;

//
// -------- Following are new Structures defined for statistics IOCTL ------
//
// STATISTICS_INFO structure will be used per LG and also per Src Device in LG.
// For LG, it stores total stats values (accumulation of all src devices stats under that LG)
// For DEV, it stores total stats values for that device only.
typedef struct STATISTICS_INFO
{
	// Foll. fields reflects User IO stats activity per Dev/or LG
	UINT64		RdCount;			// readiocnt: Applications Total number of Read ocurred to src device, 
	UINT64		WrCount;			// writeiocnt: Applications Total number of Write ocurred to src device, 
	UINT64		BlksRd;				// sectorsread	:Applications Total number of Sectors Read ocurred to src device, 
	UINT64		BlksWr;				// sectorswritten: Applications Total number of Sectors Write ocurred to src device, 

	UINT64		RemoteWrPending;	// wlentries: number of incoming writes happened to device, and yet this 
									// writes is not commited to secondary side
	UINT64		RemoteBlksWrPending;// wlsectors: Total number of incoming writes Sectors to device, and yet this 
									// writes is not commited to secondary side

	// Foll. fields reflect current view of Queue Management per Dev/or LG
	UINT64		QM_WrPending;			// Num of Wr exist in pending queue 
	UINT64		QM_BlksWrPending;		// total sectors of Wr exist in pending queue 

	UINT64		QM_WrCommit;			// Num of Wr exist in Commit queue 
	UINT64		QM_BlksWrCommit;		// total sectors of Wr exist in Commit queue 

	UINT64		QM_WrRefresh;			// Num of Wr exist in Refresh queue 
	UINT64		QM_BlksWrRefresh;		// total sectors of Wr exist in Refresh queue 

	UINT64		QM_WrRefreshPending;			// Num of Wr exist in Refresh queue 
	UINT64		QM_BlksWrRefreshPending;		// total sectors of Wr exist in Refresh queue 

	UINT64		QM_WrMigrate;			// Num of Wr exist in Migrate queue 
	UINT64		QM_BlksWrMigrate;		// total sectors of Wr exist in Migrate queue 

	UINT64		QM_WrTReceive;			// Num of Wr exist in Target Receive Queue 
	UINT64		QM_BlksWrTReceive;		// total sectors of Wr exist in Target Receive Queue 

	UINT64		QM_WrTSend;				// Num of Wr exist in Target Send Queue 
	UINT64		QM_BlksWrTSend;			// total sectors of Wr exist in Target Send Queue 

	// Foll. fields reflect current view of Memory Management usage per Dev/or LG
	// Following 2 fields are gloabl for ALL lg and Devs, same...
	UINT64		MM_TotalMemAllocated;		// At Present, Total Memory allocated for MM from OS 
											// used from MM_MANAGER->MmSlab[MM_TYPE_4K_NPAGED_MEM].TotalMemSize
	UINT64		MM_TotalMemIsUsed;			// At Present, Total Memory is used in MM by driver, 
											// used from MM_MANAGER->MmSlab[MM_TYPE_4K_NPAGED_MEM].TotalNumberOfPagesInUse * MM_MANAGER->PageSize
	
	UINT64		MM_LgTotalMemUsed;		// Till moment, total memory used for QM for current LG, this value never gets decremented
	UINT64		MM_LgMemUsed;			// At Present, total memory allocated for QM for current LG
	
	UINT64		MM_LgTotalRawMemUsed;	// Till moment, total RAW memory used for QM for current LG, this value never gets decremented
	UINT64		MM_LgRawMemUsed;		// At Present, total RAW memory allocated for QM for current LG
	
	UINT64		MM_TotalOSMemUsed;		// Till moment, total Direct OS memory used for MM under low memory situations, this value never gets decremented
	UINT64		MM_OSMemUsed;			// At Present, total Direct OS memory allocated for MM under low memory situations

	UINT64		MM_TotalNumOfMdlLocked;			// Till moment, total Number Of MDL Alloc is done. This MDL memory allocated directly from OS, this value never gets decremented
	UINT64		MM_TotalSizeOfMdlLocked;		// Till moment, total size of memory all allocated MDL is representing, this value never gets decremented

	UINT64		MM_TotalNumOfMdlLockedAtPresent;	// At Present, total Number Of MDL Alloc is done. This MDL memory allocated directly from OS.
	UINT64		MM_TotalSizeOfMdlLockedAtPresent;	// At Present, total size of memory all allocated MDL is representing, 
	

	// Foll. Fields reflects Refresh Activity stats
	ULONG		NumOfBlksPerBit;		// total number of sectors per bit values

	UINT64		TotalBitsDirty;			// Total Number Of Bits are Dirty per dev/or LG
	UINT64		TotalBitsDone;			// Total Number of Dirty bits recovered till moment per dev/or LG

	ULONG		CurrentRefreshBitIndex;	// rsyncoff: this values is in bit and valid only for Dev based, so CurrentRefreshBitIndex * NumOfBlksPerBit.

	// foll. used for pstore access stats
	UINT64		PstoreLrdbUpdateCounts;		// Number of times LRDB bitmap updated to Pstore file
	UINT64		PstoreHrdbUpdateCounts;		// Number of times LRDB bitmap updated to Pstore file

	UINT64		PstoreFlushCounts;			// total Number of times Pstore Got Flush 
	UINT64		PstoreBitmapFlushCounts;	// total Number of times Pstore Bitmaps Got Flush 

	// Following are the Session Manager Statistics metrics for all connections for a logical group

	UINT64		SM_PacketsSent;					// The total TDI Packets Sent
	UINT64		SM_BytesSent;						// The total Bytes Sent in those Packets
	
	UINT64		SM_EffectiveBytesSent;			// Effective Bytes Sent along with the compressed data

	UINT64		SM_PacketsReceived;				// The total TDI Packets Received
	UINT64		SM_BytesReceived;				// The total Bytes Received

	ULONG		SM_AverageSendPacketSize;			// Average Send Packet Size.
    ULONG		SM_MaximumSendPacketSize;		// Maximum send Packet Size.
	ULONG		SM_MinimumSendPacketSize;		// Average Send Packet Size.

    ULONG		SM_AverageReceivedPacketSize;		// The Average Received Packet Size.
    ULONG		SM_MaximumReceivedPacketSize;	// Maximum Received packet Size.
	ULONG		SM_MinimumReceivedPacketSize;	// Minimum Received Packet Size.

	UINT64		SM_AverageSendDelay;			// estimated delay on this connection.
	UINT64		SM_Throughput;					// estimated throughput on this connection.

	ULONG		SM_Connection;					// 0 - Disconnected , 1 - Connected

	// Follo. fields used for session manager for socket connections stats
#if TARGET_SIDE
	// secondary side informations

	// --- Journal Write Management
	// Current 'i' Journal information
	ULONG		JcurSR_FileNo;			// for Smart refresh, current 'i' Journal file number used in journal mode.
	ULONGLONG	JcurSR_Offest;			// for Smart refresh, current 'i' Journal file's offset where to write next record.
	ULONGLONG	JcurSN_Offest;			// for Smart refresh BAB, current 'i' Journal file's offset where to write next record.
	
	// Current 'c' Normal Mode Journal information
	//	ULONG		JcurNormalFileNo;	// Normal Mode Journal current file number used for journaling mode.
	ULONGLONG	JcurN_Offest;		// for Normal Mode, Journal current file's offset where to write next record.

	// --- statistics information for all existing journals
	ULONG		JTotalSetOfFiles;			// Counter starts from 1 to total number of Journal file
	ULONGLONG	JcurSRN_TotalFileSize;		// for Smart refresh and BAB, current 'i' Journal file size in bytes. 
	ULONGLONG	TotalAllJFileSize;			// Total all existing journals file size including 'i' and 'c' 
	ULONGLONG	TotalAllCJFileSize;			//Total all existing Commited 'c' journals file size, 

	// --- Journal Apply Informations
	ULONG				JCurApplyFileNumber;	// starts from 0 to 999 and goes in circular
	JAPPLY_FILE_TYPE	JCurApplyFileType;		// curremnt Journal file type applying, sequence is: SR, then SR_Normal then Normal	
	ULONGLONG			JCurApplyFileOffset;	// current apply file offset
	ULONGLONG			JTotalDoneApplySize;	// Till moment total size of data applied. 

	// 
	BOOLEAN		JOverFlow;					// TRUE means Journal Overflow happened
	BOOLEAN		ProtoSendResumeTracking;	// TRUE means it requires sending Resume Primary Tracking mode proto command

	// Statistics for Target Write Thread
	ULONG			TWriteNumOfAsyncWriteAllowed;		// TWrite : Max number Of Async Disk Writes allowed
	ULONG			TWriteMaxSizeOfAsyncWriteAllowed;	// TWrite : Max size Of Async Disk Writes allowed
	ULONGLONG		TWriteTotalPktProcessed;			// TWrite : Keeps counter for total all pkts processed by this thread
	ULONGLONG		TWriteTotalIOPktsProcessed;			// TWrite : Keeps counter for total IO Plts Processed 
	ULONGLONG		TWriteTotalWriteSizetoDisk;			// TWrite : Keeps Total Size Write done to disk till moment
	ULONGLONG		TWriteTotalWriteSizetoJournal;		// TWrite : Keeps Total Size Write done to Journal till moment

	// JouranlApply for Target Write Thread statistics
	ULONG			JApplyNumOfAsyncWriteAllowed;		// JApply : Max number Of Async Disk Writes allowed
	ULONGLONG		JApplyTotalWriteSizetoDisk;			// JApply : Keeps Total Size Write done to disk till moment
#endif

} STATISTICS_INFO, *PSTATISTICS_INFO;


typedef struct DEV_STATISTICS
{
	ULONG				LgNum;				// stat_buffer_t->lg_num: LG Num
#if TARGET_SIDE
	ROLE_TYPE			lgCreationRole;	// Passes Lg Creation role information
#endif
	ULONG				cdev;				// stat_buffer_t->dev_num: Device Unique Id Per LG
	ULONG				DevState;			// for future used, reflect device full/smart refresh completed or not
	//  CHAR			Devname[256];		// User supplied information, from ftd_dev_info_t
	ULONG				Disksize;			// in Sectors, Actual Device size = disksize * 512 for bytes

	STATISTICS_INFO		DevStats;			// Device Statistics information
} DEV_STATISTICS, *PDEV_STATISTICS;

#define ALL_DEV		0xFFFFFFFF	// used to get all devs stats info set in LG_STATISTICS->cdev

#define GET_ONLY_LG_INFO					1
#define GET_LG_AND_SPECIFIED_ONE_DEV_INFO	2
#define GET_LG_AND_ITS_ALL_DEV_INFO			3

typedef struct LG_STATISTICS
{
	ULONG			LgNum;			// stat_buffer_t->lg_num: LG Num
#if TARGET_SIDE
	ROLE_TYPE    lgCreationRole;	// Passes Lg Creation role information

	// following fields driver passed it back, it used only if its secondary mode lg
	// --- Role information
	ROLE_TYPE	CreationRole;		// stores creation time role, this value never get changed.
	ROLE_TYPE	PreviouseRole;		// stores Previouse Role of current running role
	ROLE_TYPE	CurrentRole;		// stores Current Running Role 

	// ---- Secondary side usage attributes which defines secondary state mode
	BOOLEAN		FailOver;			// TRUE means Failover is enbled.
	BOOLEAN		JEnable;			// TRUE means Journal is ON.
	BOOLEAN		JApplyRunning;		// TRUE means Journal Apply is running
	BOOLEAN		JApplyWorkDone;		// TRUE means Journal Apply is done
	
	// --- protocol related fields.
	BOOLEAN		ProtoRunTracking;	// Primary : Protocol command set it to TRUE when it wants Primary to be run in tracking till 
									// users says to change or protocol command says to make it FALSE
									// used in Journal Overflow, failover with no journal, or any seriouse protocol error 
#endif

	ULONG			Flags;			// GET_ONLY_LG_INFO: This Flag defines what type of information caller wants

	ULONG			cdev;			// used only if Flags == GET_LG_AND_SPECIFIED_ONE_DEV_INFO to get specified Device stats info only
	ULONG			LgState;		// LG state mode

	STATISTICS_INFO	LgStats;		// LG Statistics information

	ULONG			NumOfDevStats;	// Number of entries passed in DevStats Array
	DEV_STATISTICS	DevStats[1];	// array of DevStats

} LG_STATISTICS, *PLG_STATISTICS;


// Memory : ALL_LG_STATISTICS + LG_STATISTICS[NumOfLgEntries] + for each LG DEV_STATISTICS[NumOfDevStats]
//
//	ULONG 
//	MAX_SIZE_ALL_LG_STATISTICS( ULONG _TotalLg_, ULONG _TotalDev_)
// 
// This API return total size of ALL_LG_STATISTICS required for FTD_GET_STATS_INFO
#define MAX_SIZE_LG_STATISTICS(_TotalDev_)		\
				(	sizeof(LG_STATISTICS) + (sizeof(DEV_STATISTICS) * (_TotalDev_) ) )

// Memory : ALL_LG_STATISTICS + LG_STATISTICS[NumOfLgEntries] + for each LG DEV_STATISTICS[NumOfDevStats]
//
//	ULONG 
//	MAX_SIZE_ALL_LG_STATISTICS( ULONG _TotalLg_, ULONG _TotalDev_)
// 
// This API return total size of ALL_LG_STATISTICS required for FTD_GET_STATS_INFO
#define MAX_SIZE_ALL_LG_STATISTICS(_TotalLg_, _TotalDev_)		\
				(	sizeof(ALL_LG_STATISTICS) + (sizeof(LG_STATISTICS) * (_TotalLg_) ) +   \
					(sizeof(DEV_STATISTICS) * (_TotalDev_) ) )

typedef struct ALL_LG_STATISTICS
{
	ULONG			NumOfLgEntries;		// Memories for Total Number of Entries for LG_STATISTICS struct for Dev under LG
	ULONG			NumOfDevEntries;	// Memories for Total Number of Entries for DEV_STATISTICS struct for Dev under LG
	LG_STATISTICS	LgStats[1];		// array of DevStats

} ALL_LG_STATISTICS, *PALL_LG_STATISTICS;

//
// -------- Following are new Structures defined for Tuning Params IOCTL ------
//

#define DEFAULT_STATE_BUFFER_SIZE	(4096)

#define DEFAULT_LRDB_BITMAP_SIZE	(8192)	
#define DEFAULT_HRDB_BITMAP_SIZE	(131072)	

#define DEFAULT_MAX_SEND_BUFFERS	5
#define DEFAULT_MAX_RECEIVE_BUFFERS	5

#define DEFAULT_SR_SENDCIMMITRECORDS_PER_SRRECORDS	5

#define DEFAULT_CHUNK_SIZE          0x100000
#define DEFAULT_NUM_CHUNKS          0x40
#define DEFAULT_MAXMEM              0x00000000

// used to set SFTK_LG->AckWakeupTimeout values. TODO : make this user defined or tunable....
#define DEFAULT_TIMEOUT_FOR_ACK_THREAD				-(300*1000*1000);  // relative 30 seconds
#define DEFAULT_TIMEOUT_FOR_REFRESH_THREAD			-(600*1000*1000);  // relative 60 seconds

#define DEFAULT_TIMEOUT_FOR_TARGET_WRITE_THREAD			-(150*1000*1000);  // relative 15 seconds
#define DEFAULT_TIMEOUT_FOR_JAPPLY_THREAD				-(150*1000*1000);  // relative 15 seconds

#define DEFAULT_TIMEOUT_FOR_MASTERIRP_PROCESSING_THREAD			-(20*1000*1000);  // relative 2 seconds

#define DEFAULT_TIMEOUT_FOR_MAX_WAIT_FOR_OUTBAND_PKT			-(1800*1000*1000);  // relative 180 seconds

#define DEFAULT_TIMEOUT_FOR_OUTBAND_HANDSHAKE_SLEEP	-(300*1000*1000);  // relative 30 seconds
#define DEFAULT_TIMEOUT_FOR_CONNECTION_WAIT_SLEEP	-(300*1000*1000);  // relative 30 seconds

#define DEFAULT_TIMEOUT_FOR_REFRESHPOOL_GET_NEXT_PKT	-(10*1000*1000);  // relative 1 seconds
#define DEFAULT_TIMEOUT_FOR_RELEASE_POOL_WAIT			-(10*1000*1000);  // relative 1 seconds

#define DEFAULT_TIMEOUT_FOR_SENDBUFFER_AVAILABILITY_WAIT	-(10*1000*1000)  // relative 1 seconds
#define DEFAULT_TIMEOUT_FOR_PACKETRETRIVAL_WAIT				-(10*1000*1000)  // relative 1 seconds

#define DEFAULT_TIMEOUT_FOR_FULLREFRESH_PAUSE_WAIT			-(10*1000*1000)  // relative 1 seconds
#define DEFAULT_TIMEOUT_FOR_LISTEN_THREAD					-(10*1000*1000)  // relative 1 seconds

#define	DEFAULT_ALLOWED_NUM_OF_PKTS_PENDING_FOR_REFRESH		500	// sync with RMD
#define	DEFAULT_ALLOWED_NUM_OF_PKTS_PENDING_FOR_COMMIT		500	// sync with RMD
#define	DEFAULT_ALLOWED_MAX_SIZE_PENDING_FOR_REFRESH		(20*1024*1024)
#define	DEFAULT_ALLOWED_MAX_SIZE_PENDING_FOR_COMMIT			(20*1024*1024)

#define	DEFAULT_NUM_OF_PKTS_SEND_AT_A_TIME					(2)
#define	DEFAULT_NUM_OF_PKTS_RECV_AT_A_TIME					(2)

#define MAX_IO_FOR_TRACKING_TO_SMART_REFRESH	10

// Default Maximum Raw Data Size which gets transfered to secondary.
#define	DEFAULT_MAX_TRANSFER_UNIT_SIZE			(256 * 1024)	

// Defualt Send Buffer Size
#define	DEFAULT_MAX_SEND_BUFFER_SIZE			(DEFAULT_MAX_TRANSFER_UNIT_SIZE + 1024)
// this is for run time confirurable buffer size
#define	CONFIGURABLE_MAX_SEND_BUFFER_SIZE(_LG_MaxTransferUnit_, _NoOfPktsAtAtime_)			( ((_LG_MaxTransferUnit_) + 1024) * (_NoOfPktsAtAtime_))
// Defualt Receive Buffer Size
#define	DEFAULT_MAX_RECEIVE_BUFFER_SIZE			(DEFAULT_MAX_TRANSFER_UNIT_SIZE + 1024)
// this is for run time confirurable buffer size
#define	CONFIGURABLE_MAX_RECIEVE_BUFFER_SIZE(_LG_MaxTransferUnit_, _NoOfPktsAtAtime_)		( ((_LG_MaxTransferUnit_) + 1024) * (_NoOfPktsAtAtime_))

// Default Maximum Number of Refresh Async Read IO allowed
#define	DEFAULT_NUM_OF_ASYNC_REFRESH_IO			(5)	


// TUNING_PARAM structure will be used per LG 
typedef struct TUNING_PARAM
{
	ULONG		TrackingIoCount;		// >= MAX_IO_FOR_TRACKING_TO_SMART_REFRESH than we change the state
	ULONG		MaxTransferUnit;		// DEFAULT_MAX_TRANSFER_UNIT_SIZE, Maximum Raw Data Size which gets transfered to secondary.

	ULONG		NumOfAsyncRefreshIO;	// DEFAULT_NUM_OF_ASYNC_REFRESH_IO, Maximum Number of Refresh Async Read IO allowed

	LONGLONG	AckWakeupTimeout;		// DEFAULT_TIMEOUT_FOR_ACK_THREAD, Timeout used to wake up Ack thread and prepare its LRDB or HRDB
										// Values is in 100 nanoseconds relative to system time means values always in nagetive....		
	LONGLONG	RefreshThreadWakeupTimeout;	// DEFAULT_TIMEOUT_FOR_REFRESH_THREAD, Timeout used to sleep in refresh thread to wait for multi-events

	ULONG		sync_depth;				// Not used,... For future Usage
	ULONG		sync_timeout;			// Not used,... For future Usage
	ULONG		iodelay;				// Not used,... For future Usage

	// Veera, Add Session manager configurable tuning param fields here
	ULONG		throtal_refresh_send_pkts;		// DEFAULT_ALLOWED_NUM_OF_PKTS_PENDING_FOR_REFRESH
	ULONG		throtal_refresh_send_totalsize;	// DEFAULT_ALLOWED_MAX_SIZE_PENDING_FOR_REFRESH
	ULONG		throtal_commit_send_pkts;		// DEFAULT_ALLOWED_NUM_OF_PKTS_PENDING_FOR_COMMIT
	ULONG		throtal_commit_send_totalsize;	// DEFAULT_ALLOWED_MAX_SIZE_PENDING_FOR_COMMIT

	ULONG		NumOfPktsSendAtaTime;			// This many pkts will get send at a time, DEFAULT_NUM_OF_PKTS_SEND_AT_A_TIME
	ULONG		NumOfPktsRecvAtaTime;			// This many pkts will get Recv at a time, DEFAULT_NUM_OF_PKTS_RECV_AT_A_TIME
	ULONG		NumOfSendBuffers;				// This many send Buffers are used when we send the TDI Buffers

	// for debugging only
	ULONG		DebugLevel;	// DEFAULT_ALLOWED_MAX_SIZE_PENDING_FOR_COMMIT

	// Global Tuning params for all LG....

}TUNING_PARAM, *PTUNING_PARAM;

//
// Param Flag Definations, uses bit setting to specify multiple values settings...
//
#define LG_PARAM_FLAG_USE_TrackingIoCount				0x00000001
#define	LG_PARAM_FLAG_USE_MaxTransferUnit				0x00000002
#define LG_PARAM_FLAG_USE_NumOfAsyncRefreshIO			0x00000004
#define LG_PARAM_FLAG_USE_AckWakeupTimeout				0x00000008
#define LG_PARAM_FLAG_USE_sync_depth					0x00000010
#define LG_PARAM_FLAG_USE_sync_timeout					0x00000020
#define LG_PARAM_FLAG_USE_iodelay						0x00000040
#define LG_PARAM_FLAG_THROTAL_REFRESH_SEND_PKTS			0x00000080
#define LG_PARAM_FLAG_THROTAL_REFRESH_SEND_TOTALSIZE	0x00000100
#define LG_PARAM_FLAG_THROTAL_COMMIT_SEND_PKTS			0x00000200
#define LG_PARAM_FLAG_THROTAL_COMMIT_SEND_TOTALSIZE		0x00000400
#define LG_PARAM_FLAG_USE_RefreshThreadWakeupTimeout	0x00000800
#define LG_PARAM_FLAG_DebugLevel						0x00001000
#define LG_PARAM_FLAG_NumOfPktsSendAtaTime				0x00002000
#define LG_PARAM_FLAG_NumOfPktsRecvAtaTime				0x00004000
#define LG_PARAM_FLAG_NumOfSendBuffers					0x00008000


typedef struct LG_PARAM
{
	ULONG			LgNum;			// stat_buffer_t->lg_num: LG Num
#if TARGET_SIDE
	ROLE_TYPE		lgCreationRole;	// Passes Lg Creation role information
#endif
	ULONG			ParamFlags;		// LG_PARAM_FLAG_USE_TrackingIoCount: This Flag indicates which param fields has valid values asked to use to set in driver 

	TUNING_PARAM	Param;			// Tuning Param values 

} LG_PARAM, *PLG_PARAM;


//
// -------- Following are new Structures defined for Get Attached DiskInfo  IOCTL ------
//

typedef struct ATTACHED_DISK_INFO
{
	// Disk number for reference in WMI
    ULONG			DiskNumber;

	// Physical Device name or WMI Instance Name
    WCHAR			PhysicalDeviceNameBuffer[256];

	BOOLEAN			bValidName;	// TRUE means all follwing BOOLEAN values are TRUE

	//		IOCTL_STORAGE_GET_DEVICE_NUMBER to retrieve this info, 
	// -	if disk is PhysDisk type then IOCTL_STORAGE_GET_DEVICE_NUMBER succeeds and 
	//		we store name string \\Device\HardDisk(n)\\Partition(n) in DiskPartitionName
	// -	if disk is not PhyDisk then IOCTL_STORAGE_GET_DEVICE_NUMBER fails
	//		We use IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, to get device name string and that string 
	//		we store in DiskVolumeName.
	//		It also uses IOCTL_VOLUME_QUERY_VOLUME_NUMBER to get VOLUME_NUMBER information

    // Use to keep track of Volume info from ntddvol.h
    WCHAR			StorageManagerName[8];		// L"PhysDisk", L"LogiDisk" else the value from VOLUME_NUMBER
														// PhyDisk means "\\Device\\Harddisk%d\\Partition%d",
														// LogiDisk means "\\Device\HarDiskVolume%d

	// IOCTL_STORAGE_GET_DEVICE_NUMBER to retrieve this info, Example : \\Device\HardDisk(n)\\Partition(n)
	// if disk is PhysDisk type then we get following info successfully.
	BOOLEAN			bStorage_device_Number;		// TRUE means Storage_device_Number is valid
	ULONG			DeviceType;
	ULONG			DeviceNumber;
	ULONG			PartitionNumber;
// 	STORAGE_DEVICE_NUMBER	StorageDeviceNumber;		// values retrieved from IOCTL_STORAGE_GET_DEVICE_NUMBER
	WCHAR			DiskPartitionName[128];		// Stores \\Device\HardDisk(n)\\Partition(n)

	// IOCTL_VOLUME_QUERY_VOLUME_NUMBER to retrieve HarddiskVolume number and its volumename like logdisk, etc..
	// if disk is LogiDisk type then we get following info successfully.
	BOOLEAN			bVolumeNumber;			// TRUE means VolumeNumber is valid
	ULONG			VolumeNumber;
    WCHAR			VolumeManagerName[8];
// 	VOLUME_NUMBER	VolumeNumber;			// IOCTL_VOLUME_QUERY_VOLUME_NUMBER
	
	// IOCTL_MOUNTDEV_QUERY_DEVICE_NAME used to retrieve \Device\HarddiskVolume1 into DiskVolumeName...
	BOOLEAN			bDiskVolumeName;		// TRUE means DiskVolumeName has valid value
	WCHAR			DiskVolumeName[128];	// IOCTL_MOUNTDEV_QUERY_DEVICE_NAME returns string in DiskVolumeName	
													// Example : \Device\HarddiskVolume1 or \DosDevices\D or "\DosDevices\E:\FilesysD\mnt
	// following fields are supported only >= Win2k OS 
	// IOCTL_MOUNTDEV_QUERY_UNIQUE_ID used to retrieve Volume/Disk Unique Persistence ID, 
	// It Contains the unique volume ID. The format for unique volume names is "\??\Volume{GUID}\", 
	// where GUID is a globally unique identifier that identifies the volume.
	BOOLEAN			bUniqueVolumeId;	// TRUE means UniqueIdInfo Fields values are valid
	USHORT			UniqueIdLength;
	UCHAR			UniqueId[256];
	// PMOUNTDEV_UNIQUE_ID				UniqueIdInfo;		// Example: For instance, drive letter "D" must be represented in this manner: "\DosDevices\D:". 
	
	// following fields are supported only >= Win2k OS 
	// IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME is Optional returns Drive letter (if Drive Letter is persistent across boot) or
	// suggest Drive Letter Dos Sym,bolic Name.
	BOOLEAN			bSuggestedDriveLetter;	// TRUE means All Following Fields values are valid
	BOOLEAN			UseOnlyIfThereAreNoOtherLinks;
    USHORT			NameLength;
//	PMOUNTDEV_SUGGESTED_LINK_NAME	SuggestedDriveLinkName;	// Example: For instance, drive letter "D" must be represented in this manner: "\DosDevices\D:". 
	WCHAR			SuggestedDriveName[256];

	// Store Customize Signature GUID which gets used as alternate Volume GUID only if bUniqueVolumeID is not valid
	// Format is "volume(nnnnnnnn-nnnnnnnnnnnnnnnn-nnnnnnnnnnnnnnnn)"	: volume(Disksignature-StartingOffset-SizeInBytes)
	// Our customize alternate Disk Signature based Unique ID for Volume (Raw Disk/ Disk Partition)
	BOOLEAN			bSignatureUniqueVolumeId;	// TRUE means SignatureUniqueIdLength and SignatureUniqueId has valid values
	USHORT			SignatureUniqueIdLength;
    UCHAR			SignatureUniqueId[128];			// 128 is enough, if requires bump up this value.

	BOOLEAN			IsVolumeFtVolume; // TRUE means StorageManagerName == FTDISK && NumberOfDiskExtents > 0
	PVOID			pRawDiskDevice;	// Pointer to RAW Disk device of current partition or disk object

	ULONG			Signature;			// signature
	LARGE_INTEGER   StartingOffset;		// Starting Offset of Partition, if its RAW Disk than value is 0 
    LARGE_INTEGER   PartitionLength;	// Size of partition, if its RAW Disk than value i

	PVOID			SftkDev;	// if attached its non null value
	ULONG			cdev;
	ULONG			LGNum;
	
} ATTACHED_DISK_INFO, *PATTACHED_DISK_INFO;

// DISK_DEVICE_INFO structure will be used per LG 
typedef struct ATTACHED_DISK_INFO_LIST
{
	ULONG				NumOfDisks;
	ATTACHED_DISK_INFO	DiskInfo[1];

} ATTACHED_DISK_INFO_LIST, *PATTACHED_DISK_INFO_LIST;
//
//	ULONG 
//	MAX_SIZE_ATTACH_DISK_INFOLIST( ULONG _TotalDisk_)
// 
// This API return total size of struct ATTACHED_DISK_INFO_LIST required for FTD_GET_All_ATTACHED_DISKINFO
#define MAX_SIZE_ATTACH_DISK_INFOLIST(_TotalDisk_)		\
				(	sizeof(ATTACHED_DISK_INFO_LIST) + (sizeof(ATTACHED_DISK_INFO) * (_TotalDisk_) ) )

// DTurrin - Nov 7th, 2001
// If you are using Windows 2000 or later,
// you need to add the Global\ prefix so that
// client session will use kernel objects in
// the server's global name space.
// #define NTFOUR
#define SFTK_CTL_DIR_NAME		"\\SFTK"
#define SFTK_CTL_DIR_UNI_NAME	L"\\SFTK"
#define SFTK_CTL_NAME			"\\ctl"
#define SFTK_CTL_UNI_NAME		L"\\ctl"

// For user mode application, use SFTK_CTL_USER_MODE_DEVICE_NAME macro to open device...
#ifdef NTFOUR
#define SFTK_CTL_USER_MODE_DEVICE_NAME	"\\\\." SFTK_CTL_DIR_NAME SFTK_CTL_NAME
#else 
#define SFTK_CTL_USER_MODE_DEVICE_NAME	"\\\\.\\Global" SFTK_CTL_DIR_NAME SFTK_CTL_NAME
#endif 

// MAX_REMOTE_CLIENT_SUPPORT is used to define maximum number of remote
// secondary clients support for Replications.
#define MAX_REMOTE_CLIENT_SUPPORT	2

// Device Object naming convetions and other macro definations
// some constant definitions, NodeID values are one of followings:
#define     FTD_PANIC_IDENTIFIER        (0x86427532)

#define     FTD_DRV_DIR                 L"\\Device\\Sftk"
#define     FTD_DOS_DRV_DIR             L"\\DosDevices\\Sftk"
#define     FTD_DRV_DIR_CTL_NAME		L"\\Device\\Sftk\\ctl"
#define		FTD_DOS_DRV_DIR_CTL_NAME	L"\\DosDevices\\Sftk\\ctl"
#define		FTD_DRV_DIR_LG_NAME			L"\\Device\\Sftk\\lg"		// Appened with Logical Group Number
#define		FTD_DOS_DRV_DIR_LG_NAME		L"\\DosDevices\\Sftk\\lg" // Appened with Logical Group Number

/*
#define     FTD_DRV_DIR                 L"\\Device" SFTK_CTL_DIR_UNI_NAME
#define     FTD_DOS_DRV_DIR             L"\\DosDevices" SFTK_CTL_DIR_UNI_NAME
#define     FTD_DRV_DIR_CTL_NAME		L"\\Device" SFTK_CTL_DIR_UNI_NAME SFTK_CTL_UNI_NAME
#define		FTD_DOS_DRV_DIR_CTL_NAME	L"\\DosDevices" SFTK_CTL_DIR_UNI_NAME SFTK_CTL_UNI_NAME
#define		FTD_DRV_DIR_LG_NAME			L"\\Device" SFTK_CTL_DIR_UNI_NAME L"\\lg"		// Appened with Logical Group Number
#define		FTD_DOS_DRV_DIR_LG_NAME		L"\\DosDevices" SFTK_CTL_DIR_UNI_NAME L"\\lg" // Appened with Logical Group Number
*/
#define     FTD_LG_NUM_EVENT_NAME		L"\\BaseNamedObjects\\DTClg"	// Appened with Logical Group Number + FTD_LG_EVENT_NAME
#define     FTD_LG_EVENT_NAME           L"bab"

#define     FTD_CTL_NAME                L"ctl"
#define     FTD_LG_NAME                 L"lg"


#define     REGISTRY_NAME_BREAKONENTRY      L"BreakOnEntry"
#define     REGISTRY_NAME_DEBUGLEVEL        L"DebugLevel"
#define     REGISTRY_NAME_CHUNK_SIZE        L"chunk_size"
#define     REGISTRY_NAME_NUM_CHUNKS        L"num_chunks"

#define     REGISTRY_NAME_MAXMEM            L"maxmem"

/* the first stab at logical group states */
#define FTD_M_JNLUPDATE 0x01
#define FTD_M_BITUPDATE 0x02

#define FTD_MODE_PASSTHRU		0x10
#define FTD_MODE_TRACKING		FTD_M_BITUPDATE						// Only Bitmap Update 
#define FTD_MODE_NORMAL			FTD_M_JNLUPDATE						// Journal (BAB) update, No Bitmap update

#define FTD_MODE_REFRESH		(FTD_M_JNLUPDATE | FTD_M_BITUPDATE)	// Smart Refresh
#define FTD_MODE_FULLREFRESH    (0x40 | FTD_M_BITUPDATE)    //used only in Unix

#define FTD_MODE_BACKFRESH 0x20

#if 1 // PARAG_ADDED
#define FTD_MODE_FULL_REFRESH	FTD_MODE_FULLREFRESH 					// No Journal (BAB) update
#define FTD_MODE_SMART_REFRESH	FTD_MODE_REFRESH					// Smart Refresh mode
#endif

#if 0 // New Mode definations from ET team.
/*
// Followin All mode will do LRDB Bitmap update except FTD_MODE_PASSTHRU and FTD_MODE_BACKFRESH mode.
// Following all mode will not do HRDB update except FTD_MODE_TRACKING mode. 
// HRDB bitmap can recollect at any time from BAB queues traversing logic.
// Smart refresh always used HRDB (created from BAB queues traversing logic or Tracking mode)

// TODO : Trying to keep value same as previouse code mode definations causes Service and GUI has used
// same macro and values definations in their code. Please fix this every places to make meanging ful meaning 
// in GUI, Service, Collector and Driver.....

// SFTK_MODE_BAB_UPDATE - For new incoming write allocate BAB and pass it to remote system
#define SFTK_M_BAB_UPDATE				0x01			// = FTD_M_JNLUPDATE 0x01
// SFTK_MODE_HRDB_UPDATE - Update HRDB bitmap.
#define SFTK_M_HRDB_UPDATE				0x02			// = FTD_M_BITUPDATE 0x02

// SFTK_MODE_HRDB_UPDATE - Update JRNL/BAB and send it to remote.
#define SFTK_M_SEND_BABDATA_TO_REMOTE	0x04			// = NEW Definations, TODO : update GUI/Service/Collector


// SFTK_MODE_PASSTHRU - Driver is doing nothing its just passthru device mode 
// = 0x10
#define SFTK_MODE_PASSTHRU			0x10				// = FTD_MODE_PASSTHRU		0x10

// SFTK_MODE_NORMAL - LRDB Update: YES , HRDB Update - NO, Journal (BAB/Cache) Update - YES 
// = 0x01 | 0x04 = 0x5
#define SFTK_MODE_NORMAL			(SFTK_M_BAB_UPDATE | SFTK_M_SEND_BABDATA_TO_REMOTE)	// = #define FTD_MODE_NORMAL FTD_M_JNLUPDATE

// SFTK_MODE_TRACKING - LRDB Update: YES , HRDB Update - YES, Journal (BAB/Cache) Update - NO
// = 0x02 = 0x2
#define SFTK_MODE_TRACKING			SFTK_M_HRDB_UPDATE	// = #define FTD_MODE_TRACKING FTD_M_BITUPDATE

// SFTK_MODE_FULL_REFRESH - LRDB Update: YES , HRDB Update - YES, Journal (BAB/Cache) Update - NO
// = 0x40 | 0x2 = 0x42
#define SFTK_MODE_FULL_REFRESH		(0x40 | SFTK_MODE_TRACKING)		// = FTD_MODE_FULLREFRESH = (0x40 | FTD_M_BITUPDATE)    

// SFTK_MODE_SMART_REFRESH - LRDB Update: YES , HRDB Update - NO, Journal (BAB/Cache) Update - YES, But not sending to remote
// = 0x1 | 0x2 = 0x3
#define SFTK_MODE_SMART_REFRESH		(SFTK_M_BAB_UPDATE | SFTK_M_HRDB_UPDATE)		// FTD_MODE_REFRESH = (FTD_M_JNLUPDATE | FTD_M_BITUPDATE)

// SFTK_MODE_BACKFRESH - LRDB Update - NO, HRDB Update - NO, BAB Update - NO
// = x20
#define SFTK_MODE_BACKFRESH			0x20
*/
#else

// Same as previouse driver definations
#define SFTK_MODE_PASSTHRU			FTD_MODE_PASSTHRU		// 0x10

#define SFTK_MODE_TRACKING			FTD_MODE_TRACKING		// FTD_M_BITUPDATE			// Only Bitmap Update 
#define SFTK_MODE_NORMAL			FTD_MODE_NORMAL			// FTD_M_JNLUPDATE			// Journal (BAB) update, No Bitmap update

#define SFTK_MODE_FULL_REFRESH		FTD_MODE_FULL_REFRESH    // FTD_MODE_FULLREFRESH  = (0x40 | FTD_M_BITUPDATE)    //used only in Unix
#define SFTK_MODE_SMART_REFRESH		FTD_MODE_SMART_REFRESH	// FTD_MODE_REFRESH = (FTD_M_JNLUPDATE | FTD_M_BITUPDATE)	// Smart Refresh

#define SFTK_MODE_BACKFRESH			FTD_MODE_BACKFRESH		// 0x20

#endif


#define FTD_LGFLAG              ((MAXMIN + 1) >> 1)

#define FTD_CTL                 MAXMIN

/* calculate the size of "x" in 64-bit words */
#define sizeof_64bit(x) ((sizeof(x) + 7) >> 3)

#define FTD_WLH_QUADS(hp)       (sizeof_64bit(wlheader_t) + \
                                ((hp)->length << (DEV_BSHIFT - 3)))
#define FTD_WLH_BYTES(hp)       FTD_WLH_QUADS(hp) << 3;

// The functions below are used to convert a given ULONG IPAddress to its String Equivalent 
// The Addess is inBig Endian (Network Byte Order) and the Port is also in Network Byte order
// So to Dispaly it on Windows we swap the respective bytes

// Wide char Functions

#define SM_CONVERT_IP_PORT_TO_STRING_W( _wstring_ , _uRemoteIP_ , _uRemotePort_ )							\
				swprintf(_wstring_ ,																	\
						L"%d.%d.%d.%d :%d",															\
						(BYTE)(	( _uRemoteIP_		)	& 0x000000FF),									\
						(BYTE)(	( _uRemoteIP_ >> 8	)	& 0x000000FF),									\
						(BYTE)(	( _uRemoteIP_ >> 16 )	& 0x000000FF),									\
						(BYTE)(	( _uRemoteIP_ >> 24 )	& 0x000000FF),									\
						(USHORT)( ( _uRemotePort_ << 8 ) & 0xFF00 | ( _uRemotePort_ >> 8 ) & 0x00FF )	\
				)

#define SM_CONVERT_IP_TO_STRING_W( _wstring_ , _uRemoteIP_ )												\
				swprintf(_wstring_ ,																	\
						L"%d.%d.%d.%d",																	\
						(BYTE)(	( _uRemoteIP_		)	& 0x000000FF),									\
						(BYTE)(	( _uRemoteIP_ >> 8	)	& 0x000000FF),									\
						(BYTE)(	( _uRemoteIP_ >> 16 )	& 0x000000FF),									\
						(BYTE)(	( _uRemoteIP_ >> 24 )	& 0x000000FF)									\
				)

#define SM_CONVERT_PORT_TO_STRING_W( _wstring_ , _uRemotePort_ )												\
				swprintf(_wstring_ ,																		\
						L"%d",																				\
						(USHORT)( ( _uRemotePort_ << 8 ) & 0xFF00 | ( _uRemotePort_ >> 8 ) & 0x00FF )		\
				)

// CHAR Functions
#define SM_CONVERT_IP_PORT_TO_STRING( _string_ , _uRemoteIP_ , _uRemotePort_ )							\
				sprintf(_string_ ,																		\
						"%d.%d.%d.%d :%d",																\
						(BYTE)(	( _uRemoteIP_		)	& 0x000000FF),									\
						(BYTE)(	( _uRemoteIP_ >> 8	)	& 0x000000FF),									\
						(BYTE)(	( _uRemoteIP_ >> 16 )	& 0x000000FF),									\
						(BYTE)(	( _uRemoteIP_ >> 24 )	& 0x000000FF),									\
						(USHORT)( ( _uRemotePort_ << 8 ) & 0xFF00 | ( _uRemotePort_ >> 8 ) & 0x00FF )	\
				)

#define SM_CONVERT_IP_TO_STRING( _string_ , _uRemoteIP_ )												\
				sprintf(_string_ ,																		\
						"%d.%d.%d.%d",																	\
						(BYTE)(	( _uRemoteIP_		)	& 0x000000FF),									\
						(BYTE)(	( _uRemoteIP_ >> 8	)	& 0x000000FF),									\
						(BYTE)(	( _uRemoteIP_ >> 16 )	& 0x000000FF),									\
						(BYTE)(	( _uRemoteIP_ >> 24 )	& 0x000000FF)									\
				)

#define SM_CONVERT_PORT_TO_STRING( _string_ , _uRemotePort_ )												\
				sprintf(_string_ ,																			\
						"%d",																				\
						(USHORT)( ( _uRemotePort_ << 8 ) & 0xFF00 | ( _uRemotePort_ >> 8 ) & 0x00FF )		\
				)


//TDI_CODE
//From here its the TDI code from Veera, It contains all the definitions 
//and IOCTLS that are required

#ifdef USER_SPACE // USER_SPACE

typedef UNALIGNED struct _TA_ADDRESS {
    USHORT AddressLength;       // length in bytes of Address[] in this
    USHORT AddressType;         // type of this address
    UCHAR Address[1];           // actually AddressLength bytes long
} TA_ADDRESS, *PTA_ADDRESS;

typedef struct _TRANSPORT_ADDRESS {
    LONG TAAddressCount;            // number of addresses following
    TA_ADDRESS Address[1];          // actually TAAddressCount elements long
} TRANSPORT_ADDRESS, *PTRANSPORT_ADDRESS;

//
// IP address
//

typedef struct _TDI_ADDRESS_IP {
    USHORT sin_port;
    ULONG  in_addr;
    UCHAR  sin_zero[8];
} TDI_ADDRESS_IP, *PTDI_ADDRESS_IP;

#define TDI_ADDRESS_LENGTH_IP sizeof (TDI_ADDRESS_IP)

//
// connection primitives information structure. This is passed to the TDI calls
// (Accept, Connect, xxx) that do connecting sorts of things.
//

typedef struct _TDI_CONNECTION_INFORMATION {
    LONG UserDataLength;        // length of user data buffer
    PVOID UserData;             // pointer to user data buffer
    LONG OptionsLength;         // length of follwoing buffer
    PVOID Options;              // pointer to buffer containing options
    LONG RemoteAddressLength;   // length of following buffer
    PVOID RemoteAddress;        // buffer containing the remote address
} TDI_CONNECTION_INFORMATION, *PTDI_CONNECTION_INFORMATION;

typedef struct _TDI_CONNECTION_INFO {
    ULONG State;                        // current state of the connection.
    ULONG Event;                        // last event on the connection.
    ULONG TransmittedTsdus;             // TSDUs sent on this connection.
    ULONG ReceivedTsdus;                // TSDUs received on this connection.
    ULONG TransmissionErrors;           // TSDUs transmitted in error/this connection.
    ULONG ReceiveErrors;                // TSDUs received in error/this connection.
    LARGE_INTEGER Throughput;           // estimated throughput on this connection.
    LARGE_INTEGER Delay;                // estimated delay on this connection.
    ULONG SendBufferSize;               // size of buffer for sends - only
                                        // meaningful for internal buffering
                                        // protocols like tcp
    ULONG ReceiveBufferSize;            // size of buffer for receives - only
                                        // meaningful for internal buffering
                                        // protocols like tcp
    BOOLEAN Unreliable;                 // is this connection "unreliable".
} TDI_CONNECTION_INFO, *PTDI_CONNECTION_INFO;

typedef struct _TDI_PROVIDER_INFO {
    ULONG Version;                      // TDI version: 0xaabb, aa=major, bb=minor
    ULONG MaxSendSize;                  // max size of user send.
    ULONG MaxConnectionUserData;        // max size of user-specified connect data.
    ULONG MaxDatagramSize;              // max datagram length in bytes.
    ULONG ServiceFlags;                 // service options, defined below.
    ULONG MinimumLookaheadData;         // guaranteed min size of lookahead data.
    ULONG MaximumLookaheadData;         // maximum size of lookahead data.
    ULONG NumberOfResources;            // how many TDI_RESOURCE_STATS for provider.
    LARGE_INTEGER StartTime;            // when the provider became active.
} TDI_PROVIDER_INFO, *PTDI_PROVIDER_INFO;

typedef struct _TDI_PROVIDER_RESOURCE_STATS {
    ULONG ResourceId;                   // identifies resource in question.
    ULONG MaximumResourceUsed;          // maximum number in use at once.
    ULONG AverageResourceUsed;          // average number in use.
    ULONG ResourceExhausted;            // number of times resource not available.
} TDI_PROVIDER_RESOURCE_STATS, *PTDI_PROVIDER_RESOURCE_STATS;

typedef struct _TDI_PROVIDER_STATISTICS {
    ULONG Version;                      // TDI version: 0xaabb, aa=major, bb=minor
    ULONG OpenConnections;              // currently active connections.
    ULONG ConnectionsAfterNoRetry;      // successful connections, no retries.
    ULONG ConnectionsAfterRetry;        // successful connections after retry.
    ULONG LocalDisconnects;             // connection disconnected locally.
    ULONG RemoteDisconnects;            // connection disconnected by remote.
    ULONG LinkFailures;                 // connections dropped, link failure.
    ULONG AdapterFailures;              // connections dropped, adapter failure.
    ULONG SessionTimeouts;              // connections dropped, session timeout.
    ULONG CancelledConnections;         // connect attempts cancelled.
    ULONG RemoteResourceFailures;       // connections failed, remote resource problems.
    ULONG LocalResourceFailures;        // connections failed, local resource problems.
    ULONG NotFoundFailures;             // connections failed, remote not found.
    ULONG NoListenFailures;             // connections rejected, we had no listens.
    ULONG DatagramsSent;
    LARGE_INTEGER DatagramBytesSent;
    ULONG DatagramsReceived;
    LARGE_INTEGER DatagramBytesReceived;
    ULONG PacketsSent;                  // total packets given to NDIS.
    ULONG PacketsReceived;              // total packets received from NDIS.
    ULONG DataFramesSent;
    LARGE_INTEGER DataFrameBytesSent;
    ULONG DataFramesReceived;
    LARGE_INTEGER DataFrameBytesReceived;
    ULONG DataFramesResent;
    LARGE_INTEGER DataFrameBytesResent;
    ULONG DataFramesRejected;
    LARGE_INTEGER DataFrameBytesRejected;
    ULONG ResponseTimerExpirations;     // e.g. T1 for Netbios
    ULONG AckTimerExpirations;          // e.g. T2 for Netbios
    ULONG MaximumSendWindow;            // in bytes
    ULONG AverageSendWindow;            // in bytes
    ULONG PiggybackAckQueued;           // attempts to wait to piggyback ack.
    ULONG PiggybackAckTimeouts;         // times that wait timed out.
    LARGE_INTEGER WastedPacketSpace;    // total amount of "wasted" packet space.
    ULONG WastedSpacePackets;           // how many packets contributed to that.
    ULONG NumberOfResources;            // how many TDI_RESOURCE_STATS follow.
    TDI_PROVIDER_RESOURCE_STATS ResourceStats[1];    // variable array of them.
} TDI_PROVIDER_STATISTICS, *PTDI_PROVIDER_STATISTICS;
#endif // USER_SPACE

#if 1 // _PROTO_TDI_CODE_

// I am just defining Maximum IO Size for the Time Being, will have to discuss with Parag 
// and fix it accordingly

#define SFTK_MAX_IO_SIZE	262144	// 256 KB is the Maximum IO Size that is allowed, if any IO comes
									// more than 256KB it needs to be converted to Associated IRPs less
									// in size than 256KB

//This Structure is passed to the Client
//Client Spawns one Worker Thread per Socket and then calls the Cache Manager to Give Data
//If No Data is available the Worker Threads Just Wait and Sleep
//The Therads will wakeup either when there is some data in the cache
//OR the User issues a Kill Operation stopping all operations.
//The Worker Thread Calls the TDI_SEND once there is data in the Cache.
//All the Worker Threads are locked using a FAST_MUTEX until an Event is signalled 
//for the data in the Cache.
typedef struct _CONNECTION_INFO
{
	USHORT			nNumberOfSessions;
	TDI_ADDRESS_IP	ipLocalAddress;
	TDI_ADDRESS_IP	ipRemoteAddress;
}CONNECTION_INFO, *PCONNECTION_INFO;

//This Structure is used to ( Add | Enable | Disable | Remove ) Connections to the System
typedef struct _CONNECTION_DETAILS
{
	int			lgnum;					// Logical Group Number 

#if TARGET_SIDE
	ROLE_TYPE    lgCreationRole;	// Passes Lg Creation role information
#endif

//	LONG	nSendWindowSize;		//This is the Window Size that will be used per Socket Connection in MB
//	LONG	nReceiveWindowSize;
	USHORT	nConnections;
	CONNECTION_INFO ConnectionDetails[1];
}CONNECTION_DETAILS, *PCONNECTION_DETAILS;

//These Tunables will be used to change the Properties of the Session Manager
typedef enum {eSendWindowSize = 1 , eReceiveWindowSize , eChunkSize , eThrottle} eConnectionTunables;
//Add Tunable Parameters into thsi Structure that will be used to set the 
typedef struct _SM_TUNABLES
{
	int     lgnum;					// Logical Group Number 

	eConnectionTunables eType;		// Specifies the type of the value that needs to be modified

	union
	{
		ULONG nSendWindowSize;		// The Size of the TDI Send Window 
		ULONG nReceiveWindowSize;	// The Size of the TDI Receive Window
									// This is a test parameter, specifies the Send Chunks
									// By default this will be Zero.
		ULONG nChunkSize;
		ULONG nChunkDelay;			// The time in milliseconds that will be used to Throttle,
									// data sent over the wire.
	};
}SM_TUNABLES , *PSM_TUNABLES;


//This will be sent along with the LAUNCH_PMD as the initilization sequence
typedef struct _SM_INIT_PARAMS_
{
	int     lgnum;					// Logical Group Number 

#if TARGET_SIDE
	ROLE_TYPE    lgCreationRole;	// Passes Lg Creation role information
#endif

	ULONG nSendWindowSize;			// The Size of the TDI Send Window 

	USHORT nMaxNumberOfSendBuffers;	// The Max Number of Send Buffers that needs to be allocated

	ULONG nReceiveWindowSize;		// The Size of the TDI Receive Window

	USHORT nMaxNumberOfReceiveBuffers;	// The Max Number of Receive Buffers that needs to be allocated

	ULONG nChunkSize;				// This is a test parameter, specifies the Send Chunks
									// By default this will be Zero.

	ULONG nChunkDelay;				// The time in milliseconds that will be used to Throttle,
									// data sent over the wire.

}SM_INIT_PARAMS , *PSM_INIT_PARAMS;


//This structure contains the Performnace Metrics for one Connection
typedef struct _CONNECTION_PERFORMANCE_INFO
{
	TDI_ADDRESS_IP ipLocalAddress;
	TDI_ADDRESS_IP ipRemoteAddress;
	TDI_CONNECTION_INFO connectionInfo;
}CONNECTION_PERFORMANCE_INFO , *PCONNECTION_PERFORMANCE_INFO;

//This Structure contains the Preformance metrics for all the Connections in a Session Manager
typedef struct _SM_PERFORMANCE_INFO
{
	int							lgnum;					// Logical Group Number 

	TDI_PROVIDER_INFO			providerInfo;			// Provides the Falgs that are supported by the TCP

	TDI_PROVIDER_STATISTICS		providerStatistics;		// Provides Live Statistics that are supported 
														// by TCP

	LONG						nConnections;			// Number of Connection Performance Information Structures

	CONNECTION_PERFORMANCE_INFO conPerformanceInfo[1];	// The Connection Performance Array

}SM_PERFORMANCE_INFO , *PSM_PERFORMANCE_INFO;
#endif //_TDI_CODE_

#pragma pack(pop)

#endif /* FTDIO_H */
