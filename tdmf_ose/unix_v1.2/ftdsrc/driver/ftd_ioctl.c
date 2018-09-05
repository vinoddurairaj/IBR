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
 * $Id: ftd_ioctl.c,v 1.140 2018/02/11 02:22:01 paulclou Exp $
 */

#if defined(linux)

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
#include <generated/autoconf.h>  // WR PROD11894 SuSE 11.2
#else
#include <linux/autoconf.h>
#endif

#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/sysctl.h>
#include <linux/sched.h>
#include <linux/file.h>
#include "ftd_linux.h"
#include "ftd_linux_proc_fs.h"
#else
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
#include <sys/fp_io.h>
#include <sys/atomic_op.h>
#include <sys/sysmacros.h>
#endif /* defined(_AIX) */
#endif /* defined(linux) */

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
#include "ftd_pending_ios_monitoring.h"
#include "ftd_dynamic_activation.h"

// The linux include depends on LINUX260 which is defined in ftd_def.h
#if defined (linux)
    #include <linux/blkdev.h>
#endif

#define TDMF_TRACE_MAKER
	#include "ftd_trace.h"
#undef TDMF_TRACE_MAKER

extern ftd_void_t *ftd_memset (ftd_void_t * s, ftd_int32_t c, size_t n);

#if !defined(linux)
extern ftd_void_t biodone_psdev (struct buf *);
#endif

#if defined(linux)
extern ftd_void_t ftd_biodone (struct buf *, int ioerror);
#else
extern ftd_void_t ftd_biodone (struct buf *);
#endif

#if defined(linux)
/* -- temporary fix -- start -- */
#define CE_CONT       0		/* for FTD_DBGLVL */
#define DDI_SUCCESS   0		/* for ddi_soft_state_zalloc */
#define B_BUSY        0x0001
/* -- temporary fix --  end  -- */
#endif /* defined(linux) */

extern ftd_int32_t ftd_bab_free_size(void);

ftd_lg_map_t  Started_LG_Map;
ftd_uchar_t Started_LGS[MAXLG];

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
#if 0
	int i;
	if (cmd && cmd != FTD_GET_GROUP_STATS) /* to not overkill the trace log ... */
		{
		i = (cmd & 0xFF)-1;
		if (i<0 || i >= MAX_TDMF_IOCTL)
			FTD_ERR(FTD_DBGLVL, "Log [%s 0x%08x]: ???",orig, cmd);				
		else
 		    FTD_ERR(FTD_DBGLVL, "Log [%s 0x%08x]: %s", orig, cmd,tdmf_ioctl_text_array[i].cmd_txt);		
		}
#endif
}
#endif

static int check_if_is_power_of_2( ftd_int32_t value )
{
  unsigned int mask = 0x80000000;

  // Find the most significant bit set in the value and check all the lower bits
  while( mask >= 0x00000002 )
  {
    if( value & mask )
	{
	  // Found the most significant bit set.
      // Check if some lower bit is set (not power of 2 then).
	  if( value & ~mask )
	  {
        return( 0 ); // Not power of 2
	  }
	  else
	  {
	    return( 1 ); // Power of 2
	  }
	}
	mask >>= 1;
  }
  return( 0 ); // No bit set (0 is not power of 2) or value == 1.
}

/*-
 * ftd_clear_bab_and_stats()
 * 
 * free async buffer entries.
 */
#define	ftd_clear_bab_and_stats(lgp, context) \
{ \
  ftd_dev_t *tempdev; \
 \
  lgp->mgr->flags |= FTD_BAB_DONT_USE; \
  ftd_do_sync_done (lgp); \
  while (ftd_bab_get_pending (lgp->mgr)) \
    { \
      RELEASE_LOCK (lgp->lock, context); \
      FTD_DELAY(10000); \
      ACQUIRE_LOCK (lgp->lock, context); \
    } \
  lgp->mgr->flags &= ~FTD_BAB_DONT_USE; \
  ftd_bab_free_memory (lgp->mgr, 0x7fffffff); \
 \
  lgp->wlentries = 0; \
  lgp->wlsectors = 0; \
 \
  for (tempdev = lgp->devhead; tempdev; tempdev = tempdev->next) \
    { \
      tempdev->wlentries = 0; \
      tempdev->wlsectors = 0; \
    } \
    lgp->ulPendingByteCount = 0; \
    lgp->ulCommittedByteCount = 0; \
    lgp->ulMigratedByteCount = 0; \
}


FTD_PRIVATE ftd_int32_t ftd_ctl_get_syschk_counters(ftd_intptr_t arg);
FTD_PRIVATE ftd_int32_t	ftd_ctl_clear_syschk_counters(ftd_intptr_t arg);


/**
 * @brief Obtains the physical block device's hardware sector size as the binary bit order of the value.
 *
 * @param physical_block_device  The block device for which we want the hardware sector size.
 *
 * @return The binary bit order of the hardware sector size.
 *         I.E. 2 ^ Return value = sector size in bytes.
 *
 * @todo Implement something similar for all other platforms than linux and move the function to the proper os specific file!
 */
FTD_PRIVATE int ftd_get_hardware_sector_size_bit_order(dev_t physical_block_device)
{
   int hardware_sector_size_bit_order =  DEV_BSHIFT; // Default value
#if defined (LINUX260)
   struct block_device *bdev = bdget(physical_block_device);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32)
   hardware_sector_size_bit_order = get_bitmask_order(bdev_hardsect_size(bdev)) - 1; // get_bitmask_order's least significant bit has index number 1.
#else
   hardware_sector_size_bit_order =  get_bitmask_order(bdev_logical_block_size(bdev)) - 1;
#endif

#endif

   return hardware_sector_size_bit_order;
}

/*-
 * ftd_get_msb()
 *
 * Get the index of the most significant "set" bit in an integer. Most 
 * processors have a single instruction that does this, but "C" doesn't 
 * expose it. Note: there are faster ways of doing this, but wtf.
 */
FTD_PRIVATE ftd_int64_t
ftd_get_msb (ftd_int64_t num)
{
  ftd_int32_t i;
  ftd_uint64_t temp = (ftd_uint64_t) num;

  for (i = 0; i < 64; i++, temp >>= 1)
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
#elif defined(_AIX)
  /* lg_num is left as it is. */
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = getminor (sb.lg_num);
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (lgp == NULL)
    return (ENXIO);

  /* XXX FIXME: Need to check the sb.length to make sure that we're OK */

  info = (ftd_dev_info_t *)(unsigned long) sb.addr;
  devp = lgp->devhead;
  while (devp)
    {
#if defined(SOLARIS) && (SYSVERS >= 570)
      /*
       * Solaris 5.7/5.8:
       * convert from internal to external
       * representation of dev_t.
       */
      ditemp.lgnum = minor;
      ditemp.localcdev = DEVCMPL(devp->localcdisk);
      ditemp.cdev = DEVCMPL(devp->cdev);
      ditemp.bdev = DEVCMPL(devp->bdev);
#elif defined(_AIX)
      ditemp.lgnum = minor;
      ditemp.localcdev = DEVTODEV32(devp->localcdisk);
      ditemp.cdev = DEVTODEV32(devp->cdev);
      ditemp.bdev = DEVTODEV32(devp->bdev);
#elif defined(LINUX260)
      ditemp.lgnum = minor;
      ditemp.localcdev = huge_encode_dev(devp->localcdisk);
      ditemp.cdev = huge_encode_dev(devp->cdev);
      ditemp.bdev = huge_encode_dev(devp->bdev);
#else
      ditemp.lgnum = minor;
      ditemp.localcdev = devp->localcdisk;
      ditemp.cdev = devp->cdev;
      ditemp.bdev = devp->bdev;
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
      ditemp.disksize = devp->disksize;
      ditemp.maxdisksize = devp->maxdisksize;
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
#elif defined(_AIX)
  /* lg_num is left as it is. */
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  if ((ctlp = ftd_global_state) == NULL)
    return (ENXIO);

  /* XXX FIXME: Need to check the sb.length to make sure that we're OK */
  /* But only if lg_num == FTD_CTL */

  info = (ftd_lg_info_t *)(unsigned long) sb.addr;
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
#elif defined(_AIX)
	  lgtemp.lgdev = DEVTODEV32(lgp->dev);
#elif defined(LINUX260)
	  lgtemp.lgdev = huge_encode_dev(lgp->dev);
#else
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
#elif defined(_AIX)
	      lgtemp.lgdev = DEVTODEV32(lgp->dev);
#elif defined(LINUX260)
	      lgtemp.lgdev = huge_encode_dev(lgp->dev);
#else
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
#elif defined(_AIX)
  /* ex. dev id of /dev/dtc/lg0/rdsk/dtc0 */
  sb.dev_num = DEV32TODEV(sb.dev_num);
#elif defined(LINUX260)
  sb.dev_num = huge_decode_dev(sb.dev_num);
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
#elif defined(_AIX)
  temp.localbdisk = DEVTODEV32(devp->localbdisk);
#elif defined(LINUX260)
  temp.localbdisk = huge_encode_dev(devp->localbdisk);
#else
  temp.localbdisk = devp->localbdisk;
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
  temp.localdisksize = devp->disksize;

  temp.readiocnt = devp->readiocnt;
  temp.writeiocnt = devp->writeiocnt;
  temp.sectorsread = devp->sectorsread;
  temp.sectorswritten = devp->sectorswritten;
  temp.wlentries = devp->wlentries;
  temp.wlsectors = devp->wlsectors;
  temp.local_disk_io_captured = (devp->flags & FTD_LOCAL_DISK_IO_CAPTURED) != 0;
  
  if (ddi_copyout ((caddr_t) & temp, (caddr_t)(unsigned long) sb.addr, sizeof (temp), flag))
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
#elif defined(_AIX)
  /* lg_num is left as it is. */
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = getminor (sb.lg_num) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (lgp == NULL)
    return (ENXIO);

  /* XXX FIXME: Need to check the sb.length to make sure that we're OK */

  temp.wlentries = lgp->wlentries;
  temp.wlsectors = lgp->wlsectors;
  temp.bab_used =  lgp->wlsectors * DEV_BSIZE;
  temp.bab_free =  ctlp->bab_size - temp.bab_used;
  temp.state = lgp->state;
  temp.ndevs = lgp->ndevs;
  temp.sync_depth = lgp->sync_depth;
  temp.sync_timeout = lgp->sync_timeout;
  temp.iodelay = lgp->iodelay;
  // WI_338550 December 2017, implementing RPO / RTT
  temp.LastIOTimestamp = lgp->PMDStats.LastIOTimestamp;
  temp.OldestInconsistentIOTimestamp = lgp->PMDStats.OldestInconsistentIOTimestamp;
  temp.LastConsistentIOTimestamp = lgp->PMDStats.LastConsistentIOTimestamp;
  temp.RTT = lgp->PMDStats.RTT;
  temp.average_of_most_recent_RTTs = lgp->PMDStats.average_of_most_recent_RTTs;
  temp.WriteOrderingMaintained = lgp->PMDStats.WriteOrderingMaintained;
  temp.network_chunk_size_in_bytes = lgp->PMDStats.network_chunk_size_in_bytes;
  temp.previous_non_zero_RTT = lgp->PMDStats.previous_non_zero_RTT;

  if (ddi_copyout ((caddr_t) & temp, (caddr_t)(unsigned long) sb.addr, sizeof (temp), flag))
    return (EFAULT);

  return (0);
}

// WI_338550 December 2017, implementing RPO / RTT
/************************************************************************************//**

            Set the group RPO / RTT stats.  This is meant as a method to combine PMD RPO / RTT stats 
            and driver RPO / RTT stats.

\param      pLg 
\param      pStatBuf 

\return     

****************************************************************************************/
FTD_PRIVATE ftd_int32_t ftd_lg_set_RPO_stats(ftd_lg_t* pLg, ftd_stat_t* pStatBuf)
{
    ftd_context_t context;

    if ((pStatBuf == NULL) || (pLg == NULL))
    {
        FTD_ERR (FTD_WRNLVL, "%s: ftd_lg_set_RPO_stats: receiving NULL parameter\n.", DRIVERNAME)        ;
        return ENXIO;
    }

    ACQUIRE_LOCK (pLg->lock, context);

    // Update oldest timestamp only if it was not previously set by the driver, or if it is a PMD reset (=0)
    // Note: the field LastIOTimestamp is controlled by the driver itself, not by the user space
    if (pLg->PMDStats.OldestInconsistentIOTimestamp == 0 ||
        pStatBuf->OldestInconsistentIOTimestamp == 0)
        {
            if( pStatBuf->OldestInconsistentIOTimestamp != DO_NOT_CHANGE_OLDEST_INCONSISTENT_IO_TIMESTAMP )
            {
                pLg->PMDStats.OldestInconsistentIOTimestamp = pStatBuf->OldestInconsistentIOTimestamp;
            }
        }
    pLg->PMDStats.LastConsistentIOTimestamp = pStatBuf->LastConsistentIOTimestamp;
    pLg->PMDStats.RTT = pStatBuf->RTT;
    pLg->PMDStats.WriteOrderingMaintained = pStatBuf->WriteOrderingMaintained;
    pLg->PMDStats.network_chunk_size_in_bytes = pStatBuf->network_chunk_size_in_bytes;
    pLg->PMDStats.average_of_most_recent_RTTs = pStatBuf->average_of_most_recent_RTTs;
    if( pStatBuf->previous_non_zero_RTT != 0 )
        pLg->PMDStats.previous_non_zero_RTT = pStatBuf->previous_non_zero_RTT;

#ifdef  ENABLE_RPO_DEBUGGING
    printk( "<<< TRACING_DRIVER ftd_lg_set_RPO_stats RECEIVED pStatBuf->OldestInconsistentIOTimestamp = %lu\n", (unsigned long)pStatBuf->OldestInconsistentIOTimestamp );
    printk( "<<< TRACING_DRIVER ftd_lg_set_RPO_stats RECEIVED pStatBuf->LastConsistentIOTimestamp = %lu\n", (unsigned long)pStatBuf->LastConsistentIOTimestamp );
    printk( "<<< TRACING_DRIVER ftd_lg_set_RPO_stats RECEIVED pStatBuf->RTT = %lu\n", (unsigned long)pStatBuf->RTT );
    printk( "<<< TRACING_DRIVER ftd_lg_set_RPO_stats RECEIVED pStatBuf->WriteOrderingMaintained = %lu\n", (unsigned long)pStatBuf->WriteOrderingMaintained );
    printk( "<<< TRACING_DRIVER ftd_lg_set_RPO_stats RECEIVED pStatBuf->network_chunk_size_in_bytes = %lu\n", (unsigned long)pStatBuf->network_chunk_size_in_bytes );
    printk( "<<< TRACING_DRIVER ftd_lg_set_RPO_stats RECEIVED pStatBuf->previous_non_zero_RTT = %lu\n", (unsigned long)pStatBuf->previous_non_zero_RTT );
#endif
    RELEASE_LOCK (pLg->lock, context);

    return 0;
}

// WI_338550 December 2017, implementing RPO / RTT
/*-
 * ftd_ctl_set_group_RPO_stats()
 *
 * set logical group stats for RPO and RTT
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_set_group_RPO_stats (ftd_intptr_t arg, ftd_int32_t flag)
{
  ftd_int32_t status = 0;
  ftd_lg_t *lgp;
  ftd_stat_t *pStats;
  ftd_stat_t Stats;
  
  stat_buffer_t sb;
  minor_t minor;
  ftd_ctl_t *ctlp;
  
  pStats = &Stats;

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
#elif defined(_AIX)
  /* lg_num is left as it is. */
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = getminor (sb.lg_num) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (lgp == NULL)
    return (ENXIO);

  /* XXX FIXME: Need to check the sb.length to make sure that we're OK */

  if (ddi_copyin ((caddr_t)(unsigned long) sb.addr, (caddr_t)(unsigned long)pStats, sb.len, flag))
    return (EFAULT);
  status = ftd_lg_set_RPO_stats(lgp, pStats);

  return (status);
}

