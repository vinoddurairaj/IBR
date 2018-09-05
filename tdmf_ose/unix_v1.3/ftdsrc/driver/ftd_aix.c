/*-
 * Copyright (c) 1996, 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 * $Id: ftd_aix.c,v 1.3 2001/01/17 17:58:55 hutch Exp $
 *
 */
#include "ftd_kern_ctypes.h"
#if defined(_AIX)
#include <sys/device.h>
#include <sys/atomic_op.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/sysmacros.h>
#include "ftd_def.h"
#include "ftd_ddi.h"
#include "ftd_klog.h"
#include "ftd_kern_cproto.h"

/* ftd_aix.c is: */
ftd_int32_t
ftd_config (dev_t dev, ftd_int32_t cmd, struct uio *uiop);
ftd_int32_t
ftd_open (dev_t dev, ftd_uint32_t devflag, chan_t chan, ftd_int32_t ext);
ftd_int32_t
ftd_close (dev_t dev, chan_t chan);
ftd_int32_t
ftd_ioctl (dev_t dev, ftd_int32_t cmd, ftd_int32_t arg, ftd_uint32_t devflag, chan_t chan, ftd_int32_t ext);
ftd_int32_t
ftd_read (dev_t dev, struct uio *uiop, chan_t chan, ftd_int32_t ext);
ftd_int32_t
ftd_write (dev_t dev, struct uio *uiop, chan_t chan, ftd_int32_t ext);
ftd_void_t
ftd_strat (struct buf *bp);
ftd_int32_t
ftd_select (dev_t dev, ftd_uint16_t reqevents, ftd_uint16_t * reventp, ftd_int32_t chan);
ftd_void_t
ftd_wakeup (ftd_lg_t * lgp);
struct buf *
  ftd_bioclone (ftd_dev_t * softp, struct buf *sbp, struct buf *dbp);
ftd_dev_t *
  ftd_ld2soft (dev_t dev);
ftd_dev_t *
  ftd_get_softp (struct buf *bp);
ftd_int32_t
ftd_ctl_get_device_nums (dev_t dev, ftd_int32_t arg, ftd_int32_t flag);
ftd_int32_t
ftd_dev_ioctl (dev_t dev, ftd_int32_t cmd, ftd_int32_t arg, ftd_int32_t flag);

extern ftd_int32_t nodev ();

extern ftd_ctl_t *ftd_global_state;
extern ftd_void_t *ftd_lg_state;
extern ftd_void_t *ftd_dev_state;

struct devsw ftd_devsw =
{
  ftd_open,			/* entry point for open routine          */
  ftd_close,			/* entry point for close routine         */
  ftd_read,			/* entry point for read routine         */
  ftd_write,			/* entry point for write routine        */
  ftd_ioctl,			/* entry point for ioctl routine        */
  (ftd_int32_t (*)())ftd_strat,	/* entry point for strategy routine     */
  0,				/* pointer to tty device structure      */
  ftd_select,			/* entry point for select routine       */
  ftd_config,			/* entry point for config routine       */
  nodev,			/* entry point for print routine        */
  nodev,			/* entry point for dump routine         */
  nodev,			/* entry point for mpx routine          */
  nodev,			/* entry point for revoke routine       */
  0				/* pointer to device specific data      */
};

Simple_lock ftd_config_lock;
ftd_int32_t need_config_lock_init = 1;
ftd_int32_t config_lock_initialized = 0;

