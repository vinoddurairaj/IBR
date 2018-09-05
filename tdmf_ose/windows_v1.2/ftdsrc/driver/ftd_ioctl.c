/*
 * ftd_ioctl.c - FullTime Data driver for Solaris IOCTLs
 *
 * Copyright (c) 1996, 1998 The FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 * $Id: ftd_ioctl.c,v 1.6 2004/02/20 23:38:55 szg00 Exp $
 *
 */
/* XXX MENTAL NOTE: remove unneeded #includes here -- wl */

/* ftd_ddi.h must be included after all the system stuff to be ddi compliant */
#include "ftd_ddi.h"
#include "ftd_def.h"
#include "ftd_nt.h"
#include "ftd_bab.h"
#include "ftd_bits.h"
#include "ftd_var.h"
#include "ftd_all.h"
#include "ftdio.h"
#include "ftd_klog.h"

extern void finish_biodone(PIRP Irp);
extern FTD_STATUS ftd_nt_start_lg_thread(ftd_lg_t *lgp);
extern FTD_STATUS ftd_nt_start_dev_thread(ftd_dev_t *softp);
extern void *ftd_memset(void *s, int c, size_t n);
extern void biodone_psdev(struct buf *);
extern int ftd_biodone(struct buf *);

FTD_PRIVATE FTD_STATUS ftd_lg_update_dirtybits(ftd_lg_t *lgp);


/*-
 * ftd_synctimo()
 */
extern void ftd_synctimo(struct wlheader_s  *);

//
// Global variable so we are able to change it in debuger on the fly
//
//
// Retry up to iRetryStatic * iDelay for the bab to clear...
//
static  int     iRetryStatic = 100*60*2; 

/*
 * Waste the bab.
 */
void
ftd_clear_bab_and_stats(ftd_lg_t *lgp, FTD_CONTEXT context)
{
    ftd_dev_t     *tempdev;
    int           committed, used;
    caddr_t       sleepaddr;
    int           sleeper;
    int           iDelay = 10; // ms delay time
    unsigned int  iRetry = iRetryStatic;

    //
    // Special case for the mutex
    //
    // We want to actually give back the mutex for a time!!
    //
    // The whole PRE_ACQUIRE_MUTEX was there not to 
    // re-acquire a mutex taken somewhere else
    // but in this special case,
    // we really want to release it!!
    //

    PRE_ACQUIRE_MUTEX_FORCE

    IN_FCT(ftd_clear_bab_and_stats)

    sleepaddr = (caddr_t) &sleeper;
    ftd_do_sync_done(lgp);
    lgp->mgr->flags |= FTD_BAB_DONT_USE;
    while (     ftd_bab_get_pending(lgp->mgr) 
            &&  iRetry                          ) 
    {
        RELEASE_MUTEX(lgp->mutex, context);
        /* Wait iDelay ms and try again */
        delay(iDelay);
        iRetry--;

#if _DEBUG
        if (!(iRetry%500))
        {
            //
            // Do something here... breakpoint!
            //
            DbgPrint("iRetry=%ld - still have a pending buffer\n",iRetry);
        }
#endif

        ACQUIRE_MUTEX(lgp->mutex, context);

    }

    if (!iRetry)
    {
        FTD_ERR (FTD_WRNLVL, "Clear BAB was unsuccessful because of a pending entry\n");
#if _DEBUG
        DbgPrint("CLEAR_BAB_STOPED_RETRYING!!!\n");
#endif
    }
#if _DEBUG
    else
    {
        DbgPrint("Clear BAB was successful!!! iRetry=%ld\n",iRetry);
    }
#endif

    lgp->mgr->flags &= ~FTD_BAB_DONT_USE;
    ftd_bab_free_memory(lgp->mgr, 0x7fffffff);

    lgp->wlentries = 0;
    lgp->wlsectors = 0;
    
    for (tempdev = lgp->devhead; tempdev; tempdev = tempdev->next) 
    {
        tempdev->wlentries = 0;
        tempdev->wlsectors = 0;
    }

    OUT_FCT(ftd_clear_bab_and_stats)
}

/* 
 * Get the index of the most significant "set" bit in an integer. Most 
 * processors have a single instruction that does this, but "C" doesn't 
 * expose it. Note: there are faster ways of doing this, but wtf.
 */
FTD_PRIVATE FTD_STATUS
ftd_get_msb(int num)
{
    int i;
    unsigned int temp = (unsigned int)num;

    IN_FCT(ftd_get_msb)

    for (i = 0;i < 32;i++, temp >>= 1) 
    {
        if (temp == 0) break;
    }

    OUT_FCT(ftd_get_msb)
    return (i-1);
}

FTD_PRIVATE FTD_STATUS
ftd_ctl_get_config(dev_t dev, int cmd, int arg, int flag)
{
    return ENOTTY;
}

/*
 *
 */
FTD_PRIVATE FTD_STATUS
ftd_ctl_get_num_devices(int arg, int flag)
{
    int       num;
    minor_t   minor_num;
    ftd_lg_t  *lgp;
    ftd_dev_t *devp;

    IN_FCT(ftd_ctl_get_num_devices)

    num = *(int *)arg;

    minor_num = getminor(num); 
    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, minor_num);
    if (lgp == NULL) 
    {
        OUT_FCT(ftd_ctl_get_num_devices)
        return ENXIO;
    }
    num = 0;
    devp = lgp->devhead;
    while (devp) 
    {
        devp = devp->next;
        num++;
    }

    *((int *)arg) = num;

    OUT_FCT(ftd_ctl_get_num_devices)
    return FTD_SUCCESS;;
}

/*
 * Return the number of groups 
 */
FTD_PRIVATE FTD_STATUS
ftd_ctl_get_num_groups(int arg, int flag)
{
    ftd_ctl_t     *ctlp;
    struct ftd_lg *temp;
    int           num;

    IN_FCT(ftd_ctl_get_num_groups)

    if ((ctlp = ftd_global_state) == NULL)
    {
        OUT_FCT(ftd_ctl_get_num_groups)
        return (ENXIO);
    }
    num = 0;
    temp = ctlp->lghead;
    while (temp) 
    {
        temp = temp->next;
        num++;
    }

    *((int *)arg) = num;

    OUT_FCT(ftd_ctl_get_num_groups)
    return FTD_SUCCESS;;
}

/*
 *
 */
FTD_PRIVATE FTD_STATUS
ftd_ctl_get_devices_info(int arg, int flag)
{
    ftd_dev_t      *devp;
    ftd_lg_t       *lgp;
    ftd_dev_info_t *info, ditemp;
    stat_buffer_t  sb;
    minor_t        minor_num;

    IN_FCT(ftd_ctl_get_devices_info)

    sb = *(stat_buffer_t *)arg;

    minor_num = getminor(sb.lg_num); 
    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, minor_num);
    if (lgp == NULL) 
    {
        OUT_FCT(ftd_ctl_get_devices_info)
        return ENXIO;
    }

    /* XXX FIXME: Need to check the sb.length to make sure that we're OK */

    info = (ftd_dev_info_t *)sb.addr;
    devp = lgp->devhead;
    while (devp) 
    {
        ditemp.lgnum = devp->lgp->dev; 
        ditemp.localcdev = devp->localcdisk;
        ditemp.cdev = devp->cdev;
        ditemp.bdev = devp->bdev;
        ditemp.disksize = devp->disksize;
        ditemp.lrdbsize32 = devp->lrdb.len32;
        ditemp.hrdbsize32 = devp->hrdb.len32;
        ditemp.lrdb_res = devp->lrdb.bitsize;
        ditemp.hrdb_res = devp->hrdb.bitsize;
        ditemp.lrdb_numbits = devp->lrdb.numbits;
        ditemp.hrdb_numbits = devp->hrdb.numbits;
        ditemp.statsize = devp->statsize;
        if (ddi_copyout((caddr_t)&ditemp, (caddr_t)info, sizeof(ditemp), flag))
        {
            OUT_FCT(ftd_ctl_get_devices_info)
            return EFAULT;
        }
        info++;
        devp = devp->next;
    }

    OUT_FCT(ftd_ctl_get_devices_info)
    return FTD_SUCCESS;
}

/*
 *
 */
FTD_PRIVATE FTD_STATUS
ftd_ctl_get_groups_info(int arg, int flag)
{
    minor_t       minor_num;
    ftd_lg_t      *lgp;
    ftd_ctl_t     *ctlp;
    ftd_lg_info_t *info, lgtemp;
    stat_buffer_t sb;

    IN_FCT(ftd_ctl_get_groups_info)

    sb = *(stat_buffer_t *)arg;

    if ((ctlp = ftd_global_state) == NULL)
    {
        OUT_FCT(ftd_ctl_get_groups_info)
        return (ENXIO);
    }

    /* XXX FIXME: Need to check the sb.length to make sure that we're OK */
    /* But only if lg_num == FTD_CTL */

    info = (ftd_lg_info_t *)sb.addr;
    lgp = ctlp->lghead;
    while (lgp) 
    {
        if (sb.lg_num == FTD_CTL) 
        {
            lgtemp.lgdev = lgp->dev; 
            lgtemp.statsize = lgp->statsize;
            if (ddi_copyout((caddr_t)&lgtemp, (caddr_t)info, sizeof(lgtemp),
                        flag))
            {
                OUT_FCT(ftd_ctl_get_groups_info)
                return EFAULT;
            }
            info++;
        } 
        else 
        {
            minor_num = getminor(lgp->dev) & ~FTD_LGFLAG;
            if (sb.lg_num == minor_num) 
            {
                lgtemp.lgdev = lgp->dev; /* FIXME: minor_num? */
                lgtemp.statsize = lgp->statsize;
                if (ddi_copyout((caddr_t)&lgtemp, (caddr_t)info, 
                            sizeof(lgtemp), flag))
                {
                    OUT_FCT(ftd_ctl_get_groups_info)
                    return EFAULT;
                }
                break;
            }
        }
        lgp = lgp->next;
    }

    OUT_FCT(ftd_ctl_get_groups_info)
    return FTD_SUCCESS;;
}

