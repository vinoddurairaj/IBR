/*HDR************************************************************************
 *                                                                         
 * Softek - Fujitsu                                    
 *
 *===========================================================================
 *
 * N usr_main.c
 * P Replicator 
 * S User space functions
 * V Windows Kernel specific
 * A J. Christatos - jchristatos@softek.fujitsu.com
 * D 03.05.2004
 * O User space unit test framework for the cache
 * T Cache requirements and design specifications - v 1.0.0.
 * C DBG - 
 * H 03.05.2004 - Creation - 
 *
 *===========================================================================
 *
 * rcsid[]="@(#) $Id: usr_main.c,v 1.5 2004/06/17 22:03:27 jcq40 Exp $"
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
 * Pseudo Device LG structure
 * to test the callback mechanism
 */
typedef struct 
{

	CC_lgstate_t State;
	int          flags;

} DeviceLG_t;

/* ******************************************* TREE TESTING ***************************** */

#ifdef NO_TREE

/* keys to feed the tree */
static int values[50][2] = 
{
	{0,3}, {5,3}, {6,4},
	{15,8}, {16,5}, {25,5},
	{17,2}, {19,1}, {26,0},
	{8,1}
};

/* keys to query the tree */
static int query[20][3] = 
{
	{9,7,TRUE},{12,1,FALSE},
	{12,7,TRUE},{40,1,FALSE}
};

/* nodes to remove */
static int delorder[20] = 
{
	7, 4, 8, 9, 10, 1, 0
};


/*B**************************************************************************
 * Test_tree - 
 * 
 * Unit Test the dtb.c red/black tree package.
 * All Device ID must be the same to check on one database only
 *E==========================================================================
 */

BOOLEAN
Test_tree()

