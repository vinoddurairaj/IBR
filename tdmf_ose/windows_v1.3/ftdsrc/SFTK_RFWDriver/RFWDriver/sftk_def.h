/**************************************************************************************

Module Name: sftk_def.h   
Author Name: Parag sanghvi
Description: Define all Private Structures used in drivers
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#ifndef _SFTK_DEF_H_
#define _SFTK_DEF_H_

#pragma pack(push,1)

// Predeclaration of external defined structure 
struct _DEVICE_EXTENSION;	// defined in Sftk_Nt.h file

/**************************************************************************
    each structure has a unique "node type" or signature associated with it
**************************************************************************/
#define NODE_TYPE_SFTK_CONFIG			(0xfd10)		// Global Structure used struct SFTK_CONFIG
#define NODE_TYPE_SFTK_CTL				(0xfd11)		// CTL Device Extension used struct SFTK_CTL_DEV_EXT
#define NODE_TYPE_SFTK_LG				(0xfd12)		// LG Device Extension used struct SFTK_LG
#define NODE_TYPE_SFTK_DEV				(0xfd13)		// Replication Src Device Information used struct SFTK_DEV
#define NODE_TYPE_FILTER_DEV			(0xfd14)		// Disk Filter Device Extention used struct DEVICE_EXTENSION

// SFTK_CONFIG->Flags definations
#define SFTK_CONFIG_FLAG_SYMLINK_CREATED	(0x0001)
#define SFTK_CONFIG_FLAG_CACHE_CREATED		(0x0002)
#define SFTK_CONFIG_FLAG_LGIO_CREATED		(0x0004)

/* // OLD Code --- Not used anymore....
#define FTD_LG_OPEN             (0x00000001)
#define FTD_LG_COLLISION        (0x00000002)
#define FTD_LG_SYMLINK_CREATED  (0x00000004)
#define FTD_LG_EVENT_CREATED    (0x00000008)
#define FTD_DIRTY_LAG			1024
*/
/* we won't attempt to reset dirtybits when bab is more than this
   number of sectors full. (Walking the bab is expensive) */
#define FTD_RESET_MAX  (1024 * 20)

// SFTK_LG->Flags definations
#define SFTK_LG_FLAG_CACHE_INITIALIZED					(0x00000001)
#define SFTK_LG_FLAG_SOCKET_INITIALIZED					(0x00000002)
#define SFTK_LG_FLAG_SYMLINK_INITIALIZED				(0x00000004)
#define SFTK_LG_FLAG_ADDED_TO_LGDEVICE_ANCHOR			(0x00000008)
#define SFTK_LG_FLAG_STARTED_LG_THREADS					(0x00000010)
#define SFTK_LG_FLAG_OPENED_EXCLUSIVE					(0x00000020)	// same as FTD_LG_OPEN
#define SFTK_LG_FLAG_NAMED_EVENT_CREATED				(0x00000040)	// same as FTD_LG_EVENT_CREATED
#define SFTK_LG_FLAG_QUEUE_MANAGER_INIT					(0x00000080)	
#define SFTK_LG_FLAG_REG_CREATED						(0x00000100)	
#define SFTK_LG_FLAG_PSTORE_FILE_CREATED				(0x00000200)	
#define SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE			(0x00000400)	
#define SFTK_LG_FLAG_NEW_PSTORE_FILE_FORMATTED			(0x00000800)	
#define SFTK_LG_FLAG_AFTER_BOOT_RESUME_FULL_REFRESH		(0x00001000)	

// SFTK_DEV->Flags definations
#define SFTK_DEV_FLAG_SRC_DEVICE_ONLINE			(0x00000001)	// set means backend Src device is online
#define SFTK_DEV_FLAG_MASTER_THREAD_STARTED		(0x00000002)	// set means device Pstore thread is started
#define SFTK_DEV_FLAG_PSTORE_FILE_ADDED			(0x00000004)	
#define SFTK_DEV_FLAG_REG_CREATED				(0x00000008)	


//Sync Depth Default Values...
#define	SFTK_LG_SYNCH_DEPTH_DEFAULT			(-1)

#define FTD_DEV_OPEN            (0x1)
#define FTD_STOP_IN_PROGRESS    (0x2)
#define FTD_DEV_ATTACHED        (0x4)

/* A HRDB bit should address no less than a sector */
#define MINIMUM_HRDB_BITSIZE      DEV_BSHIFT

/* A LRDB bit should address no less than 256K */
#define MINIMUM_LRDB_BITSIZE      18

/* for a couple of functions that work on both dirtybit arrays */
#define FTD_HIGH_RES_DIRTYBITS 0
#define FTD_LOW_RES_DIRTYBITS  1

//
// Following code is not used at all, just to keep definations, we must remove this definations too.
//
#if 1
/* stolen from Sun sysmacros.h */

#define	O_BITSMAJOR	7	/* # of SVR3 major device bits */
#define	O_BITSMINOR	8	/* # of SVR3 minor device bits */
#define	O_MAXMAJ	0x7f	/* SVR3 max major value */
#define	O_MAXMIN	0xff	/* SVR3 max major value */


#define	L_BITSMAJOR	14	/* # of SVR4 major device bits */
#define	L_BITSMINOR	18	/* # of SVR4 minor device bits */
#define	L_MAXMAJ	0x3fff	/* SVR4 max major value */
#define	L_MAXMIN	0x3ffff	/* MAX minor for 3b2 software drivers. */
				/* For 3b2 hardware devices the minor is */
				/* restricted to 256 (0-255) */