/*
 *
 */
FTD_PRIVATE FTD_STATUS
ftd_ctl_get_device_stats(int arg, int flag)
{
    disk_stats_t    temp, *stats, *statsend;
    stat_buffer_t   sb;
    ftd_lg_t        *lgp = NULL;
    ftd_dev_t       *devp = NULL;
    minor_t         minor_num;
    int             do_all = 0;

    IN_FCT(ftd_ctl_get_device_stats)
    sb = *(stat_buffer_t *)arg;

    /* XXX FIXME: Need to check the sb.length to make sure that we're OK */
    /* get the device state */
    minor_num = getminor(sb.lg_num);
    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, minor_num);
    if (!lgp)
    {
        OUT_FCT(ftd_ctl_get_device_stats)
        return EINVAL;
    }

    /* get the device state */
    minor_num = getminor(sb.dev_num);
    if (minor_num != L_MAXMIN) 
    {
        devp = (ftd_dev_t *) ddi_get_soft_state(ftd_dev_state, minor_num);
        if (!devp)
        {
            OUT_FCT(ftd_ctl_get_device_stats)
            return EINVAL;
        }
    } 
    else 
    {
        devp = lgp->devhead;
        do_all= 1;
    }

    stats = (disk_stats_t *)sb.addr;
    statsend = stats + sb.len;

    while (devp && stats < statsend) 
    {
#if defined(_WINDOWS)
        strcpy(temp.devname, devp->devname);
#endif
        temp.localbdisk = devp->localbdisk;
        temp.localdisksize = devp->disksize;

        temp.readiocnt = devp->readiocnt;
        temp.writeiocnt = devp->writeiocnt;
        temp.sectorsread = devp->sectorsread;
        temp.sectorswritten = devp->sectorswritten;
        temp.wlentries = devp->wlentries;
        temp.wlsectors = devp->wlsectors;

        if (ddi_copyout((caddr_t)&temp, (caddr_t)stats, sizeof(temp), flag))
        {
            OUT_FCT(ftd_ctl_get_device_stats)
            return EFAULT;
        }
        if ( !do_all )
            break;
        stats++;
        devp = devp->next;
    }

    OUT_FCT(ftd_ctl_get_device_stats)
    return FTD_SUCCESS;;
}

/*
 *
 */
FTD_PRIVATE FTD_STATUS
ftd_ctl_get_group_stats(int arg, int flag)
{
    ftd_ctl_t       *ctlp;
    ftd_lg_t        *lgp;
    ftd_stat_t      temp, *stats;
    stat_buffer_t   sb;
    minor_t         minor_num;
    FTD_CONTEXT     context;
    int             do_all = 0;
    PRE_ACQUIRE_MUTEX

    IN_FCT(ftd_ctl_get_group_stats)

    if ((ctlp = ftd_global_state) == NULL)
    {
        OUT_FCT(ftd_ctl_get_group_stats)
        return (ENXIO);
    }

    sb = *(stat_buffer_t *)arg;

    minor_num = getminor(sb.lg_num); 
    if (minor_num != L_MAXMIN) 
    {
        minor_num = minor_num & ~FTD_LGFLAG;
        lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, minor_num);
        if (lgp == NULL) 
        {
            OUT_FCT(ftd_ctl_get_group_stats)
            return ENXIO;
        }
    } 
    else 
    {
        lgp = ctlp->lghead;
        do_all= 1;
    }

    stats = (ftd_stat_t *)sb.addr;
    while (lgp) 
    {
        ACQUIRE_MUTEX(lgp->mutex, context);
        /* XXX FIXME: Need to check the sb.length to make sure that we're OK */

        temp.lgnum = lgp->dev & ~FTD_LGFLAG;
        temp.wlentries = lgp->wlentries;
        temp.wlsectors = lgp->wlsectors;
        temp.bab_free = ftd_bab_get_free_length(lgp->mgr) * sizeof(ftd_uint64_t);
        temp.bab_used = ftd_bab_get_used_length(lgp->mgr) * sizeof(ftd_uint64_t);
        temp.state = lgp->state;
        temp.ndevs = lgp->ndevs;
        temp.sync_depth = lgp->sync_depth;
        temp.sync_timeout = lgp->sync_timeout;
        temp.iodelay = lgp->iodelay;

        RELEASE_MUTEX(lgp->mutex, context);

        if (ddi_copyout((caddr_t)&temp, (caddr_t)stats, sizeof(temp), flag))
        {
            OUT_FCT(ftd_ctl_get_group_stats)
            return EFAULT;
        }

        if ( !do_all )
            break;
        stats++;
        lgp = lgp->next;
    }

    OUT_FCT(ftd_ctl_get_group_stats)
    return FTD_SUCCESS;;
}

/*
 * induce a driver state transition.
 */
FTD_PRIVATE FTD_STATUS
ftd_ctl_set_group_state(int arg, int flag)
{
    FTD_CONTEXT         context;
    minor_t       minor_num;
    ftd_lg_t      *lgp;
    ftd_state_t   sb;
    ftd_dev_t     *tempdev;
    PRE_ACQUIRE_MUTEX

    IN_FCT(ftd_ctl_set_group_state)

    sb = *(ftd_state_t *)arg;

    minor_num = getminor(sb.lg_num) & ~FTD_LGFLAG; 
    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, minor_num);
    if (lgp == NULL) 
    {
        OUT_FCT(ftd_ctl_set_group_state)
        return ENXIO;
    }
    ACQUIRE_MUTEX(lgp->mutex, context);

    if (lgp->state == sb.state) 
    {
        RELEASE_MUTEX(lgp->mutex, context);
        OUT_FCT(ftd_ctl_set_group_state)
        return FTD_SUCCESS;;
    }

    switch (lgp->state) 
    {
    case FTD_MODE_NORMAL:
        if ((sb.state == FTD_MODE_REFRESH) ||
            (sb.state == FTD_MODE_TRACKING)) 
        {
            /* clear all dirty bits */
            ftd_clear_dirtybits(lgp);
        } 
        else if (sb.state == FTD_MODE_PASSTHRU) 
        {
            ftd_set_dirtybits(lgp);
            ftd_clear_bab_and_stats(lgp, context);
        } 
        else 
        {
            /* -> Transition to backfresh */
        }
        break;
    case FTD_MODE_REFRESH:
        if (sb.state == FTD_MODE_NORMAL) 
        {
            ftd_clear_dirtybits(lgp);
        } 
        else if (sb.state == FTD_MODE_TRACKING) 
        {
            /* clear the journal */
            ftd_clear_bab_and_stats(lgp, context);
        } 
        else if (sb.state == FTD_MODE_PASSTHRU) 
        {
            ftd_set_dirtybits(lgp);
            ftd_clear_bab_and_stats(lgp, context);
        } 
        else 
        {
            /* -> Transition to backfresh */
        }
        break;
    case FTD_MODE_TRACKING: 
        if (sb.state == FTD_MODE_NORMAL) 
        {
            ftd_clear_dirtybits(lgp);
            /* update low res dirty bits with journal */
            ftd_compute_dirtybits(lgp, FTD_LOW_RES_DIRTYBITS);
        } 
        else if (sb.state == FTD_MODE_PASSTHRU) 
        {
            ftd_set_dirtybits(lgp);
            ftd_clear_bab_and_stats(lgp, context);
        } 
        else if (sb.state != FTD_MODE_REFRESH) 
        {
            /* -> Transition to backfresh */
        }
        break;
    case FTD_MODE_PASSTHRU: 
        /* FIXME: we should probably do something here */
        if (sb.state == FTD_MODE_REFRESH) 
        {
            ftd_set_dirtybits(lgp);
        } 
        else if ((sb.state == FTD_MODE_NORMAL) ||
                   (sb.state == FTD_MODE_TRACKING)) 
        {
/*
            ftd_clear_dirtybits(lgp);
*/
            ftd_clear_bab_and_stats(lgp, context);
        } 
        else 
        {
            /* -> Transition to backfresh */
        }
        break;
    default:
        /* -> Transition to backfresh */
        break;
    }

    lgp->state = sb.state;
    /* Need to somehow flush the state to disk here, no? XXX FIXME XXX */


    /* flush the low res dirty bits */
    tempdev = lgp->devhead;
    while(tempdev) 
    {
        ftd_flush_lrdb(tempdev, NULL);
        tempdev = tempdev->next;
    }

    RELEASE_MUTEX(lgp->mutex, context);

    ftd_wakeup(lgp);

    OUT_FCT(ftd_ctl_set_group_state)
    return FTD_SUCCESS;;
}

