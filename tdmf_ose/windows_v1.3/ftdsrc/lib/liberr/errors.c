/*
 * errors.c - error log interface
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#if defined(_WINDOWS)
#include <tchar.h>
#endif

// Mike Pollett
#include "../../tdmf.inc"

#include "errors.h"
#include "ftd_error.h"
#include "ftd_cfgsys.h"
#include "errmsg_list.h"
#include "../libResMgr/ResMgr.h"

#include "assert.h"

// global variables
char	*err_glb_fnm;
int	    err_glb_lnum;
unsigned char gucTraceLevel = 0;

// Use with DebugView utility
void DebugTrace(char* pcpMsg, ... )
{
	va_list     lVaLst;
	static char lscaMsg [128] = {0};

	va_start ( lVaLst, pcpMsg );
	_vsnprintf ( lscaMsg, 128, pcpMsg, lVaLst );

	OutputDebugString(lscaMsg);
}

// To replace DPRINTF()
void error_tracef ( unsigned char pucLevel, char* pcpMsg, ... )
{
	int         liErrType;
	va_list     lVaLst;
	char        lscaMsg [ MAXERR + 1 ];

	if ( pucLevel > gucTraceLevel )
	{
		return;
	}
	else switch ( pucLevel )
	{
		case 0:
			return;
		case 1:
			liErrType  = EVENTLOG_ERROR_TYPE;
            strcpy ( lscaMsg, "ERR:" );
			break;
		case 2:
			liErrType  = EVENTLOG_WARNING_TYPE;
            strcpy ( lscaMsg, "WRN:" );
			break;
		default:
			liErrType  = EVENTLOG_INFORMATION_TYPE;
            strcpy ( lscaMsg, "INF:" );
			break;
	}

	va_start ( lVaLst, pcpMsg );
	_vsnprintf ( lscaMsg+4, MAXERR-4, pcpMsg, lVaLst );

#if defined( _TLS_ERRFAC )
	if ( TlsGetValue( TLSIndex ) )
#else
    if (ERRFAC)
#endif
   {
	error_syslog( ERRFAC, liErrType, lscaMsg );
    }
    else
    {
        assert(ERRFAC);
    }

} // error_tracef ()

//same as error_tracef () but no variables args and no prefix added
void error_tracef_msg ( unsigned char pucLevel, char* pcpMsg )
{
	int         liErrType;

	if ( pucLevel > gucTraceLevel )
	{
		return;
	}
	else switch ( pucLevel )
	{
		case 0:
			return;
		case 1:
			liErrType  = EVENTLOG_ERROR_TYPE;
			break;
		case 2:
			liErrType  = EVENTLOG_WARNING_TYPE;
			break;
		default:
			liErrType  = EVENTLOG_INFORMATION_TYPE;
			break;
	}

	error_syslog( ERRFAC, liErrType, pcpMsg );

} // error_tracef_msg () 

// Do Not Use.
// Temporary to help migrate from DBPRINT() to error_tracef()
void error_printf ( errfac_t* ppErrFac, int piPriority, char* pcpMsg, ...)
{
	va_list     lVaLst;
	char        lscaMsg [ MAXERR ];

	switch ( piPriority )
	{
		case EVENTLOG_ERROR_TYPE:
			if ( gucTraceLevel < 1 )
			{
				return;
			}
            break;
		case EVENTLOG_WARNING_TYPE:
			if ( gucTraceLevel < 2 )
			{
				return;
			}
			break;
		case EVENTLOG_INFORMATION_TYPE:
			if ( gucTraceLevel < 3 )
			{
				return;
			}
			break;
		default:
			return;
	}

	va_start ( lVaLst, pcpMsg );
	_vsnprintf ( lscaMsg, MAXERR, pcpMsg, lVaLst );

	error_syslog( ppErrFac, piPriority, lscaMsg );

} // error_printf ()


extern void error_SetTraceLevel ( unsigned char pucLevel )
{
	gucTraceLevel = pucLevel;

} // error_SetTraceLevel ()


/*
 * errfac_create --
 * create an ftd_errfac_t object
 */
