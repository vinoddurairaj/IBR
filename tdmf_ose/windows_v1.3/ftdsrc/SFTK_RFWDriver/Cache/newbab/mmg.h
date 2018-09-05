/*HDR************************************************************************
 *                                                                         
 * Softek - Fujitsu                                    
 *
 *===========================================================================
 *
 * N mmg.h
 * P Replicator 
 * S Common definitions, prototypes
 * V Generic
 * A J. Christatos - jchristatos@softek.fujitsu.com
 * D 02.25.2004
 * O Interface prototypes to cache.
 * T Cache requirements and design specifications - v 1.0.0.
 * C DBG - _KERNEL_ 
 * H 02.25.2004 - Creation - 
 *
 *===========================================================================
 *
 * rcsid[]="@(#) $Id: mmg.h,v 1.4 2004/06/17 22:03:27 jcq40 Exp $"
 *
 *HDR************************************************************************/

#ifndef _MMG_
#define _MMG_

/*
 ********************************** parameters structures ********************************************
 */

typedef enum 
{
	RPLCC_REGULARWRITE = 1,
	RPLCC_REFRESHWRITE = 2

} Qtype_e;

/*
 * Various flags passed to APIs
 */
#define RPLCC_WAITDATA (1)		// Wait for incoming data
#define RPLCC_NOWAIT   (2)      // Don't even dare to wait

/*
 * This Structure describe an IO request coming from he storage stack
 */

typedef struct _rplcc_iorp_
{
	PVOID         DevicePtr;     // Windows Device ptr.
	ULONG         DeviceId;      // bdev;
	ULONGLONG     Blk_Start;     // 64bits;
	ULONG         Size;
	PVOID         pBuffer;
	PVOID         pHdlEntry;     // Ptr to cache entry.
	sentinels_e   SentinelType;  // if the data is a Sentinel

} rplcc_iorp_t;

/*
 ********************************** PUBLIC Data structures *******************************************
 */

#define RPLCC_MAXFIFOENTRY (100)  // Maximum nb of entries per fifo chunk

/*
 * What to do with cache node entry after iteration
 */
typedef enum 
{

	NODEACTION_NOTHING = 1,
	NODEACTION_DELETE  = 2 

} CC_nodeaction_t;

/*
 * A 'special' pool descriptor
 */

typedef enum 
{
	RPLCC_TDI_POOL     = 1,
	RPLCC_REFRESH_POOL = 2

} PoolType_t;

typedef enum
{
	WRAP   = 1,
	NOWRAP = 2

} Policy_t;

typedef struct 
{
	signatures_e    sigtype;	  // Who are we ?
	PoolType_t      poolt;
	Policy_t        policy;
	int             Id;
	int             chunk_size;
	int             num_chunk;
	PVOID           p_start;
	PVOID           p_internal;  // ptr to internal pool_t

} PoolDescr_t;

/*
 * State of Logical group
 */
typedef enum 
{
	REFRESH  = 1,
	NORMAL   = 2,
	TRACKING = 3,
	PASSTHRU = 4

} CC_lgstate_t;

/* 
 * A logical group
 */
typedef enum 
{
	RPLCC_LG_INIT    = (1),     // LG has been init	
	RPLCC_LG_PENDING = (2),     // LG has pending IOS
	RPLCC_LG_COMIT	 = (4),     // LG has committed data
	RPLCC_LG_LOCKED  = (8),     // LG is locked
	RPLCC_LG_MIGRATE = (0x10),  // LG has migrate(free) headers
	RPLCC_LG_PINMEM  = (0x20),  // LG has pinned memory
	RPLCC_LG_WBLOCK  = (0x40)   // LG is Waiting for Pending Block

} lgflags_e;

 
typedef struct _lgt_ 
{
	signatures_e    sigtype;	      // Who are we ?
	lgflags_e       flags;            // State of logical group

	PVOID           p_backtr;         // Back ptr to device LG.

    //    Callback to get LG State
	CC_lgstate_t    (*GetState)(PVOID);      

	// Callback when refresh Q is empty
	VOID            (*RefreshQFreeCB)(PVOID);

	// Callback when migrate Q is empty
	VOID            (*MigrateQFreeCB)(PVOID); 
	
	PVOID           RefreshQFreeCtxt;
	PVOID           MigrateQFreeCtxt;

	// Various ...

	OS_LOOKASIDE    hdr_alist;      // lookaside list of headers
	OS_LOOKASIDE    miga_alist;     // migrate anchor list.

	OS_LIST_ENTRY   PendingQ;       // Pending Q
    OS_LIST_ENTRY   CommitQ;        // Commit Q linked list
	OS_LIST_ENTRY   refreshQ;       // start of refresh Q.
	OS_LIST_ENTRY   migrateQ;       // Before freeing headers.
	OS_LIST_ENTRY   lnk;			// next logical group in cache
	ULONG           AtomicLock;		// A pseudo spinlock

	// Synchronisation Events

	OS_SYNC_EVENT   waitForBlock;	// The pending block IO is committed
	OS_SYNC_EVENT   waitForData;	// New data

	// Statistics

	int				nb_devices;
	int				hotgrp_limit;	// hot group thresholds
	ULONGLONG       total_pending;  // total pending bytes
	ULONGLONG       total_commit;   // total committed bytes
	             
} CC_logical_grp_t;