/*
 * get the actual size of the bab
 */
FTD_PRIVATE FTD_STATUS
ftd_ctl_get_bab_size(int arg)
{
    ftd_ctl_t     *ctlp;

    IN_FCT(ftd_ctl_get_bab_size)

    if ((ctlp = ftd_global_state) == NULL)
    {
        OUT_FCT(ftd_ctl_get_bab_size)
        return (ENXIO);
    }

    *((int *)arg) = ctlp->bab_size;

    OUT_FCT(ftd_ctl_get_bab_size)
    return FTD_SUCCESS;;
}

/*
 * update the lrdbs of a group
 */
FTD_PRIVATE FTD_STATUS
ftd_ctl_update_lrdbs(int arg)
{
    dev_t     lg_num;
    minor_t   minor_num;
    ftd_lg_t  *lgp;

    IN_FCT(ftd_ctl_update_lrdbs)

    lg_num = *(dev_t *)arg;

    minor_num = getminor(lg_num) & ~FTD_LGFLAG; 
    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, minor_num);

    if (lgp == NULL) 
    {
        OUT_FCT(ftd_ctl_update_lrdbs)
        return ENXIO;
    }

    ftd_compute_dirtybits(lgp, FTD_LOW_RES_DIRTYBITS);

    OUT_FCT(ftd_ctl_update_lrdbs)
    return FTD_SUCCESS;;
}

/*
 * update the hrdbs of a group
 */
FTD_PRIVATE FTD_STATUS
ftd_ctl_update_hrdbs(int arg)
{
    dev_t     lg_num;
    minor_t   minor_num;
    ftd_lg_t  *lgp;

    IN_FCT(ftd_ctl_update_hrdbs)

    lg_num = *(dev_t *)arg;

    minor_num = getminor(lg_num) & ~FTD_LGFLAG; 
    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, minor_num);

    if (lgp == NULL) 
    {
        OUT_FCT(ftd_ctl_update_hrdbs)
        return ENXIO;
    }

    ftd_compute_dirtybits(lgp, FTD_HIGH_RES_DIRTYBITS);

    OUT_FCT(ftd_ctl_update_hrdbs)
    return FTD_SUCCESS;;
}

/*
 * get the state of the group
 */
FTD_PRIVATE FTD_STATUS
ftd_ctl_get_group_state(int arg, int flag)
{
    dev_t    lg_num;
    minor_t  minor_num;
    ftd_lg_t *lgp;

    IN_FCT(ftd_ctl_get_group_state)

    lg_num = ((ftd_state_t *)arg)->lg_num;

    minor_num = getminor(lg_num) & ~FTD_LGFLAG; 
    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, minor_num);
    if (lgp == NULL) 
    {
        OUT_FCT(ftd_ctl_get_group_state)
        return ENXIO;
    }

    ((ftd_state_t *)arg)->state = lgp->state;

    OUT_FCT(ftd_ctl_get_group_state)
    return FTD_SUCCESS;;
}

/*
 * clear the bab for this group
 */
FTD_PRIVATE FTD_STATUS
ftd_ctl_clear_bab(int arg)
{
    minor_t       minor_num;
    ftd_lg_t      *lgp;
    dev_t         dev;
    int           context;
    PRE_ACQUIRE_MUTEX

    IN_FCT(ftd_ctl_clear_bab)

    dev = *(dev_t *)arg;

    minor_num = getminor(dev) & ~FTD_LGFLAG; 
    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, minor_num);
    if (lgp == NULL) 
    {
        OUT_FCT(ftd_ctl_clear_bab)
        return ENXIO;
    }
    ACQUIRE_MUTEX(lgp->mutex, context);
    ftd_clear_bab_and_stats(lgp, context);
    RELEASE_MUTEX(lgp->mutex, context);

    OUT_FCT(ftd_ctl_clear_bab)
    return FTD_SUCCESS;;
}

/*
 * clear the bab for this group
 */
FTD_PRIVATE FTD_STATUS
ftd_ctl_clear_dirtybits(int arg, int type)
{
    FTD_CONTEXT         context;
    minor_t       minor_num;
    ftd_lg_t      *lgp;
    ftd_dev_t     *tempdev;
    dev_t         dev;
    PRE_ACQUIRE_MUTEX

    IN_FCT(ftd_ctl_clear_dirtybits)

    dev = *(dev_t *)arg;

    minor_num = getminor(dev) & ~FTD_LGFLAG; 
    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, minor_num);
    if (lgp == NULL) 
    {
        OUT_FCT(ftd_ctl_clear_dirtybits)
        return ENXIO;
    }
    ACQUIRE_MUTEX(lgp->mutex, context);

    tempdev = lgp->devhead;
    while(tempdev) 
    {
        if (type == FTD_HIGH_RES_DIRTYBITS) 
        {
            bzero((caddr_t)tempdev->hrdb.map, tempdev->hrdb.len32*4);
        } 
        else 
        {
            bzero((caddr_t)tempdev->lrdb.map, tempdev->lrdb.len32*4);
        }
        ftd_flush_lrdb(tempdev, NULL);
        tempdev = tempdev->next;
    }

    RELEASE_MUTEX(lgp->mutex, context);

    OUT_FCT(ftd_ctl_clear_dirtybits)
    return FTD_SUCCESS;;
}

FTD_PRIVATE FTD_STATUS
ftd_ctl_new_device(dev_t dev, int cmd, int arg, int flag)
{
    ftd_dev_t           *softp;
    ftd_lg_t            *lgp;
    ftd_dev_info_t      *info;
    int                 diskbits, lrdbbits, hrdbbits, i;
    minor_t             dev_minor_num, lg_minor_num;
    struct buf          *buf;
    ftd_ctl_t           *ctlp;

    IN_FCT(ftd_ctl_new_device)

    /* the caller needs to give us: info about the device and the group device
       number */
    info = (ftd_dev_info_t *)arg;
    dev_minor_num = getminor(info->cdev); /* ??? */
    ctlp = ftd_global_state;

    softp = (ftd_dev_t *) ddi_get_soft_state(ftd_dev_state, dev_minor_num);
    if (softp != NULL) 
    {
        OUT_FCT(ftd_ctl_new_device)
        return EADDRINUSE;
    }

    if (ftd_nt_add_device(ctlp->DriverObject, info->devname, info->vdevname, &softp) != FTD_SUCCESS) 
    {
        OUT_FCT(ftd_ctl_new_device)
        return ENXIO;
    }

    if (ddi_soft_state_zalloc(ftd_dev_state, dev_minor_num, softp) != DDI_SUCCESS) 
    {
        ftd_nt_del_device(softp);
        OUT_FCT(ftd_ctl_new_device)
        return EADDRINUSE;
    }
    softp = (ftd_dev_t *) ddi_get_soft_state(ftd_dev_state, dev_minor_num);
    if (softp == NULL) 
    {
        ftd_nt_del_device(softp);
        OUT_FCT(ftd_ctl_new_device)
        return ENXIO;
    }

    lg_minor_num = info->lgnum;
    lgp = ddi_get_soft_state(ftd_lg_state, lg_minor_num);
    if (lgp == NULL) 
    {
        FTD_ERR(FTD_WRNLVL, "Can't get the soft state for logical group %d", 
            lg_minor_num);
        ftd_nt_del_device(softp);
        OUT_FCT(ftd_ctl_new_device)
        return ENXIO;
    }

    softp->lgp = lgp;

    softp->localcdisk = softp->cdev = info->cdev;
    softp->localbdisk = softp->bdev = info->bdev;
    
    softp->disksize = info->disksize;

    diskbits = ftd_get_msb(info->disksize) + 1 + DEV_BSHIFT;
    lrdbbits = ftd_get_msb(info->lrdbsize32 * 4);
    hrdbbits = ftd_get_msb(info->hrdbsize32 * 4);

#ifdef _PRINT_BITMAP_MEMORY_SIZE_
    DbgPrint("Creating New Device LogicalGroup = %ld\n",info->lgnum);  \
    DbgPrint("Device = %s size=%ld\n",info->devname,info->disksize);  
#endif

    softp->lrdb.bitsize = diskbits - lrdbbits;
    if (softp->lrdb.bitsize < MINIMUM_LRDB_BITSIZE)
        softp->lrdb.bitsize = MINIMUM_LRDB_BITSIZE;
    softp->lrdb.shift = softp->lrdb.bitsize - DEV_BSHIFT;
    softp->lrdb.len32 = info->lrdbsize32;
    softp->lrdb.numbits = (info->disksize + 
        ((1 << softp->lrdb.shift) - 1)) >> softp->lrdb.shift;
    softp->lrdb.map = (unsigned int *)kmem_zalloc(info->lrdbsize32 * 4, KM_SLEEP);

    if (softp->lrdb.map==NULL)
    {
        ddi_soft_state_free(ftd_dev_state, dev_minor_num);
        ftd_nt_del_device(softp);
        OUT_FCT(ftd_ctl_new_device)
        return ENXIO;
    }

#ifdef _PRINT_BITMAP_MEMORY_SIZE_
    DbgPrint("LowResolution BitMap size = %ld Bytes\n",(info->lrdbsize32*4));  
    DbgPrint("Bits = %ld\n",softp->lrdb.numbits);
    DbgPrint("BitSize = %ld bits = %ld\n",softp->lrdb.bitsize,(1<<softp->lrdb.bitsize));
#endif

    softp->hrdb.bitsize = diskbits - hrdbbits;
    if (softp->hrdb.bitsize < MINIMUM_HRDB_BITSIZE)
        softp->hrdb.bitsize = MINIMUM_HRDB_BITSIZE;
    softp->hrdb.shift = softp->hrdb.bitsize - DEV_BSHIFT;
    softp->hrdb.len32 = info->hrdbsize32;
    softp->hrdb.numbits = (info->disksize + 
        ((1 << softp->hrdb.shift) - 1)) >> softp->hrdb.shift;
    softp->hrdb.map = (unsigned int *)kmem_zalloc(info->hrdbsize32 * 4, KM_SLEEP);

    if (softp->hrdb.map==NULL)
    {
        kmem_free(softp->lrdb.map, info->lrdbsize32 * 4);
        ddi_soft_state_free(ftd_dev_state, dev_minor_num);
        ftd_nt_del_device(softp);
        OUT_FCT(ftd_ctl_new_device)
        return ENXIO;
    }

    softp->lrdb_offset = info->lrdb_offset; // sectors

    softp->statbuf = (char *)kmem_zalloc(info->statsize, KM_SLEEP);

    if (softp->statbuf==NULL)
    {
        kmem_free(softp->hrdb.map, info->hrdbsize32 * 4);
        kmem_free(softp->lrdb.map, info->lrdbsize32 * 4);
        ddi_soft_state_free(ftd_dev_state, dev_minor_num);
        ftd_nt_del_device(softp);
        OUT_FCT(ftd_ctl_new_device)
        return ENXIO;
    }

#ifdef _PRINT_BITMAP_MEMORY_SIZE_
    DbgPrint("HighResolution BitMap size = %ld Bytes\n",(info->hrdbsize32 * 4));  
    DbgPrint("NumBits = %ld\n",softp->hrdb.numbits);
    DbgPrint("BitSize = %ld bits = %ld\n",softp->hrdb.bitsize,(1<<softp->hrdb.bitsize));
#endif

    softp->statsize = info->statsize;

    ALLOC_LOCK(softp->lock, QNM " device specific lock");

    if ( !NT_SUCCESS(ftd_nt_start_dev_thread(softp)) ) 
    {
            kmem_free(softp->statbuf, info->statsize);
            kmem_free(softp->hrdb.map, info->hrdbsize32 * 4);
            kmem_free(softp->lrdb.map, info->lrdbsize32 * 4);
        ddi_soft_state_free(ftd_dev_state, dev_minor_num);
        ftd_nt_del_device(softp);
        OUT_FCT(ftd_ctl_new_device)
        return ENXIO;
    }

    /* add device to linked list */
    softp->next = lgp->devhead;
    lgp->devhead = softp;
    lgp->ndevs++;

    OUT_FCT(ftd_ctl_new_device)
    return FTD_SUCCESS;;
}

