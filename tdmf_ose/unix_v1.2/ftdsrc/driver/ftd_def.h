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
/*
 * Copyright (c) 1996, 1998 by FullTime Software, Inc.  All Rights Reserved.
 *
 * Defines and structures used only within the driver.
 *
 * $Id: ftd_def.h,v 1.59 2018/01/30 22:37:34 paulclou Exp $
 */

#ifndef _FTD_DEF_H
#define _FTD_DEF_H

#include "ftd_kern_ctypes.h"

#if defined(_HPUX_SOURCE)
    #if (SYSVERS >= 1123)
        #include <sys/buf.h>
        #include <sys/param.h>
        #include <sys/conf.h>
        #include <sys/errno.h>
        #include <sys/specfs.h> /* opend()/closed() */
    #else
        #include "../h/buf.h"
        #include "../h/param.h"
        #include "../h/tuneable.h"
        #include "../h/conf.h"
        #include "../h/errno.h"
        // 11.11 Doesn't provide declarations for these.
        extern int closed (int, unsigned int, unsigned int);
        extern int opend (int *, unsigned int, unsigned int, int *);
#endif

#if defined(HPUX) && (SYSVERS >= 1100)
#include <sys/kthread_iface.h>
#endif

#define maxphys     MAXPHYS

typedef int minor_t; /* As seen in the casted return value of the minor() macro. */
typedef int major_t; /* As seen in the casted return value of the major() macro. */
typedef lock_t* ftd_lock_t;

#ifndef getminor
// Turns out that /usr/include/sys/stream.h may already have defined this.
#define getminor(dev) minor(dev)
#endif

#ifndef getmajor
// Turns out that /usr/include/sys/stream.h may already have defined this.
#define getmajor(dev) major(dev)
#endif

#elif defined(SOLARIS)

#include <sys/types.h>
#include <sys/dditypes.h>
#include <sys/buf.h>
#include <sys/poll.h>
#include <sys/errno.h>
#include <sys/mkdev.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#define NODEV32 (dev32_t)-1
#if (OSVER < 570)
/* mask warnings about splhigh(), splclock proto mismatch */
#undef splhigh
#undef splclock
#endif /* (OSVER >= 570) */
#include <sys/conf.h>

typedef kmutex_t ftd_lock_t;

#elif defined(_AIX)
/* AIX specific header files */
#include <sys/syslog.h>
#include <sys/types.h>
#define _KERNSYS
#include <sys/buf.h>
#undef _KERNSYS
#include <sys/poll.h>
#include <sys/errno.h>
#include <sys/sleep.h>
#include <sys/lock_def.h>
#include <sys/lock_alloc.h>
#include <sys/intr.h>
#include <sys/dir.h>
#include <sys/syspest.h>
#include <sys/sysmacros.h>

/* DEV_BSHIFT is set for 512 bytes */
#define DEV_BSHIFT 9
typedef int minor_t; /* As seen in the casted return value of the minor_num()/minor()/minor64() macros. */
typedef int major_t; /* As seen in the casted return value of the major_num()/major()/major64() macros. */
typedef Simple_lock ftd_lock_t;

#define minphys ftd_minphys
#define physio(strat, buf, dev, flag, min, uio) uphysio(uio, flag, BUF_CNT, dev, strat, min, MIN_PAR)
extern ftd_int32_t ftd_minphys (struct buf *bp, ftd_void_t * arg);
#define biodone(bp) iodone(bp)
#define biowait(bp) iowait(bp)
#define getminor(dev) minor_num(dev)
#define getmajor(dev) major_num(dev)
#define bdev_strategy(bp) devstrat(bp)

/* AIX defines nothing equivalent for these: */

#define KM_NOSLEEP 1
#define KM_SLEEP 2

#define MAXLVS 512