/*-
 * ftd_ctl_get_started_group ()
 *
 * Return a map of started groups
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_get_started_group  (ftd_intptr_t arg, ftd_int32_t flag)
{
    ftd_ctl_t *ctlp;
    int rc;
    ftd_context_t context;
    struct ftd_lg *lgp;
    ftd_lg_map_t lgm;

    bcopy((void*)arg, (void*)&lgm, sizeof(ftd_lg_map_t));

    if ((ctlp = ftd_global_state) == NULL)
            return (ENXIO);

    lgm.count = Started_LG_Map.count;
    lgm.lg_max = Started_LG_Map.lg_max;
    if (ddi_copyout((caddr_t)(unsigned long)Started_LGS, (caddr_t)(unsigned long)lgm.lg_map, 
	(sizeof(ftd_uchar_t) * MAXLG), flag))
      return(EFAULT);
    bcopy((void*)&lgm, (void*)arg, sizeof (ftd_lg_map_t));

    return (0);
}

/* ftd_ctl_set_lrdb()
 *
 * set all the bits in LRDB
 * reset lrt_mode
 *
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_set_lrdb (ftd_intptr_t arg)
{
    minor_t minor;
    ftd_lg_t *lgp;
    ftd_dev_t *tempdev;	
    dev_t dev;
    ftd_dev_t_t dev64;
    ftd_context_t context;
#if defined(FTD_DEBUG)
    FTD_ERR (FTD_DBGLVL, "ftd_ctl_set_lrdb(): entered");
#endif
#if defined(MISALIGNED)
    dev = *(dev_t *) arg;
#else /* defined(MISALIGNED)*/
    bcopy ((void *)arg, (void *)&dev64, sizeof(dev64));
    dev = (dev_t)dev64;
#if defined(SOLARIS) && (SYSVERS >= 570)
    dev=DEVEXPL(dev);
#elif defined(_AIX)
    dev = DEV32TODEV(dev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

    minor = dev;
    lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
    if (lgp == NULL) {
       return (ENXIO);
    }
    ACQUIRE_LOCK (lgp->lock, context);
    tempdev = lgp->devhead;
    while (tempdev) {
       ftd_memset ((caddr_t) tempdev->lrdb.map, 0xff, tempdev->lrdb.len32 * 4);
       ftd_memset ((caddr_t) tempdev->lrdb.mapnext, 0xff, tempdev->lrdb.len32 * 4);
       lgp->tempdev = tempdev; /* IPF related */
#if defined(HPUX) || defined(SOLARIS)
       RELEASE_LOCK (lgp->lock, context);
#if defined(HPUX)
       b_psema(&tempdev->lrdbsema);
#elif defined(SOLARIS)
       sema_p(&tempdev->lrdbsema);
#endif /* defined(HPUX) */
#endif /* defined(HPUX) || defined(SOLARIS) */
#if defined (linux) || defined (_AIX)
	RELEASE_LOCK (lgp->lock, context);
#endif
       ftd_flush_lrdb (lgp->tempdev, NULL, context);
#if defined (linux) || defined (_AIX)
	ACQUIRE_LOCK (lgp->lock, context);
#endif
#if defined(HPUX) || defined(SOLARIS)
#if defined(HPUX)
       b_vsema(&tempdev->lrdbsema);
#elif defined(SOLARIS)
       sema_v(&tempdev->lrdbsema);
#endif /* defined(HPUX) */
       ACQUIRE_LOCK (lgp->lock, context);
#endif /* defined(HPUX) || defined(SOLARIS) */
       tempdev = tempdev->next;
    }
    RELEASE_LOCK (lgp->lock, context);
    lgp->lrt_mode = 0;
    return (0);
}

/* ftd_ctl_set_lrdb_mode()
 *
 * sets lrt_mode only
 *
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_set_lrdb_mode (ftd_intptr_t arg)
{
    minor_t minor;
    ftd_lg_t *lgp;
    ftd_dev_t *tempdev;
    dev_t dev;
    ftd_dev_t_t dev64;
    ftd_context_t context;
#if defined(FTD_DEBUG)
    FTD_ERR(FTD_DBGLVL, "ftd_ctl_set_lrdb_mode: entered");
#endif
#if defined(MISALIGNED)
    dev = *(dev_t *) arg;
#else /* defined(MISALIGNED) */
    bcopy((void *)arg, (void *)&dev64, sizeof(dev64));
    dev = (dev_t)dev64;
#if defined(SOLARIS) && (SYSVERS >= 570)
    dev = DEVEXPL(dev);
#elif defined(_AIX)
    dev = DEV32TODEV(dev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */
    minor = dev;
    lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
    if (lgp == NULL) {
       return (ENXIO);
    }
    lgp->lrt_mode = 1;
    return (0);
}

/*
 * ftd_ctl_set_device_size()
 *
 * set device size for expanding.
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_set_device_size(ftd_intptr_t arg)
{
  ftd_dev_t *softp;
  ftd_dev_info_t info;
  minor_t minor;
  ftd_context_t context;

#if defined(MISALIGNED)
  info = *(ftd_dev_info_t *) arg;
#else /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&info, sizeof(info));
#if defined(SOLARIS) && (SYSVERS >= 570)
 /*
  * Solaris 5.7/5.8:
  * convert from external to internal
  * representation of dev_t.
  */
  info.cdev = DEVEXPL(info.cdev);
#elif defined(_AIX)
  /* lg_num is left as it is. */
  info.cdev = DEV32TODEV(info.cdev);
#elif defined(LINUX260)
  info.cdev = huge_decode_dev(info.cdev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = getminor (info.cdev);
  softp = (ftd_dev_t *) ddi_get_soft_state (ftd_dev_state, U_2_S32(minor));
  if (softp == NULL)
  {
    return (ENXIO);
  }

  ACQUIRE_LOCK (softp->lock, context);

  softp->disksize = info.disksize;
  
  RELEASE_LOCK (softp->lock, context);

  return (0);
}   


FTD_PRIVATE ftd_int32_t
ftd_ctl_get_max_device_size(ftd_intptr_t arg)
{
      ftd_dev_t *softp;
  ftd_dev_info_t info;
  minor_t minor;
  ftd_context_t context;

#if defined(MISALIGNED)
  info = *(ftd_dev_info_t *) arg;
#else /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&info, sizeof(info));
#if defined(SOLARIS) && (SYSVERS >= 570)
 /*
  * Solaris 5.7/5.8:
  * convert from external to internal
  * representation of dev_t.
  */
  info.cdev = DEVEXPL(info.cdev);
#elif defined(_AIX)
  /* lg_num is left as it is. */
  info.cdev = DEV32TODEV(info.cdev);
#elif defined(LINUX260)
  info.cdev = huge_decode_dev(info.cdev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = getminor (info.cdev);
  softp = (ftd_dev_t *) ddi_get_soft_state (ftd_dev_state, U_2_S32(minor));
  if (softp == NULL)
  {
    return (ENXIO);
  }

  ACQUIRE_LOCK (softp->lock, context);

  info.maxdisksize = softp->maxdisksize;

  RELEASE_LOCK (softp->lock, context);

#if defined(MISALIGNED)
    *((ftd_dev_info_t *) arg) = info;
#else /* defined(MISALIGNED) */
    bcopy((void *)&info, (void *)arg, sizeof(ftd_dev_info_t));
#endif /* defined(MISALIGNED) */
  
  return (0);
}

  
/*-
 * ftd_ctl_set_mode_tracking()
 *
 * set driver state to tracking whith out bab and bit operation.
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_set_mode_tracking (ftd_intptr_t arg)
{   
  minor_t minor;
  ftd_lg_t *lgp;
  ftd_state_t sb;
  ftd_context_t context;

#if defined(MISALIGNED)
  sb = *(ftd_state_t *) arg;
#else /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&sb, sizeof(sb));
#if defined(SOLARIS) && (SYSVERS >= 570)
 /* Solaris 5.7/5.8:convert from external to internal
    representation of dev_t.  */
  sb.lg_num = DEVEXPL(sb.lg_num);
#elif defined(_AIX)
  /* lg_num is left as it is. */
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  /* get minor from lg_num */
  minor = getminor (sb.lg_num) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (lgp == NULL)
    return (ENXIO);

  ACQUIRE_LOCK (lgp->lock, context);
  if (lgp->state != FTD_MODE_TRACKING)
  {
      lgp->state = FTD_MODE_TRACKING;
      FTD_ERR (FTD_WRNLVL, "%s: Transitioning to Tracking mode by ctl_set_mode_tracking.", DRIVERNAME)        ;
  }
  RELEASE_LOCK (lgp->lock, context);
  return (0);
}

/*-
 * ftd_ctl_set_group_state()
 * 
 * induce a driver state transition.
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_set_group_state (ftd_intptr_t arg, ftd_int32_t override)
{
  minor_t minor;
  ftd_lg_t *lgp;
  ftd_state_t sb;
  ftd_dev_t *tempdev;
  ftd_context_t context;
  ftd_uint32_t lgnum;		/* for Dynamic Mode Change */
  ftd_int32_t rc = 0;
  
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
#elif defined(_AIX)
  /* lg_num is left as it is. */
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */
  minor = (minor_t)(sb.lg_num);
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  
  if (lgp == NULL)
    return (ENXIO);

  ACQUIRE_LOCK (lgp->lock, context);
  if (lgp->state == sb.state)
    {
      RELEASE_LOCK (lgp->lock, context);
      return (0);
    }

  lgp->KeepHistoryBits = FALSE;    /* to prevent forever duplication */

  /*
   * Dynamic Mode Change
   */
  lgnum = (ftd_uint32_t)(getminor(lgp->dev) & ~FTD_LGFLAG);
  if (0 <= lgnum && lgnum < MAXLG) {
    if (sync_time_out_flg[lgnum] == 1) {
      switch (sb.state) {	/* requested state */
      case FTD_MODE_PASSTHRU:
      case FTD_MODE_FULLREFRESH:
        /* may be dtcoverride request */
        sync_time_out_flg[lgnum] = 0;
        break;
      case FTD_MODE_REFRESH:
        switch (lgp->state) {	/* current driver state */
        case FTD_MODE_NORMAL:
        case FTD_MODE_FULLREFRESH:
        case FTD_MODE_TRACKING:
          sync_time_out_flg[lgnum] = 0;
          break;
        default:
          break;
        }
      case FTD_MODE_NORMAL:
      case FTD_MODE_TRACKING:
      case FTD_MODE_CHECKPOINT_JLESS:
      default:
        break;
      }
    }
  }

  switch (lgp->state) {
    case FTD_MODE_NORMAL:
	  switch (sb.state) {
         /*case FTD_MODE_REFRESH:  // RFW Clear the bit Normal-> F>Refresh */
         /*case FTD_MODE_TRACKING:*/
	  case FTD_MODE_FULLREFRESH:
        /* clear all dirty bits */
        ftd_clear_dirtybits (lgp);
        break;
	  case FTD_MODE_PASSTHRU:
        ftd_set_dirtybits (lgp);
        ftd_clear_bab_and_stats(lgp, context);
        break;
	  case FTD_MODE_CHECKPOINT_JLESS:
         /*ftd_clear_dirtybits (lgp);  // RFW Do not clear the bit when going to here*/
         /*ftd_compute_dirtybits (lgp, FTD_HIGH_RES_DIRTYBITS);*/
            break;
	  case FTD_MODE_BACKFRESH:
		/* no op */
        break;
	  }
      break;
    case FTD_MODE_REFRESH:
	  switch (sb.state) {
      case FTD_MODE_NORMAL:
         /*ftd_clear_dirtybits (lgp); // RFW Do not clear the bit when going to normal*/
         break; 
      case FTD_MODE_FULLREFRESH:
         /* FRF - For full refresh restart, don't clear the dirty bits */
        if(! sb.fullrefresh_restart)
              ftd_clear_dirtybits (lgp);
        break;
      case FTD_MODE_TRACKING:
        /* clear the journal */
        ftd_clear_bab_and_stats(lgp, context);
        break;
      case FTD_MODE_PASSTHRU:
        ftd_set_dirtybits (lgp);
        ftd_clear_bab_and_stats(lgp, context);
        break;
      case FTD_MODE_CHECKPOINT_JLESS:
      case FTD_MODE_BACKFRESH:
        /* no op */
        break;
      }
      break;
    case FTD_MODE_FULLREFRESH:
	  switch (sb.state) {
      case FTD_MODE_NORMAL:
         /*ftd_clear_dirtybits (lgp); // RFW Do not clear the bit when going to normal */
        break;
      case FTD_MODE_TRACKING:
        /* clear the journal */
        ftd_clear_bab_and_stats(lgp, context);
        break;
      case FTD_MODE_PASSTHRU:
        ftd_set_dirtybits (lgp);
        ftd_clear_bab_and_stats(lgp, context);
        break;
      case FTD_MODE_CHECKPOINT_JLESS:
      case FTD_MODE_BACKFRESH:
        /* no op */
        break;
      }
      break;
    case FTD_MODE_TRACKING:
	  switch (sb.state) {
      case FTD_MODE_NORMAL:
        /*
         * This is not a valid state transition for PMD,
         * which can happen when bab overflows after PMD's check for 
         * bab overflow. By returning error we force PMD to recheck its 
         * refresh logic.
         */
        if (!override) {
		RELEASE_LOCK (lgp->lock, context);
		return (EINVAL);
        } 
        /* RFW Do not clear the bit when going to normal
         *ftd_clear_dirtybits (lgp); */
        /* update low res dirty bits with journal *
         *ftd_compute_dirtybits (lgp, FTD_LOW_RES_DIRTYBITS); */
        break;
      case FTD_MODE_PASSTHRU:
		ftd_set_dirtybits (lgp);
		ftd_clear_bab_and_stats(lgp, context);
        break;
      case FTD_MODE_FULLREFRESH:
         /* FRF - For full refresh restart, don't clear the dirty bits */
            if(! sb.fullrefresh_restart)
               ftd_clear_dirtybits (lgp);
            ftd_clear_bab_and_stats(lgp, context);
            break;
      case FTD_MODE_REFRESH:
      case FTD_MODE_CHECKPOINT_JLESS:
      case FTD_MODE_BACKFRESH:
        /* no op */
        break;
      }
	  break;
    case FTD_MODE_PASSTHRU:
      /* FIXME: we should probably do something here */
	  switch (sb.state) {
      case FTD_MODE_REFRESH:
        ftd_set_dirtybits (lgp);
        break;
      case FTD_MODE_NORMAL:
      case FTD_MODE_TRACKING:
#if defined(DEPRECATE)
/*-
 * XXX
 * this transition happens everytime start runs
 * don't want to clobber bitmaps on startup.
 * deprecate this?
 */
        ftd_clear_dirtybits (lgp);
#endif /* defined(DEPRECATE) */

        ftd_clear_bab_and_stats(lgp, context);
        break;
      case FTD_MODE_FULLREFRESH:
	    ftd_clear_dirtybits(lgp);
	    ftd_clear_bab_and_stats(lgp, context);
        break;
      case FTD_MODE_CHECKPOINT_JLESS:
      case FTD_MODE_BACKFRESH:
		/* no op */
        break;
      }
      break;
    case FTD_MODE_CHECKPOINT_JLESS:
	  switch (sb.state) {
      case FTD_MODE_FULLREFRESH:
        FTD_ERR(FTD_DBGLVL, "driver state cp_jless to full refresh");				
        ftd_clear_dirtybits (lgp);
        ftd_clear_bab_and_stats(lgp, context);
        break;
      case FTD_MODE_PASSTHRU:
        FTD_ERR(FTD_DBGLVL, "driver state cp_jless to passthru");				
        ftd_set_dirtybits (lgp);
        ftd_clear_bab_and_stats(lgp, context);
        break;
      case FTD_MODE_NORMAL:
        FTD_ERR(FTD_DBGLVL, "driver state cp_jless to normal");				
        /*ftd_clear_dirtybits (lgp);  // RFW Do not clear the bit when going to here*/
        /* update low res dirty bits with journal */
        /*ftd_compute_dirtybits (lgp, FTD_LOW_RES_DIRTYBITS);*/
        break;
      case FTD_MODE_REFRESH:
        FTD_ERR(FTD_DBGLVL, "driver state cp_jless to refresh");				
        break;
      case FTD_MODE_TRACKING:
        FTD_ERR(FTD_DBGLVL, "driver state cp_jless to tracking");				
        break;
      case FTD_MODE_BACKFRESH:
        FTD_ERR(FTD_DBGLVL, "driver state cp_jless to backfresh");				
        break;
      }
      break;
    case FTD_MODE_BACKFRESH:
    default:
      /* -> Transition to backfresh */
      break;
    }

  if (sb.state == FTD_MODE_REFRESH)
  {
      // Smarter Smart-Refresh
      // =====================
      // Since we clear each bit of PMD local HRDB every time we acknowledge a refresh packet,
      // at exit we can update the driver hrdb with our local hrdb and merge it with the 
      // historical hrdb (map.next) that was initialise and start at the begining of 
      // Smart-Refresh to make sure we do not miss any bits during Smart Refresh.
      ftd_clear_hires_historical_dirtybits(lgp);
      lgp->DualBitmaps = TRUE;

#if defined(FTD_DEBUG)
      FTD_ERR(FTD_DBGLVL, "ftd_ctl_set_group_state group %03d: Setting DualBitmaps to TRUE. History bits: %d\n", lgnum, lgp->KeepHistoryBits);
#endif /* FTD_DEBUG */
  }
  else if ((lgp->DualBitmaps == TRUE) && ((sb.state == FTD_MODE_PASSTHRU) || (sb.state == FTD_MODE_FULLREFRESH) || (sb.state == FTD_MODE_NORMAL)))
  {
      // Before setting the group into tracking where the DualBitmaps is stopped, the PMD should merge the bitmaps.
      lgp->DualBitmaps = FALSE;

#if defined(FTD_DEBUG)
      FTD_ERR(FTD_DBGLVL, "ftd_ctl_set_group_state group %03d: Setting DualBitmaps to FALSE. History bits: %d\n", lgnum, lgp->KeepHistoryBits);
#endif /* FTD_DEBUG */
  }
  
  lgp->state = sb.state;

  /*
   * Need to somehow flush the state to disk here, no? XXX FIXME XXX
   * flush the low res dirty bits
   */

   /*
    * WR 37217: Not all platforms allow spinlock to be held across modules.
    * We use semaphore for flushing lrdb in HPUX and Solaris platforms.
    */
