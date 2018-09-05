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

#include "ftd_kern_ctypes.h"
#include <sys/types.h>

ftd_void_t *
ftd_memset (ftd_void_t * b, ftd_int32_t c, size_t len)
{
  ftd_uchar_t *temp = b;

  while (len--)
    *temp++ = (ftd_uchar_t) c;
  return b;
}