errfac_t *
errfac_create
(char *facility, char *procname, char *msgdbpath, char *logpath,
	int reportwhere, int logstderr)
{
	errfac_t *errfac;
#if defined(_WINDOWS)
	char errorloglevel[256];
#endif
	char szProductName[256];

	if ((errfac = (errfac_t*)calloc(1, sizeof(errfac_t))) == NULL) {
		return NULL;
	}

	if ((errfac->last = (err_msg_t*)calloc(1, sizeof(err_msg_t))) == NULL) {
		return NULL;
	}

	/* Re-branding */
	/* Get product name from resource dll */
	GET_FULLPRODUCTNAME_FROM_RESMGR(szProductName, 256);
	if (strlen(szProductName) > 0)
	{
		strcpy(errfac->facility, szProductName);
	}
	else
	{
		strcpy(errfac->facility, facility);
	}
	strcpy(errfac->procname, procname);
	strcpy(errfac->msgdbpath, msgdbpath);
	strcpy(errfac->logpath, logpath);

	errfac->reportwhere = reportwhere;

#if defined(_AIX)
	errfac->reportwhere = 1;
#elif defined(_WINDOWS)
	if ((cfg_get_software_key_value("errorloglevel", errorloglevel,
		CFG_IS_NOT_STRINGVAL)) == CFG_OK)
		errfac->reportwhere = 1;
	else
		errfac->reportwhere = 0;

#endif /* defined(_AIX) */
	
	errfac->logstderr = logstderr;

	if (errfac_init(errfac) < 0) {
		return NULL;
	}
	
	return errfac;
}

/*
 * errfac_delete --
 * delete an ftd_errfac_t object
 */
int
errfac_delete(errfac_t *errfac)
{
    if (errfac == NULL)
        return -1;

#if !defined(_WINDOWS)
	if (errfac->logfd)
		fclose(errfac->logfd);
#endif

	if (errfac->last)
		free(errfac->last);
	if (errfac->emsgs)
		free(errfac->emsgs);
	
	free(errfac);

	return 0;
}

#if defined(_WINDOWS)
/*
 * FUNCTION: GetLastErrorText
 *
 * PURPOSE: copies error message text to string
 *
 * PARAMETERS:
 *	lpszBuf - destination buffer
 *	dwSize - size of buffer
 *
 * RETURN VALUE:
 *	destination buffer
 *
 * COMMENTS:
 */
LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize )
{
    DWORD dwRet;
    LPTSTR lpszTemp = NULL;

    dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_IGNORE_INSERTS,
                           NULL,
                           GetLastError(),
                           LANG_NEUTRAL,
                           (LPTSTR)&lpszTemp,
                           0,
                           NULL );

    // supplied buffer is not long enough
    if ( !dwRet || ( (long)dwSize < (long)dwRet+14 ) )
        lpszBuf[0] = TEXT('\0');
    else
    {
        lpszTemp[lstrlen(lpszTemp)-2] = TEXT('\0');  //remove cr and newline character
        _stprintf( lpszBuf, TEXT("%s (0x%x)"), lpszTemp, GetLastError() );
    }

    if ( lpszTemp )
        LocalFree((HLOCAL) lpszTemp );

    return lpszBuf;
}

/*
 * error_syslog --
 * log the message to the NT system log
 */
