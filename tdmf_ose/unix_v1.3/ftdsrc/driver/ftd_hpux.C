#ifdef HPUX
/*
 * ftd_hpux.c - FullTime Data driver for HP-UX
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
#include "../h/sem_beta.h"
#include "../h/conf.h"
#include "../h/stat.h"
/* Get access to the 'u' structure */
#include "../h/user.h"
#include <sys/kthread_iface.h>
extern struct user *uptr;

/* ftd_ddi.h must be included after all the system stuff to be ddi compliant */
#include "ftd_kern_ctypes.h"
#include "ftd_ddi.h"
#include "ftd_def.h"
#include "ftd_all.h"
#include "ftd_klog.h"
#include "ftd_kern_cproto.h"
#if (SYSVERS >= 1100)
#include "../h/moddefs.h"
#include "../h/mod_conf.h"
#endif
#include "../wsio/wsio.h"

#if defined(HPUX) && 0
/* TMP TWS */
int iodoneCnt = 0;
lock_t *mylock;
/* TMP TWS */
#endif

/************** BEGIN HPUX REQUIRED STRUCTURES: *****************/

#if defined(HPUX) && (SYSVERS >= 1100)
  /* 
   * wrapper table 
   */
  FTD_PUBLIC struct mod_operations	gio_mod_ops;
  FTD_PRIVATE drv_info_t		ftd_drv_info;
  FTD_PUBLIC struct mod_conf_data	%Q%_conf_data;

  /* module type specific data */
  FTD_PRIVATE struct mod_type_data ftd_drv_link = {
	"%Q% - Loadable/Unloadable Module",
	(void *)NULL
  };

  FTD_PRIVATE struct modlink ftd_mod_link[] = {
	{ &gio_mod_ops, (void *)&ftd_drv_link }, /* WSIO */
	{ NULL, (void *)NULL }
  };

  struct modwrapper %Q%_wrapper = {
	MODREV,
	%Q%_load,
	%Q%_unload,
	(void (*) () )NULL,		/* halt function, currently is NULL */
	(void *)&%Q%_conf_data,	/* configuration data by config(1M) */
	ftd_mod_link
  };
#endif

/* local functions */
FTD_PRIVATE ftd_int32_t (*ftd_saved_dev_init) ();
FTD_PRIVATE ftd_int32_t ftd_init (void);
FTD_PRIVATE ftd_int32_t ftd_link_init (void);

#if (SYSVERS >= 1100)
/* kernel configurable paramenters for DLKM only */
extern ftd_int32_t ftd_chunk_size; 
extern ftd_int32_t ftd_num_chunk;
extern ftd_int32_t ftd_debug;
#else
ftd_int32_t ftd_debug = 0;
#endif

FTD_PRIVATE drv_ops_t ftd_drv_ops =
{
  ftd_open,			/* int (*d_open)(); */
  ftd_close,			/* int (*d_close)(); */
  ftd_strategy,			/* int (*d_strategy)(); */
  NULL,				/* int (*d_dump)(); */
  NULL,				/* int (*d_psize)(); */
  NULL,				/* int (*reserved0)(); */
  ftd_read,			/* int (*d_read)(); */
  ftd_write,			/* int (*d_write)(); */
  ftd_ioctl,			/* int (*d_ioctl)(); */
  ftd_select,			/* int (*d_select)(); */
  NULL,				/* int (*d_option1)(); */
  NULL,				/* pfilter_t *pfilter; */
  NULL,				/* int (*reserved1)(); */
  NULL,				/* int (*reserved2)(); */
  NULL,				/* int (*reserved3)(); */
  C_MGR_IS_MP			/* int d_flags; */
};

FTD_PRIVATE drv_info_t ftd_drv_info =
{
  QNM,		/* this must match the name prepended to "_install" */
  "pseudo",
  DRV_PSEUDO | DRV_BLOCK | DRV_CHAR | DRV_MP_SAFE,
  -1,
  -1,
  NULL,
  NULL,
  NULL
};

FTD_PRIVATE wsio_drv_data_t ftd_wsio_drv_data =
{
  "pseudo",
  T_DEVICE,
  DRV_CONVERGED,
  NULL,
  NULL
};

