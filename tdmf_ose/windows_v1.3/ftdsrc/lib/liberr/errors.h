/*
 * errors.h - error log interface
 *
 * Copyright (c) 1999 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#ifndef _ERRORS_H
#define _ERRORS_H

/* Error messages, automatically generated from errors.msg */

#include <stdio.h>

#ifdef _WINDOWS
#include <windows.h>
#include "..\..\LogMsg.h"
#else
#include <syslog.h>
#endif

#ifndef MAXPATH
#ifdef _WINDOWS
#define MAXPATH         260 //same logic as in ftd_port_win.h - see MAX_PATH in windef.h
#else
#error properly define MAXPATH here 
#endif
#endif

#define FTD_INITERRCNT  256
#define MAXERR          512 

//#define TDMF_TRACE

typedef struct emsg {
    char name[32];
    char msg[MAXERR];
} emsg_t;

typedef struct err_msg_s {
    int level;
    char msg[MAXERR];
} err_msg_t;

typedef struct errfac_s {
    int magicvalue;                 /* magic number for init determination */
    err_msg_t   *last;              /* last reported error message */
    char facility[MAXPATH];         /* error facility name */
    char msgdbpath[MAXPATH];        /* error message database path */
    char logpath[MAXPATH];          /* error log path */
    char logname[MAXPATH];          /* current log file name */
    char logname1[MAXPATH];         /* previous log file name */
    char hostname[MAXPATH];         /* host name creating this object */
    char procname[MAXPATH];         /* process name creating this object */
    FILE *dbgfd;                    /* trace file handle */
    FILE *logfd;                    /* error log file handle */
    int logerrs;                    /* log errors flag */
    int logstderr;                  /* log errors to stderr ? */
    int reportwhere;                /* report source file, line */
    int errcnt;                     /* # errors in list */
    emsg_t *emsgs;                  /* list of error msgs */
} errfac_t;

#define EMSGFTD 0
#define EMSGUNKNOWN 1

#define ERRINFO 0
#define ERRWARN 1
#define ERRCRIT 2

#define TRACEERR   1
#define TRACEWRN   2
#define TRACEINF   3
#define TRACEINF4  4
#define TRACEINF5  5
#define TRACEINF6  6

#define TRACECMT  10

#ifdef _WINDOWS
#define LOG_CRIT    EVENTLOG_ERROR_TYPE
#define LOG_WARNING EVENTLOG_WARNING_TYPE
#define LOG_INFO    EVENTLOG_INFORMATION_TYPE
#define LOG_ERR     EVENTLOG_WARNING_TYPE
#endif

#ifdef __cplusplus
extern "C"{ 
#endif

extern errfac_t *errfac_create(char *facility, char *procname, char *msgdbname, char *logpath,
                               int reportwhere, int logstderr);
extern int errfac_delete(errfac_t *errfac);
extern int errfac_init(errfac_t *errfac);
extern int errfac_get_errmsg(errfac_t *errfac, char *name, char *msg);
extern void errfac_log_errmsg(errfac_t *errfac, int level, char *name, char *msg);
static void error_logger(errfac_t *errfac, char *msglevel,  char *name, char* msg, char *retmsg);
extern void error_format_datetime(const errfac_t *errfac, const char *msglevel, const char *name, const char *msg, char *retmsg);
extern void error_syslog(errfac_t *errfac, int priority, char *f, ...);
extern void error_syslog_DirectAccess(char* ApplicationName, char *f, ...);
extern void error_tracef        ( unsigned char pucLevel, char* pcpMsg, ... );
// Ex.: error_tracef( TRACEINF, "TheCurrentFunctionName():The infamous message %s, %d", lcpAnyMsg, liAnyMsg );
extern void error_tracef_msg    ( unsigned char pucLevel, char* pcpMsg );
extern void error_printf        ( errfac_t* ppErrFac, int piPriority, char* pcpMsg, ...);
extern void error_SetTraceLevel ( unsigned char pucevel );
//Use with DebugView
extern void DebugTrace        ( char* pcpMsg, ... );
#ifdef __cplusplus 
}
#endif

extern char *err_glb_fnm;
extern int  err_glb_lnum;

#define reporterr err_glb_fnm = __FILE__; err_glb_lnum = __LINE__; _reporterr

#ifndef GCC_CHECKING
extern void _reporterr(errfac_t *errfac, char* name, int level, ...);
#else
extern void _reporterr(errfac_t *errfac, char* name, int level, ...)
    __attribute__((format (printf, 2, 4)));
#endif

extern void errfac_log_stderr(errfac_t *errfac);
extern void get_log_msgs(errfac_t *errfac,
    char** msgbuf, int* msgsize, int* nummsgs,int *offset);

#if defined(_WINDOWS)
extern LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize );
#endif

#endif /* _ERROR_H */

