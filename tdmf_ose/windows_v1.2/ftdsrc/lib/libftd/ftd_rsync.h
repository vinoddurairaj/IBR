/*
 * ftd_rsync.h - FTD rsync interface 
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

#ifndef _FTD_RSYNC_H
#define _FTD_RSYNC_H 

#include "ftd_lg.h"
#include "ftd_dev.h"

/* external prototypes */
extern int ftd_lg_refresh(ftd_lg_t *lgp);
extern int ftd_lg_refresh_full(ftd_lg_t *lgp);
extern int ftd_lg_backfresh(ftd_lg_t *lgp);
extern int ftd_rsync_chksum_seg(ftd_lg_t *lgp, ftd_dev_t *devp);
extern int ftd_rsync_chksum_diff(ftd_dev_t *devp, ftd_dev_t *rdevp);
extern int ftd_rsync_flush_delta(ftd_sock_t *fsockp, ftd_lg_t *lgp,
	ftd_dev_t *devp);
extern int ftd_rsync_lg_flush_delta(ftd_lg_t *lgp);
	

#endif 

