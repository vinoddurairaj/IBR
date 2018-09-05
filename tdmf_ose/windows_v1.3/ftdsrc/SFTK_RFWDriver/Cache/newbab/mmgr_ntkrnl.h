/*HDR************************************************************************
 *                                                                         
 * Softek - Fujitsu                                    
 *
 *===========================================================================
 *
 * N mmg_ntkrnl.h
 * P Replicator 
 * S OS definitions, prototypes
 * V Windows Kernel specific
 * A J. Christatos - jchristatos@softek.fujitsu.com
 * D 03.04.2004
 * O OS macros
 * T Cache requirements and design specifications - v 1.0.0.
 * C DBG - _KERNEL_ 
 * H 03.04.2004 - Creation - 
 *
 *===========================================================================
 *
 * rcsid[]="@(#) $Id: mmgr_ntkrnl.h,v 1.2 2004/06/17 22:03:27 jcq40 Exp $"
 *
 *HDR************************************************************************/

#ifndef _MMG_NTKRNL_
#define _MMG_NTKRNL_

#if defined(DBG)
extern  MMG_PUBLIC ULONG MmgDebugLevel;
#define DBGSTATIC static
#define MMGDEBUG(LEVEL, STRING)    if (LEVEL <= MmgDebugLevel)   \
                                   {                             \
								   if (LEVEL == MMGDBG_FATAL) {  \
								   DbgPrint("***");              \
								   }                             \
								   DbgPrint STRING;              \
								   }
#endif /* DBG */

/* 
 * Emulate ftd_config from services
 */
typedef struct 
{
	int dummy;

} ftd_lg_info_t;


/* Generic layer is started on Windows
 * So it is almost a one-2-one mapping
 */
#define OS_NTSTATUS                        NTSTATUS
#define OS_NT_SUCCESS                      NT_SUCCESS
#define OS_LIST_ENTRY                      LIST_ENTRY
#define OS_MUTEX_LOCK                      KMUTEX
#define OS_ERESOURCE                       ERESOURCE
#define OS_SYNC_EVENT                      KEVENT
#define OS_LOOKASIDE					   NPAGED_LOOKASIDE_LIST
#define OS_MEMSECTION                      HANDLE
#define OS_ExInitializeNPagedLookasideList ExInitializeNPagedLookasideList 
#define OS_ExDeleteNPagedLookasideList     ExDeleteNPagedLookasideList 
#define OS_AllocateSection                 RplKrnlAllocateSection
#define OS_FreeSection					   RplKrnlFreeSection
#define OS_IsValidSection				   RplKrnlIsValidSection


#define OS_ExFreeToNPagedLookasideList        ExFreeToNPagedLookasideList
#define OS_ExAllocateFromNPagedLookasideList  ExAllocateFromNPagedLookasideList
#define OS_KeInitializeEvent                  KeInitializeEvent
#define OS_ExInitializeResourceLite           ExInitializeResourceLite
#define OS_ExAcquireResourceExclusiveLite     ExAcquireResourceExclusiveLite
#define OS_ExReleaseResourceLite              ExReleaseResourceLite
#define OS_KeDelayExecutionThread             KeDelayExecutionThread
#define OS_KeWaitForSingleObject              KeWaitForSingleObject
#define OS_ExAllocatePoolWithTag			  ExAllocatePoolWithTag
#define OS_ExFreePool                         ExFreePool
#define OS_KeSetEvent                         KeSetEvent
#define OS_InterlockedExchange                InterlockedExchange
#define OS_InterlockedCompareExchange         InterlockedCompareExchange
#define OS_KeGetCurrentThread                 KeGetCurrentThread


BOOLEAN
RplKrnlAllocateSection(IN OS_MEMSECTION  *memSection, IN LARGE_INTEGER size);

NTSTATUS
RplKrnlFreeSection(IN OS_MEMSECTION  *memSection);

BOOLEAN
RplKrnlIsValidSection(IN OS_MEMSECTION  *memSection);

VOID
OS_FsRtlDissectName(IN  UNICODE_STRING  u1,
				    OUT PUNICODE_STRING p_u2,
				    OUT PUNICODE_STRING p_u3);


#endif _MMG_NTKRNL_
/* EOF */