void
error_syslog(errfac_t *errfac, int priority, char *f, ...)
{
	va_list ap;
    int     rtnval,i=0;
    char    szMsg[MAXERR];
    HANDLE  hEventSource;
    LPSTR   lpszStrings[1];
	LPVOID	lpRawData = NULL;

#if defined( _TLS_ERRFAC )
	errfac = TlsGetValue( TLSIndex );
#endif

	//
	// Use event logging to log the error.
    //
    hEventSource = RegisterEventSource(NULL, errfac->facility);

    va_start(ap, f);
	rtnval = _vsnprintf(szMsg, MAXERR, f, ap);
    va_end(ap);

    //szMsg[rtnval - 1] = '\0'; // get rid of the LF

#ifdef TDMF_TRACE
	if (rtnval)
	{
		while (szMsg[i])
		{
			if 	(szMsg[i]=='\n')
					szMsg[i]= ' ';
			i++;
		}
	}
#else
    if ( szMsg[rtnval - 1] == '\n' )
        szMsg[rtnval - 1] = '\0'; // get rid of the LF
#endif

    lpszStrings[0] = szMsg;

    if (hEventSource != NULL) {
        ReportEvent(hEventSource, // handle of event source
            (WORD)priority,             // event type
            0,                    // event category
            FTD_SIMPLE_MESSAGE,   // event ID
            NULL,                 // current user's SID
            1,                    // strings in lpszStrings
            0,                    // no bytes of raw data
            lpszStrings,          // array of error strings
            lpRawData);           // no raw data

        (VOID) DeregisterEventSource(hEventSource);
    }

    //add to global msg list
    error_msg_list_addMessage(priority,szMsg);

}

void
error_syslog_DirectAccess(char* ApplicationName, char *f, ...)
{
	va_list ap;
    int     rtnval,i=0;
    char    szMsg[MAXERR];
    HANDLE  hEventSource;
    LPSTR   lpszStrings[1];

	//
	// Use event logging to log the error.
    //
    hEventSource = RegisterEventSource(NULL, ApplicationName);

    va_start(ap, f);
	rtnval = _vsnprintf(szMsg, MAXERR, f, ap);
    va_end(ap);

    //szMsg[rtnval - 1] = '\0'; // get rid of the LF

#ifdef TDMF_TRACE
	if (rtnval)
	{
		while (szMsg[i])
		{
			if 	(szMsg[i]=='\n')
					szMsg[i]= ' ';
			i++;
		}
	}
#else
    if ( szMsg[rtnval - 1] == '\n' )
        szMsg[rtnval - 1] = '\0'; // get rid of the LF
#endif

    lpszStrings[0] = szMsg;

    if (hEventSource != NULL) {
        ReportEvent(hEventSource,       // handle of event source
            EVENTLOG_INFORMATION_TYPE,  // event type
            0,                          // event category
            FTD_SIMPLE_MESSAGE,         // event ID
            NULL,                       // current user's SID
            1,                          // strings in lpszStrings
            0L,                         // no bytes of raw data
            lpszStrings,                // array of error strings
            NULL     );                 // no raw data

        (VOID) DeregisterEventSource(hEventSource);
    }
}
#endif /* _WINDOWS */

/*
 * error_log_stderr --
 * tells the error system to always log to stderr
 */
void
error_log_stderr(errfac_t *errfac)
{
	errfac->logstderr = 1;
}

/*
 * error_logger --
 * writes an error message to the error log file
 */
static void
error_logger(errfac_t *errfac, char *msglevel, char *name, char *msg,
	char *retmsg)
{
	struct tm	*tim;
	time_t		t;
	int			i;

#if !defined(_WINDOWS)
	pid_t		ppid = getppid();

	if (errfac->logfd == NULL) {
		return;
	}
	
	if (ftell(errfac->logfd) >= 32768) {
		fflush(errfac->logfd);
		fclose(errfac->logfd);
		unlink(errfac->logname1);
		rename(errfac->logname, errfac->logname1);
		
		if ((errfac->logfd = fopen(errfac->logname, "w")) == NULL) {
			return;
		}
	}
#endif	

	(void)time(&t);
	tim = localtime(&t);
	i = strlen(msg) - 1;
	while ((!isprint(msg[i])) || msg[i] == ' ') {
		msg[i] = '\0';
		i--;
	}

	if (errfac->reportwhere) {
		sprintf(retmsg, 
#if !defined(_WINDOWS)
	"[%04d/%02d/%02d %02d:%02d:%02d] %s: [%s:%s] [%s / %s] [ppid: %d] [src,line: %s, %d] %s\n", 
#else
	"[%04d/%02d/%02d %02d:%02d:%02d] %s: [%s:%s] [%s / %s] [src,line: %s, %d] %s\n", 
#endif
			(1900+tim->tm_year), (1+tim->tm_mon), tim->tm_mday,
			tim->tm_hour, tim->tm_min, tim->tm_sec, 
			errfac->facility,
			errfac->hostname,
			errfac->procname,
			msglevel, name,	
#if !defined(_WINDOWS)
			ppid, 
#endif
			err_glb_fnm, err_glb_lnum,
			msg);
	} else {
		sprintf(retmsg, 
			"[%04d/%02d/%02d %02d:%02d:%02d] %s: [%s:%s] [%s / %s] %s\n", 
			(1900+tim->tm_year), (1+tim->tm_mon), tim->tm_mday,
			tim->tm_hour, tim->tm_min, tim->tm_sec, 
			errfac->facility,
			errfac->hostname,
			errfac->procname,
			msglevel, name,	
			msg);
	}


#if !defined(_WINDOWS)
	fprintf(errfac->logfd, "%s", retmsg);
	fflush(errfac->logfd);
#endif

#if defined(DEBUG) && !defined(_WINDOWS)
	fprintf(errfac->dbgfd, "%s", retmsg);
	fflush(errfac->dbgfd);
#endif

	return;
}


