#ifndef _STAT_INTR_H_
#define _STAT_INTR_H_

/*
 * stat_intr.h - FTD Statistics interface
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

int initdvrstats(void);
void initstats(machine_t *mysys);
int savestats(int force);
void dumpstats(machine_t *sys);
void dump_pmd_stats(machine_t *sys);
void dump_rmd_stats(machine_t *sys);
#endif /* _STAT_INTR_H_ */
