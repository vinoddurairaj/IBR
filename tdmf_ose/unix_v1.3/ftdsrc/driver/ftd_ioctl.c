/*-
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
 * $Id: ftd_ioctl.c,v 1.8 2001/12/06 17:30:21 bouchard Exp $
 *
 */

#include <sys/conf.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/file.h>
#if !defined(_AIX)
#include <sys/buf.h>
#endif /* !defined(_AIX) */
#if defined(_AIX)
#include <sys/device.h>
#endif /* defined(_AIX) */

/* ftd_ddi.h must be included after all the system stuff to be ddi compliant */
#include "ftd_kern_ctypes.h"
#include "ftd_ddi.h"
#include "ftd_def.h"
#include "ftd_bab.h"
#include "ftd_bits.h"
#include "ftd_var.h"
#include "ftd_all.h"
#include "ftdio.h"
#include "ftd_klog.h"
#include "ftd_kern_cproto.h"

#define TDMF_TRACE_MAKER
	#include "ftd_trace.h"
#undef TDMF_TRACE_MAKER

extern ftd_void_t *ftd_memset (ftd_void_t * s, ftd_int32_t c, size_t n);

extern ftd_void_t biodone_psdev (struct buf *);

extern ftd_void_t ftd_biodone (struct buf *);

#if defined(notdef)
#if defined(_AIX)
extern ftd_void_t ftd_synctimo (struct trb *);
#else /* defined(_AIX) */
extern ftd_void_t ftd_synctimo (struct wlheader_s *);
#endif /* defined(_AIX) */
#endif /* defined(notdef) */

#if defined(HPUX) && (SYSVERS >= 1100)
extern int ftd_debug;
#endif

#if defined(SOLARIS)
extern int ftd_layered_open (struct dev_ops ** dopp, dev_t * devp);
#endif /* defined(SOLARIS) */

ftd_bitmap_t *lrdbptr;
ftd_uint32_t numupd;

#ifdef TDMF_TRACE
void ftd_ioctl_trace(char *orig, ftd_int32_t cmd)
{
	int i;
	if (cmd && cmd != FTD_GET_GROUP_STATS) /* to not overkill the trace log ... */
		{
		i = (cmd & 0xFF)-1;
		if (i<0 || i >= MAX_TDMF_IOCTL)
			FTD_ERR(FTD_DBGLVL, "Log [%s 0x%08x]: ???",orig, cmd);				
		else
 		    FTD_ERR(FTD_DBGLVL, "Log [%s 0x%08x]: %s", orig, cmd,tdmf_ioctl_text_array[i].cmd_txt);		
		}
}
#endif	    

/*-
 * ftd_clear_bab_and_stats()
 * 
 * free async buffer entries.
 */
FTD_PRIVATE ftd_void_t
ftd_clear_bab_and_stats (ftd_lg_t * lgp)
{
  ftd_dev_t *tempdev;
#if defined(HPUX)
  ftd_int32_t sleeper;
  ftd_context_t context;
#endif /* defined(HPUX) */

  ftd_do_sync_done (lgp);
  lgp->mgr->flags |= FTD_BAB_DONT_USE;
  while (ftd_bab_get_pending (lgp->mgr))
    {
      RELEASE_LOCK (lgp->lock, context);
      /* wait 10ms and try again */
#if defined(HPUX)
      ftd_delay (drv_usectohz (10000), (caddr_t)&sleeper);
#else
      delay (drv_usectohz (10000));
#endif
      ACQUIRE_LOCK (lgp->lock, context);
    }
  lgp->mgr->flags &= ~FTD_BAB_DONT_USE;
  ftd_bab_free_memory (lgp->mgr, 0x7fffffff);

  lgp->wlentries = 0;
  lgp->wlsectors = 0;

  for (tempdev = lgp->devhead; tempdev; tempdev = tempdev->next)
    {
      tempdev->wlentries = 0;
      tempdev->wlsectors = 0;
    }

  return;
}

/*-
 * ftd_get_msb()
 *
 * Get the index of the most significant "set" bit in an integer. Most 
 * processors have a single instruction that does this, but "C" doesn't 
 * expose it. Note: there are faster ways of doing this, but wtf.
 */
FTD_PRIVATE ftd_int32_t
ftd_get_msb (ftd_int32_t num)
{
  ftd_int32_t i;
  ftd_uint32_t temp = (ftd_uint32_t) num;

  for (i = 0; i < 32; i++, temp >>= 1)
    {
      if (temp == 0)
	break;
    }

  return (i - 1);
}

/*-
 * ftd_ctl_get_config()
 *
 * XXX guess this could be deprecated. doesn't compute anything.
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_get_config ()
{
  return (ENOTTY);
}

#if defined(DEPRECATED)
/*-
 * ftd_ctl_get_num_devices()
 *
 * returns the number of configured devices in a logical group
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_get_num_devices (ftd_intptr_t arg)
{
  ftd_int32_t num;
  minor_t minor;
  ftd_lg_t *lgp;
  ftd_dev_t *devp;

#if defined(MISALIGNED)
  num = *(ftd_int32_t *) arg;
#else /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&num, sizeof(num));
#endif /* defined(MISALIGNED) */

  minor = getminor (num);
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (lgp == NULL)
    return (ENXIO);
  num = 0;
  devp = lgp->devhead;
  while (devp)
    {
      devp = devp->next;
      num++;
    }

#if defined(MISALIGNED)
  *((ftd_int32_t *) arg) = num;
#else /* defined(MISALIGNED) */
  bcopy((void *)&num, (void *)arg, sizeof(num));
#endif /* defined(MISALIGNED) */

  return (0);
}

/*-
 * ftd_ctl_get_num_groups()
 * 
 * return the number of configured logical groups 
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_get_num_groups (ftd_intptr_t arg)
{
  ftd_ctl_t *ctlp;
  struct ftd_lg *temp;
  ftd_int32_t num;

  if ((ctlp = ftd_global_state) == NULL)
    return (ENXIO);
  num = 0;
  temp = ctlp->lghead;
  while (temp)
    {
      temp = temp->next;
      num++;
    }

#if defined(MISALIGNED)
  *((ftd_int32_t *) arg) = num;
#else /* defined(MISALIGNED) */
  bcopy((void *)&num, (void *)arg, sizeof(num));
#endif /* defined(MISALIGNED) */

  return (0);
}
#endif /* defined(DEPRECATED) */

/*-
 * ftd_ctl_get_devices_info()
 *
 * return logical device state
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_get_devices_info (ftd_intptr_t arg, ftd_int32_t flag)
{
  ftd_dev_t *devp;
  ftd_lg_t *lgp;
  ftd_dev_info_t *info, ditemp;
  stat_buffer_t sb;
  minor_t minor;

#if defined(MISALIGNED)
  sb = *(stat_buffer_t *) arg;
#else /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&sb, sizeof(sb));
#if defined(SOLARIS) && (SYSVERS >= 570)
  /*
   * Solaris 5.7/5.8:
   * convert from external to internal 
   * representation of dev_t.
   */
   sb.lg_num = DEVEXPL(sb.lg_num);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = getminor (sb.lg_num);
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (lgp == NULL)
    return (ENXIO);

  /* XXX FIXME: Need to check the sb.length to make sure that we're OK */

  info = (ftd_dev_info_t *) sb.addr;
  devp = lgp->devhead;
  while (devp)
    {
#if defined(SOLARIS) && (SYSVERS >= 570)
      /*
       * Solaris 5.7/5.8:
       * convert from internal to external
       * representation of dev_t.
       */
      ditemp.lgnum = DEVCMPL(devp->lgp->dev);
      ditemp.localcdev = DEVCMPL(devp->localcdisk);
      ditemp.cdev = DEVCMPL(devp->cdev);
      ditemp.bdev = DEVCMPL(devp->bdev);
#else  /* defined(SOLARIS) && (SYSVERS >= 570) */
      ditemp.lgnum = devp->lgp->dev;
      ditemp.localcdev = devp->localcdisk;
      ditemp.cdev = devp->cdev;
      ditemp.bdev = devp->bdev;
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
      ditemp.disksize = devp->disksize;
      ditemp.lrdbsize32 = devp->lrdb.len32;
      ditemp.hrdbsize32 = devp->hrdb.len32;
      ditemp.lrdb_res = devp->lrdb.bitsize;
      ditemp.hrdb_res = devp->hrdb.bitsize;
      ditemp.lrdb_numbits = devp->lrdb.numbits;
      ditemp.hrdb_numbits = devp->hrdb.numbits;
      ditemp.statsize = devp->statsize;
      if (ddi_copyout ((caddr_t) & ditemp, (caddr_t) info, sizeof (ditemp), flag))
	return (EFAULT);
      info++;
      devp = devp->next;
    }


  return (0);
}

/*-
 * ftd_ctl_get_groups_info()
 * 
 * return state info for all configured logical groups
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_get_groups_info (ftd_intptr_t arg, ftd_int32_t flag)
{
  minor_t minor;
  ftd_lg_t *lgp;
  ftd_ctl_t *ctlp;
  ftd_lg_info_t *info, lgtemp;
  stat_buffer_t sb;

#if defined(MISALIGNED)
  sb = *(stat_buffer_t *) arg;
#else /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&sb, sizeof(sb));
#if defined(SOLARIS) && (SYSVERS >= 570)
 /*
  * Solaris 5.7/5.8:
  * convert from external to internal
  * representation of dev_t.
  */
  sb.lg_num = DEVEXPL(sb.lg_num);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  if ((ctlp = ftd_global_state) == NULL)
    return (ENXIO);

  /* XXX FIXME: Need to check the sb.length to make sure that we're OK */
  /* But only if lg_num == FTD_CTL */

  info = (ftd_lg_info_t *) sb.addr;
  lgp = ctlp->lghead;
  while (lgp)
    {
      if (sb.lg_num == FTD_CTL)
	{
#if defined(SOLARIS) && (SYSVERS >= 570)
         /*
          * Solaris 5.7/5.8:
          * convert from internal to external
          * representation of dev_t.
          */
	  lgtemp.lgdev = DEVCMPL(lgp->dev);
#else  /* defined(SOLARIS) && (SYSVERS >= 570) */
	  lgtemp.lgdev = lgp->dev;
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
	  lgtemp.statsize = lgp->statsize;
	  if (ddi_copyout ((caddr_t) & lgtemp, (caddr_t) info, sizeof (lgtemp),
			   flag))
	    return (EFAULT);
	  info++;
	}
      else
	{
	  minor = getminor (lgp->dev) & ~FTD_LGFLAG;
	  if (sb.lg_num == minor)
	    {
#if defined(SOLARIS) && (SYSVERS >= 570)
	     /*
	      * Solaris 5.7/5.8:
              * convert from internal to external
              * representation of dev_t.
              */
	      lgtemp.lgdev = DEVCMPL(lgp->dev);
#else  /* defined(SOLARIS) && (SYSVERS >= 570) */
	      lgtemp.lgdev = lgp->dev;
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
	      lgtemp.statsize = lgp->statsize;
	      if (ddi_copyout ((caddr_t) & lgtemp, (caddr_t) info,
			       sizeof (lgtemp), flag))
		return (EFAULT);
	      break;
	    }
	}
      lgp = lgp->next;
    }

  return (0);
}

