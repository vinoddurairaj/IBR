/*
 * ftd_all.c - FullTime Data driver code for ALL platforms
 *
 * Copyright (c) 1996, 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 *
 */

/*
 * Includes, Declarations and Local Data
 */

#include "ftd_ddi.h"
#include "ftd_def.h"
#if defined(_WINDOWS)
#include "ftd_nt.h"
#endif
#include "ftd_bab.h"
#include "ftd_bits.h"
#include "ftd_var.h"
#include "ftd_all.h"
#include "ftd_klog.h"

/*
 * Local Function Prototypes
 */
FTD_PRIVATE FTD_STATUS ftd_iodone_generic(struct buf *bp);
FTD_PRIVATE FTD_STATUS ftd_iodone_journal(struct buf *bp);
FTD_PRIVATE FTD_STATUS ftd_do_io(ftd_dev_t *softp, struct buf *userbp, struct buf *mybp,
    FTD_CONTEXT context);
FTD_PRIVATE void ftd_do_read(ftd_dev_t *softp, struct buf *userbp, struct buf *mybp);
FTD_PRIVATE void ftd_do_write(ftd_dev_t *softp, struct buf *userbp, struct buf *mybp);

extern void ftd_clear_bab_and_stats(ftd_lg_t *lgp, FTD_CONTEXT context);
extern void finish_biodone(PIRP Irp);
extern void *ftd_memset(void *s, int c, size_t n);

/*
 * Local Static Data
 */
void      *ftd_dev_state = NULL;
void      *ftd_lg_state = NULL;
ftd_ctl_t *ftd_global_state = NULL;
int       ftd_failsafe = 0;

/*
 ********************************************************
 * Data device routines, generally
 ********************************************************
 */

/*
 * ftd_biodone
 *
 * This routine should be called to biodone any buffers that
 * have returned to us from the driver below.  
 *
 */
FTD_STATUS 
ftd_biodone(PIRP bp) {

    IN_FCT(ftd_biodone)

    finish_biodone(bp);

    OUT_FCT(ftd_biodone)

    return FTD_SUCCESS;
}

/*
 * Return the number of open devices for all logical groups.
 */
int
ftd_dev_n_open(ftd_ctl_t *ctlp)
{
    int retval = 0;
    ftd_lg_t *lgwalk;
    ftd_dev_t *devwalk;

    IN_FCT(ftd_dev_n_open)

    for (lgwalk = ctlp->lghead; lgwalk; lgwalk = lgwalk->next) {
        for (devwalk = lgwalk->devhead; devwalk; devwalk = devwalk->next) {
            if (devwalk->flags & FTD_DEV_OPEN)
                retval++;
        }
    }

    OUT_FCT(ftd_dev_n_open)

    return retval;
}

/*
 * Close the device.  Called when all references to this device go
 * away.
 */
FTD_STATUS
ftd_dev_close(dev_t dev, int flag)
{
    ftd_dev_t           *softp;
    minor_t minor = getminor(dev);

    IN_FCT(ftd_dev_close)

    if ((softp = ddi_get_soft_state(ftd_dev_state, minor)) == NULL )
    {
        OUT_FCT(ftd_dev_close)
        return (ENXIO);         /* invalid minor number */
    }

    softp->flags &= ~FTD_DEV_OPEN;
    /*
     * OK, save state if we're the last thing to close
     * in this logical group and the lgp is closed.  We don't
     * do this now because we do it in userland and have pessimal, but
     * correct crash recovery operations.
     */

    OUT_FCT(ftd_dev_close)
    return FTD_SUCCESS;
}

/*
 * Open the device.  Gets called once per open(2) call.
 *
 * XXX it appears that we don't check the exclusive flag passed to us,
 * XXX and I think we should.  Not sure what the consequences of that
 * XXX are.
 */
