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
#if (SYSVERS >= 1123)
    #include <sys/sem_beta.h>
    #include <sys/conf.h>
    #include <sys/stat.h>
    #include <sys/kc_meta.h>
    #include <sys/ktune.h>

    #if (SYSVERS == 1123)
    // The pointer to the currently running thread is obtained by copying it from u.u_kthreadp;
    #include <sys/user.h>
    #endif

    #if (SYSVERS >= 1131)
    // The pointer to the currently running thread is obtained by calling threadp_self();
    #include <sys/kthread.h>

    // The preprocessor black magic was adapted from http://okmij.org/ftp/c++-digest/computable-include.txt
    // The idea is to either include "dtc.modmeta.h" or "dtmf.modmeta.h"
    #define QMAKESTR(x) #x
    #define MAKESTR(x) QMAKESTR(x)
    #define MOD_META_H_STRING MAKESTR(MOD_META_H) 

    // Rely on the brand named and generated modmeta defines.
    #include MOD_META_H_STRING
    #endif

#else
    #include "../h/sem_beta.h"
    #include "../h/conf.h"
    #include "../h/stat.h"
    /* Get access to the 'u' structure */
    #include "../h/user.h"
#endif /* (SYSVERS >= 1123) */

#if defined(HPUX) && (SYSVERS >= 1100)
    #include <sys/kthread_iface.h>
    #include <sys/sysmacros.h>
#endif

extern struct user *uptr;

/* ftd_ddi.h must be included after all the system stuff to be ddi compliant */
#include "ftd_kern_ctypes.h"
#include "ftd_ddi.h"
#include "ftd_def.h"
#include "ftd_all.h"
#include "ftd_klog.h"
#include "ftd_kern_cproto.h"
#include "ftd_dynamic_activation.h"

#if (SYSVERS >= 1123)
    #include <sys/moddefs.h>
    #include <sys/mod_conf.h>
    #include <sys/wsio.h>
#elif (SYSVERS == 1100) ||  (SYSVERS == 1111)
    #include "../h/moddefs.h"
    #include "../h/mod_conf.h"
    #include "../wsio/wsio.h"
#else
    #include "../wsio/wsio.h"
#endif

#if defined(HPUX) && 0
/* TMP TWS */
int iodoneCnt = 0;
lock_t *mylock;
/* TMP TWS */
#endif

/************** BEGIN HPUX REQUIRED STRUCTURES: *****************/

// Starting with 11.31, all of these structures are automatically generated within xxx.modmeta.c.
#if defined(HPUX) && (SYSVERS >= 1100) && (SYSVERS <= 1123)
  /* 
   * wrapper table 
   */
  FTD_PUBLIC struct mod_operations	gio_mod_ops;
  FTD_PRIVATE drv_info_t		ftd_drv_info;
  FTD_PUBLIC struct mod_conf_data	dtc_conf_data;

  /* module type specific data */
  FTD_PRIVATE struct mod_type_data ftd_drv_link = {
	"dtc - Loadable/Unloadable Module",
	(void *)NULL
  };

  FTD_PRIVATE struct modlink ftd_mod_link[] = {
	{ &gio_mod_ops, (void *)&ftd_drv_link }, /* WSIO */
	{ NULL, (void *)NULL }
  };

  struct modwrapper dtc_wrapper = {
	MODREV,
	dtc_load,
	dtc_unload,
	(void (*) () )NULL,		/* halt function, currently is NULL */
	(void *)&dtc_conf_data,	/* configuration data by config(1M) */
	ftd_mod_link
  };
#endif

/* local functions */
FTD_PRIVATE ftd_int32_t (*ftd_saved_dev_init) ();
FTD_PRIVATE ftd_int32_t ftd_init (void);
FTD_PRIVATE ftd_int32_t ftd_link_init (void);

#if (SYSVERS >= 1123)
  ftd_int32_t ftd_chunk_size;
  ftd_int32_t ftd_num_chunk;
  ftd_int32_t ftd_debug;
#elif (SYSVERS >= 1100)
  /* kernel configurable paramenters for DLKM only */
  extern ftd_int32_t ftd_chunk_size; 
  extern ftd_int32_t ftd_num_chunk;
  extern ftd_int32_t ftd_debug;
#else
  ftd_int32_t ftd_debug = 0;
#endif

/* WR34794: negative spinlock checking for HP service (check spinunlock).
 * Since it is not a Softek bug, set it to not checking owns_spinlock 
 * prior spinunlock call, by default.
 */
int ftd_lockdbg = 0;

extern ftd_lg_map_t	Started_LG_Map;
extern ftd_uchar_t      Started_LGS[];

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

