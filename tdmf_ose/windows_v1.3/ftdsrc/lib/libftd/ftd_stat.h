/*
 * ftd_stat.h - FTD logical group statistics interface
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
#ifndef _FTD_STAT_H
#define _FTD_STAT_H

#include "ftd_lg.h"

/* external prototypes */
#if !defined(_WINDOWS)
extern int ftd_stat_init_file(ftd_lg_t *lgp);
extern int ftd_stat_dump_file(ftd_lg_t *lgp);
#endif

extern int ftd_stat_init_driver(ftd_lg_t *lgp);
extern int ftd_stat_dump_driver(ftd_lg_t *lgp);
extern int ftd_stat_connection_driver(HANDLE ctlfd, int lgnum, int state);

extern int ftd_stat_set_connect(ftd_lg_t *lgp, int state);

#endif

