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
 * ftd_all.c - FullTime Data driver code for ALL platforms
 *
 * Copyright (c) 1996, 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 * $Id: ftd_klog.h,v 1.6 2010/12/20 20:17:19 dkodjo Exp $
 *
 */

/* ftd common kernel error logging interface */

#if !defined(_FTD_KLOG_H_)
#define  _FTD_KLOG_H_

#if defined(linux)
#include <linux/types.h>
#include <linux/param.h>
#else
#include <sys/types.h>
#include <sys/param.h>
#endif /* defined(linux) */

/* varargs usage */
#if defined(_AIX) || defined(HPUX) || defined(linux)
#include <stdarg.h>
#else  /* defined(_AIX) || defined(HPUX) */
#include <sys/varargs.h>
#endif /* defined(_AIX) || defined(HPUX) */

#if !defined(HPUX)
#if defined(linux)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
#include <sys/syslog.h>
#endif
#else
#include <sys/syslog.h>
#endif
#endif /* !defined(HPUX) */

#if !defined(linux)
/* cmn_err, or errsave usage */
#if defined(_AIX)
#include <sys/err_rec.h>
#include <sys/errids.h>
#else  /* defined(_AIX) */
#if !defined(MAIN)
#include <sys/cmn_err.h>
/* quiet some cpp(1) warnings ... */
#ifdef splstr
#undef splstr
#endif
#endif /* !define(MAIN) */
#endif /* defined(_AIX) */
#endif /* !defined(linux) */

#if defined(SOLARIS) && !defined(MAIN)
#include <sys/ddi.h>
#include <sys/sunddi.h>
#endif /* defined(SOLARIS) && !defined(MAIN) */

#if defined(SOLARIS) && defined(MAIN)
#define	CE_CONT  0	/* continuation				*/
#define	CE_NOTE  1	/* notice				*/
#define	CE_WARN	 2	/* warning				*/
#define	CE_PANIC 3	/* panic				*/
#endif /* defined(SOLARIS) && defined(MAIN) */

#if defined(linux)
#define CE_CONT  0      /* continuation                         */
#define CE_NOTE  1      /* notice                               */
#define CE_WARN  2      /* warning                              */
#define CE_PANIC 3      /* panic                                */
#endif /* defined(linux) */

extern void ftd_err(int s, char *m, int l, char *f, ...);

#if defined(_AIX)
#define WRNLVL LOG_WARNING
#define NTCLVL LOG_NOTICE
#define DBGLVL LOG_DEBUG
#else /* defined(_AIX) */
#define WRNLVL CE_WARN
#define NTCLVL CE_NOTE
#define DBGLVL CE_CONT
#endif /* defined(_AIX) */

#define FTD_WRNLVL WRNLVL,__FILE__,__LINE__
#define FTD_NTCLVL NTCLVL,__FILE__,__LINE__
#define FTD_DBGLVL DBGLVL,__FILE__,__LINE__

#define FTD_ERR   ftd_err
#define FTD_ERR_V FTD_ERR

extern  int ftddprintf;
#define FTD_DPRINTF if(ftddprintf)ftd_err

#define FTDERRMSGBUFSZ 256

/* type of error log record */
struct error_log_def_s {
#if defined(_AIX)
	struct err_rec0   errhead;
#endif /* defined(_AIX) */
	char _errmsg[FTDERRMSGBUFSZ];
};

/* kern exec trace defs */