/*-
 * ftd_config()
 *
 * The ddconfig entry point is used to configure a device driver. It
 * can be called to do the following tasks:
 *
 *   o Initialize the device driver.
 *   o Terminate the device driver.
 *   o Request configuration data for the supported device.
 *   o Perform other device-specific configuration functions.
 *
 * The  ddconfig  routine  is  called  by  the  device's  Configure,
 * Unconfigure,  or  Change method. Typically, it is called once for
 * each device number (major and minor) to be  supported.  This  is,
 * however,   device-dependent.   The  specific  device  method  and
 * ddconfig routine determines the number of times it is called.
 *
 * The ddconfig routine can also provide additional  device-specific
 * functions  relating  to  configuration,  such as returning device
 * vital product  data  (VPD).   The  ddconfig  routine  is  usually
 * invoked  through  the sysconfig subroutine by the device-specific
 * Configure method.
 *
 * Device drivers and their methods typically support  these  values
 * for the cmd parameter:
 *
 * CFG_INIT Initializes the device driver and internal  data  areas.
 * This  typically  involves the minor number specified by the devno
 * parameter, for validity. The  device  driver's  ddconfig  routine
 * also  installs  the  device  driver's  entry points in the device
 * switch table,  if  this  was  the  first  time  called  (for  the
 * specified  major  number).  This can be accomplished by using the
 * devswadd kernel service along with a devsw structure to  add  the
 * device  driver's  entry points to the device switch table for the
 * major device number supplied in the devno parameter.
 *
 * The CFG_INIT command  parameter  should  also  copy  the  device-
 * dependent  information  (found  in the device-dependent structure
 * provided by the caller) into a static  or  dynamically  allocated
 * save  area  for  the specified device. This information should be
 * used when the ddopen routine is later called.
 *
 * The device-dependent structure's address and length are described
 * in  the  uio  structure  pointed  to  by  the uiop parameter. The
 * uiomove kernel service can be used to copy  the  device-dependent
 * structure into the device driver's data area.
 *
 * When the ddopen routine  is  called,  the  device  driver  passes
 * device-dependent  information  to  the  routines  or other device
 * drivers providing the device handler role in order to  initialize
 * the device. The delay in initializing the device until the ddopen
 * call is received is useful in order to delay the use of  valuable
 * system  resources  (such  as  DMA  channels and interrupt levels)
 * until the device is actually needed.
 *
 * CFG_TERM  Terminates  the  device  driver  associated  with   the
 * specified   device   number,   as   represented   by   the  devno
 * parameter.The  ddconfig  routine  determines  if  any  opens  are
 * outstanding  on  the  specified devno parameter. If none are, the
 * CFG_TERM command  processing  marks  the  device  as  terminated,
 * disallowing  any  subsequent opens to the device. All dynamically
 * allocated data areas associated with the specified device  number
 * should be freed.
 *
 * If this termination removes the last minor  number  supported  by
 * the device driver from use, the devswdel kernel service should be
 * called to remove the device driver's entry points from the device
 * switch table for the specified devno parameter.
 *
 * If opens are outstanding on the specified device,  the  terminate
 * operation  is  rejected  with an appropriate error code returned.
 * The Unconfigure method can subsequently unload the device  driver
 * if all uses of it have been terminated.
 *
 * To determine if all the uses  of  the  device  driver  have  been
 * terminated, a device method can make a sysconfig subroutine call.
 * By using the sysconfig SYS_QDVSW operation, the device method can
 * learn  whether  or  not the device driver has removed itself from
 * the device switch table.
 *
 * CFG_QVPD Queries device-specific vital product data (VPD).
 *
 * For this function, the calling routine sets up  a  uio  structure
 * pointed  at  by  the uiop parameter to the ddconfig routine. This
 * uio structure defines an area in the caller's  storage  in  which
 * the  ddconfig  routine  is  to  write the VPD. The uiomove kernel
 * service can be used to provide the data copy operation.
 *
 * The data area pointed at by the uiop parameter has two  different
 * purposes,  depending on the cmd function. If the CFG_INIT command
 * has been requested, the uiop structure describes the location and
 * length of the device-dependent data structure (DDS) from which to
 * read the information. If the CFG_QVPD command has been requested,
 * the  uiop  structure  describes  the area in which to write vital
 * product  data  information.  The  content  and  format  of   this
 * information  is  established  by  the  specific device methods in
 * conjunction with the device driver.
 *
 * The uiomove kernel service can  be  used  to  facilitate  copying
 * information  into or out of this data area. The format of the uio
 * structure is  defined  in  the  /usr/include/sys/uio.h  file  and
 * described further in the uio structure.
 */
ftd_int32_t
ftd_config (dev_t dev, ftd_int32_t cmd, struct uio *uiop)
{
  Enter (int, FTD_CONFIG, dev, cmd, uiop, 0, 0);
  ftd_int32_t err = 0;
  static ftd_int32_t load_count = 0;
  static ftd_int32_t devsw_added = 0;
  static ftd_int32_t pinned = 0;

  if (fetch_and_and (&need_config_lock_init, 0))
    {
      /* very first process */
      simple_lock_init (&ftd_config_lock);
      fetch_and_or (&config_lock_initialized, 1);
    }
  else
    {
      /* spin loop waiting for above to complete */
      while (!fetch_and_or (&config_lock_initialized, 0));
    }

  simple_lock (&ftd_config_lock);
  switch (cmd)
    {
    case CFG_INIT:
      if (err = devswadd (dev, &ftd_devsw))
	break;
      devsw_added = 1;
      if (err = pincode (ftd_config))
	break;
      pinned = 1;
		/*-
		 * configure from device dependent structure (DDS). this
		 * struct is passed by our configuration method. the
		 * convention will be that this is an initialized struct
		 * ftd_ctl buffer. so, our configuration method will have to
		 * determine the values of tunable parms when it is invoked,
		 * encode them in a properly init'ed struct ftd_ctl buffer,
		 * and pass them with this sysconfig() call... 
		 */
      err = ftd_config_drv (dev, uiop);
      break;

    case CFG_TERM:
      err = ftd_unconfig_drv (dev);
      break;

    default:
      err = EINVAL;
      break;
    }

  if (!err)
    if (cmd == CFG_INIT)
      ++load_count;
    else if (cmd == CFG_TERM)
      --load_count;

  if (load_count == 0)
    {
      if (devsw_added)
	{
	  devswdel (dev);
	  devsw_added = 0;
	}
      if (pinned)
	{
	  unpincode (ftd_config);
	  pinned = 0;
	}
    }
  simple_unlock (&ftd_config_lock);
  Return (err);
  Exit;
}

/*-
 * configure drv, ala Solaris vers attach routine. allocate/initialize global
 * state struct. decode ddconfig.sysconfig() call DDS parms. initial values
 * are those which are passed. 
 */
