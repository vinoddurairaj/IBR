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
#include "ftd_bab.h"
#include "ftd_bits.h"
#include "ftd_def.h"
#include "ftdio.h"
#include "ftd_klog.h"

/*
 * Set bits between [x1, x2] (inclusive) in an array of 32-bit integers.
 * The bit ordering within the integer is controlled by the macros 
 * START_MASK and END_MASK. 
 *
 * We assume the caller has clamped x1 and x2 to the boundaries of the
 * array.
 */
void
ftd_set_bits(unsigned int *ptr, int x1, int x2)
{
    unsigned int *dest;
    unsigned int mask, word_left, n_words;

    IN_FCT(ftd_set_bits)

    word_left = WORD_BOUNDARY(x1);
    dest = ptr + word_left;
    mask = START_MASK(x1);
    if ((n_words = (WORD_BOUNDARY(x2) - word_left)) == 0) 
    {
        mask &= END_MASK(x2);
        *dest |= mask;
        OUT_FCT(ftd_set_bits)
        return;
    }
    *dest++ |= mask;
    while (--n_words > 0) 
    {
        *dest++ = 0xffffffff;
    }
    *dest |= END_MASK(x2);
    
    OUT_FCT(ftd_set_bits)
    return;
}

/*
 * Given a buffer, modify the high resolution dirty bitmap. 
 */
int
ftd_update_hrdb(ftd_dev_t *softp, PIRP Irp)
{
    PIO_STACK_LOCATION PtrCurrentStackLocation = IoGetCurrentIrpStackLocation(Irp);
    int b_blkno = (int) (PtrCurrentStackLocation->Parameters.Write.ByteOffset.QuadPart >> DEV_BSHIFT);
    int b_bcount = PtrCurrentStackLocation->Parameters.Write.Length;
    int startbit, endbit;

    IN_FCT(ftd_update_hrdb)

    startbit = b_blkno >> softp->hrdb.shift; 
    endbit = (b_blkno + ((b_bcount-1) >> DEV_BSHIFT)) >> softp->hrdb.shift;

    ftd_set_bits(softp->hrdb.map, startbit, endbit);

    OUT_FCT(ftd_update_hrdb)
    return 0;
}

/*
 * Given a buffer, see if the contents will modify the low resolution dirty
 * bitmap.
 */
int
ftd_check_lrdb(ftd_dev_t *softp, PIRP Irp)
{
    PIO_STACK_LOCATION PtrCurrentStackLocation = IoGetCurrentIrpStackLocation(Irp);
    int     b_blkno = (int) (PtrCurrentStackLocation->Parameters.Write.ByteOffset.QuadPart >> DEV_BSHIFT);
    int     b_bcount = PtrCurrentStackLocation->Parameters.Write.Length;
    int     startbit, endbit;
    int     bit, delta = 0;

    IN_FCT(ftd_check_lrdb)

    startbit = b_blkno >> softp->lrdb.shift; 
    endbit = (b_blkno + ((b_bcount-1) >> DEV_BSHIFT)) >> softp->lrdb.shift;

    for (bit = startbit; bit <= endbit; bit++)
    {
        if (TEST_BIT(softp->lrdb.map, bit) == 0) 
        {
            delta = 1;
        }
    }

    OUT_FCT(ftd_check_lrdb)
    return delta;
}

/*
 * Given a buffer, see if the contents will modify the low resolution dirty
 * bitmap. If so, flush the bitmap out to disk.
 */
int
ftd_update_lrdb(unsigned int *map, int shift, PIRP Irp)
{
    PIO_STACK_LOCATION PtrCurrentStackLocation = IoGetCurrentIrpStackLocation(Irp);
    int     b_blkno = (int) (PtrCurrentStackLocation->Parameters.Write.ByteOffset.QuadPart >> DEV_BSHIFT);
    int     b_bcount = PtrCurrentStackLocation->Parameters.Write.Length;
    int     startbit, endbit;
    int     bit, delta = 0;

    IN_FCT(ftd_update_lrdb)

    startbit = b_blkno >> shift; 
    endbit = (b_blkno + ((b_bcount-1) >> DEV_BSHIFT)) >> shift;

    for (bit = startbit; bit <= endbit; bit++)
    {
        if (TEST_BIT(map, bit) == 0) 
        {
            SET_BIT(map, bit);
            delta = 1;
        }
    }

    OUT_FCT(ftd_update_lrdb)
    return delta;
}