/* 
 * uphysio() usage
 * BUF_CNT specifies the maximum number of buf structs to use
 * when calling the strategy routine specified by the strat
 * parameter. This parameter is used to indicate the maximum
 * amount of concurrency the device can support and minimize 
 * the I/O redrive time. The value of BUF_CNT can range from
 * 1 to 64.
 * MIN_PAR points to parameters to be used by the mincnt 
 * parameter.
 * AIX port minphys(), ftd_minphys is currently a stub.
 */
#define BUF_CNT 1
#define MIN_PAR 0

/* SunOS/SOLARIS kernel extern. XXX: needs defined */
ftd_int32_t maxphys;

#elif defined(linux)
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/param.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/blkdev.h>
#include "ftd_linux.h"
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
#define LINUX240
#else
#define LINUX260
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0) */

typedef spinlock_t ftd_lock_t;
#endif

#if defined(linux)
/**
 * Under linux, the atomic bit operation functions are used, so it is important to use the proper type for these functions.
 * Within kernels 2.6.9 and 2.6.23.1, i386 and s390 both expect an unsigned long*, while x86_64 expects a void*.
 *
 * The exact type needed is platform specific, so we might have to adjust this if we ever support linux on other architectures.
 */
typedef unsigned long atomic_bitops_t;
#else
/**
 * All the other OSes test and modify the value while holding a lock.
 * I've left the type as it was before just as a precaution, but changing it to be unsigned long as well would
 * probably not cause any harm.
 */
typedef ftd_int32_t atomic_bitops_t;
#endif /* linux */

#define DRVVERSION      PRODUCTNAME " " VERSION
#define DRIVERNAME      QNM

#ifdef NO_PROTOTYPES
#define _ANSI_ARGS(x) ()
#else
#define _ANSI_ARGS(x) x
#endif

#include "ftd_bab.h"
#include "ftd_buf.h"
#include "ftdio.h"
#include "ftd_klog.h"

/*
 * Local definitions, for clarity of code
 */

#define FTD_LRDB_BLKSIZE 1024       /* Must match 1024 block size in ftd_start.c */

#define FTD_MASK(n)           (0x1 << FTD_BITX_##n)

/* a bitmap descriptor */
typedef struct ftd_bitmap
  {				/* SIZEOF == 20 */
    ftd_int32_t bitsize;	/* a bit represents this many bytes */
    ftd_int32_t shift;		/* converts sectors to bit values   */
    ftd_int32_t len32;		/* length of "map" in 32-bit words  */
    ftd_int32_t numbits;	/* number of valid bits */
    ftd_uint32_t *map;
    ftd_uint32_t *mapnext;  /* historical bitmap */
  }
ftd_bitmap_t;

#define FTD_DEFAULT_LG          (8)
#define FTD_DEFAULT_DEVS        (32)
#define FTD_DEFAULT_MEM         (24)

typedef struct ftd_ctl
  {
    ftd_int32_t chunk_size;	/* 00 */
    ftd_int32_t num_chunks;	/* 04 */
    caddr_t bab;		/* 08 */
    ftd_int32_t bab_size;	/* 0c */
#if defined(SOLARIS)
    dev_info_t *dev_info;	/* 10 */
#endif
    ftd_int32_t dev_open;
    ftd_lock_t lock;
    struct ftd_lg *lghead;
    struct buf *ftd_buf_pool_headp;
    ftd_int32_t minor_index;
    ftd_int32_t hrdb_type;
  }
ftd_ctl_t;

/*
 * bit-wise flags in lg object
 */

//#define FTD_BITX_LG_OPEN	   0 /* Obsolete */
#define FTD_BITX_LG_STOP       1
#define FTD_BITX_LG_JLESS      2
#define FTD_BITX_LG_CHECKPOINT 3

//#define FTD_LG_OPEN	        FTD_MASK(LG_OPEN) /* Obsolete */
#define FTD_LG_STOP         FTD_MASK(LG_STOP)
#define FTD_LG_JLESS        FTD_MASK(LG_JLESS)
#define FTD_LG_CHECKPOINT   FTD_MASK(LG_CHECKPOINT)

