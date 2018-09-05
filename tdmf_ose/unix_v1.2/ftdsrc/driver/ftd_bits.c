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
 * ftd_bits.c - Bitmap manipulation routines 
 *
 * Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#include "ftd_kern_ctypes.h"
#if defined(linux)
#include "ftd_linux.h"
#endif
#include "ftd_bab.h"
#include "ftd_bits.h"
#include "ftd_def.h"
#include "ftdio.h"
#include "ftd_klog.h"
#include "ftd_kern_cproto.h"

/*-
 * ftd_set_bits()
 *
 * set bits between [x1, x2] (inclusive) in an array of 32-bit integers.
 * the bit ordering within the integer is controlled by the macros 
 * START_MASK and END_MASK. 
 *
 * we assume the caller has clamped x1 and x2 to the boundaries of the
 * array.
 */
FTD_PRIVATE ftd_void_t
ftd_set_bits (ftd_uint32_t * ptr, ftd_int32_t x1, ftd_int32_t x2)
{
  ftd_uint32_t *dest;
  ftd_uint32_t mask, word_left;
  ftd_int32_t n_words;

  /* WR35600
   * This is just a defensive code. We shall not have end bit(x2) smaller than
   * starting bit(x1).
   */
  if (x2 < x1)
    {
      return;
    }

  word_left = WORD_BOUNDARY (x1);
  dest = ptr + word_left;
  mask = START_MASK (x1);
  if ((n_words = (WORD_BOUNDARY (x2) - word_left)) == 0)
    {
      mask &= END_MASK (x2);
      *dest |= mask;
      return;
    }
      
  *dest++ |= mask;
  while (--n_words > 0)
    {
      *dest++ = 0xffffffff;
    }
  *dest |= END_MASK (x2);
  return;
}

/*-
 * ftd_update_hrdb()
 *
 * given a buffer, modify the high resolution dirty bitmap. 
 *
 * **NB**:  Caller is assumed to *NOT* be holding softp->lgp->lock.
 *
 */
FTD_PUBLIC ftd_int32_t
ftd_update_hrdb (ftd_dev_t * softp, struct buf * bp)
{
    ftd_int32_t startbit, endbit;
    ftd_context_t context;

    /* don't set bits on reads */
#if defined(ftd_debugf)
    ftd_debugf(write, "ftd_update_hrdb(%p, %p)\n", softp, bp);
#endif 
#if !defined(linux)
    if (bp->b_flags & B_READ)
        return (0);
#endif 
    startbit = bp->b_blkno >> softp->hrdb.shift;
    endbit = (bp->b_blkno + ((bp->b_bcount - 1) >> DEV_BSHIFT)) >> softp->hrdb.shift;


    ACQUIRE_LOCK (softp->lgp->lock, context);
    ftd_set_bits (softp->hrdb.map, startbit, endbit);
    
    if (softp->lgp->KeepHistoryBits || softp->lgp->DualBitmaps)
    {
       ftd_set_bits(softp->hrdb.mapnext, startbit, endbit);
    }
    RELEASE_LOCK (softp->lgp->lock, context);

    return (0);
}

/*-
 * ftd_update_lrdb()
 * 
 * given a buffer, see if the contents will modify the low resolution dirty
 * bitmap. if so, flush the bitmap out to disk.
 *
 * **NB**: Caller is assumed to be holding softp->lock.  This is *different*
 *		   from ftd_update_hrdb.
 */
FTD_PUBLIC ftd_int32_t
ftd_update_lrdb (ftd_dev_t * softp, struct buf * bp)
{
    ftd_int32_t startbit, endbit;
    ftd_int32_t bit, delta = 0, ret;

    /* don't set bits on reads */
#if defined(ftd_debugf)
    ftd_debugf(write, "ftd_update_lrdb(%p, %p)\n", softp, bp);
#endif 
#if !defined(linux)
    if (bp->b_flags & B_READ)
        return (delta);
#endif 

    startbit = bp->b_blkno >> softp->lrdb.shift;
    endbit = (bp->b_blkno + ((bp->b_bcount - 1) >> DEV_BSHIFT)) >> softp->lrdb.shift;

    for (bit = startbit; bit <= endbit; bit++) {
       // There once was a linux specific optimization here that relied on test_and_set_bit() but it was removed because:
       // 1-) The pointer type required for the address to start counting from varies from one architecture to another.
       // 2-) It wasn't clear that, for all platforms, the bit location can always be arbitrarily large.
       // 3-) It smelled like premature optimization that brought additional complexity, especially because of the previous two points.
       // 4-) According to the test_and_set_bit() documentation for the i386 and x86_64 platforms, combined with the way ftd_set_bits() used by
       //     ftd_compute_dirtybits() works, the bit ordering within a 32 bits word must be the same between test_and_set_bit() and the following code.
       if (TEST_BIT (softp->lrdb.map, bit) == 0) {
            SET_BIT (softp->lrdb.map, bit);
            delta = 1;
        }

        if (softp->lgp->KeepHistoryBits || softp->lgp->DualBitmaps)
        {
           if (TEST_BIT (softp->lrdb.mapnext, bit) == 0)
           {
              SET_BIT (softp->lrdb.mapnext, bit);
           }
        }
    }

    return (delta);
}

