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

#ifdef LDEBUG
extern FILE* dbgfd;
#endif

extern char *default_error_facility;
extern int log_errs;  
extern int log_internal;
#define LOGINTERNAL "LOG_INTERNAL"
#define SAVEJOURNALS "SAVE_JOURNALS"

#define ERRFAC default_error_facility

#define ERRINFO 0
#define ERRWARN 1
#define ERRCRIT 2

extern int initerrmgt (char* facility);
extern void setsyslog_prefix(const char *fmt, ...);
extern int geterrmsg (char* facility, char* name, char* msg);
extern void _logerrmsg (char *filename, int lineno, char* facility, int level, char* name, char* errmsg);

/* list file in line number in error displays...  */

#define reporterr(facility, name, ...) _reporterr(__FILE__, __LINE__, facility, name, __VA_ARGS__)
#define logerrmsg(facility, level, ...) _logerrmsg(__FILE__, __LINE__, facility, level, __VA_ARGS__)

#ifndef GCC_CHECKING
extern void _reporterr (char *filename, int lineno, char* facility, char* name, int level, ...);
#else
extern void _reporterr (char *filename, int lineno, char* facility, char* name, int level, ...)
    __attribute__((format (printf, 2, 4)));
#endif
extern const char *get_error_str(int err);
extern void logstderr (int flag);
extern void getlogmsgs (char** msgbuf, int* msgsize, int* nummsgs, 
			int* offset);
extern void log_command (int argc, char** argv);
#endif /* _ERRORS_H */

