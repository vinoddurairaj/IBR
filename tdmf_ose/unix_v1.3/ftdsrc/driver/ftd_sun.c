#ifdef SOLARIS
/*
 * ftd_sun.c - FullTime Data driver for Solaris
 *
 * Copyright (c) 1996, 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 * $Id: ftd_sun.c,v 1.5 2001/10/04 14:30:01 bouchard Exp $
 *
 */

/*
 * Includes, Declarations and Local Data
 */
#include <sys/conf.h>
#include <sys/types.h>
#include <sys/errno.h>		/* XXX Not sure this is compliant */
#include <sys/kmem.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/open.h>
#include <sys/buf.h>
#include <sys/mkdev.h>		/* XXX Not sure about this one */
#include <sys/modctl.h>
#include <sys/utsname.h>

/* ftd_ddi.h must be included after all the system stuff to be ddi compliant */
#include "ftd_kern_ctypes.h"
#include "ftd_ddi.h"
#include "ftd_def.h"
#include "ftd_bab.h"
#include "ftd_all.h"
#include "ftd_klog.h"
#include "ftd_kern_cproto.h"

/* ----------------------------------------------------------------------------
 *      Module Loading/Unloading and Autoconfiguration Routines
 */

/*
 * Device driver ops vector
 * cb_ops(9S) structure, defined in sys/devops.h
 * Unsupported entry points (e.g. for xxprint() and xxdump()) are set to nodev,
 * which returns ENXIO.
 */
FTD_PRIVATE struct cb_ops ftd_cb_ops =
{
  ftd_open,			/* b/c open */
  ftd_close,			/* b/c close */
  ftd_strategy,		/* b strategy */
  nodev,				/* b print */
  nodev,				/* b dump */
  ftd_read,			/* c read */
  ftd_write,			/* c write */
  ftd_ioctl,			/* c ioctl */
  nodev,				/* c devmap */
  nodev,				/* c mmap */
  nodev,				/* c segmap */
  ftd_chpoll,			/* c poll */
  ddi_prop_op,			/* cb_prop_op */
  0,				/* streamtab  */
  D_64BIT | D_MP | D_NEW,	/* Driver compatibility flag */
  CB_REV,				/* cb_rev */
  nodev,          		/* cb_aread */	 /* Brad M. Fix newfs on Solaris 8, need to be verified on Solaris 7 */
  nodev           		/* cb_awrite */	 /* Brad M. Fix newfs on Solaris 8, need to be verified on Solaris 7 */

};


/*
 * dev_ops(9S) structure, defined in sys/devops.h.
 * Device Operations table, for autoconfiguration
 */

FTD_PRIVATE struct dev_ops ftd_ops =
{
  DEVO_REV,			/* devo_rev, */
  0,				/* refcnt  */
  ftd_info,			/* info */
  ftd_identify,			/* identify */
  ftd_probe,			/* probe */
  ftd_attach,			/* attach */
  ftd_detach,			/* detach */
  nodev,			/* reset */
  &ftd_cb_ops,			/* driver operations */
  (struct bus_ops *) 0,		/* bus operations */
  NULL				/* devo_power */
};


/*
 * Module Loading and Unloading
 * See modctl.h for external structure definitions.
 */

extern struct mod_ops mod_driverops;

extern ftd_char_t ftd_version_string[];

FTD_PRIVATE struct modldrv modldrv =
{
  &mod_driverops,		/* Type of module (driver). */
  ftd_version_string,		/* Description */
  &ftd_ops,			/* driver ops */
};

FTD_PRIVATE struct modlinkage modlinkage =
{
  MODREV_1, (ftd_void_t *) & modldrv, NULL
};

/*
 * Solaris DKI syntax and semantics have changed
 * going from 2.5.1/2.6 -> 2.7/2.8.
 *
 * to maintain backward compatability,
 * usage of affected DKI's now depends on 
 * on Solaris version number. the version is
 * sensed at attach time, and a global
 * variable is assigned a value accordingly.
 */

/* global data */
ftd_solvers_t ftd_solvers=SOL_VERS_UNKN;

/*
 * ftd_getutsname()
 * 
 * sense Solaris version number
 */