/*-
 * ftd_lg_get_device()
 * 
 * given a group and a device number, return the device state struct.
 */
FTD_PUBLIC ftd_dev_t *
ftd_lg_get_device (ftd_lg_t * lginfo, dev_t dev)
{
    ftd_dev_t *temp;

    for (temp = lginfo->devhead; temp != NULL; temp = temp->next) {
        if ((temp->cdev == dev) || (temp->bdev == dev) ||
            (temp->localbdisk == dev) || (temp->localcdisk == dev)) {
            break;
        }
    }
    return (temp);
}

/*-
 * ftd_compute_dirtybits()
 * 
 * porpoise thru the writelog updating either the LRDB or HRDB of all
 * devices in the logical group.
 *
 *     type = FTD_HIGH_RES_DIRTYBITS => HRDB.
 *     type = FTD_LOW_RES_DIRTYBITS  => LRDB.
 */
FTD_PUBLIC void
ftd_compute_dirtybits (ftd_lg_t * lginfo, ftd_int32_t type)
{
    ftd_dev_t_t lastdev;
    ftd_int32_t startbit, endbit;
    ftd_uint64_t *temp;
    bab_buffer_t *buf;
    bab_mgr_t *mgr;
    ftd_dev_t *softp;
    ftd_bitmap_t *bm;
    wlheader_t *hp;
    ftd_int32_t len64, nlen64, i;

    mgr = lginfo->mgr;

    if (mgr->num_in_use == 0)
        return;

    BAB_MGR_FIRST_BLOCK (mgr, buf, temp);
    lastdev = (ftd_dev_t_t)-1;
    bm = NULL;
    while (temp != NULL) {
        hp = (wlheader_t *) temp;

        if (hp->majicnum != DATASTAR_MAJIC_NUM)
            break;

        /* get the bitmap */
        if (hp->dev != lastdev) {
            if ((softp = ftd_lg_get_device (lginfo, hp->dev)) != NULL) {
                bm = (type == FTD_LOW_RES_DIRTYBITS) ?  &softp->lrdb : &softp->hrdb;
                lastdev = hp->dev;
            } else {
                lastdev = (ftd_dev_t_t)-1;
                bm = NULL;
            }
        }
        if (bm) {
            /* offset is the bp->b_blkno, 
               length is ((bp->b_bcount-1) >> DEV_BSHIFT) */
            startbit = hp->offset >> bm->shift;
            endbit = (hp->offset + hp->length - 1) >> bm->shift;

            ftd_set_bits (bm->map, startbit, endbit);
            if (lginfo->KeepHistoryBits || lginfo->DualBitmaps)
            {
               /* as necessary, we update */
               ftd_set_bits(bm->mapnext, startbit, endbit); /* WR37202 update also bits in case of incoming BAB overflow or Server crash */
            }
        }

        /* go to the next block of data */
        /*BAB_MGR_NEXT_BLOCK (temp, hp, buf);*/
/* + */
        if (hp->span > 1) {
            len64 = buf->alloc - (temp + sizeof_64bit(wlheader_t));
            nlen64 = FTD_LEN_QUADS(hp) - len64;
            for(i = 2; i< (hp->span); i++) {
                buf = buf->next;
                nlen64 -= buf->alloc - buf->start;
            }

            if ((buf = buf->next) == NULL)
                break;
            temp = buf->start + nlen64;

            /* WR35600
             * Boundary check, the end of the wl may as well the end
             * of the buffer.
             */
            if (temp == buf->alloc) {
                if ((buf = buf->next) == NULL)
                    break;
                temp = buf->start;
            }
        } else {
            temp += BAB_WL_LEN(hp);
            if (temp == buf->alloc) {
                if ((buf = buf->next) == NULL)
                    break;
                temp = buf->free;
            }
        }  
/* - */    
    }
}
