/**************************************************************************************

Module Name: MMDll.h  
Author Name: Saumya Tripathi
Description: All MM API definations and structures are defined here
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2004 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#ifndef __MMDLL_H__
#define __MMDLL_H__

#define	MMAPI	__declspec(dllexport)

#define _WIN32_WINNT 0x0500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <errno.h>
#include <tchar.h>
#include <lmerr.h>			// for GetErrorText()

#include <Windows.h>
#include <string.h>
#include <stdio.h>
#include <memory.h>
#include <WinIoctl.h>	// to make call to DeviceIoControl() API
#include <conio.h>	// to make call to DeviceIoControl() API

#include "..\RFWDriver\ftdio.h"	// to make call DeviceIoControl() API to Our Driver 

//
// Macro Definations
//

// Defined this if you testing this as Test sample Exe.
#define TEST_AS_EXE

#define FTD_DEV_DIR "\\\\.\\Global\\DTC"
#define FTD_CTLDEV  FTD_DEV_DIR "\\ctl"
// 
// Default Values Macro
//
#define MM_DEFAULT_VAllocChunkSize						(64 * 1024)
#define MM_DEFAULT_MaxAvailPhysMemUsedInPercentage		60
#define MM_DEFAULT_InitRawMemoryToAllocateInMB			64
#define MM_DEFAULT_IncrementAllocationChunkSizeInMB		32
#define MM_DEFAULT_MAX_ALLOC_SIZE_IN_BYTES				(32 * ONEMILLION)
#define DEFAULT_SM_THREAD_WAIT_IN_MILLI_SECONDS			(30000)	// 30,000 milliseconds = 30 seconds
#define DEFAULT_SM_THREAD_EVENT_SIGNALLED_DELAY_SLEEP	(15000)	// 15,000 milliseconds = 15 seconds

// Kernel uses this Default values passed from service or its registry based defined
#define DEFAULT_MM_ALLOC_THRESHOLD						80	// in percentage
#define DEFAULT_MM_ALLOC_INCREMENT						(MM_DEFAULT_IncrementAllocationChunkSizeInMB * ONEMILLION)	
#define DEFAULT_MM_ALLOC_THRESHOLD_TIMEOUT				(60000)	// 60,000 milliseconds = 60 seconds
#define DEFAULT_MM_ALLOC_THRESHOLD_COUNT				100

#define DEFAULT_MM_FREE_THRESHOLD						50	// in percentage
#define DEFAULT_MM_FREE_INCREMENT						DEFAULT_MM_ALLOC_INCREMENT	
#define DEFAULT_MM_FREE_THRESHOLD_TIMEOUT				(60000)	// 60,000 milliseconds = 60 seconds
#define DEFAULT_MM_FREE_THRESHOLD_COUNT					100

// Round Up Value to BaseValue
#define ROUND_UP(Value,BaseValue)	\
			(Value = ( ((Value) + (BaseValue - 1)) & ~(BaseValue - 1)) )

// Round Down Value to BaseValue
#define ROUND_DOWN(Value,BaseValue) \
	(Value = ((Value) & ~(BaseValue - 1)) ) 


#define NUM_OF_PAGES_TO_FREE_AT_ATIME		1
#define PERCENTAGE_AVAIL_MEMORY				70	// in percentage of Total Available Physical Memory
#define RESERVE_MEMORY_INMB_KEEP_FREE		5	// 5MB keep free if needed
#define ONE_K								1024
#define ONEMILLION							(1024 * 1024)

#define PAGE_SIZE	4096
#define	SHIFT		12L

#define	SetPage(pagevalue_64, n)	{ pagevalue_64 = (((LONGLONG) n) << SHIFT); }
#define	GetPageNumber(retpagevalue_32, pagevalue_64)	\
											{ retpagevalue_32 = (unsigned long)(pagevalue_64 >> SHIFT); }

//
// Structure Definations
//
typedef struct GMM_INFO
{
	// MM_Initialized = TRUE means MM_Start is alrady called successfully before.
	BOOLEAN		MM_Initialized;	
	
	// UsedAWE = TRUE if we have used AWE APIs else FALSE
	BOOLEAN		UsedAWE;	

	// ThreeGB == TRUE means system supports 3 GB > above Virual memory
	BOOLEAN		ThreeGB;

	// Stores Current Process Handle, this is requires for AWE and Virtual Alloc/Free routines
	HANDLE		ProcessHandle;

	// Stores Total Physical memory of system in Mega Bytes
	ULONG		TotalPhysMemInMB;		// in Mega Bytes

	// Stores MM_Init called time, Avaliable Physical memory in system in Mega Bytes
	ULONG		AvailPhysMemInMB;		// in Mega Bytes

	// Stores Maximum Percentage values of total Available Physical memory we will be used for MM
	ULONG		MaxAvailPhysMemUsedInPercentage;	// in Percentage

	// Stores Maximum amount of Memory will get allocated from system for our Memory Manager
	ULONG		MaxAllocatPhysMemInMB;	// in Mega Bytes
	
	// MM_Start Initialize time RAW Memory will allocate to used.
	ULONG		InitRawMemoryToAllocateInMB;		// in Mega bytes

	// These is maximum number of bytes we can allocate using Virtual Alloc at a time, Since MDL has limitation
	// to lock memory, we need this limitation here too. 
	ULONG		MaxAllocAllowedInBytes;		// in bytes

	// If UsedAWE == FALSE than VAllocChunkSize is used to allocate memory in this unit
	ULONG		VAllocChunkSize;		// in bytes

	// PageSize is nothing but system's actual current Page size 
	ULONG		PageSize;

	// IncrementAllocationChunkSize is used to allocate on demand from driver, this memory is max at a time to allocate
	// when driver ask to allocate
	ULONG		IncrementAllocationChunkSizeInMB;

	// Currently Total Amount o memory allocated in bytes
	ULONGLONG	TotalMemoryAllocated;				// in bytes

	UCHAR		AllocThreshold;			// in Percentage, Used to alloc new memory from system/Service 
	ULONG		AllocIncrement;			// amount of mem used to alloc in this increment
	ULONG		AllocThresholdTimeout;	// Threshold check time interval In Milliseconds
	ULONG		AllocThresholdCount;	// Optional, Threshold check counter, mey be not needed 

	UCHAR		FreeThreshold;			// in Percentage, Used to Free new memory from system/Service 
	ULONG		FreeIncrement;			// amount of mem used to Free in this increment
	ULONG		FreeThresholdTimeout;	// Threshold check time interval In Milliseconds
	ULONG		FreeThresholdCount;		// Optional, Threshold check counter, mey be not needed 

	// Handle to our driver's ctl device 
	HANDLE		HandleCtl;

	// Thread informtaions, Thread uses Shared Mem for IPC communication with driver
	HANDLE		HThread;	
	HANDLE		HThreadTerminateEvent;
	BOOLEAN		TerminateThread;		// TRUE means Terminate Thread

	PSM_CMD		PtrSM_Cmd;		// Shared memory map Virtual memory pointer.

	// Shared Memory map section information
	BOOLEAN		OpenedSMHandle;	// TRUE means valid values in HFile and HSharedSection
	HANDLE		HFile;
	HANDLE		HFileMap;
	
	HANDLE		H_SMNamedEvent;			// Named Mutex Sync object for Shared MM
	ULONG		ThreadWaitMilliseconds;	
	ULONG		ThreadSleepMilliseconds;	

	//
	MM_SET_DATABASE_SIZE_ENTRIES	MM_Db;

} GMM_INFO, *PGMM_INFO;

//
// Function Prototype Definations
//
BOOLEAN Test_GetSystemInfo(SYSTEM_INFO *SysInfo);
BOOLEAN Test_GlobalMemoryStatusEx( MEMORYSTATUSEX *statex);
BOOLEAN Test_GlobalMemoryStatus( MEMORYSTATUS *statex);
BOOLEAN Test_MM();
BOOL	LoggedSetLockPagesPrivilege ( HANDLE hProcess,BOOL bEnable);

#ifdef _DEBUG

#define DEBUG_BUFFER_LENGTH 512
UCHAR DbgBufferString[DEBUG_BUFFER_LENGTH];

VOID
SwrDebugPrint(
    // ULONG DebugPrintLevel,
    CHAR *DebugMessage,
    ...
    );

#ifdef TEST_AS_EXE
#define	GetErrorText()		
#define OS_DbgPrint(x)		printf(x) // OutputDebugString(x) 
// #define OS_ASSERT(x)		ASSERT(x)
#else
#define	GetErrorText()		
#define OS_DbgPrint(x)		OutputDebugString(x)	// or used fprintf to print log file !!
#endif

#define DebugPrint(x)		SwrDebugPrint x

#else
#define DebugPrint(x)		
#define	GetErrorText()		

#endif

BOOLEAN MMAPI MM_Start();

BOOLEAN MMAPI MM_Stop();

VOID MMAPI MM_Init_default( PGMM_INFO	PtrGMM_Info );

ULONG MMAPI MM_Init_from_registry(PGMM_INFO	PtrGMM_Info);

ULONG MMAPI MM_Init( PGMM_INFO	PtrGMM_Info);

VOID MMAPI MM_DeInit( PGMM_INFO	PtrGMM_Info);

DWORD MMAPI
MM_DeviceIOControl(	HANDLE	Handle,
					ULONG	OperationCode, 
					PVOID	InBuf, 
					ULONG	InLen, 
					PVOID	OutBuf, 
					ULONG	OutLen, 
					PULONG	PtrBytesReturned);

ULONG MMAPI
MM_SharedIPC_Open(PGMM_INFO	PtrGMM_Info);

BOOL MMAPI
MM_SharedIPC_Close(PGMM_INFO	PtrGMM_Info);

ULONG MMAPI
SM_Thread( PGMM_INFO PtrGMM_Info);

DWORD MMAPI
SM_IOCTL_Thread( PGMM_INFO PtrGMM_Info);

//
//	Function:	MM_is_AWESupported()
//	Parameters: 
//	Returns: TRUE means AWE is supported for current process by system else FALSE
//
BOOLEAN MMAPI
MM_is_AWESupported();

//
// Function Prototype Used for Memory Management
//

//
//	Function:	Allocate_AWE_Memory()
//	Parameters: 
//		IN		HANDLE	ProcessHandle:		IN ProcessHandle used to Allocate AWE Pages
//		IN OUT	PULONG	PtrNumberOfPages:	IN Total Number of Pages Asked to Allocate,
//											OUT Total Number of Pages Actually Allocated
//		IN BOOLEAN AllocateArrayMem,		IN TRUE means Allocate Array Memory to hold Physical pages and 
//													retrun pointer of Array Memory into PtrArrayOfMem
//		IN OUT	PULONG	*PtrArrayOfMem 		IN	Pointer to Pointer of Array Of Physical Pages memory 
//												Valid memory if AllocateArrayMem == FALSE
//											OUT it Allocates this memory if AllocateArrayMem == TRUE and Returns 
//												Valid Pointer of memory in this parameter if API returns TRUE	
// Returns: TRUE means successed else FALSE
//
BOOLEAN MMAPI
Allocate_AWE_Memory(IN HANDLE	ProcessHandle, 
					IN PULONG	PtrNumberOfPages, 
					IN BOOLEAN AllocateArrayMem,
					PULONG		*PtrArrayOfMem );

//
//	Function:	Free_AWE_Memory()
//	Parameters: 
//		IN		HANDLE	ProcessHandle:		IN ProcessHandle used to Free AWE Pages
//		IN OUT	PULONG	PtrNumberOfPages:	IN Total Number of Pages to be Free,
//											OUT Total Number of Pages got Free
//		IN		ULONG	FreePagesAtATime,	IN Number of Pages to be free at a time
//		IN		BOOLEAN	FreeArrayMem,		TRUE means free memory for PtrArrayOfMem
//		IN OUT	PULONG	*PtrArrayOfMem )	IN Pointer to Pointer of Array Of Physical Pages memory
//											OUT it frees this memory if FreeArrayMem == TRUE and Returns 
//											NULL in this pointer	
// Returns: TRUE means successed else FALSE
//
BOOLEAN MMAPI
Free_AWE_Memory(	IN HANDLE	ProcessHandle,
					IN OUT	PULONG	PtrNumberOfPages,	
					IN		ULONG	FreePagesAtATime,	
					IN		BOOLEAN	FreeArrayMem,		
					IN OUT	PULONG	*PtrArrayOfMem );

//
//	Function:	Allocate_Virtual_Memory_Pages()
//	Parameters: 
//		IN OUT	PULONG	PtrnumberOfChunks:	IN Total Number of Chunks Asked to Allocate,
//											OUT Total Number of Chunks Actually Allocated
//		IN ULONG	ChunkSize:				IN bytes of one chunksize 
//		IN BOOLEAN AllocateArrayMem,		IN if TRUE means Allocate Array Memory to hold Virtual memory and 
//											retrun pointer of Array Memory into PtrArrayOfMem
//		IN OUT	PULONG	*PtrArrayOfMem 		IN	Pointer to Pointer of Array Of Virtual memory 
//												Valid memory if AllocateArrayMem == FALSE
//											OUT it Allocates this memory if AllocateArrayMem == TRUE and Returns 
//												Valid Pointer of memory in this parameter if API returns TRUE	
// Returns: TRUE means successed else FALSE
//
BOOLEAN MMAPI
Allocate_Virtual_Memory_Pages(	IN PULONG	PtrNumberOfChunks, 
								IN ULONG	ChunkSize,
								IN BOOLEAN	AllocateArrayMem,
								PULONG		*PtrArrayOfMem );

//
//	Function:	Free_Virtual_Memory_Pages()
//	Parameters: 
//		IN OUT	ULONG	NumberOfChunks:		IN Total Number of Chunks in array to be Free,
//		IN		BOOLEAN	FreeArrayMem,		TRUE means free memory for PtrArrayOfMem
//		IN OUT	PULONG	*PtrArrayOfMem )	IN Pointer to Pointer of Array Of Physical Pages memory
//											OUT it frees this memory if FreeArrayMem == TRUE and Returns 
//											NULL in this pointer	
// Returns: TRUE means successed else FALSE
//
BOOLEAN MMAPI
Free_Virtual_Memory_Pages(	IN OUT	ULONG	NumberOfChunks,	
							IN		BOOLEAN	FreeArrayMem,
							IN OUT	PULONG	*PtrArrayOfMem );




#endif