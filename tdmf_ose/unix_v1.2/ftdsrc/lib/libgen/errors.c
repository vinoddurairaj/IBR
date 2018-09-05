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
#include "common.h"
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

#define EMSGFTD 0
#define EMSGUNKNOWN 1

/*
 * The number of messages that can define in errors.MSG file
 */
#define	NUM_OF_MSGS	512

typedef struct emsg {
    char name[16];
    char msg[256];
} emsg_t;

#ifdef LDEBUG
FILE *dbgfd;
#endif

#define ERRLOGMAX (1024*1024)
char *default_error_facility = CAPQ;
int log_errs = 1;
int log_internal = 0; /* 0 => suppress M_INTERNAL logging */

static int nummsgs = 0;
static emsg_t emsgs[NUM_OF_MSGS];
static int always_log_stderr = 0;
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
static FILE *errlogfd = NULL;
#elif defined(SOLARIS)
static int  errlogfd = -1;
#endif
static char errlogpath[FILE_PATH_LEN];
static char errlogpath2[FILE_PATH_LEN];
static void ftderrorlogger(char *filename, int lineno, char *msg);

extern char *argv0;

/****************************************************************************
 * get_error_str -- return a text string interpretation of errno
 * returns: pointer to a circular buffer of text strng messages.
 *          Period is _MAXERRSTRS
 ***************************************************************************/
#define _MAXERRSTRS 8
#define _MAXLINE 32
const char *get_error_str(int err)
{
    static char msgbufs[_MAXERRSTRS][_MAXLINE];
    static char *msgbuf = msgbufs[0];
    char *p = msgbuf;
    char *errmsg = NULL;
    #define CASE(errno) case errno: errmsg = #errno; break
    int left = _MAXLINE - 1;

    if (err > 0)
        errmsg = strerror(err);
    else
        err = -err;
    if (!errmsg) {
        switch (err) {
		CASE(EPERM);
		CASE(ENOENT);
		CASE(ESRCH);
		CASE(EINTR);
		CASE(EIO);
		CASE(ENXIO);
		CASE(E2BIG);
		CASE(ENOEXEC);
		CASE(EBADF);
		CASE(ECHILD);
		CASE(EAGAIN);
		CASE(ENOMEM);
		CASE(EACCES);
		CASE(EFAULT);
		CASE(ENOTBLK);
		CASE(EBUSY);
		CASE(EEXIST);
		CASE(EXDEV);
		CASE(ENODEV);
		CASE(ENOTDIR);
		CASE(EISDIR);
		CASE(EINVAL);
		CASE(ENFILE);
		CASE(EMFILE);
		CASE(ENOTTY);
		CASE(ETXTBSY);
		CASE(EFBIG);
		CASE(ENOSPC);
		CASE(ESPIPE);
		CASE(EROFS);
		CASE(EMLINK);
		CASE(EPIPE);
		CASE(EDOM);
		CASE(ERANGE);
		CASE(EDEADLK);
		CASE(ENAMETOOLONG);
		CASE(ENOLCK);
		CASE(ENOSYS);
		CASE(ELOOP);
		CASE(ENOMSG);
		CASE(EIDRM);
		CASE(ECHRNG);
		CASE(EL2NSYNC);
		CASE(EL3HLT);
		CASE(EL3RST);
		CASE(ELNRNG);
		CASE(EUNATCH);
		CASE(ENOCSI);
		CASE(EL2HLT);
		CASE(ENOSTR);
		CASE(ENODATA);
		CASE(ETIME);
		CASE(ENOSR);
		CASE(EREMOTE);
		CASE(ENOLINK);
		CASE(EPROTO);
		CASE(EMULTIHOP);
		CASE(EBADMSG);
		CASE(EOVERFLOW);
		CASE(EILSEQ);
		CASE(EUSERS);
		CASE(ENOTSOCK);
		CASE(EDESTADDRREQ);
		CASE(EMSGSIZE);
		CASE(EPROTOTYPE);
		CASE(ENOPROTOOPT);
		CASE(EPROTONOSUPPORT);
		CASE(ESOCKTNOSUPPORT);
		CASE(EOPNOTSUPP);
		CASE(EPFNOSUPPORT);
		CASE(EAFNOSUPPORT);
		CASE(EADDRINUSE);
		CASE(EADDRNOTAVAIL);
		CASE(ENETDOWN);
		CASE(ENETUNREACH);
		CASE(ENETRESET);
		CASE(ECONNABORTED);
		CASE(ECONNRESET);
		CASE(ENOBUFS);
		CASE(EISCONN);
		CASE(ENOTCONN);
		CASE(ESHUTDOWN);
		CASE(ETOOMANYREFS);
		CASE(ETIMEDOUT);
		CASE(ECONNREFUSED);
		CASE(EHOSTDOWN);
		CASE(EHOSTUNREACH);
		CASE(EALREADY);
		CASE(EINPROGRESS);
		CASE(ESTALE);
		CASE(EDQUOT);
        case 0:
            errmsg = "<no error>";
            break;
        default:
            errmsg = "*unknown*";
        }
    }
    p += snprintf(p, left, "%s", errmsg);
    left -= (p - msgbuf);
    if (left > strlen("[000]"))
        p += snprintf(p, left, "[%d]", errno);
    *p = '\0';
    p = msgbuf;
    if (msgbuf >= msgbufs[_MAXERRSTRS - 1])
        msgbuf = msgbufs[0];
    else
        msgbuf += _MAXLINE;
    return p;
}