/*
 * error_format_datetime --
 * combine date-time , errfac_t information, message level, name, text into one text string.
 * same formatting as done in error_logger()
 */
void error_format_datetime(const errfac_t *errfac, const char *msglevel, const char *name, const char *msg,
	char *retmsg)
{
	struct tm	*tim;
	time_t		t;
	int			i;

#if defined( _TLS_ERRFAC )
	errfac = TlsGetValue( TLSIndex );
#endif


	(void)time(&t);
	tim = localtime(&t);

	sprintf(retmsg, 
		"[%04d/%02d/%02d %02d:%02d:%02d] %s: [%s:%s] [%s / %s] %s\n", 
		(1900+tim->tm_year), (1+tim->tm_mon), tim->tm_mday,
		tim->tm_hour, tim->tm_min, tim->tm_sec, 
		errfac->facility,
		errfac->hostname,
		errfac->procname,
		msglevel, name,	
		msg);

    //cut off trailing spaces
	i = strlen(retmsg) - 1;
	while ((!isprint(retmsg[i])) || retmsg[i] == ' ') {
		retmsg[i] = '\0';
		i--;
	}
}

/*
 * errfac_init --
 * initialize ftd_errfac_t object
 */
static int
errfac_init(errfac_t *errfac) 
{
#ifdef DEBUG
	char		tracefile[256];
#endif 
	char		line[256], facility[32];
	FILE		*emsgfd;
	int			i, len;
	struct stat	statbuf;
	emsg_t		emsg;

    /* by default we attempt to write to log files */
	errfac->logerrs = 1;  

#if !defined(_WINDOWS)
	openlog(errfac->facility, LOG_PID, LOG_DAEMON);
	setlogmask(LOG_UPTO(LOG_INFO));
#endif
#if defined(DEBUG) && !defined(_WINDOWS)
	sprintf(tracefile, "/tmp/%s.trace", errfac->facility);
	if ((errfac->dbgfd = fopen(tracefile, "w")) == NULL) {
		fprintf(stderr,"\nCouldn't open error trace file: %s\n",
			tracefile);
		return -1;
	}
#endif

	memset(errfac->hostname, 0, sizeof(errfac->hostname));
	gethostname(errfac->hostname, sizeof(errfac->hostname));

	/* open the error log */
	i = 0;
	while (errfac->facility[i]) {
		facility[i] = tolower(errfac->facility[i]);
		i++;
	}
	facility[i] = 0;
#if defined(_WINDOWS)
	sprintf(errfac->logname, "%s\\%serror.log", errfac->logpath, facility);
	sprintf(errfac->logname1, "%s\\%serror1.log", errfac->logpath, facility);
#else	
	sprintf(errfac->logname, "%s/%serror.log", errfac->logpath, facility);
	sprintf(errfac->logname1, "%s/%serror1.log", errfac->logpath, facility);
#endif
	if (0 == stat(errfac->logname, &statbuf)) {
		if (statbuf.st_size >= 32768) {
			(void)unlink(errfac->logname1);
			(void)rename(errfac->logname, errfac->logname1);
		}
	}
#if !defined(_WINDOWS)
	if ((errfac->logfd = fopen(errfac->logname, "a")) == NULL) {
		errfac->logerrs = 0;  /* Turn off writes to log files */
		sprintf(line,"\nCouldn't open error log file: %s\n",
			 errfac->logname);
		fprintf(stderr, line);
		error_syslog(errfac, LOG_CRIT, "%s", line);
		return -1; 
	}
#endif		

	/* read the message file into memory */
	emsgfd = fopen(errfac->msgdbpath, "r");
	if (emsgfd == NULL) {
		errfac->logerrs = 0;  /* Turn off writes to log files */
		sprintf(line, "%s: fatal error: cannot open %s\n",
			errfac->facility, errfac->msgdbpath);
		fprintf(stderr, line);
#if defined(_WINDOWS)
#if defined( _TLS_ERRFAC )
		TlsSetValue( TLSIndex, errfac );
#endif
		error_syslog(errfac, LOG_CRIT, "%s", line);
#endif		
		return -1;
	}

	errfac->emsgs = (emsg_t*)calloc(FTD_INITERRCNT, sizeof(emsg_t));
	errfac->errcnt = 0;

	while (!feof(emsgfd)) {
		(void)fgets(line, 160, emsgfd);
		len = strlen(line);
		if (len < 5 || line[0] == '#') {
			continue;
		}
		i = 0;
		emsg.name[i] = '\0';
		while (i<len && i < 15 && line[i] != ' ' && line[i] != '\t') {
			i++;
		}
		strncpy(emsg.name, line, i);
		emsg.name[i] = '\0';
		while (i<len && (line[i] == ' ' || line[i] == '\t')) {
			i++;
		}
		len = len - i;
		if (len > 159) {
			len = 159;
		}
		strncpy(emsg.msg, &line[i], len);
		emsg.msg[len] = '\0';

#if defined(_WINDOWS)
		{
			char *str = emsg.msg;

			while ( (str = strstr(str, "%llu")) != NULL) {
				memmove(str + 1, str, strlen(str));
				memcpy(str, "%I64u", 5);
			}

			str = emsg.msg;

			while ( (str = strstr(str, "daemon")) != NULL) {
				memcpy(str, "thread", 6);
			}

			str = emsg.msg;

			while ( (str = strstr(str, "Daemon")) != NULL) {
				memcpy(str, "Thread", 6);
			}

			str = emsg.msg;

			while ( (str = strstr(str, "throtd")) != NULL) {
				memcpy(str, "statd ", 6);
			}

			str = emsg.msg;
			{
				size_t qnmlen = strlen(CMDPREFIX);

				while ( (str = strstr(str, "CMDPREFIX\\")) != NULL) 
                {
					memcpy(str, CMDPREFIX, qnmlen);
					if (qnmlen < strlen("CMDPREFIX\\")) 
                    {
						memmove(str + qnmlen, str + strlen("CMDPREFIX\\"), strlen(str) - strlen("CMDPREFIX\\") + 1 /*\0*/);
					}
				}
			}
		}
#endif
        memcpy(&errfac->emsgs[errfac->errcnt++], &emsg, sizeof(emsg_t));	

		if (errfac->errcnt >= FTD_INITERRCNT) {
			errfac->emsgs = (emsg_t*)realloc(errfac->emsgs,
				sizeof(emsg_t) * (errfac->errcnt + 50));
		}
	}
	fclose(emsgfd);

	return 0;
}

