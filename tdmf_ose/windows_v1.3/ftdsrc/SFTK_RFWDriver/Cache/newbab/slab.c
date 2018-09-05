/*HDR************************************************************************
 *                                                                         
 * Softek - Fujitsu                                    
 *
 *===========================================================================
 *
 * N slab.c
 * P Replicator 
 * S Cache & FIFO
 * V Generic
 * A J. Christatos - jchristatos@softek.fujitsu.com
 * D 03.05.2004
 * O Buddy allocator functions for the cache
 *   This is a classical power of 2 buddy allocator.
 *   This is a large size allocator ((1<<PAGE_ALIGN granularity)
 *
 *   ref: [1]'Buddy systems' - J.L. Peterson. 
 *        [2]'Fast (De)Allocation with an improved bd' - E. Demaine and J. Munro.
 *        [3]'A watermark-based lazy buddy system' - Lee and Barkley.
 *
 * T Cache design requirements and design specifications - v 1.0.0.
 * C DBG - _KERNEL_
 * H 03.05.2004 - Creation - 
 *
 *===========================================================================
 *
 * rcsid[]="@(#) $Id: slab.c,v 1.1 2004/05/29 00:45:28 vqba0 Exp $"
 *
 *HDR************************************************************************/

#include "common.h"

#if defined(WIN32)
#if defined(_KERNEL)
#include <ntddk.h>
#include "mmgr_ntkrnl.h"
#else          /* !_KERNEL_*/
#include <windows.h>
#include <stdio.h>
#include "mmgr_ntusr.h"
#endif         /* _KERNEL_ */
#else          /* WIN32    */
#endif         /* _WINDOWS */

#include "slab.h"
#include "mmg.h"

/*
 * All these macros needs to use the p_sl->map_addr variable in local stack.
 *
 * SLAB_TO_VIRT(ADDR) convert from slab offset to virtual address location
 * VIRT_TO_SLAB(ADDR) convert from virtual address to Slab offset
 * 
 */

#define CC_PAGE_ALIGN        (9) // 2^9 == 512

//#define DEBUG 1


#ifdef DEBUG
/*
 * Some debugging functions to access the memory
 */
vaddr_t 
_dbg_slab_to_virt(vaddr_t addr, slab_t *p_sl)
{
	vaddr_t temp = (addr<<CC_PAGE_ALIGN);
	vaddr_t p  = (vaddr_t)p_sl->map_addr + temp;
	ASSERT(temp <= p_sl->slab_size );
return(p);
}
vaddr_t
_dbg_virt_to_slab(vaddr_t addr, slab_t *p_sl)
{
	vaddr_t temp = addr - (vaddr_t)p_sl->map_addr;
	vaddr_t p    = temp>>CC_PAGE_ALIGN;
	ASSERT(p <= p_sl->slab_size );
return(p);
}
#define SLAB_TO_VIRT(ADDR)     _dbg_slab_to_virt(ADDR, p_sl)
#define VIRT_TO_SLAB(ADDR)     _dbg_virt_to_slab(ADDR, p_sl)
#else
#define SLAB_TO_VIRT(ADDR)     ((char *)p_sl->map_addr+(ADDR<<CC_PAGE_ALIGN))
#define VIRT_TO_SLAB(ADDR)     ((vaddr_t)((char *)ADDR - (char *)p_sl->map_addr)>>CC_PAGE_ALIGN)
#endif

#define IS_BLOCK_INUSE(ADDR)   (((mm_hdr_t *)SLAB_TO_VIRT(ADDR))->sigtype == RPLCC_SLHDR)
#define IS_BLOCK_FREE(ADDR)    (((mm_hdr_t *)SLAB_TO_VIRT(ADDR))->sigtype != RPLCC_SLHDR)
#define MARK_BLOCK_INUSE(ADDR) (((mm_hdr_t *)SLAB_TO_VIRT(ADDR))->sigtype = RPLCC_SLHDR)
#define MARK_BLOCK_FREE(ADDR)  (((mm_hdr_t *)SLAB_TO_VIRT(ADDR))->sigtype = 0)
#define ORDER_OFF(ADDR)        ((mm_hdr_t *)SLAB_TO_VIRT(ADDR))->order
#define BACK_PTR(ADDR)         ((mm_hdr_t *)SLAB_TO_VIRT(ADDR))->p_entry