FTD_PRIVATE FTD_STATUS
ftd_ctl_new_lg(dev_t dev, int cmd, int arg, int flag)
{
    ftd_lg_info_t *info;
    minor_t       lg_minor_num;
    ftd_ctl_t     *ctlp;
    ftd_lg_t      *lgp;

    IN_FCT(ftd_ctl_new_lg)

    info = (ftd_lg_info_t *)arg;

    if ((ctlp = ftd_global_state) == NULL)
    {
        OUT_FCT(ftd_ctl_new_lg)
        return (ENXIO);
    }

    lg_minor_num = getminor(info->lgdev) & ~FTD_LGFLAG; 

    // We need to check if it exists already, if so say INUSE to user
    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, lg_minor_num);
    if (lgp != NULL) 
    {
        OUT_FCT(ftd_ctl_new_lg)
        return EADDRINUSE;
    }

    // Nope so create a NT one
    if (ftd_nt_add_lg(ctlp->DriverObject, lg_minor_num, info->vdevname, &lgp) != FTD_SUCCESS) 
    {
        OUT_FCT(ftd_ctl_new_lg)
        return ENXIO;
    }

    // This should not fail, cause we check to see if it exists already
    if (ddi_soft_state_zalloc(ftd_lg_state, lg_minor_num, lgp) != DDI_SUCCESS) 
    {
        OUT_FCT(ftd_ctl_new_lg)
        return EADDRINUSE;
    }

    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, lg_minor_num);
    if (lgp == NULL) 
    {
#if defined(FTD_DEBUG)
        FTD_ERR(FTD_WRNLVL, "ftd_ctl_new_lg: ddi_get_soft_state(%08x,%08x)",
                ftd_lg_state, lg_minor_num);
#endif /* defined(FTD_DEBUG) */
        OUT_FCT(ftd_ctl_new_lg)
        return(ENOTTY);
    }
 
    /* allocate a bab manager */
    if ((lgp->mgr = ftd_bab_alloc_mgr(ctlp->maxmem)) == NULL) 
    {
        ddi_soft_state_free(ftd_lg_state, lg_minor_num);
        OUT_FCT(ftd_ctl_new_lg)
        return(ENXIO);
    }

    lgp->statbuf = (char *)kmem_zalloc(info->statsize, KM_SLEEP);
    if (lgp->statbuf == NULL) 
    {
        ftd_bab_free_mgr(lgp->mgr);
        ddi_soft_state_free(ftd_lg_state, lg_minor_num);
        OUT_FCT(ftd_ctl_new_lg)
        return(ENXIO);
    }

    ALLOC_LOCK(lgp->lock, "ftd lg lock");
    ALLOC_MUTEX(lgp->mutex, "ftd lg mutex");

    lgp->statsize = info->statsize;

    lgp->ctlp = ctlp;
    lgp->dev = info->lgdev;

    lgp->next = ctlp->lghead;
    ctlp->lghead = lgp;

    lgp->state = FTD_MODE_PASSTHRU;

    lgp->sync_depth = (unsigned int) -1;
    lgp->sync_timeout = 0;
    lgp->iodelay = 0;
    lgp->ndevs = 0;
    lgp->dirtymaplag = 0;

    if ( !NT_SUCCESS(ftd_nt_start_lg_thread(lgp)) ) 
    {
        ftd_bab_free_mgr(lgp->mgr);
        ddi_soft_state_free(ftd_lg_state, lg_minor_num);
        OUT_FCT(ftd_ctl_new_lg)
        return(ENXIO);
    }

    OUT_FCT(ftd_ctl_new_lg)
    return(FTD_SUCCESS);
}

static int
ftd_ctl_init_stop(ftd_lg_t *lgp)
{
    ftd_dev_t  *devp;    
    minor_t     minor;

    IN_FCT(ftd_ctl_init_stop)

    if (!lgp)
    {
        OUT_FCT(ftd_ctl_init_stop)
        return ENXIO;
    }

    for (devp = lgp->devhead; devp; devp = devp->next) 
    {
        if (devp->flags & FTD_DEV_OPEN) 
        {
            OUT_FCT(ftd_ctl_init_stop)
            return EBUSY;
        }
    }
    
    for (devp = lgp->devhead; devp; devp = devp->next) 
    {
        devp->flags |= FTD_STOP_IN_PROGRESS;
    }

    OUT_FCT(ftd_ctl_init_stop)
    return FTD_SUCCESS;
}

static int
ftd_ctl_del_device(dev_t dev, int cmd, int arg, int flag)
{
    ftd_dev_t   *softp, **link;
    minor_t     minor;
    dev_t       devdev = *(dev_t *)arg;
    int         RetValue;

    IN_FCT(ftd_ctl_del_device)

    minor = getminor(devdev);

    softp = (ftd_dev_t *) ddi_get_soft_state(ftd_dev_state, minor);

    if (!softp)
    {
        OUT_FCT(ftd_ctl_del_device)
        return ENXIO;
    }

    if (softp->flags & FTD_DEV_OPEN)
    {
        OUT_FCT(ftd_ctl_del_device)
        return EBUSY;
    }

    RetValue = ftd_del_device(softp, minor);
    OUT_FCT(ftd_ctl_del_device)
    return RetValue;
}

FTD_PRIVATE FTD_STATUS
ftd_ctl_del_lg(dev_t dev, int cmd, int arg, int flag)
{
    minor_t     minor;
    ftd_lg_t    *lgp, **link;
    dev_t       devdev = *(dev_t *)arg;
    FTD_STATUS  RetValue;


    IN_FCT(ftd_ctl_del_lg)

    minor = getminor(devdev) & ~FTD_LGFLAG;

    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, minor);

    if (!lgp)
    {
        OUT_FCT(ftd_ctl_del_lg)
        return ENXIO;
    }

    RetValue = ftd_del_lg(lgp, minor);
    OUT_FCT(ftd_ctl_del_lg)
    return RetValue;
}

FTD_PRIVATE FTD_STATUS
ftd_ctl_ctl_config(dev_t dev, int cmd, int arg, int flag)
{
    IN_FCT(ftd_ctl_ctl_config)
    return ENOTTY;
    OUT_FCT(ftd_ctl_ctl_config)
}