FTD_STATUS
ftd_dev_open(dev_t dev, int flag)
{
    ftd_dev_t           *softp;
    minor_t minor = getminor(dev);

    IN_FCT(ftd_dev_open)

    if ((softp = ddi_get_soft_state(ftd_dev_state, minor)) == NULL )
    {
        OUT_FCT(ftd_dev_open)
        return (ENXIO);         /* invalid minor number */
    }

    /* if stop has been initiated on this group, don't allow the
       open */
    if (softp->flags & FTD_STOP_IN_PROGRESS) 
    {
        OUT_FCT(ftd_dev_open)
        return (EBUSY);
    }

    softp->flags |= FTD_DEV_OPEN;

    OUT_FCT(ftd_dev_open)
    return FTD_SUCCESS;
}

/*
 * Delete the device.  We get called when the FTD_DEL_DEVICE call has
 * validated its arguments and wants to actually delete the device
 *
 * Here we should close down the device that we have opened that is the
 * data device.  We then detach from the logical group and cleanup
 * all memory for this device.
 */
FTD_STATUS
ftd_del_device(ftd_dev_t *softp, minor_t minor)
{
    ftd_dev_t   **link;
    struct buf  *freelist;
    struct buf  *buf;
    FTD_IRQL    context;

    PREACQUIRE_LOCK

    IN_FCT(ftd_del_device)

    if ( ftd_nt_detach_device(softp) != FTD_SUCCESS )
    {
        OUT_FCT(ftd_del_device)
       return (EACCES);
    }
    ACQUIRE_LOCK(softp->lock, context);
    
    /* detach from lg here */
    link = &softp->lgp->devhead;
    while ((*link != NULL) && (*link != softp)) 
    {
        link = &(*link)->next;
    }
    if (*link != NULL) 
    {
        *link = softp->next;
    }

    kmem_free(softp->statbuf, softp->statsize);

    /* waste the bitmaps */
    kmem_free(softp->lrdb.map, softp->lrdb.len32 * 4);
    kmem_free(softp->hrdb.map, softp->hrdb.len32 * 4);
    
    RELEASE_LOCK(softp->lock, context);

    DEALLOC_LOCK(softp->lock);

    if ( ftd_nt_del_device(softp) != FTD_SUCCESS )
    {
        OUT_FCT(ftd_del_device)
       return (EACCES);
    }

    ddi_soft_state_free(ftd_dev_state, minor);

    softp->lgp->ndevs--;

    OUT_FCT(ftd_del_device)
    return FTD_SUCCESS;
}

/*
 * Call biodone on all of the completed entries in the bab because
 * something with sync mode change.  This sometimes causes the writes 
 * to complete sooner than they would have otherwise, but that happens
 * only when when you change the sync depth, which isn't all that
 * frequent, and at most (old_sync_depth - new_sync_depth) entries
 * are prematurely biodone'd.
 *
 * You must hold the lgp->context lock when you call this routine.
 */
void
ftd_do_sync_done(ftd_lg_t *lgp)
{
    u_int           bytes;
    ftd_uint64_t    *temp;
    wlheader_t      *wl;
    bab_buffer_t    *buf;

    IN_FCT(ftd_do_sync_done)

    if (lgp->mgr == NULL || 
        lgp->mgr->in_use_head == NULL)
    {
        OUT_FCT(ftd_do_sync_done)
        return;
    }
    
    /*
     * Walk through the entries being migrated, calling biodone(hp->bp)
     * if hp->bp is not NULL.
     */
    bytes = 0x7fffffff;
    BAB_MGR_FIRST_BLOCK(lgp->mgr, buf, temp);
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
        if (wl->majicnum != DATASTAR_MAJIC_NUM) 
        {
            FTD_ERR(FTD_WRNLVL, "LG %d WriteLog header corrupt, (ftd_do_sync_done).", lgp->dev & ~FTD_LGFLAG);

            if (lgp->mgr->flags & FTD_BAB_PHYSICAL)
                ftd_bab_unmap_memory(wl, sizeof(wlheader_t));
        
            break;
        }
        if (wl->complete && wl->bp) 
        {
            IoSetCancelRoutine(wl->bp, NULL);
            ftd_biodone(wl->bp);
            wl->bp = 0;
        }
        bytes -= FTD_WLH_BYTES(wl);
        BAB_MGR_NEXT_BLOCK(temp, wl, buf);

        if (lgp->mgr->flags & FTD_BAB_PHYSICAL)
            ftd_bab_unmap_memory(wl, sizeof(wlheader_t));
    }
    OUT_FCT(ftd_do_sync_done)
}

