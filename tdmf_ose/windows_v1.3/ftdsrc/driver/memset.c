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

#include <sys/types.h>
#include <stddef.h>

//
// FOR IN_FCT/OUT_FCT definitions
//
#include "ftd_def.h"

void *
ftd_memset(void *b, int c, size_t len)
{
    unsigned char *temp = b;
    IN_FCT(ftd_memset)

    while (len--)
    {
        *temp++ = (unsigned char)c;
    }
    
    OUT_FCT(ftd_memset)
    return b;
}