ftd_int32_t
ftd_config_drv (dev_t dev, struct uio *uiop)
{
  Enter (int, FTD_CONFIG_DRV, dev, uiop, 0, 0, 0);
  ftd_int32_t inst = 0;
  ftd_ctl_t *ctlp;

  ctlp = ftd_global_state =
    (ftd_ctl_t *) kmem_zalloc (sizeof (ftd_ctl_t), KM_NOSLEEP);

  if (ctlp == NULL)
    {
      FTD_ERR (FTD_WRNLVL, "%s-ctl%d: Can't allocate state info",
	       DRVNAME, inst);
      Return (-1);
    }
  if (ddi_soft_state_init (&ftd_dev_state, sizeof (ftd_dev_t), 16) != 0)
    {
      return -1;
    }
  if (ddi_soft_state_init (&ftd_lg_state, sizeof (ftd_lg_t), 16) != 0)
    {
      ddi_soft_state_fini (&ftd_dev_state);
      return -1;
    }
  if (uiomove ((caddr_t) ctlp, sizeof (ftd_ctl_t), UIO_WRITE, uiop) < 0)
    {
      FTD_ERR (FTD_WRNLVL, "%s-ctl%d: Can't configure state info",
	       DRVNAME, inst);
      Return (-1);
    }

#if defined(FTD_DEBUG) && 0
  FTD_ERR (FTD_WRNLVL, "ftd_config_drv: ftd_bab_init(0x%08x,0x%08x)",
	   ctlp->chunk_size, ctlp->num_chunks);
#endif /* defined(FTD_DEBUG) */

  /* init's... */

  /* ... asynch buffer ... */
  ctlp->bab_size = ftd_bab_init (ctlp->chunk_size, ctlp->num_chunks);
  if (ctlp->bab_size == 0)
    {
      FTD_ERR (FTD_WRNLVL, "%s-ctl%d: Can't init BAB", DRVNAME, inst);
      Return (-1);
    }


  /* ... private buffer pool ... */
  ctlp->ftd_buf_pool_headp = FTD_INIT_BUF_POOL ();
  if (ctlp->ftd_buf_pool_headp == (struct buf *) 0)
    {
      FTD_ERR (FTD_WRNLVL, "%s-ctl%d: Can't init buffer pool.", DRVNAME, inst);
      Return (-1);
    }

  /* ... timer request block pool. */
  if (ftd_timer_init () < 0)
    {
      FTD_ERR (FTD_WRNLVL, "%s-ctl%d: Can't init timer request block pool.",
	       DRVNAME, inst);
      Return (-1);
    }

  ALLOC_LOCK (ctlp->lock, "ctlp mutex");

  Return (0);
  Exit;

}

/* unconfigure drv, ala Solaris vers detach routine */
ftd_int32_t
ftd_unconfig_drv (dev_t dev)
{
  Enter (int, FTD_UNCONFIG_DRV, dev, 0, 0, 0, 0);
  ftd_int32_t inst = 0;
  ftd_uint32_t context;
  ftd_ctl_t *ctlp;

  if ((ctlp = ftd_global_state) == NULL)
    Return (-1);

  ACQUIRE_LOCK (ctlp->lock, context);

  if (ctlp->lg_open || ftd_dev_n_open (ctlp))
    {
      RELEASE_LOCK (ctlp->lock, context);
      Return (-1);
    }
  while (ctlp->lghead)
    ftd_del_lg (ctlp->lghead,
		getminor (ctlp->lghead->dev) & ~FTD_LGFLAG);

  RELEASE_LOCK (ctlp->lock, context);

  DEALLOC_LOCK (ctlp->lock);

#if defined(FTD_DEBUG)
  FTD_ERR (FTD_WRNLVL, "ftd_unconfig_drv: ftd_bab_fini(ftd_void_t)");
#endif /* defined(FTD_DEBUG) */

  /* fini's ... */

  /* ... asynch buffer ... */
  ftd_bab_fini ();


  /* ... private buffer pool ... */
  FTD_FINI_BUF_POOL ();

  /* ... timer request block pool. */
  if (ftd_timer_fini () < 0)
    {
      FTD_ERR (FTD_WRNLVL, "%s-ctl%d: Can't finish timer request block pool.",
	       DRVNAME, inst);
      Return (-1);
    }

  ddi_soft_state_fini (&ftd_dev_state);
  ddi_soft_state_fini (&ftd_lg_state);

  kmem_free (ftd_global_state, sizeof (ftd_ctl_t));

  Return (0);
  Exit;
}


/* AIX ftd unix drv entry points */

/*-
 * ftd_open()
 *
 * The kernel calls the ddopen routine of a  device  driver  when  a
 * program  issues  an open or creat subroutine call. It can also be
 * called when a system call, kernel process, or other device driver
 * uses the fp_opendev or fp_open kernel service to use the device.
 *
 * The ddopen routine must first  ensure  exclusive  access  to  the
 * device,  if  necessary.  Many character devices, such as printers
 * and plotters, should be opened by only one process at a time. The
 * ddopen  routine  can  enforce  this  by maintaining a static flag
 * variable, which is set to 1 if the device is open and 0 if not.
 *
 * Each time the ddopen routine is called, it checks  the  value  of
 * the  flag.  If  the  value  is  other  than 0, the ddopen routine
 * returns with a return code of EBUSY to indicate that  the  device
 * is  already open. Otherwise, the ddopen routine sets the flag and
 * returns normally. The ddclose entry point later clears  the  flag
 * when the device is closed.
 *
 * Since most block devices can be  used  by  several  processes  at
 * once,  a  block  driver  should  not  try to enforce opening by a
 * single user.
 *
 * Flags Defined for the devflag Parameter
 *
 * The devflag parameter has the following flags, as defined in  the
 * /usr/include/sys/device.h file:
 *
 * DKERNEL Entry point called by kernel routine using the fp_opendev
 * or fp_open kernel service.
 *
 * DREAD Open for reading.
 *
 * DWRITE Open for writing.
 *
 * DAPPEND Open for appending.
 *
 * DNDELAY Device open in nonblocking mode.
 */
