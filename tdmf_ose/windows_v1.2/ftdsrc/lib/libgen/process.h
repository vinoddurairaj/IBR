/*
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

#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "common.h"

/* function prototypes */
extern void wait_child(int*, int);
extern void killpmds(void);
extern int execpmd(int, char**);
extern int startmaster(char*, char**);
extern int startpmd(int, int, char**);
extern int wlscan(sddisk_t*);
extern int startapply(int cfgidx, int sig, char **pmdargv);

#endif