#if (SYSVERS >= 1131)
// Starting with 11.31, the drv_info structure is automatically generated within xxx.modmeta.c.
// Since the module is named dtc, the generated drv_info structure is named dtc_drv_info.
// We rely on a define in order to avoid having to change all the existing code expecting a ftd_drv_info structure.
#define ftd_drv_info dtc_drv_info

#else
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
#endif

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
dtc_load (ftd_void_t *arg)
{
  drv_info_t *t = (drv_info_t *)arg;

  /* use passed in drv_info instead of static version */
  ftd_wsio_drv_info.drv_info = (drv_info_t *) arg;

  /* register with WSIO */
  if (!wsio_install_driver(&ftd_wsio_drv_info))
  {
	printf("dtc_load> wsio_install_driver failed!!\n");
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
dtc_unload (ftd_void_t *arg)
{
  /* unregister with WSIO */
  if (wsio_uninstall_driver(&ftd_wsio_drv_info))
  {
	printf("dtc_unload> wsio_uninstall_driver failed!!\n");
	return (ENXIO);
  }

  /* verify it's ok to unload */
  /* free all resources */
  if (ftd_detach() != DDI_SUCCESS)
  {
	printf("dtc_unload> fails to detach resources!!\n");
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
dtc_install ()
{
  extern ftd_int32_t (*dev_init) ();

  ftd_saved_dev_init = dev_init;
  dev_init = ftd_link_init;

  return (wsio_install_driver (&ftd_wsio_drv_info));
}

#if (SYSVERS >= 1131)
void void_dtc_install ()
{
   dtc_install();
}
#endif /* (SYSVERS >= 1131) */

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
ftd_open (dev_t dev, ftd_int32_t flag, intptr_t dummy, int mode)
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
ftd_close (dev_t dev, ftd_int32_t flag, int mode)
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

      if (softp->freelist == (struct buf *) 0)
	{
	  struct buf *nbp;
	  if ((nbp = GETRBUF (KM_NOSLEEP)) != (struct buf *) 0)
	    {
	      nbp->b_flags |= B_BUSY;
	      BP_PRIVATE (nbp) = softp->freelist;
	      softp->freelist = nbp;
	      softp->bufcnt++;
	    }

	}

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
    dbp->b_back = (struct buf *) 0;
    dbp->b_blockb = (struct buf **) 0;

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
	#if defined(HPUX) && (SYSVERS >= 1100)
	/* Don't call selwakeup unless we have someone to wakeup */

	/* WR 34691: added spinlock protection.
	 * Race condtion in the IO completion path.
	 * lgp->read_waiter_thread can be set to NULL by one CPU and
	 * cause another processor to panic.
	 */
	ACQUIRE_LOCK (lgp->lock, context);
	if (lgp->read_waiter_thread) 
		{
		selwakeup (lgp->read_waiter_thread, 0);
		lgp->read_waiter_thread = NULL;
		}
	RELEASE_LOCK (lgp->lock, context);
	#else
	selwakeup (lgp->readproc, 0);
	#endif
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
#if defined(HPUX) && (SYSVERS >= 1100) && (SYSVERS <= 1123)
  lgp->read_waiter_thread = u.u_kthreadp;
#elif (SYSVERS >= 1131)
  lgp->read_waiter_thread = kthreadp_self();
#else  
  lgp->readproc = u.u_procp;
#endif
  RELEASE_LOCK (lgp->lock, context);
  return 0;
}

FTD_PRIVATE ftd_int32_t
ftd_ioctl (dev_t dev, ftd_int32_t cmd, caddr_t arg, ftd_int32_t flag)
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

  /* HPUX: should not be here at all ?? */
  if (ftd_global_state)
    goto error;

  ctlp = ftd_global_state = kmem_zalloc (sizeof (ftd_ctl_t), KM_NOSLEEP);

  if (ddi_soft_state_init (&ftd_dev_state, sizeof (ftd_dev_t), 16) != 0) {
      if (ftd_global_state)
         kmem_free (ftd_global_state, sizeof (ftd_ctl_t));
      return DDI_FAILURE;
  }
  if (ddi_soft_state_init (&ftd_lg_state, sizeof (ftd_lg_t), 16) != 0) {
      if (ftd_global_state)
         kmem_free (ftd_global_state, sizeof (ftd_ctl_t));
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

#if defined(TUNABLE_CHUNKSIZE)
  if (ctlp->chunk_size < MIN_MEMCHUNK_SIZE) {
    if (ftd_global_state) {
      kmem_free (ftd_global_state, sizeof (ftd_ctl_t));
    }
    /* release memory allocated for stat structure */
    ddi_soft_state_fini(&ftd_dev_state);
    ddi_soft_state_fini(&ftd_lg_state);

    printf("ftd_attach: chunk_size has to be larger than %d. chunk_size=%d, num_chunks=%d\n",
        MIN_MEMCHUNK_SIZE, ctlp->chunk_size, ctlp->num_chunks);
    return (DDI_FAILURE);
  }
#else /* TUNABLE_CHUNKSIZE */
  if (ctlp->chunk_size != DEFAULT_MEMCHUNK_SIZE) {
    printf("ftd_attach: chunk_size is modified %d. "
           "reset chunk_size to default. chunk_size=%d, num_chunks=%d\n",
           ctlp->chunk_size, DEFAULT_MEMCHUNK_SIZE, ctlp->num_chunks);
    ctlp->chunk_size = DEFAULT_MEMCHUNK_SIZE;
  }
#endif /* TUNABLE_CHUNKSIZE */

  ctlp->bab_size = ftd_bab_init (ctlp->chunk_size, ctlp->num_chunks);
  if (ctlp->bab_size == 0) {
	if (ftd_global_state)
           kmem_free (ftd_global_state, sizeof (ftd_ctl_t));
	/* release memory allocated for stat structure */
	ddi_soft_state_fini(&ftd_dev_state);
	ddi_soft_state_fini(&ftd_lg_state);

	printf("ftd_attach: failed to init BAB. chunk_size=%d, num_chunks=%d\n",
		ctlp->chunk_size, ctlp->num_chunks);
	return (DDI_FAILURE);
  }
  ctlp->hrdb_type = FTD_HS_NOT_SET;
  
  Started_LG_Map.lg_map = (ftd_uint64ptr_t) 0;
  Started_LG_Map.count=0;	
  Started_LG_Map.lg_max=0;	
  ftd_memset (Started_LGS, FTD_STOP_GROUP_FLAG, MAXLG);

  /* initialize buffer pool list for use by getrbuf and freerbuf */
  if ( (ctlp->ftd_buf_pool_headp = FTD_INIT_BUF_POOL ()) == 0)
  {
	if (ftd_global_state)
           kmem_free (ftd_global_state, sizeof (ftd_ctl_t));
	/* release memory allocated for stat structure */
	ddi_soft_state_fini(&ftd_dev_state);
	ddi_soft_state_fini(&ftd_lg_state);
	/* free allocated BAB memory */
	ftd_bab_fini();

	printf("ftd_attach: failed to FTD_INIT_BUF_POOL.\n");
	return (DDI_FAILURE);
  }

  /* establishes ftd_timer */
  ftd_timer (0);

  if(ftd_dynamic_activation_init(makedev(ftd_drv_info.c_major, FTD_CTL)) != 0) {
      goto error;
  }
  
  /* for Dynamic Mode Change */
  sync_time_out_flg = (ftd_int32_t *)kmem_zalloc(
			sizeof(ftd_int32_t) * MAXLG, KM_NOSLEEP);
  if (sync_time_out_flg == NULL) {
      goto error;
  }

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

  printf("ftd_attach: successful,  chunk_size=%d, num_chunks=%d\n",
		ctlp->chunk_size, ctlp->num_chunks);

  return (DDI_SUCCESS);

error:
  if (ftd_debug)
	printf("ftd_attach: ERROR!!\n");

  /* release memory allocated for stat structure */
  ddi_soft_state_fini(&ftd_dev_state);
  ddi_soft_state_fini(&ftd_lg_state);
  /* free allocated BAB memory */
  ftd_bab_fini ();
  /* release buffer pool */
  FTD_FINI_BUF_POOL ();
  /* disarm sync timer */
  ftd_disarm_timer();

  if (ftd_global_state)
    kmem_free (ftd_global_state, sizeof (ftd_ctl_t));
  if (sync_time_out_flg)
    kmem_free (sync_time_out_flg, sizeof(ftd_int32_t) * MAXLG);

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
  if (ftd_dev_n_open (ctlp))
  {
    if (ftd_debug)
	printf("dtc_unload> device busy\n");
    return DDI_FAILURE;
  }

  if (ctlp->lock)
    DEALLOC_LOCK (ctlp->lock);
  /*
   * Remove other data structures allocated in attach()
   */
  ftd_bab_fini ();
/* TWS, replace with FREE */
/*
  kmem_free (ftd_global_state, sizeof (ftd_ctl_t));
*/
  if (ftd_global_state)
    FREE (ftd_global_state, M_IOSYS);
  if (sync_time_out_flg)
    FREE (sync_time_out_flg, M_IOSYS);

  /* release buffer pool */
  FTD_FINI_BUF_POOL ();

  /* disarm sync timer */
  ftd_disarm_timer();

  ftd_dynamic_activation_finish();
  
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

#if (SYSVERS >= 1123)
/***********************************************************************
 * Function name: dtc_init_tunables
 * Synopsis: This Function is used to initialize the DLKM tunables in 11.23.
 *        It is used to register the module's tunable and retrive the
 *        value from the tunable infrastructure.
 *	  Three currently defined tunables:
 *		ftd_chunk_size, ftd_debug, ftd_num_chunk
 * Parameters: None
 * Return Value:0, Success
 *            1, Failure
 ***********************************************************************/

#define CHUNK_SIZE_DFL	1048576
#define CHUNK_SIZE_FS	1048576

#define NUM_CHUNK_DFL	32
#define NUM_CHUNK_FS	16

#define DBG_DFL		0
#define DBG_FS		0

ktune_id_t dtc_cs_tuneid;
ktune_id_t dtc_nc_tuneid;
ktune_id_t dtc_dbg_tuneid;

int ftd_init_tunables(void)
{
	int result;
        int ret;
        kc_tunable_t *tunable;
        static ktune_id_t tmp_tuneid= KTUNE_ID_NULL;
        int reason = 2;
        static uint64_t currentval;
        static uint64_t pendingval;
        const char* tunablename;
        int defaultval;
        int autoval;

 	msg_printf("[ dtc_init_tunables enter\n", QNM);
 	msg_printf("dtc_init_tunables setting ftd_chunk_size\n", QNM);

        /* Can also be used instead of writing our own handler , 
	 * this is good ONLY for changing tunables dynamically  
	 * that are simple integers. It should  be used only 
	 * for integer variables that can be changed without 
	 * any need for locking or synchronization
         */
        ret = ktune_register_handler(KTUNE_VERSION, "ftd_chunk_size", 0,
                                        KEN_UNORDERED, "BAB chunk_size",
                                        (ktune_handler_t)ktune_simple_dynamic,
                                        &ftd_chunk_size);

        if(ret != 0) {
                msg_printf("ftd_init_tunables:  ktune_register_handler failed for ftd_chunk_size \n", QNM);
                return 1;
        }
        ftd_chunk_size = (uint32_t)ktune_get("ftd_chunk_size", (ftd_int32_t) CHUNK_SIZE_FS);

        /* Get Chunk Size tuneid*/
        tmp_tuneid = ktune_id("ftd_chunk_size");

        /*Pass tuneid globally, later to be
         *used during unload
         */
        dtc_cs_tuneid = tmp_tuneid;


 	msg_printf("dtc_init_tunables setting ftd_debug\n", QNM);
        ret = ktune_register_handler(KTUNE_VERSION, "ftd_debug", 0,
                                        KEN_UNORDERED, "ftd debug flag",
                                        (ktune_handler_t)ktune_simple_dynamic,
                                        &ftd_debug);
        if(ret != 0) {
                msg_printf("ftd_init_tunables:  ktune_register_handler failed for ftd_debug\n", QNM);
                return 1;
        }
        ftd_debug= (uint32_t)ktune_get("ftd_debug", (ftd_int32_t) DBG_FS);
        tmp_tuneid = ktune_id("ftd_debug");
        dtc_dbg_tuneid = tmp_tuneid;

 	msg_printf("dtc_init_tunables setting ftd_num_chunk\n", QNM);
        ret = ktune_register_handler(KTUNE_VERSION, "ftd_num_chunk", 0,
                                        KEN_UNORDERED, "BAB num_chunk",
                                        (ktune_handler_t)ktune_simple_dynamic,
                                        &ftd_num_chunk);
        if(ret != 0) {
                msg_printf("ftd_init_tunables:  ktune_register_handler failed for ftd_num_chunk\n", QNM);
                return 1;
        }
        ftd_num_chunk= (uint32_t)ktune_get("ftd_num_chunk", (ftd_int32_t)NUM_CHUNK_FS); 
        tmp_tuneid = ktune_id("ftd_num_chunk");
        dtc_nc_tuneid = tmp_tuneid;

 	msg_printf("ftd_init_tunables exit ]\n", QNM);
	return 0;
}

#if (SYSVERS >= 1131)
void void_ftd_init_tunables(void)
{
   ftd_init_tunables();
}
#endif /* (SYSVERS >= 1131) */

#endif /* (SYSVERS >= 1123) */

#endif /* HPUX */
