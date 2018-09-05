/*
 * ftd_all.h - FullTime Data driver code for ALL platforms
 *
 * Copyright (c) 1996, 1998 FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 * $Id: ftd_all.h,v 1.3 2001/01/17 17:58:55 hutch Exp $
 *
 */
#ifndef  _FTD_ALL_H_
#define  _FTD_ALL_H_

/* our global data */
extern ftd_void_t *ftd_dev_state;
extern ftd_void_t *ftd_lg_state;
extern ftd_ctl_t *ftd_global_state;

/* 
 * ftd_failsafe can be set in a debugger early in the boot process to allow
 * ftd to not attach if you have a really FUBAR'd code installed which would
 * otherwise panic on boot.
 */
extern ftd_int32_t ftd_failsafe;

/* macro for devstrat funnel on all platforms */
#if defined(HPUX)
#define BDEVSTRAT bdev_strategy
#elif defined(SOLARIS)
#define BDEVSTRAT bdev_strategy
#elif defined(_AIX)
#define BDEVSTRAT devstrat
#endif /* defined(HPUX) */


#endif /* _FTD_ALL_H_ */
