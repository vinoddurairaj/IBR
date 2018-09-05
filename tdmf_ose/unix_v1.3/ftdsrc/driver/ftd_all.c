/*-
 * ftd_all.c - FullTime Data driver code for ALL platforms
 *
 * Copyright (c) 1996, 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 */

#include "ftd_kern_ctypes.h"
#include "ftd_ddi.h"
#include "ftd_def.h"
#include "ftd_bab.h"
#include "ftd_bits.h"
#include "ftd_var.h"
#include "ftd_all.h"
#include "ftd_klog.h"
#include "ftd_kern_cproto.h"

#ifdef INJECTERR
int injectErr = 0;
#endif

/* local static data */
ftd_void_t *ftd_dev_state = NULL;
ftd_void_t *ftd_lg_state = NULL;
ftd_ctl_t *ftd_global_state = NULL;
ftd_int32_t ftd_failsafe = 0;
ftd_char_t ftd_version_string[] = PRODUCTNAME " Release Version " VERSION;

/* data device routines: */

/*-
 * ftd_synctimo()
 * 
 * called to service synchronous mode timeout events.
 */
#if defined(_AIX)
FTD_PRIVATE ftd_void_t
ftd_synctimo (trbp)
     struct trb *trbp;
#else /* defined(_AIX) */
#if defined(SOLARIS) && (SYSVERS >= 570)
FTD_PRIVATE ftd_void_t
ftd_synctimo (argp)
     ftd_void_t *argp;
#else /* defined(SOLARIS) && (SYSVERS >= 570) */
FTD_PRIVATE ftd_void_t
ftd_synctimo (hp)
     struct wlheader_s *hp;
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(_AIX) */
{
  struct buf *bp;

#if defined(SOLARIS) && (SYSVERS >= 570)
  struct wlheader_s *hp = (struct wlheader_s *) argp;
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */

#if defined(_AIX)
	/*-
	 * _AIX calls the timo function with the address of the trb.
	 * the data address for the callback to biodone was stashed
	 * there when the timer was started...
	 */
  struct wlheader_s *hp = (struct wlheader_s *) trbp->t_func_addr;
#endif /* defined(_AIX) */

  /* save off any buf hdr pointer */
  bp = (struct buf *)hp->bp;

  /* clear header's pending buffer reference */
  hp->bp = 0;

  /* clear header's timer block reference */
#if defined(_AIX)

  /* _AIXism: release the timer */
  ftd_timer_rele (trbp);

  hp->timoid = INVALID_TIMEOUT_ID;

#else /* defined(_AIX) */

  hp->timoid = INVALID_TIMEOUT_ID;

#endif /* defined(_AIX) */

	/*-
	 * it can happen that in the interim of the start
	 * of the timer and this callout, a daemon has
	 * shutdown, with a call to ftd_do_sync_done().
	 * side effect of this call is like as follows.
	 * if this is already done, bp will be nil; in
	 * which case, just return.
	 */
  if (bp)
    ftd_biodone (bp);

  return;
}

/*-
 * ftd_biodone()
 *
 * called to biodone buffers returned from the driver below.  
 */
FTD_PUBLIC ftd_void_t
ftd_biodone (struct buf * bp)
{

  biodone (bp);

  return;
}

/*-
 * ftd_dev_n_open()
 *
 * return the number of open devices for all logical groups.
 */
FTD_PUBLIC ftd_int32_t
ftd_dev_n_open (ftd_ctl_t * ctlp)
{
  ftd_int32_t retval = 0;
  ftd_lg_t *lgwalk;
  ftd_dev_t *devwalk;

  for (lgwalk = ctlp->lghead; lgwalk; lgwalk = lgwalk->next)
    {
      for (devwalk = lgwalk->devhead; devwalk; devwalk = devwalk->next)
	{
	  if (devwalk->flags & FTD_DEV_OPEN)
	    retval++;
	}
    }

  return (retval);
}

/*-
 * ftd_rw()
 * 
 * raw read/write routine
 * just verify the unit number and transfer offset & length, and call
 * strategy via physio. physio(9f) will take care of address mapping
 * and locking, and will split the transfer if ncessary, based on minphys,
 * possibly calling the strategy routine multiple times.
 *
 * this routine gets called for the character device for this disk
 * drive.  things like fsck and newfs will go through this entry point
 * rather than the ftd_strategy routine.  generally, this makes no
 * difference at all, but is something to keep in mind when debugging
 * certain aspects of the system.
 */
FTD_PUBLIC ftd_int32_t
ftd_rw (dev_t dev, struct uio * uio, ftd_int32_t flag)
{
  minor_t minor = getminor (dev);

  /* test device number validity */
  if (minor & FTD_LGFLAG)
    return (EINVAL);

  /* test device number validity */
  if (ddi_get_soft_state (ftd_dev_state, U_2_S32 (minor)) == NULL) 
  {
    return (ENXIO);
  }

#if defined(SOLARIS)
  /* test offset/length validity */
  if ((uio->uio_offset & (DEV_BSIZE - 1)) ||
      (uio->uio_iov->iov_len & (DEV_BSIZE - 1)))
    {
      return (ENXIO);
    }
#endif /*#if defined(SOLARIS) || defined(HPUX) */

  return (physio (ftd_strategy, (struct buf *) 0, dev, flag,
		  minphys, uio));
}

/*-
 * ftd_finish_io()
 *
 * this routine is called when we want to finish up an I/O that has
 * completed.  dodone is true if we're supposed to biodone the user's
 * buf.
 *
 * once we finish up mybp and biodone the users' bp, we then see if we
 * have things to do (softp->qhead).  if we do, then we remove the
 * head of the queue and start that I/O.  otherwise, we place mybp
 * back on the free list.
 *
 * the purpose of this routine is to make sure that we do all of
 * these things every time we're finishing up an I/O.  since we have
 * two iodone routines that can be called, it makes sense to have the
 * common code here.
 */