FTD_PRIVATE FTD_STATUS
ftd_ctl_get_dev_state_buffer(dev_t dev, int cmd, int arg, int flag)
{
    stat_buffer_t *sbptr, sb;
    ftd_dev_t     *devp;
    ftd_lg_t      *lgp;
    minor_t       minor;

    IN_FCT(ftd_ctl_get_dev_state_buffer)

    sbptr = (stat_buffer_t *)arg;

    sb = *sbptr;

    /* get the logical group state */
    minor = getminor(sb.lg_num) & ~FTD_LGFLAG;

    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, minor);
    if (!lgp)
    {
        OUT_FCT(ftd_ctl_get_dev_state_buffer)
        return ENXIO;
    }

    /* get the device state */
    minor = getminor(sb.dev_num);
    devp = (ftd_dev_t *) ddi_get_soft_state(ftd_dev_state, minor);
    if (!devp)
    {
        OUT_FCT(ftd_ctl_get_dev_state_buffer)
        return ENXIO;
    }

    if (sb.len != devp->statsize)
    {
        OUT_FCT(ftd_ctl_get_dev_state_buffer)
        return EINVAL;
    }

    if (ddi_copyout((caddr_t)devp->statbuf, (caddr_t)sb.addr, sb.len, flag))
    {
        OUT_FCT(ftd_ctl_get_dev_state_buffer)
        return EFAULT;
    }

    OUT_FCT(ftd_ctl_get_dev_state_buffer)
    return FTD_SUCCESS;;
}

FTD_PRIVATE FTD_STATUS
ftd_ctl_get_lg_state_buffer(dev_t dev, int cmd, int arg, int flag)
{
    stat_buffer_t *sbptr, sb;
    ftd_lg_t      *lgp;
    minor_t       minor;

    IN_FCT(ftd_ctl_get_lg_state_buffer)

    sbptr = (stat_buffer_t *)arg;

    sb = *sbptr;

    /* get the logical group state */
    minor = getminor(sb.lg_num) & ~FTD_LGFLAG;
    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, minor);
    if (!lgp)
    {
        OUT_FCT(ftd_ctl_get_lg_state_buffer)
        return ENXIO;
    }
    if (sb.len != lgp->statsize)
    {
        OUT_FCT(ftd_ctl_get_lg_state_buffer)
        return EINVAL;
    }
    if (ddi_copyout((caddr_t)lgp->statbuf, (caddr_t)sb.addr, sb.len, flag))
    {
        OUT_FCT(ftd_ctl_get_lg_state_buffer)
        return EFAULT;
    }

    OUT_FCT(ftd_ctl_get_lg_state_buffer)
    return FTD_SUCCESS;;
}

FTD_PRIVATE FTD_STATUS
ftd_ctl_set_dev_state_buffer(dev_t dev, int cmd, int arg, int flag)
{
    stat_buffer_t *sbptr, sb;
    ftd_dev_t     *devp;
    ftd_lg_t      *lgp;
    minor_t       minor;

    IN_FCT(ftd_ctl_set_dev_state_buffer)

    sbptr = (stat_buffer_t *)arg;

    sb = *sbptr;

    /* get the logical group state */
    minor = getminor(sb.lg_num) & ~FTD_LGFLAG;
    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, minor);
    if (!lgp)
    {
        OUT_FCT(ftd_ctl_set_dev_state_buffer)
        return ENXIO;
    }

    /* get the device state */
    minor = getminor(sb.dev_num);
    devp = (ftd_dev_t *) ddi_get_soft_state(ftd_dev_state, minor);
    if (!devp)
    {
        OUT_FCT(ftd_ctl_set_dev_state_buffer)
        return ENXIO;
    }

    if (sb.len != devp->statsize)
    {
        OUT_FCT(ftd_ctl_set_dev_state_buffer)
        return EINVAL;
    }

    if (ddi_copyin((caddr_t)sb.addr, (caddr_t)devp->statbuf, sb.len, flag))
    {
        OUT_FCT(ftd_ctl_set_dev_state_buffer)
        return EFAULT;
    }

    OUT_FCT(ftd_ctl_set_dev_state_buffer)
    return FTD_SUCCESS;;
}

FTD_PRIVATE FTD_STATUS
ftd_ctl_set_lg_state_buffer(dev_t dev, int cmd, int arg, int flag)
{
    stat_buffer_t *sbptr, sb;
    ftd_lg_t      *lgp;
    minor_t       m;

    IN_FCT(ftd_ctl_set_lg_state_buffer)

    sbptr = (stat_buffer_t *)arg;

    sb = *sbptr;

    /* get the logical group state */
    m = getminor(sb.lg_num) & ~FTD_LGFLAG;
    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, m);
    if (lgp == NULL)
    {
        OUT_FCT(ftd_ctl_set_lg_state_buffer)
        return ENXIO;
    }
    if (sb.len != lgp->statsize)
    {
        OUT_FCT(ftd_ctl_set_lg_state_buffer)
        return EINVAL;
    }
    if (ddi_copyin((caddr_t)sb.addr, (caddr_t)lgp->statbuf, sb.len, flag))
    {
        OUT_FCT(ftd_ctl_set_lg_state_buffer)
        return EFAULT;
    }
    OUT_FCT(ftd_ctl_set_lg_state_buffer)
    return FTD_SUCCESS;;
}

FTD_PRIVATE FTD_STATUS
ftd_ctl_set_iodelay(int arg)
{
    ftd_param_t         vb;
    ftd_lg_t            *lgp;
    minor_t             minor_num;
    FTD_CONTEXT         context;
    PRE_ACQUIRE_MUTEX

    IN_FCT(ftd_ctl_set_iodelay)

    vb = *(ftd_param_t *)arg;

    minor_num = getminor(vb.lgnum) & ~FTD_LGFLAG;
    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, minor_num);
    if (lgp == NULL) 
    {
        OUT_FCT(ftd_ctl_set_iodelay)
        return ENXIO;
    }
    ACQUIRE_MUTEX(lgp->mutex, context);
    lgp->iodelay = vb.value;
    RELEASE_MUTEX(lgp->mutex, context);

    OUT_FCT(ftd_ctl_set_iodelay)
    return FTD_SUCCESS;;
}

FTD_PRIVATE FTD_STATUS
ftd_ctl_set_sync_depth(int arg)
{
    ftd_param_t         vb;
    ftd_lg_t            *lgp;
    minor_t             minor_num;
    FTD_CONTEXT         context;
    PRE_ACQUIRE_MUTEX

    IN_FCT(ftd_ctl_set_sync_depth)

    vb = *(ftd_param_t *)arg;

    minor_num = getminor(vb.lgnum) & ~FTD_LGFLAG;
    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, minor_num);
    if (lgp == NULL) 
    {
        OUT_FCT(ftd_ctl_set_sync_depth)
        return ENXIO;
    }
    if (vb.value == 0)
    {
        OUT_FCT(ftd_ctl_set_sync_depth)
        return EINVAL;
    }
    ACQUIRE_MUTEX(lgp->mutex, context);
    /*
     * Go ahead and release the sleepers in sync mode
     */
    ftd_do_sync_done(lgp);
    lgp->sync_depth = vb.value;
    RELEASE_MUTEX(lgp->mutex, context);

    OUT_FCT(ftd_ctl_set_sync_depth)
    return FTD_SUCCESS;;
}

FTD_PRIVATE FTD_STATUS
ftd_ctl_set_sync_timeout(int arg)
{
    ftd_param_t         vb;
    ftd_lg_t            *lgp;
    minor_t             minor_num;
    FTD_CONTEXT         context;
    PRE_ACQUIRE_MUTEX

    IN_FCT(ftd_ctl_set_sync_timeout)

    vb = *(ftd_param_t *)arg;

    minor_num = getminor(vb.lgnum) & ~FTD_LGFLAG;
    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, minor_num);
    if (lgp == NULL) 
    {
        OUT_FCT(ftd_ctl_set_sync_timeout)
        return ENXIO;
    }
    ACQUIRE_MUTEX(lgp->mutex, context);
    lgp->sync_timeout = vb.value;
    RELEASE_MUTEX(lgp->mutex, context);

    OUT_FCT(ftd_ctl_set_sync_timeout)
    return FTD_SUCCESS;;
}

/*
 * Start the logical group.
 *
 * This call should tell the logical group that all initiailization is
 * complete and to start journalling or accumulating dirty bits. We should
 * add a bunch of code to make sure that state of the group is complete.
 */
FTD_PRIVATE FTD_STATUS
ftd_ctl_start_lg(int arg, int flag)
{
    dev_t    dev;
    ftd_lg_t *lgp;
    minor_t  minor;

    IN_FCT(ftd_ctl_start_lg)

    dev = *(dev_t *)arg;

    minor = getminor(dev) & ~FTD_LGFLAG;
    lgp = (ftd_lg_t *) ddi_get_soft_state(ftd_lg_state, minor);
    if (!lgp)
    {
        OUT_FCT(ftd_ctl_start_lg)
        return ENXIO;
    }

    /* FIXME: to make this work correctly, the init code should pass down 
      dirtybits and other state crap from the persistent store and then 
      we can decide what state to go into. Obviously, all of the state
      handling in this driver is terribly incomplete. */

    switch (lgp->state) 
    {
    case FTD_MODE_PASSTHRU:
    case FTD_MODE_TRACKING:
    case FTD_MODE_REFRESH:
    case FTD_MODE_NORMAL: /* do nothing */
        break;
    default:
        OUT_FCT(ftd_ctl_start_lg)
        return ENOTTY;
    }
    /* Need to somehow flush the state to disk here, no? XXX FIXME XXX */

    OUT_FCT(ftd_ctl_start_lg)
    return FTD_SUCCESS;;
}