/* major part of a device internal to the kernel */

#define	major(x)	(int)((unsigned)((x)>>O_BITSMINOR) & O_MAXMAJ)
#define	bmajor(x)	(int)((unsigned)((x)>>O_BITSMINOR) & O_MAXMAJ)

/* get internal major part of expanded device number */

#define	getmajor(x)	(int)((unsigned)((x)>>L_BITSMINOR) & L_MAXMAJ)

/* minor part of a device internal to the kernel */

#define	minor(x)	(int)((x) & O_MAXMIN)

/* get internal minor part of expanded device number */

#define	getminor(x)	(int)((x) & L_MAXMIN)

/*-
 * sync mode states
 *
 * a logical group is in sync mode where the sync depth is 
 * set positive indefinite.
 * 
 * sync mode behavior is triggered only when some threshold 
 * value number of unacknowledge write log entries accumulates.
 */
/*
#define LG_IS_SYNCMODE(lgp) ((lgp)->sync_depth != -1)
#define LG_DO_SYNCMODE(lgp) \
    (LG_IS_SYNCMODE((lgp)) && ((lgp)->wlentries >= (lgp)->sync_depth))
*/
#endif // #if 1

// Ack Thread does it work either thru timeout or signaled to its event
// It does it works when LG state is either NORMAL MODE Or 
//
// Ack thread will clean all bits before updation of bitmaps
#define DEV_LASTBIT_CLEAN_ALL			0xFFFFFFFF

// Ack thread will Not make any bit to zero before updation of bitmaps, keep the same
#define DEV_LASTBIT_NO_CLEAN			0x0

#define DEFAULT_UPDATE_LASTBIT_TO_PS	10

// EXTRA_BIT_TO_COUNT is used In refresh (full refresh and smart refresh) state to update 
// LRDB and HRDB bitmap for incoming Writes. This is exta bit to add up to check lower than lastbitindex
// to update range of bitsa for incoming writes. To Optimize Refresh logic.
#define	EXTRA_BIT_TO_COUNT				2

#define FULL_REFRESH_LAST_INDEX(currentBitIndex)		(currentBitIndex + EXTRA_BIT_TO_COUNT)
// Ack thread will make bit range (starting bit 0 to specified bit value in dev->RefreshLastBitIndex) to zeros
// before updation of bitmaps, keep the same
// #define DEV_LASTBIT_VALID_VALUE					0x0

// -------------- Performance Related Structures and enums ---------------------

// Performance monitor type category based on components. It defines which type is used to 
// monitor which area of modules and components. 
typedef enum PERF_TYPE
{
#if	MM_TEST_WINDOWS_SLAB
	MM_ALLOC_PROCESSING		=	0,	// Mem Alloc from LookAsideList Perf Time
	MM_FREE_PROCESSING		=	1,	// Mem Free to LookAsideList Perf Time
#endif
	WR_IRP_PROCESSING,	// Write IRP Processing (time elapse from IoCalldriver() to IoCompleteRequest() )
	RD_IRP_PROCESSING,				// Read IRP Processing (time elapse from IoCalldriver() to IoCompleteRequest() )

	RD_REFRESH_DATA,				// Time for Refresh Read,(time elapse from IoCalldriver() to IoCompleteRequest() )
	BAB_OVERFLOW_STATE_TRANISITION,	// Cache OverFlow State Tranisition, ,(time elapse from Detected Cache Overflow to tracking mode completed )

	QM_FREE_ALL_PKTS_AND_UPDATE_BITMAPS,	// Perf Time for Free All QM Pkts and also update Bitmaps per LG
	QM_FREE_ALL_PKTS_AND_NO_UPDATE_BITMAPS, // Perf Time for Free All QM Pkts without updating Bitmaps per LG
	QM_SCAN_ALL_PKTS_AND_UPDATE_BITMAPS,	// Perf Time for Scan All QM Pkts and update Bitmaps per LG

	ACK_THREAD_PREPARE_BITMAPS,				// Ack Thread Preparing Bitmaps API sftk_ack_prepare_bitmaps()
	ACK_THREAD_ONLY_PARSE_QUEUE_LIST,		// Ack Thread Traverse Cache Manager's Queue along with Delete, Records Total time it takes,
	ACK_THREAD_PARSE_AND_DELTE_QUEUE_LIST,	// Ack Thread Traverse Cache Manager's Queue only, Records Total time it takes,
	
	MAX_PERF_TYPE,		// Move this value to eliminate some type of performance monitoring 
	
} PERF_TYPE;


//	xxxDiffTime field value is in 100 nanoseconds units, since KeQuerySystemTime() returns values 
//	in 100 nanoseconds units
// so Average seconds will be = (xxxDiffTime * 10) MicroSeconds, (1 Micro Seconds = 10^(-6))
#define Get_MicroSec_From100NanoSec(xxxDiffTime)	(xxxDiffTime * 10)	// returns value in us (microseconds)

typedef struct PERF_INFO
{
	ULONG			TotalCount;
	ULONGLONG		TotalLoad;		// Total Number of entries existed in Queue, or load or etc...
	LARGE_INTEGER	TotalDiffTime;	// Total Diference time, average is (TotalDiffTime / TotalCount) nano

	ULONG			MinLoadCount;	// MinLoadCount is number of loads (IOsize, or total entries) when recored MinDiffTime
	LARGE_INTEGER	MinDiffTime;	// Mimimun Diff time.

	ULONG			MaxLoadCount;	// MaxLoadCount is number of loads (IOsize, or total entries) when recored MaxDiffTime
	LARGE_INTEGER	MaxDiffTime;	// Maximum Diff time.
    
} PERF_INFO, *PPERF_INFO;

