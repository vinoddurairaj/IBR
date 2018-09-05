/*HDR************************************************************************
 *                                                                         
 * Softek - Fujitsu                                    
 *
 *===========================================================================
 *
 * N comon.h
 * P Replicator 
 * S Common definitions
 * V Generic 
 * A J. Christatos - jchristatos@softek.fujitsu.com
 * D 02.25.2004
 * O common definitions to ALL modes
 * T Cache requirements and design specifications - v 1.0.0.
 * C DBG - _KERNEL_ 
 * H 02.25.2004 - Creation - 
 *
 *===========================================================================
 *
 * rcsid[]="@(#) $Id: common.h,v 1.4 2004/06/17 22:03:27 jcq40 Exp $"
 *
 *HDR************************************************************************/

#ifndef _MMG_COMMON_
#define _MMG_COMMON_

/*
 * Debug levels for MmgrDebugLevel
 */

#define MMGDBG_LOW		(1)
#define MMGDBG_INFO		(2)
#define MMGDBG_VERBOSE	(3)
#define MMGDBG_HIGH		(4)
#define MMGDBG_FATAL    (5)

#if !defined(_KERNEL)
#if !defined(DBG) && !defined(_DEBUG)
#define DBGSTATIC static
#undef  ASSERTMSG
#define ASSERTMSG(msg,exp) 
#define ASSERT
#define MMGDEBUG(LEVEL, STRING)
#define CHECK_IRQL(msg, irql)
#endif /* DBG */
#endif

/* scope */

#define MMG_PUBLIC
#define MMG_PRIVATE static

/*
 * All data structures are tagged for debugging.
 */
typedef enum
{
	RPLCC_SLAB   = 0xcafecaf1,
	RPLCC_MIGA   = 0xcafecaf2,
	RPLCC_LG     = 0xcafecaf3,
	RPLCC_DTB    = 0xcafecaf4,
	RPLCC_DBROOT = 0xcafecaf5,
	RPLCC_POOL   = 0xcafecaf6,
	RPLCC_HDR    = 0xcafecaf7,
	RPLCC_CC     = 0xcafecaf8,
	RPLCC_SLNODE = 0xcafecaf9,
	RPLCC_SLHDR  = 0xcafecafa

} signatures_e;

/*
 * defines the type of sentinels we can support
 * in the cache.
 */
enum sentinals
{
	MSGNONE = 1,
	MSGINCO,
	MSGCO,
	MSGCPON,
	MSGCPOFF,
	MSGAVOIDJOURNALS
};

typedef enum sentinals sentinels_e;


/* 
 * Type for a virtual address
 * this allow bitwise operation
 * on addresses (slab.c)
 */
typedef unsigned long long vaddr_t;

/*
 * Interlock values
 */
#define ATOMIC_TAKEN      (2)
#define ATOMIC_FREE       (3)

#endif _MMG_COMMON_

/*EOF*/