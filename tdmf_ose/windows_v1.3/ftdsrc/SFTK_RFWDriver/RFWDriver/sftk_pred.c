//#include <stdlib.h>
//#include <memory.h>
#include <ntddk.h>
#include <sys/types.h>
#include "sftk_pred.h"

#define min(A, B) ((A<B)?(A):(B))


/* inefficient realloc implementation */
PVOID Myrealloc(PVOID old_ptr , int newsize, int oldsize)
{
	PVOID new_ptr = NULL;

	new_ptr = ExAllocatePool(PagedPool, newsize);
	if (new_ptr == NULL)
		return NULL;

	if (old_ptr) 
	{
		// We need to get the old size from the ptr.
		RtlCopyMemory(  new_ptr, 
			            old_ptr, 
						min(newsize, oldsize));

		ExFreePool(old_ptr);
	}

return(new_ptr);
}

int
pred_compress_init(pred_t *predp)
{

	memset(predp->InputGuessTable, 0, sizeof(predp->InputGuessTable));
	memset(predp->OutputGuessTable, 0, sizeof(predp->OutputGuessTable));
	predp->iHash = 0;
	predp->oHash = 0;

    return 0;
} /* pred_compress_init */

int
pred_compress(pred_t *predp)
{
	int i, bitmask;
	int old_targetlen;
    unsigned char *flagtarget, flags, *orgtarget, *savetarget;
    int olen;

    olen = 0;
    orgtarget = predp->target;
	old_targetlen = predp->targetlen;

    while (predp->srclen) {
        if ((++olen) > predp->targetlen) {
            /* -- keep output buffer from overflowing */
            predp->targetlen = predp->targetlen + (predp->targetlen >> 1) + 1;
            savetarget = predp->target;
            predp->target = (unsigned char*)Myrealloc(predp->target, predp->targetlen, old_targetlen);
            predp->target = (unsigned char*)predp->target + (savetarget-orgtarget);
            orgtarget = (unsigned char*)predp->target;
        }
        flagtarget = predp->target++;
        flags = 0;			/* All guess wrong initially */

        for (bitmask = 1, i = 0; i < 8 && predp->srclen; i++, bitmask <<= 1) {
            if (predp->OutputGuessTable[predp->oHash] == *predp->src) {
                flags |= bitmask;	/* Guess was right - don't output */
            } else {
                predp->OutputGuessTable[predp->oHash] = *predp->src;
                if ((++olen) > predp->targetlen) {
                    /* -- keep output buffer from overflowing */
                    predp->targetlen = predp->targetlen + (predp->targetlen >> 1) + 1;
                    savetarget = predp->target;
                    predp->target = (unsigned char*)Myrealloc(predp->target, predp->targetlen, old_targetlen);
                    predp->target = (unsigned char*)predp->target + (savetarget-orgtarget);
					orgtarget = (unsigned char*)predp->target;
                }
                *predp->target++ = *predp->src;	/* Guess wrong, output char */
            }
            OHASH(predp->oHash, *predp->src++);
            predp->srclen--;
        }
        *flagtarget = flags;
    }

    return (predp->target - orgtarget);
} /* pred_compress */

int
pred_decompress(pred_t *predp)
{
	int i, bitmask;
    unsigned char flags, *orgtarget, *orgsrc;

	orgsrc = predp->src;
    orgtarget = predp->target;

    while (predp->srclen) {
        flags = *predp->src++;
        predp->srclen--;

        for (i = 0, bitmask = 1; i < 8; i++, bitmask <<= 1) {
            if (flags & bitmask) {
				/* Guess correct */
                *predp->target = predp->InputGuessTable[predp->iHash];
            } else {
                if (predp->srclen == 0)
                    break;		/* we seem to be really done -- cabo */
				/* Guess wrong */
                predp->InputGuessTable[predp->iHash] = *predp->src;
				/* Read from source */
                *predp->target = *predp->src++;
                predp->srclen--;
            }
            IHASH(predp->iHash, *predp->target++);
        }
    }

    return (predp->target - orgtarget);
} /* pred_decompress */