/**************************************************************************
    every structure has a node type, and a node size associated with it.
    The node type serves as a signature field. The size is used for
    consistency checking ...
**************************************************************************/
typedef struct SFTK_NODE_ID 
{
    USHORT			NodeType;           // Identify node type
    USHORT			NodeSize;           // total size of node.

} SFTK_NODE_ID, *PSFTK_NODE_ID;

typedef struct SFTK_BITMAP 
{
	OS_ERESOURCE	Lock;	// used to synchronize access to this Bitmaps
	ULONG			Sectors_per_volume;			// How many disk blocks (sectors) are on this volume?  
												// Disk size = Sectors_per_volume * 512	
	ULONG			Sectors_per_bit;			// How many disk blocks (sectors) are represented by each bit in the bitmap
												// Chunk size = Sectors_per_bit * 512
	ULONG			TotalNumOfBits;				// Total number of bits 
	ULONG			BitmapSize;					// total Size of Buffer used for bitmap.IN BYTES
	ULONG			BitmapSizeBlockAlign;		// We always allocate bitmap buffer this much size which is Sector aligned.
												// its needed since we have to flush it to pstore file, (file I/O is always 
												// bound to sector in kernel)	
	struct SFTK_PS_BITMAP_REGION_HDR	*pPsBitmapHdr;	// Used for Pstore flush Validation/Checksum of Bitmap Region LRDB versuse HRDB
	PULONG						pBits;			// pointer buffer used for bitmap; must be allocated from non-paged pool

	OS_RTL_BITMAP	*pBitmapHdr;				// Used by the native bitmap routines

	UINT64			DirtyMap;					// TODO : Opimization code : Keeps dirty bit Region info to flush specific LRDB to Pstore.
	UINT64			Ranges_per_map;				// TODO : 

	// For Backward support, remove it later once not needed
	ULONG			len32;		// length of "map" in 32-bit words  
	ULONG			bitsize;	//  a bit represents this many bytes 
	ULONG			shift;		// converts sectors to bit values   
    ULONG			numbits;	// number of valid bits */

} SFTK_BITMAP, *PSFTK_BITMAP;

// IRP_CONTEXT strcutre is used as (PIRP_CONTEXT) &pIrp->Tail.Overlay.DriverContext[0];
// to queue IRP to sftk_dev_master_thread for IRP processing.
typedef struct IRP_CONTEXT 
{
    LIST_ENTRY		ListEntry;    // Double Link Link 
    PIRP			Irp;          // Back Pointer to IRP

} IRP_CONTEXT, *PIRP_CONTEXT;

typedef struct RCONTEXT
{
	struct SFTK_LG	*Sftk_Lg;
	KEVENT			Event;

} RCONTEXT, *PRCONTEXT; 

typedef enum QUEUE_TYPE
{
	PENDING_QUEUE,
	COMMIT_QUEUE,
	REFRESH_QUEUE,
	MIGRATION_QUEUE,
	REFRESH_PENDING_QUEUE,
	TRECEIVE_QUEUE,
	TSEND_QUEUE,
	NO_QUEUE,
} QUEUE_TYPE, *PQUEUE_TYPE;

// Queue Manager exist per Logical group, Since per LG we have socket connections
typedef struct QUEUE_MANAGER
{
	// OS_ERESOURCE		Lock;			// used to synchronize access to this structure.
	OS_KSPIN_LOCK		Lock;
	ANCHOR_LINKLIST		PendingList;	// Anchor List for Pending Queue 
	ANCHOR_LINKLIST		CommitList;		// Anchor List for Commit Queue 
	ANCHOR_LINKLIST		RefreshPendingList;	// Anchor List for Refresh Queue 
	ANCHOR_LINKLIST		RefreshList;	// Anchor List for Refresh Queue 
	ANCHOR_LINKLIST		MigrateList;	// Anchor List for Migration Queue 

	// Following Queues used for secondary side code.
	ANCHOR_LINKLIST		TRecieveList;	// Anchor List for Target Recieve Queue 
	ANCHOR_LINKLIST		TSendList;		// Anchor List for Target Send Queue 

	ULONGLONG			TotalRawDataSize;	// in bytes, Total RAW Data Size in All Queue has used till moment
	ULONGLONG			CommitRawDataSize;	// in bytes, Currently RAW Data Size allocated and used in Commit Queue
	ULONGLONG			RefreshRawDataSize;	// Currently RAW Data Size allocated and used in Refresh Queue

	ULONG				SR_SendCommitRecordsPerSRRecords; // DEFAULT_SR_SENDCIMMITRECORDS_PER_SRRECORDS: Counter how many BAB Commit Records will send per smart refresh Record Pkts
	ULONG				SR_SentCommitRecordCounter; // Counter how many Commit Records pkts have been set, always starts from 0
													// 

} QUEUE_MANAGER, *PQUEUE_MANAGER; 