int
ftd_ctl_ioctl(dev_t dev, int cmd, int arg, int flag)
{
    int         err = 0;
    ftd_ctl_t   *ctlp;

    IN_FCT(ftd_ctl_ioctl)

    if ((ctlp = ftd_global_state) == NULL)
    {
        OUT_FCT(ftd_ctl_ioctl)
        return (ENXIO);         /* invalid minor number */
    }

    switch (IOC_CST(cmd)) 
    {
    case IOC_CST(FTD_GET_CONFIG):
        err = ftd_ctl_get_config(dev, cmd, arg, flag);
        break;
    case IOC_CST(FTD_NEW_DEVICE):
        err = ftd_ctl_new_device(dev, cmd, arg, flag);
        break;
    case IOC_CST(FTD_NEW_LG):
        err = ftd_ctl_new_lg(dev, cmd, arg, flag);
        break;
    case IOC_CST(FTD_DEL_DEVICE):
        err = ftd_ctl_del_device(dev, cmd, arg, flag);
        break;
    case IOC_CST(FTD_DEL_LG):
        err = ftd_ctl_del_lg(dev, cmd, arg, flag);
        break;
    case IOC_CST(FTD_CTL_CONFIG):
        err = ftd_ctl_ctl_config(dev, cmd, arg, flag);
        break;
    case IOC_CST(FTD_GET_DEV_STATE_BUFFER):
        err = ftd_ctl_get_dev_state_buffer(dev, cmd, arg, flag);
        break;
    case IOC_CST(FTD_GET_LG_STATE_BUFFER):
        err = ftd_ctl_get_lg_state_buffer(dev, cmd, arg, flag);
        break;
    case IOC_CST(FTD_SET_DEV_STATE_BUFFER):
        err = ftd_ctl_set_dev_state_buffer(dev, cmd, arg, flag);
        break;
    case IOC_CST(FTD_SET_LG_STATE_BUFFER):
        err = ftd_ctl_set_lg_state_buffer(dev, cmd, arg, flag);
        break;
    case IOC_CST(FTD_GET_DEVICE_NUMS):
        err = ftd_ctl_get_device_nums(dev, arg, flag);
        break;
    case IOC_CST(FTD_START_LG):
        err = ftd_ctl_start_lg(arg, flag);
        break;
    case IOC_CST(FTD_GET_NUM_DEVICES):
        err = ftd_ctl_get_num_devices(arg, flag);
        break;
    case IOC_CST(FTD_GET_NUM_GROUPS):
        err = ftd_ctl_get_num_groups(arg, flag);
        break;
    case IOC_CST(FTD_GET_DEVICES_INFO):
        err = ftd_ctl_get_devices_info(arg, flag);
        break;
    case IOC_CST(FTD_GET_GROUPS_INFO):
        err = ftd_ctl_get_groups_info(arg, flag);
        break;
    case IOC_CST(FTD_GET_DEVICE_STATS):
        err = ftd_ctl_get_device_stats(arg, flag);
        break;
    case IOC_CST(FTD_GET_GROUP_STATS):
        err = ftd_ctl_get_group_stats(arg, flag);
        break;
    case IOC_CST(FTD_SET_GROUP_STATE):
        err = ftd_ctl_set_group_state(arg, flag);
        break;
    case IOC_CST(FTD_CLEAR_BAB):
        err = ftd_ctl_clear_bab(arg);
        break;
    case IOC_CST(FTD_CLEAR_LRDBS):
        err = ftd_ctl_clear_dirtybits(arg, FTD_LOW_RES_DIRTYBITS);
        break;
    case IOC_CST(FTD_CLEAR_HRDBS):
        err = ftd_ctl_clear_dirtybits(arg, FTD_HIGH_RES_DIRTYBITS);
        break;
    case IOC_CST(FTD_GET_GROUP_STATE):
        err = ftd_ctl_get_group_state(arg, flag);
        break;
    case IOC_CST(FTD_GET_BAB_SIZE):
        err = ftd_ctl_get_bab_size(arg);
        break;
    case IOC_CST(FTD_UPDATE_LRDBS):
        err = ftd_ctl_update_lrdbs(arg);
        break;
    case IOC_CST(FTD_UPDATE_HRDBS):
        err = ftd_ctl_update_hrdbs(arg);
        break;
    case IOC_CST(FTD_SET_IODELAY):
        err = ftd_ctl_set_iodelay(arg);
        break;
    case IOC_CST(FTD_SET_SYNC_DEPTH):
        err = ftd_ctl_set_sync_depth(arg);
        break;
    case IOC_CST(FTD_SET_SYNC_TIMEOUT):
        err = ftd_ctl_set_sync_timeout(arg);
        break;
    default:
#if defined(FTD_DEBUG)
FTD_ERR(FTD_WRNLVL, "ftd_ctl_ioctl: Unknown IOC.\n");
#endif /* defined(FTD_DEBUG) */
        err = ENOTTY;
        break;
    }
    OUT_FCT(ftd_ctl_ioctl)
    return (err);
}

FTD_PRIVATE FTD_STATUS
ftd_lg_send_lg_message(ftd_lg_t *lgp, int cmd, int arg, int flag)
{
    stat_buffer_t   sb;
    FTD_CONTEXT     context;
    ftd_uint64_t    *buf;
    unsigned int    size64;
    wlheader_t  *hp;
    PRE_ACQUIRE_MUTEX
    
    IN_FCT(ftd_lg_send_lg_message)

    sb = *(stat_buffer_t *)arg;
    /* enforce that this be a multiple of DEV_BSIZE */
    if (sb.len & (DEV_BSIZE - 1))
    {
        OUT_FCT(ftd_lg_send_lg_message)
        return EINVAL;
    }
    size64 = sizeof_64bit(wlheader_t) + (sb.len / 8);
    ACQUIRE_MUTEX(lgp->mutex, context);
    if ((buf = ftd_bab_alloc_memory(lgp->mgr, size64)) != NULL) 
    {
        if (lgp->mgr->flags & FTD_BAB_PHYSICAL) 
        {
            hp = ftd_bab_map_memory(buf, size64 * 8);
        }
        else 
        {
            hp = (wlheader_t *) buf;
        }

        hp->majicnum = DATASTAR_MAJIC_NUM;
        hp->offset = -1;
        hp->length = sb.len >> DEV_BSHIFT;
        hp->dev = -1;
        hp->diskdev = -1;
        hp->group_ptr = lgp;
        hp->complete = 1;
        hp->flags = 0;
        hp->bp = 0;
        lgp->wlentries++;
        lgp->wlsectors += hp->length;
        /* Copy the message in */
        if (ddi_copyin(sb.addr, (caddr_t) (((caddr_t)hp) + sizeof(wlheader_t)),
            sb.len, flag)) 
        {
            if (lgp->mgr->flags & FTD_BAB_PHYSICAL)
                ftd_bab_unmap_memory(hp, size64 * 8);
            RELEASE_MUTEX(lgp->mutex, context);

            OUT_FCT(ftd_lg_send_lg_message)
            return EFAULT;
        }
        /*
         * Don't commit the memory unless we're the last one on the list.
         * Otherwise we may commit part of an outstanding I/O and all hell
         * is likely to break loose.
         */
        
        if (ftd_bab_get_pending(lgp->mgr) == (ftd_uint64_t *)buf) 
        {
            ftd_bab_commit_memory(lgp->mgr, size64);
        }
    } 
    else 
    {
        RELEASE_MUTEX(lgp->mutex, context);
        OUT_FCT(ftd_lg_send_lg_message)
        return EAGAIN;
    }
    
    if (lgp->mgr->flags & FTD_BAB_PHYSICAL)
        ftd_bab_unmap_memory(hp, size64 * 8);

    RELEASE_MUTEX(lgp->mutex, context);

    ftd_wakeup(lgp);

    OUT_FCT(ftd_lg_send_lg_message)
    return FTD_SUCCESS;;
}