FTD_PRIVATE wsio_drv_info_t ftd_wsio_drv_info =
{
  &ftd_drv_info,
  &ftd_drv_ops,
  &ftd_wsio_drv_data
  
#if (SYSVERS >= 1111)		/* PBouchard: Add support for HP-11i (11.11) */
  , WSIO_DRV_CURRENT_VERSION 
#endif  
};

/*
 * q4cat() is used for cataloging the ftd data structure for Q4 debugging
 */
void
q4cat()
{
   ftd_ctl_t *t_ctl;
   ftd_lg_t *t_lg;
   ftd_dev_t *t_dev;
}

#if (SYSVERS >= 1100)
/*
 * LOAD
 */
FTD_PUBLIC ftd_int32_t
%Q%_load (ftd_void_t *arg)
{
  drv_info_t *t = (drv_info_t *)arg;

  /* use passed in drv_info instead of static version */
  ftd_wsio_drv_info.drv_info = (drv_info_t *) arg;

  /* register with WSIO */
  if (!wsio_install_driver(&ftd_wsio_drv_info))
  {
	printf("%Q%_load> wsio_install_driver failed!!\n");
	return (ENXIO);
  }

  /* save the c_major & b_major */
  ftd_drv_info.b_major = t->b_major;
  ftd_drv_info.c_major = t->c_major;

  /* init driver */
  (void) ftd_init();

  return (0);
}


/*
 * UNLOAD
 */
FTD_PUBLIC ftd_int32_t
%Q%_unload (ftd_void_t *arg)
{
  /* unregister with WSIO */
  if (wsio_uninstall_driver(&ftd_wsio_drv_info))
  {
	printf("%Q%_unload> wsio_uninstall_driver failed!!\n");
	return (ENXIO);
  }

  /* verify it's ok to unload */
  /* free all resources */
  if (ftd_detach() != DDI_SUCCESS)
  {
	printf("%Q%_unload> fails to detach resources!!\n");
  }

  return (0);
}
#endif

/*
 * ftd_init - this function is for dynamic loading, does not call init chain
 * Driver load function. Allocate structures, initialize globals, etc.
 */
FTD_PRIVATE ftd_int32_t
ftd_init (ftd_void_t)
{
  int rc;

  if ( (rc = ftd_attach () ) != DDI_SUCCESS)
  {
	printf("ftd_init: attach failed\n");
  }
  return (0);
}

/*
 * ftd_link_init - this function is for static loading, call init chain.
 * Driver install function. Allocate structures, initialize globals, etc.
 */
FTD_PRIVATE ftd_int32_t
ftd_link_init (ftd_void_t)
{
  ftd_attach ();

  /* chain to next drivers init function */
  return ((*ftd_saved_dev_init) ());
}

/*
 * Required function for driver install. Tells the kernel which function to 
 * call at a later time.
 */
ftd_int32_t
%Q%_install ()
{
  extern ftd_int32_t (*dev_init) ();

  ftd_saved_dev_init = dev_init;
  dev_init = ftd_link_init;

  return (wsio_install_driver (&ftd_wsio_drv_info));
}

/* ----------------------------------------------------------------------------
 *    Unix Entry Points
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
 *     see also cred(9s).
 *     Flag shows the access mode (read/write). Check it if the device is
 *     read or write only, or has modes where this is relevant.
 *     Otyp is an open type flag, see open.h.
 */
