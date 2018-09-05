/*HDR************************************************************************
 *                                                                         
 * Softek - Fujitsu                                    
 *
 *===========================================================================
 *
 * N mmrg_ntkrnl.c
 * P Replicator 
 * S functions specific to the nt kernel Apis.
 * V Windows NT 4.0/W2K/W2K3
 * A J. Christatos - jchristatos@softek.fujitsu.com
 * D 02.25.2004
 * O functions specific to the nt kernel Apis.
 * T Cache design requirements and design specifications - v 1.0.0.
 * C DBG - _KERNEL_
 * H 02.25.2004 - Creation - 
 *
 *===========================================================================
 *
 * rcsid[]="@(#) $Id: mmgr_ntkrnl.c,v 1.1 2004/05/29 00:45:28 vqba0 Exp $"
 *
 *HDR************************************************************************/


#include <ntddk.h>

#include "common.h"
#include "mmgr_ntkrnl.h"

/*B**************************************************************************
 * RplKrnlAllocateSection - 
 * 
 * Allocate a creation for the huge memory pool
 *E==========================================================================
 */
BOOLEAN
RplKrnlAllocateSection(IN OS_MEMSECTION  *memSection, IN LARGE_INTEGER size)

/**/
{
	BOOLEAN ret   = FALSE;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplKrnlAllocateSection \n"));

	try {

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplKrnlAllocateSection \n"));

return(ret);
} /* RplKrnlAllocateSection */

/*B**************************************************************************
 * RplKrnlFreeSection - 
 * 
 * Delete a memory section
 *E==========================================================================
 */
NTSTATUS
RplKrnlFreeSection(IN OS_MEMSECTION  *memSection)

/**/
{
	OS_NTSTATUS ret   = STATUS_SUCCESS;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplKrnlFreeSection \n"));

	try {

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplKrnlFreeSection \n"));

return(ret);
} /* RplKrnlFreeSection */

/*B**************************************************************************
 * RplKrnlIsValidSection - 
 * 
 * Check intergrity of mapped section
 *E==========================================================================
 */
BOOLEAN
RplKrnlIsValidSection(IN OS_MEMSECTION  *memSection)

/**/
{
	BOOLEAN ret   = FALSE;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplKrnlIsValidSection \n"));

	try {

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplKrnlIsValidSection \n"));

return(ret);
} /* RplKrnlIsValidSection */

/* EOF */