FTD_PRIVATE ftd_solvers_t
ftd_getutsname() 
{
  extern struct utsname utsname;

  /* only need to match up through major and minor release numbers */
  if (strncmp("5.5", utsname.release, strlen("5.5")) == 0) 
    {
      return (ftd_solvers = SOL_VERS_550);
    }
  if (strncmp("5.6", utsname.release, strlen("5.6")) == 0) 
    {
      return (ftd_solvers = SOL_VERS_560);
    }
  if (strncmp("5.7", utsname.release, strlen("5.7")) == 0) 
    {
      return (ftd_solvers = SOL_VERS_570);
    }
  if (strncmp("5.8", utsname.release, strlen("5.8")) == 0) 
    {
      return (ftd_solvers = SOL_VERS_580);
    }
  return (ftd_solvers = SOL_VERS_UNKN);

}

/*
 * ftd_logutsname()
 *
 * log sensed Solaris version number
 */
FTD_PRIVATE void
ftd_logutsname(instr) 
  char *instr;
{
  char *logstr;
  char fmtbuf[64];

  switch (ftd_solvers) 
    {
      case SOL_VERS_UNKN:
        sprintf(&fmtbuf[0],"SunOS release (unknown)");
        break;
      case SOL_VERS_550:
      case SOL_VERS_560:
      case SOL_VERS_570:
      case SOL_VERS_580:
        sprintf(&fmtbuf[0],"SunOS release %s", utsname.release);
        break;
    }
    return;
}


/* ----------------------------------------------------------------------------
 *      Module Loading/Installation Routines
 */

/*
 * Module Installation
 */
FTD_PUBLIC ftd_int32_t
_init (ftd_void_t)
{
  ftd_int32_t e;
  ftd_uint64_t x=0x12345678ffffffffll;


  if ((e = ddi_soft_state_init (&ftd_dev_state, sizeof (ftd_dev_t), 16)))
    return (e);
  if ((e = ddi_soft_state_init (&ftd_lg_state, sizeof (ftd_lg_t), 16)))
    return (e);

  if ((e = mod_install (&modlinkage)) != 0)
    {
      ddi_soft_state_fini (&ftd_dev_state);
      ddi_soft_state_fini (&ftd_lg_state);
      return (e);
    }

  return (e);
}


/*
 * Module removal.  Must remove stuff in the reverse order that we got them
 */
FTD_PUBLIC ftd_int32_t
_fini (ftd_void_t)
{
  ftd_int32_t e;


  if ((e = mod_remove (&modlinkage)) != 0)
    {
      return (e);
    }

  ddi_soft_state_fini (&ftd_dev_state);
  ddi_soft_state_fini (&ftd_lg_state);

  return (e);
}

/*
 * Return module info
 */
FTD_PUBLIC ftd_int32_t
_info (struct modinfo * modinfop)
{
  ftd_int32_t retcode;


  retcode = mod_info (&modlinkage, modinfop);
  return (retcode);
}

/***********************************************************************
 * ftd_bioclone
 * 
 * This routine should be called for solaris only.  It attmempt to copy
 * relevant field from a struct buf into a new buf which it creates.  The
 * new buf is intended to represent the same data as the original buf, 
 * except that the I/O will be redirected to the local data device.
 *
 * WARNING:
 * MUST CALL THIS ROUTINE with softp->lock held
 * MUST CALL THIS ROUTINE with softp->freelist != NULL
 *
 * Miscellaneous Notes:
 * (These are Warner's thoughts on things before we decided to ditch 
 *  bioclone for HP)
 * The last thing we want to do here is a copyin.
 *
 * There is a similar sounding routine in the solaris DDK, but it is
 * solaris specific and specific to 2.6 and later. 
 *
 * This routine tries to set things up so that you have two bufs that
 * represent the same data, yet can be treated independently wrt the
 * driver.
 *
 * On Solaris, we need to do this because we had tried to do smashing of
 * the struct buf, but it turns out that we can't reliably do that because
 * it lead to corruption that was hard to track down.  Also, we didn't
 * take out the proper locks.  We got a big hint about how to do this from
 * folks working at sun, which helped us to write it.
 *
 * HP, on the other hand, doesn't seem to have this functionality.
 * For the moment, we're doing a bcopy and hoping for the best.
 * However, other ways to deal with this problem on HP would include
 * just copying the data, and using our own struct buf.  This would
 * introduce overhead in the process.  The reason that copying the
 * data would work is that we'd have a known good context which to
 * copy the data from, which might not be the case later if we have to
 * reffer to the buf outside of that context.  More care could be had
 * with setting of the flags in the buf structure, but that might
 * require some trial and error and/or help from HP.  We could go back
 * to smashing the struct buf and dealing with things that way.  It
 * may be possible as well to come up with an adaptive smart copy (of
 * buf) routine that will do the right thing, plus maybe panic or
 * otherwise tell us when things aren't going quite right for it.
 *
 * Given that we're doing nearly completely different things for the
 * two systems, this routine might be better off in ftd_sun.c and
 * ftd_hpux.c.  
 */