/****************************************************************************
 * logstderr -- tells the error system to always log to stderr
 ***************************************************************************/
void
logstderr(int flag)
{
    always_log_stderr = flag;
}
int
open_errlog(void)
{
    struct stat statbuf;
    mode_t save_mode, new_mode=0177; /* WR16793 */

#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if (errlogfd != NULL) {
	fclose(errlogfd);
	errlogfd = NULL;
    }
#elif defined(SOLARIS)
    if (errlogfd != -1) {
	close(errlogfd);
	errlogfd = -1;
    }
#endif

    if (stat(errlogpath, &statbuf) == 0) {
	if (statbuf.st_size >= ERRLOGMAX) {
	    (void)unlink(errlogpath2);
	    (void)rename(errlogpath, errlogpath2);
	}
    }
    save_mode = umask(new_mode); /* WR16793 */
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    errlogfd = fopen(errlogpath, "a");
#elif defined(SOLARIS)
    errlogfd = open(errlogpath, O_WRONLY|O_APPEND|O_CREAT, 0600);
#endif
    umask(save_mode); /* WR16793 */
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if (errlogfd == NULL) {
#elif defined(SOLARIS)
    if (errlogfd == -1) {
#endif
	log_errs = 0;	/* Turn off writes to log files */
	if (errno == EMFILE)
	    reporterr(ERRFAC, M_FILE, LOG_CRIT, errlogpath, strerror(errno));
	return (-1);
    }
    log_errs = 1;
    return 0;
}
void
check_error_log_size(void)
{
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if (ftell(errlogfd) >= ERRLOGMAX) {
#elif defined(SOLARIS)
    if (tell(errlogfd) >= ERRLOGMAX) { 
#endif
	(void)open_errlog();
    }
}

/****************************************************************************
 * ftderrorlogger -- writes an error message to the error log
 ***************************************************************************/

static void
ftderrorlogger(char *filename, int lineno, char *string)
{
    struct tm *tim;
    time_t t;
    int i;
    char timebuf[20];
#if defined(SOLARIS)
    char buf[512];
#endif

    extern char *argv0;
    int pid = getpid();
    int ppid = getppid();

#if defined(SOLARIS)
    memset(buf, 0, sizeof(char)*512);
#endif

#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if (errlogfd == NULL) {
#elif defined(SOLARIS)
    if (errlogfd == -1) {    
#endif
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
    
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    fprintf(errlogfd,
	    "[%s] [proc: %s] [pid,ppid: %d, %d] [src,line: %s, %d] %s\n",
	    timebuf, argv0, pid, ppid, filename, lineno, string);
#elif defined(SOLARIS)  /* 1LG-ManyDEV ManyLG-1DEV */ 
    sprintf(buf,
	    "[%s] [proc: %s] [pid,ppid: %d, %d] [src,line: %s, %d] %s\n",
	    timebuf, argv0, pid, ppid, filename, lineno, string);
    write(errlogfd, buf, strlen(buf));
#endif 

#if 0
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    fprintf(errlogfd, "[%s] %s\n", timebuf, string);
#elif defined(SOLARIS)
    sprintf(buf, "[%s] %s\n", timebuf, string);
    write(errlogfd, buf, strlen(buf));
#endif
#endif
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    fflush(errlogfd);
#elif defined(SOLARIS) /* 1LG-ManyDEV ManyLG-1DEV */
    fsync(errlogfd);
#endif
}

/****************************************************************************
 * initerrmgt -- initialize error and message management and reporting
 * returns: 0 - Ok, -1 - writes to log files disabled, failed open.
 ***************************************************************************/
static char syslog_prefix[LINE_MAX];
void
setsyslog_prefix(const char *fmt, ...)
{
    char *p = syslog_prefix;
    int remain = sizeof(syslog_prefix) - 1;
    int N;
    pid_t ppid = getppid();
    va_list ap;
    va_start(ap, fmt);
    N = vsnprintf(syslog_prefix, remain, fmt, ap);
    va_end(ap);
    p += N;
    remain -= N;
    if (ppid != 1) {
        snprintf(p, remain - N, "[%d/%d]", getpid(), ppid);
    }
}

int
initerrmgt(char *facility)
{
#ifdef LDEBUG
    char tracefile[256];
#endif
    char msgfilepath[256];
    char line[256];
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    FILE *emsgfd;
#elif defined(SOLARIS) /* 1LG-ManyDEV ManyLG-1DEV */
    int  emsgfd;
#endif
    int i, len;
    struct stat statbuf;

    /* by default we attempt to write to log files */
#if defined(linux)
    pid_t ppid = getppid();
    if (ppid != 1)
        openlog((sprintf(syslog_prefix, "%s[%d/%d]", facility,
            getpid(), ppid), syslog_prefix), 0, LOG_DAEMON);
    else
#else
    openlog(facility, LOG_PID, LOG_DAEMON);
#endif
    setlogmask(LOG_UPTO(LOG_INFO));
    log_errs = 1;
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
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    emsgfd = fopen(msgfilepath, "r");
#elif defined(SOLARIS)
    emsgfd = open(msgfilepath, O_RDONLY);
#endif
    nummsgs = 0;
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if (emsgfd == NULL)
#elif defined(SOLARIS)
    if (emsgfd == -1)
#endif
    {
        log_errs = 0;		/* Turn off writes to log files */
        if (errno == EMFILE) {
            reporterr(ERRFAC, M_FILE, ERRCRIT, msgfilepath, strerror(errno));
        }
        fprintf(stderr, "%s: fatal error: cannot open %s\n", facility,
            msgfilepath);
        return (-1);
    }
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    while ((!feof(emsgfd)) && (nummsgs < NUM_OF_MSGS)) {
        (void)fgets(line, 265, emsgfd);
#elif defined(SOLARIS)
    while ((!ftd_feof(emsgfd)) && (nummsgs < NUM_OF_MSGS)) {
        (void)ftd_fgets(line, 265, emsgfd);
#endif
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
        if (len > 240)
            len = 240;
        strncpy(emsgs[nummsgs].msg, &line[i], len);
        emsgs[nummsgs].msg[len] = '\0';
        nummsgs++;
    }
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    fclose(emsgfd);
#elif defined(SOLARIS)
    close(emsgfd);
#endif
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
 * _logerrmsg -- log an error message to the syslog facility
 ***************************************************************************/
void
_logerrmsg(char *filename, int lineno, char *facility, int level, char *name, char *errmsg)
{
    int priority;
    char msg[256];
    char *p;
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

    sprintf(msg, "%s: [%s / %s]: %s", facility, msglevel, name, errmsg);
#ifdef LDEBUG
    fprintf(dbgfd, "%s", msg);
#endif
    if (strcmp(name, M_INTERNAL))
        ftderrorlogger(filename, lineno, msg);
    if (0 == strcmp(name, "COMMAND"))
	return;   /* don't echo command logging */

#if defined(linux)
    if (log_internal) {
        p = msg;
        p += sprintf(p, "\"%s\":%d ", filename, lineno);
        sprintf(p, "[%c:%s]: %s", msglevel[0], name, errmsg);
    }
#endif

#ifdef TDMF_TRACE
	syslog(LOG_DEBUG, "%s", msg);
#else
    syslog(priority, "%s", msg); 
#endif
  
    if (always_log_stderr || isatty(0)) {
        strcat(msg, "\n");
        write(2, msg, strlen(msg));
    }
    return;
}

/****************************************************************************
 * reporterr -- report an error
 ***************************************************************************/
void
_reporterr(char *filename, int lineno, char *facility, char *name, int level, ...)
{
	va_list args;
	char fmt[256];
	char msg[256];

    if (!log_internal && !strcmp(name, M_INTERNAL))
        return;
	va_start(args, level);
	if (0 == geterrmsg(facility, name, fmt)) {
		vsnprintf(msg, sizeof(msg), fmt, args);
		if (log_errs)
		 	_logerrmsg(filename, lineno, facility, level, name, msg);
    } else {
        if (log_errs)
             _logerrmsg(filename, lineno, facility, ERRWARN, name, fmt);
    }
    va_end(args);
    
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
    )
{
    struct stat statbuf;
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    FILE *fd;
#elif defined(SOLARIS)
    int fd;
#endif
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
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
	if((fd = fopen(errlogpath2, "r")) != NULL) {
#elif defined(SOLARIS)
        if((fd = open(errlogpath2, O_RDONLY)) != -1) {
#endif
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
	    (void)fseek(fd, *nbytes, SEEK_SET);
	    len = fread((void *)ret_buff, (size_t) 1, (size_t)old_log_bytes, fd);
	    fclose(fd);
#elif defined(SOLARIS)
            (void)lseek(fd, *nbytes, SEEK_SET);
            len = read(fd, (void *)ret_buff, (size_t) 1 * (size_t)old_log_bytes);
            close(fd);
#endif
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
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
	if((fd = fopen(errlogpath, "r")) != NULL) {
#elif defined(SOLARIS)
        if((fd = open(errlogpath, O_RDONLY)) != -1) {
#endif
	    if(curfile_start_pt != 0) {
		/* 
		 * old number of bytes reported is offset
		 * to first unreported byte
		 */
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
		(void)fseek(fd, curfile_start_pt, SEEK_SET);
#elif defined(SOLARIS)
                (void)lseek(fd, curfile_start_pt, SEEK_SET);
#endif
		/*
		 * BUG: what if file rolls between stat
		 * and fopen, fseek will return EFBIG
		 */
	    }
	    /*
	     * set the length of the read to newlogsize-1, they think 
	     * the last character is a newline and they do not want it
	     */
#if defined(HPUX) ||  defined(_AIX)
	    len += fread((void *)(ret_buff + len), (size_t) 1, 
			 (size_t)(malloc_size - len), fd);
	    fclose(fd);
#elif defined(SOLARIS)
            len += read(fd, (void *)(ret_buff + len), 
			 (size_t)(malloc_size - len));
	    close(fd);
#elif defined(linux)
	    len += fread((void *)(ret_buff + len), (size_t) 1, 
			 (size_t)(malloc_size - len), fd);
	    fclose(fd);
#endif
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

/****************************************************************************
 * log_command -- log the command and its arguments to dtcerror.log
 ***************************************************************************/
void
log_command (int argc, char** argv)
{
  char cmdline[1024];
  int i;
  int l;
  int idx;

  /* copy the command and its arguments into a string */
  i = 0;
  idx = 0;
  while (argc--) {
    l = strlen(argv[idx]);
    strncpy (&cmdline[i], argv[idx], l);
    i += l;
    idx++;
    cmdline[i++] = ' ';
  }
  cmdline[i] = '\0';
  /* write an INFO message to the error log documenting the command */
  reporterr(ERRFAC, M_COMMAND, ERRINFO, cmdline);
  return;
}

/**********************************************************************/

#if defined(_OBSOLETE)
main()
{
    char *string = "pluke me, sweetheart";

    if (initerrmgt(ERRFAC) < 0) {
        EXIT(1);
    }

    reporterr(ERRFAC, M_THROTLOG, ERRWARN, string);

}
#endif /* defined(_OBSOLETE) */