#if defined(HPUX) || defined(SOLARIS)
  RELEASE_LOCK (lgp->lock, context);
#endif

if (lgp->lrt_mode)
{
  tempdev = lgp->devhead;
  while (tempdev) {
    /* need to hold tempdev->lrdbsema */
#if defined(HPUX)
    b_psema(&tempdev->lrdbsema);
#elif defined(SOLARIS)
    sema_p(&tempdev->lrdbsema);
#endif

      lgp->tempdev = tempdev; 
#if defined (linux) || defined (_AIX)
        RELEASE_LOCK (lgp->lock, context);
#endif 
      ftd_flush_lrdb (lgp->tempdev, NULL, context);
#if defined (linux) || defined (_AIX)
        ACQUIRE_LOCK (lgp->lock, context);
#endif

#if defined(HPUX)
      b_vsema(&tempdev->lrdbsema);
#elif defined(SOLARIS)
      sema_v(&tempdev->lrdbsema);
#endif
      tempdev = tempdev->next;
    }
}

#if !defined(HPUX) && !defined(SOLARIS)
  RELEASE_LOCK (lgp->lock, context);
#endif
  
  // We must wait for any pending IOs to terminate before we return in order to guarantee that
  // any IO submitted before or during the time we were in the process of changing the state have been written to disk.
  // This is primarilly to ensure that any refresh starting immediately after this change of state will be ensured
  // to read any recently updated data.

  // Should we wait for IO completion only when switching to certain states?  FULLREFRESH and REFRESH come to mind.
  rc = pending_ios_monitoring_wait_for_group_pending_ios_completion(lgp);

  ftd_wakeup (lgp);
  
  return rc;
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
  ftd_babinfo_t babinfo;

    if ((ctlp = ftd_global_state) == NULL)
      return (ENXIO);

    babinfo.actual = ctlp->bab_size;
    babinfo.free = ftd_bab_free_size();
#if defined(MISALIGNED)
    *((ftd_babinfo_t *) arg) = babinfo;
#else /* defined(MISALIGNED) */
    bcopy((void *)&babinfo, (void *)arg, sizeof(babinfo));
#endif /* defined(MISALIGNED) */

  return (0);
}

/*-
 * ftd_ctl_open_num_info ()
 *
 * returns unique True opened value of an inode given by user.
 *
 * Return: 0 for no open or non blockdevice
 *         n for n times opened a file
 */

#if defined(linux)
FTD_PUBLIC ftd_int32_t ftd_ctl_open_num_info (ftd_open_num_t *arg)
{
	struct block_device *bdevice;
	ftd_open_num_t ptr;

	if (copy_from_user(&ptr, arg, sizeof(ftd_open_num_t)) != 0)
	{
          ftd_debugf(ioctl,
                     "ftd_ctl_open_num_info(%p): copy_from_user(%p, %p, %zd) failed\n",
                     arg,
                     &ptr, arg, sizeof(ftd_open_num_t));
          return EFAULT;
	}


    {
       dev_t dev = new_decode_dev(ptr.dno);
       bdevice = bdget(dev);

       ftd_blkdev_get(bdevice, O_RDWR|O_SYNC);

    }

	if( bdevice )
	{
		ptr.open_num = bdevice->bd_openers;

		ftd_blkdev_put(bdevice);

	}
	else{
		/* get blockdevice fails assume no open*/
		ptr.open_num = 0;
	}

	if (copy_to_user(arg, &ptr, sizeof(ftd_open_num_t)) != 0) {
          ftd_debugf(ioctl,
                     "ftd_ctl_open_num_info(%p): copy_to_user(%p, %p, %zd) failed\n",
                     arg,
                     arg, &ptr, sizeof(ftd_open_num_t));
          return EFAULT;
        }
	return (0);
}
#endif /* defined(linux) */

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
  ftd_context_t context;
  ftd_int32_t ret;
  ftd_int32_t minor_index;
#if defined(_AIX)
  ftd_int32_t fix_minor;
#endif

  if ((ctlp = ftd_global_state) == NULL)
    return (ENXIO);
  if (once)
    {
#if defined(_AIX)
      ctlp->minor_index = MAXLVS - 1;
#else
      ctlp->minor_index = 0;
#endif
      once = 0;
    }

  ACQUIRE_LOCK (ctlp->lock, context);

#if defined(SOLARIS)
  minor_index = ++ctlp->minor_index;
#else
# if defined(_AIX)
#   if defined(MISALIGNED)
      fix_minor  = *((ftd_int32_t *) arg)
#   else /* defined(MISALIGNED) */
      bcopy((void *)arg, (void *)&fix_minor, sizeof(fix_minor));
#   endif /* defined(MISALIGNED) */
    if (fix_minor < 0) {
      minor_index = ctlp->minor_index;
    } else {
      minor_index = fix_minor - 1;
    }
# else
    minor_index = ctlp->minor_index;
# endif

  ret = ddi_soft_state_reserve(ftd_dev_state, minor_index);
  if (ret == -1) {
    RELEASE_LOCK (ctlp->lock, context);
    return (ENXIO);
  }
# if defined(_AIX)
    /* only reset the ctlp_minor_index when not fixing the minor device number */
    if (fix_minor < 0) {
      ctlp->minor_index = ret;
    }
# else
    ctlp->minor_index = ret;
# endif
  minor_index = ret;
#endif

#if defined(MISALIGNED)
  *((ftd_int32_t *) arg) = minor_index;