/*
 * 
 */
ftd_dev_t *
ftd_lg_get_device(ftd_lg_t *lginfo, dev_t dev)
{
    ftd_dev_t *temp;

    IN_FCT(ftd_lg_get_device)

    for (temp = lginfo->devhead; temp != NULL; temp = temp->next) 
    {
        if ((temp->cdev == dev) || (temp->bdev == dev) ||
            (temp->localbdisk == dev) || (temp->localcdisk == dev)) 
        {
            OUT_FCT(ftd_lg_get_device)
            return temp;
        }
    }
    OUT_FCT(ftd_lg_get_device)

    return NULL;
}

/*
 * Porpoise thru the writelog updating either the LRDB or HRDB of all
 * devices in the logical group.
 *
 *     type = FTD_HIGH_RES_DIRTYBITS => HRDB.
 *     type = FTD_LOW_RES_DIRTYBITS  => LRDB.
 */
int
ftd_compute_dirtybits(ftd_lg_t *lginfo, int type)
{
    unsigned int    lastdev;
    int             startbit, endbit;
    ftd_uint64_t     *temp;
    bab_buffer_t *buf;
    bab_mgr_t    *mgr;
    ftd_dev_t    *softp;
    ftd_bitmap_t *bm;
    wlheader_t   *hp;

    IN_FCT(ftd_compute_dirtybits)

    mgr = lginfo->mgr;

    if (mgr->num_in_use == 0) 
    {
        OUT_FCT(ftd_compute_dirtybits)
        return 0;
    }

    BAB_MGR_FIRST_BLOCK(mgr, buf, temp);
    lastdev = -1;
    bm = NULL;
    while (temp != NULL) 
    {
        if (mgr->flags & FTD_BAB_PHYSICAL) 
        {
            hp = ftd_bab_map_memory(temp, sizeof(wlheader_t));
        } 
        else 
        {
            hp = (wlheader_t *) temp;
        }

        if (hp->majicnum != DATASTAR_MAJIC_NUM) 
        {
            FTD_ERR(FTD_WRNLVL, "LG %d WriteLog header corrupt, (ftd_compute_dirtybits).", lginfo->dev & ~FTD_LGFLAG);

            if (mgr->flags & FTD_BAB_PHYSICAL)
            {
                ftd_bab_unmap_memory(hp, sizeof(wlheader_t));
            }
            break;
        }

        /* get the bitmap */
        if (hp->dev != lastdev) 
        {
            if ((softp = ftd_lg_get_device(lginfo, hp->dev)) != NULL) 
            {
                if (type == FTD_LOW_RES_DIRTYBITS) 
                {
                    bm = &softp->lrdb; 
                } 
                else 
                {
                    bm = &softp->hrdb; 
                }
                lastdev = hp->dev;
            } 
            else 
            {
                lastdev = -1;
                bm = NULL;
            }
        }
        if (bm) 
        {

            /* offset is the bp->b_blkno, 
               length is ((bp->b_bcount-1) >> DEV_BSHIFT) */
            startbit = hp->offset >> bm->shift; 
            endbit = (hp->offset + hp->length-1) >> bm->shift;

            ftd_set_bits(bm->map, startbit, endbit);
        }

        /* go to the next block of data */
        BAB_MGR_NEXT_BLOCK(temp, hp, buf);

        if (mgr->flags & FTD_BAB_PHYSICAL)
            ftd_bab_unmap_memory(hp, sizeof(wlheader_t));
    }

    OUT_FCT(ftd_compute_dirtybits)
    return 1;
}
