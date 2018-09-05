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

#ifdef LDEBUG
FILE *dbgfd;
#endif

#define ERRLOGMAX (128*1024)
char *default_error_facility = CAPQ;
int log_errs = 1;

static int nummsgs = 0;
static emsg_t emsgs[256];
static int always_log_stderr = 0;
static FILE *errlogfd = NULL;
static char errlogpath[FILE_PATH_LEN];
static char errlogpath2[FILE_PATH_LEN];
static void ftderrorlogger(char *msg);

extern char *argv0;

/****************************************************************************
 * logstderr -- tells the error system to always log to stderr
 ***************************************************************************/
void
logstderr(int flag)
{
    always_log_stderr = flag;
}
int
open_errlog()
{
    struct stat statbuf;

    if (errlogfd != NULL) {
	fclose(errlogfd);
	errlogfd = NULL;
    }

    if (stat(errlogpath, &statbuf) == 0) {
	if (statbuf.st_size >= ERRLOGMAX) {
	    (void)unlink(errlogpath2);
	    (void)rename(errlogpath, errlogpath2);
	}
    }
    if ((errlogfd = fopen(errlogpath, "a")) == NULL) {
	log_errs = 0;	/* Turn off writes to log files */
	if (errno == EMFILE)
	    reporterr(ERRFAC, M_FILE, LOG_CRIT, errlogpath, strerror(errno));
	return (-1);
    }
    log_errs = 1;
}
void
check_error_log_size()
{
    if (ftell(errlogfd) >= ERRLOGMAX) {
	(void)open_errlog();
    }
}

/****************************************************************************
 * ftderrorlogger -- writes an error message to the error log
 ***************************************************************************/

#if defined(reporterr_where)
char *err_glb_fnm;
int err_glb_lnum;
#endif /* defined(reporterr_where) */

static void
ftderrorlogger(char *string)
{
    struct tm *tim;
    time_t t;
    int i;
    char timebuf[20];

#if defined(reporterr_where)
    extern char *argv0;
    int pid = getpid();
    int ppid = getppid();
#endif /* defined(reporterr_where) */

    if (errlogfd == NULL) {
	return;
    }
    check_error_log_size();
    (void)time(&t);
    tim = localtime(&t);
    i = strlen(string) - 1;
    while ((!isprint(string[i])) || string[i] == ' ') {
	string[i] = '\0';
	i--;
    }
    strftime(timebuf, sizeof(timebuf), "%Y/%m/%d %H:%M:%S", tim);
    
#if defined(reporterr_where)
    fprintf(errlogfd,
	    "[%s] [proc: %s] [pid,ppid: %d, %d] [src,line: %s, %d] %s\n",
	    timebuf, argv0, pid, ppid, err_glb_fnm, err_glb_lnum, string);
#else /* reporterr_where */
    fprintf(errlogfd, "[%s] %s\n", timebuf, string);
#endif /* reporterr_where */
    fflush(errlogfd);
}

/****************************************************************************
 * initerrmgt -- initialize error and message management and reporting
 * returns: 0 - Ok, -1 - writes to log files disabled, failed open.
 ***************************************************************************/
