/***************************************************************************
 * errors.c - FullTime Data Daemon error management module
 *
 * (c) Copyright 1996, 1997 FullTime Software, Inc. All Rights Reserved
 *
 * History:
 *   10/07/96 - Steve Wahl - original code
 *
 ***************************************************************************
 */
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "config.h"
#include "errors.h"
#include "pathnames.h"
#include <unistd.h>

#define EMSGFTD 0
#define EMSGUNKNOWN 1

typedef struct emsg {
    char name[16];
    char msg[256];
} emsg_t;

#ifdef DEBUG
FILE *dbgfd;
#endif

char *default_error_facility = CAPQ;
int  log_errs = 1; 

static int nummsgs = 0;
static emsg_t emsgs[256];
static int always_log_stderr = 0;
static FILE* errlogfd = 0;
static char errlogpath[FILE_PATH_LEN];
static char errlogpath2[FILE_PATH_LEN];
static void ftderrorlogger (char* msg);

extern char *argv0;

/****************************************************************************
 * logstderr -- tells the error system to always log to stderr
 ***************************************************************************/
void
logstderr(int flag)
{
    always_log_stderr = flag;
}

/****************************************************************************
 * ftderrorlogger -- writes an error message to the error log
 ***************************************************************************/

#if defined(reporterr_where)
char *err_glb_fnm;
int   err_glb_lnum;
#endif /* defined(reporterr_where) */

static void
ftderrorlogger (char* string)
{
    struct tm *tim;
    time_t t;
    int i;

#if defined(reporterr_where)
    extern char *argv0;
    int pid=getpid();
    int ppid=getppid();
#endif /* defined(reporterr_where) */
  
    if (errlogfd == NULL) {
        return;
    }
    if (ftell(errlogfd) >= 32768) {
        fflush (errlogfd);
        fclose (errlogfd);
        unlink (errlogpath2);
        rename (errlogpath, errlogpath2);
        if ((errlogfd = fopen (errlogpath, "w")) == NULL) {
            if (errno == EMFILE)
                reporterr(ERRFAC, M_FILE, ERRCRIT, errlogpath, strerror(errno));
            exit (1);
        }
    }
    (void) time(&t);
    tim = localtime(&t);
    i = strlen(string) - 1;
    while ((!isprint(string[i])) || string[i] == ' ') {
        string[i] = '\0';
        i--;
    }
    fprintf (errlogfd, 
#if defined(reporterr_where)
"[%04d/%02d/%02d %02d:%02d:%02d] [proc: %s] [pid,ppid: %d, %d] [src,line: %s, %d] %s\n", 
#else  /* defined(reporterr_where) */
        "[%04d/%02d/%02d %02d:%02d:%02d] %s\n", 
#endif /* defined(reporterr_where) */
        (1900+tim->tm_year), (1+tim->tm_mon), tim->tm_mday,
        tim->tm_hour, tim->tm_min, tim->tm_sec, 
#if defined(reporterr_where)
        argv0, pid, ppid, err_glb_fnm, err_glb_lnum,
#endif /* defined(reporterr_where) */
        string);
    fflush (errlogfd);
}

/****************************************************************************
 * initerrmgt -- initialize error and message management and reporting
 * returns: 0 - Ok, -1 - writes to log files disabled, failed open.
 ***************************************************************************/
int
initerrmgt (char* facility) 
{
#ifdef TDMF_TRACE
    char tracefile[256];
#endif 
    char msgfilepath[256];
    char line[256];
    FILE* emsgfd;
    int i, len;
    struct stat statbuf;

    /* by default we attempt to write to log files */
    log_errs = 1;  
    openlog (facility, LOG_PID, LOG_DAEMON);
    setlogmask(LOG_UPTO(LOG_INFO));
#ifdef TDMF_TRACE
    sprintf (tracefile, "/tmp/%s.trace", facility);
    if ((dbgfd = fopen (tracefile, "w")) == NULL) {
        if (errno == EMFILE)
            reporterr(ERRFAC, M_FILE, ERRCRIT, tracefile, strerror(errno));
        fprintf(stderr,"\nCouldn't open error trace file: %s", tracefile);
        return (-1);
    }
#endif
    /* -- open the error log */
    sprintf (errlogpath, "%s/" QNM "error.log", PATH_RUN_FILES);
    sprintf (errlogpath2, "%s/" QNM "error.log", PATH_RUN_FILES);
    if (0 == stat(errlogpath, &statbuf)) {
        if (statbuf.st_size >= 32768) {
            (void) unlink (errlogpath2);
            (void) rename (errlogpath, errlogpath2);
        }
    }
    if ((errlogfd = fopen (errlogpath, "a")) == NULL) {
        log_errs = 0;  /* Turn off writes to log files */
        if (errno == EMFILE)
            reporterr(ERRFAC, M_FILE, ERRCRIT, errlogpath, strerror(errno));
        return (-1); 
    }
  
    /* -- read the message file into memory -- */
    sprintf(msgfilepath, "%s/errors.msg", PATH_CONFIG);
    emsgfd = fopen (msgfilepath, "r");
    nummsgs = 0;
    if (emsgfd == NULL) {
        log_errs = 0;  /* Turn off writes to log files */
        if (errno == EMFILE)
            reporterr(ERRFAC, M_FILE, ERRCRIT, errlogpath, strerror(errno));
        fprintf (stderr, "%s: fatal error: cannot open %s\n", facility, msgfilepath);
        return (-1);
    }
    while ((!feof(emsgfd)) && (nummsgs < 256)) {
        (void) fgets (line, 160, emsgfd);
        len = strlen(line);
        if (len < 5 || line[0] == '#') continue;
        i = 0;
        emsgs[nummsgs].name[i] = '\0';
        while (i<len && i < 15 && line[i] != ' ' && line[i] != '\t') {
            i++;
        }
        strncpy(emsgs[nummsgs].name, line, i);
        emsgs[nummsgs].name[i+1] = '\0';
        while (i<len && (line[i] == ' ' || line[i] == '\t')) i++;
        len = len - i;
        if (len > 159) len = 159;
        strncpy(emsgs[nummsgs].msg, &line[i], len);
        emsgs[nummsgs].msg[len] = '\0';
        nummsgs++;
    }
    fclose (emsgfd);
    return (0);
}