ftd_int32_t
ftd_open (dev_t dev, ftd_uint32_t devflag, chan_t chan, ftd_int32_t ext)
{
  Enter (int, FTD_OPEN, dev, devflag, chan, ext, 0);
  ftd_int32_t minor = getminor (dev);

  if (minor == FTD_CTL)
    {
      Return (ftd_ctl_open (dev, devflag));
    }
  else if (minor & FTD_LGFLAG)
    {
      Return (ftd_lg_open (dev, devflag));
    }
  else
    {
      Return (ftd_dev_open (dev, devflag));
    }
  Exit;
}

/*-
 * ftd_close()
 *
 * The ddclose entry point is called when a previously opened device
 * instance  is  closed  by  the close subroutine or fp_close kernel
 * service.  The  kernel   calls   the   routine   under   different
 * circumstances for non-multiplexed and multiplexed device drivers.
 *
 * For non-multiplexed device drivers, the kernel calls the  ddclose
 * routine  when  the  last  process having the device instance open
 * closes  it.  This  causes  the  g-node  reference  count  to   be
 * decremented to 0 and the g-node to be deallocated.
 *
 * For multiplexed device drivers, the ddclose routine is called for
 * each  close associated with an explicit open. In other words, the
 * device driver's ddclose routine is invoked once for each time its
 * ddopen routine was invoked for the channel.
 *
 * In some instances, data buffers should be written to  the  device
 * before  returning  from  the  ddclose  routine. These are buffers
 * containing data to be written to the device that have been queued
 * by the device driver but not yet written.
 *
 * Non-multiplexed device drivers should reset the associated device
 * to  an  idle  state  and change the device driver device state to
 * closed. This can involve calling the fp_close kernel  service  to
 * issue  a  close  to  an  associated  open  device handler for the
 * device. Returning the device to an idle state prevents the device
 * from  generating any more interrupt or direct memory access (DMA)
 * requests. DMA channels and interrupt levels  allocated  for  this
 * device should be freed, until the device is re-opened, to release
 * critical system resources that this device uses.
 *
 * Multiplexed  device  drivers  should  provide  the  same   device
 * quiescing,  but  not in the ddclose routine. Returning the device
 * to the idle state and freeing its  resources  should  be  delayed
 * until  the ddmpx routine is called to deallocate the last channel
 * allocated on the device.
 *
 * In all cases, the device instance is considered closed  once  the
 * ddclose  routine  has  returned  to the caller, even if a nonzero
 * return code is returned.
 */
ftd_int32_t
ftd_close (dev_t dev, chan_t chan)
{
  Enter (int, FTD_CLOSE, dev, chan, 0, 0, 0);
  ftd_int32_t minor = getminor (dev);

  if (minor == FTD_CTL)
    Return (ftd_ctl_close (dev, 0));
  else if (minor & FTD_LGFLAG)
    Return (ftd_lg_close (dev, 0));
  else
    Return (ftd_dev_close (dev, 0));
  Exit;

}

/*-
 * ftd_ioctl()
 *
 * When a program issues an ioctl or  ioctlx  subroutine  call,  the
 * kernel  calls the ddioctl routine of the specified device driver.
 * The  ddioctl  routine  is  responsible  for  performing  whatever
 * functions  are  requested.  In  addition, it must return whatever
 * control information has been specified by the original caller  of
 * the  ioctl subroutine. The cmd parameter contains the name of the
 * operation to be performed.
 *
 * Most ioctl operations depend on  the  specific  device  involved.
 * However,  all  ioctl  routines  must  respond  to  the  following
 * command:
 *
 * IOCINFO   Returns   a   devinfo   structure   (defined   in   the
 * /usr/include/sys/devinfo.h   file)  that  describes  the  device.
 * (Refer to the description of the special file  for  a  particular
 * device  in the Application Programming Interface.) Only the first
 * two fields of the data structure  need  to  be  returned  if  the
 * remaining fields of the structure do not apply to the device.
 *
 * The  devflag  parameter  indicates  one  of  several   types   of
 * information.  It  can  give  conditions  in  which the device was
 * opened. (These conditions can  subsequently  be  changed  by  the
 * fcntl  subroutine  call.) Alternatively, it can tell which of two
 * ways the entry point was invoked:
 *
 *   o By the file system on behalf of a using application
 *   o Directly by  a  kernel  routine  using  the  fp_ioctl  kernel
 * service
 *
 * Thus  flags  in  the  devflag  parameter   have   the   following
 * definitions, as defined in the /usr/include/sys/device.h file:
 *
 * DKERNEL Entry point called by kernel routine using  the  fp_ioctl
 * service.
 *
 * DREAD Open for reading.
 *
 * DWRITE Open for writing.
 *
 * DAPPEND Open for appending.
 *
 * DNDELAY Device open in nonblocking mode.
 */