#define GET_LG_JLESS(lgp)   (((ftd_lg_t*)(lgp))->flags & FTD_LG_JLESS)
#define SET_LG_JLESS(lgp, x) \
    ((x == 0) ? (((ftd_lg_t*)(lgp))->flags &= !FTD_LG_JLESS) : \
                (((ftd_lg_t*)(lgp))->flags |= FTD_LG_JLESS))

#define GET_LG_CHECKPOINT(lgp)  (((ftd_lg_t*)(lgp))->flags & FTD_LG_CHECKPOINT)
#define SET_LG_CHECKPOINT(lgp, x) \
    ((x == 0) ? (((ftd_lg_t*)(lgp))->flags &= !FTD_LG_CHECKPOINT) : \
                (((ftd_lg_t*)(lgp))->flags |= FTD_LG_CHECKPOINT))

#define FTD_DIRTY_LAG	1024
/* we won't attempt to reset dirtybits when bab is more than this
   number of sectors full. (Walking the bab is expensive) */
#define FTD_RESET_MAX  (1024 * 20)

typedef struct ftd_lg
  {
    ftd_int32_t state;
    atomic_bitops_t flags;
    dev_t dev;			/* our device number */
    ftd_ctl_t *ctlp;
    struct ftd_dev *devhead;
    struct ftd_lg *next;
    struct ftd_dev *tempdev;
    struct _bab_mgr_ *mgr;
/* NB: "lock" appears to protect the ftd_lg struct as well as the
 * memory allocated by the "mgr" and the mgr itself.
 */
    ftd_lock_t lock;  
#if defined(SOLARIS)
    /* Support stuff for poll */
    struct pollhead ph;
#elif defined(HPUX)
	#if (SYSVERS >= 1100)
	    struct kthread *read_waiter_thread;
	#else
	 	struct proc *readproc;
	#endif

#elif defined(_AIX)
    /* selnotify() parms */
    dev_t ev_id;
    ftd_int32_t ev_subid;
    ftd_int32_t ev_rtnevents;
#elif defined(linux)
    wait_queue_head_t ph;       /* poll wait queue head */
#endif
    ftd_char_t *statbuf;
    ftd_int32_t statsize;

    ftd_int32_t wlentries;	/* number of entries in writelog */
    ftd_int64_t wlsectors;	/* number of sectors in writelog */
    ftd_int32_t dirtymaplag;	/* number of entries since last 
				   reset of dirtybits */

    ftd_int32_t ndevs;

    ftd_uint32_t sync_depth;
    ftd_uint32_t sync_timeout;
    ftd_uint32_t iodelay;
    ftd_uint32_t lrt_mode;      /* if set use LRT */
#if defined(SOLARIS)
    struct dev_ops *ps_do;
#endif
#if defined(_AIX)
    struct file *ps_fp;
    ftd_int32_t ps_opencnt;
#endif
#if defined(linux)
    struct block_device *bd;
#endif
    dev_t persistent_store;
    wlheader_t *syncHead;
    wlheader_t *syncTail;
#if defined(linux)
    int th_lgno;	/* logical group number */
    int th_pid;		/* thread id */
    struct completion th_exit;
    wait_queue_head_t th_sleep;
#endif
    int KeepHistoryBits;    /* Track in both map and mapnext when true otherwise track in map only */
    int DualBitmaps;        // Operates on map and mapnext like KeepHistoryBits, but this one is used while the driver
                            // is in refresh to achieve a resumable/"smarter" smart-refresh.
      
    // This is used to store stats on how many bytes of IO's were done
    // Received by the filter
    ftd_uint64_t    ulWriteByteCount;
    // Written on disk (completed by lower level filters)
    ftd_uint64_t    ulWriteDoneByteCount;
    // Allocated into BAB
    ftd_uint64_t    ulPendingByteCount;
    // Moved from pending -> committed
    ftd_uint64_t    ulCommittedByteCount;
    // Removed from the BAB (after migration)
    ftd_uint64_t    ulMigratedByteCount;

    // Used by the pending_ios_monitoring API
    ftd_lock_t pending_ios_monitoring_lock;
    int pending_ios_monitoring_critical_section_busy;
#if defined(linux)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
    struct mutex thrd_mutex;
#endif
#endif
    // WI_338550 December 2017, implementing RPO / RTT
    struct  
        {
        // Time that the last IO was received
        ftd_uint64_t LastIOTimestamp;
        // Time of the last IO considered "consistent"
        ftd_uint64_t LastConsistentIOTimestamp;
        // Time of the oldest IO considered "inconsistent"
        ftd_uint64_t OldestInconsistentIOTimestamp;
        ftd_uint32_t RTT;
        // Indicate if the system maintains write ordering to the mirror
        ftd_int32_t WriteOrderingMaintained;
        // Network chunk size from group tunables, to estimate time remaining to send the pending data to the target
        ftd_uint32_t network_chunk_size_in_bytes;
        ftd_uint32_t previous_non_zero_RTT;
        ftd_uint32_t average_of_most_recent_RTTs;
        } PMDStats;

}
ftd_lg_t;

