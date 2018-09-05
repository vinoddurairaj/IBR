/*HDR************************************************************************
 *                                                                         
 * Softek - Fujitsu                                    
 *
 *===========================================================================
 *
 * N mmg_ntusr.c
 * P Replicator 
 * S User space functions
 * V Windows Kernel specific
 * A J. Christatos - jchristatos@softek.fujitsu.com
 * D 03.05.2004
 * O User space kernel emulation functions
 * T Cache requirements and design specifications - v 1.0.0.
 * C DBG - 
 * H 03.05.2004 - Creation - 
 *
 *===========================================================================
 *
 * rcsid[]="@(#) $Id: mmgr_ntusr.c,v 1.3 2004/06/17 22:03:27 jcq40 Exp $"
 *
 *HDR************************************************************************/

#include "common.h"

#ifdef   WIN32
#ifdef   _KERNEL_
#include "mmgr_ntkrnl.h"
#else          /* !_KERNEL_*/
#include <windows.h>
#include <stdio.h>
#include "mmgr_ntusr.h"
#endif         /* _KERNEL_ */
#else          /* WIN32    */
#endif         /* _WINDOWS */


/*B**************************************************************************
 * RplUsrInitNPLookAsideList - 
 * 
 * Init lookaside list
 *E==========================================================================
 */
MMG_PUBLIC 
OS_NTSTATUS
RplUsrInitNPLookAsideList(  IN PNPAGED_LOOKASIDE_LIST  Lookaside,
							IN PALLOCATE_FUNCTION  Allocate  OPTIONAL,
							IN PFREE_FUNCTION      Free      OPTIONAL,
							IN ULONG   Flags,
							IN SIZE_T  Size,
							IN ULONG   Tag,
							IN USHORT  Depth )

