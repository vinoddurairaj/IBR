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
#include "config.h"
#include "hash.h"

typedef enum lgarry
{
     LGARRY_PMD=1
    ,LGARRY_RMD=2
    ,LGARRY_RMDA=3
} lgarry_e;

typedef struct pid_hash_s
{
    pid_t    pid;
    int      lgnum;
    lgarry_e lgtbl;
} pid_hash_t;

extern pid_t rmda_pid[MAXLG];

/* function prototypes */
extern pid_hash_t * pid_hash_add(pid_t pid, int lgnum, lgarry_e lgtbl);
extern pid_hash_t * pid_hash_find(pid_t pid);
extern void pid_hash_delete(pid_hash_t *php);
extern void wait_child(int*, int);
extern void killpmds(void);
extern int execpmd(int, char**);
extern int startmaster(char*, char**);
extern int startpmd(int, int, char**);
extern int wlscan(sddisk_t*);
extern pid_t startapply(int cfgidx, int sig, char **pmdargv);
extern void close_pipe_set(int lgnum, int pmdflag);
extern void close_pipes(int lgnum, int pmdflag);
extern int exec_cp_cmd(int lgnum, char *cmd_prefix, int pmdflag);
extern void ch_rundir(const char *, const int);
extern int open_rundir(const char *);
extern void close_rundir(void);
extern void rm_rundir(const char *root, const char *subdir, const int lgnum, const pid_t pid);

#endif
