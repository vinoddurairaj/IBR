/**************************************************************************************

Module Name: sftk_os.h   
Author Name: Parag sanghvi
Description: Define all os dependent strcutre, function prototypes, enum and macro 
			 used in the driver. Change or update this file to support code for 
			 other OS.
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#ifndef _SFTK_OS_H_
#define _SFTK_OS_H_

#define try_return(S)   { S; goto try_exit; }
#define try_return1(S)  { S; goto try_exit1; }
#define try_return2(S)  { S; goto try_exit2; }

#ifdef WINDOWS_NT

// Define all NT OS based functions and related structure information
#include <ntddk.h>
#include <ntdddisk.h>

#include <NDIS.H>		// defined in NETDDK
#include <TDI.H>		// defined in NETDDK
#include <TDIKRNL.H>	// defined in NETDDK

#include "stdarg.h"
#include "stdio.h"
#include <ntddvol.h>

#include <mountdev.h>
#include "wmistr.h"
#include "wmidata.h"
#include "wmiguid.h"
#include "wmilib.h"

//
// All OS Dependent API and MAcro defines here, 
//
typedef struct ANCHOR_LINKLIST 
{
    LIST_ENTRY		ListEntry;           // Anchor of link list
    ULONG			NumOfNodes;          // total Num of nodes in link list.
	PVOID			ParentPtr;			 // The Pointer to the Parent of this Anchor Link List.
} ANCHOR_LINKLIST, *PANCHOR_LINKLIST;

#define ANCHOR_InitializeListHead(_anchor_)	{\
					InitializeListHead((&(_anchor_).ListEntry));\
					(_anchor_).NumOfNodes = 0; \
					(_anchor_).ParentPtr = NULL;}


// Transfers Complete Anchor of Link list from one location to another
//
//  VOID
//  MoveSrcGroupListToDstGroupList(
//      PLIST_ENTRY DstListHead,
//      PLIST_ENTRY SrcListHead
//      );
//
#define MoveSrcGroupListToDstGroupList(DstListHead,SrcListHead) {\
		if (IsListEmpty((SrcListHead)))\
			InitializeListHead((DstListHead));\
		else\
		{\
			(SrcListHead)->Flink->Blink = (DstListHead);\
			(SrcListHead)->Blink->Flink = (DstListHead);\
			(DstListHead)->Flink	= (SrcListHead)->Flink;\
			(DstListHead)->Blink	= (SrcListHead)->Blink;\
		}\
	}

#define ANCHOR_MOVE_LIST(_DstAnchor_,_SrcAnchor_)	{\
					MoveSrcGroupListToDstGroupList( &(_DstAnchor_).ListEntry, &(_SrcAnchor_).ListEntry );\
					(_DstAnchor_).NumOfNodes = (_SrcAnchor_).NumOfNodes; \
					ANCHOR_InitializeListHead(_SrcAnchor_);	}


//
// Lock related Definations.
//

// Types of Lock definations
#define OS_ERESOURCE_LOCK		1
#define OS_FAST_MUTEX_LOCK		2
#define OS_SPIN_LOCK			3
#define OS_SEMAPHORE_LOCK		4

// Lock Aquire Access flag 
#define	OS_ACQUIRE_EXCLUSIVE	1
#define	OS_ACQUIRE_SHARED		2

//
// Lock related structures and tehir typedef.
//
typedef struct OS_ERESOURCE 
{
	UCHAR			TypeOfLock;	// must be always First field
    ERESOURCE		Lock;           
} OS_ERESOURCE, *POS_ERESOURCE;

typedef struct OS_KSPIN_LOCK 
{
	UCHAR			TypeOfLock;	// must be always First field
    KSPIN_LOCK		Lock;           
	KIRQL			Irql;		// stores acquired lock calling old Irql, required for release lock
} OS_KSPIN_LOCK, *POS_KSPIN_LOCK;

typedef struct OS_FAST_MUTEX 
{
	UCHAR			TypeOfLock;	// must be always First field
    FAST_MUTEX		Lock;           
} OS_FAST_MUTEX, *POS_FAST_MUTEX;

typedef struct OS_KSEMAPHORE 
{
	UCHAR			TypeOfLock;	// must be always First field
    KSEMAPHORE		Lock;           
} OS_KSEMAPHORE, *POS_KSEMAPHORE;

// Initialize lock
BOOLEAN	
OS_INITIALIZE_LOCK( PVOID	PtrLock, UCHAR TypeOfLock, PVOID Reserved1);

// DeInitialize lock
VOID
OS_DEINITIALIZE_LOCK( PVOID	PtrLock, PVOID Reserved1);


// TRUE means Lock Acquired else Failed to acquired...
BOOLEAN	
OS_ACQUIRE_LOCK( PVOID	PtrLock, UCHAR LockAccess, BOOLEAN Wait, PVOID Reserved1);

// TRUE means Lock Released else Failed to acquired...
BOOLEAN	
OS_RELEASE_LOCK( PVOID	PtrLock, PVOID Reserved1);

// Transfers Complete ListHead from one Anchor location to another Anchor Head
VOID
OS_MOVE_DLINKLIST( IN OUT	PLIST_ENTRY DstListHead,	
				   IN 		PLIST_ENTRY SrcListHead);



// OS_DbgPrint(x) stands for one or more than one argument...
#define OS_DbgPrint(x)		DbgPrint x

// Unicode String Operations related Macros....
#define OS_CopyUnicodeString									RtlCopyUnicodeString
#define OS_AnsiStringToUnicodeString							RtlAnsiStringToUnicodeString

// Memory Operations related Macros....
#define OS_AllocMemory(PoolType,NumberOfBytes)					ExAllocatePool(PoolType,NumberOfBytes)
#define OS_FreeMemory(addr)										ExFreePool(addr)
#define OS_ZeroMemory(Destination,Length)						RtlZeroMemory(Destination,Length)

#define OS_EqualMemory(Source1, Source2 ,Length)				RtlEqualMemory(Source1 , Source2 , Length)

#define OS_RtlCopyMemory										RtlCopyMemory

// Flag (bit-field) manipulation
#define OS_SetFlag(Flag, Value)				((Flag) |= (Value))
#define OS_ClearFlag(Flag, Value)			((Flag) &= ~(Value))

#define OS_IsFlagSet(Flag, Value)			( (((Flag) & (Value)) == 0) ? FALSE:TRUE)

//
//  VOID
//  InsertGroupListToTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY NewList
//      );
//
#define InsertGroupListToTailList(ListHead,NewList) {\
    PLIST_ENTRY _EX_Blink;\
	PLIST_ENTRY	_EX_NewListBlink;\
	_EX_Blink				= (ListHead)->Blink;\
	_EX_NewListBlink		= (NewList)->Blink;\
	_EX_Blink->Flink		= (NewList);\
	_EX_NewListBlink->Flink = (ListHead);\
	(ListHead)->Blink		= _EX_NewListBlink;\
	(NewList)->Blink		= _EX_Blink;\
	}

//
//  VOID
//  InsertGroupListToHeadList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY NewList
//      );
//
#define InsertGroupListToHeadList(ListHead,NewList) {\
    PLIST_ENTRY _EX_Flink;\
	PLIST_ENTRY	_EX_NewListBlink;\
	_EX_Flink				= (ListHead)->Flink;\
	_EX_NewListBlink		= (NewList)->Blink;\
	_EX_Flink->Blink		= _EX_NewListBlink;\
	_EX_NewListBlink->Flink = _EX_Flink;\
	(ListHead)->Flink		= (NewList);\
	(NewList)->Blink		= (ListHead);\
	}
//
// Synchronization Lock ....
//
// Event Operations related Macros....
#define OS_InitializeEvent				KeInitializeEvent

#define OS_KeWaitForSingleObject		KeWaitForSingleObject

// Bitmap Routines and associated macros
#define OS_RTL_BITMAP					RTL_BITMAP


// We are not defining All realive RTL Bitmap routines as OS indepandent, in reverse 
// we can #define RTL bitmap routines in other OS and do coding for Bitmap routines...
// is this fine ?


// Macro Definations 
// --------- Performance Macro Definations -----------------
#define PERF_MONITOR_ENABLED			// default make it on

#ifdef PERF_MONITOR_ENABLED

// 
// VOID OS_PerfGetClock(	PLARGE_INTEGER	PtrCurrentTimeReturned, 
//								PLARGE_INTEGER	Reserve			OPTIONAL )
//
//	Parameters:	PtrCurrentTimeReturned	- returns current system time.
//				Reserve					- Its optional for future usage, Pass NULL for time being
//
//	Descriptions: 
//	System time is a count of 100-nanosecond intervals since January 1, 1601. System time is 
//	typically updated approximately every ten milliseconds. This value is computed for the 
//	GMT time zone. To adjust this value for the local time zone use ExSystemTimeToLocalTime.
//
//	Callers of OS_PerfGetClock() (means KeQuerySystemTime) can be running at any IRQL.
//
#define OS_PerfGetClock(PtrCurrentTimeReturned, Reserve)	KeQuerySystemTime(PtrCurrentTimeReturned)

// 
// VOID OS_PerfGetDiffTime(		PLARGE_INTEGER	PtrStartTime, 
//								PLARGE_INTEGER	PtrEndTime,	
//								PLARGE_INTEGER	PtrDiffTime)
//
//	Parameters:	PtrStartTime-	Pass Start Time returned from OS_PerfGetClock()
//				PtrStartTime-	returns current system time to caller, 
//				PtrDiffTime	-	Returns Difference time in 100 nanoseconds units.
//
//	Descriptions: 
//	System time is a count of 100-nanosecond intervals since January 1, 1601. System time is 
//	typically updated approximately every ten milliseconds. This value is computed for the 
//	GMT time zone. To adjust this value for the local time zone use ExSystemTimeToLocalTime.
//
//	Callers of OS_PerfGetDiffTime() (means KeQuerySystemTime) can be running at any IRQL.
//

#define OS_PerfGetDiffTime( PtrStartTime, PtrEndTime, PtrDiffTime)		 	\
					{	OS_PerfGetClock(PtrEndTime, NULL); \
						(PtrDiffTime)->QuadPart = (PtrEndTime)->QuadPart - (PtrStartTime)->QuadPart;}

// definations of local variables used for Performance tracing only.
#define OS_PERF								LARGE_INTEGER	startTime;	
#define OS_PERF_STARTTIME					OS_PerfGetClock(&startTime,NULL);	
#define OS_PERF_ENDTIME(_Type_,_WorkLoad_)	sftk_perf_monitor(_Type_, &startTime,_WorkLoad_);

#else // #ifdef PERF_MONITOR_ENABLED

// definations of local variables used for Performance tracing only.
#define OS_PERF									{}
#define OS_PERF_STARTTIME						{}
#define OS_PERF_ENDTIME(Type,WorkLoad)			{}

#endif // #ifdef PERF_MONITOR_ENABLED


#endif // #ifdef WINDOWS_NT

#ifdef UNIX
// For Unix OS based definations.
#endif

//
// Following API are defined in sftk_os.c file
// 
VOID
OS_ULong_SetBit( PULONG	UBit, UCHAR	BitNo);

VOID
OS_ULong_ClearBit( PULONG	UBit, UCHAR	BitNo);

VOID
OS_ULong_SetBitRange( PULONG	UBit, UCHAR	StartingBitNo, UCHAR NumOfBit);

VOID
OS_ULong_ClearBitRange( PULONG	UBit, UCHAR	StartingBitNo, UCHAR NumOfBit);

BOOLEAN
OS_ULong_IsBitSet( PULONG	UBit, UCHAR	BitNo);

BOOLEAN
OS_ULong_IsBitClear( PULONG	UBit, UCHAR	BitNo);

UCHAR
OS_ULong_GetNextSetBit( PULONG	UBit, UCHAR	StartingBitIndex);

UCHAR
OS_ULong_GetNextClearBit( PULONG	UBit, UCHAR	StartingBitIndex);

#endif // _SFTK_OS_H_