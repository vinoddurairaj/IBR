/*
 * proc.h - proc interface
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

#ifndef _PROC_H_
#define _PROC_H_

#include <sys/types.h>

#ifdef _WINDOWS
#include <windows.h>

#define pid_t HANDLE
#endif

#define	MAXPROCNAME	256
#define	MAXCOMMAND	256

typedef struct proc_s {
	pid_t pid;						/* used by RMDThread  process id/thread HANDLE			*/
	char procname[MAXPROCNAME];		/* process (argv[0])/thread name	*/
#if !defined(_WINDOWS)
	char command[MAXCOMMAND];		/* command line						*/
#else
	LPDWORD	command;				// used by RMDThread
	void (__cdecl *function)(void *);
	int		nSignal;				// used by RMDThread
	HANDLE	hEvent;					// used by RMDThread
	DWORD	dwId;
#endif
	int hostid;						/* hostid process is running on		*/
	int type;						/* process type (ie. proc, thread)	*/
} proc_t;

#define	PROCTYPE_PROC		1
#define PROCTYPE_THREAD		2

/* prototypes */
extern int proc_init(proc_t *procp, char *commandline, char *procname);
extern int proc_exec(proc_t *procp, char **argv, char **envp, int wait);
extern proc_t *proc_create(int type);
extern int proc_delete(proc_t *procp);
extern int proc_signal(proc_t *procp, int sig);
extern int proc_terminate(proc_t *procp);
extern void proc_kill(proc_t *procp);
extern pid_t proc_get_pid(char *name, int exactflag, int *pcnt);
extern int proc_is_parent(char *procname);
#if defined(_WINDOWS)
extern int proc_wait(proc_t *procp, int secs);
#else
extern int proc_wait(proc_t *procp);
#endif

#endif