/*
 * errfac_get_errmsg --
 * retrieve an message string based on the facility and name
 */
int
errfac_get_errmsg(errfac_t *errfac, char *name, char *msg)
{
	emsg_t	*emsgp;
	int		i;

#if defined( _TLS_ERRFAC )
	errfac = TlsGetValue( TLSIndex );
#endif

	for (i = 0; i < errfac->errcnt; i++) {
		emsgp = &errfac->emsgs[i];
		if (0 == strcmp(name, emsgp->name)) {
			strcpy(msg, emsgp->msg);
			return 0;
		}
	}
	sprintf(msg,
		"UNKNOWN ERROR MESSAGE - FACILITY= %s NAME=%s",
		errfac->facility, name);
    
	return 1;
}

/*
 * errfac_log_errmsg --
 * log an error message to the syslog facility
 */
void
errfac_log_errmsg(errfac_t *errfac, int level, char *name, char *errmsg)
{
	int		priority;
	char	msglevel[25], retmsg[MAXERR];

#if defined( _TLS_ERRFAC )
	errfac = TlsGetValue( TLSIndex );
#endif

	switch(level) {
	case ERRINFO:
		priority = LOG_INFO;
		strcpy(msglevel, "INFO");
		break;
	case ERRWARN:
		priority = LOG_WARNING;
		strcpy(msglevel, "WARNING");
		break;
	case ERRCRIT:
		priority = LOG_CRIT;
		strcpy(msglevel, "FATAL");
		break;
	default:
		priority = LOG_ERR;
		strcpy(msglevel, "UNKNOWN");
	}

	error_logger(errfac, msglevel, name, errmsg, retmsg);

#ifdef _WINDOWS
	error_syslog(errfac, priority, "%s", retmsg);
	if (errfac->logstderr) {
		strcat(retmsg, "\n");
		fprintf(stderr, "%s", retmsg);
    }
#else
	syslog(priority, "%s", retmsg);
	if (errfac->logstderr || isatty(0)) {
        strcat(retmsg, "\n");
        write(2, retmsg, strlen(retmsg));
    }
#endif

	errfac->last->level = level;
	strcpy(errfac->last->msg, retmsg);

	return;
}