/*ARGSUSED */
FTD_PRIVATE ftd_int32_t
ftd_open (dev_t dev, ftd_int32_t flag)
{
  minor_t minor = getminor (dev);

  if (minor == FTD_CTL)
    {
      return ftd_ctl_open ();
    }
  else if (minor & FTD_LGFLAG)
    {
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
FTD_PRIVATE ftd_int32_t
ftd_close (dev_t dev, ftd_int32_t flag)
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
 * Character (raw) read and write routines, called via read(2) and
 * write(2). These routines perform "raw" (i.e. unbuffered) i/o.
 * Since they're so similar, there's actually one 'rw' routine for both,
 * these devops entry points just call the general routine with the
 * appropriate flag.
 */
FTD_PRIVATE ftd_int32_t
ftd_read (dev_t dev, struct uio * uio)
{
  return (ftd_rw (dev, uio, B_READ));
}

FTD_PRIVATE ftd_dev_t *
ftd_ld2soft (dev_t dev)
{
  ftd_lg_t *lgwalk;
  ftd_dev_t *devwalk;

  for (lgwalk = ftd_global_state->lghead; lgwalk; lgwalk = lgwalk->next)
    {
      for (devwalk = lgwalk->devhead; devwalk; devwalk = devwalk->next)
	{
	  if (devwalk->localbdisk == dev)
	    {
	      return devwalk;
	    }
	}
    }
  return 0;
}
/*

 * ftd_get_softp - return soft state for the FTD device from which
 * this I/O originated.  
 *
 * Parameters:  
 *     struct buf *bp - pointer to a buf which has already been modified
 *                      by us, (b_dev has been changed from FTD to local
 *                      data device)              
 *
 */
FTD_PUBLIC ftd_dev_t *
ftd_get_softp (struct buf * bp)
{
  ftd_dev_t *softp;
  struct buf *userbp;

  softp = BP_SOFTP (bp);
  if (softp == NULL)
    {
      FTD_ERR (FTD_WRNLVL,
	       "Got a bad softp! bp %x dev %d\n", bp, BP_DEV (bp));
      bioerror (bp, ENXIO);
      finish_biodone (bp);
      return NULL;
    }
  return softp;
}

/*
 *  ftd_bioclone -  This routine will save off our original buf struct that
 *  was passed to ftd_stategy.
 *  We do this because we will modify things before we pass the buf down 
 *  below us, and we want to be able to restore these when the io completes.
 *  
 *  MUST CALL THIS ROUTINE WITH softp->lock held
 *  MUST CALL THIS ROUTINE WITH softp->freelist != NULL
 */
FTD_PUBLIC struct buf *
ftd_bioclone (ftd_dev_t * softp, struct buf *sbp, struct buf *dbp)
{
  ftd_context_t context;
  space_t from_sid, to_sid;

  /* allocate a buffer header from this device's pool */
  if (!dbp)
    {
      dbp = softp->freelist;
      softp->freelist = BP_PRIVATE (dbp);
      BP_PRIVATE (dbp) = NULL;
	/*- 
         * if this freelist just went NULL,
         * add another from the global freelist.
         */
/* TWS disable
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
TWS */
    }
  bufzero (dbp);

  from_sid = ldsid ( (caddr_t)sbp );
  to_sid = ldsid ( (caddr_t)dbp );
  privlbcopy (from_sid, (caddr_t) sbp, to_sid, (caddr_t) dbp, sizeof (struct buf));

  dbp->b_flags |= B_BUSY;	/* set clone buf to busy */
  dbp->b_flags |= B_CALL;	/* iodone() points to by b_iodone */

  dbp->b_flags &= ~(B_CACHE|B_BCACHE|B_ASYNC);
  dbp->b_flags |= B_NOCACHE /* |B_PRIVATE PB 2001-07-19, HP recommandation */ ;

  /* b2_flags needs to be clear, because we're not from the buffer cache */
  dbp->b2_flags = 0;

  dbp->av_forw =
    dbp->av_back =
    dbp->b_forw =
    dbp->b_blockf =
    dbp->b_blockb = 
    dbp->b_back = (struct buf *) 0;

  /* clone buffer references cloned buf */
  BP_USER (dbp) = BP_PCAST sbp;
  BP_SOFTP (dbp) = softp;

  return dbp;
}

FTD_PRIVATE ftd_int32_t
ftd_write (dev_t dev, struct uio * uio)
{
  return (ftd_rw (dev, uio, B_WRITE));
}

/*
 * Wakeup the process sleeping on this fd in select.  Ordinarily we'd
 * need to include a test to see if multiple processes are colliding on
 * their desire to multiplex this data, but since there will be at most one
 * logical group ctl device open at once from the PMD, this isn't an issue. 
 */
FTD_PUBLIC ftd_void_t
ftd_wakeup (ftd_lg_t * lgp)
{
  /* Don't call selwakeup unless we have someone to wakeup */
  if (lgp->read_waiter_thread) {
  	selwakeup (lgp->read_waiter_thread, 0);
	lgp->read_waiter_thread = NULL;
  }
}

/* ftd_select  
 * This is the driver entry point for the select() system call.
 * Here we want to return True whenever select is called for a dtc device,
 * since we're always ready for reading and writing.  But select may also be  
 * called for a logical group or global control device (ctl device).  For the 
 * Global device (/dev/dtc/ctl) we simply return true.  Select is also used as
 * a means for the driver to wakeup the PMD when there are BAB entries  
 * available or the state has changed for the logical group.  This is done 
 * through a PMD call to select on the logical group ctl device.
 */ 
FTD_PUBLIC ftd_int32_t
ftd_select (dev_t dev, ftd_int32_t flag)
{
  ftd_context_t context;
  ftd_lg_t *lgp;
  minor_t minor;
  /*
   * There is always non-read data available.  Also, if we detect some
   * kind of error, then return "we have data" since we can't flag that
   * error as far as I can tell.
   */
  if (flag != FREAD)
    return 1;
  minor = getminor (dev);
  if ((minor == FTD_CTL) || ((minor & FTD_LGFLAG) == 0))
    return 1;
  minor = minor & ~FTD_LGFLAG;
  if ((lgp = ddi_get_soft_state (ftd_lg_state, U_2_S32 (minor))) == NULL)
    return 1;

  ACQUIRE_LOCK (lgp->lock, context);

  /*
   * If we actually have data, return right away
   */
  if ((lgp->state & FTD_M_JNLUPDATE) == 0 || lgp->wlentries)
    {
      RELEASE_LOCK (lgp->lock, context);
      return 1;
    }
  lgp->read_waiter_thread = u.u_kthreadp;
  RELEASE_LOCK (lgp->lock, context);
  return 0;
}

FTD_PRIVATE ftd_int32_t
ftd_ioctl (dev_t dev, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag)
{
  minor_t minor = getminor (dev);
  ftd_int32_t err;

  if (minor == FTD_CTL)
    err = ftd_ctl_ioctl (dev, cmd, arg, flag);
  else if (minor & FTD_LGFLAG)
    err = ftd_lg_ioctl (dev, cmd, arg, flag);
  else
    err = ftd_dev_ioctl (dev, cmd, arg, flag);

  return err;
}

/*
 * Any ioctls that we don't understand, we pass down to the next layer 
 * below us.
 */
FTD_PRIVATE ftd_int32_t
ftd_dev_ioctl (dev_t dev, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag)
{
  ftd_int32_t err = 0;
  ftd_dev_t *softp;

  minor_t minor = getminor (dev);
  if ((softp = ddi_get_soft_state (ftd_dev_state, U_2_S32 (minor))) == NULL)
    return (ENXIO);		/* invalid minor number */

  switch (cmd)
    {
    default:
      /* Layer the ioctl call, pass it on to the next level. */
      err = cdev_ioctl (softp->localcdisk, cmd, (caddr_t)arg, flag);
      break;
    }
  return (err);
}

/*
 * Open the block device. Logical volumes use a different interface.
 */
FTD_PUBLIC ftd_int32_t
bdev_open (dev_t dev, ftd_int32_t flags)
{
  drv_info_t *info;

  /* look at the name in the drv_info structure */
  info = (drv_info_t *) bdevsw[major (dev)].d_drv_info;

  /* if this is an LVM device, construct the args */
  if (strcmp (info->name, "lv") == 0)
    {
      return (bdevsw[major (dev)].d_open (dev, flags, 0, _S_IFBLK));
    }
  else
    {
      return (bdevsw[major (dev)].d_open (dev, flags, 0, _S_IFBLK));
    }
  return 0;
}

/*
 * Open the raw device. Logical volumes use a different interface.
 */
FTD_PUBLIC ftd_int32_t
cdev_open (dev_t dev, ftd_int32_t flags)
{
  drv_info_t *info;

  /* look at the name in the drv_info structure */
  info = (drv_info_t *) cdevsw[major (dev)].d_drv_info;

  /* if this is an LVM device, construct the args */
  if (strcmp (info->name, "lv") == 0)
    {
      return (cdevsw[major (dev)].d_open (dev, flags, 0, _S_IFCHR));
    }
  else
    {
      return (cdevsw[major (dev)].d_open (dev, flags, 0, _S_IFCHR));
    }
  return 0;
}

/*
 * Attach
 */
FTD_PRIVATE ftd_int32_t
ftd_attach ()
{
  ftd_ctl_t *ctlp;

  q4cat();	/* Q4 catalog */

  /*
   * DDI_ATTACH is the only thing we know and grok.
   */
  if (ftd_failsafe)
    return (DDI_FAILURE);

  if (ftd_global_state)
    goto error;

  ctlp = ftd_global_state = kmem_zalloc (sizeof (ftd_ctl_t), KM_NOSLEEP);


  if (ddi_soft_state_init (&ftd_dev_state, sizeof (ftd_dev_t), 16) != 0)
    {
      return DDI_FAILURE;
    }
  if (ddi_soft_state_init (&ftd_lg_state, sizeof (ftd_lg_t), 16) != 0)
    {
      ddi_soft_state_fini (&ftd_dev_state);
      return DDI_FAILURE;
    }

#if (SYSVERS >= 1100)
  ctlp->chunk_size = ftd_chunk_size;
  ctlp->num_chunks = ftd_num_chunk;
#else
  ctlp->chunk_size = DEFAULT_MEMCHUNK_SIZE;
  ctlp->num_chunks = 0x7fffffff;
#endif

  ctlp->bab_size = ftd_bab_init (ctlp->chunk_size, ctlp->num_chunks);
  ctlp->lg_open = 0;

  /* initialize buffer pool list for use by getrbuf and freerbuf */
  if ( (ctlp->ftd_buf_pool_headp = FTD_INIT_BUF_POOL ()) == 0)
  {
	printf("ftd_attach: failed to FTD_INIT_BUF_POOL\n");
	return (DDI_FAILURE);
  }

  /* establishes ftd_timer */
  ftd_timer (0);

  /*
   * For a fixed-disk drive, read and verify the label
   * Don't worry if it fails, the drive may not be formatted.
   */
  ALLOC_LOCK (ctlp->lock, QNM " driver mutex");
#if defined(HPUX) && 0
/* TMP TWS */
  ALLOC_LOCK (mylock, "mylock");
/* TMP TWS */
#endif


  return (DDI_SUCCESS);

error:
  if (ftd_debug)
	printf("ftd_attach: ERROR!!\n");

  ftd_bab_fini ();

  if (ftd_global_state)
    kmem_free (ftd_global_state, sizeof (ftd_ctl_t));
  return (DDI_FAILURE);
}

/*
 * Detach
 * Free resources allocated in _attach
 */
FTD_PRIVATE ftd_int32_t
ftd_detach ()
{
  ftd_int32_t inst;
  ftd_ctl_t *ctlp;
  ftd_int32_t i;

  if (ftd_failsafe)
    {
      return (DDI_FAILURE);
    }

  if ((ctlp = ftd_global_state) == NULL)
    return (DDI_FAILURE);

  /*
   * We can't detach while things are open.
   */
  if (ctlp->lg_open || ftd_dev_n_open (ctlp))
  {
    if (ftd_debug)
	printf("%Q%_unload> device busy\n");
    return DDI_FAILURE;
  }

  DEALLOC_LOCK (ctlp->lock);
  /*
   * Remove other data structures allocated in attach()
   */
  ftd_bab_fini ();
/* TWS, replace with FREE */
/*
  kmem_free (ftd_global_state, sizeof (ftd_ctl_t));
*/
  FREE (ftd_global_state, M_IOSYS);

  /* release buffer pool */
  FTD_FINI_BUF_POOL ();

  /* disarm sync timer */
  ftd_disarm_timer();

  ddi_soft_state_fini (&ftd_dev_state);
  ddi_soft_state_fini (&ftd_lg_state);

  return (DDI_SUCCESS);
}

/*
 * return major numbers for our device.
 */
FTD_PUBLIC ftd_int32_t
ftd_ctl_get_device_nums (dev_t dev, ftd_intptr_t arg, ftd_int32_t flag)
{
  ftd_devnum_t *devnum;

  devnum = (ftd_devnum_t *) arg;
  devnum->c_major = ftd_drv_info.c_major;
  devnum->b_major = ftd_drv_info.b_major;

  return 0;
}

#endif /* HPUX */