/*-
 * ftd_ctl_get_device_stats()
 *
 * returns device stats
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_get_device_stats (ftd_intptr_t arg, ftd_int32_t flag)
{
  disk_stats_t temp;
  stat_buffer_t sb;
  ftd_dev_t *devp;
  minor_t minor;

#if defined(MISALIGNED)
  sb = *(stat_buffer_t *) arg;
#else /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&sb, sizeof(sb));
#if defined(SOLARIS) && (SYSVERS >= 570)
 /*
  * Solaris 5.7/5.8:
  * convert from external to internal
  * representation of dev_t.
  */
  sb.dev_num = DEVEXPL(sb.dev_num);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  /* XXX FIXME: Need to check the sb.length to make sure that we're OK */

  /* get the device state */
  minor = getminor (sb.dev_num);
  devp = (ftd_dev_t *) ddi_get_soft_state (ftd_dev_state, U_2_S32(minor));
  if (!devp)
    return (EINVAL);

#if defined(SOLARIS) && (SYSVERS >= 570)
 /*
  * Solaris 5.7/5.8:
  * convert from external to internal
  * representation of dev_t.
  */
  temp.localbdisk = DEVCMPL(devp->localbdisk);
#else  /* defined(SOLARIS) && (SYSVERS >= 570) */
  temp.localbdisk = devp->localbdisk;
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
  temp.localdisksize = devp->disksize;

  temp.readiocnt = devp->readiocnt;
  temp.writeiocnt = devp->writeiocnt;
  temp.sectorsread = devp->sectorsread;
  temp.sectorswritten = devp->sectorswritten;
  temp.wlentries = devp->wlentries;
  temp.wlsectors = devp->wlsectors;

  if (ddi_copyout ((caddr_t) & temp, (caddr_t) sb.addr, sizeof (temp), flag))
    return (EFAULT);

  return 0;
}

/*-
 * ftd_ctl_get_group_stats()
 *
 * return logical group stats
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_get_group_stats (ftd_intptr_t arg, ftd_int32_t flag)
{
  ftd_lg_t *lgp;
  ftd_stat_t temp;
  stat_buffer_t sb;
  minor_t minor;
  ftd_ctl_t *ctlp;

  if ((ctlp = ftd_global_state) == NULL)
    return (ENXIO);

#if defined(MISALIGNED)
  sb = *(stat_buffer_t *) arg;
#else /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&sb, sizeof(sb));
#if defined(SOLARIS) && (SYSVERS >= 570)
 /*
  * Solaris 5.7/5.8:
  * convert from external to internal
  * representation of dev_t.
  */
  sb.lg_num = DEVEXPL(sb.lg_num);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = getminor (sb.lg_num) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (lgp == NULL)
    return (ENXIO);

  /* XXX FIXME: Need to check the sb.length to make sure that we're OK */

  temp.wlentries = lgp->wlentries;
  temp.wlsectors = lgp->wlsectors;
  temp.bab_free =  lgp->wlsectors * DEV_BSIZE;
  temp.bab_used =  ctlp->bab_size - temp.bab_free;
  temp.state = lgp->state;
  temp.ndevs = lgp->ndevs;
  temp.sync_depth = lgp->sync_depth;
  temp.sync_timeout = lgp->sync_timeout;
  temp.iodelay = lgp->iodelay;

  if (ddi_copyout ((caddr_t) & temp, (caddr_t) sb.addr, sizeof (temp), flag))
    return (EFAULT);

  return (0);
}

/*-
 * ftd_ctl_set_group_state()
 * 
 * induce a driver state transition.
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_set_group_state (ftd_intptr_t arg)
{
  minor_t minor;
  ftd_lg_t *lgp;
  ftd_state_t sb;
  ftd_dev_t *tempdev;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

#if defined(MISALIGNED)
  sb = *(ftd_state_t *) arg;
#else /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&sb, sizeof(sb));
#if defined(SOLARIS) && (SYSVERS >= 570)
 /*
  * Solaris 5.7/5.8:
  * convert from external to internal
  * representation of dev_t.
  */
  sb.lg_num = DEVEXPL(sb.lg_num);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */
  minor = getminor (sb.lg_num) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  
  if (lgp == NULL)
    return (ENXIO);
  ACQUIRE_LOCK (lgp->lock, context);
  if (lgp->state == sb.state)
    {
      RELEASE_LOCK (lgp->lock, context);
      return (0);
    }

  switch (lgp->state)
    {
    case FTD_MODE_NORMAL:
      if ((sb.state == FTD_MODE_REFRESH) ||
	  (sb.state == FTD_MODE_TRACKING))
	{
	  /* clear all dirty bits */
	  ftd_clear_dirtybits (lgp);
	}
      else if (sb.state == FTD_MODE_PASSTHRU)
	{
	  ftd_set_dirtybits (lgp);
	  ftd_clear_bab_and_stats (lgp);
	}
      else
	{
	  /* -> Transition to backfresh */
	  /* make lint happy */ /* EMPTY */;
	}
      break;
    case FTD_MODE_REFRESH:
      if (sb.state == FTD_MODE_NORMAL)
	{
	  ftd_clear_dirtybits (lgp);
	}
      else if (sb.state == FTD_MODE_TRACKING)
	{
	  /* clear the journal */
	  ftd_clear_bab_and_stats (lgp);
	}
      else if (sb.state == FTD_MODE_PASSTHRU)
	{
	  ftd_set_dirtybits (lgp);
	  ftd_clear_bab_and_stats (lgp);
	}
      else
	{
	  /* -> Transition to backfresh */
	  /* make lint happy */ /* EMPTY */;
	}
      break;
    case FTD_MODE_TRACKING:
      if (sb.state == FTD_MODE_NORMAL)
	{
	  ftd_clear_dirtybits (lgp);
	  /* update low res dirty bits with journal */
	  ftd_compute_dirtybits (lgp, FTD_LOW_RES_DIRTYBITS);
	}
      else if (sb.state == FTD_MODE_PASSTHRU)
	{
	  ftd_set_dirtybits (lgp);
	  ftd_clear_bab_and_stats (lgp);
	}
      else if (sb.state != FTD_MODE_REFRESH)
	{
	  /* -> Transition to backfresh */
	  /* make lint happy */ /* EMPTY */;
	}
      break;
    case FTD_MODE_PASSTHRU:
      /* FIXME: we should probably do something here */
      if (sb.state == FTD_MODE_REFRESH)
	{
	  ftd_set_dirtybits (lgp);
	}
      else if ((sb.state == FTD_MODE_NORMAL) ||
	       (sb.state == FTD_MODE_TRACKING))
	{
#if defined(DEPRECATE)
/*-
 * XXX
 * this transition happens everytime start runs
 * don't want to clobber bitmaps on startup.
 * deprecate this?
 */
	  ftd_clear_dirtybits (lgp);
#endif /* defined(DEPRECATE) */

	  ftd_clear_bab_and_stats (lgp);
	}
      else
	{
	  /* -> Transition to backfresh */
	  /* make lint happy */ /* EMPTY */;
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
  while (tempdev)
    {
      ftd_flush_lrdb (tempdev);
      tempdev = tempdev->next;
    }

  RELEASE_LOCK (lgp->lock, context);

  ftd_wakeup (lgp);


  return (0);
}

/*-
 * ftd_ctl_get_bab_size()
 *
 * get the actual size of the bab
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_get_bab_size (ftd_intptr_t arg)
{
  ftd_ctl_t *ctlp;

  if ((ctlp = ftd_global_state) == NULL)
    return (ENXIO);

#if defined(MISALIGNED)
  *((ftd_int32_t *) arg) = ctlp->bab_size;
#else /* defined(MISALIGNED) */
  bcopy((void *)&ctlp->bab_size, (void *)arg, sizeof(ctlp->bab_size));
#endif /* defined(MISALIGNED) */

  return (0);
}

/*-
 * ftd_ctl_alloc_minor()
 * 
 * return a unique minor number for each additional device
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_alloc_minor (ftd_intptr_t arg)
{
  ftd_ctl_t *ctlp;
  static ftd_int32_t once = 1;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

  if ((ctlp = ftd_global_state) == NULL)
    return (ENXIO);
  if (once)
    {
      ctlp->minor_index = 0;
      once = 0;
    }

  ACQUIRE_LOCK (ctlp->lock, context);
  ctlp->minor_index++;
#if defined(MISALIGNED)
  *((ftd_int32_t *) arg) = ctlp->minor_index;
#else /* defined(MISALIGNED) */
  bcopy((void *)&ctlp->minor_index, (void *)arg, sizeof(ctlp->minor_index));
#endif /* defined(MISALIGNED) */
  RELEASE_LOCK (ctlp->lock, context);

  return (0);
}

/*-
 * ftd_ctl_update_lrdbs()
 * 
 * update the lrdbs of a group
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_update_lrdbs (ftd_intptr_t arg)
{
  ftd_dev_t_t lg_num;
  minor_t minor;
  ftd_lg_t *lgp;

#if defined(MISALIGNED)
  lg_num = *(ftd_dev_t_t *) arg;
#else /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&lg_num, sizeof(lg_num));
#if defined(SOLARIS) && (SYSVERS >= 570)
 /*
  * Solaris 5.7/5.8:
  * convert from external to internal
  * representation of dev_t.
  */
  lg_num = DEVEXPL(lg_num);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = getminor (lg_num) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (lgp == NULL)
    return (ENXIO);

  ftd_compute_dirtybits (lgp, FTD_LOW_RES_DIRTYBITS);

  return (0);
}