/*
 * Simple routine that clears all the dirtybits in memory.  It is up to the
 * callers of this routine to arrange for that state to be flushed to disk.
 */
void
ftd_clear_dirtybits(ftd_lg_t *lgp)
{
    ftd_dev_t     *tempdev;

    IN_FCT(ftd_clear_dirtybits)

    tempdev = lgp->devhead;
    while(tempdev) 
    {
        bzero((caddr_t)tempdev->lrdb.map, tempdev->lrdb.len32*4);
        bzero((caddr_t)tempdev->hrdb.map, tempdev->hrdb.len32*4);
        tempdev = tempdev->next;
    }
    OUT_FCT(ftd_clear_dirtybits)
}

/*
 * Set all the dirty bits to 1.  Callers must make sure that this
 * state is updated on disk, where appropriate.
 */
void
ftd_set_dirtybits(ftd_lg_t *lgp)
{
    ftd_dev_t     *tempdev;

    IN_FCT(ftd_set_dirtybits)

    tempdev = lgp->devhead;
    while(tempdev) 
    {
        ftd_memset((caddr_t)tempdev->lrdb.map, 0xff, tempdev->lrdb.len32*4);
        ftd_memset((caddr_t)tempdev->hrdb.map, 0xff, tempdev->hrdb.len32*4);
        tempdev = tempdev->next;
    }
    OUT_FCT(ftd_set_dirtybits)
}

/*
 * Simple routine that clears all the high res tracking bits in memory.
 * No disk operation is needed because we don't store these in the
 * persistant store automatically.
 */
void
ftd_clear_hrdb(ftd_lg_t *lgp)
{
    ftd_dev_t     *tempdev;

    IN_FCT(ftd_clear_hrdb)

    tempdev = lgp->devhead;
    while(tempdev) 
    {
        bzero((caddr_t)tempdev->hrdb.map, tempdev->hrdb.len32*4);
        tempdev = tempdev->next;
    }
    
    OUT_FCT(ftd_clear_hrdb)
}
/*
 ********************************************************
 * Logical Group device routines, generally
 ********************************************************
 */

/*
 * Close logical group.  This usually means that the pmd has
 * gone away.
 */
FTD_STATUS
ftd_lg_close(dev_t dev, int flag)
{
    ftd_lg_t            *lgp;
    FTD_CONTEXT         context;
    FTD_IRQL            irql;
    minor_t minor = getminor(dev) & ~FTD_LGFLAG;
    PRE_ACQUIRE_MUTEX

    PREACQUIRE_LOCK

    IN_FCT(ftd_lg_close)

    if ((lgp = ddi_get_soft_state(ftd_lg_state, minor)) == NULL )
    {
        OUT_FCT(ftd_lg_close)    
        return (ENXIO);         /* invalid minor number */
    }

    ACQUIRE_MUTEX(lgp->mutex, context);
    ftd_do_sync_done(lgp);
    lgp->flags &= ~FTD_LG_OPEN;
    RELEASE_MUTEX(lgp->mutex, context);
    ACQUIRE_LOCK(lgp->ctlp->lock, irql);
    lgp->ctlp->lg_open--;
    RELEASE_LOCK(lgp->ctlp->lock, irql);

    ftd_nt_lg_close(lgp);

    OUT_FCT(ftd_lg_close)
    return FTD_SUCCESS;
}

/*
 * Open the logical group control device.  We allow only one open at a time
 * as a way of enforcing in the kernel that there is only one pmd per 
 * logical group at any given time.
 */
