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
 * $Id: ftd_klog.h,v 1.3 2004/08/27 20:26:26 vqba0 Exp $
 *
 */

/* ftd common kernel error logging interface */

#if !defined(_FTD_KLOG_H_)
#define  _FTD_KLOG_H_

#include <sys/types.h>

/* varargs usage */
#include <stdarg.h>
#include "ftd_def.h"
#include "ftd_ddi.h"

#include "..\LogMsg.h"

#define NBBY 8

extern VOID EvLogMessage(IN unsigned int dwErrorCode, IN char * str);
extern VOID cmn_err(IN PUCHAR str);
extern void ftd_err(int s, char *m, int l, char *f, ...);

#define WRNLVL 0x01
#define NTCLVL 0x02
#define DBGLVL 0x03

#define FTD_WRNLVL WRNLVL,__FILE__,__LINE__
#define FTD_NTCLVL NTCLVL,__FILE__,__LINE__
#define FTD_DBGLVL DBGLVL,__FILE__,__LINE__

#define FTD_ERR   ftd_err
#define FTD_ERR_V FTD_ERR

#define FTDERRMSGBUFSZ 256

/* type of error log record */
struct error_log_def_s {
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

#endif /* !defined(_FTD_KLOG_H_) */