ftd_int32_t
ftd_ioctl (dev_t dev, ftd_int32_t cmd, ftd_int32_t arg, ftd_uint32_t devflag, chan_t chan, ftd_int32_t ext)
{
  Enter (int, FTD_IOCTL, dev, cmd, arg, devflag, ext);
  ftd_int32_t err = 0;
  ftd_int32_t flag = 0;
  ftd_int32_t len;
  ftd_int32_t passarg;
  ftd_char_t buffer[256];
  ftd_int32_t minor = getminor (dev);


  passarg = arg;
  len = (cmd >> 16) & IOCPARM_MASK;
  if (cmd & IOC_IN)
    {
      err = ddi_copyin ((caddr_t) arg, (caddr_t) buffer, len, flag);
      if (err)
	err = EFAULT;
      passarg = (ftd_int32_t) buffer;
    }
  if (err)
    Return (err);

#if defined(FTD_DEBUG) && 0
  FTD_ERR (FTD_WRNLVL,
	   "ftd_ioctl: minor: 0x%08x cmd: 0x%08x arg: 0x%08x len: 0x%08x\n",
	   minor, cmd, arg, len);
#endif /* defined(FTD_DEBUG) && 0 */

  if (minor == FTD_CTL)
    err = ftd_ctl_ioctl (dev, cmd, passarg, devflag);
  else if (minor & FTD_LGFLAG)
    err = ftd_lg_ioctl (dev, cmd, passarg, devflag);
  else
    err = ftd_dev_ioctl (dev, cmd, passarg, devflag);

  if ((err == 0) && (cmd & IOC_OUT) && (((cmd >> 8) & 0xff) == FTD_CMD))
    {
      err = ddi_copyout ((caddr_t) buffer, (caddr_t) arg, len, flag);
      if (err)
	err = EFAULT;
    }
  Return (err);
  Exit;

}

/*-
 * ftd_read()
 *
 * When a program issues a read or readx subroutine call or when the
 * fp_rwuio  kernel  service  is  used,  the kernel calls the ddread
 * entry point.
 *
 * This entry point receives a  pointer  to  a  uio  structure  that
 * provides variables used to specify the data transfer operation.
 *
 * Character device drivers can use the ureadc  and  uiomove  kernel
 * services  to  transfer  data into and out of the user buffer area
 * during a read subroutine call. These services receive  a  pointer
 * to  the  uio  structure and update the fields in the structure by
 * the number of bytes transferred.  The  only  fields  in  the  uio
 * structure  that  cannot  be modified by the data transfer are the
 * uio_fmode and uio_segflg fields.
 *
 * For most devices, the ddread routine sends  the  request  to  the
 * device  handler  and then waits for it to finish. The waiting can
 * be accomplished by  calling  the  e_sleep  kernel  service.  This
 * service  suspends  the  driver and the process that called it and
 * permits other processes to run until a specified event occurs.
 *
 * When the I/O operation completes, the device  usually  issues  an
 * interrupt,  causing  the  device driver's interrupt handler to be
 * called. The interrupt handler  then  calls  the  e_wakeup  kernel
 * service  specifying  the  awaited event, thus allowing the ddread
 * routine to resume.
 *
 * The uio_resid field initially contains the total number of  bytes
 * to  read  from  the device. If the device driver supports it, the
 * uio_offset field indicates the byte offset  on  the  device  from
 * which the read should start.
 *
 * The uio_offset field is a 64 bit integer (offset_t) ; this allows
 * the  file system to send I/O requests to a device driver's read &
 * write entry points which have logical offsets beyond 2 gigabytes.
 * Device  drivers must use care not to cause a loss of significance
 * by assigning the offset to a 32  bit  variable  or  using  it  in
 * calculations that overflow a 32 bit variable.
 *
 * If no error occurs, the uio_resid field should  be  0  on  return
 * from the ddread routine to indicate that all requested bytes were
 * read. If an error occurs, this field should contain the number of
 * bytes remaining to be read when the error occurred.
 *
 * If a read request starts at a valid  device  offset  but  extends
 * past  the  end  of  the device's capabilities, no error should be
 * returned. However, the uio_resid field should indicate the number
 * of  bytes  not  transferred. If the read starts at the end of the
 * device's capabilities, no error should be returned. However,  the
 * uio_resid  field should not be modified, indicating that no bytes
 * were transferred. If the read starts past the end of the device's
 * capabilities,  an  ENXIO  return code should be returned, without
 * modifying the uio_resid field.
 *
 * When the ddread entry point is provided for raw I/O  to  a  block
 * device,  this  routine usually translates requests into block I/O
 * requests using the uphysio kernel service.
 */
ftd_int32_t
ftd_read (dev_t dev, struct uio *uiop, chan_t chan, ftd_int32_t ext)
{
  Enter (int, FTD_READ, dev, uiop, chan, ext, 0);
  Return (ftd_rw (dev, uiop, B_READ));
  Exit;
}