#define ROUNDUP(number)        (((number)+(1<<MAX_PWR<<CC_PAGE_ALIGN) & (~(1<<MAX_PWR<<CC_PAGE_ALIGN))))

/*
 * A free list entry node 
 */
typedef struct _frlst_node_
{
	signatures_e  sigtype;      // Who are we ?
	vaddr_t       start_addr;
	int           order;
    
	OS_LIST_ENTRY lnk;          // next free entry for this order

	ULONG         flags;
#define RPLCC_SLBUSY  (1)
#define RPLCC_SLFREE  (2)

} frlst_node_t;

/*
 * A Memory Block header
 */
typedef struct _mm_hdr_
{

	signatures_e  sigtype;      // Who are we ?
	int           order;		// Where do we start putting back the free mem.
	frlst_node_t  *p_entry;     // Back ptr to the entry in the free list

} mm_hdr_t;

/*B**************************************************************************
 * _GetOrderRange - 
 * 
 * This is a quick and dirty way to find the power of two rank of a number
 *E==========================================================================
 */

int
_GetOrderRange(int size)

/**/
{
	int value,
        count = 1,
	    max_count = 32; // 32bits arithmetics !!!
	
	/*
	 * counts the left shift until we find the 
	 * first significant bit (now the sign bit)
	 */
	value = size;
	while (value>0)
	{
		value <<=1;
		count++;
	}

	/*
	 * If it is not '2^' aligned add one order.
	 */
    value <<=1;
	if (value!=0)
	{
		count--;
	}

return(max_count - count);
}	/* _GetOrderRange */

/*B**************************************************************************
 * RplSlabInit - 
 * 
 * Initialise the slab allocator
 * The minimum allocator size is  (1<<MAX_PWR<<CC_PAGE_ALIGN)
 * We must round the allocation to (1<<MAX_PWR<<CC_PAGE_ALIGN) 
 * multiple units. 
 * 
 *E==========================================================================
 */
MMG_PUBLIC 
OS_NTSTATUS
RplSlabInit(slab_t           *p_sl, 
			LARGE_INTEGER    size)