/*-
 * ftd_ctl_update_hrdbs()
 * 
 * update the hrdbs of a group
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_update_hrdbs (ftd_intptr_t arg)
{
  ftd_dev_t_t lg_num;
  minor_t minor;
  ftd_lg_t *lgp;

#if defined(MISALIGNED)
  lg_num = *(ftd_dev_t_t *) arg;
#else /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&lg_num, sizeof(lg_num));
#if defined(SOLARIS) && (SYSVERS >= 570)
 /*
  * Solaris 5.7/5.8:
  * convert from external to internal
  * representation of dev_t.
  */
  lg_num = DEVEXPL(lg_num);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = getminor (lg_num) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (lgp == NULL)
    return (ENXIO);

  ftd_compute_dirtybits (lgp, FTD_HIGH_RES_DIRTYBITS);

  return (0);
}

/*-
 * ftd_ctl_get_group_state()
 * 
 * get the state of the group
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_get_group_state (ftd_intptr_t arg)
{
  ftd_dev_t_t lg_num;
  minor_t minor;
  ftd_lg_t *lgp;
  
#if defined(MISALIGNED)
  lg_num = ((ftd_state_t *) arg)->lg_num;
#else /* defined(MISALIGNED) */
  bcopy((void *)&(((ftd_state_t *)arg)->lg_num), (void *)&lg_num, sizeof(lg_num));
#if defined(SOLARIS) && (SYSVERS >= 570)
 /*
  * Solaris 5.7/5.8:
  * convert from external to internal
  * representation of dev_t.
  */
  lg_num = DEVEXPL(lg_num);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = getminor (lg_num) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (lgp == NULL)
    return (ENXIO);

#if defined(MISALIGNED)
  ((ftd_state_t *) arg)->state = lgp->state;
#else /* defined(MISALIGNED) */
  bcopy((void *)&lgp->state, 
        (void *)&(((ftd_state_t *)arg)->state), sizeof(lgp->state));
#endif /* defined(MISALIGNED) */

  return (0);
}

/*-
 * ftd_ctl_clear_bab()
 * 
 * clear the bab for this group
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_clear_bab (ftd_intptr_t arg)
{
  minor_t minor;
  ftd_lg_t *lgp;
  ftd_dev_t_t dev;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

#if defined(MISALIGNED)
  dev = *(ftd_dev_t_t *) arg;
#else /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&dev, sizeof(dev));
#if defined(SOLARIS) && (SYSVERS >= 570)
 /*
  * Solaris 5.7/5.8:
  * convert from external to internal
  * representation of dev_t.
  */
  dev = DEVEXPL(dev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = getminor (dev) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (lgp == NULL)
    return (ENXIO);

  ACQUIRE_LOCK (lgp->lock, context);
  ftd_clear_bab_and_stats (lgp);
  RELEASE_LOCK (lgp->lock, context);

  return (0);
}

/*-
 * ftd_ctl_clear_dirtybits()
 * 
 * clear dirty bits for this device
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_clear_dirtybits (ftd_intptr_t arg, ftd_int32_t type)
{
  minor_t minor;
  ftd_lg_t *lgp;
  ftd_dev_t *tempdev;
  dev_t dev;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

#if defined(MISALIGNED)
  dev = *(dev_t *) arg;
#else /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&dev, sizeof(dev));

#if defined(SOLARIS) && (SYSVERS >= 570)
 /*
  * Solaris 5.7/5.8:
  * convert from external to internal
  * representation of dev_t.
  */
  dev = DEVEXPL(dev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = getminor (dev) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (lgp == NULL)
    return (ENXIO);

  ACQUIRE_LOCK (lgp->lock, context);

  tempdev = lgp->devhead;
  while (tempdev)
    {
      if (type == FTD_HIGH_RES_DIRTYBITS)
	{
	  bzero ((caddr_t) tempdev->hrdb.map, tempdev->hrdb.len32 * 4);
	}
      else
	{
	  bzero ((caddr_t) tempdev->lrdb.map, tempdev->lrdb.len32 * 4);
	}
      ftd_flush_lrdb (tempdev);
      tempdev = tempdev->next;
    }

  RELEASE_LOCK (lgp->lock, context);

  return (0);
}

/*-
 * ftd_ctl_new_device()
 *
 * instantiate a new device
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_new_device (ftd_intptr_t arg)
{
  ftd_dev_t *softp;
  ftd_lg_t *lgp;
  ftd_dev_info_t info;
  ftd_int32_t diskbits, lrdbbits, hrdbbits, i;
  minor_t dev_minor, lg_minor;
  struct buf *buf;
#if defined(HPUX)
  drv_info_t *drv_info;
#elif defined(SOLARIS)
  major_t emaj, imaj;
  minor_t emin;
#endif

  /* the caller needs to give us: info about the device and the group device
     number */

#if defined(MISALIGNED)
  info = *((ftd_dev_info_t *) arg);
#else /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&info, sizeof(info));
#if defined(SOLARIS) && (SYSVERS >= 570)
 /*
  * Solaris 5.7/5.8:
  * convert from external to internal
  * representation of dev_t.
  */
  info.bdev = DEVEXPL(info.bdev);
  info.cdev = DEVEXPL(info.cdev);
  info.lgnum = DEVEXPL(info.lgnum);
  info.localcdev = DEVEXPL(info.localcdev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  dev_minor = getminor (info.cdev);	/* ??? */

  if (ddi_soft_state_zalloc (ftd_dev_state, dev_minor) != DDI_SUCCESS)
    {

      return (EADDRINUSE);
    }
  softp = (ftd_dev_t *) ddi_get_soft_state (ftd_dev_state, U_2_S32(dev_minor));
  if (softp == NULL)
    return (ENXIO);

  lg_minor = info.lgnum;
  lgp = ddi_get_soft_state (ftd_lg_state, U_2_S32(lg_minor));
  if (lgp == NULL)
    {
      FTD_ERR (FTD_WRNLVL, "Can't get the soft state for logical group %d",
	       lg_minor);
      return (ENXIO);
    }

#if defined(SOLARIS) || defined(HPUX)
  buf = softp->lrdbbp = (struct buf *) &(softp->lrdb_metabuf);
#else /* defined(SOLARIS) || defined(HPUX) */
  buf = softp->lrdbbp = GETRBUF (KM_NOSLEEP);
#endif /* defined(SOLARIS) || defined(HPUX) */
  if (buf == NULL)
    {
      FTD_ERR (FTD_WRNLVL, "Can't allocate buffer for LRT");
      return (ENXIO);
    }
  buf->b_flags = B_BUSY;

  softp->lgp = lgp;

#if defined(HPUX)
  softp->pendingios = softp->highwater = 0;
  softp->pendingbps = NULL;
  softp->localcdisk = info.localcdev;

  drv_info = cdev_drv_info (softp->localcdisk);
  softp->localbdisk = makedev (drv_info->b_major, minor (softp->localcdisk));

  if (cdev_open (softp->localcdisk, FWRITE | FREAD | FNDELAY | FEXCL) != 0)
    {
      ddi_soft_state_free (ftd_dev_state, dev_minor);
      return (ENXIO);
    }
  if (bdev_open (softp->localbdisk, FWRITE | FREAD) != 0)
    {
      cdev_close (softp->localcdisk, 0, 0);
      ddi_soft_state_free (ftd_dev_state, dev_minor);
      return (ENXIO);
    }

  softp->cdev = info.cdev;
  softp->bdev = info.bdev;
#elif defined(SOLARIS)

  emaj = getemajor (info.localcdev);
  emin = geteminor (info.localcdev);
  imaj = etoimajor (emaj);
  softp->localbdisk = makedevice (imaj, emin);
  softp->localcdisk = softp->localbdisk;

  ftd_layered_open (&softp->ld_dop, &softp->localbdisk);
  softp->cdev = makedevice (etoimajor (getemajor (info.cdev)), geteminor (info.cdev));
  softp->bdev = softp->cdev;
#elif defined(_AIX)
  softp->localcdisk = softp->localbdisk =
    makedev (major (info.localcdev), minor (info.localcdev));
  if (fp_opendev (softp->localbdisk, (DREAD | DWRITE | DNDELAY),
		  (ftd_char_t *) NULL, 0, &softp->dev_fp))
    {
      return (ENXIO);
    }
  softp->cdev = info.cdev;
  softp->bdev = info.bdev;
#endif

  softp->disksize = info.disksize;

  diskbits = ftd_get_msb (info.disksize) + 1 + DEV_BSHIFT;
  lrdbbits = ftd_get_msb (info.lrdbsize32 * 4);
  hrdbbits = ftd_get_msb (info.hrdbsize32 * 4);

  lrdbptr = &(softp->lrdb);
  softp->lrdb.bitsize = diskbits - lrdbbits;
  if (softp->lrdb.bitsize < MINIMUM_LRDB_BITSIZE)
    softp->lrdb.bitsize = MINIMUM_LRDB_BITSIZE;
  softp->lrdb.shift = softp->lrdb.bitsize - DEV_BSHIFT;
  softp->lrdb.len32 = info.lrdbsize32;
  softp->lrdb.numbits = (info.disksize +
		       ((1 << softp->lrdb.shift) - 1)) >> softp->lrdb.shift;
  softp->lrdb.map = (ftd_uint32_t *) kmem_zalloc (info.lrdbsize32 * 4, KM_SLEEP);

  softp->hrdb.bitsize = diskbits - hrdbbits;
  if (softp->hrdb.bitsize < MINIMUM_HRDB_BITSIZE)
    softp->hrdb.bitsize = MINIMUM_HRDB_BITSIZE;
  softp->hrdb.shift = softp->hrdb.bitsize - DEV_BSHIFT;
  softp->hrdb.len32 = info.hrdbsize32;
  softp->hrdb.numbits = (info.disksize +
		       ((1 << softp->hrdb.shift) - 1)) >> softp->hrdb.shift;
  softp->hrdb.map = (ftd_uint32_t *) kmem_zalloc (info.hrdbsize32 * 4, KM_SLEEP);


  BP_DEV (buf) = lgp->persistent_store;
  buf->b_un.b_addr = (caddr_t) softp->lrdb.map;
  buf->b_blkno = info.lrdb_offset;
  buf->b_bcount = info.lrdbsize32 * sizeof (ftd_int32_t);

#if defined(HPUX)
  buf->b_flags = 0;
  buf->b_flags |= B_CALL | B_WRITE | B_BUSY | B_ASYNC;
  buf->b_iodone = BIOD_CAST (biodone_psdev);
  BP_SOFTP (buf) = softp;
#if defined(LRDBSYNCH)
  LRDB_FREE_ACK (softp);
#endif /* defined(LRDBSYNCH) */

#elif defined(SOLARIS)
  buf->b_flags |= B_KERNBUF | B_WRITE | B_BUSY;
  buf->b_iodone = BIOD_CAST (biodone_psdev);
  BP_SOFTP (buf) = softp;
#if defined(LRDBSYNCH)
  LRDB_FREE_ACK (softp);
#endif /* defined(LRDBSYNCH) */

#elif defined(_AIX)


  buf->b_flags = 0;
  buf->b_flags |= B_BUSY | B_DONTUNPIN | B_NOHIDE;

  buf->b_iodone = biodone_psdev;

  buf->b_event = EVENT_NULL;

  buf->b_xmemd.aspace_id = XMEM_GLOBAL;

  BP_SOFTP (buf) = softp;

#if defined(LRDBSYNCH)
  LRDB_FREE_ACK (softp);
#endif /* defined(LRDBSYNCH) */
#endif

  softp->statbuf = (ftd_char_t *) kmem_zalloc (info.statsize, KM_SLEEP);
  softp->statsize = info.statsize;

  ALLOC_LOCK (softp->lock, QNM " device specific lock");

  softp->freelist = NULL;
  softp->bufcnt = 0;

  /* Allocate struct buf for later use */
  for (i = 0; i < FTD_MAX_BUF; i++)
    {
      buf = GETRBUF ();
      if (buf == NULL)
	{
	  if (i == 0)
	    FTD_ERR (FTD_WRNLVL, "%s%d: Can't allocate any buffers for I/O\n",
		     DRIVERNAME, dev_minor);
	  break;
	}
      buf->b_flags |= B_BUSY;
      BP_PRIVATE (buf) = softp->freelist;
      softp->freelist = buf;
    }

  /* add device to linked list */
  softp->next = lgp->devhead;
  lgp->devhead = softp;
  lgp->ndevs++;

  return (0);
}