/*-
 * sync mode states
 *
 * a logical group is in sync mode where the sync depth is 
 * set positive indefinite.
 * 
 * sync mode behavior is triggered only when some threshold 
 * value number of unacknowledge write log entries accumulates.
 */
#define LG_IS_SYNCMODE(lgp) ((lgp)->sync_depth != (ftd_uint32_t)-1)
#define LG_DO_SYNCMODE(lgp) \
	(LG_IS_SYNCMODE((lgp)) && ((lgp)->wlentries >= (lgp)->sync_depth))


typedef struct ftd_dev
  {
    struct ftd_dev *next;	/*   0 */
    atomic_bitops_t flags;
    ftd_lg_t *lgp;
    ftd_uint64_t disksize;
    ftd_uint64_t maxdisksize;
    ftd_lock_t lock;  
    ftd_uint32_t lrdb_srt;
    ftd_uint32_t lrdb_free;
#if defined(linux)
    ftd_uint32_t lrdb_state;	/* see FTD_STAT_LRDB_XXX */
    ftd_uint32_t lrdb_epoch;	/* lrdb flush epoch */
#else
    ftd_uint32_t lrdb_stat;
#endif
    ftd_uint32_t psdev_bad;     /* LRDB io error to this dev && not corrected */
    ftd_uint32_t lrdb_busycnt;
    ftd_uint32_t lrdb_freecnt;
    ftd_uint32_t lrdb_reiocnt;
    ftd_uint32_t lrdb_initcnt;
    ftd_uint32_t lrdb_finicnt;
    ftd_uint32_t lrdb_errcnt;
    ftd_uint32_t lrdb_end;
    struct buf *lrdbbp;
#if defined(linux)
    ftd_uint32_t lrdb_offset;
    atomic_t lrdb_count;	/* # of LRDB requests per flush */
#else
    ftd_metabuf_t lrdb_metabuf;
#endif
    ftd_bitmap_t lrdb;
    ftd_bitmap_t hrdb;
    struct buf *qhead;
    struct buf *qtail;
    ftd_int32_t bufcnt;
    struct buf *freelist;

#if defined (HPUX)
    /* WR 32873 (129P13); WR 34691(2.1.x) */
    b_sema_t    lrdbsema;       /* beta semaphore enforce sync write */
#elif defined (SOLARIS)
    ksema_t     lrdbsema;       /* semaphore enforce sync write */
#endif

/* four unique device ids associated with a device */
    dev_t cdev;
    dev_t bdev;
    dev_t localbdisk;
    dev_t localcdisk;
#if defined(_AIX)
    struct file *dev_fp;	/* fp_xxx() usage */
    ftd_int32_t dev_opencnt;
#endif
#if defined(linux)
    struct block_device *bd;
    struct request_queue* queue; /**< Holds the request queue for the associated gendisk. */
#endif
    ftd_char_t *statbuf;
    ftd_int32_t statsize;
    struct dev_ops *ld_dop;	/* handle on local disk */

    /* BS stats */
    ftd_int64_t sectorsread;
    ftd_int64_t readiocnt;
    ftd_int64_t sectorswritten;
    ftd_int64_t writeiocnt;
    ftd_int32_t wlentries;	/* number of entries in wl */
    ftd_int64_t wlsectors;	/* number of sectors in wl */

    // Used by the pending_ios_monitoring API:  
    ftd_uint32_t pending_ios_monitoring_pending_ios;
    ftd_uint32_t pending_ios_monitoring_waiting_for_no_pending_ios; /** @todo should be a bool. */

    /* Dynamic activation management*/
    ftd_uint32_t local_disk_captured_requests_being_processed;
}
ftd_dev_t;

