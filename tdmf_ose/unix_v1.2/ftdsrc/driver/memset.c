/********************************************************* {COPYRIGHT-TOP} ***
* IBM Confidential
* OCO Source Materials
* 6949-32F - Softek Replicator for Unix and 6949-32K - Softek TDMF (IP) for Unix
*
*
* (C) Copyright IBM Corp. 2006, 2011  All Rights Reserved.
* The source code for this program is not published or otherwise  
* divested of its trade secrets, irrespective of what has been 
* deposited with the U.S. Copyright Office.
********************************************************* {COPYRIGHT-END} **/
/*
 * memset for the kernel.  It doesn't seem to have this functionality
 * documented anywhere.
 *
 * in Ftd, b is always at least int aligned, and len & 3 == 0 if you
 * find you need to make this routine faster in the future.  Likely
 * no one will ever read this humble comment, because the bottle necks
 * will be elsewhere (locking comes to mind).
 *
 * Also, in ftd, c == 0xff always.  This could likely be optimized
 * heavily, but doesn't get called often enough to even bother.
 *
 */

#if defined(linux)
#include <linux/types.h>
#include "ftd_kern_ctypes.h"
#else /* !defined(linux) */
#include "ftd_kern_ctypes.h"
#include <sys/types.h>
#endif /* defined(linux) */

ftd_void_t *
ftd_memset (ftd_void_t * b, ftd_int32_t c, size_t len)
{
  ftd_uchar_t *temp = b;

  while (len--)
    *temp++ = (ftd_uchar_t) c;
  return b;
}
