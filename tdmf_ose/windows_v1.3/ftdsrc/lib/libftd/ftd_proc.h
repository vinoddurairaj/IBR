/*
 * ftd_proc.h - FTD proc interface
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

#ifndef _FTD_PROC_H
#define _FTD_PROC_H

#include "ftd_port.h"
#include "ftd_sock_t.h"
#include "llist.h"
#include "proc.h"

// FTD process types
#define FTD_PROC_PMD		1
#define	FTD_PROC_RMD		2
#define	FTD_PROC_APPLY		3
#define	FTD_PROC_THROT		4
#define	FTD_PROC_GENERIC	5

// FTD process exit codes
#define FTD_EXIT_NORMAL		0
#define FTD_EXIT_RESTART	1
#define FTD_EXIT_NETWORK	FTD_EXIT_RESTART
#define FTD_EXIT_DIE		2
#define FTD_EXIT_DRV_FAIL	3   //WR17511.17512

#define FTDPROCMAGIC		0xBADF00D4

typedef struct ftd_proc_args_s {
	int     lgnum;					/* used by RMDThread ftd logical group number */
	int     state;					/* used by RMDThread enumerated process state */
	int     apply;                  /* used by RMDThread starts an apply rmd */
	SOCKET  dsock;					/* used by RMDThread rmd needs pmds' socket */
	int		cpon;					// used by RMDThread
	int		consleep;
	proc_t *procp;					// used by RMDThread
} ftd_proc_args_t;

typedef struct ftd_proc_s {
	int magicvalue;					/* so we know it's been initialized */
	int lgnum;						/* ftd logical group number */
	int proctype;					/* enumerated process type */
	int state;						/* enumerated process state */
	char * msg;						/* message */
#if !defined(_WINDOWS)
	sigset_t sigs;					/* proc signal mask	*/
#else
	ftd_proc_args_t args;           /* nt arguments */
#endif
	proc_t *procp;					/* generic proc structure */
	ftd_sock_t *fsockp;				/* ipc channel to master */
	LList *csockplist;				/* list of command connectsions	*/
} ftd_proc_t;

/* external prototypes */
extern LList *ftd_proc_create_list(void);
extern int ftd_proc_add_to_list(LList *proclist, ftd_proc_t **fprocp);
extern int ftd_proc_remove_from_list(LList *proclist, ftd_proc_t **fprocp);
extern int ftd_proc_delete_list(LList *proclist);
extern ftd_proc_t *ftd_proc_create(int type);
extern int ftd_proc_init(ftd_proc_t *fprocp, char *commandline, char *procname);
extern int ftd_proc_delete(ftd_proc_t *fprocp);
extern int ftd_proc_signal(char *procname, int sig);
extern int ftd_proc_exec_pmd(LList *proclist, ftd_proc_t *fprocp,
	ftd_sock_t *listener, int lgnum, int state, int consleep, char **envp);
extern int ftd_proc_exec_rmd(LList *proclist, ftd_proc_t *fprocp,
	ftd_sock_t *fsockp, ftd_sock_t *listener, int lgnum, char **envp);
extern int ftd_proc_exec_apply(ftd_proc_t *fprocp, ftd_sock_t *listener,
	int lgnum, int cpon, char **envp);
extern int ftd_proc_exec_throt(ftd_proc_t *fprocp, int wait, char **envp);
extern int ftd_proc_exec(char *command, int wait);
extern int ftd_proc_terminate(char *procname);
extern int ftd_proc_kill(char *command);
extern int ftd_proc_kill_all_pmd(LList *proclist);
extern int ftd_proc_kill_all_rmd(LList *proclist);
extern int ftd_proc_kill_all_apply(LList *proclist);
extern int ftd_proc_kill_all(LList *proclist);
extern ftd_proc_t *ftd_proc_lgnum_to_proc(LList *proclist, int lgnum,
	int role);
extern ftd_proc_t *ftd_proc_pid_to_proc(LList *proclist, pid_t pid);
extern ftd_proc_t *ftd_proc_name_to_proc(LList *proclist, char *procname);
extern pid_t ftd_proc_get_pid(char *procname, int exactmatch, int *pcnt);

extern int ftd_proc_daemon_init(void); 

extern int ftd_proc_process_dev_info_request(ftd_sock_t *fsockp, char *msg);
extern int ftd_proc_process_proc_request(ftd_sock_t *fsockp, char *msg);
extern int ftd_proc_do_command(ftd_sock_t *fsockp, char *msg);
extern int ftd_proc_hup_pmds(LList *proclist, ftd_sock_t *fsockp,
	int consleep, char **envp);

#if defined(_WINDOWS)
	ftd_proc_process_dev_info_request(ftd_sock_t *fsockp, char *msg);
#endif

#if !defined(_WINDOWS)
extern int ftd_proc_daemon_init(void); 
extern int ftd_proc_reaper(LList *proclist, ftd_sock_t *fsockp,
	char **envp);
#else
extern int ftd_proc_reaper(LList *proclist, ftd_proc_t **fprocpp, ftd_sock_t *fsockp, char **envp);
#endif

#endif