FTD_PUBLIC struct buf *
ftd_bioclone (ftd_dev_t * softp, struct buf *sbp, struct buf *dbp)
{

  /* Get a destination buffer from the freelist if necessary */
  if (!dbp)
    {
      dbp = softp->freelist;
      softp->freelist = BP_PRIVATE (dbp);
      BP_PRIVATE (dbp) = NULL;
	/*- 
         * if this freelist just went NULL,
         * add another from the global freelist.
         */
      if (softp->freelist == (struct buf *) 0)
	{
	  struct buf *nbp;

	  if ((nbp = GETRBUF ()) != (struct buf *) 0)
	    {
	      nbp->b_flags |= B_BUSY;
	      BP_PRIVATE (nbp) = softp->freelist;
	      softp->freelist = nbp;
	      softp->bufcnt++;
	    }

	}
    }

  bufzero (dbp);

#ifdef B_SHADOW
#define BUF_CLONE_FLAGS (B_READ|B_WRITE|B_SHADOW|B_PHYS|B_PAGEIO)
#else
#ifdef B_PAGEIO
#define BUF_CLONE_FLAGS (B_READ|B_WRITE|B_PHYS|B_PAGEIO)
#else
#define BUF_CLONE_FLAGS (B_READ|B_WRITE|B_PHYS)
#endif
#endif
  /*
   * the cloned buffer does not inherit the B_REMAPPED flag. A separate
   * bp_mapin(9F) has to be done to get a kernel mapping.
   *
   * Solaris uses KERNBUF to control calling of the iodone function, 
   * while HPUX uses B_CALL.  On the theory that these may be common,
   * I've ifdef'd things like I have so that we may work on other 
   * systems.
   */
  dbp->b_flags = (sbp->b_flags & BUF_CLONE_FLAGS) |
#ifdef B_KERNBUF
    B_KERNBUF |
#endif
#ifdef B_CALL
    B_CALL |
#endif
    B_BUSY | B_NOCACHE;
  dbp->b_bcount = sbp->b_bcount;
  dbp->b_blkno = sbp->b_blkno;
  dbp->b_proc = sbp->b_proc;
  BP_DEV (dbp) = BP_DEV (sbp);
  dbp->b_un.b_addr = sbp->b_un.b_addr;

#ifdef B_SHADOW
  if (sbp->b_flags & B_SHADOW)
    {
      ASSERT (sbp->b_shadow);
      ASSERT (sbp->b_flags & B_PHYS);

      dbp->b_shadow = sbp->b_shadow;
    }
  else
#endif
#ifdef B_PAGEIO
    {
      if (sbp->b_flags & B_PAGEIO)
	{
	  dbp->b_pages = sbp->b_pages;
	}
      else
	{
	  if (sbp->b_flags & B_REMAPPED)
	    dbp->b_proc = NULL;
	}
    }
#endif
  /* have the new buf point back to the original buf */
  BP_USER (dbp) = BP_PCAST sbp;

  return dbp;

}

/* ----------------------------------------------------------------------------
 *      Autoconfiguration Routines
 */

/*
 * identify(9e) - are we the right driver?
 */
FTD_PRIVATE ftd_int32_t
ftd_identify (dev_info_t * dev)
{

  if (strcmp (ddi_get_name (dev), DRIVERNAME) == 0)
    return (DDI_IDENTIFIED);
  else
    return (DDI_NOT_IDENTIFIED);
}

/*

 * ftd_get_softp - return soft state for the FTD device from which
 * this I/O originated.  
 *
 * Parameters:  
 *     struct buf *bp - pointer to cloned buffer.
 *                
 *
 */
FTD_PUBLIC ftd_dev_t *
ftd_get_softp (struct buf * bp)
{
  ftd_dev_t *softp;
  struct buf *userbp;
  minor_t minor;

  userbp = BP_USER (bp);

  minor = getminor (BP_DEV (userbp));
  softp = ddi_get_soft_state (ftd_dev_state, U_2_S32(minor));
  if (softp == NULL)
    {
      FTD_ERR (FTD_WRNLVL, "Got a bad softp! my %x user %x dev %d\n",
	       bp, userbp, BP_DEV (userbp));
      bioerror (userbp, ENXIO);
      biodone (userbp);
      return NULL;
    }
  return softp;
}