/*
 * _reporterr --
 * report an error
 */
void
_reporterr(errfac_t *errfac, char* name, int level, ...)
{
	va_list	args;
	char	fmt[MAXERR], msg[MAXERR];

#if defined( _TLS_ERRFAC )
	errfac = TlsGetValue( TLSIndex );
#endif

	va_start(args, level);

	if (0 == errfac_get_errmsg(errfac, name, fmt)) {
		vsprintf(msg, fmt, args);
		if (errfac->logerrs) { 
			errfac_log_errmsg(errfac, level, name, msg);
		}
	} else {
		if (errfac->logerrs) {
			errfac_log_errmsg(errfac, ERRWARN, name, fmt);
		}
    }

	return;
}

/*
 * get_log_msgs --
 * return new error log messages since last call to this func.
 */
void 
get_log_msgs
(errfac_t *errfac, char** msgbuf, int* msgsize, int* nummsgs, int* offset)
{
    struct stat statbuf;
    FILE* fd;
    int nlcnt;
    int i;
    long newlogsize;
    int len;
    char elogfilename[MAXPATH];

#if defined( _TLS_ERRFAC )
	errfac = TlsGetValue( TLSIndex );
#endif

    nlcnt = 0;
    if (0 == stat(errfac->logname, &statbuf)) {
        newlogsize = statbuf.st_size - 1;
    } else {
        newlogsize = 0;
    }
    if (*offset < 0) {
        *offset = newlogsize;
        *msgbuf = (char*) NULL;
        *nummsgs = 0;
        *msgsize = 0;
        return;
    }      
    if (newlogsize < *offset) {
        *offset = 0;
        /* -- do we want to deal with ftderror.log.1 here? */
    }
    len = (newlogsize - *offset) + 2;
    *msgbuf = (char*) malloc (len);
    fd = fopen (elogfilename, "r");
    if (fd != (FILE*)NULL) {
        /* -- read from here to end of file */
        (void) fseek (fd, (*offset + 1), SEEK_SET);
        len = newlogsize - (*offset+1);
        len = fread ((void*) *msgbuf, (size_t) 1, (size_t) len, fd); 
        (void) fclose (fd);
    }
    for (i=0; i<len; i++) {
        if ((*msgbuf)[i] == '\0') {
            len = i;
            break;
        }
        if ((*msgbuf)[i] == '\n') nlcnt++;
    }
    *msgsize = len;
    *nummsgs = ((nlcnt-1)>0)?(nlcnt-1):0;
    *offset = newlogsize;

    return;
}
