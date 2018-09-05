/*
 * ftd_ddi.h 
 *
 * Copyright (c) 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#ifndef _FTD_DDI_H_
#define _FTD_DDI_H_

#if defined(HPUX)
#include "../h/cmn_err.h"
#include "../h/kernel.h"
#include "../h/stream.h"
#include "../h/buf.h"

#include "ftd_def.h"

#define KM_SLEEP         M_WAITOK
#define KM_NOSLEEP       M_NOWAIT

#define kmutex_t         b_sema_t

/* mutex support */
#define mutex_enter(x) b_psema(x)
#define mutex_exit(x)  b_vsema(x)
#define mutex_init(mutex, name, type, arg) \
                       b_initsema(mutex, SEM_WANT, FTD_MUTEX_ORDER, name)

/* there's no reason to waste a call when its value is global */
#define get_system_time()     time

#define ftd_geterror(bp) ((bp)->b_error)

/* ddi junk - use minor() number, not m_instance() number */
#define ddi_get_instance(x)      minor(x)
#define ddi_copyin(a,b,c,d)        copyin(a,b,c)
#define ddi_copyout(a,b,c,d)       copyout(a,b,c)

#define DDI_SUCCESS  0
#define DDI_FAILURE -1

/* convenience MACROS for device switch table entries */
#define bdev_close(dev) \
                    bdevsw[major(dev)].d_close(dev)
#define bdev_strategy(bp) \
                    bdevsw[major((bp)->b_dev)].d_strategy(bp)

#if TWS
/* cdevsw() needed 2 more args, temporarily masked out until talk to Leo */
#define cdev_close(dev) \
                    cdevsw[major(dev)].d_close(dev)
#endif
#define cdev_close(dev, flag, mode) \
                    cdevsw[major(dev)].d_close(dev, flag, mode)
#define cdev_ioctl(dev, cmd, arg, flag) \
                    cdevsw[major(dev)].d_ioctl(dev, cmd, arg, flag)
#define cdev_drv_info(dev) \
                    (drv_info_t *)cdevsw[major(dev)].d_drv_info
/* 
 * Mutex order:
 */
#define FTD_MUTEX_ORDER     LVM_RW_LOCK_ORDER-2		/* 80 */
#define FTD_SPINLOCK_ORDER  FTD_MUTEX_ORDER+1	/* 81 */

/* 
 * DDI emulation functions 
 */
ftd_int32_t ddi_soft_state_init _ANSI_ARGS ((ftd_void_t ** state, size_t size, size_t nitems));
ftd_void_t ddi_soft_state_fini _ANSI_ARGS ((ftd_void_t ** state));
ftd_void_t *ddi_get_soft_state _ANSI_ARGS ((ftd_void_t * state, ftd_int32_t item));
ftd_void_t ddi_soft_state_free _ANSI_ARGS ((ftd_void_t * state, ftd_int32_t item));
ftd_int32_t ddi_soft_state_zalloc _ANSI_ARGS ((ftd_void_t * state, ftd_int32_t item));

/*
 * More Solaris emulation functions
 */
#if 0				/* does not seem to be used now */
ftd_int32_t ftd_drv_getparm _ANSI_ARGS ((ftd_uint32_t param, ftd_uint32_t * val));
#endif

ftd_void_t ftd_delay _ANSI_ARGS ((ftd_int32_t ticks, caddr_t addr));
clock_t drv_usectohz _ANSI_ARGS ((clock_t usec));
ftd_void_t bioerror _ANSI_ARGS ((struct buf * bp, ftd_int32_t error));
ftd_void_t *kmem_zalloc _ANSI_ARGS ((size_t size, ftd_int32_t flag));
struct buf *getrbuf _ANSI_ARGS ((ftd_int32_t));
ftd_void_t freerbuf _ANSI_ARGS ((struct buf *));
ftd_void_t bioreset _ANSI_ARGS ((struct buf *));

#elif defined(SOLARIS)

#include <sys/poll.h>
#include <sys/sunddi.h>
#include <sys/debug.h>

#elif defined(_AIX)

#include "ftd_def.h"

#define ddi_copyin(a,b,c,d)        copyin(a,b,c)
#define ddi_copyout(a,b,c,d)       copyout(a,b,c)

#define DDI_SUCCESS  0
#define DDI_FAILURE -1
/* 
 * DDI emulation functions 
 */
ftd_int32_t ddi_soft_state_init _ANSI_ARGS ((ftd_void_t ** state, size_t size, size_t nitems));
ftd_void_t ddi_soft_state_fini _ANSI_ARGS ((ftd_void_t ** state));
ftd_void_t *ddi_get_soft_state _ANSI_ARGS ((ftd_void_t * state, ftd_int32_t item));
ftd_void_t ddi_soft_state_free _ANSI_ARGS ((ftd_void_t * state, ftd_int32_t item));
ftd_int32_t ddi_soft_state_zalloc _ANSI_ARGS ((ftd_void_t * state, ftd_int32_t item));
/*
 * More Solaris emulation functions
 */
ftd_int32_t ftd_drv_getparm _ANSI_ARGS ((ftd_uint32_t param, ftd_uint32_t * val));
ftd_void_t ftd_delay _ANSI_ARGS ((ftd_int32_t ticks, caddr_t addr));
clock_t drv_usectohz _ANSI_ARGS ((clock_t usec));
ftd_void_t bioerror _ANSI_ARGS ((struct buf * bp, ftd_int32_t error));
ftd_void_t *kmem_zalloc _ANSI_ARGS ((size_t size, ftd_int32_t flag));
struct buf *getrbuf _ANSI_ARGS ((ftd_int32_t));
ftd_void_t freerbuf _ANSI_ARGS ((struct buf *));
ftd_void_t bioreset _ANSI_ARGS ((struct buf *));

#endif

/*
 * Finish up the biodone call
 */
ftd_void_t ftd_biodone_finish (struct buf *);

#if defined(HPUX) || defined(_AIX)
extern ftd_ctl_t *ftd_global_state;
#endif


#endif /* _FTD_DDI_H_ */