#define FTD_BITX_DEV_OPEN                0
#define FTD_BITX_STOP_IN_PROGRESS        1
#define FTD_BITX_LOCAL_DISK_IO_CAPTURED  2
#define FTD_BITX_CAPTURED_DEVICE_RELEASE_INITIATED 3

#define FTD_DEV_OPEN            FTD_MASK(DEV_OPEN)
#define FTD_STOP_IN_PROGRESS    FTD_MASK(STOP_IN_PROGRESS)
#define FTD_LOCAL_DISK_IO_CAPTURED            FTD_MASK(LOCAL_DISK_IO_CAPTURED)
#define FTD_CAPTURED_DEVICE_RELEASE_INITIATED FTD_MASK(CAPTURED_DEVICE_RELEASE_INITIATED)

#define FTD_IOPENDING -1

/* A HRDB bit should address no less than a sector */
#define MINIMUM_HRDB_BITSIZE      DEV_BSHIFT

/* The following specify the resolution of HRDB for LARGE HRT model; i.e. 1 HRDB bit will map to 2^HRDB_BITSIZE_# */
#define HRDB_BITSIZE_1            10 /* 1 HRDB bit = 2^10 (1024) bytes */
#define HRDB_BITSIZE_2            11
#define HRDB_BITSIZE_3            12
#define HRDB_BITSIZE_4            13
#define HRDB_BITSIZE_5            14
#define HRDB_BITSIZE_6            15
#define HRDB_BITSIZE_7            16
#define HRDB_BITSIZE_8            17
#define HRDB_BITSIZE_9            18
#define HRDB_BITSIZE_10           19
#define HRDB_BITSIZE_11           20

/* The different disk sizes in bytes to calculate HRDB resolution */
#define DISK_SIZE_1               600*1024*1024          /* 600MB */
#define DISK_SIZE_2               3*1024*1024*1024       /* 3GB */
#define DISK_SIZE_3               50*1024*1024*1024      /* 50GB */
#define DISK_SIZE_4               200*1024*1024*1024     /* 200GB */

/* A LRDB bit should address no less than 256K */
#define MINIMUM_LRDB_BITSIZE      18

#if defined(HPUX)

/* WR34794: negative spinlock checking for HP service (check spinunlock).
 * Since it is not a Softek bug, set it to not checking owns_spinlock 
 * prior spinunlock call, by default.
 */

extern int ftd_lockdbg;

#define ALLOC_LOCK(lock,type)      \
                        ((lock) = alloc_spinlock(FTD_SPINLOCK_ORDER, type))
#if (SYSVERS >= 1123)

   #define ACQUIRE_LOCK(lock,context) spinlock(lock)

   /* struct lock definition different in IA64 and PA starting with HPUX 11.23 */
   #ifndef __ia64
      #define RELEASE_LOCK(lock,context) \
      if (ftd_lockdbg && (lock) && !owns_spinlock(lock) ) {  \
         FTD_ERR (FTD_WRNLVL, "ftd_lockdbg: lck=@0x%lx, {lock=0x%x, owner=0x%x, diags_ptr=%s, flag=0x%x, next_cpu=0x%x, check=0x%x}", (lock), lock->sl_lock, lock->sl_owner, lock->sl_diags_ptr, lock->sl_flag, lock->sl_next_cpu, lock->sl_check); \
      } \
      spinunlock(lock);
   #else
      #define RELEASE_LOCK(lock,context) \
      if (ftd_lockdbg && (lock) && !owns_spinlock(lock) ) {  \
         FTD_ERR (FTD_WRNLVL, "ftd_lockdbg: lck=@0x%lx, {owner=0x%x, diags_ptr=%s, sl_ticket=0x%x, sl_serving=0x%x, check=0x%x}", (lock), lock->sl_owner, lock->sl_diags_ptr, lock->sl_ticket, lock->sl_serving, lock->sl_check); \
      } \
      spinunlock(lock);
   #endif