/**/
{
	OS_NTSTATUS  ret          = STATUS_SUCCESS;
	frlst_node_t *p_FirstFree = NULL;
	ULONGLONG    index;
	ULONGLONG    block_size,
		         nb_blocks;

	ASSERT(p_sl);
	ASSERT(size.QuadPart  != 0);
	MMGDEBUG(MMGDBG_LOW, ("Entering RplSlabInit \n"));

	try {

		if (size.QuadPart <= ((1<<MAX_PWR)<<CC_PAGE_ALIGN))
		{
			MMGDEBUG(MMGDBG_LOW, ("allocator size too small < %d \n", ((1<<MAX_PWR)<<CC_PAGE_ALIGN)));
			ret = STATUS_INVALID_PARAMETER;
			leave;
		}

		/*
		 * Initialize the free list
	 	 */
		for (index = 0; index < MAX_PWR; index++)
		{
			InitializeListHead(&p_sl->bd_array[index].lnk);
		}

		/*
		 * Create node lists.
		 */

		OS_ExInitializeNPagedLookasideList(&p_sl->alist,
		 		 						   NULL,
										   NULL,
										   0,
										   sizeof(frlst_node_t),
										   'dnls', 0);

		/*
		 * Create First entries free list at the top most level
		 */
		block_size = (1<<MAX_PWR);
		nb_blocks  = (size.QuadPart  / (block_size<<CC_PAGE_ALIGN));
		for (index = 0; index < nb_blocks; index++)
		{
			p_FirstFree = (frlst_node_t *)OS_ExAllocateFromNPagedLookasideList(&p_sl->alist);
			if (p_FirstFree == NULL)
			{
				MMGDEBUG(MMGDBG_LOW, ("Cannot allocate first node \n"));
				ret = STATUS_INSUFFICIENT_RESOURCES;
				leave;
			}
			
			p_FirstFree->start_addr = index*block_size;

			p_FirstFree->order      = MAX_PWR;
			p_FirstFree->sigtype    = RPLCC_SLNODE;
			p_FirstFree->flags      = RPLCC_SLFREE;
			
			InitializeListHead(&p_FirstFree->lnk);
			InsertTailList(&p_sl->bd_array[MAX_PWR-1].lnk, &p_FirstFree->lnk);
		}

		p_sl->sigtype  = RPLCC_SLAB;
		p_sl->flags    = RPLCC_SLAB_INIT;

		p_sl->slab_size  = size.QuadPart; 
		p_sl->free_mem   = size.QuadPart;

		/*
		 * Create the file section
		 */
		ret = OS_AllocateSection(&p_sl->memorySection, 
			                     size);

		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_LOW, ("Cannot Allocate memory section %x \n", ret));
			leave;
		}

	} finally {

		/*
		 * rollback ? ...
		 */
		if (!OS_NT_SUCCESS(ret))
		{
/* TBD ... */
			if (OS_IsValidSection(p_sl->memorySection)==TRUE)
			{
				OS_FreeSection(p_sl->memorySection);
			}

		}

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplSlabInit \n"));

return(ret);
} /* RplSlabInit */

/*B**************************************************************************
 * RplSlabDelete - 
 * 
 * Delete the slab allocator
 *E==========================================================================
 */
MMG_PUBLIC 
OS_NTSTATUS
RplSlabDelete(slab_t *p_sl)