FTD_PRIVATE FTD_STATUS
ftd_lg_oldest_entries(ftd_lg_t *lgp, int arg, int flag)
{
    oldest_entries_t  oe;
    FTD_CONTEXT context;
    int len64, destlen64, offset64, err;
    PRE_ACQUIRE_MUTEX

    IN_FCT(ftd_lg_oldest_entries)

    oe = *(oldest_entries_t *)arg;
    offset64 = oe.offset32 >> 1;
    destlen64 = oe.len32 >> 1;

    /* get a snapshot of the amount of committed memory at this time */
    /* We do the locking only for the call to get_committed_length
     * we think that the length can never shrink.  But can't it if
     * a migrate has happend after we release the lock?  Since we have
     * a ST pmd, I don't think this is an issue.  This call will complete
     * before another call is made to migrate.  I think that we get back
     * an error message EINTR propigated back to the userland program,
     * which as of this writing is handled by splatting a big error message
     * that even I couldn't miss in the logs.
     */
    ACQUIRE_MUTEX(lgp->mutex, context);
    len64 = ftd_bab_get_committed_length(lgp->mgr);
    RELEASE_MUTEX(lgp->mutex, context);

    if (len64 == 0) 
    {
        oe.retlen32 = 0;
        err = 0;
    } 
    else 
    {
        int keep = len64;
        len64 -= offset64;
        if (len64 > 0) 
        {
            if (destlen64 > len64) 
            {
                destlen64 = len64;
            }
            /* copy the data */
            ACQUIRE_MUTEX(lgp->mutex, context);
            ftd_bab_copy_out(lgp->mgr, offset64, (ftd_uint64_t *)oe.addr, 
                destlen64);
            RELEASE_MUTEX(lgp->mutex, context);
            
            oe.retlen32 = destlen64 * 2;
            err = 0;
        } 
        else 
        {

            //
            // If len64 is 0, we just assume there is nothing
            // to copy, and this is valid.
            // if it is not 0, it means it is negative, and 
            // that is a "REAL" problem!
            //
            if (len64 == 0)
            {
                oe.retlen32 = 0;
                err = 0;
            }
            else
            {
                oe.retlen32 = 0;
                err = EINVAL;
            }
        }

        if ( (len64 - destlen64) > 0 )
            ftd_wakeup(lgp);
    }

    oe.state = lgp->state;
    *((oldest_entries_t *)arg) = oe;

    OUT_FCT(ftd_lg_oldest_entries)
    return err;
}

FTD_PRIVATE ftd_dev_t *
ftd_lookup_dev(ftd_lg_t *lgp, dev_t d)
{
    ftd_dev_t           *softp;
    
    IN_FCT(ftd_lookup_dev)

    softp = lgp->devhead;
    while (softp) 
    {
        if (softp->cdev == d)
        {
            OUT_FCT(ftd_lookup_dev)
            return softp;
        }
        softp = softp->next;
    }
    OUT_FCT(ftd_lookup_dev)
    return FTD_SUCCESS;;
}

FTD_PRIVATE FTD_STATUS
ftd_lg_migrate(ftd_lg_t *lgp, int arg, int flag)
{
    int              i;
    FTD_CONTEXT            context;
    minor_t          minor;
    ftd_dev_t        *softp;
    ftd_uint64_t     *temp;
    wlheader_t       *wl;
    migrated_entries_t me;
    bab_buffer_t     *buf;
    int             bytes;
    dev_t            lastdev;
    PRE_ACQUIRE_MUTEX

    IN_FCT(ftd_lg_migrate)

    me = *(migrated_entries_t *)arg;

    if (me.bytes == 0)
    {
        OUT_FCT(ftd_lg_migrate)
        return FTD_SUCCESS;
    }

    if (me.bytes & 0x7) 
    {
        FTD_ERR(FTD_WRNLVL, "Tried to migrate %d bytes, which is illegal", me.bytes);
        OUT_FCT(ftd_lg_migrate)
        return EINVAL;
    }
    
    if (lgp->mgr->in_use_head == NULL) 
    {
        FTD_ERR(FTD_WRNLVL, "Trying to migrate when we have no data!");
        OUT_FCT(ftd_lg_migrate)
        return EINVAL;
    }
    /* FIXME: revisit this mutex as well as the bab mutex. */
    ACQUIRE_MUTEX(lgp->mutex, context);

    /*
     * Walk through the entries being migrated, calling biodone(hp->bp)
     * if hp->bp is not NULL.  We do this to implement sync mode.
     * async mode is implemented in exactly the same way as sync mode,
     * but with a depth of 0xffffffff, which cannot be exceeded by 
     * definition of int.
     */
    BAB_MGR_FIRST_BLOCK(lgp->mgr, buf, temp);
    bytes = me.bytes;
    lastdev = -1;
    softp = NULL;
    while (temp && bytes > 0) 
    {
        if (lgp->mgr->flags & FTD_BAB_PHYSICAL) 
        {
            wl = ftd_bab_map_memory(temp, sizeof(wlheader_t));
        } 
        else 
        {
            wl = (wlheader_t *) temp;
        }

        if (lgp->wlentries <= 0) 
        {
            FTD_ERR(FTD_WRNLVL, "LG %d trying to migrate too many entries", lgp->dev & ~FTD_LGFLAG);
        
            if (lgp->mgr->flags & FTD_BAB_PHYSICAL)
                ftd_bab_unmap_memory(wl, sizeof(wlheader_t));
            
            break;
        }
        if (lgp->wlsectors <= 0) 
        {
            FTD_ERR(FTD_WRNLVL, "LG %d trying to migrate too many sectors", lgp->dev & ~FTD_LGFLAG);
        
            if (lgp->mgr->flags & FTD_BAB_PHYSICAL)
                ftd_bab_unmap_memory(wl, sizeof(wlheader_t));
            
            break;
        }
        if (wl->majicnum != DATASTAR_MAJIC_NUM) 
        {
            FTD_ERR(FTD_WRNLVL, "LG %d corrupted BAB entry migrated", lgp->dev & ~FTD_LGFLAG);
        
            if (lgp->mgr->flags & FTD_BAB_PHYSICAL)
                ftd_bab_unmap_memory(wl, sizeof(wlheader_t));
            
            break;
        }
        if (wl->complete == 0) 
        {
            FTD_ERR(FTD_WRNLVL, "LG %d trying to migrate uncompleted entry", lgp->dev & ~FTD_LGFLAG);
        }
        if (lgp->wlentries == 0) 
        {
            FTD_ERR(FTD_WRNLVL, "LG %d migrated too many entries!", lgp->dev & ~FTD_LGFLAG);
        
            if (lgp->mgr->flags & FTD_BAB_PHYSICAL)
                ftd_bab_unmap_memory(wl, sizeof(wlheader_t));
            
            break;
        }
        if (lgp->wlsectors < wl->length) 
        {
            FTD_ERR(FTD_WRNLVL, "LG %d migrated too many sectors!", lgp->dev & ~FTD_LGFLAG);
        
            if (lgp->mgr->flags & FTD_BAB_PHYSICAL)
                ftd_bab_unmap_memory(wl, sizeof(wlheader_t));
            
            break;
        }

        if (wl->bp) 
        {
        /* cancel any sync mode timout */

            IoSetCancelRoutine(wl->bp, NULL);

            finish_biodone(wl->bp);
        }

        wl->bp = 0;
        if (wl->dev != lastdev) 
        {
            softp = ftd_lookup_dev(lgp, wl->dev);
            lastdev = wl->dev;
        }
        if (softp) 
        {
            softp->wlentries--;
            softp->wlsectors -= wl->length;
        }
        bytes -= ((wl->length << (DEV_BSHIFT - 3)) + 
            sizeof_64bit(wlheader_t)) << 3;
        lgp->wlentries--;
        lgp->wlsectors -= wl->length;

        BAB_MGR_NEXT_BLOCK(temp, wl, buf);

        if (lgp->mgr->flags & FTD_BAB_PHYSICAL)
            ftd_bab_unmap_memory(wl, sizeof(wlheader_t));
    }
    if (bytes > 0)
        FTD_ERR(FTD_WRNLVL, "Too little data migrated");
    else if (bytes < 0)
        FTD_ERR(FTD_WRNLVL, "Too much data migrated");

    /* bab_free wants 64-bits words. */
    ftd_bab_free_memory(lgp->mgr, me.bytes >> 3);

    /* we want to occasionally reset dirty bits based on what is
       really dirty in the bab (so that we don't end up with a 
       completely dirty bitmap over time) */
    lgp->dirtymaplag++;
    RELEASE_MUTEX(lgp->mutex, context);
    if (lgp->dirtymaplag == FTD_DIRTY_LAG && lgp->wlsectors < FTD_RESET_MAX) 
    {
        lgp->dirtymaplag = 0;
        ftd_lg_update_dirtybits(lgp);
    }

    OUT_FCT(ftd_lg_migrate)
    return FTD_SUCCESS;;
}

FTD_PRIVATE FTD_STATUS
ftd_lg_get_dirty_bits(ftd_lg_t *lgp, int cmd, int arg, int flag)
{
    int              i, offset32;
    dev_t            dev;
    ftd_dev_t        *softp;
    dirtybit_array_t dbarray;

    IN_FCT(ftd_lg_get_dirty_bits)

    dbarray = *(dirtybit_array_t *)arg;

    /*
     * The following tricky bit of logic is intended to return ENOENT
     * if the bits are invalid.  The bits are valid only in RECOVER mode
     * and when in TRACKING mode with no entries in the writelog
     */
    if (dbarray.state_change &&
        ((lgp->state & FTD_M_BITUPDATE) == 0 || lgp->wlentries != 0))
    {
        OUT_FCT(ftd_lg_get_dirty_bits)
        return ENOENT;
    }

    offset32 = 0;
    for (i = 0; i < dbarray.numdevs; i++) 
    {
        /* get the device id */
        if (ddi_copyin((caddr_t)(dbarray.devs + i), (caddr_t)&dev, 
                    sizeof(dev_t), flag))
        {
            OUT_FCT(ftd_lg_get_dirty_bits)
            return EFAULT;
        }

        /* find the matching device in our logical group */
        softp = lgp->devhead;
        while (softp != NULL) 
        {
            if (dev == softp->cdev) 
            {
                if (IOC_CST(cmd) == IOC_CST(FTD_GET_LRDBS)) 
                {
                    if (ddi_copyout((caddr_t)softp->lrdb.map, 
                        (caddr_t)(dbarray.dbbuf + offset32), 
                        softp->lrdb.len32*4, flag))
                    {
                        OUT_FCT(ftd_lg_get_dirty_bits)
                        return EFAULT;
                    }
                    offset32 += softp->lrdb.len32;
                } 
                else 
                {
                    if (ddi_copyout((caddr_t)softp->hrdb.map, 
                        (caddr_t)(dbarray.dbbuf + offset32), 
                        softp->hrdb.len32*4, flag))
                    {
                        OUT_FCT(ftd_lg_get_dirty_bits)
                        return EFAULT;
                    }
                    offset32 += softp->hrdb.len32;
                }
                break;
            }
            softp = softp->next;
        }
    }
    if (dbarray.state_change)
        lgp->state = FTD_MODE_REFRESH;
        /* Need to somehow flush the state to disk here, no? XXX FIXME XXX */

    OUT_FCT(ftd_lg_get_dirty_bits)
    return FTD_SUCCESS;;
}