/**/
{	
	BOOLEAN        ret = TRUE;
	OS_NTSTATUS    ntstatus;
	dtbnode_t      node_array[50];
	OS_LIST_ENTRY  thedatabase;
    DeviceLG_t     dummyLG;
	int            i;
	int o;

	MMGDEBUG(MMGDBG_LOW, ("Entering Test_tree \n"));

	try {

		/*
		 * Emulate logical group part
		 */
		InitializeListHead(&thedatabase);
		ntstatus = RplCCDtbInit(thedatabase);
		if (!OS_NT_SUCCESS(ntstatus))
		{
			MMGDEBUG(MMGDBG_LOW, ("Cannot initialize database %x \n", ret));
			ret = FALSE;
			leave;
		}

		dummyLG.flags = 0xcafecafe;
		dummyLG.State = REFRESH;

		/*
		 * do some insertions 
		 */
		for (i = 0; i<10; i++)
		{

			o = sizeof(dtbnode_t);
			node_array[i].Blk_Start   = values[i][0];
			node_array[i].DeviceId    = &dummyLG;
			node_array[i].Size        = values[i][1];
			node_array[i].sigtype     = RPLCC_DTB;
			node_array[i].lnk.Blink   = NULL;
			node_array[i].lnk.Flink   = NULL;

			ntstatus = RplDtbInsertNode(&thedatabase , &node_array[i]);
			if (!OS_NT_SUCCESS(ntstatus))
			{
				MMGDEBUG(MMGDBG_LOW, ("Cannot insert node %x \n", ret));
				ret = FALSE;
				leave;
			}
		}
		
		RplDtbDump(&thedatabase);

		for (i=0;i<4; i++)
		{
			node_array[11].Blk_Start  = query[i][0];
			node_array[11].DeviceId   = &dummyLG;
			node_array[11].Size       = query[i][1];
			node_array[11].sigtype    = RPLCC_DTB;
			node_array[11].lnk.Blink  = NULL;
			node_array[11].lnk.Flink  = NULL;

			/*
			 * normalize node - this is done by cache
			 * but here we don't have a valid LG
			 */
			node_array[11].IntervalLow  = node_array[11].Blk_Start;
		    node_array[11].IntervalHigh = node_array[11].IntervalLow + 
			                                         node_array[11].Size;
			if ((ret=RplDtbSearch(&thedatabase , &node_array[11])) != query[i][2])
			{
				MMGDEBUG(MMGDBG_LOW, ("Search didn't match %d, %d\n",
						node_array[11].IntervalLow,
						node_array[11].IntervalHigh));
				MMGDEBUG(MMGDBG_LOW, ("Expected %s got %s \n",
						(query[i][2]==TRUE)?"TRUE":"FALSE",
						(ret==TRUE)?"TRUE":"FALSE" ));

				ret = FALSE;
				leave;
			} else 
			{
				MMGDEBUG(MMGDBG_LOW, ("Test %d, %d passed \n",
				  		node_array[11].IntervalLow,
						node_array[11].IntervalHigh));
			}
		} /* for */


		ret = TRUE;
		for (i=0;i<7; i++)
		{
			MMGDEBUG(MMGDBG_LOW, ("Deleting node %d, %d  \n",
					node_array[delorder[i]].IntervalLow,
						node_array[delorder[i]].IntervalHigh));
			if (!OS_NT_SUCCESS(RplDtbDeleteNode(&thedatabase, &node_array[delorder[i]])))
			{
				MMGDEBUG(MMGDBG_LOW, ("Cannot remove %d, %d  \n",
					node_array[delorder[i]].IntervalLow,
						node_array[delorder[i]].IntervalHigh));
				ret = FALSE;
				leave;
			}
			RplDtbDump(&thedatabase);
		}  /* for */
		
	} finally {
	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving Test_tree \n"));

return(ret);
} /* Test_tree */
#endif // NO_TREE
/* ******************************************* TREE TESTING ***************************** */


/* ************************************ Memory Allocator Testing ************************ */

#define MAX_TRY (10)

typedef enum 
{
	ALLOCATE,
	FREE

} testtype_e;

/*
 * This is for unit testing
 */
typedef struct _array_
{
	testtype_e tstcase;
	int        param;
	int        result;
	PVOID      mem;

} test_array_t;

/*
 * This is a Unit test run on 1024 bytes free mem segment
 * It does execise all the code path.
 * This is with a 1024 byte memory AND MAX_PWR == 10
 *
 * after 1 allocation:
 * Allocation absolute (-slab->map_addr) start @ 896 for 128 bytes
 * Free list :
 * order   7, start @ 768, End @ 895
 * order   8, start @ 512, End @ 767
 * order   9, start @ 0, End @ 511
 * 
 * after 2 allocation:
 * Allocation absolute (-slab->map_addr) start @ 832 for 64 bytes
 * Free list :
 * order   6, start @ 768, End @ 831
 * order   8, start @ 512, End @ 767
 * order   9, start @ 0, End @ 511
 *
 * after 3 allocation:
 * Allocation absolute (-slab->map_addr) start @ 768 for 64 bytes
 * Free list :
 * order   8, start @ 512, End @ 767
 * order   9, start @ 0, End @ 511
 *
 * after 4 allocation:
 * Allocation absolute (-slab->map_addr) start @ 704 for 64 bytes
 * Free list :
 * order   6, start @ 640, End @ 703
 * order   7, start @ 512, End @ 639
 * order   9, start @ 0, End @ 511
 *
 * after 5 allocation:
 * Allocation absolute (-slab->map_addr) start @ 640 for 64 bytes
 * Free list :
 * order   7, start @ 512, End @ 639
 * order   9, start @ 0, End @ 511

 * For Free:
 * 
 * Free [3]
 * Free List:
 *  order   6, start @ 704, End @ 767
 *  order   7, start @ 512, End @ 639
 *  order   9, start @ 0, End @ 511
 *
 * Free [2]
 * Free list:
 *  order   6, start @ 704, End @ 767
 *  order   6, start @ 768, End @ 831
 *  order   7, start @ 512, End @ 639
 *  order   9, start @ 0, End @ 511
 *
 * Free [0]
 * Free List:
 *  order   6, start @ 704, End @ 767
 *  order   6, start @ 768, End @ 831
 *  order   7, start @ 512, End @ 639
 *  order   7, start @ 896, End @ 1023
 *  order   9, start @ 0, End @ 511
 *
 * Free [4]
 * Free List:
 *  order   6, start @ 768, End @ 831
 *  order   7, start @ 896, End @ 1023
 *  order   8, start @ 512, End @ 767
 *  order   9, start @ 0, End @ 511
 * 
 */
test_array_t test_array[MAX_TRY] = 
{
	{ALLOCATE, 110*4096 , 0},   // 0: 
	{ALLOCATE, 46*4096, 0},     // 1: 
	{ALLOCATE, 46*4096, 0},     // 2:
	{ALLOCATE, 46*4096, 0},     // 3: 
	{ALLOCATE, 46*4096, 0},     // 4: 
	{FREE, 3 , 0}, 
	{FREE, 2, 0}, 
    {FREE, 0, 0},
	{FREE, 4, 0},
	{ALLOCATE, 26, 0}, 
};

BOOLEAN
Test_Allocator()

/**/
{
	int               ret = 0;
	int			      i=0;
	LARGE_INTEGER     size;

static slab_t slab;

	/* 
	 * Emulate cache init
	 */
	size.QuadPart = 8*1024*1024;
	RplSlabInit(&slab, size);

	for (i=0; i< MAX_TRY; i++)
	{
		switch (test_array[i].tstcase)
		{
		case ALLOCATE:
			test_array[i].mem = RplSlabAllocate(&slab, test_array[i].param, &ret);
			if (ret != test_array[i].result)
			{
				MMGDEBUG(MMGDBG_FATAL,("test failed [%d]\n", i));
				return FALSE;
			} else 
			{
				MMGDEBUG(MMGDBG_FATAL,("test passed [%d]\n", i));
			}
			RplSlabDump(&slab);
			break;

		case FREE:
			ret = RplSlabFree(&slab, test_array[test_array[i].param].mem);
			if (ret != test_array[i].result)
			{
				MMGDEBUG(MMGDBG_FATAL,("test failed [%d]\n", i));
				return FALSE;
			} else 
			{
				MMGDEBUG(MMGDBG_FATAL,("test passed [%d]\n", i));
			}
			RplSlabDump(&slab);
			break;

		default:
			MMGDEBUG(MMGDBG_FATAL,("unknown parameter [%d]\n", i));
			return FALSE;
		} /* switch */
	}

return TRUE;
}
/* End Memory allocator Testing */

/*
 * Callback for cache GetNextSegment()
 * to find the state of the current LG.
 */
CC_lgstate_t
GetLGState(DeviceLG_t *p_ctxt)

/**/
{
	return(p_ctxt->State);
} /* GetLGState */

/*
 * Callback when the refresh Q is empty.
 */
VOID
RefreshQIsEmptyCB(PVOID p_ctxt)

/**/
{
	MMGDEBUG(MMGDBG_INFO,("RefreshQIsEmptyCB \n"));

} /* RefreshQIsEmptyCB */

/*
 * Callback when the migrate Q is empty
 */
VOID
MigrateQIsEmptyCB(PVOID p_ctxt)

/**/
{
	MMGDEBUG(MMGDBG_INFO,("MigrateQIsEmptyCB \n"));

} /* MigrateQIsEmptyCB */

/*
 * Callback test for rebuildbitmap
 * Called by RplccLGForeachNode()
 */

CC_nodeaction_t
__fastcall
CBBuildBitmap(PVOID p_PersistCtxt, rplcc_iorp_t *iorq)

/**/
{
	MMGDEBUG(MMGDBG_LOW,("CBBuildBitmap \n"));
	MMGDEBUG(MMGDBG_INFO, ("iorq->DevicePtr %lx ", (ULONGLONG)iorq->DevicePtr));
	MMGDEBUG(MMGDBG_INFO, ("iorq->DeviceId %d   ", iorq->DeviceId ));
	MMGDEBUG(MMGDBG_INFO, ("iorq->Blk_Start %lx ", iorq->Blk_Start ));
	MMGDEBUG(MMGDBG_INFO, ("iorq->Size %d       ", iorq->Size ));
	MMGDEBUG(MMGDBG_INFO, ("iorq->pBuffer %lx \n", (ULONGLONG)iorq->pBuffer));

return(NODEACTION_NOTHING);
}

/*
 * Callback test for new free buffer pool
 * Called by RplCCGetNextBfromPool()
 */

OS_NTSTATUS
CBFreeBuffer(PVOID pBuffer, PVOID p_ctxt)

/**/
{
	MMGDEBUG(MMGDBG_LOW, ("CBFreeBuffer \n"));
	MMGDEBUG(MMGDBG_INFO, ("pBuufer %lx ", (ULONGLONG)pBuffer));
	MMGDEBUG(MMGDBG_INFO, ("p_context %x   ", (ULONGLONG)p_ctxt));

	return(STATUS_SUCCESS);
}

/*B**************************************************************************
 * UnitTest - 
 * 
 * Test the Entire Cache Behaviour in Kernel/User Space
 *E==========================================================================
 */

OS_NTSTATUS
UnitTest()

/**/
{
	OS_NTSTATUS      ret = STATUS_SUCCESS;
	CC_logical_grp_t *p_lgp;
	PoolDescr_t      Apool;
	rplcc_iorp_t     Arequest;
	ftd_lg_info_t    config;

	DeviceLG_t       dummyLG1, 
		             dummyLG2,
					 dummyLG3, // for refreshQ
					 dummyLG4; // for migrateQ

	PVOID            CommitArray[10];

	MMGDEBUG(MMGDBG_LOW, ("Entering main \n"));

	try {


		ret = RplCCInit(NULL, NULL, 100, &config);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Error Initializing the cache \n"));
			leave;
		}

#ifdef NOP
		/*
		 * stand alone test the allocator
		 */
		if (Test_Allocator()==FALSE)
		{
			MMGDEBUG(MMGDBG_FATAL, ("Error in Test_allocator()\n"));
			leave;
		}
#endif

#ifdef NO_TREE
		/*
		 * stand alone test the binary tree package
		 */
		if (Test_tree()==FALSE)
		{
			MMGDEBUG(MMGDBG_FATAL, ("Error in  Test_tree()\n"));
			leave;
		}
#endif


		dummyLG1.flags = 0x1;
		dummyLG1.State = REFRESH;
		dummyLG2.flags = 0x2;
		dummyLG2.State = REFRESH;

		ret = RplCCLgInit(&dummyLG2, 
			              &p_lgp, 
						  GetLGState, 
						  RefreshQIsEmptyCB,
						  &dummyLG3,
						  MigrateQIsEmptyCB,
						  &dummyLG4,
						  &config);

		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Error Initializing logical group 1 \n"));
			leave;
		}

        ret = RplCCLgInit(&dummyLG1, 
			              &p_lgp, 
						  GetLGState, 
						  RefreshQIsEmptyCB,
						  &dummyLG3,
						  MigrateQIsEmptyCB,
						  &dummyLG4,
						  &config);

		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Error Initializing logical group 2 \n"));
			leave;
		}

		/*
		 * Allocate a Special Pool
		 */

		Apool.policy      = WRAP;
		Apool.poolt       = RPLCC_REFRESH_POOL;
		Apool.chunk_size  = 1024;
		Apool.num_chunk   = 4;
		ret = RplCCAllocateSpecialPool(&Apool);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Error Allocating Pool 1 \n"));
			leave;
		}

		/*
		 * Send some request to the refresh pool
		 */	
		Arequest.pBuffer  = NULL;
		ret = RplCCGetNextBfromPool(&Apool, &Arequest.pBuffer, CBFreeBuffer, NULL);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Error Allocating Pool 1 \n"));
			leave;
		}

		ASSERT(Arequest.pBuffer != NULL);

		Arequest.Blk_Start    = 0x00;
		Arequest.Size         = Apool.chunk_size;
		Arequest.DevicePtr    = &dummyLG1;
		Arequest.SentinelType = MSGNONE;
		RtlFillMemory(Arequest.pBuffer, Apool.chunk_size, 'B');
		ret = RplCCWrite(p_lgp, &Arequest, RPLCC_REFRESHWRITE);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Error Sending write request 1 \n"));
			leave;
		}

		/*
		 * Send some request to the refresh pool
		 */	
		Arequest.pBuffer  = NULL;
		ret = RplCCGetNextBfromPool(&Apool, &Arequest.pBuffer, CBFreeBuffer, NULL);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Error buffer Pool 2 \n"));
			leave;
		}

		ASSERT(Arequest.pBuffer != NULL);

		Arequest.Blk_Start    = Apool.chunk_size+1;
		Arequest.Size         = Apool.chunk_size;
		Arequest.DevicePtr    = &dummyLG1;
		Arequest.SentinelType = MSGNONE;
		RtlFillMemory(Arequest.pBuffer, Apool.chunk_size, 'C');
		ret = RplCCWrite(p_lgp, &Arequest, RPLCC_REFRESHWRITE);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Error Sending write request 2 \n"));
			leave;
		}

		/*
		 * Send some request to the refresh pool
		 */	
		Arequest.pBuffer  = NULL;
		ret = RplCCGetNextBfromPool(&Apool, &Arequest.pBuffer, CBFreeBuffer, NULL);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Error Allocating Pool 3 \n"));
			leave;
		}

		ASSERT(Arequest.pBuffer != NULL);

		Arequest.Blk_Start    = 2*Apool.chunk_size;
		Arequest.Size         = Apool.chunk_size;
		Arequest.DevicePtr    = &dummyLG1;
		Arequest.SentinelType = MSGNONE;
		RtlFillMemory(Arequest.pBuffer, Apool.chunk_size, 'D');
		ret = RplCCWrite(p_lgp, &Arequest, RPLCC_REFRESHWRITE);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Error Sending write request 3 \n"));
			leave;
		}

		/*
		 * Send some writes to the cache 
		 * They should not overwrite with
		 * the refresh writes.
		 */
		Arequest.Blk_Start    = 0x32;
		Arequest.Size         = 200;
		Arequest.SentinelType = MSGNONE;
		Arequest.DevicePtr    = &dummyLG1;
		Arequest.pBuffer      = OS_ExAllocatePoolWithTag(PagedPool,
			                                             Arequest.Size+1, 'UnTs');
		ASSERT(Arequest.pBuffer);
		RtlFillMemory(Arequest.pBuffer, Arequest.Size, 'A');

		ret = RplCCWrite(p_lgp, &Arequest, RPLCC_REGULARWRITE);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Error Sending write request 4 \n"));
			leave;
		}

		OS_ExFreePool(Arequest.pBuffer);
		
		/*
		 * Send some writes to the cache 
		 */
		Arequest.Blk_Start    = 0x400;
		Arequest.Size         = 100;
		Arequest.SentinelType = MSGNONE;
		Arequest.DevicePtr    = &dummyLG1;
		Arequest.pBuffer      = OS_ExAllocatePoolWithTag(PagedPool,
			                                             Arequest.Size+1, 'UnTs');
		ASSERT(Arequest.pBuffer);
		RtlFillMemory(Arequest.pBuffer, Arequest.Size, 'E');

		ret = RplCCWrite(p_lgp, &Arequest, RPLCC_REGULARWRITE);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Error Sending write request 5 \n"));
			leave;
		}

		OS_ExFreePool(Arequest.pBuffer);
		CommitArray[0] = Arequest.pHdlEntry;

		/*
		 * Send some writes to the cache 
		 */
		Arequest.Blk_Start   = 0x400;
		Arequest.Size        = 100;
		Arequest.SentinelType = MSGNONE;
		Arequest.DevicePtr   = &dummyLG1;
		Arequest.pBuffer     = OS_ExAllocatePoolWithTag(PagedPool,
			                                               Arequest.Size+1, 'UnTs');
		ASSERT(Arequest.pBuffer);
		RtlFillMemory(Arequest.pBuffer, Arequest.Size, 'H');

		ret = RplCCWrite(p_lgp, &Arequest, RPLCC_REGULARWRITE);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Error Sending write request 6 \n"));
			leave;
		}

		OS_ExFreePool(Arequest.pBuffer);
		CommitArray[1] = Arequest.pHdlEntry;

		/*
		 * Send a sentinel to the cache.
		 * it is automatically committed.
		 */
		Arequest.Blk_Start    = 0;
		Arequest.Size         = 0;
		Arequest.SentinelType = MSGINCO;
		Arequest.DevicePtr    = &dummyLG1;
		Arequest.pBuffer      = NULL;

		ret = RplCCWrite(p_lgp, &Arequest, RPLCC_REGULARWRITE);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Error Sending write request 6 \n"));
			leave;
		}

		RplCCDump();

		/*
		 * Test the callback mechanism
		 */
		ret = RplCCLGIterateEntries(p_lgp, 
				                    CBBuildBitmap,
				                    NULL		   );

		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Error Sending write request 1 \n"));
			leave;
		}


		/*
		 * Get Data from the refresh pool
		 */
		Arequest.Size      = (3*Apool.chunk_size)+sizeof(ftd_header_t);
		Arequest.pBuffer   = OS_ExAllocatePoolWithTag(PagedPool,
			                                          Arequest.Size+1, 'UnTs');
		ASSERT(Arequest.pBuffer);
		ret = RplCCGetNextSegment(p_lgp,  
							      RPLCC_WAITDATA, 
							      &Arequest);

		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("not enough buffer data %d \n",  Arequest.Size));
		}

		OS_ExFreePool(Arequest.pBuffer);

		ret = RplCCMemFree(p_lgp, &Arequest);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("cannot free the request \n"));
			leave;
		}

		/* 
		 * Switch to Normal mode
		 */
		dummyLG1.flags = 0x1;
		dummyLG1.State = NORMAL;

		/*
		 * These are regular commit
		 * (no aggregation, no special case)
		 */
		Arequest.pHdlEntry = CommitArray[1];
		ret = RplCCMemCommitted(p_lgp, &Arequest);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Error committing request 1 \n"));
			leave;
		}

		Arequest.pHdlEntry = CommitArray[0];
		ret = RplCCMemCommitted(p_lgp, &Arequest);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("Error committing request 2 \n"));
			leave;
		}

		RplCCDump();

		/*
		 * Get 'regular' data from cache 
		 * the size is the CHUNKSIZE but we MUST
		 * allocate the MAXIMUM IO size !
		 */

		Arequest.Size      = 500;
		Arequest.pBuffer   = OS_ExAllocatePoolWithTag(PagedPool,
			                                           (3*Arequest.Size)+1, 'UnTs');
		ASSERT(Arequest.pBuffer);
		ret = RplCCGetNextSegment(p_lgp,  
			  				      RPLCC_WAITDATA, 
							      &Arequest);

		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("not enough buffer data %d \n",  Arequest.Size));
		}

		/*
		 * Free this segment
		 */
		Arequest.Size      = 500;
		ret = RplCCMemFree(p_lgp, &Arequest);
		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_FATAL, ("cannot free the request \n"));
			leave;
		}

		/*
		 * Check the data here
		 */
		OS_ExFreePool(Arequest.pBuffer);

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving main \n"));