//
// SFTK_ROLE is used to keep information about Role for LG 
//
typedef struct SFTK_ROLE
{
	// --- Role information
	ROLE_TYPE	CreationRole;		// stores creation time role, this value never get changed.
	ROLE_TYPE	PreviouseRole;		// stores Previouse Role of current running role
	ROLE_TYPE	CurrentRole;		// stores Current Running Role 

	// ---- Secondary side usage attributes which defines secondary state mode
	BOOLEAN		FailOver;				// TRUE means Failover is enbled.
	BOOLEAN		JEnable;				// TRUE means Journal is ON.
	BOOLEAN		JApplyRunning;			// TRUE means Journal Apply is running
	KEVENT		JApplyWorkDoneEvent;	// SynchronizationEvent: event get signalled when journal apply thread finish its all work
	
	// --- protocol related fields.
	BOOLEAN		ProtoRunTracking;	// Primary : Protocol command set it to TRUE when it wants Primary to be run in tracking till 
									// users says to change or protocol command says to make it FALSE
									// used in Journal Overflow, failover with no journal, or any seriouse protocol error 
	// --- Journal Write Management
	CHAR			JPath[256];				// stores user supplied journal path here
	UNICODE_STRING	JPathUnicode;			// stores user supplied journal path in unicode


	// Current 'i' Journal information
	ULONG		JcurSR_FileNo;			// for Smart refresh, current 'i' Journal file number used in journal mode.
	ULONGLONG	JcurSR_Offest;			// for Smart refresh, current 'i' Journal file's offset where to write next record.
	ULONGLONG	JcurSN_Offest;			// for Smart refresh BAB, current 'i' Journal file's offset where to write next record.
	
	// Current 'c' Normal Mode Journal information
//	ULONG		JcurNormalFileNo;	// Normal Mode Journal current file number used for journaling mode.
	ULONGLONG	JcurN_Offest;		// for Normal Mode, Journal current file's offset where to write next record.

	// Journal Write mode File handles	
	HANDLE		JWriteSR_File;	
	HANDLE		JWriteSN_File;	
	HANDLE		JWriteN_File;	

	// --- statistics information for all existing journals
	ULONG		JTotalSetOfFiles;			// Counter starts from 1 to total number of Journal file
	ULONGLONG	JcurSRN_TotalFileSize;		// for Smart refresh and BAB, current 'i' Journal file size in bytes. 
	ULONGLONG	TotalAllJFileSize;			// Total all existing journals file size including 'i' and 'c' 
	ULONGLONG	TotalAllCJFileSize;			//Total all existing Commited 'c' journals file size, 

	// --- Journal Apply Informations
	// Apply process Journal File handles	
	HANDLE				JApplySR_File;	
	HANDLE				JApplySRN_File;
	HANDLE				JApplyN_File;	
	
	ULONG				JCurApplyFileNumber;	// starts from 0 to 999 and goes in circular
	JAPPLY_FILE_TYPE	JCurApplyFileType;		// curremnt Journal file type applying, sequence is: SR, then SR_Normal then Normal	
	ULONGLONG			JCurApplyFileOffset;	// current apply file offset
	ULONGLONG			JTotalDoneApplySize;	// Till moment total size of data applied. 


} SFTK_ROLE, *PSFTK_ROLE;

//
// SFTK_SECONDARY is used to keep information about secondary role for LG 
//
typedef struct SFTK_SECONDARY
{
	// ---- State mode Attributes already defined in SFTK_ROLE for secondary work
	BOOLEAN		JOverFlow;					// TRUE means Journal Overflow happened
	BOOLEAN		ProtoSendResumeTracking;	// TRUE means it requires sending Resume Primary Tracking mode proto command

	BOOLEAN		DoFastMinimalJApply;		// TRUE means Skip last .NC file Apply in JApply process, this way
											// failover can start soon. Once JApply is done it signals JApplyWorkDoneEvent.

	// --- Thread informations
	// TargetWrite Thread: it uses Target Recieve Queue to get work
	BOOLEAN			TWriteThreadShouldStop;			// TRUE means terminate this thread
	PVOID			TWriteThreadObjPtr;				// sftk_Target_write_Thread()
	KEVENT			TWriteWorkEvent;				// SynchronizationEvent: gets signal when work is there to process for Target Write Thread
	LARGE_INTEGER	TWriteWakeupTimeout;			// DEFAULT_TIMEOUT_FOR_TARGET_WRITE_THREAD, Timeout used to sleep TWrite thread to wait for Next pkt to process

	// Statistics for Target Write Thread
	ULONG			TWriteNumOfAsyncWriteAllowed;		// TWrite : Max number Of Async Disk Writes allowed
	ULONG			TWriteMaxSizeOfAsyncWriteAllowed;	// TWrite : Max size Of Async Disk Writes allowed
	ULONGLONG		TWriteTotalPktProcessed;			// TWrite : Keeps counter for total all pkts processed by this thread
	ULONGLONG		TWriteTotalIOPktsProcessed;			// TWrite : Keeps counter for total IO Plts Processed 
	ULONGLONG		TWriteTotalWriteSizetoDisk;			// TWrite : Keeps Total Size Write done to disk till moment
	ULONGLONG		TWriteTotalWriteSizetoJournal;		// TWrite : Keeps Total Size Write done to Journal till moment

	// JouranlApply Thread: it uses Target Recieve Queue to get work
	BOOLEAN			JApplyThreadShouldStop;			// TRUE means terminate this thread
	PVOID			JApplyThreadObjPtr;				// sftk_JApply_Thread()
	KEVENT			JApplyWorkEvent;				// SynchronizationEvent: gets signal when work is there to process for Target Write Thread
	LARGE_INTEGER	JApplyWakeupTimeout;			// DEFAULT_TIMEOUT_FOR_JAPPLY_THREAD, Timeout used to sleep JApply thread to wait for its work

	// JouranlApply for Target Write Thread
	ULONG			JApplyNumOfAsyncWriteAllowed;		// JApply : Max number Of Async Disk Writes allowed
	ULONGLONG		JApplyTotalWriteSizetoDisk;			// JApply : Keeps Total Size Write done to disk till moment

} SFTK_SECONDARY, *PSFTK_SECONDARY;