/*
 * probe(9e) - Check to see if we're really here or not.  Since we're a
 * virtual driver, we are always here.
 */
/* ARGSUSED0 */
FTD_PRIVATE ftd_int32_t
ftd_probe (dev_info_t * devi)
{
  return (DDI_PROBE_SUCCESS);
}


/*
 * Attach
 */
FTD_PRIVATE ftd_int32_t
ftd_attach (dev_info_t * devi, ddi_attach_cmd_t cmd)
{
  ftd_int32_t inst;
  ftd_ctl_t *ctlp;

  /*
   * DDI_ATTACH is the only thing we know and grok.
   */
  if (cmd != DDI_ATTACH || ftd_failsafe)
    return (DDI_FAILURE);


  inst = ddi_get_instance (devi);
  if (inst != 0 || ftd_global_state)
    goto error;

  /* configure for OS version */
  ftd_getutsname();
  ftd_logutsname("ftd_attach(): configuring for");

  ctlp = ftd_global_state = kmem_zalloc (sizeof (ftd_ctl_t), KM_NOSLEEP);

  if (ctlp == NULL)
    {
      FTD_ERR (FTD_WRNLVL, "%s-ctl%d: Can't allocate state info",
	       DRVNAME, inst);
      goto error;
    }
  /* localdsk is obsolete, so detect it early. */
  if (ddi_getprop (DDI_DEV_T_ANY, devi, DDI_PROP_DONTPASS, "localdsk",
		   -1) != -1)
    {
      FTD_ERR (FTD_WRNLVL, "%s-ctl%d: attach: localdsk property obsolete, please update %s.conf",
	       DRVNAME, inst, DRVNAME);
      goto error;
    }

  /* Get the number of logical groups to create */
  ctlp->chunk_size = ddi_getprop (DDI_DEV_T_ANY, devi, DDI_PROP_DONTPASS,
				  "chunk_size", maxphys * 2);
  ctlp->num_chunks = ddi_getprop (DDI_DEV_T_ANY, devi, DDI_PROP_DONTPASS,
				  "num_chunks", 128);

  /* ftd_bab_init wants chunksize and numchunks as args. */
  ctlp->bab_size = ftd_bab_init (ctlp->chunk_size, ctlp->num_chunks);
  if (ctlp->bab_size == 0)
    goto error;
  ctlp->dev_info = devi;
  ctlp->lg_open = 0;

  /* initialize buffer pool */
  ctlp->ftd_buf_pool_headp = FTD_INIT_BUF_POOL ();

  /*
   * For a fixed-disk drive, read and verify the label
   * Don't worry if it fails, the drive may not be formatted.
   */
  ALLOC_LOCK (ctlp->lock, QNM " driver mutex");

  /*
   * Create a control device
   */
  if (ddi_create_minor_node (devi, "ctl", S_IFCHR, FTD_MAXMIN, DDI_PSEUDO,
			     0) == DDI_FAILURE)
    {
      FTD_ERR (FTD_WRNLVL, "%s-ctl%d: Create control device ctl failed",
	       DRVNAME, FTD_MAXMIN);
      goto error;
    }

  ddi_report_dev (devi);

  return (DDI_SUCCESS);

error:
  ftd_bab_fini ();
  ddi_remove_minor_node (devi, NULL);	/* remove all minor nodes */
  if (ftd_global_state)
    kmem_free (ftd_global_state, sizeof (ftd_ctl_t));
  return (DDI_FAILURE);
}

/*
 * Detach
 * Free resources allocated in _attach
 */
FTD_PRIVATE ftd_int32_t
ftd_detach (dev_info_t * devi, ddi_detach_cmd_t cmd)
{
  ftd_ctl_t *ctlp;

  if (cmd != DDI_DETACH || ftd_failsafe)
    {
      return (DDI_FAILURE);
    }

  (void) ddi_get_instance (devi);
  if ((ctlp = ftd_global_state) == NULL)
    return (DDI_FAILURE);

  ACQUIRE_LOCK (ctlp->lock, context);

  /*
   * We can't detach while things are open.
   */
  if (ctlp->lg_open || ftd_dev_n_open (ctlp))
    {
      RELEASE_LOCK (ctlp->lock, context);
      return DDI_FAILURE;
    }

  /*
   * Kill them all!
   */
  while (ctlp->lghead)
    ftd_del_lg (ctlp->lghead, getminor (ctlp->lghead->dev) & ~FTD_LGFLAG);

  RELEASE_LOCK (ctlp->lock, context);

  DEALLOC_LOCK (ctlp->lock);
  /*
   * Remove other data structures allocated in attach()
   */
  ftd_bab_fini ();
  kmem_free (ftd_global_state, sizeof (ftd_ctl_t));


  /* release private buffer pool */
  FTD_FINI_BUF_POOL ();

  /*
   * Remove all the nodes that we created before.
   */
  ddi_remove_minor_node (devi, NULL);

  return (DDI_SUCCESS);
}