/****************************************************************************
 * geterrmsg -- retrieve an message string based on the facility and name
 ***************************************************************************/
int
geterrmsg (char *facility, char* name, char* msg)
{
    int i;

    for (i=0; i<nummsgs; i++) {
        if (0 == strcmp(name, emsgs[i].name)) {
            strcpy (msg, emsgs[i].msg);
            return (0);
        }
    }
    sprintf(msg, "UNKNOWN " PRODUCTNAME " ERROR MESSAGE - FACILITY= %s NAME=%s",
        facility, name);
    return (1);
}


/****************************************************************************
 * logerrmsg -- log an error message to the syslog facility
 ***************************************************************************/
void
logerrmsg (char *facility, int level, char* name, char* errmsg)
{
    int priority;
    char msg[256];
    char msglevel[25];

    switch (level) {
    case ERRINFO:
        priority = LOG_INFO;
        strcpy (msglevel, "INFO");
        break;
    case ERRWARN:
        priority = LOG_WARNING;
        strcpy (msglevel, "WARNING");
        break;
    case ERRCRIT:
        priority = LOG_CRIT;
        strcpy (msglevel, "FATAL");
        break;
    default:
        priority = LOG_ERR;
        strcpy (msglevel, "UNKNOWN");
    }
        /* just use program name without path for error messages */
/*
    if (argv0) {
        pgmname = argv0;
        len = strlen(argv0);
        for(i=len;i>0;i--) {
            if (pgmname[i] == '/') break;
        }
        pgmname += (i ? i+1: 0);
    } else {
        pgmname = "";
    }
*/
    sprintf (msg, "%s: [%s / %s]: %s",
        facility, msglevel, name, errmsg);
#ifdef TDMF_TRACE
    fprintf (stderr, "%s", msg);
#endif
    ftderrorlogger (msg);
    syslog (priority, "%s", msg);
    if (always_log_stderr || isatty(0)) {
        strcat(msg, "\n");
        write(2, msg, strlen(msg));
    }
}

/****************************************************************************
 * reporterr -- report an error
 ***************************************************************************/
void _reporterr (char *facility, char* name, int level, ...)
{
    va_list args;
    char fmt[256];
    char msg[256];
 
    va_start(args, level);
    if (0 == geterrmsg (facility, name, fmt)) {
        vsprintf(msg, fmt, args);
        if (log_errs) 
            logerrmsg(facility, level, name, msg);
    } else {
        if (log_errs)
            logerrmsg(facility, ERRWARN, name, fmt);
    }
}


/***************************************************************************
 * logwatch.c - error log watcher / reporter
 *
 * Copyright (c) 1997, 1998 FullTime Software, Inc. All Rights Reserved
 *
 * History:
 *   10/30/97 - Steve Wahl - original code
 *
 ***************************************************************************
 */

/***************************************************************************
 * getlogmsgs -- return new error log messages since last call to this func.
 ***************************************************************************/
void 
getlogmsgs (char** msgbuf, int* msgsize, int* nummsgs, int* offset)
{
    struct stat statbuf;
    FILE* fd;
    int nlcnt;
    int i;
    long newlogsize;
    int len;
    char elogfilename[FILE_PATH_LEN];

    nlcnt = 0;
    sprintf (elogfilename, "%s/" QNM "error.log", PATH_RUN_FILES);
    if (0 == stat (elogfilename, &statbuf)) {
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

#if defined(MAIN)
main()
{
	char *string="pluke me, sweetheart";

	if (initerrmgt (ERRFAC) < 0) {
		exit(1);
	}

	reporterr (ERRFAC, M_THROTLOG, ERRWARN, string);

}
#endif /* defined(MAIN) */
