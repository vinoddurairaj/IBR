/*
 * Copyright (c) 1996, 1998 by FullTime Software, Inc.  All Rights Reserved.
 *
 * Defines and structures used only within the driver.
 *
 * $Id: ftd_def.h,v 1.5 2001/12/06 17:31:01 bouchard Exp $
 */

#ifndef _FTD_DEF_H
#define _FTD_DEF_H

#if defined(_HPUX_SOURCE)

#include "../h/buf.h"
#include "../h/param.h"
#include "../h/tuneable.h"
#include "../h/conf.h"
#include "../h/errno.h"

#if defined(HPUX) && (SYSVERS >= 1100)
#include <sys/kthread_iface.h>
#endif

#define maxphys     MAXPHYS

typedef dev_t minor_t;

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
#include <sys/m_intr.h>
#include <sys/dir.h>
#include <sys/syspest.h>
#include <sys/sysmacros.h>

/* DEV_BSHIFT is set for 512 bytes */
#define DEV_BSHIFT 9
typedef dev_t minor_t;

#define minphys ftd_minphys
#define physio(strat, buf, dev, flag, min, uio) uphysio(uio, flag, BUF_CNT, dev, strat, min, MIN_PAR)
extern ftd_int32_t ftd_minphys (struct buf *bp, ftd_void_t * arg);
#define biodone(bp) iodone(bp)
#define biowait(bp) iowait(bp)
#define getminor(dev) minor(dev)
#define bdev_strategy(bp) devstrat(bp)

/* AIX defines nothing equivalent for these: */

#define KM_NOSLEEP 1
#define KM_SLEEP 2

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

#endif

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

/* a bitmap descriptor */
typedef struct ftd_bitmap
  {				/* SIZEOF == 20 */
    ftd_int32_t bitsize;	/* a bit represents this many bytes */
    ftd_int32_t shift;		/* converts sectors to bit values   */
    ftd_int32_t len32;		/* length of "map" in 32-bit words  */
    ftd_int32_t numbits;	/* number of valid bits */
    ftd_uint32_t *map;
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
    ftd_int32_t lg_open;	/* 14 */
    ftd_int32_t dev_open;	/* 18 */
#if defined(HPUX)
    lock_t *lock;
#elif defined(SOLARIS)
    kmutex_t lock;		/* 20 */
#elif defined(_AIX)
    /* AIX specific fields for ftd_ctl */
    Simple_lock lock;
#endif
    struct ftd_lg *lghead;	/* 28 */
    struct buf *ftd_buf_pool_headp;
    ftd_int32_t minor_index;
  }
ftd_ctl_t;

#define FTD_LG_OPEN      (0x00000001)
#define FTD_LG_COLLISION (0x00000002)
#define FTD_DIRTY_LAG 1024
/* we won't attempt to reset dirtybits when bab is more than this
   number of sectors full. (Walking the bab is expensive) */
#define FTD_RESET_MAX  (1024 * 20)