#elif (SYSVERS == 1100) || (SYSVERS == 1111)

   #define ACQUIRE_LOCK(lock,context) spinlock(lock)

   #define RELEASE_LOCK(lock,context) \
      if (ftd_lockdbg && (lock) && !owns_spinlock(lock) ) {  \
         FTD_ERR (FTD_WRNLVL, "ftd_lockdbg: lck=@0x%lx, {lock=0x%x, owner=0x%x, name_ptr=%s, flag=0x%x, next_cpu=0x%x}", (lock), lock->sl_lock, lock->sl_owner, lock->sl_name_ptr, lock->sl_flag, lock->sl_next_cpu); \
      } \
      spinunlock(lock);

#else

   #define ACQUIRE_LOCK(lock,context) spinlock(lock, context)
   #define RELEASE_LOCK(lock,context) \
      if (ftd_lockdbg && (lock) && !owns_spinlock(lock) ) {  \
         FTD_ERR (FTD_WRNLVL, "ftd_lockdbg: lck=@0x%lx, {lock=0x%x, owner=0x%x, name_ptr=%s, flag=0x%x, next_cpu=0x%x}", (lock), lock->sl_lock, lock->sl_owner, lock->sl_name_ptr, lock->sl_flag, lock->sl_next_cpu); \
      } \
      spinunlock(lock,context);

#endif

#define DEALLOC_LOCK(lock) \
	if ((lock)) { \
		dealloc_spinlock(lock); \
	}

#elif defined(SOLARIS)

#define ALLOC_LOCK(lock,type)      mutex_init(&(lock), type, MUTEX_DRIVER, NULL)
#define ACQUIRE_LOCK(lock,context) mutex_enter(&(lock))
#define RELEASE_LOCK(lock,context) mutex_exit(&(lock))
#define DEALLOC_LOCK(lock)         mutex_destroy(&(lock))

#elif defined(linux)

#define FTD_MSG_INTERVAL 1000000

#define ALLOC_LOCK(lock,type)		spin_lock_init(&lock)
#define ACQUIRE_LOCK(lock,context)	spin_lock_irqsave(&lock,context)
#define RELEASE_LOCK(lock,context)	spin_unlock_irqrestore(&lock,context)
#define DEALLOC_LOCK(lock)

#include <asm/atomic.h>
#define ATOMIC_SET_BIT(n, t) set_bit(n, t)
#define ATOMIC_CLEAR_BIT(n, t) test_and_clear_bit(n, t)
#define ATOMIC_TEST_AND_SET_BIT(n, t) test_and_set_bit(n, t)
#define ATOMIC_TEST_AND_CLEAR_BIT(n, t) test_and_clear_bit(n, t)
#elif defined(_AIX)
/* AIX spec locking services */