FTD_PRIVATE ftd_void_t
ftd_finish_io (ftd_dev_t * softp, struct buf *bp, ftd_int32_t dodone)
{
  ftd_context_t context;
  struct buf *userbp;
  struct buf *mybp;

  /* driver and kernel buffer references */
  mybp = bp;
  userbp = BP_USER (bp);

#if defined(SOLARIS)
  /* complete PAGEIO request. map out pages mapped in by this driver. */
    /*-
     * XXX
     * hmmm. another bit of suspicious usage. it appears that we aren't 
     * using bp_mapin() and bp_mapout() consistently. look at our usage of
     * bp_mapin. seems that we aren't calling it for READs, and only some
     * WRITEs, and that here we map out indiscriminately, and regardless of 
     * whether B_REMAPPED is set. not too cool, folx.
     */
  if (bp->b_flags & B_PAGEIO)
    bp_mapout (bp);
#endif /* defined(SOLARIS) */

  if (dodone)
    {
       /*-
        * if we're acking the I/O to the upper layers, then do so now.
        * if we have an I/O delay, then wait that long before doing so do
        * help throttle the applications.  don't hold locks around calls
        * to biodone.
        */
#if defined(notdef)
      if (softp->lgp->iodelay)
	{

	    /*-
             * XXX
             * iodelay functionality is no longer supported
             * (it was a stupid idea to begin with).
             * need to deprecate all of the associated little
             * bits of it appearing in structs and other code
             * throughout the docs, and driver and UI code.
             */
	  delay (softp->lgp->iodelay);

	}
#endif /* defined(notdef) */

#if defined(HPUX) && 0
/* TMP TWS */
extern int iodoneCnt;
extern lock_t *mylock;
ACQUIRE_LOCK(mylock, context);
iodoneCnt--;
userbp->b_s8 = 0;
RELEASE_LOCK(mylock, context);
/* TMP TWS */
#endif

      ftd_biodone (userbp);
    }

  ACQUIRE_LOCK (softp->lock, context);

    /*-
     * reset the driver's buffer and
     * check whether to initiate a queued IO request,
     * or rather free up the driver's buffer.
     */
  bufzero (mybp);
  mybp->b_flags |= B_BUSY;
  if (softp->qhead)
    {
      /* process a request that was queued */
      bp = softp->qhead;
      softp->qhead = bp->av_forw;
      bp->av_forw = NULL;
      bp->av_back = NULL;
      if (!softp->qhead)
	softp->qtail = NULL;
      /* softp->lock is released as a side effect of ftd_do_io()... */
      ftd_do_io (softp, bp, mybp, context);
    }
  else
    {
      /* free the driver's buffer */
      if (softp->bufcnt > FTD_MAX_BUF)
	{
	  /* buffer was loaned from the common pool */
	  FREERBUF (mybp);
	  softp->bufcnt--;
	}
      else
	{
	  /* buffer was from this device's pool */
	  BP_PRIVATE (mybp) = softp->freelist;
	  softp->freelist = mybp;
	}
      RELEASE_LOCK (softp->lock, context);
    }

  return;
}

/*-
 * ftd_iodone_generic()
 *
 * we have the driver below us call this routine when we're doing
 * simple things.  for all reads we go through here, as well as writes
 * that we're not journalling for some reason.  this routine is
 * supposed to be fairly simple.  we get the pointer to the user's bp,
 * modify some stats, adjust the resid(ual) for the buf and then call
 * ftp_finish_io to wrap things up.
 */
FTD_PRIVATE ftd_int32_t
ftd_iodone_generic (struct buf * bp)
{
  ftd_dev_t *softp;
  struct buf *userbp;
  ftd_int32_t dodone = 1;
  ftd_lg_t *lgp;
  ftd_context_t context;

  userbp = BP_USER (bp);

  /* bp -> device state */
  if (!(softp = ftd_get_softp (bp)))
    {
      return ENXIO;
    }

  /* get logical group */
  lgp = softp->lgp;

  /* update per dev stats */
  if (bp->b_flags & B_READ)
    {
      softp->readiocnt++;
      softp->sectorsread += (bp->b_bcount >> DEV_BSHIFT);
    }
  else
    {
      softp->writeiocnt++;
      softp->sectorswritten += (bp->b_bcount >> DEV_BSHIFT);
    }

    /*-
     * XXX 
     * hmmm. this call is suspicious. 
     * occurring as it does before the call below to
     * ftd_finish_io, on a real error it seems that
     * this could result in two calls to ftd_biodone.
     * TWS
     * fix the above suspicion by deprecating ftd_get_error (bp) and replace with
     * the code below
     */
    /* complete the IO, according to error states and current driver mode */
    ACQUIRE_LOCK (lgp->lock, context);

/* TWS: */
#ifdef INJECTERR
if (injectErr)
   bioerror (bp, ENOENT);
#endif
/* TWS */

    if (geterror (bp))
    {
      bioerror (userbp, geterror (bp));
    }

#if defined(SOLARIS)
    /*-
     * we need to add the number of residual bytes to the users' residual
     * byte cound is correct in the case when we've adjusted the request
     * in the case of end of media.  see ftd_strategy routine for where
     * we do this.
     */
     userbp->b_resid += bp->b_resid;
#endif

    RELEASE_LOCK (lgp->lock, context);

  ftd_finish_io (softp, bp, dodone);

  return (0);
}

/*-
 * ftd_iodone_journal()
 *
 * for all journalled writes, we come through this routine when they
 * finish.  once they finish, we commit the data in the bab so the pmd
 * can see it.  if we're not in sync mode, then we'll cause the users'
 * bp to be acknowledged to the upper layers when we finally call
 * ftd_finish_io.
 */
