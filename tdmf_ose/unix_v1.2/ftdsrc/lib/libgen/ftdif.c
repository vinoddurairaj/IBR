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
#include "common.h"

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
    sb.addr = (ftd_uint64ptr_t)(unsigned long) &lginfo;
    if (FTD_IOCTL_CALL(ctlfd, FTD_GET_GROUP_STATS, &sb) < 0)
        return 1;

    *dip = (ftd_dev_info_t *) malloc(sizeof(ftd_dev_info_t) * lginfo.ndevs);
#ifdef PURE
    memset(*dip, 0, sizeof(ftd_dev_info_t) * lginfo.ndevs);
#endif
    sb.addr = (ftd_uint64ptr_t)(unsigned long) *dip;
    if (FTD_IOCTL_CALL(ctlfd, FTD_GET_DEVICES_INFO, &sb) < 0)
        return 1;
    db->devs = (dev_t *) malloc(sizeof(dev_t) * lginfo.ndevs);
    len = 0;
    for (i = 0; i < lginfo.ndevs; i++) {
        db->devs[i] = (*dip)[i].cdev;
        len += (*dip)[i].hrdbsize32;
    }
    db->dbbuf = (ftd_uint64ptr_t)(unsigned long) malloc(len * sizeof(int));
	db->dblen32 = len;
    db->state_change = 1;
    if (FTD_IOCTL_CALL(fd, FTD_GET_HRDBS, &db) < 0)
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
    int save_errno = 0;
    int i;
    int rc;
    ftd_dev_t_t pass_lgnum = lgnum;
    group_t *group_ptr;

    for (i = 0; i < 10; i++) {
        rc = FTD_IOCTL_CALL(ctlfd, FTD_CLEAR_BAB, &pass_lgnum);
        if ((rc < 0 && errno != EAGAIN) || rc == 0) {
            break;
        }
        save_errno = errno;
        sleep(1);
    }
    // WI_338550 December 2017, implementing RPO / RTT
    // Clear RPO queue
    group_ptr = mysys->group;
    Chunk_Timestamp_Queue_Clear(group_ptr->pChunkTimeQueue);
    
    if (rc < 0) {
        reporterr(ERRFAC, M_FLUSHBAB, ERRCRIT, strerror(save_errno));
    }
    return rc;
} /* flush_bab */


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
    return FTD_IOCTL_CALL(ctlfd, FTD_SET_SYNC_DEPTH, pa);
}
/***********************************************************************
 * set_lrt_bits -- Sets the lrt bits
 ***********************************************************************/
int
set_lrt_bits (int ctlfd, int lgnum)
{
    int i;
    ftd_dev_t_t lg_num;
 
    lg_num = lgnum;
    i = FTD_IOCTL_CALL (ctlfd, FTD_SET_LRDB_BITS, &lg_num);
    return i;
}
/***********************************************************************
 * set_lrt_mode -- Sets lrt_mode only
 ***********************************************************************/
int
set_lrt_mode (int ctlfd, int lgnum)
{
    int i;
    ftd_dev_t_t lg_num;

    lg_num = lgnum;
    i = FTD_IOCTL_CALL (ctlfd, FTD_SET_LRDB_MODE, &lg_num);
    return i;
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
    return FTD_IOCTL_CALL(ctlfd, FTD_SET_SYNC_TIMEOUT, pa);
}
/*
 * get_bab_size -- Gets the actual BAB size in sectors
 */
int
get_bab_size(int ctlfd)
{
    ftd_babinfo_t babinfo;

    if (FTD_IOCTL_CALL(ctlfd, FTD_GET_BAB_SIZE, &babinfo) < 0) {
        return -1;
    }
    return (babinfo.actual/DEV_BSIZE);
}