/*-
 * ftd_open_persistent_store()
 *
 * prepare a pstore device for IO
 */
FTD_PRIVATE ftd_int32_t
ftd_open_persistent_store (ftd_lg_t * lgp)
{
#if defined(HPUX)
  dev_t psbdisk, pscdisk;
  drv_info_t *drv_info;
#endif

#if defined(HPUX)
  drv_info = cdev_drv_info (lgp->persistent_store);
  psbdisk = makedev (drv_info->b_major, minor (lgp->persistent_store));
  pscdisk = makedev (drv_info->c_major, minor (lgp->persistent_store));

  if (cdev_open (pscdisk, FWRITE | FREAD | FNDELAY | FEXCL) != 0)
    {
      return (ENXIO);
    }
  if (bdev_open (psbdisk, FWRITE | FREAD) != 0)
    {
      cdev_close (pscdisk, 0, 0);
      return (ENXIO);
    }
#elif defined(SOLARIS)
  ftd_layered_open (&lgp->ps_do, &lgp->persistent_store);
#elif defined(_AIX)
  if (fp_opendev (lgp->persistent_store, (DREAD | DWRITE | DNDELAY),
		  (ftd_char_t *) NULL, 0, &lgp->ps_fp))
    {
      FTD_ERR (FTD_WRNLVL, "ftd_open_persistent_store: returned %d", ENXIO);
      return (ENXIO);
    }
#endif

  return (0);
}

/*-
 * ftd_ctl_new_lg()
 *
 * instantiate a new logical group
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_new_lg (ftd_intptr_t arg)
{
  ftd_lg_info_t info;
  minor_t lg_minor;
  ftd_ctl_t *ctlp;
  ftd_lg_t *lgp;

  Enter (int, FTD_CTL_NEW_LG, dev, cmd, arg, flag, 0);

#if defined(MISALIGNED)
  info = *((ftd_lg_info_t *) arg);
#else /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&info, sizeof(info));
#if defined(SOLARIS) && (SYSVERS >= 570)
  /*
   * Solaris 5.7/5.8:
   * convert from external to internal 
   * representation of dev_t.
   */
   info.lgdev = DEVEXPL(info.lgdev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

#if defined(FTD_DEBUG) && 0
  FTD_DPRINTF (FTD_WRNLVL,
	   "ftd_ctl_new_lg: info.lgdev: 0x%08x info.persistent_store: 0x%08x info.statsize: 0x%08x", info.lgdev, info.persistent_store, info.statsize);
#endif /* defined(FTD_DEBUG) */

  if ((ctlp = ftd_global_state) == NULL)
    Return (ENXIO);

  lg_minor = getminor (info.lgdev) & ~FTD_LGFLAG;

  if (ddi_soft_state_zalloc (ftd_lg_state, lg_minor) != DDI_SUCCESS)
    {

      Return (EADDRINUSE);
    }
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(lg_minor));
  if (lgp == NULL)
    {
#if defined(FTD_DEBUG)
      FTD_DPRINTF (FTD_WRNLVL, "ftd_ctl_new_lg: ddi_get_soft_state(%08x,%08x)\n",
	       ftd_lg_state, lg_minor);
#endif /* defined(FTD_DEBUG) */
      Return (ENOTTY);
    }

  /* allocate a bab manager */
  if ((lgp->mgr = ftd_bab_alloc_mgr ()) == NULL)
    {
      ddi_soft_state_free (ftd_lg_state, lg_minor);
      Return (ENXIO);
    }

  lgp->statbuf = (ftd_char_t *) kmem_zalloc (info.statsize, KM_SLEEP);
  if (lgp->statbuf == NULL)
    {
      ftd_bab_free_mgr (lgp->mgr);
      ddi_soft_state_free (ftd_lg_state, lg_minor);
      Return (ENXIO);
    }

  ALLOC_LOCK (lgp->lock, "ftd lg lock");

  lgp->statsize = info.statsize;

  lgp->ctlp = ctlp;
  lgp->dev = info.lgdev;

  lgp->next = ctlp->lghead;
  ctlp->lghead = lgp;

  lgp->state = FTD_MODE_PASSTHRU;

  lgp->sync_depth = (ftd_uint32_t) - 1;
  lgp->sync_timeout = 0;
  lgp->iodelay = 0;
  lgp->ndevs = 0;
  lgp->dirtymaplag = 0;
#if defined(SOLARIS) && (SYSVERS >= 570)
  lgp->persistent_store = DEVEXPL(info.persistent_store);
#else  /* defined(SOLARIS) && (OSVERS >= 570) */
  lgp->persistent_store = info.persistent_store;
#endif /* defined(SOLARIS) && (OSVERS >= 570) */
  ftd_open_persistent_store (lgp);
  lgp->mgr->pending_io = 0;
#if defined(HPUX) && (SYSVERS >= 1100)
	  lgp->read_waiter_thread = 0;
#endif
  Return (0);
  /* NOTREACHED */
  Exit;
}

/*-
 * ftd_ctl_init_stop()
 *
 * logical group state transition
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_init_stop (ftd_lg_t * lgp)
{
  ftd_dev_t *devp;

  if (!lgp)
    return (ENXIO);

  for (devp = lgp->devhead; devp; devp = devp->next)
    {
      if (devp->flags & FTD_DEV_OPEN)
	{
	  return (EBUSY);
	}
    }
  for (devp = lgp->devhead; devp; devp = devp->next)
    {
      devp->flags |= FTD_STOP_IN_PROGRESS;
    }
  return (0);
}

/*-
 * ftd_ctl_del_device()
 *
 * destroy a logical device
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_del_device (ftd_intptr_t arg)
{
  ftd_dev_t *softp;
  minor_t minor;
  dev_t devdev;
  ftd_dev_t_t tmpdev;

#if defined(MISALIGNED)
  devdev = *(dev_t *) arg;
#else  /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&tmpdev, sizeof(tmpdev));
  devdev = tmpdev;
#if defined(SOLARIS) && (SYSVERS >= 570)
  /*
   * Solaris 5.7/5.8:
   * convert from external to internal 
   * representation of dev_t.
   */
   devdev = DEVEXPL(devdev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */


  minor = getminor (devdev);

  softp = (ftd_dev_t *) ddi_get_soft_state (ftd_dev_state, U_2_S32(minor));


  if (!softp)
    return (ENXIO);


  if (softp->flags & FTD_DEV_OPEN)
    return (EBUSY);


  ftd_del_device (softp, minor);
  return (0);
}

/*-
 * ftd_ctl_del_lg()
 *
 * destroy a logical group
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_del_lg (ftd_intptr_t arg)
{
  minor_t minor;
  ftd_lg_t *lgp;
  dev_t devdev;
  ftd_dev_t_t tmpdev;

#if defined(MISALIGNED)
  devdev = *(dev_t *) arg;
#else  /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&tmpdev, sizeof(tmpdev));
  devdev = tmpdev;
#if defined(SOLARIS) && (SYSVERS >= 570)
  /*
   * Solaris 5.7/5.8:
   * convert from external to internal 
   * representation of dev_t.
   */
   devdev = DEVEXPL(devdev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = getminor (devdev) & ~FTD_LGFLAG;

  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));


  if (!lgp)
    return (ENXIO);


  ftd_del_lg (lgp, minor);

  return (0);
}