FTD_PRIVATE ftd_int32_t
ftd_iodone_journal (struct buf * bp)
{
  ftd_int32_t length;
  wlheader_t *hp;
  ftd_lg_t *lgp;
  bab_mgr_t *mgr;
  ftd_dev_t *softp;
  ftd_int32_t dowakeup;
  ftd_int32_t dodone = 1;
  struct buf *userbp, *mybp;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

  /* kernel and driver buffer references */
  mybp = bp;

  /* write log header, group, bab mgr references */
  hp = (wlheader_t *) BP_WLHDR (mybp);
  if (hp->majicnum != DATASTAR_MAJIC_NUM)
       FTD_ERR(FTD_WRNLVL, "ftd_iodone_journal: BAB DATASTAR_MAJIC_NUM");
  lgp = (ftd_lg_t *) hp->group_ptr;
  mgr = lgp->mgr;

  ACQUIRE_LOCK (lgp->lock, context); 

  /* test state consistency. */
  userbp = (struct buf *)hp->bp;
  if (userbp == NULL)
    {
      panic ("FTD: BAB entry corrupt");
    }

  /* bp -> device state */
  if (!(softp = ftd_get_softp (bp)))
    {
      RELEASE_LOCK (lgp->lock, context);
      return ENXIO;
    }

#if defined(SOLARIS)
  /* see comment in ftd_iodone_generic for why we do this */
  userbp->b_resid += mybp->b_resid;
#endif

  /* header state transistion. pending -> complete */
  hp->bp = NULL;
  if(mybp->b_resid > 0) 
     FTD_ERR(FTD_WRNLVL, "iodone_journal:Error Incomplete I/O: %d", mybp->b_resid ); 
  hp->length -= (mybp->b_resid >> DEV_BSHIFT);
  hp->complete = 1;
  mgr->pending_io--;
  /* whether to wake the daemon */
  dowakeup = 0;

  /* update disk stats */
  softp->writeiocnt++;
  softp->sectorswritten += (mybp->b_bcount >> DEV_BSHIFT);
  softp->wlentries++;
  softp->wlsectors += hp->length;

  /* update group stats */
  lgp->wlentries++;
  lgp->wlsectors += hp->length;

  /* complete the IO, according to error states and current driver mode */
/* TWS */
#ifdef INJECTERR
if (injectErr)
   bioerror (bp, ENOENT);
#endif
/* TWS */

  if (geterror (bp))
    {
      bioerror (userbp, geterror (bp));
    }
  else if (!LG_DO_SYNCMODE (lgp))
    {
      /* asynch mode */
      hp->bp = NULL;
      hp->timoid = INVALID_TIMEOUT_ID;
      dodone = 1;
    }
  else
    {
      /* synch mode */
#if defined(HPUX)
      /* timestamp & enqueue wlheader */
      hp->bp = userbp;
      if (lgp->sync_timeout > 0)
	{
	  ftd_timer_enqueue (lgp, hp);
	}
#elif defined(SOLARIS)
      /* register a timeout(9f) to bound sync mode ack */
      hp->bp = (ftd_uint64ptr_t) userbp;
      if (lgp->sync_timeout > 0)
	hp->timoid = (ftd_uint64_t) timeout (ftd_synctimo, (char *)hp,
			      drv_usectohz (lgp->sync_timeout * 1000000));
#elif defined(_AIX)
      /* register a timeout(9f) to bound sync mode ack */
      hp->bp = userbp;
      if (lgp->sync_timeout > 0)
	hp->timoid = ftd_timeout (ftd_synctimo, (caddr_t) hp,
				  (HZ * lgp->sync_timeout));
#endif
      dodone = 0;
    }


    /*-
     * if this is the first pending request, commit it.
     * the bab keeps two head pointers.  one is to the last I/O to 
     * complete, and the other is to the last I/O to have started (well,
     * to have allocated space, which should almost be the same 
     * thing, in the absense of benign races).  here we commit the memory
     * as long as we find headers that are complete.  sanity code
     * could be added here (to check to make sure the length isn't too long,
     * or that the magic numbers are in place, etc).
     */
  if (ftd_bab_get_pending (mgr) == (ftd_uint64_t *) hp)
    {
      if(hp->majicnum != DATASTAR_MAJIC_NUM)
        {
          if(mgr->pending_io == 0)
          {
            FTD_ERR(FTD_WRNLVL, "Moving Pending#1"); 
            mgr->pending_buf = mgr->in_use_tail;
            mgr->pending     = mgr->pending_buf->alloc;
            dowakeup=1;
            goto manipulate_pending;
          }
         else
           FTD_ERR(FTD_WRNLVL, "ftd_iodone_journal: BAB DATASTAR_MAJIC_NUM#1");                } 
      length = FTD_WLH_QUADS (hp);
      ftd_bab_commit_memory (mgr, length);

      /* commit any completed requests past this one! */
      while ((hp = (wlheader_t *) ftd_bab_get_pending (mgr)) != NULL)
	{
          if (hp->majicnum != DATASTAR_MAJIC_NUM)
        {
          if(mgr->pending_io == 0)
          {
            FTD_ERR(FTD_WRNLVL, "Moving Pending#2");
            mgr->pending_buf = mgr->in_use_tail;
            mgr->pending     = mgr->pending_buf->alloc;
            dowakeup=1;
            goto manipulate_pending;
          }
         else
          FTD_ERR(FTD_WRNLVL, "ftd_iodone_journal: BAB DATASTAR_MAJIC_NUM#2");
        }

	  if (hp->complete == 1)
	    {
	      length = FTD_WLH_QUADS (hp);
	      ftd_bab_commit_memory (mgr, length);
	    }
	  else
	    {
	      break;
	    }
	}

      dowakeup = 1;

    }
manipulate_pending:
  RELEASE_LOCK (lgp->lock, context);

  ftd_finish_io (softp, bp, dodone);

    /*-
     * we do the wakeup last to try to avoid some races with finish I/O
     * and to give the upper layers a chance to complete.  it was moved
     * here as part of the big bug hunt which ultimately was caused by 
     * the improper unmapping of I/O requests.
     */
  if (dowakeup)
    ftd_wakeup (lgp);

  return (0);
}

/*-
 * ftd_do_read()
 *
 * process a read to the device.  setup things so that we can call our
 * bdev_strategy with the buf and have it do the right thing.
 */
FTD_PRIVATE ftd_void_t
ftd_do_read (ftd_dev_t * softp, struct buf * mybp)
{
  struct buf *passdownbp = mybp;

  /* reads are trivial... */
  passdownbp->b_iodone = BIOD_CAST (ftd_iodone_generic);
  BP_DEV (passdownbp) = softp->localbdisk;

  return;
}

/*-
 * ftd_do_write()
 *
 * copy the contents of the buffer to our memory log and/or manage the
 * high resolution and low resolution dirty bits.
 */