MMG_PUBLIC OS_NTSTATUS 
RplCCInit(int (*AllocCallBack)(), 
		  int (*FreeCallBack)(), 
		  int mem_threshold,
		  ftd_lg_info_t *p_config);

MMG_PUBLIC OS_NTSTATUS
RplCCLgInit(PVOID            DeviceLG,
			CC_logical_grp_t **LgPtr,
			CC_lgstate_t     (*GetState)(PVOID),
			VOID             (*RFFreeCB)(PVOID),
			PVOID            RFFreeCtxt,
			VOID             (*MGFreeCB)(PVOID),
			PVOID            MGFreeCtxt,
			ftd_lg_info_t    *p_config);

MMG_PUBLIC OS_NTSTATUS
RplCCAllocateSpecialPool(IN OUT PoolDescr_t *p_pool);

MMG_PUBLIC OS_NTSTATUS
RplCCGetNextBfromPool(IN PoolDescr_t  *p_pool,
					  OUT PVOID       *p_callerbuf,
					  IN OS_NTSTATUS  (*callback)(PVOID pBuffer, PVOID p_ctxt),
					  IN PVOID        p_ctxt
					  );

MMG_PUBLIC OS_NTSTATUS 
RplCCLGIterateEntries(IN CC_logical_grp_t *LgPtr, 
			   	      IN CC_nodeaction_t  (__fastcall *callback)(PVOID, rplcc_iorp_t *),
				      PVOID               p_PersistCtxt );

MMG_PUBLIC OS_NTSTATUS
RplCCLGIterateAndRemoveEntries(CC_logical_grp_t   *LgPtr, 
				               CC_nodeaction_t    (__fastcall *callback)
				                                       (PVOID        p_PersistCtxt, 
				                                       rplcc_iorp_t *iorq),
				               PVOID              p_PersistCtxt
				               );

MMG_PUBLIC OS_NTSTATUS 
RplCCWrite(IN CC_logical_grp_t *LgPtr, 
		   IN OUT rplcc_iorp_t *p_iorequest,
		   IN Qtype_e           type_Q );

MMG_PUBLIC BOOLEAN
RplCCIsDataCached(IN CC_logical_grp_t *LgPtr, IN rplcc_iorp_t *p_iorequest);

MMG_PUBLIC OS_NTSTATUS
RplCCPinMemSegment(PVOID p_start, ULONG size, int flags);

MMG_PUBLIC OS_NTSTATUS
RplCCUnpinMemSegment(PVOID p_start, ULONG size );

MMG_PUBLIC OS_NTSTATUS
RplCCMemCommitted(CC_logical_grp_t *LgPtr, IN rplcc_iorp_t *p_iorequest);

MMG_PUBLIC OS_NTSTATUS
RplCCGetNextSegment(CC_logical_grp_t *LgPtr,  ULONG Flags, IN rplcc_iorp_t *p_iorequest);

MMG_PUBLIC OS_NTSTATUS
RplCCMemFree(CC_logical_grp_t *LgPtr, IN rplcc_iorp_t *p_iorequest);

MMG_PUBLIC VOID
RplCCDump(VOID);

#endif /* _MMG_*/

/* EOF */