/*-
 * ftd_write()
 *
 * When a program issues a write or writex subroutine call  or  when
 * the fp_rwuio kernel service is used, the kernel calls the ddwrite
 * entry point.
 *
 * This entry point receives a pointer to  a  uio  structure,  which
 * provides variables used to specify the data transfer operation.
 *
 * Character device drivers can use the uwritec and  uiomove  kernel
 * services  to  transfer  data into and out of the user buffer area
 * during a write subroutine  call.  These  services  are  passed  a
 * pointer  to  the  uio  structure.  They  update the fields in the
 * structure by the number of bytes transferred. The only fields  in
 * the  uio  structure that are not potentially modified by the data
 * transfer are the uio_fmode and uio_segflg fields.
 *
 * For most devices, the ddwrite routine queues the request  to  the
 * device  handler  and  then waits for it to finish. The waiting is
 * typically accomplished by calling the e_sleep kernel  service  to
 * wait for an event. The e_sleep kernel service suspends the driver
 * and the process that called it and  permits  other  processes  to
 * run.
 *
 * When the I/O operation is completed, the device usually causes an
 * interrupt,  causing  the  device driver's interrupt handler to be
 * called. The interrupt handler  then  calls  the  e_wakeup  kernel
 * service  specifying  the awaited event, thus allowing the ddwrite
 * routine to resume.
 *
 * The uio_resid field initially contains the total number of  bytes
 * to  write  to  the  device. If the device driver supports it, the
 * uio_offset field indicates the byte offset  on  the  device  from
 * where the write should start.
 *
 * The uio_offset field is a 64 bit integer (offset_t) ; this allows
 * the  file system to send I/O requests to a device driver's read &
 * write entry points which have logical offsets beyond 2 gigabytes.
 * Device  drivers must use care not to cause a loss of significance
 * by assigning the offset to a 32  bit  variable  or  using  it  in
 * calculations that overflow a 32 bit variable.
 *
 * If no error occurs, the uio_resid field should  be  0  on  return
 * from  the  ddwrite  routine  to indicate that all requested bytes
 * were written. If an error occurs, this field should  contain  the
 * number of bytes remaining to be written when the error occurred.
 *
 * If a write request starts at a valid device  offset  but  extends
 * past  the  end  of  the device's capabilities, no error should be
 * returned. However, the uio_resid field should indicate the number
 * of  bytes not transferred. If the write starts at or past the end
 * of the device's capabilities, no data should be transferred.   An
 * error  code  of ENXIO should be returned, and the uio_resid field
 * should not be modified.
 *
 * When the ddwrite entry point is provided for raw I/O to  a  block
 * device,  this  routine usually uses the uphysio kernel service to
 * translate requests into block I/O requests.
 */
ftd_int32_t
ftd_write (dev_t dev, struct uio *uiop, chan_t chan, ftd_int32_t ext)
{
  Enter (int, FTD_WRITE, dev, uiop, chan, ext, 0);
  Return (ftd_rw (dev, uiop, B_WRITE));
  Exit;
}

/*-
 *
 * ftd_strat()
 *
 * When the  kernel  needs  a  block  I/O  transfer,  it  calls  the
 * ddstrategy strategy routine of the device driver for that device.
 * The strategy routine  schedules  the  I/O  to  the  device.  This
 * typically requires the following actions:
 *
 *   o The request or requests must be added  on  the  list  of  I/O
 * requests that need
 *     to be processed by the device.
 *   o If the request list was empty before the preceding additions,
 * the device's
 *     start I/O routine must be called.
 *
 * Required Processing
 *
 * The ddstrategy routine can receive a single request with multiple
 * buf  structures.  However, it is not required to process requests
 * in any specific order.
 *
 * The strategy routine can  be  passed  a  list  of  operations  to
 * perform.  The  av_forw  field  in  the  buf header describes this
 * null-terminated list of buf headers.  This  list  is  not  doubly
 * linked: the av_back field is undefined.
 *
 * Block device drivers must  be  able  to  perform  multiple  block
 * transfers.  If  the device cannot do multiple block transfers, or
 * can only do multiple block transfers  under  certain  conditions,
 * then  the device driver must transfer the data with more than one
 * device operation.
 *
 * Kernel Buffers and Using the buf Structure
 *
 * An area of memory is set aside within the kernel memory space for
 * buffering  data  transfers  between  a program and the peripheral
 * device. Each kernel buffer has a header, the buf structure, which
 * contains  all  necessary  information  for  performing  the  data
 * transfer. The ddstrategy  routine  is  responsible  for  updating
 * fields in this header as part of the transfer.
 *
 * The caller of the strategy routine should set the b_iodone  field
 * to  point to the caller's I/O done routine. When an I/O operation
 * is complete, the device driver calls the iodone  kernel  service,
 * which  then  calls the I/O done routine specified in the b_iodone
 * field. The  iodone  kernel  service  makes  this  call  from  the
 * INTIODONE interrupt level.
 *
 * The value of the b_flags field is constructed by logically  ORing
 * zero or more possible b_flags field flag values.
 *
 * 	Attention: Do not modify any of the following fields  of  the
 * 	buf structure passed to the ddstrategy entry point: the b_forw ,
 * 	b_back  , b_dev , b_un , or  b_blkno  field.  Modifying   these
 * 	fields   can   cause unpredictable and disastrous results.
 *
 *     	Attention: Do not modify any of the following fields of a buf
 * 	structure acquired with the geteblk service: the  b_flags  ,
 * 	b_forw  , b_back , b_dev , b_count , or b_un field. Modifying
 * 	any of  these  fields  can cause unpredictable and disastrous
 * 	results.
 */
