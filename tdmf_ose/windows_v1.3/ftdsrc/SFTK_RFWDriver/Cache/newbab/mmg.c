/*HDR************************************************************************
 *                                                                         
 * Softek -                           
 *
 *===========================================================================
 *
 * N mmr.c
 * P Replicator 
 * S Cache & FIFO
 * V Generic
 * A J. Christatos - jchristatos@softek.com
 * D 02.25.2004
 * O Generic memory manager functions & Cache Interface
 * T Cache design requirements and design specifications - v 1.0.0.
 * C DBG - _KERNEL_
 * H 02.25.2004 - Creation - 
 *
 *===========================================================================
 *
 * rcsid[]="@(#) $Id: mmg.c,v 1.6 2004/06/17 22:03:27 jcq40 Exp $"
 *
 *
 *HDR************************************************************************/

#include "common.h"

#ifdef   WIN32
#ifdef   _KERNEL
#include <ntddk.h>
#include <wchar.h>      // for time_t, dev_t
#include "mmgr_ntkrnl.h"
#else    /* !_KERNEL_*/
#include <windows.h>
#include <stdio.h>
#include <sys/types.h>
#include "mmgr_ntusr.h"
#endif   /* _KERNEL_ */
#endif   /* _WINDOWS */

#include "slab.h"
#include "mmg.h"
#include "sftkprotocol.h"
#include "dtb.h"


/*
 ********************************** PRIVATE Data structures *********************************************
 */

/*
 * One Pool Entry size
 */
#define POOL_ENTRYSZ(CHUNKSIZE) (CHUNKSIZE+sizeof(header_t))

/*
 * A memory pool 
 */

typedef enum 
{
	TDI_POOL     = 1,
	REFRESH_POOL = 2

} pooltype_t;


typedef struct 
{
	signatures_e  sigtype;	       // Who are we ?
	pooltype_t    type;

	OS_LIST_ENTRY lnk;		       // next pool
	ULONG         AtomicLock;      // A pseudo spinlock
	Policy_t      policy;          // wrap or not ?

	ULONG         flags;           // state of this pool
#define RPLCC_POOL_INIT   (1)      // node Initialized
#define RPLCC_POOL_WAITER (2)      // Waiter for block
#define RPLCC_POOL_LOCKED (4)      // The pool is locked.

	int           entry_in_use;    // how many entries in use [0..numchunk[
	int           chunk_size;	   // one IO chunk
	int           num_chunk;       // number of chunksize

	OS_SYNC_EVENT WaitForBlock;    // Wait for a Free block.

	PVOID         p_startaddr;     // start @ for user consumption 
	PVOID         p_endaddr;   
	PVOID         p_current;       // the current next free entry

    // callback to be trigerred when new free buffer

	OS_NTSTATUS  (*callback)(PVOID pBuffer, PVOID p_ctxt);
	PVOID        p_ctxt;

} pool_t;

/*
 * Main header for each new IO in the cache.
 */

typedef struct 
{
	signatures_e  sigtype;	       // Who are we ?
	ULONG         flags;           // state of this header
#define RPLCC_HDR_INIT   (1)       // node Initialized
#define RPLCC_HDR_INDTB  (2)       // We are in the database.
#define RPLCC_HDR_FREE   (4)       // This header is free
#define RPLCC_HDR_INUSE  (8)       // This header is in USE.
#define RPLCC_HDR_COMMIT (0x10)    // This header is in the COMMIT Q.
#define RPLCC_HDR_MIGR   (0x20)    // This header is in the MIGRATE Q.
#define RPLCC_HDR_REFQ   (0x40)    // This header is in the Refresh Q.
#define RPLCC_HDR_POOL   (0x80)    // This header is part of a 'special pool
	                               // Don't allocate.

	pool_t      *p_pool;             // back ptr to pool header if HDR_POOL defined.

	dtbnode_t   dtb_info;        // if the header is in database.
	wlheader_t  *p_oldheader;    // old compatible header for secondary
	PVOID       p_data;          // The data 

} header_t;

/* 
 * This is the anchor for the migration Q.
 */
typedef struct _migrate_anchor_
{
	signatures_e  sigtype;	    // Who are we ?

	OS_LIST_ENTRY miga_lnk;     // linked list on migrate Q
	OS_LIST_ENTRY hdr_lnk;      // linked list of headers

} migrate_anchor_t;

/*
 * The main data structure : The cache anchor
 */

typedef struct 
{
	signatures_e  sigtype;	       // Who are we ?

	slab_t		  slab_instance;

	ULONG         AtomicLock;      // A pseudo spinlock
	OS_LIST_ENTRY lg_lnk;          // nxt LG for cache.
	OS_LIST_ENTRY pool_lnk;        // 
	OS_LOOKASIDE  db_node;
	OS_LOOKASIDE  lg_lookaside;
	OS_LOOKASIDE  pool_lookaside;
	
	LARGE_INTEGER memorySize;     // Max size of memory section
	int           mem_threshold;  // Threshold for pool vs fifo
	
	int (* AllocCallBack)(), 
		(* FreeCallBack)();
	
	// Statistics

} thecache_t;

/*
 * Global Data (NpagedPool)
 */

MMG_PUBLIC thecache_t RplCCCache;
MMG_PUBLIC ULONG MmgDebugLevel = MMGDBG_HIGH;

/*B**************************************************************************
 * _RplCCacheConfig - 
 * 
 * Configure the lcache from registry parameters.                                                                                          
 *
 *E==========================================================================
 */

MMG_PRIVATE
OS_NTSTATUS
_RplCCacheConfig(ftd_lg_info_t *p_config)

/**/
{
	OS_NTSTATUS      ret         = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Entering _RplCCacheConfig \n"));

	try {

	} finally {
	}

	MMGDEBUG(MMGDBG_LOW, ("leaving _RplCCacheConfig \n"));

return(ret);
} /* _RplCCacheConfig */

/*B**************************************************************************
 * _RplCCRemovePendingE - 
 * 
 * Remove Entry from the pending Q.
 *
 * ASSUME: 
 * 1) Locks on list has been taken by caller !
 * 2) Memory has been unpinned.
 *
 *E==========================================================================
 */

MMG_PRIVATE
OS_NTSTATUS
_RplCCRemovePendingE(CC_logical_grp_t *LgPtr,
					 header_t         *p_hdr)

/**/
{
	OS_NTSTATUS      ret = STATUS_SUCCESS;

	ASSERT(LgPtr);
	ASSERT(LgPtr->sigtype == RPLCC_LG);
	ASSERT(LgPtr->flags & RPLCC_LG_LOCKED);
	ASSERT(p_hdr);
	ASSERT(p_hdr->sigtype == RPLCC_HDR);
	MMGDEBUG(MMGDBG_LOW, ("Entering _RplCCRemovePendingE \n"));

	try {

		if (p_hdr->flags & RPLCC_HDR_INDTB)
		{
			MMGDEBUG(MMGDBG_INFO, ("Deleting node from Pending Q \n"));
			LgPtr->total_pending  -= p_hdr->dtb_info.Size ;
			RemoveEntryList(&p_hdr->dtb_info.lnk);
		}

		/*
		 * Return header and associated data 
		 * Assume p_fifo->nb_entries points 
		 * to current data.
		 */
		if (p_hdr->flags & RPLCC_HDR_INIT)
		{
			MMGDEBUG(MMGDBG_INFO, ("Deleting header \n", ret));
			p_hdr->flags = 0;
			RplSlabFree(&RplCCCache.slab_instance,
						p_hdr->p_data);
			RplSlabFree(&RplCCCache.slab_instance,
						(PVOID )p_hdr);
		}

		OS_ExFreeToNPagedLookasideList(&LgPtr->hdr_alist, p_hdr);

	} finally {
	}

	MMGDEBUG(MMGDBG_LOW, ("leaving _RplCCRemovePendingE \n"));

return(ret);
} /*_RplCCRemovePendingE */

/*B**************************************************************************
 * RplCCInit - 
 * 
 *  Init The cache at driver init time.
 *  Register callback to allocate/free memory.
 *  Initialize the memory manager.                                                                                                         
 *
 *E==========================================================================
 */

MMG_PUBLIC
OS_NTSTATUS 
RplCCInit(int (*AllocCallBack)(),
		  int (*FreeCallBack)(), 
		  int mem_threshold,
		  ftd_lg_info_t *p_config)

/**/
{
	OS_NTSTATUS ret = STATUS_SUCCESS;

	ASSERT(mem_threshold != 0);
	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCInit \n"));

	try {

		RtlFillMemory(&RplCCCache, sizeof(thecache_t), 0);
		RplCCCache.sigtype       = RPLCC_CC;
		RplCCCache.FreeCallBack  = FreeCallBack;
		RplCCCache.AllocCallBack = AllocCallBack;
		RplCCCache.mem_threshold = mem_threshold;
		RplCCCache.memorySize.QuadPart    = 8*1024*1024;
		OS_InterlockedExchange(&RplCCCache.AtomicLock, ATOMIC_FREE);

		ret = RplSlabInit(&RplCCCache.slab_instance, 
			              RplCCCache.memorySize);

		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_LOW, ("Cannot initialize slab allocator %x \n", ret));
			leave;
		}

		/* node list for memory 'special' pools */
		OS_ExInitializeNPagedLookasideList(
				&RplCCCache.pool_lookaside, 
				NULL,  // Allocate callback
				NULL,  // Free callback
				0,     // reserved flag
				sizeof(pool_t),
				'loop',
				0      // reserved depth
				);
	
		/* Node list for logical groups */
		OS_ExInitializeNPagedLookasideList(
				&RplCCCache.lg_lookaside, 
				NULL,  // Allocate callback
				NULL,  // Free callback
				0,     // reserved flag
				sizeof(CC_logical_grp_t),
				'loop',
				0      // reserved depth
				);

#ifdef NO_TREE
		/* node list for root database entities */
		OS_ExInitializeNPagedLookasideList(
				&RplCCCache.db_node, 
				NULL,  // Allocate callback
				NULL,  // Free callback
				0,     // reserved flag
				SizeOfDtbNode(),
				'loop',
				0      // reserved depth
				);
#endif // NO_TREE

		InitializeListHead(&RplCCCache.pool_lnk);
		InitializeListHead(&RplCCCache.lg_lnk);

		ret = _RplCCacheConfig(p_config);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_LOW, ("cannot configure the cache %x \n", ret));
			leave;
		}

	} finally {

		/*
		 * Error ? rollback...
		 */
		if (!OS_NT_SUCCESS(ret))
		{
			if (IsValidSlab(&RplCCCache.slab_instance)==TRUE)
			{
				RplSlabDelete(&RplCCCache.slab_instance);
			}
			
			OS_ExDeleteNPagedLookasideList(&RplCCCache.pool_lookaside);
			OS_ExDeleteNPagedLookasideList(&RplCCCache.lg_lookaside);
		}
	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCInit \n"));

return(ret);
} /* RplCCInit */
 
/*B**************************************************************************
 * _RplCCLgConfig - 
 * 
 * Configure the logical group from registry parameters.                                                                                          
 *
 *E==========================================================================
 */