typedef struct ftd_lg
  {
    ftd_int32_t state;
    ftd_int32_t flags;
    dev_t dev;			/* our device number */
    ftd_ctl_t *ctlp;
    struct ftd_dev *devhead;
    struct ftd_lg *next;
    struct _bab_mgr_ *mgr;
/* NB: "lock" appears to protect the ftd_lg struct as well as the
 * memory allocated by the "mgr" and the mgr itself.
 */
#if defined(SOLARIS)
    kmutex_t lock;
    /* Support stuff for poll */
    struct pollhead ph;
#elif defined(HPUX)
    lock_t *lock;
	#if (SYSVERS >= 1100)
	    struct k_thread *read_waiter_thread;
	#else
	 	struct proc *readproc;
	#endif

#elif defined(_AIX)
    Simple_lock lock;
    /* selnotify() parms */
    ftd_int32_t ev_id;
    ftd_int32_t ev_subid;
    ftd_int32_t ev_rtnevents;
#endif
    ftd_char_t *statbuf;
    ftd_int32_t statsize;

    ftd_int32_t wlentries;	/* number of entries in writelog */
    ftd_int32_t wlsectors;	/* number of sectors in writelog */
    ftd_int32_t dirtymaplag;	/* number of entries since last 
				   reset of dirtybits */

    ftd_int32_t ndevs;

    ftd_uint32_t sync_depth;
    ftd_uint32_t sync_timeout;
    ftd_uint32_t iodelay;
#if defined(SOLARIS)
    struct dev_ops *ps_do;
#endif
#if defined(_AIX)
    struct file *ps_fp;
    ftd_int32_t ps_opencnt;
#endif
    dev_t persistent_store;
    wlheader_t *syncHead;
    wlheader_t *syncTail;
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
    ftd_int32_t flags;		/*   4 */
    ftd_lg_t *lgp;		/*   8 */
    daddr_t disksize;
#if defined(HPUX)
    lock_t *lock;
#elif defined(SOLARIS)
    kmutex_t lock;		/*  12 */
#elif defined(_AIX)
    /* AIX specific fields for ftd_dev */
    Simple_lock lock;
#endif
    ftd_uint32_t lrdb_srt;
    ftd_uint32_t lrdb_free;
    ftd_uint32_t lrdb_stat;
    ftd_uint32_t lrdb_busycnt;
    ftd_uint32_t lrdb_freecnt;
    ftd_uint32_t lrdb_reiocnt;
    ftd_uint32_t lrdb_initcnt;
    ftd_uint32_t lrdb_finicnt;
    ftd_uint32_t lrdb_errcnt;
    ftd_uint32_t lrdb_end;
#if defined(SOLARIS) || defined(HPUX)
    ftd_metabuf_t lrdb_metabuf;
#endif				/* defined(SOLARIS) */
    ftd_bitmap_t lrdb;		/*  20 */
    ftd_bitmap_t hrdb;		/*  40 */
    struct buf *qhead;		/*  60 */
    struct buf *qtail;		/*  64 */
    ftd_int32_t bufcnt;
    struct buf *freelist;
    struct buf *pendingbps;
    struct buf *lrdbbp;
    ftd_int32_t pendingios;
    ftd_int32_t highwater;

/* four unique device ids associated with a device */
    dev_t cdev;			/*  72 */
    dev_t bdev;			/*  76 */
    dev_t localbdisk;		/*  80 */
    dev_t localcdisk;		/*  84 */
#if defined(_AIX)
    struct file *dev_fp;	/* fp_xxx() usage */
    ftd_int32_t dev_opencnt;
#endif
    ftd_char_t *statbuf;	/*  88 */
    ftd_int32_t statsize;	/*  92 */
    struct dev_ops *ld_dop;	/* handle on local disk *//* 96 */

    /* BS stats */
    ftd_int64_t sectorsread;	/* 100 */
    ftd_int64_t readiocnt;	/* 108 */
    ftd_int64_t sectorswritten;	/* 116 */
    ftd_int64_t writeiocnt;	/* 124 */
    ftd_int32_t wlentries;	/* number of entries in wl *//* 132 */
    ftd_int32_t wlsectors;	/* number of sectors in wl *//* 136 */
  }
ftd_dev_t;

#define FTD_DEV_OPEN            (0x1)
#define FTD_STOP_IN_PROGRESS    (0x2)

/* A HRDB bit should address no less than a sector */
#define MINIMUM_HRDB_BITSIZE      DEV_BSHIFT

/* A LRDB bit should address no less than 256K */
#define MINIMUM_LRDB_BITSIZE      18

#if defined(HPUX)

#define ALLOC_LOCK(lock,type)      \
                        ((lock) = alloc_spinlock(FTD_SPINLOCK_ORDER, type))
#if (SYSVERS >= 1100)
#define ACQUIRE_LOCK(lock,context) spinlock(lock)
#define RELEASE_LOCK(lock,context) spinunlock(lock)
#else
#define ACQUIRE_LOCK(lock,context) spinlock(lock, context)
#define RELEASE_LOCK(lock,context) spinunlock(lock, context)
#endif
#define DEALLOC_LOCK(lock)         dealloc_spinlock(lock)

#elif defined(SOLARIS)

#define ALLOC_LOCK(lock,type)      mutex_init(&(lock), type, MUTEX_DRIVER, NULL)
#define ACQUIRE_LOCK(lock,context) mutex_enter(&(lock))
#define RELEASE_LOCK(lock,context) mutex_exit(&(lock))
#define DEALLOC_LOCK(lock)         mutex_destroy(&(lock))

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

/* unset LRT map free */
#define LRDB_FREE_ACK(l) (l)->lrdb_free = LRDB_FREE_FREE

/* unset LRT map want */
#define LRDB_WANT_ACK(l) fetch_and_and(&((l)->lrdb_stat), LRDB_STAT_WANT)

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