int
initerrmgt(char *facility)
{
#ifdef LDEBUG
    char tracefile[256];
#endif
    char msgfilepath[256];
    char line[256];
    FILE *emsgfd;
    int i, len;
    struct stat statbuf;

    /* by default we attempt to write to log files */
    log_errs = 1;
    openlog(facility, LOG_PID, LOG_DAEMON);
    setlogmask(LOG_UPTO(LOG_INFO));
#ifdef LDEBUG
    sprintf(tracefile, "/tmp/%s.trace", facility);
    if ((dbgfd = fopen(tracefile, "w")) == NULL) {
	if (errno == EMFILE)
	    reporterr(ERRFAC, M_FILE, ERRCRIT, tracefile, strerror(errno));
	fprintf(stderr, "\nCouldn't open error trace file: %s", tracefile);
	return (-1);
    }
#endif
    /* -- open the error log */
    sprintf(errlogpath, "%s/" QNM "error.log", PATH_RUN_FILES);
    sprintf(errlogpath2, "%s/" QNM "error.log.1", PATH_RUN_FILES);
    if (open_errlog() < 0) {
	return (-1);
    }
    /* -- read the message file into memory -- */
    sprintf(msgfilepath, "%s/errors.msg", PATH_CONFIG);
    emsgfd = fopen(msgfilepath, "r");
    nummsgs = 0;
    if (emsgfd == NULL) {
	log_errs = 0;		/* Turn off writes to log files */
	if (errno == EMFILE) {
	    reporterr(ERRFAC, M_FILE, ERRCRIT, msgfilepath, strerror(errno));
	}
	fprintf(stderr, "%s: fatal error: cannot open %s\n", facility,
		msgfilepath);
	return (-1);
    }
    while ((!feof(emsgfd)) && (nummsgs < 256)) {
	(void)fgets(line, 160, emsgfd);
	len = strlen(line);
	if (len < 5 || line[0] == '#')
	    continue;
	i = 0;
	emsgs[nummsgs].name[i] = '\0';
	while (i < len && i < 15 && line[i] != ' ' && line[i] != '\t') {
	    i++;
	}
	strncpy(emsgs[nummsgs].name, line, i);
	emsgs[nummsgs].name[i + 1] = '\0';
	while (i < len && (line[i] == ' ' || line[i] == '\t'))
	    i++;
	len = len - i;
	if (len > 159)
	    len = 159;
	strncpy(emsgs[nummsgs].msg, &line[i], len);
	emsgs[nummsgs].msg[len] = '\0';
	nummsgs++;
    }
    fclose(emsgfd);
    return (0);
}

/****************************************************************************
 * geterrmsg -- retrieve an message string based on the facility and name
 ***************************************************************************/