MMG_PRIVATE
OS_NTSTATUS
_RplCCLgConfig(ftd_lg_info_t *p_config)

/**/
{
	OS_NTSTATUS      ret         = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Entering _RplCCLgConfig \n"));


	try {

	} finally {
	}

	MMGDEBUG(MMGDBG_LOW, ("leaving _RplCCLgConfig \n"));

return(ret);
} /* _RplCCLgConfig */

/*B**************************************************************************
 * RplCCLgInit - 
 * 
 * Register a  Logical Group with the cache.
 * Allocate a memory manager structure from pool of memory manager.                                                                                               
 *
 *E==========================================================================
 */

OS_NTSTATUS
RplCCLgInit(PVOID            DeviceLG,
			CC_logical_grp_t **LgPtr,
			CC_lgstate_t     (*GetState)(PVOID),
			VOID             (*RFFreeCB)(PVOID),
			PVOID            RFFreeCtxt,
			VOID             (*MGFreeCB)(PVOID),
			PVOID            MGFreeCtxt,
			ftd_lg_info_t    *p_config)

/**/
{
	OS_NTSTATUS       ret        = STATUS_SUCCESS;
	CC_logical_grp_t  *p_lg      = NULL;
	LARGE_INTEGER     timeout    = {-1000, 0};

	ASSERT(DeviceLG);
	ASSERT(LgPtr);
	ASSERT(RplCCCache.sigtype == RPLCC_CC);
	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCLgInit \n"));

	try {

		p_lg = (CC_logical_grp_t *)OS_ExAllocateFromNPagedLookasideList(&RplCCCache.lg_lookaside);
		if (p_lg == NULL)
		{
			MMGDEBUG(MMGDBG_LOW, ("Cannot Allocate from Lookasidelist \n"));
			ret = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		/*
		 * populate the new logical group
		 */
		RtlFillMemory(p_lg, sizeof(CC_logical_grp_t), 0);
		p_lg->sigtype      = RPLCC_LG;
		p_lg->nb_devices   = 0;
	    p_lg->hotgrp_limit = 0;
		p_lg->p_backtr     = DeviceLG;
		p_lg->GetState     = GetState;

		p_lg->MigrateQFreeCB = MGFreeCB;
		p_lg->RefreshQFreeCB = RFFreeCB;

		p_lg->RefreshQFreeCtxt = RFFreeCtxt;
		p_lg->MigrateQFreeCtxt = MGFreeCtxt;

		OS_KeInitializeEvent(&p_lg->waitForBlock, SynchronizationEvent, FALSE);
	    OS_KeInitializeEvent(&p_lg->waitForData, SynchronizationEvent, FALSE);
		OS_InterlockedExchange(&p_lg->AtomicLock, ATOMIC_FREE);
		
		InitializeListHead(&p_lg->lnk);
		InitializeListHead(&p_lg->PendingQ);
		InitializeListHead(&p_lg->CommitQ);
		InitializeListHead(&p_lg->migrateQ);
		InitializeListHead(&p_lg->refreshQ);

		/* node list for 'hard' headers */
		OS_ExInitializeNPagedLookasideList(
				&p_lg->hdr_alist, 
				NULL,  // Allocate callback
				NULL,  // Free callback
				0,     // reserved flag
				sizeof(header_t),
				'loop',
				0      // reserved depth
				);

		/* node list for migrate Q */
		OS_ExInitializeNPagedLookasideList(
				&p_lg->miga_alist, 
				NULL,  // Allocate callback
				NULL,  // Free callback
				0,     // reserved flag
				sizeof(migrate_anchor_t),
				'loop',
				0      // reserved depth
				);

		/* Now MP safely Insert in Cache link.
		 * Use a 32bits ptr as a uniq id for 
		 * the thread until we find a better
		 * way for 64bits.
		 */
		while(OS_InterlockedCompareExchange(&RplCCCache.AtomicLock, 
			                                (LONG)OS_KeGetCurrentThread(), 
			                                ATOMIC_FREE) != (LONG)OS_KeGetCurrentThread())
		{
			MMGDEBUG(MMGDBG_LOW, ("Waiting for cache lock... \n"));
			OS_KeDelayExecutionThread(KernelMode, FALSE, &timeout);
		}

		InsertTailList(&RplCCCache.lg_lnk, &p_lg->lnk);
		OS_InterlockedExchange(&RplCCCache.AtomicLock, ATOMIC_FREE);
		
		p_lg->total_pending  = 0;
		p_lg->total_commit   = 0;

		/*
		 * Now call the configuration part
		 */
		ret = _RplCCLgConfig(p_config);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Cannot configure the Logical group ... \n"));
			leave;
		}

		p_lg->flags = RPLCC_LG_INIT;
    	*LgPtr = p_lg;

	} finally {

		/*
		 * Rollback in case of error
		 */
		if (!OS_NT_SUCCESS(ret) && p_lg != NULL)
		{
			if (p_lg->flags == RPLCC_LG_INIT)
			{
				/* Now MP safely Insert in Cache link */
				while(OS_InterlockedCompareExchange(&RplCCCache.AtomicLock, 
			                                (LONG)OS_KeGetCurrentThread(), 
			                                ATOMIC_FREE) != (LONG)OS_KeGetCurrentThread())
				{
					MMGDEBUG(MMGDBG_LOW, ("Waiting for cache lock... \n"));
					OS_KeDelayExecutionThread(KernelMode, FALSE, &timeout);
				}
				RemoveEntryList(&p_lg->lnk);
				OS_InterlockedExchange(&RplCCCache.AtomicLock, ATOMIC_FREE);
				OS_ExDeleteNPagedLookasideList(&p_lg->hdr_alist);
				OS_ExDeleteNPagedLookasideList(&p_lg->miga_alist);
			}

			OS_ExFreeToNPagedLookasideList(&RplCCCache.lg_lookaside, p_lg);
		}

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCLgInit \n"));

return(ret);
} /* RplCCLgInit */

/*B**************************************************************************
 * RplCCAllocateSpecialPool - 
 * 
 * Reserve a contiguous range of memory of given size for either TDI thread
 * or Refresh thread from Slab allocator.
 *
 * The call can round up the size.
 *
 *E==========================================================================
 */

OS_NTSTATUS
RplCCAllocateSpecialPool(IN OUT PoolDescr_t *p_pool)

/**/
{
	OS_NTSTATUS    ret     = STATUS_SUCCESS;
	LARGE_INTEGER  timeout = {-1000, 0};
	header_t       *p_hdr  = NULL;
	pool_t         *p_thepool = NULL;
    int            total_size;
	int            current_chunk;

	ASSERT(p_pool);
	ASSERT(p_pool->sigtype);
	ASSERT(p_pool->poolt >= RPLCC_TDI_POOL && p_pool->poolt <= RPLCC_REFRESH_POOL);

	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCAllocateSpecialPool \n"));

	try {

		/*
		 * Allocate a pool node from the lookaside list
		 */
		p_thepool = OS_ExAllocateFromNPagedLookasideList(&RplCCCache.pool_lookaside);
		if (p_thepool == NULL)
		{
			MMGDEBUG(MMGDBG_LOW, ("Entering RplCCAllocateSpecialPool \n"));
			ret = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		p_thepool->sigtype       = RPLCC_POOL;
		p_thepool->entry_in_use  = 0;
		p_thepool->type          = p_pool->poolt;

		InitializeListHead(&p_thepool->lnk);
		OS_KeInitializeEvent(&p_thepool->WaitForBlock, SynchronizationEvent, FALSE);
		OS_InterlockedExchange(&p_thepool->AtomicLock, ATOMIC_FREE);
		
		p_pool->Id       = 1;
		p_pool->policy   = WRAP;
		total_size       = POOL_ENTRYSZ(p_pool->chunk_size) * p_pool->num_chunk;
		p_pool->p_start  = RplSlabAllocate(&RplCCCache.slab_instance, total_size, &ret);
		if (p_pool->p_start == NULL)
		{
			MMGDEBUG(MMGDBG_FATAL, ("Cannot Allocate from slab \n"));
			p_pool->Id = -1; 
			ret = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		p_pool->p_internal     = (PVOID )p_thepool;
		p_thepool->chunk_size  = POOL_ENTRYSZ(p_pool->chunk_size);
		p_thepool->num_chunk   = p_pool->num_chunk;
		p_thepool->policy      = p_pool->policy;
		p_thepool->p_startaddr = p_pool->p_start;
		p_thepool->p_current   = p_pool->p_start;
		p_thepool->p_endaddr   = p_pool->p_start;
		(char *)p_thepool->p_endaddr += p_thepool->chunk_size * p_pool->num_chunk;
		

		/*
		 * Now initialize each header entry at chunksize boundary
		 * to say that memory was from Pool so that RplCCWrite() 
		 * and RplCCGet.... can identify them.
		 *
		 * For RPLCC_HDR_POOL type headers header/ data are
		 * contiguous in memory.
		 */
		p_hdr = (header_t *)p_pool->p_start;
		current_chunk = 0;
		while (current_chunk < p_pool->num_chunk)
		{
			/*
			 * The chunk size is not multiple of entries size.
			 * We just bail out without errors.
			 */
			if ((PVOID)p_hdr >= p_thepool->p_endaddr)
			{
				MMGDEBUG(MMGDBG_INFO, ("Chunk size pool not aligned to entry size \n"));
				break;
			}
			
			p_hdr->sigtype  = RPLCC_HDR;
			p_hdr->flags    = RPLCC_HDR_POOL|RPLCC_HDR_FREE;
			p_hdr->p_pool   = p_thepool;

			InitializeListHead(&p_hdr->dtb_info.lnk);

			// This is the end condition
			// for the Btree.
			//p_hdr->dtb_info.lnk.Blink =
			//p_hdr->dtb_info.lnk.Flink = NULL;   
			
			/*
			 * 04.29.04-
			 * The refresh pool does not use 
			 * soft headers.
			 *
			 * p_hdr->p_oldheader = (wlheader_t *)p_hdr;
			 * (char *)p_hdr->p_oldheader += sizeof(header_t);
			 * p_hdr->p_data = p_hdr->p_oldheader;
			 * (char *)p_hdr->p_data += sizeof(wlheader_t);
			 */
			p_hdr->p_data = p_hdr;
			(char *)p_hdr->p_data += sizeof(header_t);
			(char *)p_hdr         += p_thepool->chunk_size;
			current_chunk++;
		}

		/*
		 * The pool is now valid, insert it inside the cache link
		 */
				
		while(OS_InterlockedCompareExchange(&RplCCCache.AtomicLock, 
			                                (LONG)OS_KeGetCurrentThread(), 
			                                ATOMIC_FREE) != (LONG)OS_KeGetCurrentThread())
		{
			MMGDEBUG(MMGDBG_LOW, ("Waiting for cache lock... \n"));
			OS_KeDelayExecutionThread(KernelMode, FALSE, &timeout);
		}
		InsertTailList(&RplCCCache.pool_lnk, &p_thepool->lnk);
		OS_InterlockedExchange(&RplCCCache.AtomicLock, ATOMIC_FREE);

		/*
		 * Properly initilalized
		 */
		p_thepool->flags = RPLCC_POOL_INIT;

	} finally {

		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_LOW, ("Error rolling back ... \n"));
			if (p_thepool != NULL)
			{
				if (!(p_thepool->flags & RPLCC_POOL_INIT))
				{
					/* Now MP safely Insert in Cache link */
		            while(OS_InterlockedCompareExchange(&RplCCCache.AtomicLock, 
														(LONG)OS_KeGetCurrentThread(), 
														ATOMIC_FREE) != (LONG)OS_KeGetCurrentThread())
					{
						MMGDEBUG(MMGDBG_LOW, ("Waiting for cache lock... \n"));
						OS_KeDelayExecutionThread(KernelMode, FALSE, &timeout);
					}

					RemoveEntryList(&p_thepool->lnk);
					OS_InterlockedExchange(&RplCCCache.AtomicLock, ATOMIC_FREE);
					p_thepool->flags &= ~RPLCC_POOL_INIT;
				}
				RplSlabFree(&RplCCCache.slab_instance, p_thepool->p_startaddr);
				OS_ExFreeToNPagedLookasideList(&RplCCCache.pool_lookaside,
					                           p_thepool);
			}
		}
	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCAllocateSpecialPool \n"));

return(ret);
} /* RplCCAllocateSpecialPool */


/*B**************************************************************************
 * RplCCFreeSpecialPool - 
 * 
 * Call slab allocator to free Memory for this special Pool.
 *
 *E==========================================================================
 */

OS_NTSTATUS
RplCCFreeSpecialPool(IN PoolDescr_t *p_pool)

/**/
{
	OS_NTSTATUS    ret        = STATUS_SUCCESS;
	LARGE_INTEGER  timeout    = {-1000, 0};
	pool_t         *p_thepool = NULL;

	ASSERT(p_pool);
	ASSERT(p_pool->sigtype);
	ASSERT(p_pool->poolt >= RPLCC_TDI_POOL && p_pool->poolt <= RPLCC_REFRESH_POOL);

	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCFreeSpecialPool \n"));

	try {

		/* Get internal pool structure */
		p_thepool = (pool_t *)p_pool->p_internal;
		ASSERT(p_thepool);

		/* Now MP safely Insert in Cache link */
		while(OS_InterlockedCompareExchange(&p_thepool->AtomicLock, 
			                                (LONG)OS_KeGetCurrentThread(), 
			                                ATOMIC_TAKEN)!=(LONG)OS_KeGetCurrentThread())
		{
			MMGDEBUG(MMGDBG_LOW, ("Waiting for lock... \n"));
			OS_KeDelayExecutionThread(KernelMode, FALSE, &timeout);
		}
		p_thepool->flags |= RPLCC_POOL_LOCKED;

		ret = RplSlabFree(&RplCCCache.slab_instance, p_pool->p_start);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Cannot free pool \n"));
			p_pool->Id = -1; 
			leave;
		}
		
		if (!(p_thepool->flags & RPLCC_POOL_INIT))
		{

			/* Now MP safely Insert in Cache link */
			while(OS_InterlockedCompareExchange(&RplCCCache.AtomicLock, 
				                                (LONG)OS_KeGetCurrentThread(), 
				                                ATOMIC_TAKEN)!=(LONG)OS_KeGetCurrentThread())
			{
				MMGDEBUG(MMGDBG_LOW, ("Waiting for lock... \n"));
				OS_KeDelayExecutionThread(KernelMode, FALSE, &timeout);
			}
			RemoveEntryList(&p_thepool->lnk);
			OS_InterlockedExchange(&RplCCCache.AtomicLock, ATOMIC_FREE);
			p_thepool->flags &= ~RPLCC_POOL_INIT;
		}

		OS_ExFreeToNPagedLookasideList(&RplCCCache.pool_lookaside,
									   p_thepool);

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCFreeSpecialPool \n"));

return(ret);
} /* RplCCFreeSpecialPool */

/*B**************************************************************************
 * RplCCGetNextBfromPool - 
 * 
 * Return next free buffer (not pending/not committed state) from the 
 * circular pool.
 * If no available buffers: sleep.
 * Pbuffer is align to the “soft header”.
 *
 *E==========================================================================
 */

OS_NTSTATUS
RplCCGetNextBfromPool(IN PoolDescr_t  *p_pool,
					  OUT PVOID       *p_callerbuf,
					  IN OS_NTSTATUS  (*callback)(PVOID pBuffer, PVOID p_ctxt),
					  IN PVOID        p_ctxt
					  )

/**/
{
	OS_NTSTATUS    ret        = STATUS_SUCCESS;
	LARGE_INTEGER  timeout    = {-1000, 0};
	pool_t         *p_thepool = NULL;
	
	ASSERT(p_pool);
	ASSERT(p_pool->sigtype);
	ASSERT(p_pool->poolt >= RPLCC_TDI_POOL && p_pool->poolt <= RPLCC_REFRESH_POOL);

	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCGetNextBfromPool \n"));

	try {

		/* Get internal pool structure */
		p_thepool = (pool_t *)p_pool->p_internal;
		ASSERT(p_thepool);
		ASSERT(p_thepool->sigtype == RPLCC_POOL);

		/* Now MP safely Insert in Cache link */
		while(OS_InterlockedCompareExchange(&p_thepool->AtomicLock, 
			                                (LONG)OS_KeGetCurrentThread(), 
											ATOMIC_FREE)!=(LONG)OS_KeGetCurrentThread())
		{
			MMGDEBUG(MMGDBG_LOW, ("Waiting for lock... \n"));
			OS_KeDelayExecutionThread(KernelMode, FALSE, &timeout);
		}
		p_thepool->flags |= RPLCC_POOL_LOCKED;

		/*
		 * If the pool is full, now
		 * use the callback to signal the caller.
		 */
		if (p_thepool->entry_in_use == p_thepool->num_chunk)
		{
			MMGDEBUG(MMGDBG_INFO, ("Pool is full \n"));
			p_thepool->flags |= RPLCC_POOL_WAITER;
			p_thepool->callback = callback;
			p_thepool->p_ctxt   = p_ctxt;
			ret = STATUS_PENDING;
			leave;
		}

#ifdef NOP
		while (p_thepool->entry_in_use == p_thepool->num_chunk)
		{
			MMGDEBUG(MMGDBG_LOW, ("Waiting for new free entry ... \n"));
			
			OS_ExReleaseResourceLite(&p_thepool->lock);
			p_thepool->flags &= ~RPLCC_POOL_LOCKED;
			OS_KeWaitForSingleObject(&p_thepool->WaitForBlock,
									 Executive,
									 KernelMode,
									 TRUE,
									 &timeout);

			MMGDEBUG(MMGDBG_LOW, ("Wake up ... \n"));

			/* Now MP safe again ... */
			while(OS_ExAcquireResourceExclusiveLite(&p_thepool->lock, TRUE)==FALSE)
			{
				MMGDEBUG(MMGDBG_LOW, ("Waiting for  resource lock... \n"));
				OS_KeDelayExecutionThread(KernelMode, FALSE, &timeout);
			}
			p_thepool->flags |= RPLCC_POOL_LOCKED;
		}
#endif

		ASSERT(p_thepool->entry_in_use >= 0);
		ASSERT(p_thepool->entry_in_use < p_thepool->num_chunk);

		if (p_thepool->p_current == p_thepool->p_endaddr &&
			p_thepool->policy    == WRAP)
		{
			MMGDEBUG(MMGDBG_INFO, ("Resetting p_current to start of pool \n"));
			p_thepool->p_current = p_thepool->p_startaddr;
		}

		p_thepool->entry_in_use++;
		*p_callerbuf = (PVOID)((header_t *)p_thepool->p_current)->p_data; 

		/*
		 * The assumption is that the buffers in the pool
		 * will be consummed/free in sequential order.
		 * Here p_thepool->entry_in_use >= 0 and
		 * p_current must points to the next free hdr.
		 */
		ASSERT(((header_t *)p_thepool->p_current)->flags & RPLCC_HDR_FREE);
		((header_t *)p_thepool->p_current)->flags &= ~RPLCC_HDR_FREE;
		((header_t *)p_thepool->p_current)->flags |= RPLCC_HDR_INUSE;
		(char *)p_thepool->p_current += p_thepool->chunk_size;
		ASSERT(p_thepool->p_current < p_thepool->p_endaddr);

	} finally {

		if (p_thepool->flags & RPLCC_POOL_LOCKED)
		{
			OS_InterlockedExchange(&p_thepool->AtomicLock, ATOMIC_FREE);
			p_thepool->flags &= ~RPLCC_POOL_LOCKED;
		}
	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCGetNextBfromPool \n"));

return(ret);
} /* RplCCGetNextBfromPool */


/*B**************************************************************************
 * RplCCInvalidateCache - 
 * 
 * Invalidate all data from the cache for the given logical group.
 * No IOs can be process until the operation finishes.
 * The logical group will automatically accept new IO at the successful 
 * completion of the call.
 * If flag == REFRESH_POOL, include refresh pool IOs when invalidating.
 *
 *E==========================================================================
 */

OS_NTSTATUS
RplCCInvalidateCache(CC_logical_grp_t *LgPtr, int flags)

/**/
{
	OS_NTSTATUS ret = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCInvalidateCache \n"));

	try {

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCInvalidateCache \n"));

return(ret);
} /* RplCCInvalidateCache */

/*B**************************************************************************
 * RplCCInvalidatePendingIO - 
 * 
 * Invalidate The given Pending IO for this logical group.
 *E==========================================================================
 */

OS_NTSTATUS
RplCCInvalidatePendingIO(IN CC_logical_grp_t *LgPtr,
						 IN rplcc_iorp_t     *p_iorequest
						 )

/**/
{
	OS_NTSTATUS  ret = STATUS_SUCCESS;
	header_t     *p_hdr = NULL;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCInvalidatePendingIO \n"));

	try {

		p_hdr = p_iorequest->pHdlEntry;
		ret   = _RplCCRemovePendingE(LgPtr, p_hdr);

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCInvalidatePendingIO \n"));

return(ret);
} /* RplCCInvalidatePendingIO */

/*B**************************************************************************
 * RplCCHotGroups - 
 * 
 * Register a callback to notifies of ‘hot’ groups.  Groups trashing the 
 * given defined threshold. 
 * Insert a threashold and callback into logical group data structure,
 *
 *E==========================================================================
 */
 
OS_NTSTATUS
RplCCHotGroups(CC_logical_grp_t *LgPtr, 
			   int Threashold, 
               int (*HotGroupCallBack)(PVOID Context))

/**/
{
	OS_NTSTATUS ret = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCHotGroups \n"));

	try {

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCHotGroups \n"));

return(ret);
} /* RplCCHotGroups */


/*B**************************************************************************
 * RplCCDataReady - 
 * 
 *  Call DataReady function when some data committed in the cache. 
 *  Set to NULL to disable. 
 *  Insert a threashhold and callback into logical group data structure,
 *  NOT USED
 *
 *E==========================================================================
 */

OS_NTSTATUS
RplCCDataReady(IN CC_logical_grp_t *LgPtr, 
			   IN OS_NTSTATUS (*DataReadyCallback)(PVOID context))

/**/
{
	OS_NTSTATUS   ret = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCDataReady  \n"));

	try {

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCDataReady \n"));

return(ret);
} /* RplCCDataReady */

/*B**************************************************************************
 *
 * RplCCLGIterateCache - 
 *
 *	Interate through each entry in the cache for this particular group
 *  and call the given callback function.
 *
 *  The lists ordering is defined as:
 *  - Refresh 
 *  - Migrate 
 *  - Commit  
 *  - Pending 
 *
 *E==========================================================================
 */

OS_NTSTATUS
RplCCLGIterateEntries(CC_logical_grp_t   *LgPtr, 
				      CC_nodeaction_t    (__fastcall *callback)
				                            (PVOID        p_PersistCtxt, 
				                             rplcc_iorp_t *iorq),
				      PVOID              p_PersistCtxt
				      )

/**/
{ 
 	OS_NTSTATUS      ret     = STATUS_SUCCESS;
	LARGE_INTEGER    timeout = {-100, 0};
	OS_LIST_ENTRY    *ArrayQ[4];          /* anchor to the 3 Queues   */
	OS_LIST_ENTRY    *p_lnk    = NULL;    /* list node entry          */
	OS_LIST_ENTRY    *p_hdrlnk = NULL;    /* list node entry          */
	header_t         *p_hdr    = NULL;    /* The 'hard' header        */
	wlheader_t       *hp       = NULL;    /* The compatibility header */
	migrate_anchor_t *p_miga  = NULL;
	dtbnode_t        *p_dtb    = NULL;
	rplcc_iorp_t     Callback_param;
	CC_nodeaction_t  action;
	int              i;
	

	ASSERT(LgPtr);
	ASSERT(callback);
	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCLGIterateEntries \n"));

	try {

		/*
		 * If for some reason lg exist 
		 * but is not initialized ?
		 */
		if (!(LgPtr->flags & RPLCC_LG_INIT))
		{
			MMGDEBUG(MMGDBG_FATAL, ("LG is not initialized \n"));
			ret = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		/*  Lock the logical group */
		while(OS_InterlockedCompareExchange(&LgPtr->AtomicLock, 
			                                (LONG)OS_KeGetCurrentThread(), 
			                                ATOMIC_TAKEN)==(LONG)OS_KeGetCurrentThread())
		{
			MMGDEBUG(MMGDBG_LOW, ("Waiting for LG resource lock... \n"));
			OS_KeDelayExecutionThread(KernelMode, FALSE, &timeout);
		}

		/* we are now serialize for this LG */
		LgPtr->flags |= RPLCC_LG_LOCKED;

		ArrayQ[0] = &LgPtr->refreshQ;
		ArrayQ[1] = &LgPtr->migrateQ;
		ArrayQ[2] = &LgPtr->CommitQ;
		ArrayQ[3] = &LgPtr->PendingQ;

		for (i=0; i < 4; i++)
		{
			if (i == 1)
			{  
				/*
			 	 * Special case for the migrate Q
				 * composed of linked list of headers.
				 */
				
				/* Retrieve first entry */
				p_lnk  = ArrayQ[i]->Flink;
		
				/*
				 * Scan thru the migrate Q.
				 */
				while (p_lnk != &LgPtr->migrateQ)
				{
			
					/* Get the database and hard header ptrs. */
					p_miga = CONTAINING_RECORD(p_lnk, migrate_anchor_t, miga_lnk);
					ASSERT(p_miga->sigtype == RPLCC_MIGA);

					p_hdrlnk = p_miga->hdr_lnk.Flink;

					while (p_hdrlnk != &p_miga->hdr_lnk)
					{
						p_dtb = CONTAINING_RECORD(p_hdrlnk, dtbnode_t, lnk);
						ASSERT(p_dtb);
						ASSERT(p_dtb->sigtype == RPLCC_DTB);

						p_hdr = CONTAINING_RECORD(p_dtb, header_t, dtb_info);
		
						ASSERT(p_hdr);
						ASSERT(p_hdr->sigtype == RPLCC_HDR);
						ASSERT(!(p_hdr->flags & RPLCC_HDR_INDTB));
						ASSERT(p_hdr->flags & RPLCC_HDR_MIGR);
						
						Callback_param.DevicePtr = p_dtb->DeviceId;
						Callback_param.DeviceId  = 0;
						Callback_param.Blk_Start = p_dtb->Blk_Start;
						Callback_param.Size      = p_dtb->Size;
						Callback_param.pBuffer   = NULL;
						action = callback(p_PersistCtxt, &Callback_param);
						p_hdrlnk = p_hdrlnk->Flink;

					} /* for */

					p_lnk = p_lnk->Flink;
				} /* while */

			} else 
			{
				p_lnk = ArrayQ[i]->Flink;
				while (p_lnk != ArrayQ[i])
				{
					p_dtb = CONTAINING_RECORD(p_lnk, dtbnode_t, lnk);
					ASSERT(p_dtb);
					ASSERT(p_dtb->sigtype == RPLCC_DTB);

					p_hdr = CONTAINING_RECORD(p_dtb, header_t, dtb_info);

					ASSERT(p_hdr);
					ASSERT(p_hdr->sigtype == RPLCC_HDR);
					
					Callback_param.DevicePtr = p_dtb->DeviceId;
					Callback_param.DeviceId  = 0;
					Callback_param.Blk_Start = p_dtb->Blk_Start;
					Callback_param.Size      = p_dtb->Size;
					Callback_param.pBuffer   = NULL;
					action = callback(p_PersistCtxt, &Callback_param);

					/* 
					* action is not used for now.
					*/
					p_lnk = p_lnk->Flink;
				}
			}
		} /* for */

	} finally {

  	   /*
		* In all cases release the logical group now.
		*/
		LgPtr->flags &= ~RPLCC_LG_LOCKED;
		OS_InterlockedExchange(&LgPtr->AtomicLock, ATOMIC_FREE);
	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCLGIterateEntries \n"));

return(ret);
} /* RplCCLGIterateEntries */

/*B**************************************************************************
 *
 * RplCCLGIterateAndRemoveEntries - 
 *
 *	Interate through each entry in the cache for this particular group
 *  and call the given callback function.
 *
 *  The lists ordering is defined as:
 *  - Refresh 
 *  - Migrate 
 *  - Commit  
 *  - Pending 
 *
 *E==========================================================================
 */

OS_NTSTATUS
RplCCLGIterateAndRemoveEntries(CC_logical_grp_t   *LgPtr, 
				               CC_nodeaction_t    (__fastcall *callback)
				                                       (PVOID        p_PersistCtxt, 
				                                        rplcc_iorp_t *iorq),
				               PVOID              p_PersistCtxt
				               )

/**/
{ 
 	OS_NTSTATUS     ret     = STATUS_SUCCESS;
	LARGE_INTEGER   timeout = {-100, 0};
	OS_LIST_ENTRY   *ArrayQ[4];        /* anchor to the 3 Queues   */
	OS_LIST_ENTRY   *p_lnk    = NULL;  /* list node entry          */
	OS_LIST_ENTRY   *p_hdrlnk = NULL;  /* header linked list */
	header_t        *p_hdr    = NULL;  /* The 'hard' header        */
	wlheader_t      *hp       = NULL;  /* The compatibility header */
	dtbnode_t       *p_dtb    = NULL;  /* where the link exist     */
    migrate_anchor_t *p_miga  = NULL;
	rplcc_iorp_t    Callback_param;
	CC_nodeaction_t action;
	int             i;
	

	ASSERT(LgPtr);
	ASSERT(callback);
	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCLGIterateAndRemoveEntries \n"));

	try {

		/*
		 * If for some reason lg exist 
		 * but is not initialized ?
		 */
		if (!(LgPtr->flags & RPLCC_LG_INIT))
		{
			MMGDEBUG(MMGDBG_FATAL, ("LG is not initialized \n"));
			ret = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		/*  Lock the logical group */
		while(OS_InterlockedCompareExchange(&LgPtr->AtomicLock, 
			                                (LONG)OS_KeGetCurrentThread(), 
			                                 ATOMIC_TAKEN)==(LONG)OS_KeGetCurrentThread())
		{
			MMGDEBUG(MMGDBG_LOW, ("Waiting for LG resource lock... \n"));
			OS_KeDelayExecutionThread(KernelMode, FALSE, &timeout);
		}
		/* we are now serialize for this LG */
		LgPtr->flags |= RPLCC_LG_LOCKED;

		ArrayQ[0] = &LgPtr->refreshQ;
		ArrayQ[1] = &LgPtr->migrateQ;
		ArrayQ[2] = &LgPtr->CommitQ;
		ArrayQ[3] = &LgPtr->PendingQ;

		for (i=0; i < 4; i++)
		{
			if (i == 1)
			{  
				/*
			 	 * Special case for the migrate Q
				 * composed of  headers.
				 */
				
				/* Retrieve first entry */
				p_lnk  = ArrayQ[i]->Flink;
		
				/*
				 * Scan thru the migrate Q.
				 */
				while (p_lnk != &LgPtr->migrateQ)
				{
					/* Get the database and hard header ptrs. */
					p_miga = CONTAINING_RECORD(p_lnk, migrate_anchor_t, miga_lnk);
					ASSERT(p_miga->sigtype == RPLCC_MIGA);

					p_hdrlnk = p_miga->hdr_lnk.Flink;

					while (p_hdrlnk != &p_miga->hdr_lnk)
					{
						p_dtb = CONTAINING_RECORD(p_hdrlnk, dtbnode_t, lnk);
						ASSERT(p_dtb);
						ASSERT(p_dtb->sigtype == RPLCC_DTB);

						p_hdr = CONTAINING_RECORD(p_dtb, header_t, dtb_info);
						ASSERT(p_hdr);
						ASSERT(p_hdr->sigtype == RPLCC_HDR);
						ASSERT(!(p_hdr->flags & RPLCC_HDR_INDTB));
						ASSERT(p_hdr->flags & RPLCC_HDR_MIGR);
						
						Callback_param.DevicePtr = p_dtb->DeviceId;
						Callback_param.DeviceId  = 0;
						Callback_param.Blk_Start = p_dtb->Blk_Start;
						Callback_param.Size      = p_dtb->Size;
						Callback_param.pBuffer   = NULL;
						action = callback(p_PersistCtxt, &Callback_param);

						if (p_hdr->flags & RPLCC_HDR_REFQ)
						{
							RemoveEntryList(&p_hdr->dtb_info.lnk);		
							OS_ExFreeToNPagedLookasideList(&LgPtr->hdr_alist, p_hdr);
						} else 
						{
							ret = _RplCCRemovePendingE(LgPtr, p_hdr);
						}
						p_hdrlnk = p_hdrlnk->Flink;
					} /* while */

					p_lnk = p_lnk->Flink;
					RemoveEntryList(&p_miga->miga_lnk);
					OS_ExFreeToNPagedLookasideList(&LgPtr->miga_alist, p_miga);

				} /* while */

			} else 
			{
				p_lnk = ArrayQ[i]->Flink;
				while (p_lnk != ArrayQ[i])
				{
					p_dtb = CONTAINING_RECORD(p_lnk, dtbnode_t, lnk);
					ASSERT(p_dtb->sigtype == RPLCC_DTB);

					p_hdr = CONTAINING_RECORD(p_dtb, header_t, dtb_info);

					ASSERT(p_hdr);
					ASSERT(p_hdr->sigtype == RPLCC_HDR);
					
					Callback_param.DevicePtr = p_dtb->DeviceId;
					Callback_param.DeviceId  = 0;
					Callback_param.Blk_Start = p_dtb->Blk_Start;
					Callback_param.Size      = p_dtb->Size;
					Callback_param.pBuffer   = NULL;
					action = callback(p_PersistCtxt, &Callback_param);

					/*
					 * If we reach the pin memory segment ?
					 *
					 * Warning: If  bad memory bring us here
					 * touching it again may create a double fault?
					 */
					if (LgPtr->flags & RPLCC_LG_PINMEM)
					{
						RplCCUnpinMemSegment(p_hdr->p_data,
							                 p_hdr->dtb_info.Size);
				
						LgPtr->flags &= ~RPLCC_LG_PINMEM;
					}

					if (p_hdr->flags & RPLCC_HDR_REFQ)
					{
						RemoveEntryList(&p_hdr->dtb_info.lnk);
						OS_ExFreeToNPagedLookasideList(&LgPtr->hdr_alist, p_hdr);
					} else 
					{
						ret = _RplCCRemovePendingE(LgPtr, p_hdr);
						if (!OS_NT_SUCCESS(ret))
						{
							MMGDEBUG(MMGDBG_LOW, ("Removing pending Q failed %x \n", ret));
							leave;
						}
					}

					/* 
					 * action is not used for now.
					 * We now remove this entry from the list
					 */
					p_lnk = p_lnk->Flink;
				} /* while */
			} /* if */
		} /* for */

	ASSERT(IsListEmpty(&LgPtr->refreshQ));
	ASSERT(IsListEmpty(&LgPtr->migrateQ));
	ASSERT(IsListEmpty(&LgPtr->CommitQ));
	ASSERT(IsListEmpty(&LgPtr->PendingQ));

	} finally {

  	   /*
		* In all cases release the logical group now.
		*/
		LgPtr->flags &= ~RPLCC_LG_LOCKED;
		OS_InterlockedExchange(&LgPtr->AtomicLock, ATOMIC_FREE);
	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCLGIterateAndRemoveEntries \n"));

return(ret);
} /* RplCCLGIterateAndRemoveEntries */

/*B**************************************************************************
 * RplCCWrite - 
 * 
 *  Write data to Cache with Flag to overwrite existing data 
 *  (lg/disk/start/end tuple) 
 *
 *  New write data will be inserted at {Last Fifo in LG, fifo.nb_entries}
 *  New refresh data will be inserted in the refreshQ list now
 *  In band Sentinels in Refresh pool are not processed just Queued.
 *  In band Sentinels in Normal mode are created and  Queued.
 *
 *  We accept either independent IO comming from the filter thread or
 *  special pool IOs (refresh). (we don't allocate data)
 *
 *    In the old days refresh path and 'regular' writes used the
 *  same fifo pool from the Lg group to keep ordering. Now that
 *  we don't send 'regular' writes during refresh operation this
 *  fifo ordering is not requested. So only regular writes goes
 *  to the fifo Q. (it should be easy to reverse the changes)
 *
 *E==========================================================================
 */

MMG_PUBLIC
OS_NTSTATUS 
RplCCWrite(CC_logical_grp_t *LgPtr,  
		   rplcc_iorp_t     *p_iorequest, 
		   Qtype_e           type_Q )

/**/
{
	OS_NTSTATUS   ret          = STATUS_SUCCESS;	
	LARGE_INTEGER timeout      = {-100, 0};
	header_t      *p_hdr       = NULL;	   /* The 'hard' header        */
	wlheader_t    *hp          = NULL;     /* The compatibility header */
	
	ASSERT(LgPtr);
	ASSERT(LgPtr->sigtype == RPLCC_LG);
	ASSERT(p_iorequest);
	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCWrite \n"));

	try {

		/*
		 * If for some reason lg exist 
		 * but is not initialized ?
		 */
		if (!(LgPtr->flags & RPLCC_LG_INIT))
		{
			MMGDEBUG(MMGDBG_FATAL, ("LG is not initialized \n"));
			ret = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		/*  Lock the logical group */
		while(OS_InterlockedCompareExchange(&LgPtr->AtomicLock, 
			                                (LONG)OS_KeGetCurrentThread(),
			                                ATOMIC_TAKEN)==(LONG)OS_KeGetCurrentThread())
		{
			MMGDEBUG(MMGDBG_LOW, ("Waiting for LG resource lock... \n"));
			OS_KeDelayExecutionThread(KernelMode, FALSE, &timeout);
		}

		/* we are now serialize for this LG */
		LgPtr->flags |= RPLCC_LG_LOCKED;

		if (type_Q == RPLCC_REFRESHWRITE)
		{
			/*
		 	 * This Write comes from a special pool
			 * header/data has already been allocated from 
			 * the pool.
			 */
			MMGDEBUG(MMGDBG_INFO, ("Detected special pool \n"));
			p_hdr  = p_iorequest->pBuffer;
			(char *)p_hdr -= sizeof(header_t);
			ASSERT(p_hdr->sigtype == RPLCC_HDR);
			p_hdr->flags &= ~RPLCC_HDR_FREE;
			p_hdr->flags |= (RPLCC_HDR_INIT|RPLCC_HDR_REFQ);

		} else  
		{
		   /*
			* NORMAL WRITE PATH -
			* for Sentinel or Regular write
			*
			* NOTE: p_hdr is invalid here.
			*
			*/

			MMGDEBUG(MMGDBG_INFO, ("'Regular' write data \n"));
			MMGDEBUG(MMGDBG_INFO, ("Allocating a new hard header \n"));
			p_hdr = (header_t *)OS_ExAllocateFromNPagedLookasideList(&LgPtr->hdr_alist);
			if (p_hdr == NULL)
			{
				MMGDEBUG(MMGDBG_FATAL, ("Cannot Allocate a header node \n"));
				ret = STATUS_INSUFFICIENT_RESOURCES;
				leave;
			}

			RtlFillMemory(p_hdr, sizeof(header_t), 0);
		
			p_hdr->flags   = RPLCC_HDR_INIT;
			p_hdr->sigtype = RPLCC_HDR;

			InitializeListHead(&p_hdr->dtb_info.lnk);

			// This is the end condition
			// for the Btree.
			//p_hdr->dtb_info.lnk.Blink =
			//p_hdr->dtb_info.lnk.Flink = NULL;   

			/*
			 * Sentinel data size 1 sector
			 */
			if (p_iorequest->SentinelType != MSGNONE)
			{
				p_iorequest->Size = 512;
			}

			/*
		 	 * We allocate from the slab at this time
			 * we may move to a special pool later ?
			 */
			p_hdr->p_oldheader = 
				  (wlheader_t *)RplSlabAllocate(&RplCCCache.slab_instance,
										  		sizeof(wlheader_t), 
												&ret);

			if (!OS_NT_SUCCESS(ret))
			{
				MMGDEBUG(MMGDBG_FATAL, ("Cannot Allocate 'soft' Header \n"));
				leave;
			}

			/*
			 * Allocate a new memory slot &
			 * Copy data to the new Memory
			 */
			p_hdr->p_data = 
				(PVOID )RplSlabAllocate(&RplCCCache.slab_instance,
						  				p_iorequest->Size,
										&ret);

			if (!OS_NT_SUCCESS(ret))
			{
				MMGDEBUG(MMGDBG_FATAL, ("Cannot Allocate new data segment %x \n", ret));
				leave;
			}
			
			/*
			 * Insert the new entry in the fifo Q.
			 */
			ret = SftkPrepareSoftHeader((PVOID)p_hdr->p_oldheader, p_iorequest);
			if (!OS_NT_SUCCESS(ret))
			{
				MMGDEBUG(MMGDBG_FATAL, ("Cannot create 'soft' Header \n"));
				leave;
			}

		}  /* End of regular write */

		/*
	 	 * populate the new header for the 3 cases
		 * Sentinel, Pool, regular write.
		 */
		p_hdr->dtb_info.Blk_Start = p_iorequest->Blk_Start;
		p_hdr->dtb_info.DeviceId  = p_iorequest->DevicePtr;
		p_hdr->dtb_info.sigtype   = RPLCC_DTB;
		p_hdr->dtb_info.Size      = p_iorequest->Size;

		/*
		 * If it is a new sentinel,
		 * Create it, insert it in commit Q
		 * and done.
		 */
		if (p_iorequest->SentinelType != MSGNONE)
		{
			MMGDEBUG(MMGDBG_INFO, ("Creating sentinel \n"));	

			/*
			 * If we have a user buffer or
			 * if we come from a refresh pool:error
			 */
			if (p_iorequest->pBuffer != NULL &&
				(p_hdr->flags & RPLCC_HDR_REFQ))
			{
				MMGDEBUG(MMGDBG_FATAL, ("user buffer not NULL for sentinel or refresh pool\n"));
				ret = STATUS_INVALID_PARAMETER;
				leave;
			}
			
			p_iorequest->pBuffer = p_hdr->p_data;

			/*
			 * Create the Sentinel Paquet now.
			 */
			ret = SftkPrepareSentinal(  p_iorequest->SentinelType, 
										p_hdr->p_oldheader, 
										p_iorequest,
										NULL);

			/*  
			 * Queue it to the commit Q now
			 */
			InsertTailList(&LgPtr->CommitQ, &p_hdr->dtb_info.lnk);
			leave;
			/* NOTREACHED */
		}

		/*
		 * Pool or regular write
		 */

		/* Pin memory
		*/

		ret = RplCCPinMemSegment(p_hdr->p_data,
			                     p_iorequest->Size, 0);
		if (!OS_NT_SUCCESS(ret))
		{	
			MMGDEBUG(MMGDBG_FATAL, ("Cannot Lock new data segment %x \n", ret));
			leave;
		}

		ret = RplCCPinMemSegment(p_iorequest->pBuffer,
			                     p_iorequest->Size, 0);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Cannot Lock IO data segment %x \n", ret));
			leave;
		}

		/*
		 * If the copy trigger a fault and we reach finally w/o unpinning
		 * we need to try to unpin the memory in the rollback.
		 */
		LgPtr->flags |= RPLCC_LG_PINMEM;

		if (!(p_hdr->flags & RPLCC_HDR_REFQ))
		{
			/* Copy to cache */
			RtlCopyMemory(p_hdr->p_data, 
						  p_iorequest->pBuffer, 
						  p_iorequest->Size );
		}

		/* Unpin memory
		*/
		
		ret = RplCCUnpinMemSegment(p_hdr->p_data,
			                       p_iorequest->Size);
		if (!OS_NT_SUCCESS(ret))
		{	
			MMGDEBUG(MMGDBG_FATAL, ("Cannot UnLock new data segment %x \n", ret));
			leave;
		}

		ret = RplCCUnpinMemSegment(p_iorequest->pBuffer,
			                       p_iorequest->Size);
		if (!OS_NT_SUCCESS(ret))
		{	
			MMGDEBUG(MMGDBG_FATAL, ("Cannot UnLock IO data segment %x \n", ret));
			leave;
		}

		LgPtr->flags &= ~RPLCC_LG_PINMEM;

		if (p_hdr->flags & RPLCC_HDR_REFQ)
		{
			/*
			 * We insert the request in the RefreshQ
			 */
			InsertTailList(&LgPtr->refreshQ, &p_hdr->dtb_info.lnk);

			/*
			 * We should put a flag 
			 */
			MMGDEBUG(MMGDBG_INFO,("Wake up waiters for data\n"));
			OS_KeSetEvent(&LgPtr->waitForData,
				          IO_NO_INCREMENT,
						  FALSE);

		} else
		{
			/* This is pending IO */
			LgPtr->flags |= RPLCC_LG_PENDING;

			/*
			 * Insert the new IO request into the Pending Q
			 */
			InsertTailList(&LgPtr->PendingQ, &p_hdr->dtb_info.lnk);
			LgPtr->total_pending  += p_hdr->dtb_info.Size ;
			p_hdr->flags |= RPLCC_HDR_INDTB;
			
			/*
			 * The new data is now pending and is 
			 * not available for consumption yet
			 * by other size of the net.
			 */
		}

		p_iorequest->pHdlEntry = p_hdr;

		/*
		 * Did caller ask threshold monitoring ?
		 */

	} finally {

		/*
		 * Error ? try to rollback...
		 */
		if (!OS_NT_SUCCESS(ret))
		{
			/*
			 * Remove from database if needed
			 */
			if (p_hdr != NULL && LgPtr != NULL)
			{

				/*
				 * If we reach the pin memory segment ?
				 *
				 * Warning: If  bad memory bring us here
				 * touching it again may create a double fault?
				 */
				if (p_iorequest->pBuffer != NULL   &&
					(LgPtr->flags & RPLCC_LG_PINMEM))
				{
					RplCCUnpinMemSegment(p_hdr->p_data,
										 p_iorequest->Size);
			
					RplCCUnpinMemSegment(p_iorequest->pBuffer,
										 p_iorequest->Size);
			
					LgPtr->flags &= ~RPLCC_LG_PINMEM;
				}
				
				if (p_hdr->flags & RPLCC_HDR_REFQ)
				{
					RemoveEntryList(&p_hdr->dtb_info.lnk);
					OS_ExFreeToNPagedLookasideList(&LgPtr->hdr_alist, p_hdr);
				} else
				{
					_RplCCRemovePendingE(LgPtr, p_hdr);
				}
			}	/* if */
		}
	}	/* finally */

	/*
	 * In all cases release the logical group now.
	 */
	LgPtr->flags &= ~RPLCC_LG_LOCKED;
	OS_InterlockedExchange(&LgPtr->AtomicLock, ATOMIC_FREE);

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCWrite \n"));

return(ret);
} /* RplCCWrite */

#ifdef NOT_USED
/*B**************************************************************************
 * RplCCIsCached - 
 * 
 *  Return TRUE if Data(lg/disk/start/size tuple) is already in cache. 
 *  Front End to search engine database.
 *
 *  The logical group needs to be previously locked
 *  NOT USED.
 *E==========================================================================
 */

MMG_PUBLIC 
BOOLEAN
RplCCIsDataCached(IN CC_logical_grp_t *LgPtr, IN rplcc_iorp_t *p_iorequest) 

/**/
{
	BOOLEAN   ret = FALSE;
	dtbnode_t Aquery;     

	ASSERT(LgPtr);
	ASSERT(LgPtr->sigtype == RPLCC_LG);
	ASSERT(LgPtr->flags & RPLCC_LG_INIT);
	ASSERT(LgPtr->flags & RPLCC_LG_LOCKED);
	ASSERT(p_iorequest);

	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCIsDataCached \n"));

	try {

		/* 
		 * Normalize the request to dtbnode
		 * The database understand only dtbnode
		 * for now.
		 */
		Aquery.Blk_Start = p_iorequest->Blk_Start;
		Aquery.DeviceId  = p_iorequest->DevicePtr;
		Aquery.Size      = p_iorequest->Size;
		Aquery.sigtype   = RPLCC_DTB; // emulate Database node ?

		/*
		 * Now compute the range
		 */
		Aquery.IntervalLow   = Aquery.Blk_Start ;
		Aquery.IntervalHigh  = Aquery.IntervalLow  + 
			                             Aquery.Size ;
		ret = RplDtbSearch(&LgPtr->database, &Aquery);

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCIsDataCached \n"));

return(ret);
} /* RplCCIsDataCached */
#endif

/*B**************************************************************************
 * RplCCMemCommitted - 
 * 
 * Data Has been committed to disk locally. 
 * Move Data in a logical group from pending memory state to committed 
 * memory linked list.
 *
 * This functions allows for out of order completion.
 * 
 * May run at completion routine time > PASSIVE_LEVEL
 *
 *E==========================================================================
 */

MMG_PUBLIC
OS_NTSTATUS
RplCCMemCommitted(CC_logical_grp_t *LgPtr, IN rplcc_iorp_t *p_iorequest) 

/**/
{
	OS_NTSTATUS    ret = STATUS_SUCCESS;
	LARGE_INTEGER  timeout     = {-100, 0};
	header_t       *p_hdr      = NULL; /* The 'hard' header                   */

	ASSERT(LgPtr);
	ASSERT(LgPtr->sigtype == RPLCC_LG);
	ASSERT(p_iorequest);
	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCMemCommitted \n"));

	try {

		/*
		 * If for some reason lg exist 
		 * but is not initialized ?
		 */
		if (!(LgPtr->flags & RPLCC_LG_INIT))
		{
			MMGDEBUG(MMGDBG_FATAL, ("LG is not initialized \n"));
			ret = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		/*  Lock the logical group */
		while(OS_InterlockedCompareExchange(&LgPtr->AtomicLock, 
			                                (LONG)OS_KeGetCurrentThread(), 
			                                ATOMIC_TAKEN)==(LONG)OS_KeGetCurrentThread())
		{
			MMGDEBUG(MMGDBG_LOW, ("Waiting for LG resource lock... \n"));
			OS_KeDelayExecutionThread(KernelMode, FALSE, &timeout);
		}

		/* we are now serialize for this LG */
		LgPtr->flags |= RPLCC_LG_LOCKED;

		/*
		 * If they are no pending IOs bail out now
		 */
		if (!(LgPtr->flags & RPLCC_LG_PENDING))
		{
			MMGDEBUG(MMGDBG_FATAL, ("LG Has no pending IOs \n"));
			ret = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}
	
		p_hdr = p_iorequest->pHdlEntry;
		ASSERT(p_hdr->sigtype == RPLCC_HDR);

		/*
		 * Transition this entry to committed zone
		 */
		LgPtr->total_commit  += p_hdr->dtb_info.Size;
		
		/*
		 * Remove this entry from the pending IO database
		 */
		RemoveEntryList(&p_hdr->dtb_info.lnk);
		
		p_hdr->flags &= ~RPLCC_HDR_INDTB;
		if (LgPtr->flags & RPLCC_LG_WBLOCK)
		{
			OS_KeSetEvent(&LgPtr->waitForBlock,
						  IO_NO_INCREMENT,
						  FALSE);
		}

		/*
		 * Do we still have pending IOs for this LG
		 */
		LgPtr->total_pending  -= p_hdr->dtb_info.Size ;
		if (LgPtr->total_pending  == 0)
		{
			LgPtr->flags &= ~RPLCC_LG_PENDING;
		} 

		/*
		 * Insert the entry into the commit Q
		 * and signal waiters if first commit entry
		 */
		p_hdr->flags |= RPLCC_HDR_COMMIT;
		InsertTailList(&LgPtr->CommitQ, &p_hdr->dtb_info.lnk);
		if (!(LgPtr->flags & RPLCC_LG_COMIT))
		{
			LgPtr->flags |= RPLCC_LG_COMIT;
		}

	} finally {

		/*
		 * If there is committed memory now
		 * wakeup all waiters.
		 */
		if (LgPtr->flags & RPLCC_LG_COMIT)
		{
			MMGDEBUG(MMGDBG_INFO,("Wake up waiters for commit\n"));
			OS_KeSetEvent(&LgPtr->waitForData,
				          IO_NO_INCREMENT,
						  FALSE);
		}

		/*
		 * In all cases release the logical group now.
		 */
		if(LgPtr->flags & RPLCC_LG_LOCKED)
		{
			LgPtr->flags &= ~RPLCC_LG_LOCKED;
			OS_InterlockedExchange(&LgPtr->AtomicLock, ATOMIC_FREE);
		}
	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCMemCommitted \n"));

return(ret);
} /* RplCCMemCommitted */


/*B**************************************************************************
 * RplCCGetNextSegment - 
 *
 * Return next committed data up to size. 
 * We traverse the LG commit Q list or Refresh Q
 * up to given size, end of list, or 1 entry if > given size.
 *
 * WARNING: We assume the buffer size if the Maximum IO size.
 *
 * WARNING: This a logical group scale operation. 
 *          ie: we don't differentiate by Device ID.
 *
 * If flags set return 0 if no data in buffer instead of waiting...
 *
 *E==========================================================================
 */

MMG_PUBLIC
OS_NTSTATUS
RplCCGetNextSegment(IN CC_logical_grp_t *LgPtr,  
					IN ULONG            Flags, 
					IN rplcc_iorp_t     *p_iorequest) 

/**/
{
	OS_NTSTATUS      ret          = STATUS_SUCCESS;
	LARGE_INTEGER    timeout      = {-100, 0};	
	ULONG            size_oldhrd  = {sizeof(wlheader_t)};
	LONG             size_left;           /* Must be signed               */
	ULONG            total_size;
	protocol_e       packType;            /* type of protocol packet      */
	CC_lgstate_t     State;               /* device LG state              */
	OS_LIST_ENTRY    *p_thelist;          /* Ptr to the current list      */
	OS_LIST_ENTRY    *p_hdrlnk;           /* the commit list ptr.         */
	dtbnode_t        *p_dtb       = NULL; /* where the link exist         */
	char             *p_callerbuf = NULL; /* scratch ptr to caller buffer */
	header_t         *p_hdr       = NULL; /* The 'hard' header            */
	migrate_anchor_t *p_miga      = NULL; /* migrate anchor structure     */ 
	

	ASSERT(LgPtr);
	ASSERT(LgPtr->sigtype == RPLCC_LG);
	ASSERT(p_iorequest);

	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCGetNextSegment \n"));

	try {
		
		if (p_iorequest->Size == 0)
		{
			MMGDEBUG(MMGDBG_INFO, ("size == 0 \n"));
			leave;
		}

		p_iorequest->pHdlEntry = NULL;

		/*
		 * If for some reason lg exist 
		 * but is not initialized ?
		 */
		if (!(LgPtr->flags & RPLCC_LG_INIT))
		{
			MMGDEBUG(MMGDBG_FATAL, ("LG is not initialized \n"));
			ret = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		/*  Lock the logical group */
		while(OS_InterlockedCompareExchange(&LgPtr->AtomicLock, 
			                                (LONG)OS_KeGetCurrentThread(),
			                                ATOMIC_TAKEN)==(LONG)OS_KeGetCurrentThread())
		{
			MMGDEBUG(MMGDBG_LOW, ("Waiting for LG resource lock... \n"));
			OS_KeDelayExecutionThread(KernelMode, FALSE, &timeout);
		}

		/* we are now serialize for this LG */
		LgPtr->flags |= RPLCC_LG_LOCKED;

		/*
		 * Get the state of the LG from 
		 * external component to decide
		 * What to do
		 */
		ASSERT(LgPtr->GetState);
		State = LgPtr->GetState(LgPtr->p_backtr);

		switch(State)
		{
		case REFRESH:
			MMGDEBUG(MMGDBG_INFO, ("using refresh pool ...\n"));
			
			/* Retrieve first entry */
			p_thelist = &LgPtr->refreshQ;
			if (IsListEmpty(&LgPtr->refreshQ))
			{
				if (Flags & RPLCC_WAITDATA)
				{
					MMGDEBUG(MMGDBG_INFO, ("Waiting committed data \n"));
					OS_KeWaitForSingleObject(&LgPtr->waitForData, 
											Executive,
											KernelMode,
											TRUE,
											NULL);
				} else 
				{
					MMGDEBUG(MMGDBG_INFO, ("empty refresh Q ...\n"));
				    ret = STATUS_PIPE_EMPTY;
					leave;
				}
			}

			packType = FTDCRFBLK;

			break;

		case NORMAL:

			MMGDEBUG(MMGDBG_INFO, ("using normal pool ...\n"));

		   /*
			* Default mode we come here
			*/

			if (!(LgPtr->flags & RPLCC_LG_COMIT))
			{
				if (Flags & RPLCC_WAITDATA)
				{
					MMGDEBUG(MMGDBG_INFO, ("Waiting committed data \n"));
					OS_KeWaitForSingleObject(&LgPtr->waitForData, 
											Executive,
											KernelMode,
											TRUE,
											NULL);
				} else 
				{
					MMGDEBUG(MMGDBG_INFO, ("No committed data \n"));
					ret = STATUS_PIPE_EMPTY;
					leave;
				}
			}

			/* We MUST have committed data */
			ASSERT(!IsListEmpty(&LgPtr->CommitQ));

			/* Retrieve first entry */
			p_thelist = &LgPtr->CommitQ;

			packType = FTDCCHUNK;
			
			break;

		case TRACKING:
			MMGDEBUG(MMGDBG_INFO, ("tracking mode, leaving...\n"));
			leave;
			/* NOTREACHED*/
		}

		p_hdrlnk  = p_thelist->Flink;

		/* Lock caller memory segment for session */
		ret = RplCCPinMemSegment(p_iorequest->pBuffer,
								 p_iorequest->Size, 0);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Cannot Lock caller's segment %x \n", ret));
			leave;
		}

		/*
		 * LG has migrate data now.
		 */
		LgPtr->flags |=  RPLCC_LG_MIGRATE;

		p_miga = (migrate_anchor_t *)OS_ExAllocateFromNPagedLookasideList(&LgPtr->miga_alist);
		if (p_miga == NULL)
		{
			MMGDEBUG(MMGDBG_FATAL, ("Cannot Allocate a migrate anchor node \n"));
			ret = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}
		RtlFillMemory(p_miga, sizeof(migrate_anchor_t), 0);

		p_miga->sigtype = RPLCC_MIGA;
		InitializeListHead(&p_miga->miga_lnk);
		InitializeListHead(&p_miga->hdr_lnk);
		InsertTailList(&LgPtr->migrateQ, &p_miga->miga_lnk);

		/*
		 * Prepare the while loop.
		 */
		size_left   = p_iorequest->Size;
		total_size  = 0;
		p_callerbuf = (char *)p_iorequest->pBuffer;

		/*
		 * Now skip the protocol header
		 * so we can first copy entries
		 * in the user buffer.
		 */
		p_callerbuf += sizeof(ftd_header_t);

		/*
		 * Now the real work
		 * while we still have space 
		 * left and enough data.
		 */
		while (size_left > 0 &&
			   p_hdrlnk != p_thelist)
		{
			
			/* Get the database and hard header ptrs. */
			p_dtb = CONTAINING_RECORD(p_hdrlnk, dtbnode_t, lnk);
			ASSERT(p_dtb);
			ASSERT(p_dtb->sigtype == RPLCC_DTB);

			p_hdr = CONTAINING_RECORD(p_dtb, header_t, dtb_info);

			ASSERT(p_hdr);
			ASSERT(p_hdr->sigtype == RPLCC_HDR);
			ASSERT(!(p_hdr->flags & RPLCC_HDR_INDTB));

			size_left -= p_hdr->dtb_info.Size;

			if (size_left < 0)
			{
				/*
				 * we blow the CHUNKSIZE limit
				 */
				MMGDEBUG(MMGDBG_INFO, ("entry > CHUNKSIZE \n"));
				if (total_size != 0)
				{
					/*
					 * This entry is oversized but
					 * not the first in the buffer
					 * so we cannot copy it safely.
					 * Do it next call.
					 */
					MMGDEBUG(MMGDBG_INFO, ("oversized non first entry leaving.\n"));
					continue;
					/* NOTREACHED */
				} 

				/*
				 * This is the first oversized
				 * entry copy it stand alone
				 * and return.
				 */
			}

			/*
			 * Refresh Q Don't have soft headers
			 */
			if (!(p_hdr->flags & RPLCC_HDR_POOL))
			{
				/*
				 * Copy this entry to caller space
				 */	
				ret = RplCCPinMemSegment(p_hdr->p_oldheader,
										size_oldhrd, 0);
	
				if (!OS_NT_SUCCESS(ret))
				{	
					MMGDEBUG(MMGDBG_FATAL, ("Cannot Lock old header segment %x \n", ret));
					leave;
				}
				
				LgPtr->flags |= RPLCC_LG_PINMEM;

				/* Copy to caller buffer */
				RtlCopyMemory(p_callerbuf,
							  p_hdr->p_oldheader, 
							  size_oldhrd );
				
				p_callerbuf += sizeof(wlheader_t);

				/* Unpin memory
				*/
				ret = RplCCUnpinMemSegment(p_hdr->p_oldheader,
										size_oldhrd);
				if (!OS_NT_SUCCESS(ret))
				{	
					MMGDEBUG(MMGDBG_FATAL, ("Cannot Lock old header segment %x \n", ret));
					leave;
				}

				LgPtr->flags &= ~RPLCC_LG_PINMEM;
				size_left  -= size_oldhrd;
			}

			ret = RplCCPinMemSegment(p_hdr->p_data,
								     p_hdr->dtb_info.Size, 0);
			if (!OS_NT_SUCCESS(ret))
			{	
				MMGDEBUG(MMGDBG_FATAL, ("Cannot Lock new data segment %x \n", ret));
				leave;
			}

			LgPtr->flags |= RPLCC_LG_PINMEM;

			/* Copy to caller buffer */
			RtlCopyMemory(p_callerbuf,
				          p_hdr->p_data, 
						  p_hdr->dtb_info.Size );

			ret = RplCCUnpinMemSegment(p_hdr->p_data,
									   p_hdr->dtb_info.Size);
			if (!OS_NT_SUCCESS(ret))
			{	
				MMGDEBUG(MMGDBG_FATAL, ("Cannot UnLock data segment %x \n", ret));
				leave;
			}
			
			LgPtr->flags &= ~RPLCC_LG_PINMEM;			
			p_callerbuf += p_hdr->dtb_info.Size;
			total_size  += p_hdr->dtb_info.Size;
			p_hdrlnk = p_hdrlnk->Flink;

			/*
			 * We remove this entry from the Q.
			 */
			RemoveEntryList(&p_dtb->lnk);
			InsertTailList(&p_miga->hdr_lnk, &p_hdr->dtb_info.lnk);
			p_hdr->flags |=  RPLCC_HDR_MIGR;

			if (!(p_hdr->flags & RPLCC_HDR_POOL))
			{
				ASSERT(packType == FTDCCHUNK);
				if (p_hdrlnk == p_thelist)
				{
				   /* 
					* They may not be a next iteration
					* if sizeleft is 0, we are the last
					* commit entry
					*/
					LgPtr->flags &=  ~RPLCC_LG_COMIT;
				}
				
				p_hdr->flags &= ~RPLCC_HDR_COMMIT;
			
				/*
			 	 * Decommission this header and
				 * related data structures
				 */
				MMGDEBUG(MMGDBG_INFO, ("Freeing entry \n"));
				RplSlabFree(&RplCCCache.slab_instance, p_hdr->p_data);
				RplSlabFree(&RplCCCache.slab_instance, p_hdr->p_oldheader);
			} 
		} /* while */

		/*
		 * Now we have all the entries
		 * Setup the protocol header.
		 */

		p_iorequest->Size      = total_size;
		p_iorequest->pHdlEntry = p_miga;

		ret = SftkPrepareProtocolHeader(packType,
			                            p_iorequest->pBuffer,
			                            p_iorequest,
										NULL,
										1     );

		if (!OS_NT_SUCCESS(ret))
		{	
			MMGDEBUG(MMGDBG_FATAL, ("Cannot build protocol header %x \n", ret));
			leave;
		}

		ret = RplCCUnpinMemSegment(p_iorequest->pBuffer,
		  						   p_iorequest->Size);
		if (!OS_NT_SUCCESS(ret))
		{	
			MMGDEBUG(MMGDBG_FATAL, ("Cannot UnLock IO caller segment %x \n", ret));
			leave;
		}

	} finally {

		/*
		 * TBD: We need to remove the p_fifo from migrate Q
		 * in case of error.
		 */
		if (!OS_NT_SUCCESS(ret))
		{
			if (p_miga != NULL)
			{
				OS_ExFreeToNPagedLookasideList(&LgPtr->miga_alist, p_miga);
			}
		}

		/*
		 * In all cases release the logical group now.
		 */
		if(LgPtr->flags & RPLCC_LG_LOCKED)
		{
			LgPtr->flags &= ~RPLCC_LG_LOCKED;
			OS_InterlockedExchange(&LgPtr->AtomicLock, ATOMIC_FREE);
		}
	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCGetNextSegment \n"));

return(ret);
} /* RplCCGetNextSegment */


/*B**************************************************************************
 * RplCCMemFree - 
 *
 * Free hard headers, (from migrate state to free)
 *
 * p_iorequest->pHdlEntry give us all the entries to free.
 * from the migrate Q.
 *
 * They are no retranmission from the cache so no need to keep the data 
 * around.
 *
 *E==========================================================================
 */

MMG_PUBLIC
OS_NTSTATUS
RplCCMemFree(CC_logical_grp_t *LgPtr,
			 IN rplcc_iorp_t  *p_iorequest) 

/**/
{
	OS_NTSTATUS      ret        = STATUS_SUCCESS;  
	LARGE_INTEGER    timeout    = {-100, 0};	
	OS_LIST_ENTRY    *p_hdrlnk;            /* the migrate list ptr.        */
	dtbnode_t        *p_dtb     = NULL;    /* where the link exist         */
	header_t         *p_hdr     = NULL;	   /* The 'hard' header            */
	migrate_anchor_t *p_miga    = NULL;
	

	ASSERT(LgPtr);
	ASSERT(LgPtr->sigtype == RPLCC_LG);
	ASSERT(p_iorequest);


	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCMemFree \n"));

	try {
		
		if (p_iorequest->Size == 0)
		{
			MMGDEBUG(MMGDBG_INFO, ("size == 0 \n"));
			leave;
		}

		/*
		 * If for some reason lg exist 
		 * but is not initialized ?
		 */
		if (!(LgPtr->flags & RPLCC_LG_INIT))
		{
			MMGDEBUG(MMGDBG_FATAL, ("LG is not initialized \n"));
			ret = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		/*  Lock the logical group */
		while(OS_InterlockedCompareExchange(&LgPtr->AtomicLock, 
			                                (LONG)OS_KeGetCurrentThread(), 
			                                ATOMIC_TAKEN)==(LONG)OS_KeGetCurrentThread())
		{
			MMGDEBUG(MMGDBG_LOW, ("Waiting for LG resource lock... \n"));
			OS_KeDelayExecutionThread(KernelMode, FALSE, &timeout);
		}

		/* we are now serialize for this LG */
		LgPtr->flags |= RPLCC_LG_LOCKED;

		if (!(LgPtr->flags & RPLCC_LG_MIGRATE))
		{
			MMGDEBUG(MMGDBG_FATAL, ("No migrate data \n"));
			ret = STATUS_INVALID_PARAMETER;
			leave;
		}
	
		ASSERT(!IsListEmpty(&LgPtr->migrateQ));

		p_miga = p_iorequest->pHdlEntry;
		if (p_miga->sigtype != RPLCC_MIGA)
		{
			MMGDEBUG(MMGDBG_INFO, ("Invalid fifo entry \n"));
			ret = STATUS_INVALID_PARAMETER;
			leave;
		}

		MMGDEBUG(MMGDBG_INFO, ("found entry \n"));
		p_hdrlnk = p_miga->hdr_lnk.Flink;

		while (p_hdrlnk != &p_miga->hdr_lnk)
		{
			p_dtb = CONTAINING_RECORD(p_hdrlnk, dtbnode_t, lnk);
			ASSERT(p_dtb);
			ASSERT(p_dtb->sigtype == RPLCC_DTB);

			p_hdr = CONTAINING_RECORD(p_dtb, header_t, dtb_info);
			ASSERT(p_hdr);
			ASSERT(p_hdr->sigtype == RPLCC_HDR);
			ASSERT(!(p_hdr->flags & RPLCC_HDR_INDTB));
			ASSERT(p_hdr->flags & RPLCC_HDR_MIGR);

			p_hdr->flags &= ~RPLCC_HDR_MIGR;
			p_hdr->flags |= RPLCC_HDR_FREE;

			p_hdrlnk = p_hdrlnk->Flink;

			if (!(p_hdr->flags & RPLCC_HDR_POOL))
			{
				MMGDEBUG(MMGDBG_INFO, ("Freeing entry \n"));
				p_hdr->flags &= ~RPLCC_HDR_INIT;
				RemoveEntryList(&p_hdr->dtb_info.lnk);
				OS_ExFreeToNPagedLookasideList(&LgPtr->hdr_alist, p_hdr);
			} else 
			{
				/*
				 * Free entry in pool
				 */
				ASSERT(p_hdr->p_pool);
				ASSERT(p_hdr->p_pool->sigtype == RPLCC_POOL);
				p_hdr->p_pool->entry_in_use--;

				/*
				 * If waiter on new memory chunk: wake him up.
				 */
				if (p_hdr->p_pool->flags & RPLCC_POOL_WAITER)
				{
					if (p_hdr->p_pool->callback)
					{
						MMGDEBUG(MMGDBG_INFO, ("Wake up waiter for block in pool \n"));
						ret = p_hdr->p_pool->callback(p_hdr->p_data, 
													  p_hdr->p_pool->p_ctxt);
					}
				}

				/*
				 * If caller wants to be notified when 
				 * Refresh Q is empty
				 */
				if (p_hdr->p_pool->entry_in_use == 0 &&
					LgPtr->RefreshQFreeCB != NULL)
				{
					MMGDEBUG(MMGDBG_INFO, ("Wake up waiter for block in pool \n"));
					(LgPtr->RefreshQFreeCB)(LgPtr->RefreshQFreeCtxt);
				}
			}
		}

		/* remove from migrate Q */
		RemoveEntryList(&p_miga->miga_lnk);
			
		/* return anchor to the pool */
		OS_ExFreeToNPagedLookasideList(&LgPtr->miga_alist, p_miga);

		if (IsListEmpty(&LgPtr->migrateQ))
		{
			LgPtr->flags &= ~RPLCC_LG_MIGRATE;

			/* SetEvent */
			if (LgPtr->MigrateQFreeCB)
			{
				MMGDEBUG(MMGDBG_INFO, ("Calling migration free CB \n"));
				LgPtr->MigrateQFreeCB(LgPtr->MigrateQFreeCtxt);
			}
		}

	} finally {

		/*
		 * In all cases release the logical group now.
		 */
		if(LgPtr->flags & RPLCC_LG_LOCKED)
		{
			LgPtr->flags &= ~RPLCC_LG_LOCKED;
			OS_InterlockedExchange(&LgPtr->AtomicLock, ATOMIC_FREE);
		}
	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCMemFree \n"));

return(ret);
} /* RplCCMemFree */

/*B**************************************************************************
 * RplCCPinMemSegment - 
 * 
 *  Pin data memory segment into Memory. 
 *  Wrapper around locking paged memory: This allow locking in one place.
 *  Assume Pinned memory cannot go to free list.                                                                                                   
 *
 *E==========================================================================
 */

MMG_PUBLIC
OS_NTSTATUS
RplCCPinMemSegment(PVOID p_start, 
				   ULONG size, 
				   int flags)

/**/
{
	OS_NTSTATUS ret = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCPinMemSegment \n"));

	try {



	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("LeavingRplCCPinMemSegment \n"));

return(ret);
} /* RplCCPinMemSegment */

/*B**************************************************************************
 * RplCCUnpinMemSegment - 
 * 
 *  Unpin data into memory. Wrapper around Unlock paged memory. 
 *
 *E==========================================================================
 */
 
MMG_PUBLIC
OS_NTSTATUS
RplCCUnpinMemSegment(PVOID p_start, ULONG size )

/**/
{
	OS_NTSTATUS ret = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCUnpinMemSegment \n"));

	try {

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCUnpinMemSegment \n"));

return(ret);
} /* RplCCUnpinMemSegment */


/*B**************************************************************************
 * RplCCEmpty - 
 * 
 * Flush Cache per Lg

Remove all data from logical group:

Free list first.
Pending list second with unpinning memory.
Committed list third.
New memory last.
 *
 *E==========================================================================
 */
 
OS_NTSTATUS
RplCCEmpty(CC_logical_grp_t *LgPtr)

/**/
{
	OS_NTSTATUS ret = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCEmpty  \n"));

	try {

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCEmpty \n"));

return(ret);
} /* RplCCEmpty */


/*B**************************************************************************
 * RplCCUnload - 
 * 
 *  Empty Cache per Driver + free memory.                                                                                                       
 *
 *E==========================================================================
 */

OS_NTSTATUS
RplCCUnload() 

/**/
{
	OS_NTSTATUS ret = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCUnload \n"));

	try {

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCUnload \n"));

return(ret);
} /* RplCCUnload */

/*B**************************************************************************
 * RplLgDump - 
 * 
 * Dump all logical group data structures for Debugging.
 *
 *E==========================================================================
 */

RplLgDump(p_lg)

/**/
{
	MMGDEBUG(MMGDBG_LOW, ("Entering RplLgDump \n"));

	try {
		
		/* 
		 * To BE done
		 */

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplLgDump \n"));

} /* RplLgDump */

/*B**************************************************************************
 * RplCCDump - 
 * 
 * Dump all Cache data structures for Debugging.
 * This function doesn't need context or parameters so you can call it
 * directly this function from a debugger or Debugger extension.
 *
 *E==========================================================================
 */

MMG_PUBLIC 
VOID
RplCCDump(VOID)

/**/
{
	OS_LIST_ENTRY   *p_lglnk = NULL;
	CC_logical_grp_t *p_lg    = NULL;

	MMGDEBUG(MMGDBG_LOW, ("*********** Dumping Cache Data Structures ***\n"));

	try {
		
		ASSERT(RplCCCache.sigtype == RPLCC_CC);
		MMGDEBUG(MMGDBG_LOW,("mem_threshold %x \n", RplCCCache.mem_threshold));
		MMGDEBUG(MMGDBG_LOW,("memorySize %x \n", RplCCCache.memorySize));
		
		p_lglnk = RplCCCache.lg_lnk.Flink;
		while (p_lglnk != &RplCCCache.lg_lnk)
		{
			p_lg = CONTAINING_RECORD(p_lglnk, CC_logical_grp_t, lnk);
			RplLgDump(p_lg);
			p_lglnk = p_lg->lnk.Flink;
		}

		RplSlabDump(&RplCCCache.slab_instance);

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("*********** End of Dump *** \n"));

} /* RplCCDump */

#ifdef NOT_USED
/*
 * Some utility functions 
 */

/*B**************************************************************************
 * RplCCAllocDbNode - 
 * 
 *  Database callback.
 * 
 *  Allocate a Database node from the lookaside list in the cache
 *  Anchor, This function limit the visibility of the cache for
 *  safety.
 *
 *E==========================================================================
 */

MMG_PUBLIC
PVOID
RplCCAllocDbNode() 

/**/
{
	PVOID ret = NULL;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCAllocDbNode \n"));

	try {

		if (RplCCCache.sigtype != RPLCC_CC)
		{
			MMGDEBUG(MMGDBG_FATAL, ("Cache not initialized \n"));
			leave;
		}
		ret = OS_ExAllocateFromNPagedLookasideList(&RplCCCache.db_node);

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCAllocDbNode \n"));

return(ret);
} /* RplCCAllocDbNode */

/*B**************************************************************************
 * RplCCDelDbNode - 
 * 
 * Database callback.
 * 
 * Free a Database node from the lookaside list in the cache
 * Anchor, This function limit the visibility of the cache for
 * safety.
 *
 *E==========================================================================
 */

MMG_PUBLIC 
OS_NTSTATUS
RplCCDelDbNode(PVOID p_node)

/**/
{
	OS_NTSTATUS ret = STATUS_SUCCESS;

	ASSERT(p_node);
	ASSERT(IsValidDtbNode(p_node));
	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCDelDbNode \n"));

	try {
		
		if (RplCCCache.sigtype != RPLCC_CC)
		{
			MMGDEBUG(MMGDBG_FATAL, ("Cache not initialized \n"));
			leave;
		}
		OS_ExFreeToNPagedLookasideList(&RplCCCache.db_node, p_node);

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCDelDbNode \n"));

return(ret);
} /* RplCCDelDbNode */
#endif

/* EOF */