/*
 * Return an array of devices and bitmap size sizes for each device in
 * the logical group. 
 */
FTD_PRIVATE FTD_STATUS
ftd_lg_get_dirty_bit_info(ftd_lg_t *lgp, int cmd, int arg, int flag)
{
    dirtybit_array_t dbarray;
    int i, offset32;
    ftd_dev_t *softp;

    IN_FCT(ftd_lg_get_dirty_bit_info)

    /* XXX FIXME: Need to check the length to make sure that we're OK */
    /* XXX Maybe we don't have enough info? */

    dbarray = *(dirtybit_array_t *)arg;

    /* get the number of devices in the group and make sure the buffer
      is big enough */
    for (i = 0, softp = lgp->devhead; softp != NULL; i++) 
    {
        softp = softp->next;
    }
    if (ddi_copyout((caddr_t)&i, (caddr_t)&dbarray.numdevs, sizeof(int), flag))
    {
        OUT_FCT(ftd_lg_get_dirty_bit_info)
        return EFAULT;
    }

    if (i > dbarray.numdevs)
    {
        OUT_FCT(ftd_lg_get_dirty_bit_info)
        return EINVAL;
    }

    offset32 = 0;
    softp = lgp->devhead;
    for (i = 0; softp != NULL; i++) 
    {
        if (ddi_copyout((caddr_t)&softp->cdev, (caddr_t)(dbarray.devs + i),
            sizeof(dev_t), flag))
        {
            OUT_FCT(ftd_lg_get_dirty_bit_info)
            return EFAULT;
        }
        if (IOC_CST(cmd) == IOC_CST(FTD_GET_LRDB_INFO)) \
        {
            if (ddi_copyout((caddr_t)&softp->lrdb.len32, 
                (caddr_t)(dbarray.dbbuf + i), sizeof(int), flag))
            {
                OUT_FCT(ftd_lg_get_dirty_bit_info)
                return EFAULT;
            }
        } 
        else 
        { /* IOC_CST(FTD_GET_HRDB_INFO) */
            if (ddi_copyout((caddr_t)&softp->hrdb.len32, 
                (caddr_t)(dbarray.dbbuf + i), sizeof(int), flag))
            {
                OUT_FCT(ftd_lg_get_dirty_bit_info)
                return EFAULT;
            }
        }
        softp = softp->next;
    }

    OUT_FCT(ftd_lg_get_dirty_bit_info)
    return FTD_SUCCESS;;
}

/*
 * given an array of devices and dirty bitmaps, copy the dirty bitmaps into
 * our bitmaps.
 */
FTD_PRIVATE FTD_STATUS
ftd_lg_set_dirty_bits(ftd_lg_t *lgp, int cmd, int arg, int flag)
{
    dirtybit_array_t dbarray;
    dev_t            dev;
    int              i, offset32;
    ftd_dev_t        *softp;

    IN_FCT(ftd_lg_set_dirty_bits)

    dbarray = *(dirtybit_array_t *)arg;

    offset32 = 0;
    for (i = 0; i < dbarray.numdevs; i++) 
    {
        if (ddi_copyin((caddr_t)(dbarray.devs + i), (caddr_t)&dev, 
            sizeof(dev_t), flag))
        {
            OUT_FCT(ftd_lg_set_dirty_bits)
            return EFAULT;
        }
        if ((softp = ftd_lg_get_device(lgp, dev)) != NULL) 
        {
            if (IOC_CST(cmd) == IOC_CST(FTD_SET_LRDBS)) 
            {
                if (ddi_copyin((caddr_t)(dbarray.dbbuf + offset32), 
                    (caddr_t)softp->lrdb.map, softp->lrdb.len32*4, flag))
                {
                    OUT_FCT(ftd_lg_set_dirty_bits)
                    return EFAULT;
                }
                offset32 += softp->lrdb.len32;
            } 
            else 
            {
                if (ddi_copyin((caddr_t)(dbarray.dbbuf + offset32), 
                    (caddr_t)softp->hrdb.map, softp->hrdb.len32*4, flag))
                {
                    OUT_FCT(ftd_lg_set_dirty_bits)
                    return EFAULT;
                }
                offset32 += softp->hrdb.len32;
            }
        } 
        else 
        {
            OUT_FCT(ftd_lg_set_dirty_bits)
            return ENXIO;
        }
    }

    OUT_FCT(ftd_lg_set_dirty_bits)
    return FTD_SUCCESS;;
}

/*
 * If we are in journal mode, update the lrdb with the contents of the
 * journal. Flush the lrdb to disk or do we just wait for the next I/O?
 * I say we wait, since a false positive won't hurt too much.
 */
FTD_PRIVATE FTD_STATUS
ftd_lg_update_dirtybits(ftd_lg_t *lgp)
{
    FTD_CONTEXT     context;
    ftd_dev_t *tempdev;
    PRE_ACQUIRE_MUTEX
    
    IN_FCT(ftd_lg_update_dirtybits)

    /* grab the group lock */
    ACQUIRE_MUTEX(lgp->mutex, context);

    /* we can only do this if we are in the proper state */
    if (lgp->state != FTD_MODE_NORMAL) 
    {
        RELEASE_MUTEX(lgp->mutex, context);
        OUT_FCT(ftd_lg_update_dirtybits)
        return EINVAL;
    }

    tempdev = lgp->devhead;
    while(tempdev) 
    {
        bzero((caddr_t)tempdev->lrdb.map, tempdev->lrdb.len32*4);
        bzero((caddr_t)tempdev->hrdb.map, tempdev->hrdb.len32*4);
        tempdev = tempdev->next;
    }

    /* compute the drtybits */
    ftd_compute_dirtybits(lgp, FTD_LOW_RES_DIRTYBITS);
    ftd_compute_dirtybits(lgp, FTD_HIGH_RES_DIRTYBITS);

    /* release the group lock */
    RELEASE_MUTEX(lgp->mutex, context);

    /* for (tempdev = lgp->devhead; tempdev; tempdev = tempdev->next) {
        ftd_flush_lrdb(tempdev, NULL);
    }
    */
    OUT_FCT(ftd_lg_update_dirtybits)
    return FTD_SUCCESS;;
}

int
ftd_lg_ioctl(dev_t dev, int cmd, int arg, int flag)
{
    int      err = 0;
    ftd_lg_t *lgp;

    minor_t minor = getminor(dev) & ~FTD_LGFLAG;

    IN_FCT(ftd_lg_ioctl)

    if ((lgp = ddi_get_soft_state(ftd_lg_state, minor)) == NULL ) 
    {
        OUT_FCT(ftd_lg_ioctl)
        return (ENXIO);         /* invalid minor number */
    }

    switch (IOC_CST(cmd)) 
    {
    case IOC_CST(FTD_OLDEST_ENTRIES):
        err = ftd_lg_oldest_entries(lgp, arg, flag);
        break;
    case IOC_CST(FTD_MIGRATE):
        err = ftd_lg_migrate(lgp, arg, flag);
        break;
    case IOC_CST(FTD_GET_LRDBS):
    case IOC_CST(FTD_GET_HRDBS):
        err = ftd_lg_get_dirty_bits(lgp, cmd, arg, flag);
        break;
    case IOC_CST(FTD_SET_LRDBS):
    case IOC_CST(FTD_SET_HRDBS):
        err = ftd_lg_set_dirty_bits(lgp, cmd, arg, flag);
        break;
    case IOC_CST(FTD_GET_LRDB_INFO):
    case IOC_CST(FTD_GET_HRDB_INFO):
        err = ftd_lg_get_dirty_bit_info(lgp, cmd, arg, flag);
        break;
    case IOC_CST(FTD_UPDATE_DIRTYBITS):
        err = ftd_lg_update_dirtybits(lgp);
        break;
    case IOC_CST(FTD_SEND_LG_MESSAGE):
        err = ftd_lg_send_lg_message(lgp, cmd, arg, flag);
        break;
    case IOC_CST(FTD_INIT_STOP):
        err = ftd_ctl_init_stop(lgp);
        break;
    default:
        err = ENOTTY;
        break;
    }
    OUT_FCT(ftd_lg_ioctl)
    return (err);
}