/* 
 * lock_alloc() usage:
 * The  lock_alloc kernel service allocates system memory for a sim-
 * ple or complex lock. The lock_alloc kernel service must be called
 * for  each  simple  or  complex before the lock is initialized and
 * used. The memory allocated is for internal  lock  instrumentation
 * use, and is not returned to the caller; no memory is allocated if
 * instrumentation is not used.
 * 
 * simple_lock_init() usage:
 * The  simple_lock_init  kernel  service initializes a simple lock.
 * This kernel service must be called  before  the  simple  lock  is
 * used.  The  simple  lock must previously have been allocated with
 * the lock_alloc kernel service.
 *
 * lock_free() usage:
 * The lock_free kernel service frees the memory of a simple or com-
 * plex lock. The memory freed is the internal operating system mem-
 * ory which was allocated with the lock_alloc kernel service.
 * 
 * without lock instrumentation enabled, AIX ALLOC_LOCK(), and 
 * DEALLOC_LOCK() are effectively noops. since this instrumentation 
 * might come in  handy, build it into the driver...
 * 
 * disable_lock() usage:
 * The  disable_lock  kernel  service raises the interrupt priority,
 * and locks a simple lock if necessary, in order to  provide  opti-
 * mized thread-interrupt critical section protection for the system
 * on which it is executing. On a multiprocessor system, calling the
 * disable_lock  kernel  service is equivalent to calling the i_dis-
 * able and simple_lock kernel services. On a  uniprocessor  system,
 * the  call  to  the  simple_lock  service is not necessary, and is
 * omitted. However, you should still pass a valid lock  address  to
 * the  disable_lock kernel service. Never pass a NULL lock address.
 * 
 * unlock_enable() usage:
 * The  unlock_enable kernel service unlocks a simple lock if neces-
 * sary, and restores the interrupt priority, in  order  to  provide
 * optimized  thread-interrupt  critical  section protection for the
 * system on which it is  executing.  On  a  multiprocessor  system,
 * calling the unlock_enable kernel service is equivalent to calling
 * the simple_unlock and i_enable kernel services. On a uniprocessor
 * system,  the  call to the simple_unlock service is not necessary,
 * and is omitted.  However, you should still pass  the  valid  lock
 * address  which  was  used with the corresponding call to the dis-
 * able_lock kernel service. Never pass a NULL lock address.
 */

#define FTD_LOCK_CLASS             0
#define LOCK_PRI                   INTIODONE

#define ALLOC_LOCK(lock,type)      { \
	simple_lock_init(&(lock)); \
}

#define ACQUIRE_LOCK(lock,context) \
        ((context) = (ftd_uint32_t)disable_lock(LOCK_PRI, &(lock)))

#define RELEASE_LOCK(lock,context) unlock_enable((ftd_int32_t)(context), &(lock))


#define DEALLOC_LOCK(lock)         lock_free((ftd_void_t *)&(lock))


#define TRY_LOCK(lock)             \
        ((simple_lock_try((ftd_void_t *)&(lock)) == TRUE)?1:0)

#define LOCK_MINE(lock)             \
        ((lock_mine((ftd_void_t *)&(lock)) == TRUE)?1:0)

#endif

#if defined(linux)
#define	FTD_DELAY(us) { \
         struct timeval tv = { 0, us }; \
         set_current_state(TASK_UNINTERRUPTIBLE); \
         schedule_timeout (timeval_to_jiffies(&tv)); \
 	}
#else
#define	FTD_DELAY(us)	delay (drv_usectohz(us))
#endif

#define NOT_IMPLEMENETED        @@@; ERROR HERE: CODE NOT IMPLEMENTED @@@;

/*
 * The following macros should expand to a lval based on the bp.  we use
 * them to hide differences between hp and solaris that would otherwise 
 * obfuscate the code.
 */
#if defined(HPUX)

/*-
 * whether to serialize LRT map accesses.
 */
#define LRDBSYNCH

#elif defined(SOLARIS)

/*-
 * whether to serialize LRT map accesses.
 */
#define LRDBSYNCH

#elif defined(_AIX)

/*-
 * whether to serialize LRT map accesses.
 */
#define LRDBSYNCH

#elif defined(linux)

#define LRDBSYNCH

#else
NOT_IMPLEMENTED
#endif

#if !defined(_AIX)
extern ftd_int32_t compare_and_swap(ftd_uint32_t *waddr, ftd_uint32_t *oval, ftd_uint32_t nval);
extern ftd_int32_t fetch_and_and(ftd_uint32_t *waddr, ftd_uint32_t mask);
extern ftd_int32_t fetch_and_or(ftd_uint32_t *waddr, ftd_uint32_t mask);
#endif /* !defined(_AIX) */