#else /* defined(MISALIGNED) */
  bcopy((void *)&minor_index, (void *)arg, sizeof(minor_index));
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
#elif defined(_AIX)
  /* lg_num is left as it is. */
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = lg_num;
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
#elif defined(_AIX)
  /* lg_num is left as it is. */
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = lg_num;
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
#elif defined(_AIX)
  /* lg_num is left as it is. */
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
  ftd_context_t context;

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
#elif defined(_AIX)
  dev = DEV32TODEV(dev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = (minor_t)dev;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (lgp == NULL)
return (ENXIO);

  ACQUIRE_LOCK (lgp->lock, context);
  ftd_clear_bab_and_stats(lgp, context);
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
  ftd_dev_t_t dev64;
  ftd_context_t context;

#if defined(MISALIGNED)
  dev = *(dev_t *) arg;
#else /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&dev64, sizeof(dev64));
  dev = (dev_t)dev64;

#if defined(SOLARIS) && (SYSVERS >= 570)
 /*
  * Solaris 5.7/5.8:
  * convert from external to internal
  * representation of dev_t.
  */
  dev = DEVEXPL(dev);
#elif defined(_AIX)
  dev = DEV32TODEV(dev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = dev;
#if defined(ftd_debugf)
  ftd_debugf(dirty, "ftd_ctl_clear_dirtybits: %lx %x\n", (unsigned long)dev, minor);
#endif /* defined(linux) */
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (lgp == NULL)
    return (ENXIO);

#if defined(ftd_debugf)
  ftd_debugf(dirty, "ftd_ctl_clear_dirtybits: lgp          %p\n", lgp);
  ftd_debugf(dirty, "ftd_ctl_clear_dirtybits: lgp->devhead %p\n", lgp->devhead);
#endif /* defined(linux) */

  ACQUIRE_LOCK (lgp->lock, context);

  lgp->KeepHistoryBits = FALSE;

  tempdev = lgp->devhead;
  while (tempdev) {
      if (type == FTD_HIGH_RES_DIRTYBITS) {
#if defined(ftd_debugf)
        ftd_debugf(dirty, "ftd_ctl_clear_dirtybits: HIGH %p %p %u\n",
            tempdev, tempdev->hrdb.map, tempdev->hrdb.len32);
#endif /* defined(linux) */
        bzero ((caddr_t) tempdev->hrdb.map, tempdev->hrdb.len32 * 4);
        bzero ((caddr_t) tempdev->hrdb.mapnext, tempdev->hrdb.len32 * 4);
      } else {
#if defined(ftd_debugf)
        ftd_debugf(dirty, "ftd_ctl_clear_dirtybits: LOW  %p %p %x\n",
                      tempdev, tempdev->hrdb.map, tempdev->hrdb.len32);
#endif /* defined(linux) */
        if (tempdev->lgp->lrt_mode)
            bzero ((caddr_t) tempdev->lrdb.map, tempdev->lrdb.len32 * 4);
        if (tempdev->lgp->lrt_mode)
            bzero ((caddr_t) tempdev->lrdb.mapnext, tempdev->lrdb.len32 * 4);
      }
      /*
       * note this flush operation is bottom-half safe for all
       * all platforms, so it can be protected by spin lock.
       * WR 37217: Not all platforms allow spinlock to be held across modules
       * we use semaphore for flushing lrdb
       */
      lgp->tempdev = tempdev;	/* IPF related */
#if defined(HPUX) || defined(SOLARIS)
      RELEASE_LOCK (lgp->lock, context);
#if defined(HPUX)
      b_psema(&tempdev->lrdbsema);
#elif defined(SOLARIS)
      sema_p(&tempdev->lrdbsema);
#endif
#endif /* HPUX || SOLARIS */

#if defined (linux) || defined (_AIX)
        RELEASE_LOCK (lgp->lock, context);
#endif
      ftd_flush_lrdb (lgp->tempdev, NULL, context);
#if defined (linux) || defined (_AIX)
        ACQUIRE_LOCK (lgp->lock, context);
#endif

#if defined(HPUX) || defined(SOLARIS)
#if defined(HPUX)
      b_vsema(&tempdev->lrdbsema);
#elif defined(SOLARIS)
      sema_v(&tempdev->lrdbsema);
#endif
     ACQUIRE_LOCK (lgp->lock, context);
#endif /* HPUX || SOLARIS */
      tempdev = tempdev->next;
  }

  RELEASE_LOCK (lgp->lock, context);

  return (0);
}

/*-
 * ftd_get_hrdb_resolution()
 *
 * Returns the HRDB resolution for a particular device size.
 * NOTE: this is done only in the case of LARGE HRT.
 */
FTD_PRIVATE ftd_int32_t
ftd_get_hrdb_resolution(ftd_uint64_t disksize)
{
    ftd_int32_t bitsize, shift, numbits, hrdbsize;

    if (disksize <= (ftd_uint64_t)DISK_SIZE_1/DEV_BSIZE)
        bitsize = MINIMUM_HRDB_BITSIZE;
    else if (disksize <= (ftd_uint64_t)DISK_SIZE_2/DEV_BSIZE)
        bitsize = HRDB_BITSIZE_1;
    else if (disksize <= (ftd_uint64_t)DISK_SIZE_3/DEV_BSIZE)
        bitsize = HRDB_BITSIZE_4;
    else if (disksize <= (ftd_uint64_t)DISK_SIZE_4/DEV_BSIZE)
        bitsize = HRDB_BITSIZE_6;
    else
        bitsize = HRDB_BITSIZE_8;

    /*
     * The HRDB resolution needs to be adjusted so that the HRDB size does
     * not exceed FTD_MAXIMUM_HRDB_SIZE. The following loop will reduce
     * the HRDB resolution (i.e. increment bitsize) until the HRDB size
     * falls below FTD_MAXIMUM_HRDB_SIZE.
     */

    do {
        shift = bitsize - DEV_BSHIFT;
        numbits = (disksize + ((1 << shift) - 1)) >> shift;
        hrdbsize = numbits / 8;
        bitsize++;
    } while (hrdbsize > FTD_MAXIMUM_HRDB_SIZE_LARGE);

    return (bitsize - 1);
}

/*-
 * ftd_ctl_new_device()
 *
 * instantiate a new device
 */
FTD_PRIVATE ftd_int32_t ftd_ctl_new_device (ftd_intptr_t arg)
{
  ftd_dev_t *softp;
  ftd_lg_t *lgp;
  ftd_dev_info_t info, tmpinfo;
  ftd_int32_t diskbits, lrdbbits, hrdbbits, i;
  minor_t dev_minor, lg_minor;
  int s_major, s_minor;
  struct buf *buf;
  int err = 0;

#if defined(HPUX)
  drv_info_t *drv_info;
#elif defined(SOLARIS)
  major_t emaj, imaj;
  minor_t emin;
#elif defined(linux)
  char thread_name[16];
  struct buf *buf2;
#if defined (LINUX260) 
  struct page *bpage, *bpage2;
  minor_t  min;
  unsigned long long localsize;
  struct block_device *localbdev, *bdev = NULL, *pstbdev;
#endif /* defined (LINUX260) */
#endif

  ftd_ctl_t *ctlp;
  int local_device_hardware_sector_size_bit_order = 0;
  
  if ((ctlp = ftd_global_state) == NULL)
    return (ENXIO);

  /* the caller needs to give us: info about the device and the group device
     number */

#if defined(MISALIGNED)
  info = *((ftd_dev_info_t *) arg);
  tmpinfo = *((ftd_dev_info_t *) arg);
#else /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&info, sizeof(info));
  // The following (second) structure, tmpinfo, will be used for returning info fields modified
  // by the driver (for instance hrdbsize32).
  bcopy((void *)arg, (void *)&tmpinfo, sizeof(tmpinfo));
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
#elif defined(_AIX)
  info.bdev = DEV32TODEV(info.bdev);
  info.cdev = DEV32TODEV(info.cdev);
  /* lg_num is left as it is. */
  info.localcdev = DEV32TODEV(info.localcdev);
#elif defined(LINUX260)
  info.bdev = huge_decode_dev(info.bdev);
  info.cdev = huge_decode_dev(info.cdev);
  info.localcdev = huge_decode_dev(info.localcdev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  dev_minor = getminor (info.cdev);	/* ??? */

  if (ddi_soft_state_zalloc (ftd_dev_state, dev_minor) != DDI_SUCCESS) {
	return (EADDRINUSE);
  }
  softp = (ftd_dev_t *) ddi_get_soft_state (ftd_dev_state, U_2_S32(dev_minor));

  if (softp == NULL)
    return (ENXIO);

  lg_minor = info.lgnum;
  lgp = ddi_get_soft_state (ftd_lg_state, U_2_S32(lg_minor));
  if (lgp == NULL) {
	FTD_ERR (FTD_WRNLVL, "Can't get the soft state for " GROUPNAME " group %d",
		 lg_minor);
	return (ENXIO);
  }

#if defined(linux)
  softp->lrdb.map = NULL;
  softp->hrdb.map = NULL;
  softp->lrdb.mapnext = NULL;
  softp->hrdb.mapnext = NULL;

#else
  buf = softp->lrdbbp = (struct buf *) &(softp->lrdb_metabuf);
  buf->b_flags = B_BUSY;
#endif /* defined(linux) */

  softp->lgp = lgp;

#if defined(HPUX)
  softp->localcdisk = info.localcdev;

  drv_info = cdev_drv_info (softp->localcdisk);
  softp->localbdisk = makedev (drv_info->b_major, minor (softp->localcdisk));

  // The following is a bit of black magic, as I couldn't find any clear doc on the API.
  // Because of this, the proper decision between _S_IFBLK and _S_IFCHR isn't clear.
  // If we ever switch to _S_IFCHR, make sure we pass the proper char device number.
  err = opend(&softp->localbdisk, _S_IFBLK, FWRITE | FREAD, NULL);
  
  if (err)
  {
      ddi_soft_state_free (ftd_dev_state, dev_minor);
      return err;
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
    makedevno (major_num (info.localcdev), minor_num (info.localcdev));
  if (fp_opendev (softp->localbdisk, (DREAD | DWRITE | DNDELAY),
		  (ftd_char_t *) NULL, 0, &softp->dev_fp)) {
	return (ENXIO);
  }
  softp->cdev = info.cdev;
  softp->bdev = info.bdev;
#elif defined(linux)

  ftd_debugf( device, " new_device : before  localcdev %016llx localbdev %016llx\n", info.localcdev, info.localcdev);

  softp->localcdisk = softp->localbdisk = info.localcdev;
  s_major = FTDGETMAJOR(softp->localbdisk);
  s_minor = FTDGETMINOR(softp->localbdisk);

  ftd_debugf(device, "ftd_ctl_new_device: lower <%x:%x> dtc <%x:%x>\n",
        s_major, s_minor, getmajor(info.bdev), getminor(info.bdev));
ftd_debugf( device, "new_device: after  localcdev %016llx localbdev %016llx\n",
        info.localcdev, info.localcdev);
  /* need to open block device? */
  if ((err = ftd_layered_open(&softp->bd,softp->localbdisk))) {
	FTD_ERR (FTD_WRNLVL, "ftd_ctl_new_device:  ftd_layered_open(.., %lx) fails with %d\n", 
        softp->localbdisk, err);
    goto ERROR_EXIT;
  }
  softp->cdev = info.cdev;
  softp->bdev = info.bdev;
  /* 
   * register disk size to kernel global blk_size[major][minor] array.
   * major = dtc device MAJOR(ftd_major), minor = dtc device minor.
   * here we adjust(one more block) # of blks just in case.....
   * if partition has odd # of sectors.
   */
#endif


  softp->disksize = info.disksize;
  softp->maxdisksize = info.maxdisksize;

  diskbits = ftd_get_msb (info.disksize) + 1 + DEV_BSHIFT;
  // NOTE: for HRDB, this modification for using all bits applies only in Small HRT mode.
  //       For LRDB, it applies to all modes.
  lrdbbits = ftd_get_msb (info.lrdbsize32 * 4 * 8); // PROD7660
  hrdbbits = ftd_get_msb (info.hrdbsize32 * 4 * 8);

  lrdbptr = &(softp->lrdb);
  softp->lrdb.bitsize = diskbits - lrdbbits;
  if (softp->lrdb.bitsize < MINIMUM_LRDB_BITSIZE)
    softp->lrdb.bitsize = MINIMUM_LRDB_BITSIZE;
  softp->lrdb.shift = softp->lrdb.bitsize - DEV_BSHIFT;
  softp->lrdb.len32 = info.lrdbsize32;
  softp->lrdb.numbits = (info.disksize +
		       ((1 << softp->lrdb.shift) - 1)) >> softp->lrdb.shift;
  // Return the following fields to the user as they are calculated only here
  // but the user has these fields also in the user space structure.
  tmpinfo.lrdb_res = softp->lrdb.shift;	// WR PROD10057
  tmpinfo.lrdb_numbits = softp->lrdb.numbits;

  softp->lrdb.map = (ftd_uint32_t *) kmem_zalloc (info.lrdbsize32 * 4, KM_SLEEP);
  if (!softp->lrdb.map) {
    err = ENOMEM;
    goto ERROR_EXIT;
  }

  lgp->KeepHistoryBits = FALSE;
  lgp->DualBitmaps = FALSE;
  
  softp->lrdb.mapnext = (ftd_uint32_t *) kmem_zalloc (info.lrdbsize32 * 4, KM_SLEEP);
  if (!softp->lrdb.mapnext) {
      err = ENOMEM;
      goto ERROR_EXIT;
  }

  // The following is normally DEV_BSHIFT (default) unless special case on specific platform
  local_device_hardware_sector_size_bit_order = ftd_get_hardware_sector_size_bit_order(softp->localbdisk);
  
  if (ctlp->hrdb_type == FTD_HS_LARGE)
  {
      softp->hrdb.bitsize = ftd_get_hrdb_resolution(softp->disksize);
  }
  else if (ctlp->hrdb_type == FTD_HS_PROPORTIONAL)
  {
      // For Proportional HRDB, calculations are done in User space
      if( info.hrdb_res < DEV_BSHIFT )
      {
        err = EINVAL;
        goto ERROR_EXIT;
      }
	  softp->hrdb.bitsize = info.hrdb_res;
  }

  if( ctlp->hrdb_type == FTD_HS_LARGE )
  {
      if(softp->hrdb.bitsize < local_device_hardware_sector_size_bit_order)
      {
         softp->hrdb.bitsize = local_device_hardware_sector_size_bit_order;
      }
      
      softp->hrdb.shift = softp->hrdb.bitsize - DEV_BSHIFT;
      softp->hrdb.numbits = (info.disksize +
		       ((1 << softp->hrdb.shift) - 1)) >> softp->hrdb.shift;
      softp->hrdb.len32 = softp->hrdb.numbits/32;
      tmpinfo.hrdbsize32 = softp->hrdb.len32;
      softp->hrdb.map = (ftd_uint32_t *) kmem_zalloc (softp->hrdb.len32 * 4, KM_SLEEP);
      softp->hrdb.mapnext = (ftd_uint32_t *) kmem_zalloc (softp->hrdb.len32 * 4, KM_SLEEP);
  }
  else if( ctlp->hrdb_type == FTD_HS_PROPORTIONAL )
  {
      // bit_size must satisfy physical sector size; if not, we must adjust
	  // the values and return them to the User space
      if(softp->hrdb.bitsize < local_device_hardware_sector_size_bit_order)
      {
         softp->hrdb.bitsize = local_device_hardware_sector_size_bit_order;
         softp->hrdb.shift = softp->hrdb.bitsize - DEV_BSHIFT;
         softp->hrdb.numbits = (info.disksize +
		       ((1 << softp->hrdb.shift) - 1)) >> softp->hrdb.shift;
         softp->hrdb.len32 = softp->hrdb.numbits/32;
         tmpinfo.hrdbsize32 = softp->hrdb.len32; // Must return adjusted value to the user
      }
	  else
	  {
	     // Take the values as calculated in User space
         softp->hrdb.shift = softp->hrdb.bitsize - DEV_BSHIFT;
         softp->hrdb.numbits = info.hrdbsize32 * 32;
         softp->hrdb.len32 = info.hrdbsize32;
	  }
      softp->hrdb.map = (ftd_uint32_t *) kmem_zalloc (softp->hrdb.len32 * 4, KM_SLEEP);
      softp->hrdb.mapnext = (ftd_uint32_t *) kmem_zalloc (softp->hrdb.len32 * 4, KM_SLEEP);
  }
  else if (ctlp->hrdb_type == FTD_HS_SMALL) {
      softp->hrdb.bitsize = diskbits - hrdbbits;

      if(softp->hrdb.bitsize < local_device_hardware_sector_size_bit_order) {
         softp->hrdb.bitsize = local_device_hardware_sector_size_bit_order;
      }
      
      softp->hrdb.shift = softp->hrdb.bitsize - DEV_BSHIFT;
      softp->hrdb.len32 = info.hrdbsize32;
      softp->hrdb.numbits = (info.maxdisksize +
                       ((1 << softp->hrdb.shift) - 1)) >> softp->hrdb.shift;
      softp->hrdb.map = (ftd_uint32_t *) kmem_zalloc (info.hrdbsize32 * 4, KM_SLEEP);
      softp->hrdb.mapnext = (ftd_uint32_t *) kmem_zalloc (info.hrdbsize32 * 4, KM_SLEEP);
  }
  // Return the following final values to the user
  tmpinfo.hrdb_numbits = softp->hrdb.numbits; // WR PROD10057
  tmpinfo.hrdb_res = softp->hrdb.bitsize;

  if (!softp->hrdb.map) {
    err = ENOMEM;
    goto ERROR_EXIT;
  }

  if (!softp->hrdb.mapnext) {
      err = ENOMEM;
      goto ERROR_EXIT;
  }

#if !defined(linux)
  BP_DEV (buf) = lgp->persistent_store;
  buf->b_un.b_addr = (caddr_t) softp->lrdb.map;
  buf->b_blkno = info.lrdb_offset;
  buf->b_bcount = info.lrdbsize32 * sizeof (ftd_int32_t);
#endif /* !defined(linux) */

#if defined(HPUX)
  buf->b_flags = 0;
  buf->b_flags |= B_CALL | B_WRITE | B_BUSY | B_ASYNC;
  buf->b_iodone = BIOD_CAST (biodone_psdev);
  BP_SOFTP (buf) = softp;
#if defined(LRDBSYNCH)
  LRDB_FREE_ACK (softp);
#endif /* defined(LRDBSYNCH) */

#elif defined(SOLARIS)
#ifdef B_KERNBUF
  buf->b_flags |= B_KERNBUF | B_WRITE | B_BUSY;
#else
  buf->b_flags |= B_WRITE | B_BUSY;
#endif /* B_KERNBUF */
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

#elif defined(linux)
{
	softp->lrdb_offset = info.lrdb_offset;
	softp->lrdb_state = 0;
	softp->lrdb_epoch = 1;	/* lrdb epoch is non-zero number */
	softp->qhead = softp->qtail = NULL;
}
#endif

  softp->statbuf = (ftd_char_t *) kmem_zalloc (info.statsize, KM_SLEEP);
  softp->statsize = info.statsize;

  ALLOC_LOCK (softp->lock, QNM " device specific lock");

#if defined (HPUX)
  /* WR 32873: 1-2-9-p13 Korea, WR34691 (2.1.x)
   * initialize lrdb semaphore to not owned.  lock order FTD_SPINLOCK_ORDER
   * ONLY us know of this semaphore.
   */
  b_initsema(&(softp->lrdbsema), 1, FTD_SPINLOCK_ORDER | SEMA_DEADLOCK_SAFE,
                "lrdb_sema");
#elif defined (SOLARIS)
  sema_init(&(softp->lrdbsema), 1, "lrdb_sema", SEMA_DRIVER, NULL);
#endif

  /*
   * use a dynamic buffer allocation on linux instead of pre-allocation
   * as some linux functions are not bottom-half safe.
   */
  softp->freelist = NULL;
  softp->bufcnt = 0;

#if !defined(linux)
  /* Allocate struct buf for later use */
  for (i = 0; i < FTD_MAX_BUF; i++)
  {
	  // PROD8842: allow memory allocation to sleep when called from this ioctl
	  // NOTE: we must not hold any lock here
      buf = GETRBUF (KM_SLEEP);
      if (buf == NULL)
      {
          if (i == 0)
          {
              FTD_ERR (FTD_WRNLVL, "%s%d: Can't allocate any buffers for I/O\n", DRIVERNAME, dev_minor);
		      // Still want to return what we have calculated for lrdb and hrdb to the user
#if defined(MISALIGNED)
              *((ftd_dev_info_t *) arg) = tmpinfo;
#else /* defined(MISALIGNED) */
              bcopy((void *)&tmpinfo, (void *)arg, sizeof(tmpinfo));
#endif
              return ENOMEM;
          }
          else
          {
              FTD_ERR (FTD_WRNLVL, "%s%d: Could only allocate %d/%d I/O buffers. Performance might be affected.\n", DRIVERNAME, dev_minor, i, FTD_MAX_BUF);
              break;
          }   
      }
      buf->b_flags |= B_BUSY;
      BP_PRIVATE (buf) = softp->freelist;
      softp->freelist = buf;
  }
#endif

  /* add device to linked list */
  softp->next = lgp->devhead;
  lgp->devhead = softp;
  lgp->ndevs++;
  
#if defined (LINUX260) 
  min = getminor(info.bdev);
  localbdev = bdget(softp->localbdisk);

  ftd_blkdev_get(localbdev, O_RDWR|O_SYNC);

  localsize = localbdev->bd_inode->i_size >> DEV_BSHIFT;

  bdev = ftd_blkdev + (min - 1);
  
  bdev->bd_disk = alloc_disk(1);
  if(bdev->bd_disk == NULL)
  {
      err = ENOMEM;
      goto ERROR_EXIT;
  }

  softp->queue = blk_alloc_queue(GFP_KERNEL);
  if(softp->queue == NULL)
  {
      err = ENOMEM;
      goto ERROR_EXIT;
  }
  bdev->bd_disk->queue = softp->queue;
  
  ftd_gendisk_init(bdev, min-1);

  set_capacity(bdev->bd_disk, localsize);
  map_physical_device_attributes_to_logical_device(localbdev, bdev);

  ftd_blkdev_put(localbdev);

  add_disk(bdev->bd_disk);
#endif

#if defined(MISALIGNED)
  *((ftd_dev_info_t *) arg) = tmpinfo;
#else /* defined(MISALIGNED) */
  bcopy((void *)&tmpinfo, (void *)arg, sizeof(tmpinfo));
#endif
  

  return (0);
ERROR_EXIT:
#if defined(linux)
  if (softp->lrdb.map) {
    kmem_free(softp->lrdb.map, info.lrdbsize32 * 4);
    softp->lrdb.map = NULL;
  }
  if (softp->hrdb.map) {
    kmem_free(softp->hrdb.map, softp->hrdb.len32 * 4);
    softp->hrdb.map = NULL;
  }
  if (softp->lrdb.mapnext) {
      kmem_free(softp->lrdb.mapnext, info.lrdbsize32 * 4);
      softp->lrdb.mapnext = NULL;
  }
  if (softp->hrdb.mapnext) {
      kmem_free(softp->hrdb.mapnext, softp->hrdb.len32 * 4);
      softp->hrdb.mapnext = NULL;
  }

  if(bdev && bdev->bd_disk)
  {
      del_gendisk(bdev->bd_disk);
  }
  
#endif
  return err;
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
    // The following is a bit of black magic, as I couldn't find any clear doc on the API.
    // Because of this, the proper decision between _S_IFBLK and _S_IFCHR isn't clear.
    // If we ever switch to _S_IFCHR, make sure we pass the proper char device number.
    int rc = opend(&lgp->persistent_store, _S_IFBLK, FWRITE, NULL);

    if (rc)
    {
        return rc;
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
#elif defined(linux)
  ftd_layered_open( &lgp->bd, lgp->persistent_store);
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
  ftd_context_t context;
  ftd_uint32_t lgnum;		/* for Dynamic Mode Change */


  Enter (int, FTD_CTL_NEW_LG, 0, 0, arg, 0, 0);

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
  info.persistent_store = DEVEXPL(info.persistent_store);
#elif defined(_AIX)
  info.lgdev = DEV32TODEV(info.lgdev);
  info.persistent_store = DEV32TODEV(info.persistent_store);
#elif defined (LINUX260) 
  info.lgdev = huge_decode_dev(info.lgdev);
  info.persistent_store = huge_decode_dev(info.persistent_store);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

#if defined(FTD_DEBUG) && 0
  FTD_DPRINTF (FTD_WRNLVL,
	   "ftd_ctl_new_lg: info.lgdev: 0x%08x info.persistent_store: 0x%08x info.statsize: 0x%08x", info.lgdev, info.persistent_store, info.statsize);
#endif /* defined(FTD_DEBUG) */

  if ((ctlp = ftd_global_state) == NULL)
    return (ENXIO);

  lg_minor = getminor (info.lgdev) & ~FTD_LGFLAG;

  if (ddi_soft_state_zalloc (ftd_lg_state, lg_minor) != DDI_SUCCESS) {
    return (EADDRINUSE);
  }
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(lg_minor));
  if (lgp == NULL) {
#if defined(FTD_DEBUG)
	FTD_DPRINTF (FTD_WRNLVL, "ftd_ctl_new_lg: ddi_get_soft_state(%08x,%08x)\n",
		 ftd_lg_state, lg_minor);
#endif /* defined(FTD_DEBUG) */
	return (ENOTTY);
  }
#if defined(linux)
  /* initialize poll wait queue */
  init_waitqueue_head(&(lgp->ph));
#endif /* defined(linux) */

  /* allocate a bab manager */
  if ((lgp->mgr = ftd_bab_alloc_mgr ()) == NULL) {
	ddi_soft_state_free (ftd_lg_state, lg_minor);
	return (ENXIO);
  }

  lgp->statbuf = (ftd_char_t *) kmem_zalloc (info.statsize, KM_SLEEP);
  if (lgp->statbuf == NULL) {
	ftd_bab_free_mgr (lgp->mgr);
	ddi_soft_state_free (ftd_lg_state, lg_minor);
	return (ENOMEM);
  }

  ALLOC_LOCK (lgp->lock, "ftd lg lock");

  lgp->statsize = info.statsize;

  lgp->ctlp = ctlp;
  lgp->dev = info.lgdev;

  lgp->next = ctlp->lghead;
  ctlp->lghead = lgp;

  lgp->state = FTD_MODE_PASSTHRU;

  pending_ios_monitoring_init(lgp);

  lgp->sync_depth = (ftd_uint32_t) - 1;
  lgp->sync_timeout = 0;
  lgp->iodelay = 0;
  lgp->ndevs = 0;
  lgp->dirtymaplag = 0;
  lgp->devhead = NULL; 
  lgp->lrt_mode = 1;
  lgp->persistent_store = info.persistent_store;
  ftd_open_persistent_store (lgp);
  lgp->mgr->pending_io = 0;
#if defined(HPUX) && (SYSVERS >= 1100)
  lgp->read_waiter_thread = 0;
#endif
  // WI_338550 December 2017, implementing RPO / RTT
  // Init RPO fields
  lgp->PMDStats.LastIOTimestamp = 0;
  lgp->PMDStats.LastConsistentIOTimestamp = 0;
  lgp->PMDStats.OldestInconsistentIOTimestamp = 0;
  lgp->PMDStats.RTT = 0;
  lgp->PMDStats.average_of_most_recent_RTTs = 0;
  lgp->PMDStats.WriteOrderingMaintained = 0;
  lgp->PMDStats.network_chunk_size_in_bytes = 0;
  lgp->PMDStats.previous_non_zero_RTT = 0;
  Started_LG_Map.count++;
  if (Started_LG_Map.lg_max < lg_minor)
    Started_LG_Map.lg_max = lg_minor;
  Started_LGS[lg_minor]=FTD_STARTED_GROUP_FLAG;

  /* for Dynamic Mode Change */
  lgnum = (ftd_uint32_t)(getminor(lgp->dev) & ~FTD_LGFLAG);
  if (0 <= lgnum && lgnum < MAXLG) {
    sync_time_out_flg[lgnum] = 0;
  }
#if defined (linux)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
  mutex_init( &lgp->thrd_mutex );
#endif

  proc_fs_register_lg_proc_entries(lgp);
  
  lgp->th_lgno = lgnum;
  ftd_thrd_create(lgp);
  if (lgp->th_pid < 0)
        return (-lgp->th_pid);
#endif

  return (0);
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

  for (devp = lgp->devhead; devp; devp = devp->next) {
    if (devp->flags & FTD_DEV_OPEN) {
	  return (EBUSY);
	}
  }

  for (devp = lgp->devhead; devp; devp = devp->next) {
      if (devp->flags & FTD_LOCAL_DISK_IO_CAPTURED) {
          int rc = ftd_dynamic_activation_init_captured_device_release(devp);
          if(rc) {
              return rc;
          }
      }
  }
  
  for (devp = lgp->devhead; devp; devp = devp->next) {
#ifdef ATOMIC_SET_BIT
      ATOMIC_SET_BIT(FTD_BITX_STOP_IN_PROGRESS, &devp->flags);
#else
      devp->flags |= FTD_STOP_IN_PROGRESS;
#endif
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
#elif defined(_AIX)
  devdev = DEV32TODEV(devdev);
#elif defined(LINUX260)
  devdev = huge_decode_dev(devdev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = getminor (devdev);
  softp = (ftd_dev_t *) ddi_get_soft_state (ftd_dev_state, U_2_S32(minor));
  if (!softp)
    return (ENXIO);

  /*
   * XXX: ideally should have spin lock protection.
   */
  if ((softp->flags & FTD_DEV_OPEN) || softp->qhead)
    return (EBUSY);

  return ftd_del_device (softp, minor);
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
  ftd_int32_t sleeper;
  ftd_ctl_t *ctlp;
  ftd_context_t context;
  int del_lg_rc = 0;
  
  if ((ctlp = ftd_global_state) == NULL)
    return (ENXIO);

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
#elif defined(_AIX)
  devdev = DEV32TODEV(devdev);
#elif defined(LINUX260)
  devdev = huge_decode_dev(devdev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = getminor (devdev) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));


  if (!lgp)
    return (ENXIO);

#if defined(linux)
  proc_fs_unregister_lg_proc_entries(lgp);
  
  ftd_thrd_destroy(lgp);
#endif

  while ((del_lg_rc = ftd_del_lg (lgp, minor)) == FTD_IOPENDING)
  {
      FTD_DELAY(10000);
  }

  if (del_lg_rc != 0)
  {
      return del_lg_rc;
  }
  
  Started_LGS[minor]=FTD_STOP_GROUP_FLAG;
  Started_LG_Map.count--;

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
#elif defined(_AIX)
  /* lg_num is left as it is. */
  sb.dev_num = DEV32TODEV(sb.dev_num);
#elif defined(LINUX260)
  sb.dev_num = huge_decode_dev(sb.dev_num);
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

  if (ddi_copyout ((caddr_t) devp->statbuf, (caddr_t)(unsigned long) sb.addr, sb.len, flag))
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
#elif defined(_AIX)
  /* lg_num is left as it is. */
  sb.dev_num = DEV32TODEV(sb.dev_num);
#elif defined(LINUX260)
  sb.dev_num = huge_decode_dev(sb.dev_num);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  /* get the logical group state */
  minor = getminor (sb.lg_num) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (!lgp)
    return (ENXIO);
  if (sb.len != lgp->statsize)
    return (EINVAL);
  if (ddi_copyout ((caddr_t)(unsigned long) lgp->statbuf, (caddr_t)(unsigned long) sb.addr, sb.len, flag))
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
#elif defined(_AIX)
  /* lg_num is left as it is. */
  sb.dev_num = DEV32TODEV(sb.dev_num);
#elif defined(LINUX260)
  sb.dev_num = huge_decode_dev(sb.dev_num);
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

  if (ddi_copyin ((caddr_t)(unsigned long) sb.addr, (caddr_t)(unsigned long) devp->statbuf, sb.len, flag))
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
#elif defined(_AIX)
  /* lg_num is left as it is. */
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  /* get the logical group state */
  minor = getminor (sb.lg_num) & ~FTD_LGFLAG;
  lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
  if (lgp == NULL)
    return (ENXIO);

  if (sb.len != lgp->statsize)
    return (EINVAL);

  if (ddi_copyin ((caddr_t)(unsigned long) sb.addr, (caddr_t)(unsigned long) lgp->statbuf, sb.len, flag))
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
  ftd_context_t context;

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
  ftd_context_t context;

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
  ftd_context_t context;

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
 * ftd_ctl_set_jless()
 *
 * set logical group JLESS parameter
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_set_jless (ftd_intptr_t arg)
{
  ftd_param_t vb;
  ftd_lg_t *lgp;
  minor_t minor;
  ftd_context_t context;

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
  SET_LG_JLESS(lgp, vb.value);
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
#elif defined(_AIX)
  dev = DEV32TODEV(dev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */

  minor = dev;
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
    case FTD_MODE_CHECKPOINT_JLESS:
    case FTD_MODE_NORMAL:	/* do nothing */
      break;
    default:
      return (ENOTTY);
    }
  /* Need to somehow flush the state to disk here, no? XXX FIXME XXX */

  return (0);
}


/*-
 * ftd_ctl_capture_source_device_io
 *
 * Enable or release the capture of the source device IO according to the capture parameter.
 * Enables if capture = 1, releases otherwise.
 *
 * A request to release a captured device on a non captured device is not considered an error.
 */
FTD_PRIVATE ftd_int32_t
ftd_ctl_capture_source_device_io(ftd_intptr_t arg, int capture)
{
    ftd_int32_t rc = 0;
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
#elif defined(_AIX)
    devdev = DEV32TODEV(devdev);
#elif defined(LINUX260)
  devdev = huge_decode_dev(devdev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */
#endif /* defined(MISALIGNED) */
    
    minor = getminor (devdev);
    softp = (ftd_dev_t *) ddi_get_soft_state (ftd_dev_state, U_2_S32(minor));
    if (!softp)
        return (ENXIO);
    
    if(capture)
    {
        rc = ftd_dynamic_activation_capture_device(softp);

#if defined(_AIX)
        // When capturing a device, we must drop our handle to it and close it.
        // Otherwise, at least mount reports the device as being busy, interfering with a transparent usage of the captured device.
        // The side effect is that the associated dtc devices are rendered useless, since we cannot forward IO to the source device
        // if it is not currently opened.
        // This behavior has only been detected on AIX.
        if(softp->dev_fp)
        {
            fp_close (softp->dev_fp);
            softp->dev_fp = NULL;
        }
#endif
        
    }
    else if(softp->flags & FTD_LOCAL_DISK_IO_CAPTURED)
    {
        // We only release if needed.
        rc = ftd_dynamic_activation_release_captured_device(softp);

        // If we ever allow the captured devices to be released without stopping the group,
        // we would then need to reopen the target device on AIX here.
        // This is needed because the source device must be opened for us to be able to forward IO to it.
        // Not reopening the source device here will leave the dtc devices useless.
    }
    
    return rc;
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
#if defined(linux)
    case IOC_CST (FTD_OPEN_NUM_INFO):
      err = ftd_ctl_open_num_info((ftd_open_num_t *)(unsigned long)arg);
      break;
#endif /* defined(linux) */
    case IOC_CST (FTD_HRDB_TYPE):
      err = ftd_ctl_hrdb_type (arg);
      break;
    case IOC_CST (FTD_SET_LRDB_BITS):
      err = ftd_ctl_set_lrdb (arg);
      break;
    case IOC_CST (FTD_SET_LRDB_MODE):
      err = ftd_ctl_set_lrdb_mode (arg);
      break;
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
    // WI_338550 December 2017, implementing RPO / RTT
    case IOC_CST(FTD_SET_GROUP_RPO_STATS):
        err = ftd_ctl_set_group_RPO_stats(arg, flag);
        break;
    case IOC_CST (FTD_GET_GROUP_STARTED):
      err = ftd_ctl_get_started_group (arg, flag);
      break;
    case IOC_CST (FTD_SET_GROUP_STATE):
      err = ftd_ctl_set_group_state (arg, 0);
      break;
    case IOC_CST (FTD_OVERRIDE_GROUP_STATE):
      err = ftd_ctl_set_group_state (arg, 1);
      break;
    case IOC_CST (FTD_SET_DEVICE_SIZE):
      err = ftd_ctl_set_device_size (arg);
      break;
    case IOC_CST (FTD_GET_MAX_DEVICE_SIZE):
      err = ftd_ctl_get_max_device_size (arg);
      break;
    case IOC_CST (FTD_SET_MODE_TRACKING):
      err = ftd_ctl_set_mode_tracking (arg);
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
    case IOC_CST (FTD_SET_JLESS):
      err = ftd_ctl_set_jless (arg);
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
    case IOC_CST (FTD_GET_SYSCHK_COUNTERS):
      err = ftd_ctl_get_syschk_counters(arg);
      break;
    case IOC_CST (FTD_CLEAR_SYSCHK_COUNTERS):
      err = ftd_ctl_clear_syschk_counters(arg);
      break;
    case IOC_CST (FTD_CAPTURE_SOURCE_DEVICE_IO):
      err = ftd_ctl_capture_source_device_io(arg, 1);
      break;
    case IOC_CST (FTD_RELEASE_CAPTURED_SOURCE_DEVICE_IO):
      err = ftd_ctl_capture_source_device_io(arg, 0);
      break;
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
  ftd_context_t context;
  void *voidp;
  int i;

#if defined(MISALIGNED)
  sb = *(stat_buffer_t *) arg;
#else  /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&sb, sizeof(sb));
#endif /* defined(MISALIGNED) */

  /* enforce that this be a multiple of DEV_BSIZE */
  if (sb.len & (DEV_BSIZE - 1))
    return (EINVAL);

  if ((voidp = kmem_zalloc(sb.len, KM_SLEEP)) == NULL)
    return (ENOMEM);

  if (ddi_copyin((caddr_t)(unsigned long)sb.addr, voidp, sb.len, flag)) {
    kmem_free(voidp, sb.len);
    return (EINVAL);
  }

  size64 = sizeof_64bit (wlheader_t) + (sb.len / 8);
  ACQUIRE_LOCK (lgp->lock, context);
  if ((ftd_bab_alloc_memory (lgp->mgr, size64)) != 0) {
      /* 
       * this arithmetic assumes lgp->mgr->from[0] and starting pointer of
       * this write log are in the same BAB chunk. Make sure we guarantee
       * this when writing new log into BAB.
       */
      temp = lgp->mgr->from[0] - sizeof_64bit(wlheader_t);
      hp = (wlheader_t *) temp;
      hp->majicnum = DATASTAR_MAJIC_NUM;
      hp->offset = (ftd_uint64_t)-1;
      hp->length = sb.len >> DEV_BSHIFT;
      hp->span = lgp->mgr->num_frags;
      hp->dev = (dev_t)-1;
      hp->diskdev = (dev_t)-1;
#if defined(SOLARIS) || defined(_AIX)
      hp->group_ptr = (ftd_uint64ptr_t) lgp;
#elif defined(linux)
      hp->group_ptr = (ftd_uint64ptr_t)(unsigned long) lgp;
#else  /* defined(SOLARIS) || defined(_AIX) */
      hp->group_ptr = lgp;
#endif /* defined(SOLARIS) || defined(_AIX) */
      hp->complete = 1;
      hp->timoid = INVALID_TIMEOUT_ID;
      hp->flags = 0;
      hp->bp = 0;
      lgp->wlentries++;
      lgp->wlsectors += hp->length;
      /* Copy the message in */
      /* WR36350:
       * Data copied into BAB are CP_ON, CP_OFF, RFD_INCO, and RFD_CO markers.
       * Although markers are short strings, each of them still occupies a
       * DEV_BSIZE which may cross 2 BAB chunks.
       */  
      for (i = 0; i < lgp->mgr->num_frags; i++) {
          bcopy(voidp, (caddr_t)(lgp->mgr->from[i]), lgp->mgr->count[i]);
      }
      /*-
       * Don't commit the memory unless we're the last one on the list.
       * Otherwise we may commit part of an outstanding I/O and all hell
       * is likely to break loose.
       */
      if (ftd_bab_get_pending(lgp->mgr) == (ftd_uint64_t *) hp) {
          if (hp->majicnum != DATASTAR_MAJIC_NUM)
              panic("FTD: ftd_lg_send_lg_message: BAD DATASTAR_MAJIC_NUM");

          ftd_bab_commit_memory (lgp->mgr, size64);
          // ##@@ Update stats
          lgp->ulCommittedByteCount += ((ftd_uint64_t)hp->length << DEV_BSHIFT);
      }
    } else {
      RELEASE_LOCK (lgp->lock, context);
      kmem_free(voidp, sb.len);
      return (EAGAIN);
    }

    // ##@@ Update stats
    lgp->ulPendingByteCount += ((ftd_uint64_t)hp->length << DEV_BSHIFT);
  RELEASE_LOCK (lgp->lock, context);
  kmem_free(voidp, sb.len);
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
  ftd_context_t context;
  ftd_uint32_t lgnum;		/* for Dynamic Mode Change */

#if defined(MISALIGNED)
  oe = *(oldest_entries_t *) arg;
#else  /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&oe, sizeof(oe));
#endif /* defined(MISALIGNED) */

  offset64 = oe.offset32 >> 1;
  destlen64 = oe.len32 >> 1;

  /* get a snapshot of the amount of committed memory at this time */
  ACQUIRE_LOCK (lgp->lock, context);
  len64 = ftd_bab_get_committed_length (lgp->mgr);

  if (len64 == 0) {
      oe.retlen32 = 0;
      err = 0;
  } else {
    len64 -= offset64;
    if (len64 > 0) {
	  if (destlen64 > len64) {
        destlen64 = len64;
      }
	  /* copy the data */
      // WARNING: The lgp->lock must be held when calling ftd_bab_copy_out().
	  ftd_bab_copy_out (lgp->mgr, offset64, (ftd_uint64_t *)(unsigned long) oe.addr, destlen64, &lgp->lock, &context);
	  oe.retlen32 = destlen64 * 2;
	  err = 0;
	} else {
	  oe.retlen32 = 0;
	  err = EINVAL;
	}
  }

  // WARNING: The lgp->lock must be held when calling ftd_bab_copy_out().
  RELEASE_LOCK (lgp->lock, context);

  
  /* for Dynamic Mode Change */
  lgnum = (ftd_uint32_t)(getminor(lgp->dev) & ~FTD_LGFLAG);
  if (0 <= lgnum && lgnum < MAXLG) {
    if (lgp->state == FTD_MODE_TRACKING && sync_time_out_flg[lgnum] == 1) {
      oe.state = FTD_MODE_SYNCTIMEO;
    } else {
      oe.state = lgp->state;
    }
  } else {
    oe.state = lgp->state;
  }

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
  ftd_context_t context;

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
      if (tempdev->lgp->lrt_mode)
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
      lgp->tempdev = tempdev;	/* IPF only */
      ftd_flush_lrdb (lgp->tempdev, NULL, context);
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
    ftd_context_t context;
    int rc = 0;  /* default success */

#if defined(MISALIGNED)
    me = *(migrated_entries_t *) arg;
#else  /* defined(MISALIGNED) */
    bcopy((void *)arg, (void *)&me, sizeof(me));
#endif /* defined(MISALIGNED) */

    if (me.bytes == 0)
        return (0);

    if (me.bytes & 0x7) {
        FTD_ERR (FTD_WRNLVL, "Tried to migrate %d bytes, which is illegal", me.bytes);
        return (EINVAL);
    }

    if (lgp->mgr->in_use_head == NULL) {
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
    while (temp && bytes > 0) {
        wl = (wlheader_t *) temp;


        if (lgp->wlentries <= 0) {
            FTD_ERR (FTD_WRNLVL, "%s: Trying to migrate too many entries", DRIVERNAME);
            rc = EINVAL;
            break;
        }
        if (lgp->wlsectors <= 0) {
            FTD_ERR (FTD_WRNLVL, "%s: Trying to migrate too many sectors", DRIVERNAME);
            rc = EINVAL;
            break;
        }
        if (wl->majicnum != DATASTAR_MAJIC_NUM) {
            FTD_ERR (FTD_WRNLVL, "%s: corrupted BAB entry migrated", DRIVERNAME);
            rc = EINVAL;
            break;
        }
        if (wl->complete == 0) {
            FTD_ERR (FTD_WRNLVL, "%s: Trying to migrate uncompleted entry", DRIVERNAME);
            rc = EINVAL;
            break;
        }
        if (lgp->wlsectors < wl->length) {
            FTD_ERR (FTD_WRNLVL, "%s: Migrated too many sectors!", DRIVERNAME);
            rc = EINVAL;
            break;
        }
        if (wl->bp) {
            /* cancel any sync mode timout */
#if defined(SOLARIS)
            if (wl->timoid != INVALID_TIMEOUT_ID) {
#if (SYSVERS >= 570)
                timeout_id_t passtimoid=(timeout_id_t)wl->timoid;
                untimeout (passtimoid);
#else  /* (SYSVERS >= 570) */
                untimeout (wl->timoid);
#endif /* (SYSVERS >= 570) */
                wl->timoid = INVALID_TIMEOUT_ID;
            }
#elif defined(_AIX)
            if (wl->timoid != INVALID_TIMEOUT_ID) {
                ftd_aixuntimeout (wl->timoid);
                wl->timoid = INVALID_TIMEOUT_ID;
            }
#elif defined(HPUX)
            /* dequeue this entry from timer queue */
#if (SYSVERS >= 1100)
            if (ftd_debug == 5)
                FTD_ERR (FTD_WRNLVL, "ftd_lg_migrate: ftd_timer_dequeue");
#endif
            if (wl->timoid != INVALID_TIMEOUT_ID) {
                ftd_timer_dequeue (lgp, wl);
            }
#elif defined(linux)
            if (wl->timoid != INVALID_TIMEOUT_ID) {
                del_timer_sync((struct timer_list*)(unsigned long)wl->timoid);
                wl->timoid = INVALID_TIMEOUT_ID;
            }

#endif /* defined(SOLARIS) */
	  {
	  struct buf *passbp = (struct buf *)(unsigned long)wl->bp;
#if defined(linux)
          ftd_biodone (passbp, 0);
#else
	  ftd_biodone (passbp);
#endif
	  }
        }
        wl->bp = 0;
        if (wl->dev != lastdev) {
            softp = ftd_lookup_dev (lgp, wl->dev);
            lastdev = wl->dev;
        }
        if (softp) {
            softp->wlentries--;
            softp->wlsectors -= wl->length;
        }
        bytes -= ((wl->length << (DEV_BSHIFT - 3)) +
        sizeof_64bit (wlheader_t)) << 3;
        lgp->wlentries--;
        lgp->wlsectors -= wl->length;
        // ##@@ Update the stats
        lgp->ulMigratedByteCount += ((ftd_uint64_t)wl->length << DEV_BSHIFT);
        /*BAB_MGR_NEXT_BLOCK (temp, wl, buf); */
/* + */
        if (wl->span > 1) {
            len64 = buf->alloc - (temp + sizeof_64bit(wlheader_t));
            nlen64 = FTD_LEN_QUADS(wl) - len64;
            for (i = 2; i < (wl->span); i++) {
                buf = buf->next;
                nlen64 -= buf->alloc - buf->start;
            }
            if ((buf = buf->next) == NULL) {
                temp = NULL;
            } else {
                temp = buf->start + nlen64;

                /* WR35600: 
                * Boundary check, the end of the wl may as well the end
                * of the buffer usage.
                */
                if (temp == buf->alloc) {
                    if ((buf = buf->next) == NULL) {
                        temp = NULL;
                    } else {
                        temp = buf->start;
                    }
                }
                /* we will not check MIN_BAB_SPACE_REQUIRED here in "else"
                since these codes shall not be reached. But make sure checking
                here shall be exactly the same as checking below. 
                */
            }
        } else {
            temp += BAB_WL_LEN(wl);
            if (temp == buf->alloc) {
                if ((buf = buf->next) == NULL) {
                    temp = NULL;
                } else {
                    temp = buf->free;
                }
            } else {
                /*
                * WR36257: skip fragmental space at the end of BAB
                * this code will not be reached as buf->alloc shall always
                * point to the last used BAB space.
                */
                if ((bytes > 0) && ((buf->end - temp) <= MIN_BAB_SPACE_REQUIRED)) {
                    if ((buf = buf->next) == NULL) {
                        temp = NULL;
                    } else {
                        temp = buf->start;
                    }
                }
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
    /* PB WR37202
    lgp->dirtymaplag++;
    */
    RELEASE_LOCK (lgp->lock, context);
    /* PB WR37202
    if (lgp->dirtymaplag == FTD_DIRTY_LAG && lgp->wlsectors < FTD_RESET_MAX) {
        lgp->dirtymaplag = 0;
        ftd_lg_update_dirtybits (lgp);
    }
    */

    return (rc);
}


/**
 * ftd_lg_get_dirty_bits()
 *
 * for a given device, returns HRT map
 *
 * @bug  Nothing prevents the values of the bitmaps to be changed while they're being copied to userspace.
 * @todo Since we can't copy to userspace while holding the locks, we should make a local copy of the bitmaps while
 *       holding the locks and copy to userspace from this local copy.  This should be trivial with a variable length
 *       array if we can rely on C99 for all compilers.
 */
FTD_PRIVATE ftd_int32_t
ftd_lg_get_dirty_bits (ftd_lg_t * lgp, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag)
{
    ftd_int32_t i;
    ftd_dev_t_t tmpdev;
    dev_t dev;
    ftd_dev_t *softp;
    dirtybit_array_kern_t dbarray;
    ftd_int32_t *tmpdbbuf;
  
#if defined(MISALIGNED)
    dbarray = *(dirtybit_array_kern_t *) arg;
#else  /* defined(MISALIGNED) */
    bcopy((void *)arg, (void *)&dbarray, sizeof(dbarray));
    tmpdbbuf = (ftd_int32_t *)(unsigned long)dbarray.dbbuf;
#endif /* defined(MISALIGNED) */

    /*-
     * The following tricky bit of logic is intended to return ENOENT
     * if the bits are invalid.  The bits are valid only in RECOVER mode
     * and when in TRACKING mode with no entries in the writelog
     */
    if (dbarray.state_change &&
        ((lgp->state & FTD_M_BITUPDATE) == 0 || lgp->wlentries != 0))
        return (ENOENT);

    dev = dbarray.dev;
#if defined(SOLARIS) && (SYSVERS >= 570)
	dev = DEVEXPL(dev);
#elif defined(_AIX)
	dev = DEV32TODEV(dev);
#elif defined(LINUX260)
	dev = huge_decode_dev(dev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */

    /* find the matching device in our logical group */
    softp = lgp->devhead;
    while (softp != NULL) {
        if (dev == softp->cdev) {
            if (IOC_CST (cmd) == IOC_CST (FTD_GET_LRDBS)) {

                if (ddi_copyout ((caddr_t) softp->lrdb.map, (caddr_t) tmpdbbuf, softp->lrdb.len32 * 4, flag)) {
                    return (EFAULT);
                }

            } else {
			    if( dbarray.dblen32 < softp->hrdb.len32 )
				{
                    // Protect against copy buffer overflow
	                FTD_ERR (FTD_WRNLVL, "ftd_lg_get_dirty_bits: destination buffer length (%d) is less than source (%d)\n",
	                         dbarray.dblen32, softp->hrdb.len32);
                    return (EFAULT);
				}

                if (ddi_copyout ((caddr_t) softp->hrdb.map, (caddr_t) tmpdbbuf, softp->hrdb.len32 * 4, flag)) {
                    return (EFAULT);
                }

                dbarray.dblen32=softp->hrdb.len32;
            }
            break;
	    }
        softp = softp->next;
	}

    if (softp == NULL)
    {
        // The given device wasn't found.
        return ENXIO;
    }
    
if (dbarray.state_change)
    lgp->state = FTD_MODE_REFRESH;
    /* Need to somehow flush the state to disk here, no? XXX FIXME XXX */

#if defined(MISALIGNED)
    *((dirtybit_array_kern_t *) arg) = dbarray;
#else /* defined(MISALIGNED) */
    bcopy((void *)&dbarray, (void *)arg, sizeof(dbarray));
#endif /* defined(MISALIGNED) */

    // Waiting for no pending ios here ensures that all dirty bits in the bitmap obtained above have been
    // properly written to disk before the bitmap is returned.

    // This ensures that any smart refresh starting immediately after obtaining this bitmap will read any recently
    // updated data that is in the bitmap.

    // Since the bitmaps are normally obtained by the PMD after changing to the refresh mode, waiting for pending io completion
    // here would logically not be needed, as we already wait for pending IO completion in the FTD_SET_GROUP_STATE ioctl.

    // But since the PMD empties the bab before launching its smart refresh, not waiting for pending IOs here 
    // opens the door to the following scenario:

    // 1-) While in FTD_MODE_FULLREFRESH, an IO comes in to write the value v at location x.
    // 2-) FTD_SET_GROUP_STATE is called to put the driver in FTD_MODE_REFRESH, before returning, we are guaranteed
    //     that v has been written.
    // 3-) Another IO comes in to write the value v' at the same location x.
    // 4-) The bitmap is obtained, while v' is pending and hasn't been sent to disk yet.
    // 5-) The bab is emptied and v' gets sent to the RMD
    // 6-) While performing the refresh, the old value of v is read at location x.
    // 7-) v' is finally written to disk.

    // I wonder why the bab is emptied at the beginning of a smart refresh.
    // A hunch tells me that there may be no valid reason to do so anymore, and that it's a leftover from
    // the time that the INCO sentinel was put in the bab rather than sent on the network and needed to be
    // explicitely drained from the bab then.

    // We should review if the empty_bab() calls in rsyncinit are worthwile or not, and remove them and even possibly this
    // call to wait for IO completion if not.
    // If we don't call empty_bab() at the beginning of a smart refresh, are there any remaining risks?
    return pending_ios_monitoring_wait_for_pending_ios_completion(softp);
}

/**
 * ftd_lg_merge_high_resolution_dirty_bits()
 */
FTD_PRIVATE ftd_int32_t
ftd_lg_merge_high_resolution_dirty_bits (ftd_lg_t * lgp, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag)
{
    dirtybit_array_kern_t dbarray;
    ftd_dev_t_t tmpdev;
    dev_t dev;
    ftd_int32_t i;
    ftd_dev_t *softp;
    ftd_int32_t err;
    ftd_int32_t *tmpdbbuf;
    ftd_int32_t rc = 0;
    
#if defined(MISALIGNED)
    dbarray = *(dirtybit_array_kern_t *) arg;
#else  /* defined(MISALIGNED) */
    bcopy((void *)arg, (void *)&dbarray, sizeof(dbarray));
    tmpdbbuf = (ftd_int32_t *)(unsigned long)dbarray.dbbuf;
#endif /* defined(MISALIGNED) */

    dev = dbarray.dev;
#if defined(SOLARIS) && (SYSVERS >= 570)
    dev = DEVEXPL(dev);
#elif defined(_AIX)
       dev = DEV32TODEV(dev);
#elif defined(LINUX260)
       dev = huge_decode_dev(dev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */

    if (lgp->DualBitmaps == FALSE)
    {
        FTD_ERR (FTD_WRNLVL,"ftd_lg_merge_high_resolution_dirty_bits for group %03d: Silently ignoring merge request because the mechanism hasn't been enabled.\n",
                 getminor(lgp->dev) & ~FTD_LGFLAG);
        return 0;
    }
       
    if ((softp = ftd_lg_get_device (lgp, dev)) != NULL)
	{
        ftd_uint32_t* user_space_hrdb = (ftd_uint32_t *) kmem_zalloc (softp->hrdb.len32 * 4, KM_SLEEP);

        if(user_space_hrdb == NULL)
        {
            rc = ENOMEM;
        }
        else
        {
            if (ddi_copyin ((caddr_t) tmpdbbuf, (caddr_t) user_space_hrdb, softp->hrdb.len32 * 4, flag))
            {
                rc = EFAULT;
            }
            else
            {
                ftd_context_t context;
                ACQUIRE_LOCK (lgp->lock, context);
                {
                    ftd_uint32_t hrdb_32bit_chunk_index = 0;

                    // Set the current bitmap to be the merge (logical or) of the incoming HRDB and of the historical/dual one (mapnext).
                    for (hrdb_32bit_chunk_index = 0; hrdb_32bit_chunk_index < softp->hrdb.len32; hrdb_32bit_chunk_index++)
                    {
                        ftd_uint32_t* hrdb_32bit_chunk = softp->hrdb.map + hrdb_32bit_chunk_index;
                        ftd_uint32_t* user_space_hrdb_32bit_chunk =  user_space_hrdb + hrdb_32bit_chunk_index;
                        ftd_uint32_t* mapnext_hrdb_32bit_chunk = softp->hrdb.mapnext + hrdb_32bit_chunk_index;
                        ftd_uint32_t merged_hrdb_32bit_chunk = (*user_space_hrdb_32bit_chunk) | (*mapnext_hrdb_32bit_chunk);

#if defined(FTD_DEBUG)
                        if ( merged_hrdb_32bit_chunk != *hrdb_32bit_chunk)
                        {
                            FTD_ERR(FTD_DBGLVL, "MERGE HRDBS (group %03d): Offset %d:  Driver Before=%08lx, in=%08lx, next=%08lx, so Driver=%08lx\n",
                                    getminor(lgp->dev) & ~FTD_LGFLAG,
                                    hrdb_32bit_chunk_index,
                                    *hrdb_32bit_chunk,
                                    *user_space_hrdb_32bit_chunk,
                                    *mapnext_hrdb_32bit_chunk,
                                    merged_hrdb_32bit_chunk);
                        }
#endif /* FTD_DEBUG */
                        
                        *hrdb_32bit_chunk= merged_hrdb_32bit_chunk;
                    }
                
                    // Disable the dual bitmap when merged.
                    // Cannot do it per device when all devices of the group are affected!
                    //lgp->DualBitmaps = FALSE;
                }
                RELEASE_LOCK(lgp->lock, context);
            }
            
            kmem_free(user_space_hrdb, softp->hrdb.len32 * 4);
        }
    }
    else
	{
        rc = ENXIO;
	}
    
    return rc;
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

/**
 * ftd_lg_set_dirty_bits()
 * 
 * given an array of devices and dirty bitmaps, copy the dirty bitmaps into
 * our bitmaps.
 *
 * @bug  Nothing prevents the values of the driver's bitmaps to be changed while they're being updated from userspace.
 * @todo Since we can't copy from userspace while holding the locks, we should update the driver's bitmaps from a local copy while
 *       holding the locks.  This should be trivial with a variable length array if we can rely on C99 for all compilers.
 */
FTD_PRIVATE ftd_int32_t
ftd_lg_set_dirty_bits (ftd_lg_t * lgp, ftd_int32_t cmd, ftd_intptr_t arg, ftd_int32_t flag)
{
    dirtybit_array_kern_t dbarray;
    ftd_dev_t_t tmpdev;
    dev_t dev;
    ftd_int32_t i;
    ftd_dev_t *softp;
    ftd_int32_t err;
    ftd_int32_t *tmpdbbuf;

#if defined(MISALIGNED)
    dbarray = *(dirtybit_array_kern_t *) arg;
#else  /* defined(MISALIGNED) */
    bcopy((void *)arg, (void *)&dbarray, sizeof(dbarray));
    tmpdbbuf = (ftd_int32_t *)(unsigned long)dbarray.dbbuf;
#endif /* defined(MISALIGNED) */

    dev = dbarray.dev;
#if defined(SOLARIS) && (SYSVERS >= 570)
    dev = DEVEXPL(dev);
#elif defined(_AIX)
       dev = DEV32TODEV(dev);
#elif defined(LINUX260)
       dev = huge_decode_dev(dev);
#endif /* defined(SOLARIS) && (SYSVERS >= 570) */

    if ((softp = ftd_lg_get_device (lgp, dev)) != NULL)
	{
        if (IOC_CST (cmd) == IOC_CST (FTD_SET_LRDBS))
	    {
            if (ddi_copyin ((caddr_t) tmpdbbuf, (caddr_t) softp->lrdb.map, softp->lrdb.len32 * 4, flag))
            {
                return (EFAULT);
            }
	    }
        else
	    {
            if (ddi_copyin ((caddr_t) tmpdbbuf, (caddr_t) softp->hrdb.map, softp->hrdb.len32 * 4, flag))
            {
                return (EFAULT);
            }
	    }
	}
    else
	{
        return (ENXIO);
	}
   

    return (0);
}

/***************************************************************************************\

Function:       ftd_lg_backup_history_bits

Description:    This is used to keep a backup version of the dirty bitmaps, during a bits
clearing transition.  Once in a while, we want to clear the dirty bits since
they have been sent to the mirror.  However, to clear them, we have to make 
sure we received acknowledgment that they were written successfully on the 
other side.  So during the transition (data sent - waiting for ack) we keep
a version of the bits containing new IO's received since the clear.

Parameters:     lgp     Information about the associated logical group.
                arg     A pointer to a ftd_int32_t parameter.
                *arg    backup_mode.
                        When true, signals that we should produce a clean bitmap and update it in parallel with the older bitmap which is probably dirtier than needed.
                        When false, signals that we can throw away the older and too dirty bitmap, and only use the recently produced and cleaner bitmap.

Return Value:   FTD_STATUS 
Not Specified.

Comments:       implemented for RFW WR37202 

\***************************************************************************************/
FTD_PRIVATE ftd_int32_t
ftd_lg_backup_history_bits(ftd_lg_t* lgp, ftd_intptr_t arg)
{
    ftd_dev_t*  tempdev = NULL;
    unsigned int *pcurr = NULL;
    ftd_int32_t backup_mode=FALSE;
    ftd_context_t context;

    if (lgp == NULL)
    {
        FTD_ERR (FTD_WRNLVL, "ftd_lg_backup_history_bits: no logical group associated\n");
        return ENXIO;
    }

#if defined(MISALIGNED)
    backup_mode = *(ftd_param_t *) arg;
#else  /* defined(MISALIGNED) */
    bcopy((void *)arg, (void *)&backup_mode, sizeof(backup_mode));
#endif /* defined(MISALIGNED) */

    /* grab the group lock */
    ACQUIRE_LOCK (lgp->lock, context);

    if (lgp->state != FTD_MODE_NORMAL ||     /****** if not in NORMAL mode, exit here *****/
        backup_mode == lgp->KeepHistoryBits) /* or being out of sync */
    {
       lgp->KeepHistoryBits=FALSE;          /* reset the mode. */
        RELEASE_LOCK(lgp->lock, context);
        return (0); /* Reject smoothly to not force a Failure seen by PMD */
    }


    if (lgp->DualBitmaps == TRUE)
    {
        // TODO: Should we return some error code?
        // We're currently not doing anything since the Windows implementation doesn't do anything about it as well and
        // that it's not clear how we could end up in such a situation.
        FTD_ERR (FTD_WRNLVL,"Group %03d: Conflicting dual bitmaps mechanism.\n",  getminor(lgp->dev) & ~FTD_LGFLAG);
    }
    
#if defined(ftd_debugf)
    ftd_debugf(ioctl, "LG(%d) Backup_history_bits = %s\n", getminor(lgp->dev) & ~FTD_LGFLAG, (backup_mode == TRUE ? "ON": "OFF"));
#endif
    
    if (backup_mode == TRUE)                 /*WR37202 from ftd_lg_proc_bab() */
    {
        tempdev = lgp->devhead;
        while (tempdev)
        {
           /* WR37202 keep historical update on Lrdb.map, and is still kept persistent by write_pstore
            * but reset .mapnext for future updates */
            bzero((caddr_t) tempdev->lrdb.mapnext, tempdev->lrdb.len32 * 4);

            /* WR37202 keep historical update on Hrdb.map, even if not persistent by write_pstore
             * clean .mapnext for future updates */
            bzero((caddr_t) tempdev->hrdb.mapnext, tempdev->hrdb.len32 * 4);

            /* move to next device */
            tempdev = tempdev->next;
        }

        lgp->KeepHistoryBits = TRUE; /* WR37202  */

        /* recompute the dirtybits from BAB*/
        ftd_compute_dirtybits(lgp, FTD_LOW_RES_DIRTYBITS);       /* both .map and .mapnext will be computed */
        ftd_compute_dirtybits(lgp, FTD_HIGH_RES_DIRTYBITS);
    }

    else  /* when 1024 packets was acknowledged.  */
    {
       /* at this point,we are safe, all packets were echoed ok, so we could ignore bits prior to backup operation */

       lgp->KeepHistoryBits = FALSE; /* BACKUP MODE was active, now set it to inactive. */

        tempdev = lgp->devhead;
        while (tempdev)
        {
            /**
            * WR37202 Swap .map and .mapnext, and from now on, keep future updates just on .map, which is still persistent by write_pstore
            **/

            /* WR37202 swap .map and .mapnext for LRDB */
            
            /* Because softp->lrdbbp->b_un.b_addr always points to the initial dev->lrdb.map on non Linux implementations,
             * we cannot swap the map an mapnext pointers, as otherwise the pstore will end up writing from the incorrect bitmap
             * half of the time.  To avoid this problem, we copy the fresh bitmap over the stale one.
             * C.F. WR PROD00003667
             */
            bcopy(tempdev->lrdb.mapnext, tempdev->lrdb.map, tempdev->lrdb.len32 * 4);
            
            /* useless but ... it's cleaner, .map is now just containing bits post to backup operation */
            bzero((caddr_t) tempdev->lrdb.mapnext, tempdev->lrdb.len32 * 4); 


            /* WR37202 UNswap backup and current for HRDB */
            pcurr = tempdev->hrdb.map;
            tempdev->hrdb.map = tempdev->hrdb.mapnext; /*  <--------- important */
            tempdev->hrdb.mapnext = pcurr;                   
            /* useless but ... it's cleaner, .map is now just containing bits post to backup operation */
            bzero((caddr_t) tempdev->hrdb.mapnext, tempdev->hrdb.len32 * 4);

            /* move to next device */
            tempdev = tempdev->next;
        }

        /* compute the dirtybits ... always good to reread BAB */
        ftd_compute_dirtybits(lgp, FTD_LOW_RES_DIRTYBITS);       /* just .map will be computed */
        ftd_compute_dirtybits(lgp, FTD_HIGH_RES_DIRTYBITS);
    }       
    /* release the group lock */
    RELEASE_LOCK (lgp->lock, context);

    return 0;
}

/***************************************************************************************\

Function:       ftd_lg_get_syschk_counters

Description:    Retrieve statistics gathered on the BAB (byte count written and migrated).

Parameters:     pLg
pBuffer

Return Value:   FTD_STATUS 
Not Specified.

Comments:       None.

\***************************************************************************************/
FTD_PRIVATE ftd_int32_t
ftd_lg_get_syschk_counters(ftd_lg_t* lgp, ftd_intptr_t arg)
{
    ftd_context_t context;
    IOCTL_Get_SysChk_Counters_t Buffer;

    if (lgp == NULL)
    {
        FTD_ERR (FTD_WRNLVL, "ftd_lg_get_syschk_counters: no logical group associated\n");
        return ENXIO;
    }

    ACQUIRE_LOCK (lgp->lock, context);

    Buffer.ulWriteByteCount = lgp->ulWriteByteCount;
    Buffer.ulWriteDoneByteCount = lgp->ulWriteDoneByteCount;
    Buffer.ulPendingByteCount = lgp->ulPendingByteCount;
    Buffer.ulCommittedByteCount = lgp->ulCommittedByteCount;
    Buffer.ulMigratedByteCount = lgp->ulMigratedByteCount;

#if defined(MISALIGNED)
    *((IOCTL_Get_SysChk_Counters_t*) arg) = Buffer;
#else /* defined(MISALIGNED) */
    bcopy((void *)&Buffer, (void *)((ftd_int32_t *)arg), sizeof(Buffer));
#endif /* defined(MISALIGNED) */

    RELEASE_LOCK (lgp->lock, context);

    return 0;
}

/***************************************************************************************\

Function:       ftd_lg_clear_syschk_counters

Description:    Set all the BAB stats counters to 0.

Parameters:     pLg

Return Value:   FTD_STATUS 
Not Specified.

Comments:       None.

\***************************************************************************************/
FTD_PRIVATE ftd_int32_t
ftd_lg_clear_syschk_counters(ftd_lg_t* lgp)
{
    ftd_context_t context;

    if (lgp == NULL)
    {
        FTD_ERR (FTD_WRNLVL, "ftd_lg_clear_syschk_counters: no logical group associated\n");
        return ENXIO;
    }

    ACQUIRE_LOCK (lgp->lock, context);

    lgp->ulWriteByteCount = 0;
    lgp->ulWriteDoneByteCount = 0;
    lgp->ulPendingByteCount = 0;
    lgp->ulCommittedByteCount = 0;
    lgp->ulMigratedByteCount = 0;

    RELEASE_LOCK (lgp->lock, context);

    return 0;
}



FTD_PRIVATE ftd_int32_t
ftd_ctl_get_syschk_counters(ftd_intptr_t arg)
{
    IOCTL_Get_SysChk_Counters_t Buffer;
    minor_t minor;
    ftd_lg_t *lgp;

#if defined(MISALIGNED)
    Buffer = *(IOCTL_Get_SysChk_Counters_t *) arg;
#else  /* defined(MISALIGNED) */
    bcopy((void *)arg, (void *)&Buffer, sizeof(Buffer));
#endif /* defined(MISALIGNED) */

    minor = getminor (Buffer.lgnum) & ~FTD_LGFLAG;
    lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
    if (lgp == NULL)
        return (ENXIO);

    return ftd_lg_get_syschk_counters(lgp, arg);
}

FTD_PRIVATE ftd_int32_t
ftd_ctl_clear_syschk_counters(ftd_intptr_t arg)
{
    ftd_param_t param;
    minor_t minor;
    ftd_lg_t *lgp;

#if defined(MISALIGNED)
    param = *(ftd_param_t *) arg;
#else  /* defined(MISALIGNED) */
    bcopy((void *)arg, (void *)&param, sizeof(param));
#endif /* defined(MISALIGNED) */

    minor = getminor (param.lgnum) & ~FTD_LGFLAG;

    lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor));
    if (lgp == NULL)
        return (ENXIO);

    return ftd_lg_clear_syschk_counters(lgp);
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
  ftd_context_t context;

  minor_t minor = getminor (dev) & ~FTD_LGFLAG;
  if ((lgp = ddi_get_soft_state (ftd_lg_state, U_2_S32(minor))) == NULL) {
    return (ENXIO);		/* invalid minor number */
  }

#ifdef TDMF_TRACE
	ftd_ioctl_trace("lg",cmd);
#endif	    


  switch (IOC_CST (cmd)) {
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
  case IOC_CST (FTD_MERGE_HRDBS):
      err = ftd_lg_merge_high_resolution_dirty_bits(lgp, cmd, arg, flag);
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
  case IOC_CST (FTD_BACKUP_HISTORY_BITS):
    err = ftd_lg_backup_history_bits(lgp, arg);
    break;
  case IOC_CST (FTD_GET_SYSCHK_COUNTERS):
    err = ftd_lg_get_syschk_counters(lgp, arg);
    break;
  case IOC_CST (FTD_CLEAR_SYSCHK_COUNTERS):
    err = ftd_lg_clear_syschk_counters(lgp);
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
    ftd_int32_t psdev_bad;
    ftd_int32_t oval = LRDB_STAT_WANT;
    ftd_dev_t *softp = BP_SOFTP (bp);

    bp->b_flags |= B_DONE;

    /* Reentrant: read psdev_bad falg info for this device.
     * softp->psdev_bad should be inited to 0.
     */
    psdev_bad = softp->psdev_bad;
    
    softp->lrdb_finicnt++;

	/*-
	 * asynch error notification...
	 */

    if ((err = geterror (bp)) != 0)
      {
	softp->lrdb_errcnt++;
	FTD_ERR (FTD_WRNLVL,
	 "biodone_psdev: LRT map IO error. dev: 0x%08x blk: 0x%08x err: %d, PS_DONE_total=0x%08x, PS_DONE_err=0x%08x",
		 bp->b_dev, bp->b_blkno, err, softp->lrdb_finicnt, softp->lrdb_errcnt );
      }

    /* WR32409: LRT map IO error with errno 22 (seen only in AIX)
     *
     * If LRDB write the device is successful after a previous error,
     * show the corrected msg and mark it repaired/corrected.
     * The warning message should only show once after a sucessful write.
     */
    if (psdev_bad == 1 && err == 0) {
	FTD_ERR (FTD_WRNLVL,
	 "biodone_psdev: prior LRT map IO error CORRECTED. dev: 0x%08x blk: 0x%08x, PS_DONE_total=0x%08x, PS_DONE_err=0x%08x",
		 bp->b_dev, bp->b_blkno, softp->lrdb_finicnt, softp->lrdb_errcnt);
    }
    
    /* set to BAD or no error state.  Race condition should not occur as we
     * queue IO.
     */
    if (err == 0)
       softp->psdev_bad = 0;
    else
       softp->psdev_bad = 1;	/* psdev io error, not yet corrected */

    if (compare_and_swap ((atomic_p)(&(softp->lrdb_stat)), (int *)(&oval), nval) == TRUE)
      {

	/* reinitiate LRT map IO */

	LRDB_WANT_ACK (softp);

	softp->lrdb_reiocnt++;

        softp->lrdbbp->b_error = 0;

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

    ftd_dynamic_activation_initiate_io_to_a_possibly_captured_device(softp->lrdbbp);

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

#if defined(SOLARIS) || defined(HPUX) || defined(linux)

#if defined(HPUX)
static lock_t *atomic_l;
#endif /* defined(HPUX) */

#if defined(SOLARIS)
static kmutex_t atomic_l;
#endif /* defined(SOLARIS) */

#if defined(linux)
#include <linux/spinlock.h>
static spinlock_t atomic_l;
#endif /* defined(linux) */

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
  ftd_context_t context;

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
  ftd_context_t context;

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
  ftd_context_t context;

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

#if !defined(linux)
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
        {  /* clean up re-used buf struct */
           /* info to be kept in the buf struct before zero out */
           dev_t        psdev;
           caddr_t      lrdbmap;
           ftd_uint32_t lrdb_offset;
           size_t       bcount;
           struct buf   *pbuf;

           pbuf        = softp->lrdbbp;
           psdev       = softp->lgp->persistent_store;
           lrdbmap     = pbuf->b_un.b_addr;
           lrdb_offset = pbuf->b_blkno;
           bcount      = pbuf->b_bcount;

           /* we are re-using the buf structure, reset buf per DDI/DKI */
           bioreset(softp->lrdbbp);

           /* restore buf data info */
           BP_DEV (pbuf)      = psdev;
           pbuf->b_un.b_addr = lrdbmap;
           pbuf->b_blkno     = lrdb_offset;
           pbuf->b_bcount    = bcount;
        }  /* end clean up */

#ifdef B_KERNBUF
	softp->lrdbbp->b_flags = B_KERNBUF | B_WRITE | B_BUSY;
#else
	softp->lrdbbp->b_flags = B_WRITE | B_BUSY;
#endif/* B_KERNBUF */
	softp->lrdbbp->b_iodone = BIOD_CAST (biodone_psdev);
#endif /* defined(SOLARIS) */

#if defined(HPUX)
	softp->lrdbbp->b_flags = 0;
	softp->lrdbbp->b_iodone = BIOD_CAST (biodone_psdev);
	softp->lrdbbp->b_flags = B_CALL | B_WRITE | B_BUSY | B_ASYNC;

   #if 0	/* the following also work but different B_ASYNC ... */
        softp->lrdbbp->b_flags = B_CALL | B_WRITE | B_BUSY;
        softp->lrdbbp->b_flags &= ~(B_CACHE|B_BCACHE|B_ASYNC);
        softp->lrdbbp->b_flags |= B_NOCACHE;

   #define LVM_OPT_MWC     0x08000000      /* b_driver_un_1.longvalue */
        softp->lrdbbp->b_driver_un_1.longvalue &= ~(LVM_OPT_MWC);
        softp->lrdbbp->b2_flags = 0 | B2_NOMERGE;
   #endif

        /* make sure space id is correct */
         softp->lrdbbp->b_spaddr = ldsid(softp->lrdbbp->b_un.b_addr);

#endif /* defined(HPUX) */

	ftd_dynamic_activation_initiate_io_to_a_possibly_captured_device (softp->lrdbbp);

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
#endif /* !defined(linux) */

#endif /* defined(SOLARIS) || defined(HPUX) || defined(linux) */

FTD_PRIVATE ftd_int32_t
ftd_ctl_hrdb_type(ftd_intptr_t arg)
{
  ftd_int32_t hrt_type;
  ftd_ctl_t *ctlp;

  if ((ctlp = ftd_global_state) == NULL)
    return (ENXIO);

#if defined(MISALIGNED)
  hrt_type = *(ftd_int32_t *) arg;
#else  /* defined(MISALIGNED) */
  bcopy((void *)arg, (void *)&hrt_type, sizeof(hrt_type));
#endif /* defined(MISALIGNED) */

  if (hrt_type != FTD_HS_NOT_SET)
        ctlp->hrdb_type = hrt_type;

#if defined(MISALIGNED)
  ((ftd_int32_t *) arg) = &ctlp->hrdb_type;
#else /* defined(MISALIGNED) */
  bcopy((void *)&ctlp->hrdb_type,
        (void *)((ftd_int32_t *)arg), sizeof(ctlp->hrdb_type));
#endif /* defined(MISALIGNED) */

 return 0;
}

