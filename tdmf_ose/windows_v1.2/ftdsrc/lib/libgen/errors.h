/***************************************************************************
 * errors.h - DataStar Daemon error management module
 *
 * (c) Copyright 1996, 1997, 1998 FullTime Software, Inc. All Rights Reserved
 *
 * History:
 *   10/07/96 - Steve Wahl - original code
 *
 ***************************************************************************
 */

#ifndef _ERRORS_H
#define _ERRORS_H 1

#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#ifdef EXTERN
#undef EXTERN
#ifdef MAIN
#define EXTERN
#else
#define EXTERN extern
#endif
#endif /* EXTERN */

/* Error messages, automatically generated from errors.msg */
#include "errmsg.h"

#ifdef DEBUG
extern FILE* dbgfd;
#endif

extern char *default_error_facility;
extern int log_errs;  

#define ERRFAC default_error_facility

#define ERRINFO 0
#define ERRWARN 1
#define ERRCRIT 2

extern int initerrmgt (char* facility);
extern int geterrmsg (char* facility, char* name, char* msg);
extern void logerrmsg (char* facility, int level, char* name, char* errmsg);

/* list file in line number in error displays...  */

#if defined(_AIX)
#define reporterr_where
#endif /* defined(_AIX) */

#if defined(reporterr_where)
extern char *err_glb_fnm;
extern int err_glb_lnum;

#define reporterr \
	err_glb_fnm = __FILE__; \
	err_glb_lnum= __LINE__; \
	_reporterr
        
#else /* defined(reporterr_where) */

#define reporterr _reporterr

#endif /* defined(reporterr_where) */

#ifndef GCC_CHECKING
extern void _reporterr (char* facility, char* name, int level, ...);
#else
extern void _reporterr (char* facility, char* name, int level, ...)
    __attribute__((format (printf, 2, 4)));
#endif
extern void logstderr (int flag);
extern void getlogmsgs (char** msgbuf, int* msgsize, int* nummsgs, 
			int* offset);
#endif /* _ERRORS_H */

