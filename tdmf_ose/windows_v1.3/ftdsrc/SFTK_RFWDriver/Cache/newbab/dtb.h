/*HDR************************************************************************
 *                                                                         
 * Softek - Fujitsu                                    
 *
 *===========================================================================
 *
 * N dtb.h
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
 * rcsid[]="@(#) $Id: dtb.h,v 1.1 2004/05/29 00:45:29 vqba0 Exp $"
 *
 *HDR************************************************************************/

#ifndef _MMG_DTB_
#define _MMG_DTB_

/*
 * For tree balancing
 */
typedef enum
{

	RED,
	BLACK

} node_color_t;

/*
 * The 'opaque' database structure
 * 
 * This node lives inside the header for each IO entry in the cache.
 * This allow us to 'decouple' the database from the compatibility 
 * mode wlheader_t.
 */
typedef struct _dtbnode_
{
	signatures_e      sigtype;
	struct _dtbnode_  *p_parent;
	
	OS_LIST_ENTRY     lnk;       // Convention Left == Blink, Right == Flink
	node_color_t      color;     // for balancing. 

	PVOID         DeviceId;
	ULONGLONG     Blk_Start;
	ULONG         Size;
	//ULONGLONG
		unsigned __int64 IntervalLow,   // The key
		          IntervalHigh,
				  MaximumHigh;

} dtbnode_t;


MMG_PUBLIC OS_NTSTATUS
RplCCDtbInit(OS_LIST_ENTRY rootnode_lnk);

MMG_PUBLIC OS_NTSTATUS
RplCCDtbDel(OS_LIST_ENTRY rootnode_lnk);

MMG_PUBLIC BOOLEAN
IsValidDtbNode(PVOID p_node);

MMG_PUBLIC SIZE_T
SizeOfDtbNode();

MMG_PUBLIC OS_NTSTATUS
RplDtbInsertNode(OS_LIST_ENTRY *rootnode_lnk, dtbnode_t *p_hdr);

MMG_PUBLIC
OS_NTSTATUS
RplDtbDeleteNode(OS_LIST_ENTRY *p_rootlnk, dtbnode_t *p_node);

MMG_PUBLIC BOOLEAN
RplDtbSearch(OS_LIST_ENTRY *p_rootlnk, dtbnode_t *p_searchnode);

MMG_PUBLIC OS_NTSTATUS
RplDtbDeleteNode(OS_LIST_ENTRY *p_rootlnk, dtbnode_t *p_hdr);

MMG_PUBLIC VOID
RplDtbTraverseTree(OS_LIST_ENTRY   *p_rootlnk, 
				   CC_nodeaction_t (__fastcall *callback)(PVOID, rplcc_iorp_t *),
				   PVOID p_ctxt
				   );


MMG_PUBLIC VOID
RplDtbDump(OS_LIST_ENTRY *p_rootlnk);

#endif  /*_MMG_DTB_ */
/* EOF */