/**/
{
	OS_NTSTATUS ret = STATUS_SUCCESS;

	ASSERT(p_sl);
	MMGDEBUG(MMGDBG_LOW, ("Entering RplSlabDelete \n"));

	try {

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplSlabDelete \n"));

return(ret);
} /* RplSlabDelete */

/*B**************************************************************************
 * IsValidSalb - 
 * 
 * Check if the slab is initialized properly
 *E==========================================================================
 */
MMG_PUBLIC
BOOLEAN
IsValidSlab(slab_t *p_sl)

/**/
{
	BOOLEAN ret = TRUE;

	ASSERT(p_sl);
	MMGDEBUG(MMGDBG_LOW, ("Entering IsValidSlab \n"));

	try {
		
		if (p_sl->sigtype != RPLCC_SLAB &&
			!(p_sl->flags & RPLCC_SLAB_INIT))
		{
			ret = FALSE;
		}

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving IsValidSlab \n"));

return(ret);
} /* IsValidSlab */

/*B**************************************************************************
 * RplSlabAllocate - 
 * 
 * Allocate some memory from the buddy allocator
 *
 * We allocate at 2^9 granularity (512).
 * The maximum allocation size for one call is 524000 with MAX_PWR = 10
 * and Mgs with MAX_PWR = 11.
 *
 *E==========================================================================
 */

MMG_PUBLIC
PVOID
RplSlabAllocate(slab_t *p_sl, 
				int size, 
				OS_NTSTATUS *p_error)

/**/
{
	OS_NTSTATUS   ret = STATUS_SUCCESS;
	bd_anchor_t   *p_root     = NULL;
	frlst_node_t  *p_entry    = NULL;
	vaddr_t       p_mem       = {0};
	vaddr_t       StartAddr   = {0};
	int           order,
		          orig_order;
	int           block_size;


	ASSERT(p_sl);
	ASSERT(p_error);
	ASSERT(IsValidSlab(p_sl));
	MMGDEBUG(MMGDBG_LOW, ("Entering RplSlabAllocate \n"));

	try {

		return(OS_ExAllocatePoolWithTag(PagedPool, size, 'tstt'));

		if (p_sl->free_mem  == 0)
		{
			MMGDEBUG(MMGDBG_FATAL, ("no more memory \n"));
			*p_error = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		size += sizeof(mm_hdr_t);
		size >>= CC_PAGE_ALIGN;

		/*
		 * If size is less than a Page we
		 * allocate 1 page
		 */
		if (size == 0)
		{
			size  = 1;
			orig_order = 
				order  = 1;
		} else 
		{
			orig_order = 
				order  = _GetOrderRange(size);
			ASSERT(order >=0 && order <= MAX_PWR);
		}

		do {
			
			p_root = &p_sl->bd_array[order-1];
			if (!IsListEmpty(&p_root->lnk))
			{
				/* Get it out of the list
				*/
				MMGDEBUG(MMGDBG_INFO, ("Found one entry %d \n", order));
				p_entry = CONTAINING_RECORD(p_root->lnk.Flink , frlst_node_t , lnk);
				ASSERT(p_entry->sigtype == RPLCC_SLNODE);
				RemoveEntryList(&p_entry->lnk);
				break;
			}

			order++;
			p_root++;

		} while(order<MAX_PWR+1);

		if (p_entry == NULL)
		{
			MMGDEBUG(MMGDBG_FATAL, ("no more block f given size \n"));
			*p_error = STATUS_INSUFFICIENT_RESOURCES;
			leave;
			/*NOTREACHED*/
		}

		/*
		 * p_entry is now our candidate
		 */
		StartAddr = p_entry->start_addr;

		/*
		 * Now we have an entry.
		 * we return unwanted space to the allocator.
		 * first iteration we reuse p_entry.
		 */
		block_size = (1<<order);
		while (order > orig_order)
		{
			/*
			 * We create an entry of decreasing 2^order size
			 * for each iteration.
			 */
			p_root--;
			order--;
			block_size >>= 1;
			if (p_entry == NULL)
			{
				p_entry = (frlst_node_t *)OS_ExAllocateFromNPagedLookasideList(&p_sl->alist);
				if (p_entry == NULL)
				{
					MMGDEBUG(MMGDBG_LOW, ("Cannot allocate free list node \n"));
					*p_error = STATUS_INSUFFICIENT_RESOURCES;
					leave;
				}
				p_entry->sigtype    = RPLCC_SLNODE;
				InitializeListHead(&p_entry->lnk);
			}
			
			p_entry->order = order;
			InsertTailList(&p_root->lnk, &p_entry->lnk);
			p_mem = 
				p_entry->start_addr = StartAddr;
			
			/*
			 * for Free() below, allow to identify the
			 * block for coalescing without going thru 
			 * the list
			 */
			MARK_BLOCK_FREE(p_mem);
			ORDER_OFF(p_mem)  = order;
			BACK_PTR(p_mem)   = p_entry;
			
			StartAddr += block_size;
			p_entry = NULL;
		}

#ifdef NOP
		/*
		 * Create the file mapping
		 */
	
		ret = OS_MapViewOfSection(p_sl->memorySection, 
			                      (vaddr_t)(block_size<<CC_PAGE_ALIGN), 
								  StartAddr,
								  &p_sl->map_addr);

		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Cannot MappSection %x \n", ret));
			ret = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}
#endif

		/*
		 * Return @ to the user now from the 
		 * smallest suitable entry
		 */

		p_mem = StartAddr;
		p_sl->free_mem  -= (block_size<<CC_PAGE_ALIGN);

		/* 
		 * The first bytes allocated are the headers
		 * We return the address after these bytes
		 * multiplied by the allocation granularity (page size)
		 */
		MARK_BLOCK_INUSE(p_mem);
		ORDER_OFF(p_mem)  = order;
		BACK_PTR(p_mem)   = NULL;
		(char *)p_mem += sizeof(mm_hdr_t);
		*p_error = STATUS_SUCCESS;
		
	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplSlabAllocate \n"));

return((PVOID)SLAB_TO_VIRT(p_mem));
} /* RplSlabAllocate */

/*B**************************************************************************
 * RplSlabFree - 
 * 
 * Free some memory to the slab allocator
 *E==========================================================================
 */

MMG_PUBLIC 
OS_NTSTATUS
RplSlabFree(slab_t *p_sl, PVOID fp_memToFree)

/**/
{
	OS_NTSTATUS    ret      = STATUS_SUCCESS;
	OS_LIST_ENTRY  *p_lnk   = NULL;
	bd_anchor_t    *p_root  = NULL;
	frlst_node_t   *p_entry = NULL;
	vaddr_t         p_mem      = {0};
	vaddr_t         buddy_addr = {0};
	vaddr_t         orig_addr  = {0};
	int             order;
	int             mask;

	ASSERT(p_sl);
	ASSERT(fp_memToFree);
	ASSERT(IsValidSlab(p_sl));
	MMGDEBUG(MMGDBG_LOW, ("Entering RplSlabFree \n"));

	try {

		OS_ExFreePool(fp_memToFree);
		return(ret);

		/*
		 * First retrieve the block header
		 */
		p_mem = VIRT_TO_SLAB(fp_memToFree);
		(char *)p_mem -= sizeof(mm_hdr_t);
		if (IS_BLOCK_FREE(p_mem))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Invalid Memory Block \n"));
			ret = STATUS_INVALID_PARAMETER;
			leave;
		}

		/*
		 * Retrieve the free list
		 */
		order = ORDER_OFF(p_mem);
		ASSERT(order >= 0 && order <= MAX_PWR);
		p_root = &p_sl->bd_array[order-1];
		ASSERT(p_root);

		orig_addr  = 
		buddy_addr = p_mem;

		/*
		 * Now the main loop -
		 * Try to coalesce as much free blocks as we can.
		 */
		mask = (~0UL)<<order;  // 2's complement of 2^order
		p_sl->free_mem  += (-mask)<<CC_PAGE_ALIGN;

		while(mask +(1<<(MAX_PWR-1)))
		{
			/* 
			 * To determine if our buddy is first or second part 
			 * of the 2^order+1 block, it must be a multiple of
			 * 2^order+1. i.e: (start @) mod (2^order+1) == 0
			 * If the buddy is the first part of the 2^order+1 
			 * block we need to change the start @ of the block 
			 * (Start @ - 2^order) for the next iteration to do 
			 * proper coalescing. 
			 * Because we use power of 2, fixed aligned block
			 * addresses and 2's complement arith. the current 
			 * bitwise operations below do this processing quickly. 
			 * Note: to find the buddy flip the (order+1) rightmost 
			 * bit of the current start @. see [2]
			 */

			// flip (k+1)s rightmost bit (-mask is the block size)
			buddy_addr ^= -mask; 
		
			/*
			 * Try to access this memory location
			 * to find if our block was allocated
			 * with the same order size.
			 */
			if (IS_BLOCK_FREE(buddy_addr)      && 
				ORDER_OFF(buddy_addr) == order &&
				BACK_PTR(buddy_addr) !=  NULL)
			{
				/*
				 * This budy block is free, and of our size: Remove entry list
				 */
				MMGDEBUG(MMGDBG_INFO, ("Buddy block allocated\n"));
				p_entry = BACK_PTR(buddy_addr);

				/*
				 * Paranoid, if location match all our
				 * criteria but it is not a valid free node:
				 * We cannot coalesce more.
				 */
				if (p_entry->sigtype != RPLCC_SLNODE)
				{ 
					buddy_addr = orig_addr;
					break;
				}
				
				RemoveEntryList(&p_entry->lnk);
				OS_ExFreeToNPagedLookasideList(&p_sl->alist, p_entry);

				MARK_BLOCK_FREE(orig_addr);
		        ORDER_OFF(orig_addr) = 0;
		        BACK_PTR(orig_addr)  = NULL;

			} else {

				/*
				 * We cannot coalesce more
				 * Create a new entry below
				 */
				buddy_addr = orig_addr;
				break;
			}

			orig_addr = buddy_addr;
			order++;
			mask <<= 1;
			p_root++;

			/* start @ ( -block size, if freed first buddy here) */
			(int)buddy_addr &= mask;     
			
		} /* while */

		p_entry = (frlst_node_t *)OS_ExAllocateFromNPagedLookasideList(&p_sl->alist);
		if (p_entry == NULL)
		{
			MMGDEBUG(MMGDBG_LOW, ("Cannot allocate node \n"));
			ret = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		p_entry->sigtype    = RPLCC_SLNODE;
		p_entry->order      = order;
		p_entry->start_addr = buddy_addr;

		MARK_BLOCK_FREE(p_entry->start_addr);
		ORDER_OFF(p_entry->start_addr) = order;
		BACK_PTR(p_entry->start_addr)  = p_entry;
		InsertHeadList(&p_root->lnk, &p_entry->lnk);

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplSlabFree \n"));

return(ret);
} /* RplSlabFree */

/*B**************************************************************************
 * RplSlabDump - 
 * 
 * Free some memory to the slab allocator
 *E==========================================================================
 */
MMG_PUBLIC 
VOID
RplSlabDump(slab_t *p_sl)

/**/
{
	int index;
	ULONGLONG      total_free;
	bd_anchor_t   *p_root  = NULL;
	frlst_node_t  *p_entry = NULL;
	OS_LIST_ENTRY *p_lnk   = NULL;


	ASSERT(p_sl);
	MMGDEBUG(MMGDBG_LOW, ("Entering RplSlabDump \n"));

	try {
	
		ASSERT(IsValidSlab(p_sl));

		/*
		 * Initialize the free list
	 	 */
		MMGDEBUG(MMGDBG_INFO, ("Slab size %d \n", p_sl->slab_size ));
		MMGDEBUG(MMGDBG_INFO, ("free space %d \n", p_sl->free_mem ));

		total_free = 0;
		for (index=0; index < MAX_PWR; index++)
		{

			MMGDEBUG(MMGDBG_INFO, ("index[%d] (order %d)\n", index, index+1));
			MMGDEBUG(MMGDBG_INFO, ("block size %d \n", (1<<(index+1))));

			p_root = &p_sl->bd_array[index];
			p_lnk  = p_root->lnk.Flink;
			while (p_lnk != &p_root->lnk)
			{
				/* print entry
				*/
				p_entry = CONTAINING_RECORD(p_lnk , frlst_node_t , lnk);
				ASSERT(p_entry->sigtype == RPLCC_SLNODE);
				MMGDEBUG(MMGDBG_INFO, ("Found one entry %lx\n", (unsigned long long)p_entry));
				MMGDEBUG(MMGDBG_INFO, ("flags   %x, ", p_entry->flags));
				MMGDEBUG(MMGDBG_INFO, ("order   %d, ", p_entry->order));
				MMGDEBUG(MMGDBG_INFO, ("start @ %d, %x ", p_entry->start_addr, (unsigned long long)SLAB_TO_VIRT(p_entry->start_addr)));
				MMGDEBUG(MMGDBG_INFO, ("End @ %d \n", (p_entry->start_addr+(1<<(index+1))-1)));
				total_free += (1<<(index+1))<<CC_PAGE_ALIGN;
				p_lnk = p_lnk->Flink;
			}
		} /* for */

		MMGDEBUG(MMGDBG_INFO, ("free space %d \n", (total_free <<CC_PAGE_ALIGN)));
		if (total_free  != p_sl->free_mem )
		{
			MMGDEBUG(MMGDBG_FATAL, ("[****]  total_free != p_sl->free_mem [****] \n"));
			ASSERT(TRUE==FALSE);
		}

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplSlabDump \n"));

} /* RplSlabDump */

/* EOF */