#if !defined(TRUE)
#define TRUE 1
#define FALSE 0
#endif /* !defined(TRUE) */

/*
 * LRDB bit-wise state per device instance
 */
#define	FTD_STAT_LRDB_NONE	0
#define	FTD_STAT_LRDB_TO_FLUSH	1	/* pending flush request */
#define	FTD_STAT_LRDB_IN_FLUSH	2	/* during flush operation */
#define	FTD_STAT_LRDB_DIRTY	4	/* block write, LRDB dirty */

#if defined(LRDBSYNCH)
/*-
 * test whether LRT map IO is currently pending.
 * 
 * if not: 
 *   o mark LRT map busy.
 *   o initiate LRT map IO.
 * 
 * if so: 
 *   o mark LRT map wanted. 
 *   o return without initiating IO.
 *   o upon pending IO completion, 
 *     this will be sensed in biodone_psdev().
 *     from there, another IO will be initiated.
 */

/* LRT map is available */
#define LRDB_FREE_FREE 0x00000001

/*- 
 * LRT map not available, 
 * there is a previous IO pending.
 * initiate another IO upon 
 * completion of the pending IO.
 */
#define LRDB_STAT_WANT 0x00000001

#if defined(_AIX)
#define LRDB_FREE_SYNCH(l) {\
	ftd_uint32_t nval=0; \
	ftd_uint32_t oval=LRDB_FREE_FREE; \
	if(compare_and_swap((int *)&((l)->lrdb_free), (int *)&oval, nval) == FALSE) { \
		/* FTD_ERR(FTD_WRNLVL, "LRDB_FREE_SYNCH: busy"); */ \
		(l)->lrdb_busycnt++; \
		fetch_and_or((int *)&((l)->lrdb_stat), LRDB_STAT_WANT); \
		return(0); \
	} \
	else { \
		/* FTD_ERR(FTD_WRNLVL, "LRDB_FREE_SYNCH: free"); */ \
		(l)->lrdb_freecnt++; \
	} \
}
#else
#define LRDB_FREE_SYNCH(l) {\
	ftd_uint32_t nval=0; \
	ftd_uint32_t oval=LRDB_FREE_FREE; \
	if(compare_and_swap(&((l)->lrdb_free), &oval, nval) == FALSE) { \
		/* FTD_ERR(FTD_WRNLVL, "LRDB_FREE_SYNCH: busy"); */ \
		(l)->lrdb_busycnt++; \
		fetch_and_or(&((l)->lrdb_stat), LRDB_STAT_WANT); \
		return(0); \
	} \
	else { \
		/* FTD_ERR(FTD_WRNLVL, "LRDB_FREE_SYNCH: free"); */ \
		(l)->lrdb_freecnt++; \
	} \
}
#endif

/* unset LRT map free */
#define LRDB_FREE_ACK(l) (l)->lrdb_free = LRDB_FREE_FREE

/* unset LRT map want */
#if defined(_AIX)
#define LRDB_WANT_ACK(l) fetch_and_and((atomic_p)(&((l)->lrdb_stat)), (uint)LRDB_STAT_WANT)
#else
#define LRDB_WANT_ACK(l) fetch_and_and(&((l)->lrdb_stat), LRDB_STAT_WANT)
#endif

#endif /* defined(LRDBSYNCH) */


/* debugging support */
#if defined(FTD_DEBUG) || defined(FTD_LINT) || defined(FTD_CPROTO)
#undef FTD_PRIVATE
#undef FTD_PUBLIC

#define FTD_PRIVATE
#define FTD_PUBLIC

#else  /* defined(FTD_DEBUG) || defined(FTD_LINT) || defined(FTD_CPROTO) */

#undef FTD_PRIVATE
#undef FTD_PUBLIC

#define FTD_PRIVATE static
#define FTD_PUBLIC  extern
#endif /* defined(FTD_DEBUG) || defined(FTD_LINT) || defined(FTD_CPROTO) */

#endif /* _FTD_DEF_H */