return(ret);
}	/* UnitTest */

#if defined(_KERNEL)

/*
 * Open a system32 directory at boot time.
 */
OS_NTSTATUS
OpenBootDir(OUT HANDLE *pResultHdl);

int ourdebug = 1;
NTSTATUS 
DriverEntry(IN PDRIVER_OBJECT  DriverObject, 
            IN PUNICODE_STRING  RegistryPath 
            )
/**/
{
	OS_NTSTATUS        ret = STATUS_SUCCESS;
	HANDLE             BootHdl,
		               FileHandle;
	UNICODE_STRING     uns_name;
	OBJECT_ATTRIBUTES  obj_attrib;
	IO_STATUS_BLOCK	   IoStatusBlock;


#ifdef DBG
	__asm { int 3 };
#endif
#ifdef NOP
	if (ourdebug)
	{
		if (!(ret=OS_NT_SUCCESS(OpenBootDir(&BootHdl))))
		{
			MMGDEBUG(MMGDBG_LOW, ("OpenBootDir failed %x \n", ret));
			return(ret);
		}

		RtlInitUnicodeString(&uns_name, L"newbab.txt");
		/* Open relative to DirHandle */
		InitializeObjectAttributes(	&obj_attrib,
									&uns_name,
									OBJ_KERNEL_HANDLE,
									BootHdl,
									NULL         /* no ACL */
									);

		ret = ZwCreateFile(	&FileHandle,
							GENERIC_ALL,
							&obj_attrib,
							&IoStatusBlock,
							NULL,
							FILE_ATTRIBUTE_NORMAL,
							FILE_SHARE_READ|FILE_SHARE_WRITE,
							FILE_OPEN,
							0,
							NULL,
							0
							);

		if (!OS_NT_SUCCESS(ret))
		{
			MMGDEBUG(MMGDBG_LOW, ("ZwCreateFile failed %x \n", ret));
			return(ret);
		}
	}
	
	ZwClose(BootHdl);
	ZwClose(FileHandle);

#endif

	UnitTest();

return(STATUS_SUCCESS);
} /*DriverEntry*/

#else 
/*B**************************************************************************
 * main - 
 * 
 * Entry point for user space unit test.
 *E==========================================================================
 */

int 
main(int argc, char **argv)

/**/
{
	UnitTest();
}
#endif /*_KERNEL*/


/* EOF */