FTD_PRIVATE ftd_void_t
ftd_do_write (ftd_dev_t * softp, struct buf * userbp, struct buf * mybp)
{
  wlheader_t *hp;
  ftd_uint64_t *buf, *temp;
  ftd_int32_t size64, i,offset;
  ftd_lg_t *lgp;
  struct buf *passdownbp = mybp;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

  lgp = softp->lgp;

  passdownbp->b_iodone = BIOD_CAST (ftd_iodone_generic);
  BP_DEV (passdownbp) = softp->localbdisk;

  /* bab_mgr always sees the world as 64-bit words! */
  /* XXX This code should use the FTD_WLH_QUADS() macro XXX */
  size64 = sizeof_64bit (wlheader_t) + (userbp->b_bcount / 8);
if (userbp->b_bcount & (DEV_BSIZE - 1))
  FTD_ERR(FTD_WRNLVL, "Not Multiple of DEV_BSIZE");
  ACQUIRE_LOCK (lgp->lock, context);

    /*-
     * this is where journal memory is allocated,
     * and buffer data is copied to it. if the
     * allocation fails, a driver state transistion
     * is triggered, FTD_M_JNLUPDATE -> FTD_M_TRACKING,
     * or FTD_M_REFRESH -> FTD_M_TRACKING.
     *
     * XXX
     * this is a rotten bit of state machinery, but as it's
     * the beloved heart and soul of the product, its likely
     * to remain as is. 
     */
  if ((lgp->state & FTD_M_JNLUPDATE) &&
      ((ftd_bab_alloc_memory (lgp->mgr, size64)) != 0))
    {
      temp = lgp->mgr->from[0] - sizeof_64bit(wlheader_t);
      hp = (wlheader_t *) temp;
      hp->complete = 0;
      hp->bp = NULL;

      /* add a header and some data to the memory buffer */
      hp->majicnum = DATASTAR_MAJIC_NUM;
      hp->offset = userbp->b_blkno;
      hp->length = userbp->b_bcount >> DEV_BSHIFT;
      hp->span = lgp->mgr->num_frags;
      hp->dev = softp->cdev;
#if defined(SOLARIS) && (SYSVERS >= 570)
      /*
       * Solaris 5.7/5.8:
       * convert from internal to external
       * representation of dev_t.
       * this field is of interest only externally.
       */
      hp->diskdev = DEVCMPL(softp->localbdisk);
#else  /* defined(SOLARIS) && (SYSVERS >= 570) */
      hp->diskdev = softp->localbdisk;
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */

#if defined(SOLARIS) 
      hp->group_ptr = (ftd_uint64ptr_t)lgp;
#else  /* defined(SOLARIS) */
      hp->group_ptr = lgp;
#endif /* defined(SOLARIS) */
      hp->flags = 0;

      /* now copy the data */

	/*-
         * B_PHYS is used for raw writes to the disk from a user buffer.
         * this normally saves the copyin cost.  however, since we need
         * to save a copy of it, that doesn't work in this case.  the reason
         * we have the if rather than just always using copyin is that
         * you can only call copyin from a "user" context and we often get
         * called, on some platforms, from an interrupt context.  as such
         * there is no proc for the copyin routine to use to copy the data
         * and we die.
         */
#ifdef SOLARIS
	/*-
         * map in the pages, if needed.  we can only call mapin/out from 
         * a user context, that's why we map it in here.  however, upper 
         * layers may have already mapped it in, so we don't map it out
         * afterwards.  if you map it back in, wierd panics will happen
         * with vxfs over ftd over vxvm.
         */
      if (userbp->b_flags & B_PAGEIO)
	bp_mapin (userbp);
#endif /* SOLARIS */

#if defined(_AIX)
      {
	ftd_int32_t rc;

	/*- 
	 *  _AIXism. assures that there is a valid kernel VA
	 *  for the data during later processing in the driver's
	 *  `bottom' half.
	 */
	if ((rc = xmemin (userbp->b_un.b_addr,
			  (caddr_t) (buf + sizeof_64bit (wlheader_t)),
			  userbp->b_bcount,
			  &userbp->b_xmemd)) != XMEM_SUCC)
	  {
	    FTD_ERR (FTD_WRNLVL, "ftd_do_write: xmemin failed. errno: %d", rc);
	  }
	assert (rc == XMEM_SUCC);
      }
#elif defined(SOLARIS)
      { 
        offset = 0;
        if (userbp->b_flags & B_PHYS) {
          for(i=0;i< lgp->mgr->num_frags; i++) {
            copyin(userbp->b_un.b_addr+offset, (caddr_t)(lgp->mgr->from[i]), lgp->mgr->count[i]);
            offset += lgp->mgr->count[i];
          } /*end for */
        } else {
           for(i=0;i< lgp->mgr->num_frags; i++) {
            bcopy(userbp->b_un.b_addr+offset, (caddr_t)(lgp->mgr->from[i]), lgp->mgr->count[i]);
            offset += lgp->mgr->count[i];
           } /*end for */
        }
      }
#elif defined(HPUX)
      {
	space_t to_sid, from_sid;

        offset=0;
        if (userbp->b_flags & B_PHYS) {
          for(i=0; i<lgp->mgr->num_frags; i++) {
            copyin(userbp->b_un.b_addr+offset, (caddr_t)(lgp->mgr->from[i]),lgp->mgr->count[i]);
            offset += lgp->mgr->count[i];
          } /*end for*/
        } else {
              for(i=0;i< lgp->mgr->num_frags; i++) {
                from_sid = userbp->b_spaddr;
                to_sid = ldsid(lgp->mgr->from[i]);
                privlbcopy(from_sid, (caddr_t) userbp->b_un.b_addr+offset,
                           to_sid, (caddr_t) (lgp->mgr->from[i]),lgp->mgr->count[i]);
                offset += lgp->mgr->count[i];
              } /*end for */
        }
      }
#endif /* defined(_AIX) */
      /* auto-intra-mutual structure references */
      BP_WLHDR (mybp) = hp;
#if defined(SOLARIS)
      hp->bp = (ftd_uint64ptr_t)userbp;
#else  /* defined(SOLARIS) */
      hp->bp = userbp;
#endif /* defined(SOLARIS) */

	/*-
         * we use the iodone function that we set before to figure out
         * what to do now.
         */
      lgp->mgr->pending_io++;
      passdownbp->b_iodone = BIOD_CAST (ftd_iodone_journal);
      RELEASE_LOCK(lgp->lock, context);

    }
  else if (lgp->state == FTD_MODE_NORMAL)
    {

      /* we just overflowed the journal */
      FTD_ERR (FTD_WRNLVL, "%s: Transitioning to Tracking mode.", DRIVERNAME);

      ftd_clear_hrdb (lgp);

      lgp->state = FTD_MODE_TRACKING;

      RELEASE_LOCK (lgp->lock, context);

    }
  else if (lgp->state == FTD_MODE_REFRESH)
    {

	/*-
         * we just overflowed the journal again.   since we've been keeping
         * track of the dirty bits all along, there is little we need to
         * do here except flush the bab.  we can't recompute the dirty bits
         * here because entries may have been migrated out of the bab
         * in the interrum and we'd have no way of knowing.
         */

	/*- 
	 * XXX 
	 * WARNING don't we want to clear the bab here because we
	 * know it is dirty?  yes, in theory we do, but we don't want to
	 * step on the PMD's world view, so we let it do it when it thinks
	 * we've gone down this code path 
	 */
      FTD_ERR (FTD_WRNLVL, "%s: Transitioning to Tracking mode.", DRIVERNAME);

      lgp->state = FTD_MODE_TRACKING;

      RELEASE_LOCK (lgp->lock, context);

    }
  else
    {
	/*- 
         * XXX 
         * this `state' transistion is particularly rotten, 
         * as it suggests we're in an unknown state and that's
         * ok. seems this would be a great place to panic().
         */
      RELEASE_LOCK (lgp->lock, context);
    }

  /*
   * update the hrdb in overflow and recover modes
   */
   if (lgp->state & FTD_M_BITUPDATE)
    ftd_update_hrdb (softp, userbp);
    
  return;
}

