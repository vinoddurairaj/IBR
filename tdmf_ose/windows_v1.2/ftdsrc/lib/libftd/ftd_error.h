/*
 * ftd_error.h - ftd library error reporting interface
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
#ifndef _FTD_ERROR_H_
#define _FTD_ERROR_H_

#include "errors.h"
#include "errmsg.h"

#if defined(_WINDOWS)

extern __declspec( thread ) errfac_t *ERRFAC;
#else
extern errfac_t *ERRFAC;
#endif

#define ftd_get_last_error(ef)	(ef)->last

#define	FTDERRMAGIC	0xbadd00d

errfac_t *ftd_init_errfac(char *facility, char *procname, char *msgdbpath, char *logpath,
						  int reportwhere, int logstderr);
int ftd_delete_errfac(void);

#if defined(_WINDOWS)

extern int ftd_errno(void);
extern char *ftd_strerror(void);
extern int ftd_lockmsgbox(char *drive);

#else

#define INVALID_HANDLE_VALUE (HANDLE)-1

extern int ftd_errno(void);
#define ftd_strerror() strerror(errno)

#endif




// Please from now on, Use the error_tracef() function instead 
// of the DPRINTF() macro
#define DPRINTF(a)    error_printf a

#endif

