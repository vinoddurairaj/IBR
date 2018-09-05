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

#ifndef _COMMON_H_
#define _COMMON_H_

#include "platform.h"
#include "config.h"
#include "ipc.h"
#include "ftd_cmd.h"
#include "ps_intr.h"

#define TDMF_TRACE
#ifdef TDMF_TRACE
#define DPRINTF(a) fprintf a
#else
#define DPRINTF(a)
#endif

typedef union {
	u_char uc[4];
	u_long ul;
} encodeunion;

typedef int EntryFunc(wlheader_t*, char*);

/* function prototypes */

extern char *ftdmalloc(int);
extern char *ftdcalloc(int, int);
extern char *ftdrealloc(char*, int);
extern void write_signal(int, int);
extern int opendevs(int);
extern void closedevs(int);
extern char *cfgpathtoname(char*);
extern int cfgpathtonum (char*);
extern int cfgtonum(int);
extern int numtocfg(int);
extern pid_t getprocessid(char*, int, int*);
extern int compress(u_char *source, u_char *dest, int len);
extern int decompress(u_char *source, u_char *dest, int len);
extern int is_parent_master(void);
extern sddisk_t *get_lg_dev(group_t *group, int devid);
extern sddisk_t *get_lg_rdev(group_t *group, int rdev);
extern sddisk_t *getdevice(rsync_t *rsync);
extern void net_lock(void);
extern void net_unlock(void);
extern int getmode(int lgnum);
extern int clearmode(int lgnum);
extern int setmode(int lgnum, int mode);
extern void encodeauth (time_t ts, char* hostname, u_long hostid, u_long ip,
    int* encodelen, char* encode);
extern void decodeauth (int encodelen, char* encodestr, time_t ts, 
    u_long hostid, char* hostname, size_t hostname_len, u_long ip);

#endif