/*-
 * ftd_do_io()
 * 
 * MUST NOT CALL THIS ROUTINE with softp->freelist == NULL.
 * MUST CALL THIS ROUTINE with softp->lock held, since we unlock
 * it to avoid deadlock before calling bdev_strategy.
 *
 * we schedule an I/O to the lower layers here.  we call the setup
 * routine (misnamed do_{read,write}) and then call the undocumented
 * bdev_strategy.  this routine looks up the major number of the reuqest
 * and causes its strategy routine to be called with the buf that we pass
 * it (in this case mybp).  while the use of an undocumented function 
 * seems dangerous, it is actually one of the simpler functions which
 * would be easy to replace should it go away.
 */
FTD_PRIVATE ftd_int32_t
ftd_do_io (ftd_dev_t * softp, struct buf * userbp, struct buf * mybp, ftd_context_t context)
{
#if defined(HPUX) && 0
/* TMP TWS */
extern int iodoneCnt;
/* TMP TWS */
#endif
  mybp = ftd_bioclone (softp, userbp, mybp);
  RELEASE_LOCK (softp->lock, context);

  if (userbp->b_flags & B_READ)
    {
      ftd_do_read (softp, mybp);
    }
  else
    {
      ftd_do_write (softp, userbp, mybp);
    }

#if defined(HPUX) && 0
/* TMP: TWS */
ACQUIRE_LOCK(mylock, context);
iodoneCnt++;
RELEASE_LOCK(mylock, context);
/* TMP TWS */
#endif
  /* pass the request on to lower level */
  return (BDEVSTRAT (mybp));
}

/*-
 * ftd_strategy()
 *
 * the strategy routine.  all block I/O comes through here.  this will either
 * be called by the buffer cache to satisfy a block read/write request, or
 * will be indirectly called from the read(8e) or write(9e) entry points
 * via the physio(9f) calls they make.  physio makes sure that these requests
 * are less than maxphys.  however, the buffer cache no longer ensures that
 * requests will be less than maxphys, so we need to do some sanity checking
 * to make sure that the request isn't too big.  this checking is mostly
 * historical, since it is quite likely that we could handle larger 
 * requests.  however, it is in place and does short circuit the error
 * handling code that would otherwise have to work perfectly when the layers
 * below us couldn't deal.  and that is a whole can of warms that this
 * driver doesn't deal well with: errors after we've placed something into
 * the bab.
 */
FTD_PUBLIC ftd_int32_t
ftd_strategy (struct buf * userbp)
{
  ftd_dev_t *softp;
  ftd_uint32_t numblocks;
  ftd_context_t context;
  minor_t minor;

  Enter (int, FTD_STRATEGY, (ftd_int32_t) userbp, 0, 0, 0, 0);

  /* validate device number */
  minor = getminor (BP_DEV (userbp));
  if (minor & FTD_LGFLAG)
    Return (EINVAL);

  if ((softp = ddi_get_soft_state (ftd_dev_state, U_2_S32 (minor))) == NULL)
    {
      bioerror (userbp, ENXIO);
      biodone (userbp);
      Return (0);
    }

  /* validate device block number */
  if (userbp->b_blkno >= softp->disksize)
    {
      bioerror (userbp, ENXIO);
      biodone (userbp);
      Return (0);
    }

  userbp->b_resid = 0;

  /* truncate this request if it spans the end of the volume */
  numblocks = userbp->b_bcount >> DEV_BSHIFT;
  if ((userbp->b_blkno + numblocks) > softp->disksize)
    {
      numblocks = softp->disksize - userbp->b_blkno;
      userbp->b_resid = userbp->b_bcount - (numblocks << DEV_BSHIFT);
      userbp->b_bcount = (numblocks << DEV_BSHIFT);
    }

  /* if transfer length is 0, this call is a noop */
  if (userbp->b_bcount == 0)
    {
      userbp->b_resid = userbp->b_bcount;
      biodone (userbp);
      Return (0);
    }

  ACQUIRE_LOCK (softp->lock, context);

 /* update and flush LRT if new dirty bits */
  if (ftd_update_lrdb (softp, userbp))
    {
      ftd_flush_lrdb (softp);
    }

    /*-
     * if no driver buffers available, or queued requests, 
     * enqueue this request for deferred processing.
     * requests are enqueued here behind any other queued
     * requests to preserve the write ordering in the
     * journal.
     * 
     * XXX
     * this queueing mechanism is suspected of leading to
     * SOLARIS and HPUX application hangs. such hangs are
     * avoided for now with a hack making the driver's buffer 
     * pool absurdly large, thus avoiding deferred processing.
     * it is as yet unknown whether the hang's result from
     * a logic problem with our queueing mechanism, or from
     * the fact that the deferred processing is done in 
     * a context different than this, the original context.
     * needs work in either case.
     */
  /* first get a lock so that nothing changes after we make our decision */
  if (softp->freelist == NULL || softp->qhead)
    {
      if (softp->qtail)
	softp->qtail->av_forw = userbp;
      userbp->b_flags |= B_BUSY;
      userbp->av_forw = NULL;
      softp->qtail = userbp;
      if (!softp->qhead)
	{
	  softp->qhead = userbp;
	}
      RELEASE_LOCK (softp->lock, context);
    }
  else
    {
      return (ftd_do_io (softp, userbp, NULL, context));
    }

#if defined(HPUX)
  Return (-16);
#else
  Return (0);
#endif
  /* NOTREACHED */
  Exit;
}