/*-
 * ftd_ctl_ctl_config()
 *
 * XXX seems this could be deprecated. doesn't compute anything.
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_ctl_config ()
{
  return (ENOTTY);
}

/*-
 * ftd_ctl_get_dev_state_buffer()
 *
 * return logical device state
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_get_dev_state_buffer (ftd_intptr_t arg, ftd_int32_t flag)
{
  stat_buffer_t sb;
  ftd_dev_t *devp;
  ftd_lg_t *lgp;
  minor_t minor;


#if defined(MISALIGNED)
  stat_buffer_t *sbptr;
  sbptr = (stat_buffer_t *)arg;
  sb = *sbptr;
#else  /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&sb, sizeof(sb));
#if defined(SOLARIS) && (SYSVERS >= 570)
  /*
   * Solaris 5.7/5.8:
   * convert from external to internal 
   * representation of dev_t.
   */
   sb.lg_num = DEVEXPL(sb.lg_num);
   sb.dev_num = DEVEXPL(sb.dev_num);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  /* get the logical group state */
  minor = getminor (sb.lg_num) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (!lgp)
    return (ENXIO);

  /* get the device state */
  minor = getminor (sb.dev_num);
  devp = (ftd_dev_t *) ddi_get_soft_state (ftd_dev_state, U_2_S32(minor));
  if (!devp)
    return (ENXIO);

  if (sb.len != devp->statsize)
    return (EINVAL);

  if (ddi_copyout ((caddr_t) devp->statbuf, (caddr_t) sb.addr, sb.len, flag))
    return (EFAULT);

  return (0);
}

/*-
 * ftd_ctl_get_lg_state_buffer()
 * 
 * return logical group state
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_get_lg_state_buffer (ftd_intptr_t arg, ftd_int32_t flag)
{
  stat_buffer_t sb;
  ftd_lg_t *lgp;
  minor_t minor;

#if defined(MISALIGNED)
  stat_buffer_t *sbptr;
  sbptr = (stat_buffer_t *)arg;
  sb = *sbptr;
#else  /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&sb, sizeof(sb));
#if defined(SOLARIS) && (SYSVERS >= 570)
  /*
   * Solaris 5.7/5.8:
   * convert from external to internal 
   * representation of dev_t.
   */
   sb.lg_num = DEVEXPL(sb.lg_num);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  /* get the logical group state */
  minor = getminor (sb.lg_num) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (!lgp)
    return (ENXIO);
  if (sb.len != lgp->statsize)
    return (EINVAL);
  if (ddi_copyout ((caddr_t) lgp->statbuf, (caddr_t) sb.addr, sb.len, flag))
    return (EFAULT);

  return (0);
}

/*-
 * ftd_ctl_set_dev_state_buffer()
 * 
 * set logical device state
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_set_dev_state_buffer (ftd_intptr_t arg, ftd_int32_t flag)
{
  stat_buffer_t  sb;
  ftd_dev_t *devp;
  ftd_lg_t *lgp;
  minor_t minor;

#if defined(MISALIGNED)
  stat_buffer_t *sbptr;
  sbptr = (stat_buffer_t *)arg;
  sb = *sbptr;
#else  /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&sb, sizeof(sb));


#if defined(SOLARIS) && (SYSVERS >= 570)
  /*
   * Solaris 5.7/5.8:
   * convert from external to internal 
   * representation of dev_t.
   */
   sb.lg_num = DEVEXPL(sb.lg_num);
   sb.dev_num = DEVEXPL(sb.dev_num);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */


#endif /* defined(MISALIGNED) */

  /* get the logical group state */
  minor = getminor (sb.lg_num) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (!lgp)
    return (ENXIO);

  /* get the device state */
  minor = getminor (sb.dev_num);
  devp = (ftd_dev_t *) ddi_get_soft_state (ftd_dev_state, U_2_S32(minor));
  if (!devp)
    return (ENXIO);

  if (sb.len != devp->statsize)
    return (EINVAL);

  if (ddi_copyin ((caddr_t) sb.addr, (caddr_t) devp->statbuf, sb.len, flag))
    return (EFAULT);

  return (0);
}

/*-
 * ftd_ctl_set_dev_state_buffer()
 * 
 * set logical group state
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_set_lg_state_buffer (ftd_intptr_t arg, ftd_int32_t flag)
{
  stat_buffer_t sb;
  ftd_lg_t *lgp;
  minor_t minor;

#if defined(MISALIGNED)
  stat_buffer_t *sbptr;
  sbptr = (stat_buffer_t *)arg;
  sb = *sbptr;
#else  /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&sb, sizeof(sb));
#if defined(SOLARIS) && (SYSVERS >= 570)
  /*
   * Solaris 5.7/5.8:
   * convert from external to internal 
   * representation of dev_t.
   */
   sb.lg_num = DEVEXPL(sb.lg_num);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  /* get the logical group state */
  minor = getminor (sb.lg_num) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (lgp == NULL)
    return (ENXIO);

  if (sb.len != lgp->statsize)
    return (EINVAL);

  if (ddi_copyin ((caddr_t) sb.addr, (caddr_t) lgp->statbuf, sb.len, flag))
    return (EFAULT);
  return (0);
}


/*-
 * ftd_ctl_set_iodelay()
 *
 * set logical group iodelay parameter
 *
 * XXX this should be deprecated
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_set_iodelay (ftd_intptr_t arg)
{
  ftd_param_t vb;
  ftd_lg_t *lgp;
  minor_t minor;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

#if defined(MISALIGNED)
  vb = *(ftd_param_t *) arg;
#else  /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&vb, sizeof(vb));
#if defined(SOLARIS) && (SYSVERS >= 570)
  /*
   * Solaris 5.7/5.8:
   * convert from external to internal 
   * representation of dev_t.
   */
   vb.lgnum = DEVEXPL(vb.lgnum);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = getminor (vb.lgnum) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (lgp == NULL)
    return (ENXIO);
  ACQUIRE_LOCK (lgp->lock, context);
  lgp->iodelay = drv_usectohz (vb.value);
  RELEASE_LOCK (lgp->lock, context);
  return (0);
}

/*-
 * ftd_ctl_set_sync_depth()
 *
 * set logical group sync mode depth parameter
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_set_sync_depth (ftd_intptr_t arg)
{
  ftd_param_t vb;
  ftd_lg_t *lgp;
  minor_t minor;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

#if defined(MISALIGNED)
  vb = *(ftd_param_t *) arg;
#else  /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&vb, sizeof(vb));
#if defined(SOLARIS) && (SYSVERS >= 570)
  /*
   * Solaris 5.7/5.8:
   * convert from external to internal 
   * representation of dev_t.
   */
   vb.lgnum = DEVEXPL(vb.lgnum);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = getminor (vb.lgnum) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (lgp == NULL)
    return (ENXIO);
  if (vb.value == 0)
    return (EINVAL);
  ACQUIRE_LOCK (lgp->lock, context);

  /* Go ahead and release the sleepers in sync mode */
  ftd_do_sync_done (lgp);
  lgp->sync_depth = vb.value;

  RELEASE_LOCK (lgp->lock, context);

  return (0);
}

/*-
 * ftd_ctl_set_sync_timeout()
 *
 * set logical group sync mode timeout parameter
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_set_sync_timeout (ftd_intptr_t arg)
{
  ftd_param_t vb;
  ftd_lg_t *lgp;
  minor_t minor;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

#if defined(MISALIGNED)
  vb = *(ftd_param_t *) arg;
#else  /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&vb, sizeof(vb));
#if defined(SOLARIS) && (SYSVERS >= 570)
  /*
   * Solaris 5.7/5.8:
   * convert from external to internal 
   * representation of dev_t.
   */
   vb.lgnum = DEVEXPL(vb.lgnum);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = getminor (vb.lgnum) & ~FTD_LGFLAG;

  minor = getminor (vb.lgnum) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (lgp == NULL)
    return (ENXIO);
  ACQUIRE_LOCK (lgp->lock, context);
  lgp->sync_timeout = vb.value;
  RELEASE_LOCK (lgp->lock, context);
  return (0);
}

/*-
 * ftd_ctl_start_lg()
 *
 * start the logical group
 * this call should tell the logical group that all initialization is
 * complete and to start journalling or accumulating dirty bits. We should
 * add a bunch of code to make sure that state of the group is complete.
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_start_lg (ftd_intptr_t arg)
{
  dev_t dev;
  ftd_lg_t *lgp;
  minor_t minor;

#if defined(MISALIGNED)
  dev = *(dev_t *) arg;
#else  /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&dev, sizeof(dev));
#if defined(SOLARIS) && (SYSVERS >= 570)
  /*
   * Solaris 5.7/5.8:
   * convert from external to internal 
   * representation of dev_t.
   */
   dev = DEVEXPL(dev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = getminor (dev) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (!lgp)
    return (ENXIO);

    /*-
     * XXX FIXME: to make this work correctly, the init code should pass down 
     * dirtybits and other state stuff from the persistent store and then 
     * we can decide what state to go into. Obviously, all of the state
     * handling in this driver is terribly incomplete. 
     */

  switch (lgp->state)
    {
    case FTD_MODE_PASSTHRU:
    case FTD_MODE_TRACKING:
    case FTD_MODE_REFRESH:
    case FTD_MODE_NORMAL:	/* do nothing */
      break;
    default:
      return (ENOTTY);
    }
  /* Need to somehow flush the state to disk here, no? XXX FIXME XXX */

  return (0);
}

/*-
 * ftd_ctl_ioctl()
 *
 * ioctl(9e) switch for calls using the control device
 */
