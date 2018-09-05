/*
 * comp.c - data compression interface
 * 
 * Copyright (c) 1999 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

#include "compress.h"

int compress
(BYTE *dst, size_t *dstlen, BYTE *src, size_t srclen, int type)
{

    switch(type) {
    case PRED:
        return pred_compress(dst, dstlen, src, srclen);
        break;
    case LZHL:
        return lzhl_compress(dst, src, srclen);
        break;
    default:
        break;
    }

} /* compress */

int decompress
(BYTE *dst, size_t *dstlen, BYTE *src, size_t *srclen, int type)
{
    int len;
    int rc;

    switch(type) {
    case PRED:
        return pred_decompress(dst, dstlen, src, srclen);
        break;
    case LZHL:
        len = *dstlen;
        rc = lzhl_decompress(dst, dstlen, src, srclen);
        if (rc == FALSE) {
            return -1;
        } else {
            return len;
        }
        break;
    default:
        break;
    }
} /* decompress */

int compress_reset(void) 
{

    if (pred_compress_reset() == -1) {
        return -1;
    }
    if (lzhl_compress_reset() == -1) {
        return -1;
    }

    return 0;
} /* compress_reset */