/* ----------------------------------------------------------------------------
 *      Unix Entry Points
 */

/*
 * open(9e)
 * Called for each open(2) call on the device.
 * There are no mutexes in the open or close routines here. Synchronization
 * and locking are done with a semaphore. This serializes open and close
 * calls: if one is in progress, another will block until it completes. If
 * you add access to data that may also be accessed by other than open/close,
 * you may need to add a mutex.
 * Note: Credp can be used to restrict access to root, by calling drv_priv(9f);
 *       see also cred(9s).
 *       Flag shows the access mode (read/write). Check it if the device is
 *       read or write only, or has modes where this is relevant.
 *       Otyp is an open type flag, see open.h.
 */
/*ARGSUSED */
FTD_PRIVATE ftd_int32_t
ftd_open (dev_t * devp, ftd_int32_t flag, ftd_int32_t otyp, cred_t * credp)
{
  dev_t dev = *devp;
  minor_t minor = getminor (dev);

  if (minor == FTD_CTL)
    {
      if (otyp != OTYP_CHR)
	{
	  return ENXIO;
	}
      return ftd_ctl_open ();
    }
  else if (minor & FTD_LGFLAG)
    {
      if (otyp != OTYP_CHR)
	{
	  return ENXIO;
	}
      return ftd_lg_open (dev);
    }
  else
    {
      return ftd_dev_open (dev);
    }
}


/*
 * close(9e)
 * Called on final close only, i.e. the last close(2) call.
 */
/*ARGSUSED */
FTD_PRIVATE ftd_int32_t
ftd_close (dev_t dev, ftd_int32_t flag, ftd_int32_t otyp, cred_t * credp)
{
  minor_t minor = getminor (dev);

  if (minor == FTD_CTL)
    return ftd_ctl_close ();
  else if (minor & FTD_LGFLAG)
    return ftd_lg_close (dev);
  else
    return ftd_dev_close (dev);
}

/*
 * Device Configuration Routine
 * link instance number (unit) with dev_info structure
 */
/*ARGSUSED */
FTD_PRIVATE ftd_int32_t
ftd_info (dev_info_t * dip, ddi_info_cmd_t infocmd, ftd_void_t * arg, ftd_void_t ** result)
{
  ftd_ctl_t *ctlp;
  ftd_int32_t err;

  switch (infocmd)
    {
    case DDI_INFO_DEVT2DEVINFO:
      if ((ctlp = ftd_global_state) == NULL)
	return (DDI_FAILURE);
      *result = (ftd_void_t *) ctlp->dev_info;
      err = DDI_SUCCESS;
      break;
    case DDI_INFO_DEVT2INSTANCE:
      *result = (ftd_void_t *) 0;
      err = DDI_SUCCESS;
      break;
    default:
      err = DDI_FAILURE;
    }
  return (err);
}


/*
 * Character (raw) read and write routines, called via read(2) and
 * write(2). These routines perform "raw" (i.e. unbuffered) i/o.
 * Since they're so similar, there's actually one 'rw' routine for both,
 * these devops entry points just call the general routine with the
 * appropriate flag.
 */
/* ARGSUSED2 */
FTD_PRIVATE ftd_int32_t
ftd_read (dev_t dev, struct uio * uio, cred_t * cred_p)
{
  return (ftd_rw (dev, uio, B_READ));
}

/* ARGSUSED2 */
FTD_PRIVATE ftd_int32_t
ftd_write (dev_t dev, struct uio * uio, cred_t * cred_p)
{
  return (ftd_rw (dev, uio, B_WRITE));
}

FTD_PUBLIC ftd_void_t
ftd_wakeup (ftd_lg_t * lgp)
{
  /*
   * We can't hold any locks while we do the pollwakeup, per the Solaris
   * man page on pollwakeup(9f).  In addition, select will look for 
   * POLLRDNORM in its use of poll to emulate the traditional bsd 
   * behavior, so we return that as well as the more logical POLLIN the
   * ddi docs suggest.
   */
  pollwakeup (&lgp->ph, POLLIN | POLLRDNORM);
}