ftd_int32_t
ftd_ctl_ioctl (dev_t dev, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag)
{
  ftd_int32_t err = 0;

  if (ftd_global_state == NULL)
    return (ENXIO);		/* invalid minor number */

#ifdef TDMF_TRACE
	ftd_ioctl_trace("ctl",cmd);
#endif	    

  	switch (IOC_CST (cmd))
    {
    case IOC_CST (FTD_GET_CONFIG):
      err = ftd_ctl_get_config ();
      break;
    case IOC_CST (FTD_NEW_DEVICE):
      err = ftd_ctl_new_device (arg);
      break;
    case IOC_CST (FTD_NEW_LG):
      err = ftd_ctl_new_lg (arg);
      break;
    case IOC_CST (FTD_DEL_DEVICE):
      err = ftd_ctl_del_device (arg);
      break;
    case IOC_CST (FTD_DEL_LG):
      err = ftd_ctl_del_lg (arg);
      break;
    case IOC_CST (FTD_CTL_CONFIG):
      err = ftd_ctl_ctl_config ();
      break;
    case IOC_CST (FTD_GET_DEV_STATE_BUFFER):
      err = ftd_ctl_get_dev_state_buffer (arg, flag);
      break;
    case IOC_CST (FTD_GET_LG_STATE_BUFFER):
      err = ftd_ctl_get_lg_state_buffer (arg, flag);
      break;
    case IOC_CST (FTD_SET_DEV_STATE_BUFFER):
      err = ftd_ctl_set_dev_state_buffer (arg, flag);
      break;
    case IOC_CST (FTD_SET_LG_STATE_BUFFER):
      err = ftd_ctl_set_lg_state_buffer (arg, flag);
      break;
    case IOC_CST (FTD_GET_DEVICE_NUMS):
      err = ftd_ctl_get_device_nums (dev, arg, flag);
      break;
    case IOC_CST (FTD_START_LG):
      err = ftd_ctl_start_lg (arg);
      break;
#if defined(DEPRECATED)
    case IOC_CST (FTD_GET_NUM_DEVICES):
      err = ftd_ctl_get_num_devices (arg);
      break;
    case IOC_CST (FTD_GET_NUM_GROUPS):
      err = ftd_ctl_get_num_groups (arg);
      break;
#endif /* defined(DEPRECATED) */
    case IOC_CST (FTD_GET_DEVICES_INFO):
      err = ftd_ctl_get_devices_info (arg, flag);
      break;
    case IOC_CST (FTD_GET_GROUPS_INFO):
      err = ftd_ctl_get_groups_info (arg, flag);
      break;
    case IOC_CST (FTD_GET_DEVICE_STATS):
      err = ftd_ctl_get_device_stats (arg, flag);
      break;
    case IOC_CST (FTD_GET_GROUP_STATS):
      err = ftd_ctl_get_group_stats (arg, flag);
      break;
    case IOC_CST (FTD_SET_GROUP_STATE):
      err = ftd_ctl_set_group_state (arg);
      break;
    case IOC_CST (FTD_CLEAR_BAB):
      err = ftd_ctl_clear_bab (arg);
      break;
    case IOC_CST (FTD_CLEAR_LRDBS):
      err = ftd_ctl_clear_dirtybits (arg, FTD_LOW_RES_DIRTYBITS);
      break;
    case IOC_CST (FTD_CLEAR_HRDBS):
      err = ftd_ctl_clear_dirtybits (arg, FTD_HIGH_RES_DIRTYBITS);
      break;
    case IOC_CST (FTD_GET_GROUP_STATE):
      err = ftd_ctl_get_group_state (arg);
      break;
    case IOC_CST (FTD_GET_BAB_SIZE):
      err = ftd_ctl_get_bab_size (arg);
      break;
    case IOC_CST (FTD_UPDATE_LRDBS):
      err = ftd_ctl_update_lrdbs (arg);
      break;
    case IOC_CST (FTD_UPDATE_HRDBS):
      err = ftd_ctl_update_hrdbs (arg);
      break;
    case IOC_CST (FTD_SET_IODELAY):
      err = ftd_ctl_set_iodelay (arg);
      break;
    case IOC_CST (FTD_SET_SYNC_DEPTH):
      err = ftd_ctl_set_sync_depth (arg);
      break;
    case IOC_CST (FTD_SET_SYNC_TIMEOUT):
      err = ftd_ctl_set_sync_timeout (arg);
      break;
    case IOC_CST (FTD_CTL_ALLOC_MINOR):
      err = ftd_ctl_alloc_minor (arg);
      break;

#if defined(HPUX)
      /* We need a way to force a panic on HP for debugging */
    case IOC_CST (FTD_PANIC):
      panic ("DTC induced panic.");
      break;
#endif
    default:
      FTD_ERR(FTD_WRNLVL, 
              "ftd_ctl_ioctl(): ENOTTY: cmd: 0x%08x", IOC_CST(cmd));
      err = ENOTTY;
      break;
    }
  return (err);
}

/*-
 * ftd_lg_send_lg_message()
 *
 * daemon <-> daemon ipc protocol support.
 */
FTD_PRIVATE ftd_int32_t
ftd_lg_send_lg_message (ftd_lg_t * lgp, ftd_intptr_t arg, ftd_int32_t flag)
{
  stat_buffer_t sb;
  ftd_uint64_t *buf, *temp;
  ftd_int32_t size64;
  wlheader_t *hp;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

#if defined(MISALIGNED)
  sb = *(stat_buffer_t *) arg;
#else  /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&sb, sizeof(sb));
#endif /* defined(MISALIGNED) */

  /* enforce that this be a multiple of DEV_BSIZE */
  if (sb.len & (DEV_BSIZE - 1))
    return (EINVAL);
  size64 = sizeof_64bit (wlheader_t) + (sb.len / 8);
  ACQUIRE_LOCK (lgp->lock, context);
  if ((ftd_bab_alloc_memory (lgp->mgr, size64)) != 0)
    {
      temp = lgp->mgr->from[0] - sizeof_64bit(wlheader_t);
      hp = (wlheader_t *) temp;
      hp->majicnum = DATASTAR_MAJIC_NUM;
      hp->offset = (ftd_uint32_t)-1;
      hp->length = sb.len >> DEV_BSHIFT;
      hp->span = 1;  /* since this is a very small write */
      hp->dev = (dev_t)-1;
      hp->diskdev = (dev_t)-1;
#if defined(SOLARIS)
      hp->group_ptr = (ftd_uint64ptr_t) lgp;
#else  /* defined(SOLARIS) */
      hp->group_ptr = lgp;
#endif /* defined(SOLARIS) */
      hp->complete = 1;
      hp->flags = 0;
      hp->bp = 0;
      lgp->wlentries++;
      lgp->wlsectors += hp->length;
      /* Copy the message in */
      if (ddi_copyin ((caddr_t)sb.addr, 
                      (caddr_t) (lgp->mgr->from[0]),
		      sb.len, flag))
	{
	  RELEASE_LOCK (lgp->lock, context);
	  return (EFAULT);
	}
	/*-
         * Don't commit the memory unless we're the last one on the list.
         * Otherwise we may commit part of an outstanding I/O and all hell
         * is likely to break loose.
         */
      if (ftd_bab_get_pending (lgp->mgr) == (ftd_uint64_t *) hp)
	{
          if (hp->majicnum != DATASTAR_MAJIC_NUM)
             panic("FTD: ftd_lg_send_lg_message: BAD DATASTAR_MAJIC_NUM");

	  ftd_bab_commit_memory (lgp->mgr, size64);
	}
  }
  else

    {
      RELEASE_LOCK (lgp->lock, context);
      return (EAGAIN);
    }
  RELEASE_LOCK (lgp->lock, context);
  return (0);
}

/*-
 * ftd_lg_oldest_entries()
 *
 * return journal indices/entries for a migration pass
 */
FTD_PRIVATE ftd_int32_t
ftd_lg_oldest_entries (ftd_lg_t * lgp, ftd_intptr_t arg)
{
  oldest_entries_t oe;
  ftd_int32_t len64, destlen64, offset64, err;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

#if defined(MISALIGNED)
  oe = *(oldest_entries_t *) arg;
#else  /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&oe, sizeof(oe));
#endif /* defined(MISALIGNED) */

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
  ACQUIRE_LOCK (lgp->lock, context);
  len64 = ftd_bab_get_committed_length (lgp->mgr);
  RELEASE_LOCK (lgp->lock, context);

  if (len64 == 0)
    {
      oe.retlen32 = 0;
      err = 0;
    }
  else
    {
      len64 -= offset64;
      if (len64 > 0)
	{
	  if (destlen64 > len64)
	    {
	      destlen64 = len64;
	    }
	  /* copy the data */
	  ftd_bab_copy_out (lgp->mgr, offset64, (ftd_uint64_t *) oe.addr,
			    destlen64);
	  oe.retlen32 = destlen64 * 2;
	  err = 0;
	}
      else
	{
	  oe.retlen32 = 0;
	  err = EINVAL;
	}
    }
  oe.state = lgp->state;

#if defined(MISALIGNED)
  *((oldest_entries_t *) arg) = oe;
#else  /* defined(MISALIGNED) */
  bcopy((void *)&oe, (void *)arg, sizeof(oe));
#endif /* defined(MISALIGNED) */

  return (err);
}

/*-
 * ftd_lookup_dev()
 *
 * return a device state struct, given
 * a logical group and minor number,
 */
FTD_PRIVATE ftd_dev_t *
ftd_lookup_dev (ftd_lg_t * lgp, dev_t d)
{
  ftd_dev_t *softp;


  softp = lgp->devhead;
  while (softp)
    {

      if (softp->cdev == d)
	return (softp);
      softp = softp->next;
    }
  return (0);
}

/*-
 * ftd_lg_update_dirtybits()
 * 
 * if we are in journal mode, update the lrdb with the contents of the
 * journal. flush the lrdb to disk or do we just wait for the next I/O?
 * i say we wait, since a false positive won't hurt too much.
 */
FTD_PRIVATE ftd_int32_t
ftd_lg_update_dirtybits (ftd_lg_t * lgp)
{
  ftd_dev_t *tempdev;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

  /* grab the group lock */
  ACQUIRE_LOCK (lgp->lock, context);

  /* we can only do this if we are in the proper state */
  if (lgp->state != FTD_MODE_NORMAL)
    {
      RELEASE_LOCK (lgp->lock, context);
      return (EINVAL);
    }

  tempdev = lgp->devhead;
  while (tempdev)
    {
      bzero ((caddr_t) tempdev->lrdb.map, tempdev->lrdb.len32 * 4);
      bzero ((caddr_t) tempdev->hrdb.map, tempdev->hrdb.len32 * 4);
      tempdev = tempdev->next;
    }

  /* compute the drtybits */
  ftd_compute_dirtybits (lgp, FTD_LOW_RES_DIRTYBITS);
  ftd_compute_dirtybits (lgp, FTD_HIGH_RES_DIRTYBITS);

  /* release the group lock */
  RELEASE_LOCK (lgp->lock, context);

#if defined(notdef)
    /*-
     * XXX
     * found this commented out, without commentary.
     * deprecate it?
     */
  for (tempdev = lgp->devhead; tempdev; tempdev = tempdev->next)
    {
      ftd_flush_lrdb (tempdev);
    }
#endif /* defined(notdef) */

  return (0);
}