/*-
 * ftd_flush_lrdb()
 *
 * this routine will use the lrdbbp that we setup at creation time to
 * flush the low res dirty bits out to disk.  we then wait for the
 * operation to finish and then return any errors that happened during
 * this operation.  this routine assumes that lrdbbp is setup and does
 * nothing to make sure it is valid.  we always write the whole
 * lrdb.
 *
 * if we crash during this operation, then that is OK assuming we have
 * no data corruption.  if we fail to write this bit to the disk, then that
 * is OK because we do that operation syncronously before we allow the
 * operation to happen.  if we do write and then crash before the operation
 * can complete, that is OK too because smart refresh will not transfer the
 * identical blocks.
 */
FTD_PUBLIC ftd_int32_t
ftd_flush_lrdb (ftd_dev_t * softp)
{
  
  if (softp->lgp->persistent_store)
    {
      
     
	/*-
	 * XXX
	 * LRDB_FREE_SYNCH() hack:
	 * there are a number of unresolved problems related
	 * to pstore IO. the gravest of them is that the lrdb
	 * buffer header is a per device resource (more properly
	 * a buffer should be allocated as needed, here, on a
	 * per request basis.). as such, it was discovered that
	 * it is quite possible for independent IO requests to
	 * be concurrently sharing the buffer (ouch).
	 * the hack is to synchronize access to the lrdb buffer,
	 * when it is needed and found busy, its wanted state
	 * is noted, and this routine returns without initiating
	 * the IO.  when the busy IO completes, the IO is 
	 * restarted in biodone_psdev. 
	 * there is a finite window here for an IO request to
	 * be initiated without corresponding LRT map bits set.
	 */
#if defined(LRDBSYNCH)
      LRDB_FREE_SYNCH (softp);
#endif /* defined(LRDBSYNCH) */
  softp->lrdb_initcnt++;

#if defined(_AIX)

      softp->lrdbbp->b_forw =
	softp->lrdbbp->b_back =
	softp->lrdbbp->av_forw =
	softp->lrdbbp->av_back = 0;
      softp->lrdbbp->b_work =
	softp->lrdbbp->b_resid =
	softp->lrdbbp->b_flags = 0;
      softp->lrdbbp->b_flags |= B_BUSY | B_DONTUNPIN | B_NOHIDE;
      if (clonebp) {
        softp->lrdbbp->b_iodone = biodone_psdev;
      } else {
        softp->lrdbbp->b_iodone = 0;
      }
      softp->lrdbbp->b_event = EVENT_NULL;
      softp->lrdbbp->b_xmemd.aspace_id = XMEM_GLOBAL;

#elif defined(HPUX)

      softp->lrdbbp->b_iodone = BIOD_CAST (biodone_psdev);
      softp->lrdbbp->b_flags = B_CALL | B_WRITE | B_BUSY | B_ASYNC;

#elif defined(SOLARIS)

      softp->lrdbbp->b_forw =
	softp->lrdbbp->b_back =
	softp->lrdbbp->av_forw =
	softp->lrdbbp->av_back = 0;
      softp->lrdbbp->b_resid =
	softp->lrdbbp->b_flags = 0;
      softp->lrdbbp->b_flags |= B_KERNBUF | B_WRITE | B_BUSY;
      softp->lrdbbp->b_iodone = BIOD_CAST (biodone_psdev);
  

#endif

     BDEVSTRAT (softp->lrdbbp);
  }
return (0);
}

/*-
 * ftd_dev_close()
 * 
 * close the (logical) device.  called when all references to this device go
 * away.
 */
FTD_PUBLIC ftd_int32_t
ftd_dev_close (dev_t dev)
{
  ftd_dev_t *softp;
  minor_t minor = getminor (dev);

  if ((softp = ddi_get_soft_state (ftd_dev_state, U_2_S32 (minor))) == NULL)
  {
    return (ENXIO);
  }

  softp->flags &= ~FTD_DEV_OPEN;

    /*-
     * ok, save state if we're the last thing to close
     * in this logical group and the lgp is closed.  we don't
     * do this now because we do it in userland and have pessimal, but
     * correct crash recovery operations.
     */
  return 0;
}

/*-
 * ftd_dev_open()
 *
 * open the (logical) device.  gets called once per open(2) call.
 *
 * XXX 
 * it appears that we don't check the exclusive flag passed to us,
 * and I think we should.  not sure what the consequences of that
 * are.
 */
FTD_PUBLIC ftd_int32_t
ftd_dev_open (dev_t dev)
{
  ftd_dev_t *softp;
  minor_t minor = getminor (dev);

  if ((softp = ddi_get_soft_state (ftd_dev_state, U_2_S32 (minor))) == NULL)
    return (ENXIO);		/* invalid minor number */

  /* if stop has been initiated on this group, don't allow the open */
  if (softp->flags & FTD_STOP_IN_PROGRESS)
    {
      return (EBUSY);
    }

  softp->flags |= FTD_DEV_OPEN;

  return 0;
}

/*-
 * ftd_del_device()
 *
 * delete the (logical) device.  
 * we get called when the FTD_DEL_DEVICE call has validated its 
 * arguments and wants to actually delete the device.
 *
 * here we should close down the device that we have opened that is the
 * data device.  we then detach from the logical group and cleanup
 * all memory for this device.
 */