enum ftd_trace_values {
    FTD_CONFIG, FTD_CONFIG_DRV, FTD_UNCONFIG_DRV,
    FTD_BAB_INIT, FTD_BAB_FINI,
    FTD_OPEN, FTD_CLOSE, FTD_IOCTL, FTD_READ, FTD_WRITE,
    FTD_STRATEGY, FTD_RW, 
    FTD_STRAT, FTD_SELECT, FTD_WAKEUP, FTD_CLEAR_BAB_AND_STATS, 
    FTD_CTL_GET_CONFIG, FTD_BIOCLONE, FTD_CTL_GET_NUM_DEVICES, 
    FTD_CTL_GET_DEVICES_INFO, FTD_CTL_GET_GROUPS_INFO, FTD_CTL_GET_DEVICE_STATS,
    FTD_CTL_GET_GROUP_STATS, FTD_CTL_SET_GROUP_STATE,
    FTD_CTL_UPDATE_LRDBS, FTD_CTL_UPDATE_HRDBS,
    FTD_CTL_GET_GROUP_STATE, FTD_CTL_CLEAR_BAB,
    FTD_CTL_CLEAR_DIRTYBITS, FTD_CTL_NEW_DEVICE, FTD_CTL_NEW_LG,
    FTD_CTL_DEL_DEVICE, FTD_CTL_DEL_LG, FTD_CTL_CTL_CONFIG,
    FTD_CTL_GET_DEV_STATE_BUFFER, FTD_CTL_GET_LG_STATE_BUFFER,
    FTD_CTL_SET_DEV_STATE_BUFFER, FTD_CTL_SET_LG_STATE_BUFFER,
    FTD_OPEN_PERSISTENT_STORE, FTD_CLOSE_PERSISTENT_STORE,
    FTD_CTL_SET_PERSISTENT_STORE, FTD_CTL_SET_IODELAY,
    FTD_CTL_SET_SYNC_DEPTH, FTD_CTL_SET_SYNC_TIMEOUT,
    FTD_CTL_START_LG, FTD_CTL_IOCTL, FTD_LG_SEND_LG_MESSAGE,
    FTD_LG_OLDEST_ENTRIES, FTD_LOOKUP_DEV, FTD_LG_MIGRATE,
    FTD_LG_GET_DIRTY_BITS, FTD_LG_GET_DIRTY_BIT_INFO,
    FTD_LG_SET_DIRTY_BITS, FTD_LG_UPDATE_DIRTYBITS, FTD_LG_IOCTL
};

#define _FTDHKID(w) (0x69800000|(w))

#if defined(_AIX)
#include <sys/trchkid.h>
/*
 * Trace macros for AIX
 */
#define Enter(type, w, dev, a, b, c, d)					\
    type _FUNC;								\
    dev_t _DEV;								\
    int _FLAG;								\
    int _LINE;								\
    int _WHICH = !TRC_ISON(0) ? (_FLAG = 0) :				\
	((_DEV = (dev)),						\
	 (_FLAG = 1),							\
	 ((TRCHKGT(_FTDHKID(w), _DEV, a, b, c, d), 0)), w)

#define Enterv(w, dev, a, b, c, d)					\
    dev_t _DEV;								\
    int _FLAG;								\
    int _LINE;								\
    int _WHICH = !TRC_ISON(0) ? (_FLAG = 0) :				\
	((_DEV = (dev)),						\
	 (_FLAG = 1),							\
	 ((TRCHKGT(_FTDHKID(w), _DEV, a, b, c, d), 0)), w)

#define Return(val) do {						\
    _FUNC = (val);							\
    _LINE = __LINE__;							\
    goto ExitLabel;							\
} while(0)

#define Returnv do {							\
    _LINE = __LINE__;							\
    goto ExitLabelv;							\
} while (0)

#define Exit do {							\
ExitLabel:								\
    if (_FLAG)								\
	TRCHKGT(_FTDHKID(_WHICH|0x8000), _DEV, _FUNC, _LINE, 0, 0);	\
    return _FUNC;							\
} while (0)

#define Exitv do {							\
ExitLabelv:								\
    if (_FLAG)								\
	TRCHKGT(_FTDHKID(_WHICH|0x8000), _DEV, 0, _LINE, 0, 0);		\
    return;								\
} while (0)

#else
/*
 * Trace macros for other systems are no-ops
 */
#define Enter(type, w, dev, a, b, c, d)
#define Enterv(w, dev, a, b, c, d)   
#define Return(v) return(v)
#define Returnv   return
#define Exit
#define Exitv

#endif

#endif /* !defined(_FTD_KLOG_H_) */