/*-
 * ftd_lg_migrate()
 *
 * called to acknowledge completed migration pass
 */
FTD_PRIVATE ftd_int32_t
ftd_lg_migrate (ftd_lg_t * lgp, ftd_intptr_t arg)
{
  ftd_dev_t *softp;
  ftd_uint64_t *temp;
  wlheader_t *wl;
  migrated_entries_t me;
  bab_buffer_t *buf;
  ftd_uint32_t bytes;
  dev_t lastdev;
  ftd_int32_t len64, nlen64, i;

#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

#if defined(MISALIGNED)
  me = *(migrated_entries_t *) arg;
#else  /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&me, sizeof(me));
#endif /* defined(MISALIGNED) */

  if (me.bytes == 0)
    return (0);

  if (me.bytes & 0x7)
    {
      FTD_ERR (FTD_WRNLVL, "Tried to migrate %d bytes, which is illegal", me.bytes);
      return (EINVAL);
    }

  if (lgp->mgr->in_use_head == NULL)
    {
      FTD_ERR (FTD_WRNLVL, "Trying to migrate when we have no data!");
      return (EINVAL);
    }


  /* FIXME: revisit this mutex as well as the bab mutex. */
  ACQUIRE_LOCK (lgp->lock, context);

    /*-
     * Walk through the entries being migrated, calling biodone(hp->bp)
     * if hp->bp is not NULL.  We do this to implement sync mode.
     * async mode is implemented in exactly the same way as sync mode,
     * but with a depth of 0xffffffff, which cannot be exceeded by 
     * definition of int.
     */
  BAB_MGR_FIRST_BLOCK (lgp->mgr, buf, temp);
/** TEMP + */
    if(((wlheader_t *)temp)->majicnum != DATASTAR_MAJIC_NUM) {
      FTD_ERR(FTD_WRNLVL, "Frag Buffer: temp=%x\n", temp);
    }
/** TEMP - */
  bytes = me.bytes;
  lastdev = (dev_t)-1;
  softp = NULL;
  while (temp && bytes > 0)
    {
      wl = (wlheader_t *) temp;


      if (lgp->wlentries <= 0)
	{
	  FTD_ERR (FTD_WRNLVL, "%s: Trying to migrate too many entries", DRIVERNAME);
	  break;
	}
      if (lgp->wlsectors <= 0)
	{
	  FTD_ERR (FTD_WRNLVL, "%s: Trying to migrate too many sectors", DRIVERNAME);
	  break;
	}
      if (wl->majicnum != DATASTAR_MAJIC_NUM)
	{
	  FTD_ERR (FTD_WRNLVL, "%s: corrupted BAB entry migrated", DRIVERNAME);
	  break;
	}
      if (wl->complete == 0)
	{
	  FTD_ERR (FTD_WRNLVL, "%s: Trying to migrate uncompleted entry",
		   DRIVERNAME);
	}
      if (lgp->wlentries == 0)
	{
	  FTD_ERR (FTD_WRNLVL, "%s: Migrated too many entries!", DRIVERNAME);
	  break;
	}
      if (lgp->wlsectors < wl->length)
	{
	  FTD_ERR (FTD_WRNLVL, "%s: Migrated too many sectors!", DRIVERNAME);
	  break;
	}
      if (wl->bp)
	{
	  /* cancel any sync mode timout */
#if defined(SOLARIS)
	  if (wl->timoid != INVALID_TIMEOUT_ID)
	    {
#if (SYSVERS >= 570)
	      timeout_id_t passtimoid=(timeout_id_t)wl->timoid;
	      untimeout (passtimoid);
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
	  /* dequeue this entry from timer queue */
#if (SYSVERS >= 1100)
          if (ftd_debug == 5)
	    FTD_ERR (FTD_WRNLVL, "ftd_lg_migrate: ftd_timer_dequeue");
#endif
	  ftd_timer_dequeue (lgp, wl);

#endif /* defined(SOLARIS) */
	  {
	  struct buf *passbp = (struct buf *)wl->bp;
	  ftd_biodone (passbp);
	  }
	}
      wl->bp = 0;
      if (wl->dev != lastdev)
	{
	  softp = ftd_lookup_dev (lgp, wl->dev);
	  lastdev = wl->dev;
	}
      if (softp)
	{
	  softp->wlentries--;
	  softp->wlsectors -= wl->length;
	}
      bytes -= ((wl->length << (DEV_BSHIFT - 3)) +
		sizeof_64bit (wlheader_t)) << 3;
      lgp->wlentries--;
      lgp->wlsectors -= wl->length;
      /*BAB_MGR_NEXT_BLOCK (temp, wl, buf); */
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
  if (bytes > 0)
    FTD_ERR (FTD_WRNLVL, "Too little data migrated");
  else if ((bytes != 0) && (bytes > 0)) /* make lint happy with (bytes < 0) */
    FTD_ERR (FTD_WRNLVL, "Too much data migrated");

  /* bab_free wants 64-bits words. */
  ftd_bab_free_memory (lgp->mgr, me.bytes >> 3);

    /*-
     * we want to occasionally reset dirty bits based on what is
     * really dirty in the bab (so that we don't end up with a 
     * completely dirty bitmap over time) 
     */
  lgp->dirtymaplag++;
  RELEASE_LOCK (lgp->lock, context);
  if (lgp->dirtymaplag == FTD_DIRTY_LAG && lgp->wlsectors < FTD_RESET_MAX)
    {
      lgp->dirtymaplag = 0;
      ftd_lg_update_dirtybits (lgp);
    }

  return (0);
}


/*-
 * ftd_lg_get_dirty_bits()
 *
 * for a given device, returns HRT map
 */
FTD_PRIVATE ftd_int32_t
ftd_lg_get_dirty_bits (ftd_lg_t * lgp, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag)
{
  ftd_int32_t i, offset32;
  ftd_dev_t_t tmpdev;
  dev_t dev;
  ftd_dev_t *softp;
  dirtybit_array_kern_t dbarray;
  ftd_int32_t *tmpdbbuf;

#if defined(MISALIGNED)
  dbarray = *(dirtybit_array_kern_t *) arg;
#else  /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&dbarray, sizeof(dbarray));
  tmpdbbuf = (ftd_int32_t *)dbarray.dbbuf;
#endif /* defined(MISALIGNED) */

    /*-
     * The following tricky bit of logic is intended to return ENOENT
     * if the bits are invalid.  The bits are valid only in RECOVER mode
     * and when in TRACKING mode with no entries in the writelog
     */
  if (dbarray.state_change &&
      ((lgp->state & FTD_M_BITUPDATE) == 0 || lgp->wlentries != 0))
    return (ENOENT);

  offset32 = 0;
  for (i = 0; i < 1; i++)
    {

       dev = dbarray.dev;


#if defined(SOLARIS) && (SYSVERS >= 570)
	dev = DEVEXPL(dev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */


      /* find the matching device in our logical group */
      softp = lgp->devhead;
      while (softp != NULL)
	{
	  if (dev == softp->cdev)
	    {
	      if (IOC_CST (cmd) == IOC_CST (FTD_GET_LRDBS))
		{
		  if (ddi_copyout ((caddr_t) softp->lrdb.map,
				   (caddr_t) (tmpdbbuf + offset32),
				   softp->lrdb.len32 * 4, flag))
		    return (EFAULT);
		  offset32 += softp->lrdb.len32;
		}
	      else
		{
		  if (ddi_copyout ((caddr_t) softp->hrdb.map,
				   (caddr_t) (tmpdbbuf + offset32),
				   softp->hrdb.len32 * 4, flag))
		    return (EFAULT);
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

  return (0);
}

#if defined(DEPRECATED)
/*-
 * ftd_lg_get_dirty_bit_info()
 *
 * return an array of devices and bitmap size sizes for each device in
 * the logical group. 
 */
FTD_PRIVATE ftd_int32_t
ftd_lg_get_dirty_bit_info (ftd_lg_t * lgp, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag)
{
  dirtybit_array_kern_t dbarray;
  ftd_int32_t i;
  ftd_dev_t *softp;

  /* XXX FIXME: Need to check the length to make sure that we're OK */
  /* XXX Maybe we don't have enough info? */

#if defined(MISALIGNED)
  dbarray = *(dirtybit_array_kern_t *) arg;
#else  /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&dbarray, sizeof(dbarray));
#endif /* defined(MISALIGNED) */

    /*-
     * get the number of devices in the group and make sure the buffer
     * is big enough 
     */
  for (i = 0, softp = lgp->devhead; softp != NULL; i++)
    {
      softp = softp->next;
    }
  if (ddi_copyout ((caddr_t) & i, (caddr_t) & dbarray.numdevs, sizeof (ftd_int32_t), flag))
    return (EFAULT);

  if (i > dbarray.numdevs)
    return (EINVAL);

  softp = lgp->devhead;
  for (i = 0; softp != NULL; i++)
    {
      if (ddi_copyout ((caddr_t) & softp->cdev, (caddr_t) (dbarray.devs + i),
		       sizeof (dev_t), flag))
	return (EFAULT);
      if (IOC_CST (cmd) == IOC_CST (FTD_GET_LRDB_INFO))
	{
	  if (ddi_copyout ((caddr_t) & softp->lrdb.len32,
		 (caddr_t) (dbarray.dbbuf + i), sizeof (ftd_int32_t), flag))
	    return (EFAULT);
	}
      else
	{			/* IOC_CST(FTD_GET_HRDB_INFO) */
	  if (ddi_copyout ((caddr_t) & softp->hrdb.len32,
		 (caddr_t) (dbarray.dbbuf + i), sizeof (ftd_int32_t), flag))
	    return (EFAULT);
	}
      softp = softp->next;
    }

  return (0);
}
#endif /* defined(DEPRECATED) */

/*-
 * ftd_lg_set_dirty_bits()
 * 
 * given an array of devices and dirty bitmaps, copy the dirty bitmaps into
 * our bitmaps.
 */
FTD_PRIVATE ftd_int32_t
ftd_lg_set_dirty_bits (ftd_lg_t * lgp, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag)
{
  dirtybit_array_kern_t dbarray;
  ftd_dev_t_t tmpdev;
  dev_t dev;
  ftd_int32_t i, offset32;
  ftd_dev_t *softp;
  ftd_int32_t err;
  ftd_int32_t *tmpdbbuf;

#if defined(MISALIGNED)
  dbarray = *(dirtybit_array_kern_t *) arg;
#else  /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&dbarray, sizeof(dbarray));
  tmpdbbuf = (ftd_int32_t *)dbarray.dbbuf;
#endif /* defined(MISALIGNED) */

  offset32 = 0;
  for (i = 0; i < 1; i++)
    {

      dev = dbarray.dev;


#if defined(SOLARIS) && (SYSVERS >= 570)
   dev = DEVEXPL(dev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */


      if ((softp = ftd_lg_get_device (lgp, dev)) != NULL)
	{
	  if (IOC_CST (cmd) == IOC_CST (FTD_SET_LRDBS))
	    {
	      if (ddi_copyin ((caddr_t) (tmpdbbuf + offset32),
		    (caddr_t) softp->lrdb.map, softp->lrdb.len32 * 4, flag))
		return (EFAULT);
	      offset32 += softp->lrdb.len32;
	    }
	  else
	    {
	      if (ddi_copyin ((caddr_t) (tmpdbbuf + offset32),
		    (caddr_t) softp->hrdb.map, softp->hrdb.len32 * 4, flag))
		return (EFAULT);
	      offset32 += softp->hrdb.len32;
	    }
	}
      else
	{
	  return (ENXIO);
	}
    }

  return (0);
}

/*-
 * ftd_lg_ioctl()
 *
 * ioctl(9e) switch for calls using the lg control device
 */
ftd_int32_t
ftd_lg_ioctl (dev_t dev, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag)
{
  ftd_int32_t err = 0;
  ftd_lg_t *lgp;

  minor_t minor = getminor (dev) & ~FTD_LGFLAG;
  if ((lgp = ddi_get_soft_state (ftd_lg_state, U_2_S32(minor))) == NULL)
    {
      return (ENXIO);		/* invalid minor number */
    }

#ifdef TDMF_TRACE
	ftd_ioctl_trace("lg",cmd);
#endif	    


  switch (IOC_CST (cmd))
    {
    case IOC_CST (FTD_OLDEST_ENTRIES):
      err = ftd_lg_oldest_entries (lgp, arg);
      break;
    case IOC_CST (FTD_MIGRATE):
      err = ftd_lg_migrate (lgp, arg);
      break;
    case IOC_CST (FTD_GET_LRDBS):
    case IOC_CST (FTD_GET_HRDBS):
      err = ftd_lg_get_dirty_bits (lgp, cmd, arg, flag);
      break;
    case IOC_CST (FTD_SET_LRDBS):
    case IOC_CST (FTD_SET_HRDBS):
      err = ftd_lg_set_dirty_bits (lgp, cmd, arg, flag);
      break;
#if defined(DEPRECATED)
    case IOC_CST (FTD_GET_LRDB_INFO):
    case IOC_CST (FTD_GET_HRDB_INFO):
      err = ftd_lg_get_dirty_bit_info (lgp, cmd, arg, flag);
      break;
#endif /* defined(DEPRECATED) */
    case IOC_CST (FTD_UPDATE_DIRTYBITS):
      err = ftd_lg_update_dirtybits (lgp);
      break;
    case IOC_CST (FTD_SEND_LG_MESSAGE):
      err = ftd_lg_send_lg_message (lgp, arg, flag);
      break;
    case IOC_CST (FTD_INIT_STOP):
      err = ftd_ctl_init_stop (lgp);
      break;
    default:
      FTD_ERR(FTD_DBGLVL, "ftd_lg_ioctl(): ENOTTY: cmd: 0x%08x", cmd);
      err = ENOTTY;
      break;
    }
  return (err);
}

/*-
 * biodone_psdev()
 *
 * service pstore device IO completion event 
 */
#if defined(_AIX)
/* _AIX devstrat() wants this */
ftd_void_t
biodone_psdev (bp)
     struct buf *bp;
{
#if defined(LRDBSYNCH)
  {
    ftd_int32_t err;
    ftd_int32_t nval = 0;
    ftd_int32_t oval = LRDB_STAT_WANT;
    ftd_dev_t *softp = BP_SOFTP (bp);

    bp->b_flags |= B_DONE;

	/*-
	 * asynch error notification...
	 */
    if ((err = geterror (bp)) != 0)
      {
	softp->lrdb_errcnt++;
	FTD_ERR (FTD_WRNLVL,
	 "biodone_psdev: LRT map IO error. dev: 0x%08x blk: 0x%08x err: %d",
		 bp->b_dev, bp->b_blkno, err);
      }

    softp->lrdb_finicnt++;

    if (compare_and_swap (&(softp->lrdb_stat), &oval, nval) == TRUE)
      {

	/* reinitiate LRT map IO */

	LRDB_WANT_ACK (softp);

	softp->lrdb_reiocnt++;

	softp->lrdbbp->b_forw =
	  softp->lrdbbp->b_back =
	  softp->lrdbbp->av_forw =
	  softp->lrdbbp->av_back = 0;
	softp->lrdbbp->b_work =
	  softp->lrdbbp->b_resid =
	  softp->lrdbbp->b_flags = 0;

	softp->lrdbbp->b_flags |= B_BUSY | B_DONTUNPIN | B_NOHIDE;

	softp->lrdbbp->b_iodone = biodone_psdev;

	softp->lrdbbp->b_event = EVENT_NULL;

	softp->lrdbbp->b_xmemd.aspace_id = XMEM_GLOBAL;

	devstrat (softp->lrdbbp);

      }
    else
      {
	/* LRT map not wanted, make it available */

	LRDB_FREE_ACK (softp);
      }
  }
#endif /* defined(LRDBSYNCH) */

}
#endif /* defined(_AIX) */

#if defined(SOLARIS) || defined(HPUX)

#if defined(HPUX)
static lock_t *atomic_l;
#endif /* defined(HPUX) */

#if defined(SOLARIS)
static kmutex_t atomic_l;
#endif /* defined(SOLARIS) */

static ftd_int32_t atomic_l_inited = 0;

/*- 
 * compare_and_swap()
 *
 * emulate _AIX compare_and_swap
 */
ftd_int32_t
compare_and_swap (waddr, oval, nval)
     ftd_uint32_t *waddr;
     ftd_uint32_t *oval;
     ftd_uint32_t nval;
{
  ftd_int32_t rval;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

  if (atomic_l_inited == 0)
    {
      ALLOC_LOCK (atomic_l, "atomic ops");
      atomic_l_inited = 1;
    }

  ACQUIRE_LOCK (atomic_l, context);
  if (*waddr == *oval)
    {
      *waddr = nval;
      rval = TRUE;
    }
  else
    {
      *oval = *waddr;
      rval = FALSE;
    }
  RELEASE_LOCK (atomic_l, context);
  return (rval);
}

/*- 
 * fetch_and_and()
 *
 * emulate _AIX fetch_and_and
 */
ftd_int32_t
fetch_and_and (waddr, mask)
     ftd_uint32_t *waddr;
     ftd_uint32_t mask;
{
  ftd_uint32_t rval = *waddr;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

  if (atomic_l_inited == 0)
    {
      ALLOC_LOCK (atomic_l, "atomic ops");
      atomic_l_inited = 1;
    }
  ACQUIRE_LOCK (atomic_l, context);
  *waddr &= ~mask;
  RELEASE_LOCK (atomic_l, context);
  return (rval);

}

/*- 
 * fetch_and_or()
 *
 * emulate _AIX fetch_and_or
 */
ftd_int32_t
fetch_and_or (waddr, mask)
     ftd_uint32_t *waddr;
     ftd_uint32_t mask;
{
  ftd_uint32_t rval = *waddr;
#if defined(HPUX)
  ftd_context_t context;
#endif /* defined(HPUX) */

  if (atomic_l_inited == 0)
    {
      ALLOC_LOCK (atomic_l, "atomic ops");
      atomic_l_inited = 1;
    }
  ACQUIRE_LOCK (atomic_l, context);
  *waddr |= mask;
  RELEASE_LOCK (atomic_l, context);
  return (rval);

}

/*-
 * biodone_psdev()
 *
 * service pstore device IO completion event 



 */
 ftd_void_t
biodone_psdev (bp)
     struct buf *bp;
{
  ftd_int32_t err;
  ftd_dev_t *softp = BP_SOFTP (bp);


#if defined(LRDBSYNCH)
  {
    ftd_int32_t cmpswerr;
    ftd_uint32_t nval = 0;
    ftd_uint32_t oval = LRDB_STAT_WANT;

	/*-
	 * asynch error notification...
	 */
    if ((err = geterror (bp)) != 0)
      {
	FTD_ERR (FTD_WRNLVL,
	 "biodone_psdev: LRT map IO error. dev: 0x%08x blk: 0x%08x err: %d",
		 bp->b_dev, bp->b_blkno, err);
      }

    softp->lrdb_finicnt++;

    cmpswerr = compare_and_swap (&(softp->lrdb_stat), &oval, nval);
    if (cmpswerr == TRUE)
      {

	/* reinitiate LRT map IO */
	softp->lrdb_reiocnt++;

	LRDB_WANT_ACK (softp);


#if defined(SOLARIS)
	softp->lrdbbp->b_flags = B_KERNBUF | B_WRITE | B_BUSY;
	softp->lrdbbp->b_iodone = BIOD_CAST (biodone_psdev);
#endif /* defined(SOLARIS) */

#if defined(HPUX)
	softp->lrdbbp->b_flags = 0;
	softp->lrdbbp->b_flags = B_CALL | B_WRITE | B_BUSY | B_ASYNC;
	softp->lrdbbp->b_iodone = BIOD_CAST (biodone_psdev);
#endif /* defined(HPUX) */

	bdev_strategy (softp->lrdbbp);

      }
    else
      {
	/* LRT map not wanted, make it available */
	softp->lrdb_freecnt++;

	LRDB_FREE_ACK (softp);
      }
  }
#endif /* defined(LRDBSYNCH) */

}
#endif /* defined(SOLARIS) || defined(HPUX) */

