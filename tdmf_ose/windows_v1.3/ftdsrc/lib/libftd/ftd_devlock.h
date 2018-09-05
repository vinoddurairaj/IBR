/*
 * ftd_devlock.h - FTD device lock interface
 * 
 * Copyright (c) 2000 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */

#if !defined	_FTD_DEV_LOCK_H_
#define			_FTD_DEV_LOCK_H_

extern int ftd_dev_lock_create(void);
extern void ftd_dev_lock_delete(void);
extern HANDLE ftd_dev_lock(char *devname, int lgnum);
extern int ftd_dev_unlock(HANDLE fd);
extern int ftd_dev_locked_disksize(HANDLE fd);
extern int ftd_dev_sync(char *devname, int lgnum);

#endif