FTD_STATUS
ftd_lg_open(dev_t dev, int flag)
{
    ftd_lg_t            *lgp;
    FTD_CONTEXT          context;
    FTD_IRQL            irql;
    minor_t minor = getminor(dev) & ~FTD_LGFLAG;
    PRE_ACQUIRE_MUTEX
    PREACQUIRE_LOCK

    IN_FCT(ftd_lg_open)

    if ((lgp = ddi_get_soft_state(ftd_lg_state, minor)) == NULL ) 
    {
        OUT_FCT(ftd_lg_open)
        return (ENXIO);         /* invalid minor number */
    }
    ACQUIRE_MUTEX(lgp->mutex, context);
    if (lgp->flags & FTD_LG_OPEN) 
    {
        RELEASE_MUTEX(lgp->mutex, context);
        OUT_FCT(ftd_lg_open)
        return EBUSY;
    }
    lgp->flags |= FTD_LG_OPEN;
    RELEASE_MUTEX(lgp->mutex, context);
    ACQUIRE_LOCK(lgp->ctlp->lock, irql);
    lgp->ctlp->lg_open++;
    RELEASE_LOCK(lgp->ctlp->lock, irql);

    ftd_nt_lg_open(lgp, minor);

    OUT_FCT(ftd_lg_open)
    return FTD_SUCCESS;
}


/*
 * Called from the FTD_DEL_LG ioctl, this routine will delete any devices
 * still associated with the logical group and then remove this logical
 * group from our state structures.  Memory is then freed.
 */
FTD_STATUS
ftd_del_lg(ftd_lg_t *lgp, minor_t minor)
{
    ftd_lg_t      **link;
    FTD_CONTEXT      context;
    PRE_ACQUIRE_MUTEX

    IN_FCT(ftd_del_lg)

    ACQUIRE_MUTEX(lgp->mutex, context);

    /* remove from list of groups */
    link = &lgp->ctlp->lghead;
    while ((*link != NULL) && (*link != lgp)) 
    {
        link = &(*link)->next;
    }
    if (*link != NULL) 
    {
        *link = lgp->next;
    }

    /* Free any devices */
    while (lgp->devhead)
    {
        ftd_del_device(lgp->devhead, getminor(lgp->devhead->cdev));
    }

    ftd_clear_bab_and_stats(lgp, context);

    RELEASE_MUTEX(lgp->mutex, context);

    DEALLOC_MUTEX(lgp->mutex);
    DEALLOC_LOCK(lgp->lock);

    kmem_free(lgp->statbuf, lgp->statsize);

    /* waste the bab manager */
    if (ftd_bab_free_mgr(lgp->mgr) != 0)
    {
        FTD_ERR(FTD_WRNLVL, "There was a problem releasing the BAB manager for group: %d.", lgp->dev);
    }

    if (ftd_nt_del_lg(lgp) != FTD_SUCCESS)
    {
        OUT_FCT(ftd_del_lg)
        return (ENXIO);
    }

    ddi_soft_state_free(ftd_lg_state, minor);

    OUT_FCT(ftd_del_lg)    
    return FTD_SUCCESS;
}

/*
 ********************************************************
 * Control Device routines, generally
 ********************************************************
 */

/*
 * Close an open control device.  We have no state assocaited with
 * opens of this device, so when we close it we do nothing.  It exists
 * to be permissive about us getting data to/from ftd.  This routine
 * is only called on the LAST close, not for every close if multiple
 * people have the device open.
 */
FTD_STATUS 
ftd_ctl_close(dev_t dev, int flag)
{
    ftd_ctl_t           *ctlp;
    
    IN_FCT(ftd_ctl_close)

    if ((ctlp = ftd_global_state) == NULL)
    {
        OUT_FCT(ftd_ctl_close)
        return (ENXIO);         /* invalid minor number */
    }

    /*
     * since we don't change state in the open routine, I don't
     * think we need to do anything here
     */
    
    OUT_FCT(ftd_ctl_close)
    return FTD_SUCCESS;
}

/*
 * Open the control device.  We get called for every open.  Except
 * when we have no state at all (which is impossible), we return
 * success.
 */
FTD_STATUS 
ftd_ctl_open(dev_t dev, int flag)
{
    ftd_ctl_t           *ctlp;

    IN_FCT(ftd_ctl_open)

    if ((ctlp = ftd_global_state) == NULL)
    {
        OUT_FCT(ftd_ctl_open)
        return (ENXIO);         /* invalid minor number */
    }

    /* OK, we're pretty easy about things, so the open succeeds */

    OUT_FCT(ftd_ctl_open)
    return FTD_SUCCESS;
}
