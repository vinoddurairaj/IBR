/**************************************************************************************

Module Name: sftk_MM.h   
Author Name: Parag sanghvi 
Description: Memory Manager Structures/Macros are deinfed here. 
			 Memory Manager APIs are declared in Sftk_proto.h file
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/
#ifndef _SFTK_MM_H_
#define _SFTK_MM_H_

#include <sftk_protocol.h>

#pragma pack(push, 4) 

#define ONEMILLION				(1024 * 1024)

#define	MM_TAG					'FDMT'

// Types of Memory manager Fixed size Anchor, used in MM_ANCHOR->Type 
#define MM_TYPE_MM_ENTRY			0	// Holds 4k Pages, structure MM_ENTRY
#define MM_TYPE_4K_NPAGED_MEM		1	// Holds Raw Memory, strcuture MM_ENTRY
#define MM_TYPE_MM_HOLDER			2
#define MM_TYPE_CHUNK_ENTRY			3
#define MM_TYPE_SOFT_HEADER			4 // struct (wlheader_t) defined in sftkprotocol.h file
#define MM_TYPE_PROTOCOL_HEADER		5 // strcut (ftd_header_t) defined in sftkprotocol.h file
#define MM_TYPE_MAX					6

#define MM_DEFAULT_MAX_NODES_TO_ALLOCATE	0xFFFFFFFF	// Allocate till you get it !!!

// NumOfNodestoKeep uses this macro MM_DEFAULT_MAX_NODES_TO_ALLOCATE
#if	MM_TEST_WINDOWS_SLAB
#define MM_TYPE_4K_PAGED_MEM	4
#ifdef MM_TYPE_MAX
#undef MM_TYPE_MAX
#endif
#define MM_TYPE_MAX				5
#define MM_ANCHOR_FLAG_LOOKASIDE_INIT_DONE	0x01
#define MM_ANCHOR_FLAG_USED_USED_LIST		0x02
#endif // MM_TEST_WINDOWS_SLAB

typedef struct MM_MDL_INFO
{
	PMDL		Mdl;			// MDL created from OrgVaddr and Size
	ULONG		Size;			// Size of OrgVaddr, 
	PVOID		OrgVAddr;		// Not needed, its caller Org Vaddr from where we create MDL
	PVOID		SystemVAddr;	// System Virtual Address for MDL

} MM_MDL_INFO, *PMM_MDL_INFO;

typedef struct MM_ANCHOR_MDL_INFO
{
	LIST_ENTRY			MdlInfoLink;		// Used either in MM_MEM->MmList or Global MMFreelist

	ULONG				ChunkSize;			// in bytes, specify Memory allocation chunksize, for AWE its always PAGE_SIZE = 4K
	ULONG				NumberOfArray;
	ULONG				TotalMemorySize;	// in bytes total memory size passed with this transaction

	MM_MDL_INFO			ArrayMdl[1];

} MM_ANCHOR_MDL_INFO, *PMM_ANCHOR_MDL_INFO;

#define SIZE_OF_MM_ANCHOR_MDL_INFO(numOfarray)	(sizeof(MM_ANCHOR_MDL_INFO) + (sizeof(MM_MDL_INFO) * numOfarray))

// For MM_TYPE_4K_NPAGED_MEM type: its MM_ANCHOR->FreeList uses one of following structure
// if MM_MANAGER->AWEUsed == TRUE than its MM_ENTRY with valid Physical page value in it
// if MM_MANAGER->AWEUsed == FALSE than its MM_CHUNK_ENTRY with Anchor of MM_ENTRY in it and , 
//										each MM_ENTRY will have valid Physical page value
//
typedef struct MM_ANCHOR
{
	ANCHOR_LINKLIST	FreeList;			// Link list of MM_ENTRY->MmEntryLink or if its free
	ANCHOR_LINKLIST	UsedList;			// Link list of MM_ENTRY->MmEntryLink if its free, Not USED !!!

	// Anchor of link list of MM_ANCHOR_MDL_INFO, Stores Service supplied memory and 
	// its relative Locked MDL pointers 
	ANCHOR_LINKLIST	MdlInfoList;		// Link list of PMM_ANCHOR_MDL_INFO->MdlInfoLink

	OS_KSPIN_LOCK	Lock;				// used to synchronize access to this structure.

	UCHAR			Type;				// MM_TYPE_MM_ENTRY, MM_TYPE_4K_NPAGED_MEM

	UCHAR			Flag;				// MM_ANCHOR_FLAG_LOOKASIDE_INIT_DONE

	ULONG			NodeSize;
	ULONGLONG		TotalMemSize;		// In bytes, total memory allocated for this MM_Type	

	ULONG			TotalNumberOfPagesInUse;// Total Number of Pages is in used currently ... 
											// TotalNumberOfUsedPages * GSftk_Config.Mmgr.PageSize = Actual bytes is in used
	
	ULONG			NumOfNodestoKeep;	// MM_DEFAULT_MAX_NODES_TO_ALLOCATE, user requested number of nodes to allocate and keep
	ULONG			TotalNumberOfNodes;	// Total Number of Entries allocated, 

	ULONG			TotalNumberOfRawWFreeNodes;	// Used only in MM_TYPE_CHUNK, Stors total number Free Chunks which are ready to free to system !!
	ULONG			NumOfMMEntryPerChunk;

	
#if DBG
	ULONG			UnUsedSize;		// Total size specifies Unused space in pages 
#endif
	
#if	MM_TEST_WINDOWS_SLAB
	ULONG			MaximumSize; // MaximumSize in bytes can get allocate
	ULONG			MinimumSize; // MaximumSize in bytes can get used 

	union
	{
	PAGED_LOOKASIDE_LIST	PagedLookaside;
	NPAGED_LOOKASIDE_LIST	NPagedLookaside;
	} parm;

	PALLOCATE_FUNCTION	Allocate;	// Allocate function pointer storage
    PFREE_FUNCTION		Free;		// Free function pointer storage
#endif

} MM_ANCHOR, *PMM_ANCHOR;

//
// MM_Type : MM_TYPE_CHUNK_ENTRY : MM_ANCHOR->FreeList used to hold this structure.
// This  MM_CHUNK_ENTRY uses ULONG FreeMap = 32 bit to store used and free pages in list
// bit 0 means Free, Bit 1 means used
// if in FreeMap has  5 bit = 1 means 5th entry in MMEntryList is free page.
// if in FreeMap has  3 bit = 0 means 3rd entry in MMEntryList is Used page.
//
// If all bit in FreeMap turns to 1 than this MM_CHUNK_ENTRY gets moved to 
// MM_ANCHOR->UsedList from MM_ANCHOR->FreeList
//
#define MM_CHUNK_FLAG_FREE_LIST		0x01	// Means MM_CHUNK is in MM_ANCHOR->FreeList
#define MM_CHUNK_FLAG_USED_LIST		0x02	// Means MM_CHUNK is in MM_ANCHOR->UsedList

typedef struct MM_CHUNK_ENTRY
{
	LIST_ENTRY		MmChunkLink;	// Used in MM_ANCHOR->FreeList if its free 
									// or MM_Holder->MmEntryList if its used
	ANCHOR_LINKLIST	MmEntryList;	// Link list of MM_MEM_ENTRY
	PVOID			VAddr;
	//ULONG			FreeMap;		// maximum MM_ENTRY maps is 32 = means 32 * pagesize = 32 * 4k = 128k bytes
	UCHAR			Flag;			// Either Free = MM_CHUNK_FLAG_FREE, finds in MM_ANCHOR->FreeList 
									// or Used =MM_CHUNK_FLAG_USED, finds in MM_ANCHOR->UsedList
} MM_CHUNK_ENTRY, *PMM_CHUNK_ENTRY;

// MM_Type : MM_TYPE_MM_ENTRY		MM_ANCHOR->FreeList used to hold this structure.
//									It has PageEntry value is not valid.
// MM_Type : MM_TYPE_4K_NPAGED_MEM	MM_ANCHOR->FreeList used to hold this structure.
//									It has valid PageEntry values.
typedef struct MM_ENTRY
{
	LIST_ENTRY			MmEntryLink;	// Used either in MM_ANCHOR->FreeList if its free 
								// or MM_Holder->MmEntryList if its used
	PULONG				PageEntry;		// Stores Physical Page Value
	PMM_CHUNK_ENTRY		ChunkEntry;			// Optional, 
								//	if AWE is not used than it used for back pointer 
								// to CHUNK Holder, strcut MM_CHUNK_ENTRY 
	// UCHAR		Flag;		// Either Free or Used
} MM_ENTRY, *PMM_ENTRY;

//
// MM_Type : MM_TYPE_MM_HOLDER : MM_ANCHOR->FreeList used to hold this structure.
//

// Flag used in MM_HOLDER->FlagLink, Remember at any time one value is only set in Flag field.
// followings all flags used only if entry is used. It 
// It identifies currently MM_HOLDER is in which of ANCHOR_LINKLIST
#define MM_HOLDER_FLAG_LINK_FREE_LIST			0x01
#define MM_HOLDER_FLAG_LINK_PENDING_LIST		0x02
#define MM_HOLDER_FLAG_LINK_COMMIT_LIST			0x04
#define MM_HOLDER_FLAG_LINK_REFRESH_LIST		0x08
// #define MM_HOLDER_FLAG_LINK_MIGRATE_LIST		0x10
#define MM_HOLDER_FLAG_LINK_MDL_LOCKED			0x20
#define MM_HOLDER_FLAG_LINK_TRECEIVE_LIST		0x40
#define MM_HOLDER_FLAG_LINK_TSEND_LIST			0x80

// defines MM_HOLDER->Proto_type
#define MM_HOLDER_PROTO_TYPE_PROTO_HDR			0x01
#define MM_HOLDER_PROTO_TYPE_SOFT_HDR			0x02

typedef struct MM_HOLDER
{
	LIST_ENTRY			IrpContextList;		// Used for segmentation of Larger Data Pkts for IRP completion context 
	LIST_ENTRY			MmHolderLink;		// Used in Global MMHolderUsedList or MMHolderFreeList
	// data memory information
	ANCHOR_LINKLIST		MmEntryList;		// Link list of MM_MEM_ENTRY
	PMDL				Mdl;				// MDl Of Size 
	ULONG				Size;				// Size Of Memory holding by MDL
	PVOID				SystemVAddr;		// System Virtual Address of MDL
	
	UCHAR				FlagLink;			// MM_HOLDER_FLAG_LINK_FREE_LIST :
											// When MM_HOLDER is used, this flag identifies 
											// Also uses MM_FLAG_ALLOCATED_FROM_OS

	UCHAR				Proto_type;			// MM_HOLDER_PROTO_TYPE_PROTO_HDR:
											// When MM_HOLDER is used, this flag identifies 
											// the type of struct used for pProtocolHdr
	PVOID				pProtocolHdr;		// this may be MM_PROTO_HDR or MM_SOFT_HDR or NULL
} MM_HOLDER, *PMM_HOLDER;

#if 1 // PARAG/VEERA: PROTOCOL Defination embed in MM

typedef enum HDR_TYPE
{
	SOFT_HDR,
	PROTO_HDR,
	NO_HDR,
} HDR_TYPE, *PHDR_TYPE;

typedef struct PROTO_OUTBAND
{
	PMM_HOLDER			OutMmHolder;	// Sender Out buffer/size/ProtoHdr/SofHdr goes to wire
	KEVENT				Event;			// Completion event of Ack recieved
	PMM_HOLDER			InMmHolder;		// Ack recieved buffer/size/ProtoHdr/SofHdr 
} PROTO_OUTBAND, *PPROTO_OUTBAND;

#define MM_FLAG_ALLOCATED_FROM_OS		0x80
#define	MM_FLAG_PROTO_HDR				0x01
#define	MM_FLAG_SOFT_HDR				0x02

#define	MM_FLAG_DATA_PKTS_IN_SOFT_HDR	0x04

// for type MM_TYPE_PROTOCOL_HEADER, this fixed size node structure
typedef struct MM_PROTO_HDR
{
	LIST_ENTRY			MmProtoHdrLink;		// store either in slab->FreeList or Migration Anchor

	UCHAR				Flag;				// Always set to MM_FLAG_PROTO_HDR, MM_FLAG_DATA_PKTS_IN_SOFT_HDR
											// if set to MM_FLAG_ALLOCATED_FROM_OS means Free to OS 
											// system directly this struct memory
	PSFTK_DEV			SftkDev;			// Pointer to sftk_Dev needed to get information
	ftd_header_t		Hdr;

	// Added this MDL so that we can use it link the MM_HOLDER MDL
	// Size will be equal to sizeof(ftd_header_t) and the Mdl->next will be MM_HOLDER->Mdl->next;
	PMDL				Mdl;				// MDl Of Size 
	ULONG				Size;				// Size Of Memory holding by MDL

	ANCHOR_LINKLIST		MmSoftHdrList;		// Link list of MM_SOFT_HDR struct

	ULONG				RawDataSize;		// Raw DataSize	of Migrated Pkts used for stats

	PKEVENT				Event;				// if non null means Event to get sinagled on Ack recieved for this protoHdr
	NTSTATUS			Status;				// used only for outband packets

	// Following 2 fields used only if Data buffer payload arrives or expected for this proto Hdr send message to secondary
	// Socket Reciver thread will allocate and initialize this Buffer before signalling Event for payload Ack recieved.
	// Following all PVOID Memory will get free using ExFreePool() or equivelent macros
	PVOID				RetProtoHDr;		// pointer to returned ftd_header_t, 
	PVOID				RetBuffer;			// Return Buffer, must point to PMM_HOLDER, valid only for Payload Ack Recived
	ULONG				RetBufferSize;		// Return Buffer Size
	LONG				msgtype;			// used only in Outband Sentinal pkts, save original sender's msg type before protocol changed it 
} MM_PROTO_HDR, *PMM_PROTO_HDR;

// for type MM_TYPE_SOFT_HEADER, this fixed size node structure
typedef struct MM_SOFT_HDR
{
	LIST_ENTRY			MmSoftHdrLink;		// Used in Global MMHolderUsedList or MMHolderFreeList

	// Added this MDL so that we can use it link the MM_HOLDER MDL
	// Size will be equal to sizeof(wlheader_t) and the MM_PROTO_HDR->Mdl->next->MM_HOLDER->Mdl->next = Mdl;
	PMDL				Mdl;				// MDl Of Size 
	ULONG				Size;				// Size Of Memory holding by MDL

	UCHAR				Flag;				// Always set to MM_FLAG_SOFT_HDR
											// if set to MM_FLAG_ALLOCATED_FROM_OS means Free to OS 
											// system directly this struct memory
	PSFTK_DEV			SftkDev;			// Pointer to sftk_Dev needed to get information
	wlheader_t			Hdr;
	
} MM_SOFT_HDR, *PMM_SOFT_HDR;

#endif // PARAG/VEERA: PROTOCOL Defination embed in MM

// used to set SFTK_LG->AckWakeupTimeout values. TODO : make this user defined or tunable....
#define DEFAULT_TIMEOUT_FOR_SM_THREAD_WAIT_100NS				-(300*1000*1000);  // relative 30 seconds
#define DEFAULT_TIMEOUT_FOR_SM_CMD_EXECUTE_WAIT_100NS			-(150*1000*1000);  // relative 15 seconds
#define DEFAULT_TIMEOUT_FOR_LG_TO_FREE_MEM_TO_MM_WAIT_100NS		-(300*1000*1000);  // relative 30 seconds
#define DEFAULT_TIMEOUT_FREEALL_RAW_MEM_CMD_WAIT_100NS			-(1800*1000*1000);  // relative 180 seconds

typedef enum SM_EVENT_COMMAND
{
	SM_Event_Alloc	=	1,
	SM_Event_Free,
	SM_Event_FreeAll,
	SM_Event_None

}SM_EVENT_COMMAND, *PSM_EVENT_COMMAND;


typedef struct MM_CMD_PACKET
{
	LIST_ENTRY			cmdLink;
	SM_EVENT_COMMAND	Cmd;
} MM_CMD_PACKET, *PMM_CMD_PACKET;

//	BOOLEAN
//	MM_IsAllocMemFromServiceNeeded( ULONGLONG _TotalMemSize_, ULONG _TotalMemUsed_, ULONG _AllocThreshold_)
// 
//  It Calculates Allocation Memory needed from service, if it RETURNS true than we need memory from service
//	else we do not need memory from service
#define MM_IsAllocMemFromServiceNeeded(_TotalMemSize_,_TotalMemUsed_, _AllocThreshold_)	\
				( ((ULONG) ( ((ULONGLONG) (_TotalMemUsed_) * 100) / (_TotalMemSize_) ) >= (_AllocThreshold_))?TRUE:FALSE)


#if 0
//	BOOLEAN
//	MM_IsFreeMemFromServiceNeeded( ULONGLONG _TotalMemSize_, ULONG _TotalMemUsed_, ULONG _FreeThreshold_)
// 
//  It Calculates Allocation Memory needed from service, if it RETURNS true than we need memory from service
//	else we do not need memory from service
#define MM_IsFreeMemFromServiceNeeded(_TotalMemSize_,_TotalMemUsed_, _FreeThreshold_)	\
				( ((ULONG) ( ((ULONGLONG) (_TotalMemUsed_) * 100) / (_TotalMemSize_) ) >= (_AllocThreshold_))?TRUE:FALSE)
#endif

//
// MM_MANAGER: is main structure holds all information about Memory Manager
//
typedef struct MM_MANAGER
{
	BOOLEAN		MM_Initialized;		// TRUE when MM is initialized else FALSE
	BOOLEAN		AWEUsed;			// TRUE when AWE is used else FALSE
	BOOLEAN		MM_UnInitializedInProgress;		// TRUE when MM stop is working on to free all memory 
	ULONG		PageSize;			// PageSize = System defined page size = 4K or ...
	ULONG		VChunkSize;			// System defined usermode Virtual Alloc Chunksize = 64K or ..
	
	// Stores Maximum amount of Memory will get allocated from system for our Memory Manager
	ULONG		MaxAllocatePhysMemInMB;	// in Mega Bytes
	ULONGLONG	TotalMemAllocated;		// Total Memory is currently allocated by MM

	// IncrementAllocationChunkSize is used to allocate on demand from driver, this memory is max at a time to allocate
	// when driver ask to allocate
	ULONG		IncrementAllocationChunkSizeInMB;

	OS_KSPIN_LOCK	Lock;				// used to synchronize access to this structure.
	OS_KSPIN_LOCK	ReservePoolLock;	// used to synchronize access for Reserve Pool access for All LG 

	// Alloc/Free User memory Threshold information
	BOOLEAN		SM_AllocExecuting;
	UCHAR		AllocThreshold;			// in Percentage, Used to alloc new memory from system/Service 
	ULONG		AllocIncrement;			// amount of mem used to alloc in this increment
	ULONG		AllocThresholdTimeout;	// Threshold check time interval In Milliseconds
	ULONG		AllocThresholdCount;	// Optional, Threshold check counter, mey be not needed 
	ULONG		TotalAllocHit;			// User mode Mem Total Alloc hit count
	
	BOOLEAN		SM_FreeExecuting;
	UCHAR		FreeThreshold;			// in Percentage, Used to Free new memory from system/Service 
	ULONG		FreeIncrement;			// amount of mem used to Free in this increment
	ULONG		FreeThresholdTimeout;	// Threshold check time interval In Milliseconds
	ULONG		FreeThresholdCount;		// Optional, Threshold check counter, mey be not needed 
	ULONG		TotalFreeHit;			// User mode Mem Total Free hit count

	// Statistics
	UINT64		MM_TotalOSMemUsed;		// Till moment, total Direct OS memory used for QM for current LG, this value never gets decremented
	UINT64		MM_OSMemUsed;			// At Present, total Direct OS memory allocated for QM for current LG
	
	UINT64		MM_TotalNumOfMdlLocked;			// Till moment, total Number Of MDL Alloc is done. This MDL memory allocated directly from OS, this value never gets decremented
	UINT64		MM_TotalSizeOfMdlLocked;		// Till moment, total size of memory all allocated MDL is representing, this value never gets decremented

	UINT64		MM_TotalNumOfMdlLockedAtPresent;	// At Present, total Number Of MDL Alloc is done. This MDL memory allocated directly from OS.
	UINT64		MM_TotalSizeOfMdlLockedAtPresent;	// At Present, total size of memory all allocated MDL is representing, 

#if 1 // SM_IPC_SUPPORT
	// Shared Memory Map Info 
	HANDLE		SM_HCreateSection;			// Handle To Shared Memory Map File
	HANDLE		SM_HSection;				// Handle To Shared Memory Map View Section
	ULONG		SM_Size;				// Shared Memory Map Total size.	
	PVOID		SM_Vaddr;				// Shared memory mapped Virtual base address
	PSM_CMD		SM_Cmd;					// Shared memory map Virtual memory pointer.
	PMDL		SM_Mdl;					// Optional not used, If needed Lock Shared memory map
										// and use this for mdl, This may need if we want touch 

										// this shared memory in any kernel context !!!
	PKEVENT		SM_NamedEvent;			// Synchronize Access to SM between Service and driver
	HANDLE		SM_NamedEventHandle;			// Synchronize Access to SM between Service and driver

	
	// Shared Memory Map Thread informtaions, 
	// Thread uses Shared Mem for IPC communication between driver and service.
	BOOLEAN			SM_ThreadStop;				// TRUE means terminate the thread
	PVOID           SM_ThreadObjPtr;			// SM_Thread()
	LARGE_INTEGER	SM_CmdExecWaitTimeout;		// Timeout used to wait in SM_Thread() 
												// DEFAULT_TIMEOUT_FOR_SM_THREAD_WAIT

	
	
	LARGE_INTEGER	SM_WakeupTimeout;			// Timeout used to wait in SM_Thread() 
												// DEFAULT_TIMEOUT_FOR_SM_THREAD_WAIT
	KEVENT				SM_Event;				// Internal local event - Signals Event To give work SM Thread 
	SM_EVENT_COMMAND	SM_EventCmd;			// This command get sets when SM_Event gets signalled
	KEVENT				SM_EventCmdExecuted;	// When SM_EventCmd gets completely executed, this gets signalled.
#endif

#if 1 // IPC_IOCTL_SUPPORT
	KEVENT				CmdEvent;				// Internal IOCTL local event - to pass work to IOCTL
	OS_KSPIN_LOCK		CmdQueueLock;				// used to synchronize access to this structure.
	ANCHOR_LINKLIST		CmdList;				// Link list of MM_CMD_PACKET strcutre
	BOOLEAN				FreeAllCmd;	
	KEVENT				EventFreeAllCompleted;	// When CMD_FreeAll Is executed completely, this event gets signalled if
												// MM_CMD_PACKET->TerminateIOCTL == TRUE	
#endif

	PVOID			PSystemProcess;			// address of system Proc, Used to lock 
											// down cache pages. 
	MM_ANCHOR		MmSlab[MM_TYPE_MAX];	// Anchor or container for MM_TYPE_MM_ENTRY

} MM_MANAGER, *PMM_MANAGER;

#pragma pack()
#endif // _SFTK_MM_H_