FTD_PUBLIC ftd_void_t
ftd_del_device (ftd_dev_t * softp, minor_t minor)
{
  ftd_dev_t **link;
  struct buf *freelist;
  struct buf *buf;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

  ACQUIRE_LOCK (softp->lock, context);

#if defined(HPUX)

#if defined(notdef)
  /* XXX fixme. */
  bdev_close (softp->localbdisk);
#endif /* defined(notdef) */

#elif defined(SOLARIS)

  ftd_layered_close (softp->ld_dop, softp->localbdisk);

#elif defined(_AIX)

  fp_close (softp->dev_fp);

#endif /* defined(HPUX) */

  /* dispose of the device's buffer pool */
  freelist = softp->freelist;
  while (freelist)
    {
      buf = freelist;
      freelist = BP_PRIVATE (freelist);
      FREERBUF (buf);
    }

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

  kmem_free (softp->statbuf, softp->statsize);

  /* waste the bitmaps */
  kmem_free (softp->lrdb.map, softp->lrdb.len32 * 4);
  kmem_free (softp->hrdb.map, softp->hrdb.len32 * 4);

#if defined(_AIX)
  FREERBUF (softp->lrdbbp);
#endif /* defined(_AIX) */

  RELEASE_LOCK (softp->lock, context);

  DEALLOC_LOCK (softp->lock);

  ddi_soft_state_free (ftd_dev_state, minor);

  return;
}

/*-
 * ftd_do_sync_done()
 *
 * call biodone on all of the completed entries in the bab because
 * something with sync mode changed.  this sometimes causes the writes 
 * to complete sooner than they would have otherwise, but that happens
 * only when when you change the sync depth, which isn't all that
 * frequent, and at most (old_sync_depth - new_sync_depth) entries
 * are prematurely biodone'd.
 *
 * you must hold the lgp->context lock when you call this routine.
 */
FTD_PUBLIC ftd_void_t
ftd_do_sync_done (ftd_lg_t * lgp)
{
  ftd_uint32_t bytes;
  ftd_uint64_t *temp;
  wlheader_t *wl;
  bab_buffer_t *buf;
  ftd_int32_t sv_sync_depth = lgp->sync_depth;
  ftd_int32_t len64, nlen64, i;

  if (lgp->mgr == NULL ||
      lgp->mgr->in_use_head == NULL)
    return;

    /*-
     * walk through the entries being migrated, calling biodone(hp->bp)
     * if hp->bp is not NULL.
     */

  /* suspend sync mode IO */
  lgp->sync_depth = (ftd_uint32_t) - 1;

  bytes = 0x7fffffff;
  BAB_MGR_FIRST_BLOCK (lgp->mgr, buf, temp);
  while (temp && bytes > 0)
    {

      wl = (wlheader_t *) temp;

      if (wl->majicnum != DATASTAR_MAJIC_NUM)
	break;

      /* cancel any pending timeouts */
#if defined(SOLARIS)
      if (wl->timoid != INVALID_TIMEOUT_ID)
	{
#if (SYSVERS >= 570)
          timeout_id_t passtimoid=(timeout_id_t)wl->timoid;
#else  /* (SYSVERS >= 570) */
          untimeout (wl->timoid);
#endif /* (SYSVERS >= 570) */
	  wl->timoid = INVALID_TIMEOUT_ID;
	}
#elif defined(_AIX)
      if (wl->timoid != INVALID_TIMEOUT_ID)
	{
	  ftd_aixuntimeout (wl->timoid);
	  wl->timoid = INVALID_TIMEOUT_ID;
	}
#elif defined(HPUX)

/* XXX fixme. */
#if defined(notdef)
      if (wl->timoid != INVALID_TIMEOUT_ID)
	{
	  untimeout (ftd_synctimo, wl);
	  wl->timoid = INVALID_TIMEOUT_ID;
	}
#endif /* defined(notdef) */

#endif /* defined(SOLARIS) */

      if (wl->complete && wl->bp)
	{
	  struct buf *passbp=(struct buf *)wl->bp;
	  ftd_biodone (passbp);
	  wl->bp = 0;
	}

      bytes -= FTD_WLH_BYTES (wl);
      /*BAB_MGR_NEXT_BLOCK (temp, wl, buf);*/
/* + */
        if(wl->span > 1){
           len64 = buf->alloc - (temp + sizeof_64bit(wlheader_t));
           nlen64 = FTD_LEN_QUADS(wl) - len64;
           for(i=2; i< (wl->span); i++)
           {
            buf = buf->next;
            nlen64 -= buf->alloc - buf->start;
           }
           if((buf = buf->next) == NULL) temp=NULL;
           else temp = buf->start + nlen64;
        } else {
           temp += BAB_WL_LEN(wl);
           if(temp == buf->alloc) {
              if((buf=buf->next) == NULL) temp=NULL;
              else temp = buf->free;
           }
        }  
/* - */    
    }

  /* restore sync mode depth */
  lgp->sync_depth = sv_sync_depth;

  return;
}

/*-
 * ftd_clear_dirtybits()
 *
 * simple routine that clears all the dirtybits in memory.  it is up to the 
 * callers of this routine to arrange for that state to be flushed to disk.
 */
FTD_PUBLIC ftd_void_t
ftd_clear_dirtybits (ftd_lg_t * lgp)
{
  ftd_dev_t *tempdev;

  tempdev = lgp->devhead;
  while (tempdev)
    {
      bzero ((caddr_t) tempdev->lrdb.map, tempdev->lrdb.len32 * 4);
      bzero ((caddr_t) tempdev->hrdb.map, tempdev->hrdb.len32 * 4);
      tempdev = tempdev->next;
    }

  return;
}

/*-
 * ftd_set_dirtybits()
 * 
 * set all the dirty bits to 1.  callers must make sure that this
 * state is updated on disk, where appropriate.
 */
FTD_PUBLIC ftd_void_t
ftd_set_dirtybits (ftd_lg_t * lgp)
{
  ftd_dev_t *tempdev;

  tempdev = lgp->devhead;
  while (tempdev)
    {
      ftd_memset ((caddr_t) tempdev->lrdb.map, 0xff, tempdev->lrdb.len32 * 4);
      ftd_memset ((caddr_t) tempdev->hrdb.map, 0xff, tempdev->hrdb.len32 * 4);
      tempdev = tempdev->next;
    }

  return;
}

