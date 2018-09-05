/*HDR************************************************************************
 *                                                                         
 * Softek -                                     
 *
 *===========================================================================
 *
 * N ntifs.c
 * P Replicator 
 * S ifs kit specific
 * V NT kernel
 * A J. Christatos - jchristatos@softek.com
 * D 05.20.2004
 * O Parse directories at boot time
 * T Cache design requirements and design specifications - v 1.0.0.
 * C DBG - _KERNEL_
 * H 04.28.2004 - Creation - 
 *
 *===========================================================================
 *
 * rcsid[]="@(#) $Id: ntifs.c,v 1.1 2004/05/29 00:45:28 vqba0 Exp $"
 *
 *HDR************************************************************************/

#include <ntifs.h>
#include "common.h"
#include "mmgr_ntkrnl.h"

VOID
OS_FsRtlDissectName(IN  UNICODE_STRING  u1,
				    OUT PUNICODE_STRING p_u2,
				    OUT PUNICODE_STRING p_u3)

/**/
{
	FsRtlDissectName(u1, p_u2, p_u3);
} 
/*EOF*/