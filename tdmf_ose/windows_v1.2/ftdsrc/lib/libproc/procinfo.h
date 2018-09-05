#if !defined (_WINDOWS)

/*
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

#ifndef _PROCINFO_H
#define _PROCINFO_H_

typedef struct proc_info_s {
	char name[256];
	int pid;
} proc_info_t;

void del_proc_names(proc_info_t***, int);
int get_proc_names(char*, int, proc_info_t***);
int capture_proc_names(char*, int, char**);
int process_proc_request(int, char*);

#endif

#endif