ftd_void_t
ftd_strat (struct buf *bp)
{
  Enterv (FTD_STRAT, bp->b_dev, bp, 0, 0, 0);
  ftd_uint32_t context;
  struct buf *cbp, *nbp, *fbp;
  minor_t minor;
  ftd_dev_t *softp;

  minor = getminor (BP_DEV (bp));
  if ((softp = ddi_get_soft_state (ftd_dev_state, U_2_S32(minor))) == NULL)
    {
      bioerror (bp, ENXIO);
      biodone (bp);
      Returnv;
    }

	/*-
	 * _AIX ddstrategy N.B.:
	 * bp might be the head of a chain of buf hdrs,
	 * ftd_strategy() expects one at a time,
	 * so feed them to it cautiously.
	 */
  cbp = bp;
  while (cbp)
    {
		/*-
		 * have to be cautious here, need to
		 * save off av_forw before calling
		 * strategy, otherwise we'll wind
		 * up walking the freelist in a
		 * race with intr I/O completion.
		 */
      nbp = cbp->av_forw;
      ftd_strategy (cbp);
      cbp = nbp;
    }

  Returnv;
  Exitv;
}

/*-
 * ftd_select()
 *
 * The ddselect entry point  is  called  when  the  select  or  poll
 * subroutine  is  used,  or  when  the  fp_select kernel service is
 * invoked. It determines whether a specified event or  events  have
 * occurred on the device.
 *
 * Only character class device  drivers  can  provide  the  ddselect
 * routine.  It cannot be provided by block device drivers even when
 * providing raw read/write access.
 *
 * Requests for Information on Events
 *
 * The events parameter represents possible events to check as flags
 * (bits).  There  are three basic events defined for the select and
 * poll subroutines, when applied to devices  supporting  select  or
 * poll operations:
 *
 * POLLIN Input is present on the device.
 *
 * POLLOUT The device is capable of output.
 *
 * POLLPRI An exceptional condition has occurred on the device.
 *
 * A fourth event flag is used  to  indicate  whether  the  ddselect
 * routine  should record this request for later notification of the
 * event using the selnotify kernel service. This flag can be set in
 * the  events  parameter  if  the  device driver is not required to
 * provide asynchronous notification of the requested events:
 *
 * POLLSYNC This request is a synchronous request only. The  routine
 * need  not call the selnotify kernel service for this request even
 * if the events later occur.
 *
 * Additional event flags in  the  events  parameter  are  left  for
 * device-specific events on the poll subroutine call.
 *
 * Select Processing
 *
 * If one or more events specified in the events parameter are true,
 * the   ddselect  routine  should  indicate  this  by  setting  the
 * corresponding bits  in  the  reventp  parameter.  Note  that  the
 * reventp returned events parameter is passed by reference.
 *
 * If none of the requested  events  are  true,  then  the  ddselect
 * routine  sets the returned events parameter to 0. It is passed by
 * reference through the  reventp  parameter.  It  also  checks  the
 * POLLSYNC  flag in the events parameter. If this flag is true, the
 * ddselect routine should just return, since the event request  was
 * a synchronous request only.
 *
 * However, if the POLLSYNC flag is false, the ddselect routine must
 * notify  the kernel when one or more of the specified events later
 * happen.  For  this  purpose,  the  routine  should  set  separate
 * internal flags for each event requested in the events parameter.
 *
 * When any of these events become true, the device  driver  routine
 * should  use  the  selnotify  service  to  notify  the kernel. The
 * corresponding internal flags should then be reset to prevent  re-
 * notification of the event.
 *
 * Sometimes the device can be in a state in which a supported event
 * or  events  can  never be satisfied (such as when a communication
 * line is not operational). In  this  case,  the  ddselect  routine
 * should  simply  set  the  corresponding  reventp flags to 1. This
 * prevents the select or poll subroutine from waiting indefinitely.
 * As  a result however, the caller will not in this case be able to
 * distinguish between satisfied events and unsatisfiable ones. Only
 * when  a  later request with an NDELAY option fails will the error
 * be detected.
 *
 *     Note: Other device  driver  routines  (such  as  the  ddread,
 * ddwrite routines)
 *     may require logic to support select or poll operations.
 */
ftd_int32_t
ftd_select (dev_t dev, ftd_uint16_t events, ftd_uint16_t * reventp, ftd_int32_t chan)
{
  Enter (int, FTD_SELECT, dev, events, reventp, chan, 0);
  ftd_lg_t *lgp;
  minor_t minor = getminor (dev);

  /* data or ctl devs ? */
  if (minor == FTD_CTL || minor & FTD_LGFLAG == 0)
    Return (EINVAL);

  /* valid dev ? */
  minor = minor & ~FTD_LGFLAG;
  if ((lgp = (ftd_lg_t *) ddi_get_soft_state (ftd_lg_state, U_2_S32(minor))) == NULL)
    Return (ENXIO);

  /* io possible? */
  if ((lgp->state & FTD_M_JNLUPDATE) == 0 || lgp->wlentries)
    *reventp = (POLLIN | POLLOUT) & events;
  else
    *reventp = 0;

  /* synch poll? */
  if (events & POLLSYNC)
    Return (0);

  /* asynch poll */
  lgp->ev_id = (ftd_int32_t) dev;
  lgp->ev_subid = 0;
  lgp->ev_rtnevents = (POLLIN | POLLOUT);

  Return (0);
  Exit;

}

