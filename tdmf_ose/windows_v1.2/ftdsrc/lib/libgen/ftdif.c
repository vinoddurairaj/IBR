/***************************************************************************
 * ftdif.c - Convenience routines to interface to the driver.
 *
 * (c) Copyright 1998 FullTime Software.
 *     All Rights Reserved.
 *
 * This module implements the functionality for FTD driver I/O
 *
 ***************************************************************************/

/* Please do not include more of the PMD in this file than just the error
 * handling stuff.  I'm trying to keep it as "pure" as possible -- wl
 */

#ifdef NEED_BIGINTS
#include "bigints.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>

#include "ftdio.h"
#include "ftdif.h"
#include "errors.h"
#include "aixcmn.h"

/****************************************************************************
 * get_dirtybits -- gets the hrdb's from the driver for this device.
 *                  YOU MUST FREE db->devs, db->dbbuf and *dip.
 ***************************************************************************/
int
get_dirtybits(int ctlfd, int fd, int lgnum, dirtybit_array_t *db, ftd_dev_info_t **dip)
{
    stat_buffer_t       sb;
    ftd_stat_t          lginfo;
    int                 len;
    int                 i;

#ifdef PURE
    memset(&lginfo, 0, sizeof(lginfo));
    memset(&sb, 0, sizeof(sb));
#endif
    sb.lg_num = lgnum;
    sb.addr = (caddr_t) &lginfo;
    if (ioctl(ctlfd, FTD_GET_GROUP_STATS, &sb) < 0)
        return 1;

    *dip = (ftd_dev_info_t *) malloc(sizeof(ftd_dev_info_t) * lginfo.ndevs);
#ifdef PURE
    memset(*dip, 0, sizeof(ftd_dev_info_t) * lginfo.ndevs);
#endif
    sb.addr = (caddr_t) *dip;
    if (ioctl(ctlfd, FTD_GET_DEVICES_INFO, &sb) < 0)
        return 1;
    db->devs = (dev_t *) malloc(sizeof(dev_t) * lginfo.ndevs);
    len = 0;
    for (i = 0; i < lginfo.ndevs; i++) {
        db->devs[i] = (*dip)[i].cdev;
        len += (*dip)[i].hrdbsize32;
    }
    db->dbbuf = (int *) malloc(len * sizeof(int));
    db->state_change = 1;
    if (ioctl(fd, FTD_GET_HRDBS, &db) < 0)
        return 1;

    return 0;
} /* get_dirtybits */

/****************************************************************************
 * flush_bab -- clears out the entries in the writelog.  Tries up to 10 
 *              times to make this happen if EAGAIN is being returned.
 *
 ***************************************************************************/
int
flush_bab(int ctlfd, int lgnum)
{
    int save_errno;
    int i;
    int rc;

    for (i = 0; i < 10; i++) {
        rc = ioctl(ctlfd, FTD_CLEAR_BAB, &lgnum);
        if ((rc < 0 && errno != EAGAIN) || rc == 0) {
            break;
        }
        save_errno = errno;
        sleep(1);
    }
    if (rc < 0) {
        reporterr(ERRFAC, M_FLUSHBAB, ERRCRIT, strerror(save_errno));
    }
    return rc;
} /* flush_bab */

/***********************************************************************
 * set_iodelay -- Sets the iodelay in the driver for this logical
 *                group.
 *
 ***********************************************************************/
int
set_iodelay(int ctlfd, int lgnum, int delay)
{
    ftd_param_t pa[1];
    
    pa->lgnum = lgnum;
    pa->value = delay;
    return ioctl(ctlfd, FTD_SET_IODELAY, pa);
}

/***********************************************************************
 * set_sync_depth -- Sets the sync depth for the logical group in question.
 *                   To turn off sync mode, set the depth to -1.
 *
 ***********************************************************************/
int
set_sync_depth(int ctlfd, int lgnum, int sync_depth)
{
    ftd_param_t pa[1];
    
    pa->lgnum = lgnum;
    pa->value = sync_depth;
    return ioctl(ctlfd, FTD_SET_SYNC_DEPTH, pa);
}
/***********************************************************************
 * set_sync_timeout -- Sets the sync timeout for the logical group.
 *                     Zero indicates no timeout.
 *
 ***********************************************************************/
int
set_sync_timeout(int ctlfd, int lgnum, int sync_timeout)
{
    ftd_param_t pa[1];
    
    pa->lgnum = lgnum;
    pa->value = sync_timeout;
    return ioctl(ctlfd, FTD_SET_SYNC_TIMEOUT, pa);
}
/*
 * get_bab_size -- Gets the actual BAB size in sectors
 */
int
get_bab_size(int ctlfd)
{
    int size;

    if (ioctl(ctlfd, FTD_GET_BAB_SIZE, &size) < 0) {
        return -1;
    }
    return (size/DEV_BSIZE);
}