//
// SFTK_LG is used as Device Extension to Logical Device created for GUI Communincations !!
//
typedef struct SFTK_LG
{
    SFTK_NODE_ID		NodeId;			// NODE_TYPE_SFTK_LG type node

	PDRIVER_OBJECT      DriverObject;	
    PDEVICE_OBJECT      DeviceObject;	
	UNICODE_STRING      UserVisibleName;

	OS_ERESOURCE		Lock;					// used to synchronize access to this structure.
	OS_ERESOURCE		AckLock;				// used to synchronize Bitmap access with Acknolodege thread 

	ANCHOR_LINKLIST		LgDev_List;				// Anchor List for Devices SFTK_DEV->Lg_GroupLink
	LIST_ENTRY			Lg_GroupLink;			// LinkList for Anchor SFTK_CONFIG->Lg_GroupList, 

	BOOLEAN				ConfigStart;			// set to FALSE at FTD_CONFIG_BEGIN and if its still set to FALSE at FTD_CONFIG_END, LG gets deleted
	BOOLEAN				ResetConnections;		// used for begin/end to make sure we reset connections if any new device added or removed under lg.

	ULONG				LGroupNumber;			// Logical group number supplied from Service
	BOOLEAN             SRefreshWasNotDone;		// TRUE means Smart Refresh was not completed and state got changed to tracking...so next time smart refresh 
												// starts do not send Protocol MSGINCO otherwise RMD will have invalid Journal data....
	LONG                PrevState;				// Saving Prev state, need it for protocol...SFTK_MODE_FULL_REFRESH, etc... defined in ftdio.h file
	LONG                state;					// SFTK_MODE_FULL_REFRESH, etc... defined in ftdio.h file
	BOOLEAN				bInconsistantData;		// At new LG create, this becomes TRUE, it remains FALSE till first time Smart Refresh gets started.
												// create time LG, State mode default  get sets to Tracking mode.
	BOOLEAN				bPauseFullRefresh;		// Indicates if we have to pause during full refresh Default is FALSE, (No Pause). TRUE is PAUSE
#if TARGET_SIDE
	BOOLEAN				UseSRDB;				// TRUE means SR not completed to Normal, use SRDB to update Bits for new BAB Data, This never get clean till NORMAL mode. 
												// FALSE means Do not use SRDB
#endif		

	LONG                DoNotSendOutBandCommandForTracking;	// TRUE means Ack Thread must not send FTDCHUP outband command else Send it and wait for Ack indifinitely
	LARGE_INTEGER		MaxOutBandPktWaitTimeout;			// DEFAULT_TIMEOUT_FOR_MAX_WAIT_FOR_OUTBAND_PKT, Timeout used to wait any Outband command...after this it will cancel Pkt and reset connections...
	ULONG               flags;					//	SFTK_LG_FLAG_CACHE_INITIALIZED 
	ULONG				TrackingIoCount;			// >= MAX_IO_FOR_TRACKING_TO_SMART_REFRESH than we change the state
	ULONG				UserChangedToTrackingMode;	// TRUE if user IOCTL has changed the state to tracking mode.
													// in this case Driver does not change tracking mode to smart refresh mode
													// till user sends other atte change IOCTL.
    
	BOOLEAN				LastShutDownUpdated;	// used only duiring boot time, if flag is set we update registry		
	KEVENT				Event_LGFreeAllMemOfMM;	// signals when all memory of LG is free to MM

	ULONG				MaxTransferUnit;			// DEFAULT_MAX_TRANSFER_UNIT_SIZE, in bytes Maximum Raw Data Size 
													// which gets transfered to secondary.

	BOOLEAN             RefreshThreadShouldStop;	// TRUE means terminate the thread
	PVOID               RefreshThreadObjectPointer;	// sftk_refresh_lg_thread 
	KSEMAPHORE			RefreshStateChangeSemaphore;// Signals whenever LG state gets changed to Full/Smart Refresh
	LARGE_INTEGER		RefreshThreadWakeupTimeout;	// DEFAULT_TIMEOUT_FOR_REFRESH_THREAD, Timeout used to sleep in refresh thread to wait for multi-events
	KEVENT				EventRefreshWorkStop;		// Signals whenever LG Refresh thread stops it works due to state change
	KEVENT				RefreshEmptyAckQueueEvent;	// SynchronizationEvent: when Ack Queue gets empty, this event gets signaled by veera
	BOOLEAN				RefreshFinishedParseI;		// TRUE means we are done with all Devices under LG refresh phase I

	ULONG				NumOfAsyncRefreshIO;		// DEFAULT_NUM_OF_ASYNC_REFRESH_IO, Maximum Number of Refresh Async Read IO allowed
	ULONG				NumOfAllocatedRefreshIOPkts;// DEFAULT_NUM_OF_ASYNC_REFRESH_IO, Currently allocated total number of Asyn Refresh IO Buffer Pkts

	ANCHOR_LINKLIST		RefreshIOPkts;				// Anchor List for Refresh NumOfAsyncRefreshIO Buffer Link list 
	BOOLEAN				WaitingRefreshNextBuffer;	// Points to next Free available buffer
	BOOLEAN				ReserveIsActive;			// TRUE means ReservePool Is done else is not done
	BOOLEAN				ReleaseIsWaiting;			// TRUE means Release pool Call is waiting till all pkts gets free back to the pool...
	PVOID				RefreshNextBuffer;			// points to next Free available buffer
	KEVENT				RefreshPktsWaitEvent;		// SynchronizationEvent: Gets signalled when Free pkts ready 
	KEVENT				ReleaseFreeAllPktsWaitEvent;// SynchronizationEvent: Gets signalled when ReleaseIsWaiting = TRUE And all pkts gets free to pool back
	KEVENT				ReleasePoolDoneEvent;		// SynchronizationEvent: Gets signalled when All pkts of Refreshpool get free to MM, to inform MM all Refreshpool pkts for current LG is free 

	BOOLEAN             AckThreadShouldStop;		// TRUE means terminate the thread
	PVOID               AckThreadObjectPointer;		// sftk_acknowledge_lg_thread 
	LARGE_INTEGER		AckWakeupTimeout;			// DEFAULT_TIMEOUT_FOR_ACK_THREAD, Timeout used to wake up Ack thread and prepare its LRDB or HRDB
	KSEMAPHORE			AckStateChangeSemaphore;	// Signals whenever requires to perpare new HRDB 
	KEVENT				EventAckFinishBitmapPrep;	// Bitmap Preparation is finished by ack thread
	
	BOOLEAN				CacheOverFlow;			// TRUE means LG got BAB overflow, value gets reset in ACK thread
	BOOLEAN				UpdateHRDB;				// TRUE means Ack will Prepare or update HRDB else not.
	BOOLEAN				DualLrdbMode;			// TRUE means for every devices We use Dual LRDB update for incoming writes,
												// this used for intermediate stage when of Ack thread ispreparing new bitmaps.
	BOOLEAN				CopyLrdb;				// TRUE means when Ack thread done making new LRDB, we have to make copy of it and 
												// come out of DualLrdbMode, this happens in any IRP handling Device thread

	PDEVICE_OBJECT      PStoreDeviceObject;
    UNICODE_STRING      PStoreFileName;
	CHAR				PStoreName[256];
	struct SFTK_PS_HDR	*PsHeader;				// Pstore File header
	ULONG				SizeBAlignPsHeader;		// Sector Aliogn PsHeader memory size

	QUEUE_MANAGER		QueueMgr;				// Queue Manager
	SESSION_MANAGER		SessionMgr;				// Socket Connection information per LG

	KEVENT				EventPacketsAvailableForRetrival;	// Signals whenever there are packets available to be retrieved and sent from COMMIT or REFRESH QUEUE

	STATISTICS_INFO		Statistics;			// All Statistics information will be stored here

    ULONG				sync_depth;		// it must me LONG cause's it stores -1 SFTK_LG_SYNCH_DEPTH_DEFAULT
										// If lg->sync_depth < lg->wlEntries than don't complete Write irp but delay 
										// completion of IRP for sync_timeout seconds than complete the IRP
										// this is used to delay src disk I/O if sedondary Comit io gets slow down
										// (sync_depth number of IO or greater than IO is pending to comit on secondary side)
    ULONG				sync_timeout;	// This is not Used in current/Previous driver
										// Its is used to hold incoming IO Completion Irp fot specified seconds delay, 
    ULONG				iodelay;		// Not used. Purpose would be same as sync_timout or to delay 
										// incoming IO goes to disk, sync_timout was used to delay Completion of 
										// IRP which is completed already but don't pass back to caller.

	// Remove foll. fields later.... not needed anymore... BS stats - All statistic values 
	ULONG				Lg_OpenCount;	// Number of Logical Group is opened (by service means in used..)...
    HANDLE              hEvent;			// Named Event : old code was using this to signal Service PMD thread to 
	PKEVENT             Event;			// retrieve the data from kernel BAB, NOT NEEDED ANY MORE... 

	CHAR				*statbuf;	// Not Used, just copy buffer from and to service using IOCTL.....
    LONG                statsize;	
    ULONG				wlentries;	// Total number of writes send to secondary side and yet 
									// secondary side comiited ack is not responde.
    ULONG				wlsectors;	// Total number of Sector writes send to secondary side and yet 
									// secondary side comiited ack is not responde.
    ULONG				dirtymaplag; // number of entries since last reset of dirtybits
									 // gets increment when data gets migrated to secondary side, and gets reset 
									 // when number reaches to FTD_DIRTY_LAG (1024) .. TODO : Do we need this ?
    LONG                ndevs;		// specifies total number of device, NOT USED in NEW Code, instead use
									// ANCHOR LgDev_List.NumOfNodes value.

	ULONG		throtal_refresh_send_pkts;		// DEFAULT_ALLOWED_NUM_OF_PKTS_PENDING_FOR_REFRESH
	ULONG		throtal_refresh_send_totalsize;	// DEFAULT_ALLOWED_MAX_SIZE_PENDING_FOR_REFRESH
	ULONG		throtal_commit_send_pkts;		// DEFAULT_ALLOWED_NUM_OF_PKTS_PENDING_FOR_COMMIT
	ULONG		throtal_commit_send_totalsize;	// DEFAULT_ALLOWED_MAX_SIZE_PENDING_FOR_COMMIT

	ULONG		NumOfPktsSendAtaTime;			// This many pkts will get send at a time, DEFAULT_NUM_OF_PKTS_SEND_AT_A_TIME
	ULONG		NumOfPktsRecvAtaTime;			// This many pkts will get Recv at a time, DEFAULT_NUM_OF_PKTS_RECV_AT_A_TIME
	ULONG		NumOfSendBuffers;				// This specifies the number of send Buffers used , DEFAULT_MAX_SEND_BUFFERS

	SFTK_ROLE		Role;							// Stores LG role model info (Primary or secondary)
	SFTK_SECONDARY	Secondary;						// Stores LG Secondary side information 

} SFTK_LG, *PSFTK_LG;