/*-
 * ftd_wakeup()
 *
 * The selnotify kernel service should be  used  by  device  drivers
 * that  support  select  or poll operations. It is also used by the
 * kernel to support select  or  poll  requests  to  sockets,  named
 * pipes, and message queues.
 *
 * The selnotify kernel service wakes  up  processes  waiting  on  a
 * select or poll subroutine. The processes to be awakened are those
 * specifying the given device and one or more of  the  events  that
 * have  occurred  on  the  specified  device.  The  select and poll
 * subroutines allow a process to request information about  one  or
 * more  events  on  a  particular  device. If none of the requested
 * events have yet happened, the process is put  to  sleep  and  re-
 * awakened later when the events actually happen.
 *
 * The selnotify service should be called whenever a  previous  call
 * to  the  device driver's ddselect entry point returns and both of
 * the following conditions apply:
 *
 *   o The status of all requested events is false.
 *   o Asynchronous notification of the events is requested.
 *
 * The  selnotify  service  can  be  called  for  other  than  these
 * conditions but performs no operation.
 *
 * Sequence of Events for Asynchronous Notification
 *
 * The  device  driver  must  store  information  about  the  events
 * requested  while  in  the  driver's  ddselect  routine  under the
 * following conditions:
 *
 *   o None of the requested events are true (at  the  time  of  the
 * call).
 *   o The POLLSYNC flag is not set in the events parameter.
 *
 * The POLLSYNC flag, when  not  set,  indicates  that  asynchronous
 * notification  is  desired.  In  this  case, the selnotify service
 * should be called when one or more of the requested  events  later
 * becomes true for that device and channel.
 *
 * When the device  driver  finds  that  it  can  satisfy  a  select
 * request,  (perhaps  due  to  new  input  data) and an unsatisfied
 * request for that event is still pending, the selnotify service is
 * called with the following items:
 *
 *   o Device major and minor number specified by the id parameter
 *   o Channel number specified by the subid parameter
 *   o Occurred events specified by the rtnevents parameter
 *
 * These parameters  describe  the  device  instance  and  requested
 * events  that  have  occurred on that device. The notifying device
 * driver then resets its requested-events flags for the events that
 * have  occurred  for that device and channel. The reset flags thus
 * indicate that those events are no longer requested.
 *
 * If the rtnevents parameter indicated by the call to the selnotify
 * service is no longer being waited on, no processes are awakened.
 */
ftd_void_t
ftd_wakeup (ftd_lg_t * lgp)
{
  Enterv (FTD_WAKEUP, (ftd_uint32_t) lgp->ev_id, 0, 0, 0, 0);
  selnotify (lgp->ev_id, lgp->ev_subid, lgp->ev_rtnevents);
  Returnv;
  Exitv;
}

/* AIX ftd local */

struct buf *
ftd_bioclone (ftd_dev_t * softp, struct buf *sbp, struct buf *dbp)
{
  Enter (struct buf *, FTD_BIOCLONE, softp->localbdisk, sbp, dbp, 0, 0);
  ftd_uint32_t context;
  ftd_int32_t b_flags_sv;

  /* allocate pool struct buf */
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
  /* clone structs */
  bcopy (sbp, dbp, sizeof (struct buf));
  b_flags_sv = sbp->b_flags;

  /* zero out any list pointers */
  dbp->b_forw =
    dbp->b_back =
    dbp->av_forw =
    dbp->av_back = (struct buf *) 0;
  dbp->b_event = 0xffffffff;

  /* clone buffer flags */
  dbp->b_flags = 0;
  dbp->b_flags |= B_BUSY;

  /* rd/wr flag */
  dbp->b_flags |= (b_flags_sv & B_READ) ? B_READ : 0;

  /* clone buffer references cloned buffer */
  BP_USER (dbp) = BP_PCAST sbp;

  Return (dbp);
  Exit;

}

ftd_dev_t *
ftd_ld2soft (dev_t dev)
{
  ftd_lg_t *lgwalk;
  ftd_dev_t *devwalk;
  static dev_t lastdev = 0;
  static ftd_dev_t *lastsoftp = 0;

  if (lastdev == dev)
    return lastsoftp;
  for (lgwalk = ftd_global_state->lghead; lgwalk; lgwalk = lgwalk->next)
    {
      for (devwalk = lgwalk->devhead; devwalk; devwalk = devwalk->next)
	{
	  if (devwalk->localbdisk == dev)
	    {
	      lastdev = dev;
	      lastsoftp = devwalk;
	      return devwalk;
	    }
	}
    }
  return 0;
}

ftd_dev_t *
ftd_get_softp (struct buf * bp)
{
  ftd_dev_t *softp;
  struct buf *userbp;

  softp = ftd_ld2soft (BP_DEV (bp));
  if (softp == NULL)
    {
      FTD_ERR (FTD_WRNLVL,
	       "Got a bad softp! bp %x dev %d\n", bp, BP_DEV (bp));
      bioerror (bp, ENXIO);
      biodone (bp);
      return NULL;
    }
  return softp;
}

ftd_int32_t
ftd_ctl_get_device_nums (dev_t dev, ftd_int32_t arg, ftd_int32_t flag)
{
  ftd_devnum_t *devnum;

  devnum = (ftd_devnum_t *) arg;
  devnum->c_major = major (dev);
  devnum->b_major = devnum->c_major;

  return 0;
}

ftd_int32_t
ftd_dev_ioctl (dev_t dev, ftd_int32_t cmd, ftd_int32_t arg, ftd_int32_t flag)
{
  ftd_dev_t *softp;
  ftd_int32_t minor = getminor (dev);

  softp = (ftd_dev_t *) ddi_get_soft_state (ftd_dev_state, U_2_S32(minor));
  if (softp == NULL)
    if (softp == NULL)
      return (ENXIO);

  return fp_ioctl (softp->dev_fp, cmd, arg, flag);
}

#endif /* defined(_AIX) */