FTD_PRIVATE ftd_int32_t
ftd_chpoll (dev_t dev, ftd_int16_t events, ftd_int32_t anyyet, ftd_int16_t * reventsp,
	    struct pollhead ** phpp)
{
  ftd_lg_t *lgp;
  minor_t minor = getminor (dev);

  /*
   * Can't poll data device or ctl device
   */
  if ((minor & FTD_LGFLAG) == 0 || minor == FTD_CTL)
    return EINVAL;

  minor = minor & ~FTD_LGFLAG;

  if ((lgp = ddi_get_soft_state (ftd_lg_state, U_2_S32(minor))) == NULL)
    return (ENXIO);

  if ((lgp->state & FTD_M_JNLUPDATE) == 0 || lgp->wlentries)
    {
      *reventsp = (POLLIN | POLLRDNORM) & events;
    }
  else
    {
      *reventsp = 0;
      if (!anyyet)
	*phpp = &lgp->ph;
    }
  return 0;
}

FTD_PUBLIC ftd_int32_t
ftd_layered_open (struct dev_ops ** dopp, dev_t * devp)
{

  *dopp = ddi_hold_installed_driver (getmajor (*devp));
  if (*dopp == NULL)
    return 1;

  /* Open the device */
  return ((*dopp)->devo_cb_ops->
	  cb_open (devp, FREAD | FWRITE, OTYP_LYR, 0));
}

FTD_PUBLIC ftd_int32_t
ftd_layered_close (struct dev_ops * dop, dev_t dev)
{
  ftd_int32_t retval;

  if (dop == NULL)
    return 0;

  retval = dop->devo_cb_ops->cb_close (dev, FREAD | FWRITE, OTYP_LYR, 0);
  ddi_rele_driver (getmajor (dev));

  return retval;
}

FTD_PRIVATE ftd_int32_t
ftd_ioctl (dev_t dev, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag, cred_t * credp, ftd_int32_t * rvalp)
{
  minor_t minor = getminor (dev);
  ftd_int32_t err = 0;
  ftd_char_t buffer[256];
  size_t len;
  ftd_intptr_t passarg;


  len = (cmd >> 16) & IOCPARM_MASK;
  passarg = arg;
  if (cmd & IOC_IN)
    {
      err = ddi_copyin ((caddr_t) arg, (caddr_t) buffer, len, flag);
      if (err)
	err = EFAULT;
      passarg = (ftd_intptr_t) buffer;
    }
  if (err)
    goto out;
  if (minor == FTD_CTL)
    err = ftd_ctl_ioctl (dev, cmd, passarg, flag);
  else if (minor & FTD_LGFLAG)
    err = ftd_lg_ioctl (dev, cmd, passarg, flag);
  else
    err = ftd_dev_ioctl (dev, cmd, passarg, flag, credp, rvalp);
  if ((err == 0) && (cmd & IOC_OUT) && (((cmd >> 8) & 0xff) == FTD_CMD))
    {
      err = ddi_copyout ((caddr_t) buffer, (caddr_t) arg, len, flag);
      if (err)
	err = EFAULT;
    }
out:;
  return err;
}

/*
 * Any ioctls that we don't understand, we pass down to the next layer 
 * below us.
 */
FTD_PRIVATE ftd_int32_t
ftd_dev_ioctl (dev_t dev, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag, cred_t * credp, ftd_int32_t * rvalp)
{
  ftd_int32_t err = 0;
  ftd_dev_t *softp;

  minor_t minor = getminor (dev);
  if ((softp = ddi_get_soft_state (ftd_dev_state, U_2_S32(minor))) == NULL)
    return (ENXIO);		/* invalid minor number */

  switch (cmd)
    {
    default:
      /* Layer the ioctl call, pass it on to the next level. */
      if (softp->ld_dop)
	err = softp->ld_dop->devo_cb_ops->
	  cb_ioctl (softp->localcdisk, cmd, arg, flag, credp, rvalp);
      else
	err = ENOTTY;
      break;
    }
  return (err);
}

/* ARGSUSED2 */
FTD_PUBLIC ftd_int32_t
ftd_ctl_get_device_nums (dev_t dev, ftd_intptr_t arg, ftd_int32_t flag)
{
  ftd_devnum_t *devnum;

  devnum = (ftd_devnum_t *) arg;
  devnum->c_major = getmajor (dev);	/* Should convert to ext, but no documented way */
  devnum->b_major = devnum->c_major;

  return 0;
}
#endif /* SOLARIS */
