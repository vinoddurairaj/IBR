/*HDR************************************************************************
 *                                                                         
 * Softek - Fujitsu                                    
 *
 *===========================================================================
 *
 * N slab.h
 * P Replicator 
 * S prototypes
 * V Generic
 * A J. Christatos - jchristatos@softek.fujitsu.com
 * D 03.05.2004
 * O Interface prototypes to slab allocator
 * T Cache requirements and design specifications - v 1.0.0.
 * C DBG - _KERNEL_ 
 * H 03.05.2004 - Creation - 
 *
 *===========================================================================
 *
 * rcsid[]="@(#) $Id: slab.h,v 1.1 2004/05/29 00:45:28 vqba0 Exp $"
 *
 *HDR************************************************************************/

#ifndef _MMG_SLAB_
#define _MMG_SLAB_

#define MAX_PWR (10)	// nb of power of 2 supported

/*
 * Start of a free list for one order
 */
typedef struct _bd_anchor_
{
	OS_LIST_ENTRY lnk;

} bd_anchor_t;

/*
 * The 'opaque' slab structure
 */
typedef struct _slab_
{
	signatures_e   sigtype;         // Who are we ?

	bd_anchor_t    bd_array[MAX_PWR];

	OS_LOOKASIDE   alist;           // Lookaside list for bd_node(s)
	OS_MEMSECTION  memorySection;   // Data structure maintaining the memory section

	ULONGLONG      slab_size;       // Total size
	ULONGLONG      free_mem;        // Remaining free mem
	PVOID          map_addr;        // Start of mapping
	ULONG          flags;
#define RPLCC_SLAB_INIT (1)

} slab_t;

MMG_PUBLIC OS_NTSTATUS
RplSlabInit(slab_t         *p_sl, 
			LARGE_INTEGER  size);

MMG_PUBLIC OS_NTSTATUS
RplSlabDelete(slab_t *p_sl);

MMG_PUBLIC BOOLEAN
IsValidSlab(slab_t *p_sl);

MMG_PUBLIC PVOID
RplSlabAllocate(slab_t *p_sl, int size, OS_NTSTATUS *p_error);

MMG_PUBLIC OS_NTSTATUS
RplSlabFree(slab_t *p_sl, PVOID p_mem);

MMG_PUBLIC VOID
RplSlabDump(slab_t *p_sl);

#endif _MMG_SLAB_

/* EOF */