typedef struct SFTK_DEV
{
    SFTK_NODE_ID				NodeId;			// NODE_TYPE_SFTK_DEV type node
	struct _DEVICE_EXTENSION	*DevExtension;	// Source Disk Ptr To Dev_Ext

	KEVENT				PnpEventDiskArrived;	// gets signaled when disk gets attached for specific SFTK_DEV 

	struct SFTK_LG		*SftkLg;	// Back pointer to Logical Group

	OS_ERESOURCE		Lock;					// used to synchronize access to this structure.

	LIST_ENTRY			LgDev_Link;				// linklist used for Anchor SFTK_LG->LgDev_List
	LIST_ENTRY			SftkDev_Link;			// linklist used for Anchor SFTK_CONFIG->SftkDev_List
	LIST_ENTRY			SftkDev_PnpRemovedLink;	// linklist used for Anchor SFTK_CONFIG->SftkDev_PnpRemovedList

    ULONG               Flags;					// SFTK_DEV_FLAG_SRC_DEVICE_ONLINE, etc..
    ULONG				Disksize;				// in Sectors, Actual sizae = disksize * 512
	BOOLEAN				ConfigStart;			// set to FALSE at FTD_CONFIG_BEGIN and if its still set to FALSE at FTD_CONFIG_END, Dev gets deleted

    SFTK_BITMAP			Lrdb;					// Low Resoultion Bitmap, Always gets Flush to Pstore file. 
    SFTK_BITMAP			Hrdb;					// High Resoultion Bitmap, 	
	SFTK_BITMAP			Srdb;					// same as HRDB used only during Smart Refresh, 
												// During SR, LRDB and SRDB does not get cleanup till SR successfully completed to Normal (if SR goes in tracking, it will consider 
												// SR mode...since SR time we also send BAB data )
	SFTK_BITMAP			ALrdb;					// Acknowledge High Resoultion Bitmap. Only Acknowledge Thread uses (builds) this bitmaps and 
												// merge it to Lrdb and to make new Hrdb to reflect Ack information from secondary thread
	ULONG				RefreshLastBitIndex;	// refresh thread lastBit Index number till this number Refresh thread has sent data to secondary
												// DEV_LASTBIT_CLEAN_ALL means all bitmap is already processed
	ULONG				FirstDirtyBitNum;		// Stores first dirty bit num used only in smart refresh
	ULONG				UpdatePS_RefreshLastBitCounter;	// DEFAULT_UPDATE_LASTBIT_TO_PS
	ULONG				noOfHrdbBitsPerLrdbBit;			// 1 Lrdb bit = noOfHrdbBitsPerLrdbBit number of Hrdb Bit
	
	CHAR				Devname[256];      // User supplied information, from ftd_dev_info_t
    CHAR				Vdevname[256];     // User supplied information	from ftd_dev_info_t
	ULONG				LGroupNumber;	// Logical group number supplied from Service

	CHAR				strRemoteDeviceName[64];	// Remote Drive Letter from Config File

	BOOLEAN				bUniqueVolumeIdValid;	// TRUE means UniqueIdLength and UniqueId has valid values
	USHORT				UniqueIdLength;			// OS supported Unique ID for Volume (Raw Disk/ Disk Partition)
    UCHAR				UniqueId[256];			// 256 is enough, if requires bump up this value.
	
	BOOLEAN				bSignatureUniqueVolumeIdValid;	// TRUE means SignatureUniqueIdLength and SignatureUniqueId has valid values
	USHORT				SignatureUniqueIdLength;		// Our customize alternate Disk Signature based Unique ID for Volume (Raw Disk/ Disk Partition)
    UCHAR				SignatureUniqueId[256];			// 256 is enough, if requires bump up this value.
	
	BOOLEAN				bSuggestedDriveLetterLinkValid;	// TRUE means SuggestedDriveLetterLinkLength and SuggestedDriveLetterLink has valid values
	USHORT				SuggestedDriveLetterLinkLength;	// Optional - If Device is formatted disk Partition, than associated drive letter symbolik link name info 
    UCHAR				SuggestedDriveLetterLink[128];	// 256 is enough, if requires bump up this value.

    ULONG               cdev;					// Dev Id used for Protocol to send to secondary and also used other places..
    ULONG               bdev;					// value always same as cdev, exist only for backward compatibily
    ULONG               localbdisk;				// value always same as cdev, exist only for backward compatibily
    ULONG               localcdisk;				// value always same as cdev, exist only for backward compatibily

    CHAR                *statbuf;				// Not Needed anymore, Remove it later
    ULONG               statsize;				// Not Needed anymore, Remove it later
	ULONG               PendingIOs;				// Do we need this ? Not sure, remove it later
	ULONG               PendingIOs_highwater;	// Do we need this ? Not sure, remove it later
	ULONG               PendingIOs_Lowwater;	// Do we need this ? Not sure, remove it later
	
	STATISTICS_INFO		Statistics;			// All Statistics information will be stored here

	LONG				lrdb_offset;	// in blocks, this is PStore offset where we need to write LRDB.. 
	BOOLEAN             ThreadShouldStop;	// TRUE means Stop the thread

    PVOID               MasterThreadObjectPointer;
	OS_KSPIN_LOCK		MasterQueueLock;		// Lock is used to control Link list MasterQueueListHead,may get used in Dispatch LEVEL !!
    ANCHOR_LINKLIST     MasterQueueList;		// IRP gets inserted init using IRP_CONTEXT structure
	KEVENT				MasterQueueEvent;
    // KSEMAPHORE          MasterQueueSemaphore;
	LARGE_INTEGER		MasterIrpProcessingThreadWakeupTimeout;	// DEFAULT_TIMEOUT_FOR_REFRESH_THREAD, Timeout used to sleep in refresh thread to wait for multi-events


	struct SFTK_PS_DEV	*PsDev;				// Pstore Dev structure used to flush dev info to pstore file
	ULONG				SizeBAlignPsDev;	// Sector Aliogn PsDev memory size
	ULONG				OffsetOfPsDev;		// Actual Offset on to Pstore file for PSDev to flush 

	// Remove foll. fields later.... not needed anymore... BS stats - All statistic values 
    UINT64				sectorsread;    // Total number of Sectors Read ocurred to src device, this number never gets decrements.
    UINT64				readiocnt;      // Total number of Reads ocurred to src device, this number never gets decrements.
    UINT64				sectorswritten; // Total number of Sectors Write ocurred to src device, this number never gets decrements.
    UINT64				writeiocnt;     // Total number of Write IO ocurred to src device, this number never gets decrements.
    ULONG               wlentries;		// number of incoming writes happened to device, and yet this 
										// writes is not commited to secondary side
    ULONG               wlsectors;		// Total number of incoming writes Sectors to device, and yet this 
										// writes is not commited to secondary side
} SFTK_DEV, *PSFTK_DEV;