/*-
 * ftd_clear_hrdb()
 *
 * simple routine that clears all the high res tracking bits in memory.
 * no disk operation is needed because we don't store these in the
 * persistant store automatically.
 */
FTD_PRIVATE ftd_void_t
ftd_clear_hrdb (ftd_lg_t * lgp)
{
  ftd_dev_t *tempdev;

  tempdev = lgp->devhead;
  while (tempdev)
    {
      bzero ((caddr_t) tempdev->hrdb.map, tempdev->hrdb.len32 * 4);
      tempdev = tempdev->next;
    }

  return;
}

/* LG device routines: */

/*-
 * ftd_lg_close()
 *
 * close logical group.  this usually means that the pmd has
 * gone away.
 */
FTD_PUBLIC ftd_int32_t
ftd_lg_close (dev_t dev)
{
  ftd_lg_t *lgp;
  minor_t minor = getminor (dev) & ~FTD_LGFLAG;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

  if ((lgp = ddi_get_soft_state (ftd_lg_state, U_2_S32 (minor))) == NULL)
    return (ENXIO);		/* invalid minor number */

  ACQUIRE_LOCK (lgp->lock, context);
  ftd_do_sync_done (lgp);
  lgp->flags &= ~FTD_LG_OPEN;
  RELEASE_LOCK (lgp->lock, context);

  ACQUIRE_LOCK (lgp->ctlp->lock, context);
  lgp->ctlp->lg_open--;
  RELEASE_LOCK (lgp->ctlp->lock, context);

  return 0;
}

/*-
 * ftd_lg_open()
 *
 * open the logical group control device.  we allow only one open at a time
 * as a way of enforcing in the kernel that there is only one pmd per 
 * logical group at any given time.
 */
FTD_PUBLIC ftd_int32_t
ftd_lg_open (dev_t dev)
{
  ftd_lg_t *lgp;
  minor_t minor = getminor (dev) & ~FTD_LGFLAG;
#if defined(HPUX)
  ftd_context_t context;
  extern ftd_int32_t ftd_debug;

#endif /* defined(HPUX) */

  if ((lgp = ddi_get_soft_state (ftd_lg_state, U_2_S32 (minor))) == NULL)
    {
      return (ENXIO);		/* invalid minor number */
    }

  ACQUIRE_LOCK (lgp->lock, context);
  if (lgp->flags & FTD_LG_OPEN)
    {
      RELEASE_LOCK (lgp->lock, context);
      return EBUSY;
    }
  lgp->flags |= FTD_LG_OPEN;
  RELEASE_LOCK (lgp->lock, context);

  ACQUIRE_LOCK (lgp->ctlp->lock, context);
  lgp->ctlp->lg_open++;
  RELEASE_LOCK (lgp->ctlp->lock, context);

  return 0;
}

/*-
 * ftd_close_persistent_store()
 *
 * called upon logical group shutdown.
 */
FTD_PRIVATE ftd_int32_t
ftd_close_persistent_store (ftd_lg_t * lgp)
{
#if defined(HPUX)
  dev_t psbdisk, pscdisk;
  drv_info_t *drv_info;
#endif

#if defined(HPUX)

  drv_info = cdev_drv_info (lgp->persistent_store);
  psbdisk = makedev (drv_info->b_major, minor (lgp->persistent_store));
  pscdisk = makedev (drv_info->c_major, minor (lgp->persistent_store));

/* XXX fixme. */
#if defined(notdef)
  cdev_close (pscdisk);
  bdev_close (psbdisk);
#endif /* defined(notdef) */

#elif defined(SOLARIS)

  ftd_layered_close (lgp->ps_do, lgp->persistent_store);

#elif defined(_AIX)

/* XXX fixme. */
#if defined(notdef)
  if (lgp->ps_fp)
    {
      fp_close (lgp->ps_fp);
      lgp->ps_fp = 0;
    }
#endif /* defined(notdef) */

#endif /* defined(HPUX) */

  return 0;
}

/*-
 * ftd_del_lg()
 *
 * called from the FTD_DEL_LG ioctl, this routine will delete any devices
 * still associated with the logical group and then remove this logical
 * group from our state structures.  memory is then freed.
 */
FTD_PUBLIC ftd_void_t
ftd_del_lg (ftd_lg_t * lgp, minor_t minor)
{
  ftd_lg_t **link;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

  ACQUIRE_LOCK (lgp->lock, context);

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

  /* free any devices */
  while (lgp->devhead)
    ftd_del_device (lgp->devhead, getminor (lgp->devhead->cdev));

  /* close the pstore */
  ftd_close_persistent_store (lgp);

  RELEASE_LOCK (lgp->lock, context);

  DEALLOC_LOCK (lgp->lock);

  kmem_free (lgp->statbuf, lgp->statsize);

  /* waste the bab manager */
  ftd_bab_free_mgr (lgp->mgr);

  ddi_soft_state_free (ftd_lg_state, minor);

  return;
}

/* control device routines: */

/*-
 * ftd_ctl_close()
 *
 * close an open control device.  we have no state assocaited with
 * opens of this device, so when we close it we do nothing.  it exists
 * to be permissive about us getting data to/from ftd.  this routine
 * is only called on the LAST close, not for every close if multiple
 * people have the device open.
 */
FTD_PUBLIC ftd_int32_t
ftd_ctl_close ()
{

  if (ftd_global_state == NULL)
    return (ENXIO);		/* invalid minor number */

    /*-
     * since we don't change state in the open routine, I don't
     * think we need to do anything here
     */

  return 0;
}

/*-
 * ftd_ctl_open()
 *
 * open the control device.  we get called for every open.  except
 * when we have no state at all (which is impossible), we return
 * success.
 */
FTD_PUBLIC ftd_int32_t
ftd_ctl_open ()
{

#if defined(HPUX)
extern int ftd_debug;
#endif

  if (ftd_global_state == NULL)
  {
    return (ENXIO);		/* invalid minor number */
  }

  /* OK, we're pretty easy about things, so the open succeeds */
  return 0;
}