/**/
{
	OS_NTSTATUS ret   = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplUsrInitNPLookAsideList \n"));

	try {

		Lookaside->size = Size;

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplUsrInitNPLookAsideList \n"));

return(ret);
} /* RplUsrInitNPLookAsideList */

/*B**************************************************************************
 * RplUsrDelNPLookAsideList - 
 * 
 * Delete a lookaside list in user space
 *E==========================================================================
 */
MMG_PUBLIC 
OS_NTSTATUS
RplUsrDelNPLookAsideList(IN PNPAGED_LOOKASIDE_LIST  Lookaside)

/**/
{
	OS_NTSTATUS ret   = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplUsrDelNPLookAsideList \n"));

	try {

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplUsrDelNPLookAsideList \n"));

return(ret);
} /* RplUsrDelNPLookAsideList */

/*B**************************************************************************
 * RplUsrAllocateFromNPagedLookasideList - 
 * 
 * Allocate from a Non paged lookaside list
 *E==========================================================================
 */
MMG_PUBLIC 
PVOID
RplUsrAllocateFromNPagedLookasideList(IN PNPAGED_LOOKASIDE_LIST  Lookaside)

/**/
{
	PVOID ret  = NULL;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplUsrAllocateFromNPagedLookasideList \n"));

	try {

	ret = malloc(Lookaside->size);

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplUsrAllocateFromNPagedLookasideList \n"));

return(ret);
} /* RplUsrDelNPLookAsideList */

/*B**************************************************************************
 * RplUsrFreeToNPagedLookasideList - 
 * 
 * Return an entry to a Non paged lookaside list
 *E==========================================================================
 */

MMG_PUBLIC
VOID
RplUsrFreeToNPagedLookasideList(IN PNPAGED_LOOKASIDE_LIST  Lookaside,
                                IN PVOID  Entry)

/**/
{
	MMGDEBUG(MMGDBG_LOW, ("Entering RplUsrFreeToNPagedLookasideList \n"));

	try {

	free(Entry);

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplUsrFreeToNPagedLookasideList \n"));

} /* RplUsrFreeToNPagedLookasideList */


/*B**************************************************************************
 * RplUsrAllocateSection - 
 * 
 * Allocate a memory section
 *E==========================================================================
 */
MMG_PUBLIC 
OS_NTSTATUS
RplUsrAllocateSection(IN OS_MEMSECTION **memSection, LARGE_INTEGER memSize)

/**/
{
	OS_NTSTATUS ret    = STATUS_SUCCESS;
	HANDLE      Handle, 
			    HdlMap;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplUsrAllocateSection \n"));

	try {

		Handle = CreateFile("ReplicatorFullMm", 
			                GENERIC_READ|GENERIC_WRITE, 
							FILE_SHARE_READ|FILE_SHARE_WRITE,
							NULL, 
							CREATE_ALWAYS, 
							FILE_ATTRIBUTE_NORMAL,
							NULL);

		if (Handle == INVALID_HANDLE_VALUE)
		{
			MMGDEBUG(MMGDBG_FATAL, ("Cannot CreateFile %d \n", GetLastError()));
			ret = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		HdlMap = CreateFileMapping(	Handle, 
									NULL,
									PAGE_READWRITE, 
									memSize.HighPart, // Just map 2 Megs for now
									memSize.LowPart, 
									NULL);

		ret = GetLastError();
		if (ret != STATUS_SUCCESS)
		{
			MMGDEBUG(MMGDBG_FATAL, ("Cannot Create Mapping %d \n", GetLastError()));
			ret = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		*memSection = HdlMap;

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplUsrAllocateSection \n"));

return(ret);
} /* RplUsrAllocateSection */

/*B**************************************************************************
 * RplUsrFreeSection - 
 * 
 * Free a memory seetion
 *E==========================================================================
 */
MMG_PUBLIC 
OS_NTSTATUS
RplUsrFreeSection(IN OS_MEMSECTION *memSection)

/**/
{
	OS_NTSTATUS ret   = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplUsrFreeSection \n"));

	try {

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplUsrFreeSection \n"));

return(ret);
} /* RplUsrFreeSection */

/*B**************************************************************************
 * RplUsrIsValidSection - 
 * 
 * Check integrity of memory section
 *E==========================================================================
 */
MMG_PUBLIC
BOOLEAN
RplUsrIsValidSection(IN OS_MEMSECTION *memSection)

/**/
{
	BOOLEAN ret = TRUE;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplUsrIsValidSection \n"));

	try {

		if (memSection == INVALID_HANDLE_VALUE ||
			memSection == 0 )
		{
			ret = FALSE;
		}

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplUsrIsValidSection \n"));

return(ret);
} /* RplUsrIsValidSection */


/*B**************************************************************************
 * RplUsrKeInitializeEvent - 
 * 
 * Create a new Synchronisation Event
 *E==========================================================================
 */

MMG_PUBLIC
VOID
RplUsrKeInitializeEvent(IN PRKEVENT  Event, IN EVENT_TYPE  Type, IN BOOLEAN  State)

/**/
{
	char   string[256];
	static EventCnt = 1;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplUsrKeInitializeEvent \n"));

	try {

		sprintf(string, "SyncEvent%d", EventCnt++);

		*Event = CreateEvent(NULL, TRUE, FALSE, string);

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplUsrKeInitializeEvent \n"));

} /* RplUsrKeInitializeEvent */

/*B**************************************************************************
 * RplUsrKeWaitForSingleObject - 
 * 
 * Wait on a Synchronisation Event
 *E==========================================================================
 */

MMG_PUBLIC 
OS_NTSTATUS 
RplUsrWaitForSingleObject(IN PVOID           Object,
	                      IN KWAIT_REASON    WaitReason,
						  IN KPROCESSOR_MODE WaitMode,
						  IN BOOLEAN         Alertable,
						  IN PLARGE_INTEGER  Timeout  OPTIONAL    )

/**/
{
	OS_NTSTATUS ret = STATUS_SUCCESS;

	ASSERT(Object);
	MMGDEBUG(MMGDBG_LOW, ("Entering RplUsrKeWaitForSingleObject \n"));

	try {

		ret = WaitForSingleObject(*(HANDLE *)Object, (Timeout)?Timeout->LowPart:INFINITE);

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplUsrKeWaitForSingleObject \n"));

return(ret);
} /* RplUsrKeWaitForSingleObject */



/*
 * Semaphore depth, Only one for now, we only want exclusive access
 */
#define MAX_LOCK 1

/*B**************************************************************************
 * RplUsrInitializeResourceLite - 
 * 
 * Create a new E Resource Object.
 * Simulation is done with a User space Semaphore.
 *E==========================================================================
 */

MMG_PUBLIC
OS_NTSTATUS
RplUsrInitializeResourceLite(IN OS_ERESOURCE *Eresource)

/**/
{
	OS_NTSTATUS  ret = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplUsrInitializeResourceLite \n"));

	try {

		*Eresource = CreateSemaphore(NULL, MAX_LOCK, MAX_LOCK, NULL);
		if (*Eresource == INVALID_HANDLE_VALUE)
		{
			MMGDEBUG(MMGDBG_FATAL, ("Cannot create Semaphore %d \n", GetLastError()));
			ret = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplUsrInitializeResourceLite \n"));

return(ret);
} /* RplUsrInitializeResourceLite */

/*B**************************************************************************
 * RplUsrAcquireResourceExclusiveLite - 
 * 
 * Acquire E Resource Object.
 * Simulation is done with a User space Semaphore.
 *E==========================================================================
 */

BOOLEAN
RplUsrAcquireResourceExclusiveLite(IN OS_ERESOURCE *Eresource, IN BOOLEAN Wait)

/**/
{
	BOOLEAN      ret        = TRUE;
	OS_NTSTATUS  waitstatus = STATUS_SUCCESS;

	ASSERT(Eresource);
	MMGDEBUG(MMGDBG_LOW, ("Entering RplUsrAcquireResourceExclusiveLite \n"));

	try {

		/* Decrement semaphore */
		waitstatus = WaitForSingleObject(*Eresource, ((Wait==TRUE)?INFINITE:0));
		if (!OS_NT_SUCCESS(waitstatus))
		{
			MMGDEBUG(MMGDBG_INFO, ("Cannot acquire sempahore %d \n", GetLastError()));
			ret = FALSE;
		}

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplUsrAcquireResourceExclusiveLite \n"));

return(ret);
} /* RplUsrAcquireResourceExclusiveLite */

/*B**************************************************************************
 * RplUsrReleaseResourceLite - 
 * 
 * Release a E Resource Object.
 * Simulation is done with a User space Semaphore.
 *E==========================================================================
 */

VOID 
RplUsrReleaseResourceLite(IN OS_ERESOURCE  *Eresource)

/**/
{
	MMGDEBUG(MMGDBG_LOW, ("Entering RplUsrReleaseResourceLite \n"));

	try {

		ReleaseSemaphore(*Eresource, 1, NULL);

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplUsrReleaseResourceLite \n"));

} /* RplUsrReleaseResourceLite */

/*B**************************************************************************
 * RplUsrDelayExecutionThread - 
 * 
 * Sleep ... 
 * 
 * Only takes relative times (<0) !
 *
 *E==========================================================================
 */

OS_NTSTATUS 
RplUsrDelayExecutionThread(IN KPROCESSOR_MODE  WaitMode,
						   IN BOOLEAN  Alertable,
						   IN PLARGE_INTEGER  Interval   )
/**/
{
	OS_NTSTATUS  ret = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplUsrDelayExecutionThread \n"));

	try {

		Sleep(Interval->LowPart*-1);

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplUsrDelayExecutionThread \n"));

return(ret);
} /* RplUsrDelayExecutionThread */

/*B**************************************************************************
 * RplUsrKeSetEvent - 
 * 
 * Set an Event  
 *
 *E==========================================================================
 */
LONG
RplUsrSetEvent(IN PRKEVENT  Event,
               IN KPRIORITY Increment,
               IN BOOLEAN   Wait )

/**/
{
	LONG  ret = 0;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplUsrSetEvent \n"));

	try {

		if (SetEvent(*Event)==FALSE)
		{
			MMGDEBUG(MMGDBG_FATAL, ("Cannot Set Event %d  \n", GetLastError()));
			ret = -1;
		}

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplUsrSetEvent \n"));

return(ret);
} /* RplUsrSetEvent */

/*B**************************************************************************
 * RplUsrAllocatePoolWithTag - 
 *  
 *
 *E==========================================================================
 */
PVOID
RplUsrAllocatePoolWithTag(IN POOL_TYPE  PoolType,
						  IN SIZE_T  NumberOfBytes,
						  IN ULONG  Tag
						  )

/**/
{

	MMGDEBUG(MMGDBG_LOW, ("Entering RplUsrAllocatePoolWithTag \n"));

	try {

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplUsrAllocatePoolWithTag \n"));

return(malloc(NumberOfBytes));
} /* RplUsrAllocatePoolWithTag */

/*B**************************************************************************
 * RplUsrFreePool - 
 *  
 *
 *E==========================================================================
 */
VOID
RplUsrFreePool(IN PVOID  p_elem)

/**/
{

	MMGDEBUG(MMGDBG_LOW, ("Entering RplUsrFreePool \n"));

	try {		free(p_elem);	} finally {	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplUsrFreePool \n"));

} /* RplUsrFreePool*/

/*B**************************************************************************
 * RplUsrMapViewOfSection - 
 *  
 * Map the memory pool section in virtual address space.
 *
 *E==========================================================================
 */
OS_NTSTATUS
RplUsrMapViewOfSection(OS_MEMSECTION *p_memsection,
					   vaddr_t       size,
					   vaddr_t       offset,
					   PVOID         *p_mapaddr)

/**/
{
	OS_NTSTATUS    ret = STATUS_SUCCESS;
	LARGE_INTEGER  internal_size;
	LARGE_INTEGER  internal_offset;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplUsrMapViewOfSection \n"));
	try {

		internal_size.QuadPart   = size;
		internal_offset.QuadPart = offset;

		*p_mapaddr = NULL;
		*p_mapaddr = MapViewOfFile(p_memsection, 
			                       FILE_MAP_ALL_ACCESS, 
								   internal_offset.HighPart, 
								   internal_offset.LowPart, 
								   internal_size.LowPart);

		if (*p_mapaddr == NULL)
		{
			MMGDEBUG(MMGDBG_FATAL, ("Cannot map address space \n"));
			ret = STATUS_INVALID_PARAMETER;
			leave;
		}

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplUsrMapViewOfSection \n"));

return(ret);
} /* RplUsrMapViewOfSection */

/*B**************************************************************************
 * RplInterlockedCompareExchange - 
 *  
 * 
 *
 *E==========================================================================
 */
LONG
RplInterlockedCompareExchange(IN OUT PLONG  Destination,
                              IN LONG       Exchange,
                              IN LONG       Comparand    )

/**/
{
	OS_NTSTATUS    ret = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplInterlockedCompareExchange \n"));

	try {

		if (*Destination == Comparand)
			*Destination = Exchange;
		
	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplInterlockedCompareExchange \n"));

return(*Destination);
} /* RplInterlockedCompareExchange */

/*B**************************************************************************
 * RplInterlockedCompareExchange - 
 *  
 * 
 *
 *E==========================================================================
 */
LONG 
RplInterlockedExchange(IN OUT PLONG  Target,
                       IN LONG  Value    )

/**/
{
	OS_NTSTATUS    ret = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplInterlockedExchange \n"));

	try {
			*Target = Value;
		
	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplInterlockedExchange \n"));

return(*Target);
} /* RplInterlockedExchange */

/*B**************************************************************************
 * RplInterlockedCompareExchange - 
 *  
 * 
 *
 *E==========================================================================
 */
PVOID
RplGetCurrentThread()

/**/
{
	OS_NTSTATUS    ret = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplGetCurrentThread \n"));

	try {
		
	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplGetCurrentThread \n"));

return(GetCurrentThread());
} /* RplGetCurrentThread */

/* EOF */