typedef struct SFTK_CTL_DEV_EXT 
{
     SFTK_NODE_ID		NodeId;	// NODE_TYPE_SFTK_CTL type node

} SFTK_CTL_DEV_EXT, *PSFTK_CTL_DEV_EXT;

#include <sftk_mm.h>	// to include definations of struct MM_ANCHOR and MM_TYPE_MAX

// Global Control Device, This is Global Variable, We creates and stores
// all configuration information in this structures.
typedef struct SFTK_CONFIG
{
    SFTK_NODE_ID		NodeId;	// NODE_TYPE_SFTK_CONFIG type node

	OS_ERESOURCE		Lock;	// used to synchronize access to this structure.
	USHORT				Flag;	// SFTK_CONFIG_FLAG_SYMLINK_CREATED, ... total 16 flags can be defined
	
	ANCHOR_LINKLIST		Lg_GroupList;			// List of all configured TDMF Logical Group for source side (SFTK_LG) 
	ANCHOR_LINKLIST		TLg_GroupList;			// List of all configured TDMF Logical Group for target side (SFTK_LG) 

	ANCHOR_LINKLIST		SftkDev_List;			// List of all configured TDMF Device (SFTK_DEV) 
	ANCHOR_LINKLIST		DevExt_List;			// List of all Attached WIndows OS Device Extensions (DEVICE_EXTENSION)
	ANCHOR_LINKLIST		SftkDev_PnpRemovedList;	// List of all PNP Removed Attached SFTK_DEV configured device.

	ULONG				Lg_OpenCount;			// Number of Logical Group is opened (by service means in used..)... Not needed

	
    PDRIVER_OBJECT          DriverObject;		// Driver, control device anfd other relative information
	PDEVICE_OBJECT          CtrlDeviceObject;	// Driver, control device anfd other relative information
    PVOID                   DirectoryObject;
    PVOID                   DosDirectoryObject;
	
	PERF_INFO				PerfTable[MAX_PERF_TYPE];	// Performance monitoring statistics

#if	MM_TEST_WINDOWS_SLAB
	MM_ANCHOR				MmSlab[MM_TYPE_MAX];	// Anchor or container for MM_TYPE_MM_ENTRY
#endif

	MM_MANAGER				Mmgr;				// Stores all information used for memory manager

	// Adding the Host id which will be common for all the Logical Group
	ULONG					HostId;				// The HostId that us used to install
	CHAR					Version[32];		// The Version of the Product
	CHAR					SystemName[64];			// computer name for local system...needed for pstore file access in cluster....
	//NPAGED_LOOKASIDE_LIST   LGIoLookasideListHead;

	PFILE_OBJECT							ListenThread;		// Source Listen Thread
	PFILE_OBJECT							TListenThread;		// Target Listen Thread
	KEVENT									ListenThreadExitEvent;	// This Exit Event is set to exit the Source Listen Thread
	KEVENT									TListenThreadExitEvent;	// This Exit Event is set to exit the Target Listen Thread
} SFTK_CONFIG, *PSFTK_CONFIG;

#pragma pack(pop)

// Extern Global Variable Definations
extern UNICODE_STRING	GDriverRegistryPath;
extern SFTK_CONFIG		GSftk_Config;


#endif // _SFTK_DEF_H_