int
geterrmsg(char *facility, char *name, char *msg)
{
    int i;

    for (i = 0; i < nummsgs; i++) {
	if (0 == strcmp(name, emsgs[i].name)) {
	    strcpy(msg, emsgs[i].msg);
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
logerrmsg(char *facility, int level, char *name, char *errmsg)
{
    int priority;
    char msg[256];
    char msglevel[25];

    switch (level) {
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
    sprintf(msg, "%s: [%s / %s]: %s", facility, msglevel, name, errmsg);
#ifdef LDEBUG
    fprintf(dbgfd, "%s", msg);
#endif
    ftderrorlogger(msg);

#ifdef TDMF_TRACE
	 syslog(LOG_DEBUG, "%s", msg);
#else
    syslog(priority, "%s", msg); 
#endif
  

    if (always_log_stderr || isatty(0)) {
	strcat(msg, "\n");
	write(2, msg, strlen(msg));
    }
}

/****************************************************************************
 * reporterr -- report an error
 ***************************************************************************/
void
_reporterr(char *facility, char *name, int level, ...)
{
	va_list args;
	char fmt[256];
	char msg[256];

	va_start(args, level);
	if (0 == geterrmsg(facility, name, fmt)) 
		{
		vsprintf(msg, fmt, args);
		if (log_errs)
		 	logerrmsg(facility, level, name, msg);
		} 
	else 
		{
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
/*
 * This version of the code implements a semantic change for the fourth
 * argument.  It used to be called offset and was the offset from the
 * beginning of the file to the last of the file, the last time
 * the routine was called.  Now it is a count of the number of bytes
 * in the file the last time the routine was called.  This is a delta
 * of one byte.  It should be transparent to monitortool, the main client.
 * Much easier to understand this way, imho, TDH
 */
void
getlogmsgs(char **msgbuf,	/* tell caller about the buffer we will malloc */
	   int *msgsize,	/* size of malloced buffer */
	   int *nummsgs,	/* number of lines in buffer */
	   int *nbytes		/* number of reported bytes from errlogfile */
    ) {
    struct stat statbuf;
    FILE *fd;
    int nlcnt;
    int i;
    long newlogsize;
    long len;
    long old_log_bytes = 0;
    long malloc_size;
    char *ret_buff;
    char *cp;
    int curfile_start_pt = *nbytes;

    if (log_errs && stat(errlogpath, &statbuf) == 0) {
	newlogsize = statbuf.st_size;
    } else {
	/* file must not exist yet */
	newlogsize = 0;
    }
    
    *msgbuf = (char *)NULL;
    *nummsgs = 0;
    *msgsize = 0;
    if (*nbytes < 0) {
	/* first call, just initialize things to get future logged messages */
	*nbytes = newlogsize;
	return;
    }
    if (newlogsize < *nbytes) {
	/* log file has rolled */
	if(stat(errlogpath2, &statbuf) == 0) {
	    old_log_bytes = statbuf.st_size - *nbytes;
	    if(old_log_bytes < 0) {
		old_log_bytes = 0;
	    }
	}
	malloc_size = old_log_bytes + newlogsize;
	curfile_start_pt = 0;
    } else {
	malloc_size = newlogsize - *nbytes;
    }

    if(malloc_size == 0) {
	*nbytes = newlogsize;
	return;
    }
    
    /* extra for a possible null-byte */
    ret_buff = (char *)malloc(malloc_size + 1);
    if(ret_buff == NULL) {
	reporterr(ERRFAC, M_MALLOC, LOG_ERR, malloc_size +1);
	*nbytes = newlogsize;
	return;
    }

    len = 0;
    if(old_log_bytes != 0) {
	if((fd = fopen(errlogpath2, "r")) != NULL) {
	    (void)fseek(fd, *nbytes, SEEK_SET);
	    len = fread((void *)ret_buff, (size_t) 1, (size_t)old_log_bytes, fd);
	    fclose(fd);
	} else {
	    /* hmmmm, should not have happend */
	    old_log_bytes = 0;
	}
    }

    if(newlogsize > 0) {
	/*
	 * open our own copy of errlogpath, so we don't screw with
	 * current offset ptr, maybe multi threaded someday
	 * (pretty lame, huh?)
	 */
	if((fd = fopen(errlogpath, "r")) != NULL) {
	    if(curfile_start_pt != 0) {
		/* 
		 * old number of bytes reported is offset
		 * to first unreported byte
		 */
		(void)fseek(fd, curfile_start_pt, SEEK_SET);
		/*
		 * BUG: what if file rolls between stat
		 * and fopen, fseek will return EFBIG
		 */
	    }
	    /*
	     * set the length of the read to newlogsize-1, they think 
	     * the last character is a newline and they do not want it
	     */
	    len += fread((void *)(ret_buff + len), (size_t) 1, 
			 (size_t)newlogsize-1, fd);
	    fclose(fd);
	}
    }

    /* len should be <= malloc_size */
    /* just to be sure the buffer is null terminated */
    ret_buff[malloc_size] = '\0';
    nlcnt = 0;
    for (i = 0, cp = ret_buff; i < len; i++, cp++) {
	if(*cp == '\0') {
	    len = i;
	    break;
	}
	if(*cp == '\n') {
	    nlcnt++;
	}
    }
    *msgsize = len;
    *nummsgs = nlcnt;		/* no one uses this, what is the point */
    *nbytes = newlogsize;
    *msgbuf = ret_buff;
    return;
}
#if defined(MAIN)
main()
{
    char *string = "pluke me, sweetheart";

    if (initerrmgt(ERRFAC) < 0) {
	exit(1);
    }

    reporterr(ERRFAC, M_THROTLOG, ERRWARN, string);

}
#endif /* defined(MAIN) */
