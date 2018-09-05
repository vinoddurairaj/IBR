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
 * $Id: ftd_all.h,v 1.3 2004/04/07 14:04:51 szg00 Exp $
 *
 */
#ifndef  _FTD_ALL_H_ 
#define  _FTD_ALL_H_ 

/* our global data */
extern void      *ftd_dev_state;
extern void      *ftd_lg_state;
extern ftd_ctl_t *ftd_global_state;
/* 
 * ftd_failsafe can be set in a debugger early in the boot process to allow
 * ftd to not attach if you have a really FUBAR'd code installed which would
 * otherwise panic on boot.
 */
extern int       ftd_failsafe;

/* entries in ftd_all.c: */
int ftd_dev_n_open(ftd_ctl_t *ctlp);
FTD_STATUS ftd_rw(dev_t dev, struct uio *uio, int flag);
FTD_STATUS ftd_ctl_close(dev_t dev, int flag);
FTD_STATUS ftd_ctl_open(dev_t dev, int flag);
FTD_STATUS ftd_dev_close(dev_t dev, int flag);
FTD_STATUS ftd_dev_open(dev_t dev, int flag);
FTD_STATUS ftd_lg_close(dev_t dev, int flag);
FTD_STATUS ftd_lg_open(dev_t dev, int flag);
FTD_STATUS ftd_strategy(struct buf *bp);

FTD_STATUS ftd_flush_lrdb(ftd_dev_t *softp, PIRP Irp, KEVENT *event);

FTD_STATUS ftd_del_lg(ftd_lg_t *, minor_t);

FTD_STATUS ftd_del_device(ftd_dev_t *softp, minor_t minor);

void ftd_do_sync_done(ftd_lg_t *lgp);

void ftd_set_dirtybits(ftd_lg_t *lgp);
void ftd_clear_dirtybits(ftd_lg_t *lgp);
void ftd_clear_hrdb(ftd_lg_t *lgp);

/* entries in both ftd_hpux.c and ftd_sun.c */
int ftd_ctl_get_device_nums(dev_t dev, int arg, int flag);

/* entries in ftd_ioctl.c: */
int ftd_ctl_ioctl(dev_t, int, int, int);
int ftd_lg_ioctl(dev_t, int, int, int);

void ftd_wakeup(ftd_lg_t *);

#endif /* _FTD_ALL_H_ */
