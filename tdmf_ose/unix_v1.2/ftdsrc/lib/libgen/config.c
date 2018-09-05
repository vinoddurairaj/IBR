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
 * config.c - FullTime Data daemon configuration file processing module
 *
 * (c) Copyright 1996, 1997 FullTime Software, Inc. All Rights Reserved
 *
 * History:
 *   11/11/96 - Steve Wahl - original code
 *
 ***************************************************************************
 */

#ifdef NEED_BIGINTS
#include "bigints.h"
#endif
#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <limits.h>
#include "throttle.h"
#include "config.h"
#include "cfg_intr.h"
#include "errors.h"
#include "network.h"
#include "common.h"
#include "pathnames.h"
#include "ps_intr.h"
#include "ftdif.h"
#include "ftd_cmd.h"
#include "errmsg.h"

#if defined (SOLARIS)
#include <sys/vtoc.h>
#ifdef EFI_SUPPORT
#include <sys/efi_partition.h> 
#endif
#endif 

#define TRUE  1
#define FALSE 0
#define  ADDRSIZE  16   
#ifdef DEBUG_THROTTLE
FILE* throtfd;
FILE* oldthrotfd;
#endif /* DEBUG_THROTTLE */

/* -- structure used for reading and parsing a line from the config file */
typedef struct _line_state {
    int  invalid;       /* bogus boolean. */
    int  lineno;
    char line[1024];        /* storage for parsed line */
    char readline[1024];    /* unparsed line */
    /* next four elements used for parsing throttles */
    char word[1024];        
    int plinepos;
    int linepos;
    int linelen;
    /* next two elements for parsing a value or a string */
    long value;
    int valueflag;
    /* ptrs to parsed parameters in line[] */
    char *key;
    char *p1;
    char *p2;
    char *p3;
} LineState;

typedef struct _host_info {
    char   hostnodename[256];
    u_long ip;
} HostInfo;

/* definitions of the primary and secondary systems - index 0 is the primary,
   and index 1 is the secondary. */
machine_t sys[2];

machine_t *mysys = &sys[0];    /* this machine's description */
machine_t *othersys = &sys[1]; /* other machine's description */
u_long    *myips;              /* array of IPv4 addresses for this machine */
char      *myip6;              /* array of IP (IPv4 & Ipv6)addresses for this machine */
int       myipcount = 0;       /* number of IPv4 addresses for this machine */
int       myip6count = 0;      /* number of IP (IPv4 & IPv6)addresses for this machine */

static char ipstring[20];

char *paths;
static int lgnum;

extern char *argv0;

#if defined(SOLARIS)  /* 1LG-ManyDEV */ 
int dummyfd;
#endif
/* -- tunable parameters globals -- */

int    chunksize = CHUNKSIZE; 
int    statinterval = STATINTERVAL; 
int    maxstatfilesize = MAXSTATFILESIZE;      
int    secondaryport = SECONDARYPORT;
int    syncmode = SYNCMODE;
int    syncmodedepth = SYNCMODEDEPTH;
int    syncmodetimeout = SYNCMODETIMEOUT;
int    compression = COMPRESSION;      
int    tcpwindowsize = TCPWINDOWSIZE;      
int    netmaxkbps = NETMAXKBPS;
u_long chunkdelay = CHUNKDELAY;
int    lrt = LRT;

int verify_rmd_entries(group_t *group, char *path, int openflag);
#if defined(linux)
#define getline getline0
#endif /* defined(linux) */
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
static int getline (FILE *fd, LineState *ls);
static void get_word (FILE *fd, LineState *ls);
static void drain_to_eol (FILE* fd, LineState* ls);
#elif defined(SOLARIS)
static int getline (int fd, LineState *ls);
static void get_word (int fd, LineState *ls);
static void drain_to_eol (int fd, LineState* ls);
#endif
static void forget_throttle (throttle_t* throttle);
static int verify_pmd_entries(sddisk_t *curdev, char *path, int openflag);
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
extern int parse_throttles (FILE* fd, LineState *ls);
#elif defined(SOLARIS)
extern int parse_throttles (int fd, LineState *ls);
#endif

/*
 * Read buffer until we hit an "end of pair" delimiter, break up key 
 * and value into two strings in place, and return the pointers to the 
 * key and value. The buffer
 * pointer is incremented to the next "line." The delimiter should be 
 * a reasonable non-white space character. This code will skip over 
 * comment lines and lines that are too short to hold valid data, 
 * which should never happen but we do it anyway.
 *
 * Returns FALSE, if parsing failed (end-of-buffer reached). 
 * Returns TRUE, if parsing succeeded. 
 */
static int 
getbufline (char **buffer, char **key, char **value, char delim)
{
    int i, len;
    int blankflag;
    char *line, *tempbuf;
    int commentflag;

    *key = *value = NULL;

    /* BOGUS DELIMITER CHECK */
    if ((delim == 0) || (delim == ' ') || (delim == '\t')) {
        return FALSE;
    }

    while (1) {
        line = tempbuf = *buffer;
        if (tempbuf == NULL) {
            return FALSE;
        }

        /* search for the delimiter or a NULL */
        len = 0;
        while ((tempbuf[len] != delim) && (tempbuf[len] != 0)) {
            len++;
        }
        if (tempbuf[len] == delim) {
            tempbuf[len] = 0;
            *buffer = &tempbuf[len+1];
        } else {
            /* must be done! */
            *buffer = NULL;
        }
        if (len == 0) {
            return FALSE;
        }
  
        /* skip comment lines and lines that are too short to count */
        if (len < 5) continue;
        commentflag = 0;
        for (i=0; i<len; i++) {
            if (isgraph(line[i]) == 0) continue;
            if (line[i] == '#') {
                commentflag = 1;
            }
            break;
        }
        if (commentflag) continue;

        /* ignore blank lines */
        blankflag = 1;
        for (i = 0; i < len; i++) {
            if (isgraph(line[i])) {
                blankflag = 0;
                break;
            }
        }
        if (blankflag) continue;

        /* -- get rid of leading whitespace -- */
        i = 0;
        while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
        if (i >= len) continue;

        /* -- accumulate the key */
        *key = &line[i];
        while ((i < len) && (line[i] != ' ') && (line[i] != '\t') && 
               (line[i] != '\n')) i++;
        line[i++] = 0;

        /* -- bypass whitespace */
        while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
        if (i >= len) break;

        /* -- accumulate the value */
        *value = &line[i];
        while ((i < len) && (line[i] != ' ') && (line[i] != '\t') && 
               (line[i] != '\n')) i++;
        line[i] = 0;

        /* done */
        return TRUE;
    }
    return TRUE;        /* Impossible, but keeps gcc happy */
}
/*
 *  getpsname - retrieves ps_name from cfg file for a lg
 */
#ifdef TDMF_TRACE
int getpsname (char *mod, char *funct,int lgnum, char *ps_name)
#else
int getpsname (int lgnum, char *ps_name)
#endif
{
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    FILE *fd;
#elif defined(SOLARIS) /* 1LG-ManyDEV  ManyLG-1DEV */ 
    int fd;
#endif
    char cfg_path[MAXPATHLEN];
    LineState lstate; 
    int status = -1;

    sprintf(cfg_path, "%s/%s%03d.cfg", PATH_CONFIG, PMD_CFG_PREFIX, lgnum);

#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if ((fd = fopen(cfg_path, "r")) == NULL)
#elif defined(SOLARIS)  /* 1LG-ManyDEV  ManyLG-1DEV */ 
    if ((fd = open(cfg_path, O_RDONLY)) == -1)
#endif
    {
        if (errno == EMFILE) {
            reporterr(ERRFAC, M_FILE, ERRCRIT, cfg_path, 
                strerror(errno));
        }
        reporterr(ERRFAC, M_CFGFILE, ERRCRIT,
            argv0, cfg_path, strerror(errno));
        return status;
    }

    lstate.lineno = 0;
    lstate.invalid = TRUE;
    while (1) {
        /* if we need a line and don't get it, then we're done */
        if (!getline(fd, &lstate)) {
            break;
        }
        if (strcmp("PSTORE:", lstate.key) == 0)
        {
		    /* WR PROD6877: verify that the parameter expected with the keyword is not missing in the config file;
		       on SuSE Linux, this situation leads to a segmentation fault.
		    */
		    if(lstate.p1 ==  NULL)
			{
                reporterr(ERRFAC, M_MISSCFGPARM, ERRCRIT, lgnum, "PSTORE");
			}
			else
			{
                strcpy(ps_name, lstate.p1);
                ftd_trace_flow(FTD_DBG_FLOW12,
                    "%s/%s() getpsname: %s \n", mod, funct, ps_name);
                status = 0;
			}
	        break;
        }
    }
#if defined(HPUX) ||  defined(_AIX) || defined(linux) 
    fclose(fd);
#elif defined(SOLARIS)
    close(fd);
#endif
    return status;
}

/*
 * Remove cur files 
 */

#define CFG_PMD 0x01
#define CFG_RMD 0x02

void remove_cfg_cur_files(int lgnum, int delete_what)
{
    char full_cfg_path[MAXPATHLEN];

    if (delete_what & CFG_PMD) {
	sprintf(full_cfg_path, "%s/%s%03d.cur",
		 PATH_CONFIG, PMD_CFG_PREFIX, lgnum);
	unlink (full_cfg_path);
    }
    if (delete_what & CFG_RMD) {
	sprintf(full_cfg_path, "%s/%s%03d.cur",
		 PATH_CONFIG, RMD_CFG_PREFIX, lgnum);
	unlink (full_cfg_path);
    }
}

/*
 *  getautostart - return true if start on boot is allowed
 */
int getautostart(int lgnum)
{
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    FILE *fd;
#elif defined(SOLARIS) /* 1LG-ManyDEV  ManyLG-1DEV */ 
    int fd;
#endif
    char cfg_path[MAXPATHLEN];
    LineState lstate; 
    int autostart = 1;

    sprintf(cfg_path, "%s/%s%03d.cfg", PATH_CONFIG, PMD_CFG_PREFIX, lgnum);

#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if ((fd = fopen(cfg_path, "r")) == NULL)
#elif defined(SOLARIS)  /* 1LG-ManyDEV  ManyLG-1DEV */ 
    if ((fd = open(cfg_path, O_RDONLY)) == -1)
#endif
    {
        if (errno == EMFILE) {
            reporterr(ERRFAC, M_FILE, ERRCRIT, cfg_path, 
                strerror(errno));
        }
        reporterr(ERRFAC, M_CFGFILE, ERRCRIT,
            argv0, cfg_path, strerror(errno));
        return -1;
    }

    lstate.lineno = 0;
    lstate.invalid = TRUE;
    while (1) {
        /* if we need a line and don't get it, then we're done */
        if (!getline(fd, &lstate)) {
            break;
        }
        if (strcmp("AUTOSTART:", lstate.key) == 0) {
            if (strcasecmp("off", lstate.p1) == 0
	     || strcasecmp("no", lstate.p1) == 0
	     || strcasecmp("false", lstate.p1) == 0) {
		autostart = 0;
		break;
	    }
        }
    }
#if defined(HPUX) ||  defined(_AIX) || defined(linux) 
    fclose(fd);
#elif defined(SOLARIS)
    close(fd);
#endif
    return autostart;
}

/*
 *  getnumdtcdevices - retrieves the number of dtc devices from cfg file for a lg
 */
#ifdef TDMF_TRACE
int getnumdtcdevices (char *mod, char *funct, int lgnum, int *numdevs, char **dtcdevs)
#else
int getnumdtcdevices (int lgnum, int *numdevs, char **dtcdevs)
#endif
{
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    FILE *fd;
#elif defined(SOLARIS) /* 1LG-ManyDEV  ManyLG-1DEV */
    int fd;
#endif
    char cfg_path[MAXPATHLEN];
    LineState lstate;
    int error, devices = 0;

    sprintf(cfg_path, "%s/%s%03d.cfg", PATH_CONFIG, PMD_CFG_PREFIX, lgnum);

#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if ((fd = fopen(cfg_path, "r")) == NULL) {
#elif defined(SOLARIS)  /* 1LG-ManyDEV  ManyLG-1DEV */
    if ((fd = open(cfg_path, O_RDONLY)) == -1) {
#endif
        if (errno == EMFILE) {
            reporterr(ERRFAC, M_FILE, ERRCRIT, cfg_path,
                strerror(errno));
        }
        reporterr(ERRFAC, M_CFGFILE, ERRCRIT,
            argv0, cfg_path, strerror(errno));
        return -1;
    }

    lstate.lineno = 0;
    lstate.invalid = TRUE;
    error = 0;
    while (1) {
        /* if we need a line and don't get it, then we're done */
        if (!getline(fd, &lstate)) {
            break;
        }
        if (strcmp("DTC-DEVICE:", lstate.key) == 0) {
            if (devices == 0)
                strcpy(*dtcdevs, lstate.p1);
            else
                strcpy(*dtcdevs+(devices*MAXPATHLEN)+1, lstate.p1);
            devices++;
            *dtcdevs = ftdrealloc(*dtcdevs, (devices+1)*MAXPATHLEN);
        }
    }
    *numdevs = devices;
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    fclose(fd);
#elif defined(SOLARIS)
    close(fd);
#endif
    return 0;
}

/***********************************************************************
  setdrvrtunables - sets the parameters that the driver needs to know
                    about and also sets the driver lg state buffer to
                    contain tunables as a means for IPC of tunables 
                    between throtd and the PMD
**********************************************************************/
int setdrvrtunables (int lgnum, tunable_t *tunables) 
{
    int master, err;
    stat_buffer_t statbuf;
    
     /* open the master ftd device */
    if ((master = open(FTD_CTLDEV, O_RDWR)) < 0) {
        reporterr(ERRFAC, M_CTLOPEN, ERRCRIT,
            argv0, strerror(errno), FTD_CTLDEV);
        DPRINTF("Error: setdrvrtunables - Failed to open master device: %s.\n",FTD_CTLDEV);
        return (-1);
    }
     
    /* set sync mode depth in driver */
    if (tunables->syncmode != 0) {
        if (set_sync_depth(master, lgnum, tunables->syncmodedepth)!=0) {
            close(master);
            reporterr(ERRFAC, M_SETFAIL, ERRCRIT, "syncdepth in driver", lgnum);
            return (-1);
        }
    } else {
        if (set_sync_depth(master, lgnum, -1)!=0) {          
            reporterr(ERRFAC, M_SETFAIL, ERRCRIT, "syncdepth -1 in driver", lgnum);
            close(master);
            return (-1);
        }
    }

    /* set syncmodetimeout in driver */
    if (set_sync_timeout(master, lgnum, tunables->syncmodetimeout)!=0) {
        reporterr(ERRFAC, M_SETFAIL, ERRCRIT,
                  "sync mode timeout in driver", lgnum);
        close(master);
        return (-1);
    }
    if (tunables->use_lrt) {
        if (set_lrt_mode (master, lgnum)) {
	   reporterr (ERRFAC, M_SETFAIL, ERRCRIT, "LRT as on", lgnum);
           close(master);
           return  (-1);
        }
    } else {
        if (set_lrt_bits (master, lgnum)) {
           reporterr (ERRFAC, M_SETFAIL, ERRCRIT, "LRT as off", lgnum);
           close(master);
           return (-1);
        }
    }

    /* make driver memory match pstore */
    memset(&statbuf, 0, sizeof(stat_buffer_t)); 
    statbuf.lg_num = lgnum;
    statbuf.len = FTD_PS_DEVICE_ATTR_SIZE;
    statbuf.addr = (ftd_uint64ptr_t)(ulong)tunables;

    /* set these tunables in driver memory */
    if ((err = FTD_IOCTL_CALL(master, FTD_SET_LG_STATE_BUFFER, &statbuf)) < 0) {
        reporterr(ERRFAC, M_SETFAIL, ERRCRIT, "state buffer", lgnum);
        close(master);
        return (-1);
    }
    close(master);
    return 0;
}

/***********************************************************************
   set_tunables_from_buf - sets tunable struct from pstore buffer        
**********************************************************************/
int set_tunables_from_buf(char *buffer, tunable_t *tunables, int size, int validate_key, int *net_analysis_duration)
{
    static char *key = NULL;
    static char *value = NULL;
    static int bufcpylen = 0;
    static char *bufcpy = NULL;
    char *save_bufcpy;
    int retval = 0;

    if (key == NULL) {
        key = ftdmalloc(255);
    }
    if (value == NULL) {
        value = ftdmalloc(255);
    }
    if (size > bufcpylen) {
        bufcpylen = size;
        bufcpy = (char*)ftdrealloc(bufcpy, bufcpylen);
    }
    memcpy(bufcpy, buffer, size);
    save_bufcpy = bufcpy;

    while (getbufline(&bufcpy, &key, &value, '\n')) {
        if (verify_and_set_tunable(key, value, tunables, validate_key, net_analysis_duration)!=0) {
            retval = -1;
        }
    }
    bufcpy = save_bufcpy;
    return retval;
}

/***********************************************************************
   verify_and_set_tunable -
       verifies that a tunable is within a specified
       range, then sets that tunable in the tunable struct
**********************************************************************/
int verify_and_set_tunable(char *key, char *value, tunable_t *tunables, int validate_key, int *net_analysis_duration)
{
   
    long    longval; 
    char    *ep;

    /* FIXME: these verifications are also done in throttle.c - the max and
       min values need to be be shared through defines */
    if (strcmp("CHUNKSIZE:", key) == 0) {
        errno=0;                                    /*for WR14776 & WR14777*/
        longval=strtol(value, &ep, 10);
        if(errno == EINVAL || errno == ERANGE || 
        ep == value || *ep != '\0' ||              /*checking non numeric values*/
        longval < 64 || longval > 16384) {
            reporterr(ERRFAC, M_CHUNKSZ, ERRCRIT, value, 16384, 64);
            return -1;
        }
        tunables->chunksize = longval * 1024;
    } else if (strcmp("STATINTERVAL:", key) == 0) {
        errno=0;
        longval = strtol(value, &ep, 10);        
        if(errno == EINVAL || errno == ERANGE ||
        ep == value || *ep != '\0' ||
        longval < 1 || longval > 86400) {
            reporterr(ERRFAC, M_STATINT, ERRCRIT, value, 86400, 1);
            return -1;
        }
        tunables->statinterval = longval;
    } else if (strcmp("MAXSTATFILESIZE:", key) == 0) {
        errno=0;
        longval = strtol(value, &ep, 10);
        if(errno == EINVAL || errno == ERANGE ||
        ep == value || *ep != '\0' ||
        longval > 32000 || longval < 1){
            reporterr(ERRFAC, M_STATSIZE, ERRCRIT, value, 32000, 1);
            return -1;
        }
        tunables->maxstatfilesize = longval * 1024;
    } else if (strcmp("SYNCMODEDEPTH:", key) == 0) {
        errno=0;
        longval = strtol(value, &ep, 10);
        if(errno == EINVAL || errno == ERANGE ||
        ep == value || *ep != '\0' ||
        longval < 1 || longval > INT_MAX) {
            reporterr(ERRFAC, M_SYNCDEPTH, ERRCRIT, value, INT_MAX, 1);
            return -1;
        }
        tunables->syncmodedepth = longval;
    } else if (0 == strcmp("SYNCMODETIMEOUT:", key)) {
        errno=0;
        longval = strtol(value, &ep, 10);
        if(errno == EINVAL || errno == ERANGE ||
        ep == value || *ep != '\0' ||
        longval < 1 || longval > 86400) {
            reporterr(ERRFAC, M_SYNCTIMEOUT, ERRCRIT, value, 86400, 1);
            return -1;
        }
        tunables->syncmodetimeout = longval;
    } else if (strcmp("SYNCMODE:", key) == 0) {
        if (strcmp (value, "on") == 0 || strcmp (value, "ON")==0 || strcmp (value, "1") == 0) {
            tunables->syncmode = 1;
        } else if (strcmp(value, "off") == 0 || strcmp(value, "OFF") == 0 || strcmp (value, "0") == 0) {
            tunables->syncmode = 0;
        } else {
            reporterr(ERRFAC, M_SYNCVAL, ERRCRIT);
            return -1;
        }
    } else if (strcmp("TRACETHROTTLE:", key) == 0) {
        if (strcmp (value, "on") == 0 || strcmp (value, "ON")==0 || strcmp (value, "1") == 0) {
            tunables->tracethrottle = 1;
        } else if (strcmp(value, "off") == 0 || strcmp(value, "OFF") == 0 || strcmp (value, "0") == 0) {
            tunables->tracethrottle = 0;
        } else {
            reporterr(ERRFAC, M_TRACEVAL, ERRCRIT);
            return -1;
        }
    } else if (strcmp("COMPRESSION:", key) == 0) {
        if (strcmp (value, "on") == 0 || strcmp (value, "ON")==0 || strcmp (value, "1") == 0) {
            tunables->compression = 1;
        } else if (strcmp(value, "off") == 0 || strcmp(value, "OFF") == 0 || strcmp (value, "0") == 0) {
            tunables->compression = 0;
        } else {
            reporterr(ERRFAC, M_COMPRESSVAL, ERRCRIT);
            return -1;
        }
    } else if (strcmp("_MODE:", key) == 0) {
    } else if (strcmp("_AUTOSTART:", key) == 0) {
    } else if (strcmp("NETMAXKBPS:", key) == 0) {
        errno=0;
        longval = strtol(value, &ep, 10);
        if(errno == EINVAL || errno == ERANGE ||
        ep == value || *ep != '\0' ||
        (longval != -1 && (longval<1 || longval > INT_MAX))) {
            reporterr(ERRFAC, M_NETMAXERR, ERRCRIT, value, INT_MAX, 1, -1);
            return -1;
        }
        tunables->netmaxkbps = longval;
    } else if (strcmp("CHUNKDELAY:", key) == 0) {
        errno=0;
        longval = strtol(value, &ep, 10);
        if(errno == EINVAL || errno == ERANGE ||
        ep == value || *ep != '\0' ||
        longval < 0 || longval > 999){
            reporterr(ERRFAC, M_CHUNKDELAY, ERRCRIT, value, 999, 0);
            return -1;
        }
        tunables->chunkdelay = longval;
    } else if (strcmp("JOURNAL:", key) == 0) {
        if ((strcmp(value, "on") == 0) ||
            (strcmp(value, "ON") == 0) ||
            (strcmp(value, "1") == 0)) {
            tunables->use_journal = 1;
        } else if ((strcmp(value, "off") == 0) ||
                   (strcmp(value, "OFF") == 0) ||
                   (strcmp(value, "0") == 0)) {
            tunables->use_journal = 0;
        } else {
            reporterr(ERRFAC, M_JOURNALVAL, ERRCRIT);
            return -1;
        }
    }  else if (strcmp("LRT:", key) == 0) { 
        if ((strcmp (value, "on") == 0) ||
            (strcmp (value, "ON") == 0) ||
            (strcmp (value, "1") == 0)) {
            tunables->use_lrt = 1;
        } else if ((strcmp (value, "off") == 0) ||
                   (strcmp (value, "OFF") == 0) ||
                   (strcmp (value, "0") == 0)) {
            tunables->use_lrt = 0;
        } else {
            reporterr (ERRFAC, M_LRTVAL, ERRCRIT);
            return -1;
        }
    } else if( (strcmp("NUMSECONDS:", key) == 0) && (net_analysis_duration != NULL) ) {
        errno=0;
        longval = strtol(value, &ep, 10);
        if(errno == EINVAL || errno == ERANGE || ep == value || *ep != '\0' || longval <= 0) {
            reporterr(ERRFAC, M_NUMSECONDS_ERR, ERRCRIT);
            return -1;
        }
		*net_analysis_duration = (int)longval;
	}
    else
    {
       if( validate_key ) 
           return -1;
    }
    return 0;
}
/***********************************************************************
  settunable -  sets a tunable paramter given a <KEY>: and value.
                flag = 0x11 - set both driver & pstore
                flag = 0x01 - set driver only
                flag = 0x10 - set pstore only
**********************************************************************/
int settunable (int lgnum, char *inkey, char *invalue, int flag) 
{
    char  *inbuffer = NULL;
    char  *temp = NULL;
    char  *outbuffer = NULL;
    char  group_name[MAXPATHLEN];
    char  line[MAXPATHLEN];
    char  ps_name[MAXPATHLEN];
    int   num_ps;
    int   ret = 0;
    int  i, j, found, linelen, len;
    tunable_t tunables;
    ps_version_1_attr_t attr;
    char *ps_key[FTD_MAX_KEY_VALUE_PAIRS];
    char *ps_value[FTD_MAX_KEY_VALUE_PAIRS];
    int f_driver = 0x01;
    int f_pstore = 0x10;


    ftd_trace_flow(FTD_DBG_FLOW1, "key: %s, val: %s\n", inkey, invalue);

    if (inkey[strlen(inkey) - 1] != ':') {
        reporterr(ERRFAC, M_BADKEY, ERRCRIT, inkey, "settunable");
        EXIT(EXITANDDIE);
    }

    FTD_CREATE_GROUP_NAME(group_name, lgnum);

    if (GETPSNAME(lgnum, ps_name) != 0) {
        i = numtocfg(lgnum);
        sprintf(line, "%s/p%03d.cfg", PATH_CONFIG, lgnum);
        reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, line);
        EXIT(EXITANDDIE);
    }

    /* read the persistent store for info on this group */
    ret = ps_get_version_1_attr(ps_name, &attr, 1);
    if (ret != PS_OK) {
        reporterr(ERRFAC, M_PSRDHDR, ERRCRIT, ps_name);
        EXIT(EXITANDDIE);
    }

    /* allocate an input buffer and output buffer for the parameters */
    if (((inbuffer = (char *)malloc(attr.group_attr_size)) == NULL) ||
        ((outbuffer = (char *)calloc(attr.group_attr_size, 1)) == NULL)) {
        reporterr(ERRFAC, M_MALLOC, ERRCRIT, attr.group_attr_size);
        EXIT(EXITANDDIE);
    }
    ret = ps_get_group_attr(ps_name, group_name, inbuffer, attr.group_attr_size);
    if (ret != PS_OK) {
        reporterr(ERRFAC, M_PSRDGATTR, ERRCRIT, group_name, ps_name, ret);
        EXIT(EXITANDDIE);
    }

    /* parse the attributes into key/value pairs */
    temp = inbuffer;
    num_ps = 0;
    while (getbufline(&temp, &ps_key[num_ps], &ps_value[num_ps], '\n')) {
        num_ps++;
    }

    /* add/replace the key/value pairs */
  
    found = FALSE;
    for (j = 0; j < num_ps; j++) {
        if (strcmp(inkey, ps_key[j]) == 0) {
            /* replace value */
            ps_value[j] = invalue;
            found = TRUE;
            break;
        }
    }
    /* add key/value */
    if (!found) {
        ps_key[num_ps] = inkey;
        ps_value[num_ps] = invalue;
        num_ps++;
    }
   
    /* create a new buffer */
    len = 0;
    for (i = 0; i < num_ps; i++) {
        sprintf(line, "%s %s\n", ps_key[i], ps_value[i]);
        linelen = strlen(line);
        strncpy(&outbuffer[len], line, linelen);
        len += linelen;
    }
    outbuffer[len] = '\0';

    /* get current driver tunables */
    if ((ret = getdrvrtunables(lgnum, &tunables)) < 0) {
        goto settunableexit;
    }
    /* parse them and set the appropriate tunable fields */
    if ((ret = set_tunables_from_buf(outbuffer, 
                                     &tunables, 
                                     attr.group_attr_size, 1, NULL)) != 0) {
        goto settunableexit;
    }

    /* set the driver specific tunables and driver buffer area for 
       throtd to pmd IPC */
    if (flag & f_driver) {
        if ((ret = setdrvrtunables(lgnum, &tunables)) != 0) {
            goto settunableexit;
        }
    }

    if (flag & f_pstore) {
        /* dump new buffer to the ps */
        if ((ret = ps_set_group_attr(ps_name, 
                                     group_name, 
                                     outbuffer, 
                                     attr.group_attr_size)) != PS_OK) {
            goto settunableexit;
        }
    }

settunableexit: 
    if (inbuffer) {
        free(inbuffer);
    }

    if (outbuffer) {
        free(outbuffer);
    }

    return ret; 
}

/***************************************************************************
 * getdrvrtunables - get tunables from driver 
 **************************************************************************/
int
getdrvrtunables(int lgnum, tunable_t *tunables)
{
    char *lgbuf;
    stat_buffer_t statbuf;
    int master;
    int err;

    /* open the master control device */
    if ((master = open(FTD_CTLDEV, O_RDWR)) < 0) {
        DPRINTF("Error: getdrvrtunables - Failed to open master device: %s.\n",FTD_CTLDEV);
        return -1;
    }
    lgbuf = ftdmalloc(FTD_PS_GROUP_ATTR_SIZE);

    memset(&statbuf, 0, sizeof(stat_buffer_t)); 
    statbuf.lg_num = lgnum;
    statbuf.len = FTD_PS_GROUP_ATTR_SIZE;
    statbuf.addr = (ftd_uint64ptr_t)(ulong)lgbuf;

    /* read from driver buffer */
    if ((err = FTD_IOCTL_CALL(master, FTD_GET_LG_STATE_BUFFER, &statbuf)) < 0) {
        DPRINTF("Error: getdrvrtunables - Failed to get group state.\n");
        free(lgbuf);
        close(master);
        return -1;
    }
    memcpy(tunables, (const void *)(ulong)statbuf.addr, sizeof(tunable_t));
    free(lgbuf);
    close(master);

    return 0;
} /* getdrvrtunables */

/***************************************************************************
 * gettunables_from_net_analysis_file - get tunables from the parameter
 * file set by launchnetanalysis script.
 * These are the tunables that are monitored through dtcset in standard modes.
 **************************************************************************/
int gettunables_from_net_analysis_file (int *net_analysis_duration)
{
    char *retbuffer;              /* buffer to hold tunables */
    char *buf_ptr;
    struct stat statbuf;
    char filename[MAXPATHLEN];
	int  fd;
	int  buff_len;

	// Read the network analysis parameter file containing the tunables
	sprintf(filename, "%s/SFTKdtc_net_analysis_parms.txt", PATH_CONFIG);

    if( stat(filename, &statbuf) != 0 )
	    return( -2 );

    buff_len = statbuf.st_size +16;
    retbuffer = (char *) ftdmalloc(buff_len*sizeof(char));

	if ((fd = open(filename, O_RDONLY)) == -1) {
        free(retbuffer);
		return( -3 );
	}
	if (read(fd, retbuffer, statbuf.st_size) != statbuf.st_size) {
        free(retbuffer);
		close(fd);
		return( -4 );
	}
	close(fd);

    buf_ptr = retbuffer;

    /* parse them and set the appropriate machine_t fields */
    if( set_tunables_from_buf(buf_ptr, &(sys[0].tunables), buff_len, 0, net_analysis_duration) != 0 )
	{
        free(retbuffer);
		return( -5 );
	}

    memcpy(&(sys[1].tunables), &(sys[0].tunables), sizeof(tunable_t));
    free(retbuffer);

    return 1;
}

/***************************************************************************
 * gettunables - get tunables from persistent store
 * These are the tunables that can be monitored through dtcset
 **************************************************************************/
int gettunables (char *group_name, int psflag, int quiet)
{
    int lgnum;
    int buff_len;                 
    int rc, err;
    int master;
    ps_version_1_attr_t attr;    /* persistent store header */
    char *retbuffer;              /* buffer to hold tunables */
    char *buf_ptr;
    tunable_t tunables;
    ftd_lg_info_t lginfo;
    stat_buffer_t statbuf;
    char ps_name[MAXPATHLEN];

    if( _net_bandwidth_analysis )
	{
        if ((rc = gettunables_from_net_analysis_file( NULL )) < 0 )
		{
            reporterr(ERRFAC, M_NETTUNABLE_ERR, ERRCRIT, PATH_CONFIG, "SFTKdtc_net_analysis_parms.txt", rc );
		    return( -1 );
		}
		else
		{
		    return( 1 );
		}
	}

    lgnum = cfgpathtonum(sys[0].configpath);

    /* open the system control device */
    if ((master = open(FTD_CTLDEV, O_RDWR)) < 0) {
        reporterr(ERRFAC, M_CTLOPEN, ERRCRIT,
            argv0, strerror(errno), FTD_CTLDEV);
        DPRINTF("Error: gettunables - Failed to open master device: %s.\n",FTD_CTLDEV);
        return(0);
    }

    if (psflag) {
        if (GETPSNAME(lgnum, ps_name) !=0 ) {
            sprintf(ps_name, "%s/p%03d.cfg", PATH_CONFIG, lgnum);
            reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, ps_name);
            return -1;
        }
        /* get header info */
        if ((rc = ps_get_version_1_attr(ps_name, &attr, 1))!=PS_OK) {
            reporterr(ERRFAC, M_PSRDGATTR, ERRCRIT, group_name, ps_name, rc);
            close(master);
            return -1;
        }
        
        buff_len = attr.dev_attr_size;
        
        retbuffer = (char *) ftdmalloc(buff_len*sizeof(char));
        
        /* get the tunables */
        if ((rc = ps_get_group_attr(ps_name, group_name, retbuffer, buff_len)) != PS_OK) {
            reporterr(ERRFAC, M_PSRDGATTR, ERRCRIT, group_name, ps_name, rc);
            free(retbuffer);
            close(master);
            return -1;
        }
        buf_ptr = retbuffer;

        /*
         * WR#34655: 
         * bring the pstore tunables up to date, fixing upgrading issue
         */
        if ((rc = inittunables(ps_name, group_name, buf_ptr, buff_len)) < 0) {
            if (!quiet) {
                reporterr(ERRFAC, M_PSERROR, ERRWARN, "SET_PS_TUNABLES", 
                          strerror(errno));
            }
            return (-1);
        }

        /* get current driver tunables */
        getdrvrtunables(lgnum, &sys[0].tunables);

        /* parse them and set the appropriate machine_t fields */
        set_tunables_from_buf(buf_ptr, &(sys[0].tunables), buff_len, 1, NULL);
        memcpy(&(sys[1].tunables), &(sys[0].tunables), sizeof(tunable_t));

        /* make driver memory match pstore */
        memset(&statbuf, 0, sizeof(stat_buffer_t)); 
        statbuf.lg_num = lgnum;
        statbuf.len = FTD_PS_GROUP_ATTR_SIZE;
        statbuf.addr = (ftd_uint64ptr_t)&sys[0].tunables;

        /* set these tunables in driver memory */
        if ((err = FTD_IOCTL_CALL(master, FTD_SET_LG_STATE_BUFFER, &statbuf)) < 0) {
            if (!quiet) {
                reporterr(ERRFAC, M_DRVERR, ERRWARN, "SET_LG_STATE_BUFFER", 
                          strerror(errno));
            }
            close(master);
            return -1;
        }
        free(retbuffer);
    } else {
        if (getdrvrtunables(lgnum, &tunables) < 0) {
            close(master);
            return -1;
        }
        memcpy(&sys[0].tunables, &tunables, sizeof(tunable_t));
        memcpy(&sys[1].tunables, &tunables, sizeof(tunable_t));
    }
    close(master);
    return 1;
}

/***************************************************************************
 * stringcompare -- compare two strings for qsort
 **************************************************************************/
int stringcompare (const void* p1, const void* p2) {
    return (strcmp((char*)p1, (char*)p2));
}
/***************************************************************************
 * isthime4 - figures out if a hostname and/or IP address is this machine
 *             1 = this hostname / ip addr pertains to this machine i.e. NAMEIPFOUND
 *             0 = this hostname / ip addr does not pertain to this machine i.e. NAMEIPNOTFOUND
 *            -1 = unresolvable host name i.e. UNRESOLVNAME
 *            -2 = unresolvable IP addr  i.e.  UNRESOLVIPADDR
 *            -3 = hostname and IP addr conflict i.e. NAMEIPADDRCONFLICT
 *        this function requires that getnetconfcount and getnetconfs has
 *        been previously called.
 **************************************************************************/

int
isthisme4 (char* hostnodename, u_long ip)
{
    int nameretval;
    int ipretval;
    char **p;
    char **q;
    struct in_addr in;
    u_long hostnameip;
    int j;
    int hp;
    struct hostent *lp; 
    nameretval = -2;
    ipretval = -2;
    hostnameip = 0L;
    
    if ((hostnodename == (char*) NULL || strlen(hostnodename) == 0) &&
        ip == 0L) return (NAMEIPADDRCONFLICT);
    if (ip == 0xffffffff) return(UNRESOLVIPADDR);
    /* -- check out the node name, if it exists */
    if (hostnodename != (char*) NULL && strlen(hostnodename) > 0) {
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
        lp = gethostbyname(hostnodename);
#elif defined(SOLARIS)  /* 1LG-ManyDEV */        
        GETHOSTBYNAME_MANYDEV(hostnodename, lp, dummyfd);
#endif
        if (lp == NULL) 
            return (UNRESOLVNAME);
        nameretval = 0;
  /* -- get ip addresses for this machine */
    myipcount = 0;
    myipcount = getnetconfcount();
    myips = (u_long*) NULL;
    if (myipcount > 0) {
        myips = (u_long*) malloc ((sizeof(u_long)) * myipcount);
        getnetconfs (myips);
    }

    /* -- check out the node name, if it exists */
        (void) memcpy(&in.s_addr, *(lp->h_addr_list), sizeof(in.s_addr));
        if (0 == strcmp(hostnodename, lp->h_name)) {
            hostnameip = in.s_addr;
            for (j=0; j<myipcount; j++) {
                if (myips[j] == in.s_addr) {
                    nameretval = 1;
                    break;
                }
            }
        } else {
            for (p = lp->h_addr_list; *p != 0; p++) {
                (void) memcpy(&in.s_addr, *p, sizeof(in.s_addr));
                for (q = lp->h_aliases; *q != 0; q++) {
                    if (0 == strcmp(*q, hostnodename)) {
                        nameretval = 0;
                        hostnameip = in.s_addr;
                        for (j=0; j<myipcount; j++) {
                            if (myips[j] == in.s_addr) {
                                nameretval = 1;
                                break;
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
    /* check out the ip number, if given */
    if (ip != 0L) {
#if defined(HPUX) || defined(_AIX) || defined(linux)
        lp = gethostbyaddr ((char*)&ip, sizeof(ip), AF_INET);
#elif defined(SOLARIS)  /* 1LG-ManyDEV */
        GETHOSTBYADDR_MANYDEV((char*)&ip, sizeof(ip), AF_INET, lp, dummyfd);
#endif
        if (lp == NULL) {
            return (UNRESOLVIPADDR);
        } else {
            ipretval = 0;
            for (j=0; j<myipcount; j++) {
                if (myips[j] == ip) {
                    ipretval = 1;
                    break;
                }
            }
        }
    }
    if (nameretval == 0 && ipretval == 0) {
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
        lp = gethostbyname(hostnodename);
#elif defined(SOLARIS)  /* 1LG-ManyDEV */
        GETHOSTBYNAME_MANYDEV(hostnodename, lp, dummyfd);
#endif
        (void) memcpy(&in.s_addr, *(lp->h_addr_list), sizeof(in.s_addr));
        if (ip != in.s_addr) 
            return (NAMEIPADDRCONFLICT);
    }
    free(myips);
    if (nameretval == -2 && ipretval == -2)
        return (NAMEIPADDRCONFLICT);
    if (nameretval != -2 && ipretval == -2)
        return (nameretval);
    if (nameretval == -2 && ipretval != -2)
        return (ipretval);
    if (nameretval != ipretval)
        return (NAMEIPADDRCONFLICT);
    return (nameretval);
  }
/*------Equivalent function as isthisme for IPv6 support*/
/***************************************************************************
 * isthime6 - figures out if a hostname and/or IP address is this machine
 *             1 = this hostname / ip addr pertains to this machine i.e. NAMEIPFOUND
 *             0 = this hostname / ip addr does not pertain to this machine i.e. NAMEIPNOTFOUND
 *            -1 = unresolvable host name i.e. UNRESOLVNAME
 *            -2 = unresolvable IP addr  i.e.  UNRESOLVIPADDR 
 *            -3 = hostname and IP addr conflict i.e. NAMEIPADDRCONFLICT
 *        this function requires that getnetconfcount and getnetconfs has
 *        been previously called.
 *  This function is an equivalent function for isthisme which can handle both
 *  IPv4 as well as IPv6 addresses.   
 **************************************************************************/
#if !defined(FTD_IPV4)
int
isthisme6 (char* hostnodename,char* ipv)
{
    char tempport[30];
    int nameretval;
    int ipretval;
    struct addrinfo hints,*res,*ap;
    int j;
    int err;
    char *tmpmyip6;
    char in6str[INET6_ADDRSTRLEN];
    nameretval = -2;
    ipretval = -2;
    
    if ((hostnodename == (char*) NULL || strlen(hostnodename) == 0) &&
        ipv == (char*) NULL) return (NAMEIPADDRCONFLICT);
    if (ipv == (char*)0xffffffff) return(UNRESOLVIPADDR);

  /* -- get ip addresses for this machine */
    myip6count = 0;
    myip6count= getnetconfcount6();
    myip6 = (char*) NULL;
    if (myip6count > 0) {
         myip6 = (char*) malloc ((sizeof(char)) * myip6count * INET6_ADDRSTRLEN);
         getnetconfs6(myip6);
    }

    /* -- check out the node name, if it exists */
    if (hostnodename != (char*) NULL && strlen(hostnodename) > 0) {
       memset(&hints, '\0', sizeof(struct addrinfo));
       hints.ai_flags = AI_CANONNAME;
#if defined(linux)
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
       hints.ai_socktype = SOCK_STREAM;
#endif
#endif

       sprintf(tempport,"%d",othersys->secondaryport);
       err = getaddrinfo(hostnodename,tempport, &hints, &res);

       if (err!=0) {
	     /* First effort failed, try with NULL port */
	     err = getaddrinfo(hostnodename,NULL, &hints, &res);
	     if (err!=0) {
		 reporterr(ERRFAC,M_NAMRESOLVFAIL,ERRWARN,gai_strerror(err)); 
		 return (UNRESOLVNAME);
	     }
       }
        nameretval = 0;
        if (0 == strcmp(hostnodename, res->ai_canonname)) {
                    nameretval = 1;
                 }
        else {
            for (ap = res; ap != NULL; ap=ap->ai_next){
                 inet_ntop(res->ai_family,
			   &((struct sockaddr_in6 *)res->ai_addr)->sin6_addr,
			   in6str, sizeof(in6str));
                 ipretval = 0;
 		 tmpmyip6 = myip6;
                 for (j=0;j<myip6count;j++) {
                      if ( 0 == strcmp((char*)in6str,tmpmyip6)) {
                      ipretval = 1;
                      break;
                      }
                      tmpmyip6=tmpmyip6+INET6_ADDRSTRLEN;
                   }
                   if (ipretval == 1) { nameretval = 1; break; }

            }
	}      
    }
    free(myip6);
    if (nameretval == -2 && ipretval == -2)
        return (NAMEIPADDRCONFLICT);
    if (nameretval != -2 && ipretval == -2)
        return (nameretval);
    if (nameretval == -2 && ipretval != -2)
        return (ipretval);
    if (nameretval != ipretval)
        return (NAMEIPADDRCONFLICT);

    freeaddrinfo(res);
    return (nameretval);
}
#endif


// Function to determine if we are running on RHEL7
// Return: 1 if yes; 0 if not
int check_if_RHEL7()
{
    char cmdline[128];

	memset( cmdline, 0, sizeof(cmdline) );
	sprintf( cmdline, "/bin/grep \"release 7\" /etc/redhat-release 1> /dev/null 2>&1" );
	if (system(cmdline) == 0)
	{
		return( 1 );
	}
	else
	{
	    return( 0 );
	}
}

/***************************************************************************
 * getconfigs -- returns a sorted list of the configuration files.
 *               paths must be a character array of 256x32 entries.
 *               0 = no configuration files found
 *               >0 = number of configuration files found
 *
 *               who should be 1 for primary and 2 for secondary users.
 *               startflag should be 1 to check for group-started pstore flag 
 **************************************************************************/
#ifdef TDMF_TRACE
int getconfigs (char* mod,char *funct,char paths[][32], int who, int startflag) 
#else
int getconfigs (char paths[][32], int who, int startflag) 
#endif
{
  DIR* dfd;
  struct dirent* dent;
  char line[512];
  char cfg_path_suffix[4];
  char value_str[32];
  char group_name[MAXPATHLEN];
  char ps_name[MAXPATHLEN];
  int i;
  int j;
  int k;
  int rc;
  int started_lgs[MAXLG];
  int numlgs;
  int started;
  int family;
  int max_wait, time_waited;
  int IsRHEL7;  

  ftd_trace_flow(FTD_DBG_FLOW3, 
          "DTC %s/%s() getconfigs(): w:%d start:%d ", mod,funct,who,startflag);        
  i = 0;

 /* set the prefix and suffix for the config files */
  if (startflag==1) {
    strcpy(cfg_path_suffix, "cur");
  } else {
    strcpy(cfg_path_suffix, PATH_CFG_SUFFIX);
  }

  /* return a sorted list of started logical group config files */
  if ((startflag == 1) && (who == 1)) {

    /* get the current list of started logical groups */
    numlgs = MAXLG;
    if (get_started_groups_list(started_lgs, &numlgs))
        return 0;

    for (i=0; i<numlgs; i++) {
      snprintf(paths[i], sizeof(paths[i]), "p%03d.cur", started_lgs[i]);
    }
    paths[numlgs][0] = '\0';
    return(numlgs);
  }

  /* looking for .cfg files, walk the directory */
  dfd = opendir(PATH_CONFIG);
  if (dfd == (DIR*)NULL) {
    if (errno == EMFILE) {
      reporterr(ERRFAC, M_FILE, ERRCRIT, PATH_CONFIG, strerror(errno));
    }
    sprintf(line, "[error opening directory %s]", PATH_CONFIG);
    reporterr (ERRFAC, M_CFGFILE, ERRCRIT,
               argv0, line, strerror(errno));
    EXIT(EXITANDDIE);
  }
  while (NULL != (dent = readdir(dfd))) {
    if (8 != strlen(dent->d_name)) {
      continue;
    }
    if (0 == (strncmp(&(dent->d_name[5]), cfg_path_suffix, 3))) { 
      if (who == 1) {
        /***********************************************************/
        /* Mod for dtcstart -ba. When system reboot, auto start    */
        /* all LG re-start. 02/10/31 by pst                        */
        /***********************************************************/
        if (startflag) {    /* startflag == 2 */
          if (0 == strncmp(dent->d_name, PMD_CFG_PREFIX, strlen(PMD_CFG_PREFIX))) {
            /* conversion device file name to lgnum (atoi) */
            for (k = 1, lgnum = 0; k < 4; k++){
                if (isdigit(dent->d_name[k])) {
                      lgnum *= 10;
                      lgnum += (int)(dent->d_name[k] - '0');
                }
             }
	     k = getautostart(lgnum);
	     if (k > 0) {
		 FTD_CREATE_GROUP_NAME(group_name, lgnum);
		 if (GETPSNAME(lgnum,ps_name) ==  -1) {
		    sprintf(group_name,"%s/%s/%03d.cfg", PATH_CONFIG,PMD_CFG_PREFIX, lgnum);
		    reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, group_name);
		    return -1;
		 }
		 /* serch the LG that have _AUTOSTART:(yes/no) in Pstore */
		 // NOTE: on RHEL 7, we have seen situations at reboot where dtcstart -ab is called before the Pstores are visible (defect 74194)
		 // This is why a retry loop in inserted here if we are on RHEL7.
		 IsRHEL7 = 0;
#if defined(linux)
         IsRHEL7 = check_if_RHEL7();
#endif
        if( IsRHEL7 )
		{

	         max_wait = 120;
	         time_waited = 0;
	         rc = PS_BOGUS_PS_NAME;   // Error code when cannot open the Pstore
	         while( (rc == PS_BOGUS_PS_NAME) && ( time_waited < max_wait) )
	         {  
				 rc = ps_get_group_key_value(ps_name, group_name, "_AUTOSTART:", value_str);
				 if (rc != PS_OK)
				 {
				    sprintf(debug_msg, "Error reported by ps_get_group_key_value for _AUTOSTART, Pstore = %s, group = %s, error code = %d\n", ps_name, group_name, rc);
				    reporterr (ERRFAC, M_GENMSG, ERRCRIT, debug_msg);
				 }
				 if( rc == PS_BOGUS_PS_NAME )
				 {
				     // Cannot open the Pstore. Retry with timeout.
					 sleep( 5 );
					 time_waited += 5;
				     sprintf(debug_msg, "Will retry opening the Pstore %s (time_waited: %d seconds; max_wait = %d seconds)...\n", ps_name, time_waited, max_wait);
				     reporterr (ERRFAC, M_GENMSG, ERRINFO, debug_msg);
				 } 
			 }
		}
		else  // Non-linux or not Linux release 7 or later
		{
			 rc = ps_get_group_key_value(ps_name, group_name, "_AUTOSTART:", value_str);
			 if (rc != PS_OK)
			 {
			    sprintf(debug_msg, "Error reported by ps_get_group_key_value for _AUTOSTART, Pstore = %s, group = %s, error code = %d\n", ps_name, group_name, rc);
			    reporterr (ERRFAC, M_GENMSG, ERRCRIT, debug_msg);
			 }
		}

		 if (rc == PS_OK) {
		    /* _AUTOSTART:yes means that LG alive before reboot */
		    if (strcmp(value_str, "yes") == 0) {
			strcpy(paths[i++],dent->d_name);
			paths[i][0] = '\0';
		    }
		 }
	     } else if (k == 0) {
		/* remove cur files on boot when autostart is off */
		remove_cfg_cur_files(lgnum, CFG_PMD|CFG_RMD);
	     }
           }
         } else {    /* startflag == 0 */
           if (0 == strncmp(dent->d_name, PMD_CFG_PREFIX, strlen(PMD_CFG_PREFIX))) {
                     strcpy (paths[i++], dent->d_name);
              paths[i][0] = '\0';
           }
         }
      } else if (who == 2) {
        if (0 == strncmp(dent->d_name, RMD_CFG_PREFIX, strlen(RMD_CFG_PREFIX))) {
          strcpy (paths[i++], dent->d_name);
          paths[i][0] = '\0';
          continue;
        }
      } else if (who == 3) {
	      // Check for PMDs running in Network analysis mode (not started in the driver)
          if (0 == strncmp(dent->d_name, PMD_CFG_PREFIX, strlen(PMD_CFG_PREFIX))) {
			  // Check if the cfg file contains the network analysis keyword and if it is on
			  sprintf( line, "%s/%s", PATH_CONFIG, dent->d_name );
              if( is_network_analysis_cfg_file( line ) )
			  {
                  strcpy (paths[i++], dent->d_name);
                  paths[i][0] = '\0';
			  }
          }
	  }
    }
  }
  (void) closedir (dfd);
  qsort ((char*) paths, i, 32, stringcompare);
  return (i);
}

/***************************************************************************
 * initconfigs -- initializes machine data structures prior to processing
 *                configuration files.
 **************************************************************************/
void
initconfigs () 
{
    sys[0].notes[0] = '\0';
    sys[0].role = -1;
    sys[0].configpath[0] = '\0';
    sys[0].tag[0] = '\0';
    sys[0].name[0] = '\0';
    sys[0].hostid = 0L;
    sys[0].procfd = -1;
    sys[0].pcpu = 0;
    sys[0].pcpu_prev = 0;
    sys[0].entrysleep = 0L;
    sys[0].ip = 0L;
    sys[0].ipv = (char*) malloc (sizeof(char) * ADDRSIZE);
    sys[0].secondaryport = secondaryport;
    sys[0].sock = -1;
    sys[0].isconnected = 0;
    sys[0].lgsnmaster = 0;
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    sys[0].statfd = (FILE*) NULL;
#elif defined(SOLARIS)
    sys[0].statfd = 0;
#endif
 
    sys[0].tunables.chunksize = chunksize * 1024;
    sys[0].tunables.statinterval = statinterval;
    sys[0].tunables.maxstatfilesize = maxstatfilesize * 1024;
    sys[0].tunables.tracethrottle = tracethrottle;
    sys[0].tunables.compression = compression;
    sys[0].tsbias = (time_t) 0;
    sys[0].midnight = (time_t) 0;
    sys[0].monlastday = 31;
    sys[0].day = -1;
    sys[0].wday = -1;
    sys[0].throttles = (throttle_t*) NULL;
    sys[0].tkbps = 0;
    sys[0].prevtkbps = 0;
    sys[0].tunables.syncmode = syncmode;
    sys[0].tunables.syncmodedepth = syncmodedepth;
    sys[0].tunables.syncmodetimeout = syncmodetimeout;

    memset(sys[0].group, 0, sizeof(group_t));
    
    sys[1] = sys[0];
}

/***************************************************************************
 * iptoa -- converts a numeric IP address into printable dot notation string 
 **************************************************************************/
char *
iptoa (u_long ip) 
{
    int a1, a2, a3, a4;
    a1 = 0x000000ff & (ip >> 24);
    a2 = 0x000000ff & (ip >> 16);
    a3 = 0x000000ff & (ip >> 8);
    a4 = 0x000000ff & ip;
    sprintf(ipstring, "%d.%d.%d.%d", a1, a2, a3, a4);
    return (ipstring);
}


/***************************************************************************
 * getline -- reads and parses the next line of the config file
 **************************************************************************/
static int 
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
getline (FILE *fd, LineState *ls)
#elif defined(SOLARIS)
getline (int fd, LineState *ls)
#endif
{
    int i, len;
    int blankflag;
    char *line;
    
    if (!ls->invalid) {
        ls->invalid = TRUE;
        return TRUE;
    }
    ls->invalid = TRUE;
    
    ls->key = ls->p1 = ls->p2 = ls->p3 = NULL;
    ls->word[0] = '\0';
    ls->linelen = 0;
    ls->linepos = 0;
    ls->plinepos = 0;
    line = ls->line;
    
    while (1) {
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
        if (fgets(line, 256, fd) == NULL) {
#elif defined(SOLARIS)
        if (ftd_fgets(line, 256, fd) == NULL) {
#endif
            return FALSE;
        }
        
        ls->lineno++;
        len = strlen(line);
        ls->linelen = len;
        if (len < 5) continue;
        
        /* ignore blank lines */
        blankflag = 1;
        for (i = 0; i < len; i++) {
            if (isgraph(line[i])) {
                blankflag = 0;
                break;
            }
        }
        if (blankflag) continue;
        
        strcpy(ls->readline, ls->line);
        
        /* -- get rid of leading whitespace -- */
        i = 0;
        while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
        if (i >= len) continue;

        /* -- if the first non-whitespace character is a "#", ignore the
           line */
        if (line[i] == '#') continue;
        
        /* -- accumulate the key */
        ls->key = &line[i];
        while ((i < len) && (line[i] != ' ') && (line[i] != '\t') && 
            (line[i] != '\n')) i++;
        line[i++] = 0;
        
        /* -- bypass whitespace */
        while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
        if (i >= len) break;
        
        /* -- accumlate first parameter */
        ls->p1 = &line[i];
        while ((i < len) && (line[i] != ' ') && (line[i] != '\t') && 
            (line[i] != '\n')) i++;
        line[i++] = 0;
        
        /* -- bypass whitespace */
        while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
        if (i >= len) break;
        
        /* -- accumlate parameter */
        ls->p2 = &line[i];
        while ((i < len) && (line[i] != ' ') && (line[i] != '\t') && 
            (line[i] != '\n')) i++;
        line[i++] = 0;
        
        /* -- bypass whitespace */
        while ((i < len) && ((line[i] == ' ') || (line[i] == '\t'))) i++;
        if (i >= len) break;
        
        /* -- accumlate parameter */
        ls->p3 = &line[i];
        while ((i < len) && (line[i] != ' ') && (line[i] != '\t') && 
            (line[i] != '\n')) i++;
        line[i++] = 0;
        
        break;
    }
/*
  DPRINTF("LINE(%d): = %s\n", ls->lineno - 1, ls->readline));
  */
    return TRUE;
}

/***************************************************************************
 * get_word -- obtains the next word from the config file's line / file
 *              (and handles continuation characters)
 *
 **************************************************************************/
static void
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
get_word (FILE* fd, LineState* ls)
#elif defined(SOLARIS)
get_word (int fd, LineState* ls)
#endif
{
    int i;
    
    i = 0;
    ls->word[i] = '\0';
    ls->plinepos = ls->linepos;
    while (ls->linepos < ls->linelen) {
        if (isspace(ls->readline[ls->linepos])) {
            ls->linepos++;
        } else {
            break;
        }
    }
    if (ls->linepos < ls->linelen) {
        if (ls->readline[ls->linepos] != '\"') {
            while (ls->linepos < ls->linelen) {
                if (0 == isspace(ls->readline[ls->linepos])) {
                    ls->word[i++] = ls->readline[ls->linepos++];
                    ls->word[i] = '\0';
                } else {
                    break;
                }
            }
        } else {
            /* -- quoted string */
            ls->linepos++;
            while (ls->linepos < ls->linelen) {
                if (ls->readline[ls->linepos] == '\\') {
                    ls->linepos++;
                    if (ls->linepos < ls->linelen) {
                        ls->word[i++] = ls->readline[ls->linepos++];
                        ls->word[i] = '\0';
                    }
                } else {
                    if (ls->readline[ls->linepos] == '\"') {
                        ls->linepos++;
                        break;
                    }
                    ls->word[i++] = ls->readline[ls->linepos++];
                    ls->word[i] = '\0';
                }
            }
        }
    }
    if (0 == strcmp(ls->word, "\\")) {
        i = ls->linepos;
        while (i < ls->linelen) {
            if (0 == isspace(ls->readline[i])) return;
            i++;
        }
        ls->word[0] = '\0';
        if (getline (fd, ls)) {
            get_word (fd, ls);
        }
    }
    return;
}

/***************************************************************************
 * drain_to_eol -- reads remaining words in the logical (continued) line until
 *                 line termination encountered
 **************************************************************************/
static void
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
drain_to_eol (FILE* fd, LineState* ls)
#elif defined(SOLARIS)
drain_to_eol (int fd, LineState* ls)
#endif
{
    while (strlen(ls->word) > 0) 
        get_word (fd, ls);
}

/****************************************************************************
 * parse_value -- parse a throttle test value as a number, word, or a string
 ****************************************************************************/
static int
parse_value (LineState* ls)
{
    int i, j, len;
    int digitflag;
    int value;
    int sign;
    long long longlongval;

    sign = 1;
    ls->valueflag = 0;
    if (0 == (len = strlen(ls->word))) return (0);
    digitflag = 1;
    value = 0;
    longlongval=0;
    j=0;
    while (ls->word[j] == '-') {
        sign = -1;
        j++;
    }
    for (i=j; i<len; i++) {
        if ('0' <= ls->word[i] && '9' >= ls->word[i]) {
            longlongval = (longlongval * 10) + (int) (ls->word[i] - '0');
            if (longlongval * sign > INT_MAX){
                value = INT_MAX;
                break;  
            } else if (longlongval * sign < INT_MIN){
                value = INT_MIN;
                break;
            }
            value = (int)longlongval;
        } else {
            digitflag = 0;
            break;
        }
    }
    if (digitflag) {
        ls->valueflag = 1;
        ls->value = value * sign;
    } else {
        ls->valueflag = 0;
        ls->value = 0;
    }
    return (1);
}

/****************************************************************************
 * parseftddevnum -- return the integer value of the datastar device name
 ****************************************************************************/
int 
parseftddevnum (char* path)
{
    char numstr[32];
    int retval;
    int len;
    int i;
    int starti;
    
    if ((path == (char*)NULL) || ((len = strlen(path)) == 0)) {
        return (-1);
    }
    starti = -1;
    retval = 0;
    for (i=len-1; i>=0; i--) {
        if (isdigit(path[i])) {
            starti = i;
        } else if (isspace(path[i])) {
            continue;
        } else {
            break;
        }
    }
    if (starti == -1) {
        return (-1);
    }
    memset(numstr, 0, sizeof(numstr));
    strcpy(numstr, &path[starti]);
    retval = atoi(numstr);

    return (retval);
}

/***************************************************************************
 * NOTES for parse_??? functions.
 * 1) Set ls->invalid to FALSE and return, if the key is unknown.
 *
 ***************************************************************************/



/***************************************************************************
 * parse_system -- Parse the line passed in. It defines the system type 
 *                 (PRIMARY or SECONDARY) for the subsequent data.
 ***************************************************************************/
static int
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
parse_system(FILE *fd, LineState *ls, int *network_analysis_mode)
#elif defined(SOLARIS)
parse_system(int fd, LineState *ls, int *network_analysis_mode)
#endif
{
    machine_t *currentsys;
    group_t *group;
    char buf6[INET6_ADDRSTRLEN];
    int family;
	int invalid_parameter = 0;
    int i;

    /* we don't read a line; we just parse the one we have */
    if (strcmp("PRIMARY", ls->p2) == 0) {
        currentsys = &sys[0];
        *network_analysis_mode = 0;	 // Set default value in case the key is not there
    } else if (strcmp("SECONDARY", ls->p2) == 0) {
        currentsys = &sys[1];
    } else {
        reporterr(ERRFAC, M_CFGERR, ERRCRIT, sys[0].configpath, ls->lineno);
        return -1; 
    }
    currentsys->name[0] = 0;
    currentsys->hostid = 0;
    currentsys->ip = 0;
    currentsys->ipv = (char*) NULL;

    group = currentsys->group;
    group->sync_depth = (unsigned int) -1;
    FTD_CREATE_GROUP_NAME(group->devname, lgnum);
    group->chaining = 0;
    group->capture_io_of_source_devices = 1;
    group->autostart_on_boot = 1;
    
    // WI_338550 December 2017, implementing RPO / RTT
    // Init RTT control data
    group->RTTComputingControl.State = TRUE;
    group->RTTComputingControl.TimeInterval = RTT_COMPUTE_INTERVAL_SEC;
    group->RTTComputingControl.SequencerOffsetRTT = RTT_COMPUTE_SEQUENCER_OFFSET;
    SequencerTag_Invalidate(&group->RTTComputingControl.iSequencerTag);

    // Init QOS stats
    group->QualityOfService.LastConsistentIOTimestamp = 0;
    // (Feb8) Set this one to a special value to tell the driver to keep its own value; the group may be in tracking and the driver may have set it: 
    group->QualityOfService.OldestInconsistentIOTimestamp = DO_NOT_CHANGE_OLDEST_INCONSISTENT_IO_TIMESTAMP;
    group->QualityOfService.RTT = 0;
    for ( i = 0; i < NUM_OF_RTT_SAMPLES; i++ )
    {
        group->QualityOfService.RTT_samples_for_recent_average[i] = 0;
    }
    group->QualityOfService.current_number_of_RTT_samples = 0;
    group->QualityOfService.current_RTT_sample_index = 0;
    group->QualityOfService.average_of_most_recent_RTTs = 0;

    
    // WI_338550 December 2017, implementing RPO / RTT
    // Init chunck timestamp queue
    group->pChunkTimeQueue = NULL;
    group->pChunkTimeQueue = Chunk_Timestamp_Queue_Create();
    Chunk_Timestamp_Queue_Init(group->pChunkTimeQueue, CHUNK_TIMESTAMP_QUEUE_SIZE);

    while (1) {
        if (!getline(fd, ls)) {
            break;
        }
  
		/* WR PROD6877: verify that the parameter expected with the keyword is not missing in the config file;
		   on SuSE Linux, this situation leads to a segmentation fault.
		*/
		if ( strcmp("JOURNAL", ls->key) != 0 )
		{
		    invalid_parameter = (ls->p1 == NULL);
		    if( !invalid_parameter )
			{
		        invalid_parameter = (strlen(ls->p1) == 0);
			}
		    if( invalid_parameter )
		    {
                reporterr(ERRFAC, M_MISSCFGPARM, ERRCRIT, lgnum, ls->key);
			    return( -1 );
		    } 
		}
        if (strcmp("HOST:", ls->key) == 0) {
            strcpy(currentsys->name, ls->p1);
        } else if (strcmp("PSTORE:", ls->key) == 0) {
            strcpy(group->pstore, ls->p1);
        } else if (strcmp("JOURNAL:", ls->key) == 0) {
            strcpy(group->journal_path, ls->p1);
        } else if (strcmp("JOURNAL", ls->key) == 0) {
            /* 
             * do nothing here
            */
        } else if (0 == strcmp("SECONDARY-PORT:", ls->key )) {
            currentsys->secondaryport = atoi(ls->p1);
        } else if (0 == strcmp("CHAINING:", ls->key )) {
            if ((strcmp("on", ls->p1) == 0 ) || (strcmp("ON",ls->p1)==0) ) {
                group->chaining = 1;
            } else {
                group->chaining = 0;
            }
        } else if (0 == strcmp("DYNAMIC-ACTIVATION:", ls->key )) {
            if ((strcmp("on", ls->p1) == 0 ) || (strcmp("ON",ls->p1)==0) ) {
                group->capture_io_of_source_devices = 1;
            } else {
                group->capture_io_of_source_devices = 0;
            }
        } else if (0 == strcmp("NETWORK-ANALYSIS:", ls->key )) {
            if ((strcmp("on", ls->p1) == 0 ) || (strcmp("ON",ls->p1)==0) ) {
                *network_analysis_mode = 1;
            } else {
                *network_analysis_mode = 0;
            }
        } else if (0 == strcmp("AUTOSTART:", ls->key )) {
            if (strcasecmp("on", ls->p1) == 0 || strcasecmp("yes", ls->p1) == 0
	     || strcasecmp("true", ls->p1) == 0) {
                group->autostart_on_boot = 1;
            } else if (strcasecmp("off", ls->p1) == 0
	            || strcasecmp("no", ls->p1) == 0
	            || strcasecmp("false", ls->p1) == 0) {
                group->autostart_on_boot = 0;
            } else {
                group->autostart_on_boot = 1;
	    }
        } else {
            ls->invalid = FALSE;
            break;
        }
    }

    /* we must have either the name or the IP address for the host */
#if defined(FTD_IPV4)
    if ((strlen(currentsys->name) == 0) && (currentsys->ip == 0)) {
        reporterr(ERRFAC, M_BADNAMIP, ERRCRIT, currentsys->configpath,\
        ls->lineno, currentsys->name, iptoa(currentsys->ip));
        return -1;
       }
#else
   if ((strlen(currentsys->name) == 0) && (currentsys->ipv == 0)) {
       reporterr(ERRFAC, M_BADNAMIP, ERRCRIT, currentsys->configpath ,\
       ls->lineno, currentsys->name,inet_ntop(AF_INET6, &((struct sockaddr_in6 *)currentsys->ipv)->sin6_addr,buf6,sizeof(buf6       )));
       return -1;
      }
#endif
    /* we must have secondary journal path */
    if (currentsys == &sys[1]) {
        if (strlen(group->journal_path) == 0) {
            reporterr(ERRFAC, M_JRNMISS, ERRCRIT, currentsys->configpath,
                ls->lineno);
            return -1;
        }
    }
    return 0;
}

/***************************************************************************
 * parse_profile -- Read zero or more lines of device definitions. The 
 *                  current line passed in is of no use. Read lines until 
 *                  we don't match the key or EOF.
 ***************************************************************************/
static int
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
parse_profile(FILE *fd, LineState *ls)
#elif defined(SOLARIS)
parse_profile(int fd, LineState *ls)
#endif
{
    group_t      *group;
    sddisk_t     disk, *curdev;
    char         *rest;
    int          moreprofiles = TRUE;

    while (moreprofiles) {
        moreprofiles=FALSE;
        memset(&disk, 0, sizeof(sddisk_t));
        disk.dd_major = disk.dd_minor= -1;
        disk.md_major = disk.md_minor= -1;
        group = sys[0].group;
      
        while (1) {
            if (!getline(fd, ls)) {
                break;
            }
            /*  create and link in a group profile to both systems */
            if (0 == strcmp("REMARK:", ls->key)) {
                rest = strtok(ls->readline, " \t");
                rest = strtok((char *)NULL, "\n");
                if (rest != (char *)NULL) {
                  strcpy(disk.remark, rest);
                }
            } else if (0 == strcmp("PRIMARY:", ls->key)) {
                /* do nothing */
            } else if (0 == strcmp("SECONDARY:", ls->key)) {
                /* do nothing */
            } else if (0 == strcmp(CAPQ "-DEVICE:", ls->key) ||
                     0 == strcmp("TDMF-DEVICE:",  ls->key))  {
                /* WR 34858: make sure that TDMF 1.2.9 configuration file
                 * is fine as well.
                 * TDMF-DEVICE name is changed to DTC-DEVICE.
                 */
                strcpy(disk.sddevname, ls->p1);
                disk.devid = parseftddevnum(disk.sddevname);
            } else if (0 == strcmp("DATA-DISK:", ls->key)) {
                strcpy(disk.devname, ls->p1);
            } else if (0 == strcmp("DATA-DEVNO:", ls->key)) {
                if (ls->p1) {
                    disk.dd_minor = atol(ls->p1);
                }
                if (ls->p2) {
                    disk.dd_major = atol(ls->p2);
                }
            } else if (0 == strcmp("MIRROR-DISK:", ls->key)){
                strcpy (disk.mirname, ls->p1);
				// PROD3919 / PROD3973 : reset device fd to -1 and size to 0; may be used for deciding 
				// if a call to disksize() calculation is needed or not to obtain the device size
                // (ex.: cmd_chkconfig())
				disk.mirfd = -1;
				disk.mirsize = 0;
            } else if (0 == strcmp("MIRROR-DEVNO:", ls->key)){
                if (ls->p1 && *ls->p1) {
                    disk.md_minor = atol(ls->p1);
                }
                if (ls->p2 && *ls->p1) {
                    disk.md_major = atol(ls->p2);
                }
            } else if (0 == strcmp("DISK-LIMIT-SIZE:", ls->key)){
                disk.limitsize_multiple = atol(ls->p1);
            } else if (0 == strcmp("PROFILE:", ls->key)){
                moreprofiles=TRUE;
                break;
            } else { /* unknown key */
                reporterr(ERRFAC, M_CFGBDKEY, ERRCRIT, sys[0].configpath, ls->lineno, ls->key);
                ls->invalid = FALSE;
                return -1;
            }
        }
        /* We must have: sddevname, devname, mirname, and secondary journal */
        if (strlen(disk.sddevname) == 0) {
            /* FIXME: add new error message! */
            reporterr(ERRFAC, M_SDMISS, ERRCRIT, sys[0].configpath, ls->lineno);
            return -1;
        }
        if (strlen(disk.mirname) == 0) {
            /* FIXME: add new error message! */
            reporterr(ERRFAC, M_MIRMISS, ERRCRIT, sys[0].configpath, ls->lineno);
            return -1;
        }
        if (strlen(disk.devname) == 0) {
            reporterr(ERRFAC, M_DEVMISS, ERRCRIT, sys[0].configpath, ls->lineno);
            return -1;
        }
        /* add disk to the linked list of disks */
        if ((curdev = (sddisk_t*) calloc(1, sizeof(sddisk_t))) == NULL) {
            reporterr(ERRFAC, M_MALLOC, ERRCRIT, sizeof(sddisk_t));
            return -1;
        }
        group->numsddisks++;
        *curdev = disk;
        
        if (group->headsddisk == NULL) {
            group->headsddisk = group->tailsddisk = curdev;
        } else {
            group->tailsddisk->n = curdev;
            curdev->p = group->tailsddisk;
            group->tailsddisk = curdev;
        }
        sys[1].group->numsddisks = group->numsddisks;
        sys[1].group->headsddisk = group->headsddisk;
        sys[1].group->tailsddisk = group->tailsddisk;
/*
        memcpy(sys[1].group, sys[0].group, sizeof(group_t));
*/
    }
    return 0;
}

/***************************************************************************
 * forget_throttle -- remove a throttle (and subsequent) definition from 
 *                    the linked list of throttles
 **************************************************************************/
static void
forget_throttle (throttle_t* throttle)
{
    throttle_t* t;
    throttle_t* nt;
    if (throttle == (throttle_t*)NULL)
        return;
    if (sys[0].throttles == throttle) {
        sys[0].throttles = (throttle_t*) NULL;
    } else {
        t = sys[0].throttles;
        nt = t->n;
        while (nt != (throttle_t*)NULL) {
            if (nt == throttle) {
                t->n = (throttle_t*)NULL;
                break;
            }
            t = nt;
            nt = t->n;
        }
    }
    free ((void*)throttle);
}

/***************************************************************************
 * parse_throttles -- parse throttle definitions from the config file
 *                    and create the appropriate data structures from them
 **************************************************************************/
int
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
parse_throttles (FILE* fd, LineState *ls) 
#elif defined(SOLARIS)
parse_throttles (int fd, LineState *ls)
#endif
{
    throttle_t *throttle;
    throttle_t *t;
    throttle_t *pt;
    throt_test_t *ttest;
    action_t   *action;
    char       *tok;
    int        i;
    int        state;
    int   implied_do_flag;
    char  keyword[256];
#ifdef DEBUG_THROTTLE
    char tbuf[256];
    int   nthrots;
#endif /* DEBUG_THROTTLE */
    
    throttle = (throttle_t*) NULL;
    
    state = 0; /* state is to look for keyword "THROTTLE" */
    /* -- parse the first word of a line -- */
    /* -- extract the keyword "THROTTLE", "ACTIONLIST", "ACTION"  */
#ifdef DEBUG_THROTTLE
    nthrots = 0;
    oldthrotfd = throtfd;
    i = strlen(mysys->configpath) - 7;
    sscanf ((mysys->configpath+i), "%03d", &i);
    sprintf (tbuf, PATH_RUN_FILES "/throt%03d.parse", i);
    throtfd = fopen (tbuf, "w");
#endif /* DEBUG_THROTTLE */
    while (1) {
        get_word (fd, ls);
        tok = ls->word;
        if (strlen(tok) == 0) {
            if (0 == getline(fd, ls)) return (0);
            continue;
        }
        for (i=0; i<strlen(tok); i++) {
            keyword[i] = toupper(tok[i]);
            keyword[i+1] = '\0';
        }
        /* -- pseudo switch statement on first word */
        if (0 == strncmp("THROTTLE", keyword, 8)) {
            /*=====     T H R O T T L E     =====*/
            if (state != 0 && state != 4 && state != 5) {
                reporterr (ERRFAC, M_THROTSTA, ERRWARN, sys[0].configpath, 
                           ls->lineno);
            }
            state = 1; /* state is have "THROTTLE", look for "ACTIONLIST" */
            /* -- find the last throttle definition structure and 
               add a new one */
            throttle = (throttle_t*) malloc (sizeof(throttle_t));
            throttle->n = (throttle_t*) NULL;
            throttle->day_of_week_mask = -1;
            throttle->day_of_month_mask = -1;
            throttle->end_of_month_flag = 0;
            throttle->from = (time_t) -1;
            throttle->to = (time_t) -1;
            throttle->num_throttest = 0;
            throttle->num_actions = 0;
            
            if (sys[0].throttles == (throttle_t*) NULL) {
                sys[0].throttles = throttle;
            } else {
                t = sys[0].throttles;
                pt = t;
                while (t) {
                    pt = t;
                    t = t->n;
                }
                pt->n = throttle;
            }
            /* -- parse the days of week / days of month specification */
            get_word (fd, ls);
            tok = ls->word;
            if (strlen(ls->word) == 0) {
                reporterr (ERRFAC, M_THROTDOWM, ERRWARN, 
                           sys[0].configpath, ls->lineno);
                state = 5;
                forget_throttle (throttle);
                drain_to_eol (fd, ls);
                if (0 == getline(fd, ls)) return (0);
                continue;
            }
            if (0 != strcmp("-", tok)) {
                if (0 == parse_dowdom (tok, throttle)) {
                    reporterr (ERRFAC, M_THROTDOWM, ERRWARN, 
                               sys[0].configpath, ls->lineno);
                    state = 5;
                    forget_throttle (throttle);
                    drain_to_eol (fd, ls);
                    if (0 == getline(fd, ls)) return (0);
                    continue;
                }
            }
#ifdef DEBUG_THROTTLE
            nthrots++;
            tbuf[0] = '\0';
            printdowdom (tbuf, throttle);
            fprintf (throtfd, "========================================\n");
            fprintf (throtfd, "(#%d throttle definition in %s)\n", 
                     nthrots, mysys->configpath);
            fprintf (throtfd, "THROTTLE %s ", tbuf);
            fflush (throtfd);
#endif /* DEBUG_THROTTLE */
            /* -- parse the "from" and "to" time definitions */
            get_word (fd, ls);
            tok = ls->word;
            if (0 != strcmp("-", tok)) {
                if (0 != parse_time(tok, &throttle->from)) {
                    reporterr (ERRFAC, M_THROTTIM, ERRWARN, 
                               sys[0].configpath, ls->lineno);
                    state = 5;
                    forget_throttle (throttle);
                    drain_to_eol (fd, ls);
                    if (0 == getline(fd, ls)) return (0);
                    continue;
                }
            }
#ifdef DEBUG_THROTTLE
            tbuf[0] = '\0';
            print_time (tbuf, throttle->from);
            fprintf (throtfd, "%s ", tbuf);
            fflush (throtfd);
#endif /* DEBUG_THROTTLE */
            get_word (fd, ls);
            tok = ls->word;
            if (0 != strcmp("-", tok)) {
                if (0 != parse_time(tok, &throttle->to)) {
                    reporterr (ERRFAC, M_THROTTIM, ERRWARN, 
                               sys[0].configpath, ls->lineno);
                    state = 5;
                    forget_throttle (throttle);
                    drain_to_eol (fd, ls);
                    if (0 == getline(fd, ls)) return (0);
                    continue;
                }
            }
#ifdef DEBUG_THROTTLE
            tbuf[0] = '\0';
            print_time (tbuf, throttle->to);
            fprintf (throtfd, "%s \\\n", tbuf);
            fflush (throtfd);
#endif /* DEBUG_THROTTLE */
            if (throttle->from > throttle->to) {
                time_t tt;
                tt = throttle->from;
                throttle->from = throttle->to;
                throttle->to = tt;
            }
            /* -- parse the measurement keyword */
            while (1) {
                get_word (fd, ls);
                tok = ls->word;
                throttle->num_throttest++;
                if (throttle->num_throttest > 16) {
                    reporterr (ERRFAC, M_THROT2TST, ERRWARN, 
                               sys[0].configpath, ls->lineno);
                    state = 5;
                    forget_throttle (throttle);
                    drain_to_eol (fd, ls);
                    break;                                /*WR15031*/
                }
                i = throttle->num_throttest - 1;
                ttest = &(throttle->throttest[i]);
                ttest->measure_tok = 0;
                ttest->relop_tok = 0;
                ttest->value = 0;
                ttest->valueflag = 0;
                ttest->valuestring[0] = '\0';
                ttest->logop_tok = LOGOP_DONE;
                /* -- parse measurement keyword */
                if (0 == strlen(tok) || 0 == parse_throtmeasure(tok, ttest)) {
                    reporterr (ERRFAC, M_THROTMES, ERRWARN, 
                               sys[0].configpath, ls->lineno);
                    state = 5;
                    forget_throttle (throttle);
                    drain_to_eol (fd, ls);
                    break;                                /*WR15031*/
                }
#ifdef DEBUG_THROTTLE
                tbuf[0] = '\0';
                printmeasure (tbuf, ttest);
                fprintf (throtfd, "          %s ", tbuf);
                fflush (throtfd);
#endif /* DEBUG_THROTTLE */
                /* -- parse the relational operator */
                get_word (fd, ls);
                tok = ls->word;
                if (0 == strlen(tok) || 0 == parse_throtrelop (tok, ttest)) {
                    reporterr (ERRFAC, M_THROTREL, ERRWARN, 
                               sys[0].configpath, ls->lineno);
                    state = 5;
                    forget_throttle (throttle);
                    drain_to_eol (fd, ls);
                    break;                                /*WR15031*/
                } else if (0 == strcmp(">", tok)) {
                    ttest->relop_tok = GREATERTHAN;
                } else if (0 == strcmp(">=", tok)) {
                    ttest->relop_tok = GREATEREQUAL;
                } else if (0 == strcmp("<", tok)) {
                    ttest->relop_tok = LESSTHAN;
                } else if (0 == strcmp("<=", tok)) {
                    ttest->relop_tok = LESSEQUAL;
                } else if (0 == strcmp("==", tok)) {
                    ttest->relop_tok = EQUALTO;
                } else if (0 == strcmp("!=", tok)) {
                    ttest->relop_tok = NOTEQUAL;
                } else if (0 == strcmp("T>", tok)) {
                    ttest->relop_tok = TRAN2GT;
                } else if (0 == strcmp("T>=", tok)) {
                    ttest->relop_tok = TRAN2GE;
                } else if (0 == strcmp("T<", tok)) {
                    ttest->relop_tok = TRAN2LT;
                } else if (0 == strcmp("T<=", tok)) {
                    ttest->relop_tok = TRAN2LE;
                } else if (0 == strcmp("T==", tok)) {
                    ttest->relop_tok = TRAN2EQ;
                } else if (0 == strcmp("T!=", tok)) {
                    ttest->relop_tok = TRAN2NE;
                } else {
                    reporterr (ERRFAC, M_THROTREL, ERRWARN, 
                               sys[0].configpath, ls->lineno);
                    state = 5;
                    forget_throttle (throttle);
                    drain_to_eol (fd, ls);
                    break;                               /*WR15031*/
                }        
#ifdef DEBUG_THROTTLE
                tbuf[0] = '\0';
                printrelop (tbuf, ttest);
                fprintf (throtfd, "%s ", tbuf);
                fflush (throtfd);
#endif /* DEBUG_THROTTLE */
                /* -- parse a positive integer value or string */
                get_word (fd, ls);
                strcpy (ttest->valuestring, ls->word);
                if (0 == parse_value (ls)) {
                    reporterr (ERRFAC, M_THROTSYN, ERRWARN, 
                               sys[0].configpath, ls->lineno);
                    state = 5;
                    forget_throttle (throttle);
                    drain_to_eol (fd, ls);
                    break;                                /*WR15031*/
                } else {
                    ttest->value = ls->value;
                    ttest->valueflag = ls->valueflag;
                }
#ifdef DEBUG_THROTTLE
                tbuf[0] = '\0';
                printvalue (tbuf, ttest);
                fprintf (throtfd, "%s ", tbuf);
                fflush (throtfd);
#endif /* DEBUG_THROTTLE */
                /* -- parse an optional logical operator */
                get_word (fd, ls);
                tok = ls->word;
                ttest->logop_tok = LOGOP_DONE;
                if (0 == strlen(tok)) {
#ifdef DEBUG_THROTTLE
                    fprintf (throtfd, "\n");
                    fflush (throtfd);
#endif /* DEBUG_THROTTLE */
                    break;
                }
                if (0 == strcmp (tok, "AND")) {
                    ttest->logop_tok = LOGOP_AND;
                } else if (0 == strcmp (tok, "OR")) {
                    ttest->logop_tok = LOGOP_OR;
                } else {
                    reporterr (ERRFAC, M_THROTLOGOP, ERRWARN, 
                               sys[0].configpath, ls->lineno);
                    state = 5;
                    forget_throttle (throttle);
                    drain_to_eol (fd, ls);
                    break;                                /*WR15031*/
                }
#ifdef DEBUG_THROTTLE
                tbuf[0] = '\0';
                printlogop (tbuf, ttest);
                fprintf (throtfd, "%s \\\n", tbuf);
                fflush (throtfd);
#endif /* DEBUG_THROTTLE */
            }
        } else if (0 == strncmp("ACTIONLIST", keyword, 10)) {
            /*=====     A C T I O N L I S T     =====*/
#ifdef DEBUG_THROTTLE
            fprintf (throtfd, "    ACTIONLIST\n");
            fflush (throtfd);
#endif /* DEBUG_THROTTLE */
            if (state == 5) {
                if (0 == getline(fd, ls)) return (0);
                continue;
            }
            if (state != 1) {
                reporterr (ERRFAC, M_THROTSTA, ERRWARN, sys[0].configpath, 
                           ls->lineno);
            }
            state = 2; /* state is have "ACTIONLIST", looking for "ACTION" */
        } else if (0 == strncmp("ACTION", keyword, 6)) {
            /*=====     A C T I O N     =====*/
#ifdef DEBUG_THROTTLE
            fprintf (throtfd, "        ACTION: ");
            fflush (throtfd);
#endif /* DEBUG_THROTTLE */
            if (state == 5) {
                if (0 == getline(fd, ls)) return (0);
                continue;
            }
            if (state != 1 && state != 2 && state != 3) {
                reporterr (ERRFAC, M_THROTSTA, ERRWARN, sys[0].configpath, 
                    ls->lineno);
                state = 5;
                forget_throttle (throttle);
                drain_to_eol (fd, ls);
                if (0 == getline(fd, ls)) return (0);
                continue;
            }
            state = 3;
            if (throttle->num_actions >= 15) {
                reporterr (ERRFAC, M_THROTACT, ERRWARN, sys[0].configpath,
                    ls->lineno);
            } else {
                action = &(throttle->actions[throttle->num_actions]);
                /* -- parse action verb */
                get_word (fd, ls);
                tok = ls->word;
                implied_do_flag = 0;
                if (0 == strlen(tok)) {
                    reporterr (ERRFAC, M_THROTSYN, ERRWARN, 
                        sys[0].configpath, ls->lineno);
                    state = 5;
                    forget_throttle (throttle);
                    drain_to_eol (fd, ls);
                    if (0 == getline(fd, ls)) return (0);
                    continue;
                } else if (0 == strcmp ("set", tok)) {
                    action->actionverb_tok = VERB_SET;
                } else if (0 == strcmp ("incr", tok)) {
                    action->actionverb_tok = VERB_INCR;
                } else if (0 == strcmp ("decr", tok)) {
                    action->actionverb_tok = VERB_DECR;
                } else if (0 == strcmp ("do", tok)) {
                    action->actionverb_tok = VERB_DO;
                } else {
                    action->actionverb_tok = VERB_DO;
                    implied_do_flag = 1;
                }
                if (action->actionverb_tok != VERB_DO) {
                    /* -- parse "set", "incr", "decr" arguments */
                    get_word (fd, ls);
                    tok = ls->word;
                    action->actionwhat_tok = parse_actiontunable (tok);
                    if (action->actionwhat_tok == 0) {
                        reporterr (ERRFAC, M_THROTWHA, ERRWARN,
                                   sys[0].configpath, ls->lineno);
                        state = 5;
                        forget_throttle (throttle);
                        drain_to_eol (fd, ls);
                        if (0 == getline(fd, ls)) return (0);
                        continue;
                    }
                    get_word (fd, ls);
                    tok = ls->word;
                    if (1 == parse_value(ls)) {
                        if (ls->valueflag) action->actionvalue = ls->value;
                    } else {
                        reporterr (ERRFAC, M_THROTVAL, ERRWARN, 
                                   sys[0].configpath, ls->lineno);
                        state = 5;
                        forget_throttle (throttle);
                        drain_to_eol (fd, ls);
                        if (0 == getline(fd, ls)) return (0);
                        continue;
                    }
                    action->actionstring[0] = '\0';
                    (void) strcat (action->actionstring, tok);
                    while (1) {
                        get_word (fd, ls);
                        if (strlen(ls->word) == 0) break;
                    }
                } else {
                    /* -- parse action to take */
                    if (implied_do_flag == 0) {
                        get_word (fd, ls);
                    }
                    tok = ls->word;
                    if (0 == strlen(tok)) {
                        reporterr (ERRFAC, M_THROTSYN, ERRWARN, 
                                   sys[0].configpath, ls->lineno);
                        state = 5;
                        forget_throttle (throttle);
                        drain_to_eol (fd, ls);
                        if (0 == getline(fd, ls)) return (0);
                        continue;
                    } else if (0 == strcmp ("console", tok)) {
                        action->actionwhat_tok = ACTION_CONSOLE;
                        if (action->actionverb_tok != VERB_DO) {
                            reporterr (ERRFAC, M_THROTWHA, ERRWARN, 
                                       sys[0].configpath, ls->lineno);
                            state = 5;
                            forget_throttle (throttle);
                            drain_to_eol (fd, ls);
                            if (0 == getline(fd, ls)) return (0);
                            continue;
                        }
                    } else if (0 == strcmp ("mail", tok)) {
                        action->actionwhat_tok = ACTION_MAIL;
                        if (action->actionverb_tok != VERB_DO) {
                            reporterr (ERRFAC, M_THROTWHA, ERRWARN, 
                                       sys[0].configpath, ls->lineno);
                            state = 5;
                            forget_throttle (throttle);
                            drain_to_eol (fd, ls);
                            if (0 == getline(fd, ls)) return (0);
                            continue;
                        }
                    } else if (0 == strcmp ("exec", tok)) {
                        action->actionwhat_tok = ACTION_EXEC;
                        if (action->actionverb_tok != VERB_DO) {
                            reporterr (ERRFAC, M_THROTWHA, ERRWARN, 
                                       sys[0].configpath, ls->lineno);
                            state = 5;
                            forget_throttle (throttle);
                            drain_to_eol (fd, ls);
                            if (0 == getline(fd, ls)) return (0);
                            continue;
                        }
                    } else if (0 == strcmp ("log", tok)) {
                        action->actionwhat_tok = ACTION_LOG;
                        if (action->actionverb_tok != VERB_DO) {
                            reporterr (ERRFAC, M_THROTWHA, ERRWARN, 
                                       sys[0].configpath, ls->lineno);
                            state = 5;
                            forget_throttle (throttle);
                            drain_to_eol (fd, ls);
                            if (0 == getline(fd, ls)) return (0);
                            continue;
                        }
                    } else {
                        reporterr (ERRFAC, M_THROTWER, ERRWARN, 
                                   sys[0].configpath, ls->lineno);
                        state = 5;
                        forget_throttle (throttle);
                        drain_to_eol (fd, ls);
                        if (0 == getline(fd, ls)) return (0);
                        continue;
                    }
                    /* -- parse value or action string */
                    strcpy (action->actionstring, " ");
                    get_word (fd, ls);
                    tok = ls->word;
                    while (0 < strlen(tok)) {
                        if (1 == parse_value(ls)) {
                            if (ls->valueflag) action->actionvalue = ls->value;
                        }
                        (void) strcat (action->actionstring, tok);
                        get_word (fd, ls);
                        tok = ls->word;
                        if (0 < strlen(tok)) {
                            (void) strcat (action->actionstring, " ");
                        }
                    }
                }
#ifdef DEBUG_THROTTLE
                tbuf[0] = '\0';
                printaction (tbuf, action);
                fprintf (throtfd, "%s\n", tbuf);
                fflush (throtfd);
#endif /* DEBUG_THROTTLE */
                throttle->num_actions++;
            }
        } else if (0 == strncmp("ENDACTIONLIST", keyword, 13)) {
            /*=====     E N D A C T I O N L I S T     =====*/
#ifdef DEBUG_THROTTLE
            fprintf (throtfd, "    ENDACTIONLIST\n");
            fflush (throtfd);
#endif /* DEBUG_THROTTLE */
            if (state == 5) {
                if (0 == getline(fd, ls)) return (0);
                continue;
            }
            if (state != 2 && state != 3) {
                reporterr (ERRFAC, M_THROTSTA, ERRWARN, sys[0].configpath, 
                           ls->lineno);
            } 
            state = 4;
        } else {
            /*=====     U N R E C O G N I Z E D    K E Y W O R D     =====*/
            /* -- unrecognized keyword, simply return the line and let 
               readconfig continue its processing */
            /* hokiness: */
            ls->invalid = FALSE;
#ifdef DEBUG_THROTTLE
            fclose (throtfd);
            throtfd = oldthrotfd;
#endif /* DEBUG_THROTTLE */
            return (ls->lineno);
        }
        /* -- read the next line from the config file */
        if (0 == getline(fd, ls)) return (0);
    }
}
    
/***************************************************************************
 * clearconfig -- return system definition data structures to virgin state
 **************************************************************************/
static void
clearconfig(void)
{
    sddisk_t   *curdev;
    throttle_t *t;

    /* clear the global data from last call to this function */
    if (sys[0].group) {        
        while (sys[0].group->headsddisk) {
            curdev = sys[0].group->headsddisk;
            sys[0].group->headsddisk = curdev->n;
            free(curdev);
        }
        sys[0].group->headsddisk = sys[0].group->tailsddisk = NULL;
        sys[1].group->headsddisk = sys[1].group->tailsddisk = NULL;
    }        
    /* clear out the throttles, too. */
    while (sys[0].throttles) {
        t = sys[0].throttles;
        sys[0].throttles = t->n;
        free(t);
    }
    sys[0].throttles = NULL;
    sys[1].throttles = NULL;

    initconfigs();

    return;
}

/***************************************************************************
 * readconfig_sub -- separated from readconfig (dynamic device addition
 **************************************************************************/
int
readconfig_sub(char *filename, int *network_analysis_mode)
{
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    FILE *fd;
#elif defined(SOLARIS)  /* 1LG-ManyDEV  ManyLG-1DEV */ 
    int fd;
#endif
    LineState lstate;
    int result;
    int error;

    sprintf(sys[0].configpath, "%s/%s", PATH_CONFIG, filename);
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if ((fd = fopen(sys[0].configpath, "r")) == NULL) {
#elif defined(SOLARIS)  /* 1LG-ManyDEV  ManyLG-1DEV */ 
    if ((fd = open(sys[0].configpath, O_RDONLY)) == -1) {
#endif
        if (errno == EMFILE) {
            reporterr(ERRFAC, M_FILE, ERRCRIT, sys[0].configpath, 
                strerror(errno));
        }
        reporterr(ERRFAC, M_CFGFILE, ERRCRIT,
            argv0, sys[0].configpath, strerror(errno));
        return -1;
    }
    lgnum = cfgpathtonum(sys[0].configpath);

    /* clear the global data from last call to this function */
    clearconfig();

    sprintf(sys[0].configpath, "%s/%s", PATH_CONFIG, filename);
    sprintf(sys[1].configpath, "%s/%s", PATH_CONFIG, filename);

    /* initialize the parsing state */
    lstate.lineno = 0;
    lstate.invalid = TRUE;
    error = 0;
    while (1) {
        /* if we need a line and don't get it, then we're done */
        if (!getline(fd, &lstate)) {
            break;
        }
        /* major sections of the file */
        if (strncmp("THROTTLE", lstate.key, 8) == 0) {
            if ((result = parse_throttles (fd, &lstate)) < 0) {
                error = -1;
                break;
            }
        } else if (strcmp("SYSTEM-TAG:", lstate.key) == 0) {
            if ((result = parse_system (fd, &lstate, network_analysis_mode)) < 0) {
                error = -1;
                break;
            }

        } else if (strcmp("PROFILE:", lstate.key) == 0) {
            if ((result = parse_profile (fd, &lstate)) < 0) {
                error = -1;
                break;
            }
        } else if (strcmp("NOTES:", lstate.key) == 0) {
           continue;
        } else {
            reporterr(ERRFAC, M_CFGBDKEY, ERRCRIT, sys[0].configpath, 
                lstate.lineno, lstate.key);
            error = -1;
            break;
        }
    }
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    fclose(fd);
#elif defined(SOLARIS)
    close(fd);
#endif

    return (error);

}

/***************************************************************************
 * inittunables -- 
 *     - read tunables from pstore
 *     - init the missing tunables and write back to pstore
 **************************************************************************/
int
inittunables(char *ps_name, char *group_name, char *pstbuf, int pstbuf_len)
{
    char        tunablebuffer[512];
    char        *newtunablebuffer = NULL;
    char        *psbuffer = NULL;
    char        key[512];
    char        value[512];
    char        line[1024];
    char        *ps_key;
    char        *ps_value;
    char        *tp;
    char        *tokptr;
    char        *valid_key[FTD_MAX_KEY_VALUE_PAIRS];
    char        *key_value[FTD_MAX_KEY_VALUE_PAIRS];
    int         max_valid;
    int         i;
    int         len, kv_len, ps_len;
    int         linelen;
    int         ret;

    if ((psbuffer = (char *)malloc(pstbuf_len * sizeof(char))) == NULL) {
        reporterr(ERRFAC, M_MALLOC, ERRCRIT, pstbuf_len);
        return -1;
    } else {
        strncpy(psbuffer, pstbuf, pstbuf_len);
    }

    if ((newtunablebuffer = (char *)calloc(pstbuf_len, sizeof(char))) == NULL) {
        reporterr(ERRFAC, M_MALLOC, ERRCRIT, pstbuf_len);
		free( psbuffer );
        return -1;
    }

    strcpy(tunablebuffer, FTD_DEFAULT_TUNABLES);

    tp = tunablebuffer;
    i = 0;
    while (tp) {
        tokptr = strchr(tp,':');
        *tokptr = '\0';
        valid_key[i] = (char *) ftdmalloc((strlen(tp) + 1) * sizeof(char));
        strcpy(valid_key[i], tp);
        tp = tokptr + 1;
        if ((tokptr = strchr(tp,'\n'))) {
            *tokptr = '\0';
            tp++;
            key_value[i] = (char *) ftdmalloc((strlen(tp) + 1) * sizeof(char));
            strcpy(key_value[i], tp);
            tp = tokptr + 1;
        } else {
            /* the last tunable */
            tp++;
            key_value[i] = (char *) ftdmalloc((strlen(tp) + 1) * sizeof(char));
            strcpy(key_value[i], tp);
            tp = NULL;
        }
        i++;
    }
    max_valid = i;

    /*
     * obsolete tunables in pstore will be removed
     * new tunables will be added to pstore
     * current pstore setting shall be preserved
     */
    ps_key = key;
    ps_value = value;
    while (getbufline(&psbuffer, &ps_key, &ps_value, '\n')) {
        for (i = 0; i < max_valid; i++) {
            /*
             * ps_key is the key+':', compare only string part without ':'
             */
	    ps_len = strlen(ps_key) - 1;
            if ((strncmp(ps_key, valid_key[i], ps_len) == 0) &&
                (ps_len == strlen(valid_key[i]))) {

		/* rellocate storage if we need too */

		if (strcmp(key_value[i], ps_value)) {
		    kv_len = strlen(key_value[i]);
		    ps_len = strlen(ps_value);
		    if (kv_len != ps_len)
		    {
			key_value[i] = (char *) ftdrealloc(key_value[i],(ps_len + 1) * sizeof(char));
		    }
                    strcpy(key_value[i], ps_value);
		}
            }
        }
    }

    /*
     * create a new buffer
     */
    len = 0;
    for (i = 0; i < max_valid; i++) {
        sprintf(line, "%s: %s\n", valid_key[i], key_value[i]);
        linelen = strlen(line);
        strncpy(&newtunablebuffer[len], line, linelen);
        len += linelen;

        free(valid_key[i]);
        free(key_value[i]);
    }
    newtunablebuffer[len] = '\0';

    /*
     * write new buffer to the ps
     */
    if ((ret = ps_set_group_attr(ps_name, 
                                 group_name,
                                 newtunablebuffer,
                                 pstbuf_len)) != PS_OK) {
        goto inittunablesexit;
    }

inittunablesexit:

    if (psbuffer) {
        free(psbuffer);
    }

    if (newtunablebuffer) {
        free(newtunablebuffer);
    }

    return ret;

}

/***************************************************************************
 * readconfig -- process and parse a configuration file
 **************************************************************************/
int
readconfig(int pmdflag, int pstoreflag, int openflag, char *filename)
{

       int error;
       int tryquietly;
static int network_analysis_mode = 0;

    if ( (error = readconfig_sub(filename, (int *)&network_analysis_mode)) != 0 ) {
        return (error);
    }
	// Also save the network analysis mode flag in the global variable
	_net_bandwidth_analysis = network_analysis_mode;

    if (pstoreflag > 0) { 
        tryquietly = 0;
        if (pstoreflag == 2) {
            tryquietly = 1;
        }
        /* retrieve the tunables from the persistent store, or, if in network analysis mode,
           from the SFTKdtc_net_analysis_parms.txt file created by launchnetanalysis */
        if (gettunables(sys[0].group->devname, 1, tryquietly) == -1) {
            return -1;
        }
    }
	if( !network_analysis_mode )
	// In network bandwidth analysis mode we do not access devices, so we don't do the following
	{
	    if (pmdflag) {
	        strcpy(sys[0].pstore, sys[0].group->pstore);
	        if (verify_pmd_entries(sys[0].group->headsddisk, filename, 
	            openflag) != 0) {
	            return -1;
	        }
	    } else {
	        if (verify_rmd_entries(sys[1].group, filename, 
	            openflag) != 0) {
	            return -1;
	        }
	    }
	}
    /* if the rmd is calling, then switch mysys and othersys */
    if (!pmdflag) {
        mysys = &sys[1];
        othersys = &sys[0];
    } else {
        mysys = &sys[0];
        othersys = &sys[1];
    }

    return (0);
}

/***************************************************************************
 * verify_pmd_entries -- verify the primary system devices in the linked
 *                       list of device profiles
 **************************************************************************/
static int
verify_pmd_entries(sddisk_t *curdev, char *path, int openflag)
{
    group_t     *group;

    group = mysys->group;

    /* open the logical group device, if asked to */
    if (openflag) {
        if ((group->devfd = open(group->devname, O_RDWR)) == -1) {
            reporterr(ERRFAC, M_LGDOPEN, ERRCRIT, group->devname, strerror(errno));
            return -1;
        }
    }
    while (curdev) {
        curdev->devsize = (u_longlong_t) disksize(curdev->devname);
        // disksize() returns -1 or 0 in case of error;
        // -1 corresponds to UNSIGNED_LONG_LONG_MAX as unsigned value.
		if ((curdev->devsize == (u_longlong_t) 0) || (curdev->devsize == UNSIGNED_LONG_LONG_MAX))
        {
            reporterr (ERRFAC, M_BADDEVSIZ, ERRCRIT, curdev->devname);
            return (-1);
        }
        curdev = curdev->n;
    }
    return 0;
}

/*******************************************************************************
    dev_opened_by_whom(char *device_name, char *owner_process, int max_str_len)
    Verify if specified device is currently opend by a process.
    Return: the name of the process which has the device opened (if applicable)
    if the argument owner_process is not NULL
    Return values:
    -1: an error occurred and we cannot determine if the device is opened;
    0: the device is not opened;
    1: no error and device is opened; if owner_process is not NULL, the process
       name which has the dev opened is rerturned at the owner_process address
       provided.
 ******************************************************************************/
#define MAX_LSOF_LENGTH 512
int dev_opened_by_whom(char *device_name, char *owner_process, int max_str_len)
{
    char   work_dev_name[MAXPATHLEN+1];
    char   *lsof_path, *grep_path;
    char   lsof_1[] = "/usr/bin/lsof";
    char   lsof_2[] = "/usr/sbin/lsof";
    char   lsof_3[] = "/bin/lsof";
    char   lsof_cmd[MAX_LSOF_LENGTH], lsof_result[MAX_LSOF_LENGTH];
    char   *sub_dev_name, *s, *process_name;
    FILE   *fp;
    int    count_token_parsed = 0;
	struct stat statbuf;
    int    result = 0;
    int    i = 0;

    if(device_name == NULL)
    {
        return(-1);
    }

	if(stat(lsof_1, &statbuf) == 0)
    {
        lsof_path = lsof_1;
    }
    else if(stat(lsof_2, &statbuf) == 0)
    {
        lsof_path = lsof_2;
    }
    else if(stat(lsof_3, &statbuf) == 0)
    {
        lsof_path = lsof_3;
    }
    else
    {
        return(-1);  // lsof not available
    }

    // Parse the device name to keep only the last subname; this is for cases where lsof
    // would list the opened device in a way different than the definition in the config file;
    // example: /dev/mapper/vg02-xx5 (lsof) vs /dev/vg02/xx5 (config file)
    strncpy(work_dev_name, device_name, MAXPATHLEN);
    sub_dev_name = s = strtok(work_dev_name, "/" );
    while(((s = strtok(NULL, "/" )) != NULL) && (++count_token_parsed < 20))
    {
        sub_dev_name = s;  // preserve last token address
	}
    if(count_token_parsed >= 20)
    {
        return(-1);
    }
    // List opened files and look for sub_dev_name
    sprintf(lsof_cmd, "%s | /bin/grep %s\n", lsof_path, sub_dev_name);  
    fp = popen(lsof_cmd, "r");
    if (fp != NULL)
    {
        if(fgets(lsof_result, sizeof(lsof_result), fp) != NULL)
        {
            if(strstr(lsof_result, sub_dev_name) != NULL)
            {
                // Sub device name found
                for(s = lsof_result, i = 0; *s == ' '; s++, i++)
                {
                    /* get to owner process name (first output field of lsof) */
                }
                process_name = s;
                for(; (*s != ' ') && (*s != '\0') && (*s != '\n') && (i < MAX_LSOF_LENGTH-1); s++, i++)
                {
                    /* get to end of owner process name and append '\0' to end the string */
                }
                *s = '\0';
#ifdef TDMF_TRACE
                ftd_trace_flow(FTD_DBG_FLOW1, "Device %s is opened by: %s\n", device_name, process_name);
#endif
                // Check if caller wants the owner process name returned
                if(owner_process != NULL)
                {
                    strncpy(owner_process, process_name, max_str_len);
                    if(strlen(process_name) >= max_str_len)
                    {
                        owner_process[max_str_len-1] = '\0';
                    }
                }
                result = 1;  // Device is opened
            }
            else
            {
                result = 0;  // Device is not opened
            }
        }
        else
        {
            result = -1;  // Error getting list of opened files
        }
    }
    else
    {
        result = -1;  // lsof exec error
    }
    if(fp != NULL)
    {
        pclose(fp);
    }
    return(result);
}

/***************************************************************************
 * verify_rmd_entries -- verify the secondary system devices in the linked
 *                       list of device profiles
 **************************************************************************/
int
verify_rmd_entries(group_t *group, char *path, int openflag)
{
#if defined(SOLARIS)
    struct dk_cinfo dkinfo;
    struct vtoc dkvtoc;
#endif
    sddisk_t *curdev;
    struct stat statbuf;
    char        emsg[256];
    int open_tries;
    int save_errno;
    int i, mode;
    int ret = 0; 
#ifdef EFI_SUPPORT
    struct dk_gpt    *efip = NULL;  
#endif
    time_t now, start_time = 0;
    int    num_seconds = 0;
    int    max_wait = 300;  // If device opened by sync process, wait 300 seconds max for
                            // sync to finish and release the device
    int    sync_detected = 0;
    char   owner_process[MAXPATHLEN+1];

    curdev = group->headsddisk;
    while (curdev != NULL) {
        /* stat the mirror device and make sure it is a char device. */
        if (stat(curdev->mirname, &statbuf) != 0) {
            strcpy (emsg, strerror(errno));
            reporterr (ERRFAC, M_MIRSTAT, ERRCRIT, curdev->mirname, emsg);
            return (-1);
        }
#if !defined(linux)
        if (!S_ISCHR(statbuf.st_mode)) {
            reporterr (ERRFAC, M_MIRTYPERR, ERRCRIT, curdev->mirname,
                path, 0);
            return (-1);
        }
#endif /* !defined(linux) */
        curdev->no0write = 0;
        curdev->mirfd = -1;
        curdev->mirsize = (u_longlong_t) disksize(curdev->mirname);
        // disksize() returns -1 or 0 in case of error;
        // -1 corresponds to UNSIGNED_LONG_LONG_MAX as unsigned value.
		if ((curdev->mirsize == (u_longlong_t) 0) || (curdev->mirsize == UNSIGNED_LONG_LONG_MAX))
        {
            reporterr (ERRFAC, M_BADDEVSIZ, ERRCRIT, curdev->mirname);
            return (-1);
        }

        /* open the mirror device */
        if (openflag) {
            open_tries = 10;
            for (i = 0; i < open_tries; i++) {
                /* kluge to get around a race with RMDA */
                ftd_trace_flow(FTD_DBG_FLOW1,"open try #: %d\n",i);
                save_errno = 0;
                mode = O_RDWR; /* There used to be an O_SYNC option here, but it was found to hinder performances, especially
                                * after a full refresh.  Because this option has been removed, we must make sure that
                                * fsync() is properly called.  This is currently done through syncgroup(). */
                if (group->chaining != 1)
                        mode |= O_EXCL;
                curdev->mirfd = open(curdev->mirname, O_LARGEFILE | mode);
                if (curdev->mirfd < 0) 
                {
                    save_errno = errno;
                    if (save_errno == EMFILE) 
                    {
                        reporterr (ERRFAC, M_FILE, ERRCRIT, curdev->mirname, strerror(save_errno));
                    }
                    if( save_errno == EBUSY)  // Device opened by other process
                    {
                        time(&now);
                        start_time = now;
                        num_seconds = 0;
                        do
                        {
                            sync_detected = 0;
                            if(dev_opened_by_whom(curdev->mirname, owner_process, sizeof(owner_process)) > 0)
                            {
                                if( strcmp("sync", owner_process) == 0)
                                {
                                    // Opened by a sync process
                                    sleep(10);
                                    sync_detected = 1;
                                    time(&now);
                                    num_seconds = now - start_time;
                                    reporterr(ERRFAC, M_MIROPEN_SYNC, ERRINFO, curdev->mirname, num_seconds, max_wait);
                                }
                                else
                                {
                                    reporterr(ERRFAC, M_MIROPEN_BY, ERRINFO, curdev->mirname, owner_process, i+1, open_tries);
                                }
                            }
                        }
                        while(sync_detected && (num_seconds < max_wait));

                        if(num_seconds >= max_wait)
                        {
                            break;
                        }
                    }
                    sleep(1);
                } else {      // of if (curdev->mirfd < 0)
                    break;
                }
            } //...loop to retry opening the device until open_tries is reached

            if ((curdev->mirfd < 0 && i == open_tries) || (save_errno == EBUSY))
            {
                reporterr (ERRFAC, M_MIROPEN, ERRCRIT, argv0, curdev->mirname,
                    strerror(save_errno ? save_errno: errno));
                return -1;
            }

#if defined(SOLARIS)
            do {
#ifdef PURE
                memset(&dkinfo, 0, sizeof(dkinfo));
                memset(&dkvtoc, 0, sizeof(dkvtoc));
#endif
                if (FTD_IOCTL_CALL(curdev->mirfd, DKIOCINFO, &dkinfo) < 0) {
                    break;
                }

                if ((ret = read_vtoc(curdev->mirfd, &dkvtoc)) >= 0)
                {
                   if (dkinfo.dki_ctype != DKC_MD
                            && dkvtoc.v_part[dkinfo.dki_partition].p_start == 0) {
                       ftd_trace_flow(FTD_DBG_ERROR,
                            "Sector 0 write dissallowed for device: %s\n",
                            curdev->mirname);
                       curdev->no0write = 1;
                    }
                }
#ifdef EFI_SUPPORT
                else if (ret == VT_ENOTSUP)
                {
                   if ((ret = efi_alloc_and_read(curdev->mirfd, &efip)) < 0)
                   {
                    break;
                }
                if (dkinfo.dki_ctype != DKC_MD
                         && efip->efi_parts[dkinfo.dki_partition].p_start == 0) {
                    ftd_trace_flow(FTD_DBG_ERROR,
                            "Sector 0 write dissallowed for device: %s\n",
                            curdev->mirname);
                    curdev->no0write = 1;
                }
                }
#endif                
                else
                {
                   break;
                }
            
            } while (0);        
#endif /* SOLARIS */
        }
        curdev = curdev->n;
    }
    return 0;
}

/***************************************************************************
 * prottypes for dynamic device addition
 **************************************************************************/
void diffconf_savelg(difflg_t *target);
int diffconf_savethrl(throttle_t **target);
int diffconf_savedev(diffdev_t **phead, diffdev_t **ptail);
typedef struct chainedtbl {
    struct chainedtbl* p;
    struct chainedtbl* n;
} chainedtbl_t;
void addchain(chainedtbl_t **phead, chainedtbl_t **ptail, chainedtbl_t *entry);
void removechain(chainedtbl_t **phead, chainedtbl_t **ptail, chainedtbl_t *entry);

/***************************************************************************
 * diffconfig -- making diffrence table between old and new config files
 **************************************************************************/
diffent_t *
diffconfig(char *basefilename, char *newfilename) 
{
    diffent_t *retent=NULL;
    diffdev_t *oldent, *oldnext, *newent, *newnext;
    machine_t savesys[2];
    throttle_t *chkthrlold, *chkthrlnew;
	int network_analysis_mode;

int count;

    retent = (diffent_t*)malloc(sizeof(diffent_t));
    if (retent == NULL) {
        reporterr(ERRFAC, M_MALLOC, ERRCRIT, sizeof(diffent_t));
        return NULL;
    }
    memset(retent, 0x00, sizeof(diffent_t));
    
    /* save current sys[] and clear it */
    memcpy(savesys, sys, sizeof(savesys));
    initconfigs();            /* initconfigs is not free dev/throt info */

#define RECOVERY_SYSENT { \
    clearconfig();            /* clearconfig is do free dev/throt info */    \
    memcpy(sys, savesys, sizeof(savesys)); \
}

    /* analyze old config file */
    if (readconfig_sub(basefilename, &network_analysis_mode) < 0) {
        reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, basefilename);
        RECOVERY_SYSENT;
        freediffent(retent);
        return NULL;
    }

    /* save information in the old config file */
    diffconf_savelg(&retent->oldlg);

    if (diffconf_savethrl(&retent->oldthrl) != TRUE) {
        RECOVERY_SYSENT;
        freediffent(retent);
        return NULL;
    }
    if (diffconf_savedev(&(retent->h_remove), &(retent->t_remove)) != TRUE) {
        RECOVERY_SYSENT;
        freediffent(retent);
        return NULL;
    }

    /* analyze new config file */
    if (readconfig_sub(newfilename, &network_analysis_mode) < 0) {
        reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, newfilename);
        RECOVERY_SYSENT;
        freediffent(retent);
        return NULL;
    }
    
    /* save information in the new config file */
    diffconf_savelg(&retent->newlg);

    if (diffconf_savethrl(&retent->newthrl) != TRUE) {
        RECOVERY_SYSENT;
        freediffent(retent);
        return NULL;
    }

    if (diffconf_savedev(&retent->h_add, &retent->t_add) != TRUE) {
        RECOVERY_SYSENT;
        freediffent(retent);
        return NULL;
    }
    
    /* recover from saved sys info */
    RECOVERY_SYSENT;
#undef RECOVERY_SYSENT
    for (oldent = retent->h_remove; oldent != NULL; oldent = oldnext) {
        oldnext = oldent->n;
        for (newent = retent->h_add; newent != NULL; newent = newnext) {
            newnext = newent->n;
            if (strcmp(newent->sddevname, oldent->sddevname) == 0 &&
                strcmp(oldent->devname, newent->devname) == 0 &&
                strcmp(oldent->mirname, newent->mirname) == 0 &&
		oldent->dd_minor == newent->dd_minor &&
		oldent->md_minor == newent->md_minor ) {
                removechain((chainedtbl_t**)&(retent->h_remove),
                            (chainedtbl_t**)&(retent->t_remove),
                            (chainedtbl_t*)oldent);
                removechain((chainedtbl_t**)&(retent->h_add),
                            (chainedtbl_t**)&(retent->t_add),
                            (chainedtbl_t*)newent);
                addchain((chainedtbl_t**)&(retent->h_same),
                         (chainedtbl_t**)&(retent->t_same),
                         (chainedtbl_t*)newent);
                free(oldent);
                break;
            }
        }
    }
    /* compare and check between old config file with new config file */
    for (chkthrlold = retent->oldthrl, chkthrlnew = retent->newthrl;
         chkthrlold != NULL && chkthrlnew != NULL;
         chkthrlold = chkthrlold->n, chkthrlnew = chkthrlnew->n) {
        if (memcmp(((char*)chkthrlold + sizeof(chkthrlold->n)),
                   ((char*)chkthrlnew + sizeof(chkthrlnew->n)),
                   (sizeof(throttle_t) - sizeof(throttle_t*))) != 0) {
            break;
        }
    }
    if (chkthrlold != NULL || chkthrlnew != NULL) {
        retent->status = DIFFSTAT_UNMATCH;
    } else if (memcmp(&(retent->oldlg),
                      &(retent->newlg),
                      sizeof(difflg_t)) != 0) {
        retent->status = DIFFSTAT_UNMATCH;
    } else if (retent->h_remove != NULL) {
        retent->status = DIFFSTAT_DEV_REMOVED;
    } else if (retent->h_add != NULL) {
        retent->status = DIFFSTAT_DEV_ADDED;
    } else {
        retent->status = DIFFSTAT_ALL_MATCH;
    }

    return retent;
}

/***************************************************************************
 * freediffent -- free diffent
 **************************************************************************/
void
freediffent(diffent_t *target) {
    diffdev_t *dcur, *dnext;
    throttle_t *tcur, *tnext;

    if (target == NULL)
        return;
    for (dcur = target->h_same; dcur != NULL; dcur = dnext) {
        dnext = dcur->n;
        free(dcur); 
    }
    for (dcur = target->h_add; dcur != NULL; dcur = dnext) {
        dnext = dcur->n;
        free(dcur);
    }
    for (dcur = target->h_remove; dcur != NULL; dcur = dnext) {
        dnext = dcur->n;
        free(dcur);
    } 
    for (tcur = target->oldthrl; tcur != NULL; tcur = tnext) {
        tnext = tcur->n;
        free(tcur);
    } 
    for (tcur = target->newthrl; tcur != NULL; tcur = tnext) {
        tnext = tcur->n;
        free(tcur);
    } 
    free(target);
}   

/***************************************************************************
 * diffconf_savelg -- save lg information
 **************************************************************************/
void
diffconf_savelg(difflg_t *target) {
    memset(target, 0x00, sizeof(difflg_t));
    strcpy(target->phost, sys[0].name);
    strcpy(target->shost, sys[1].name);
    strcpy(target->journal_path, sys[1].group[0].journal_path);
    strcpy(target->pstore, sys[0].group[0].pstore);
    strcpy(target->devname, sys[0].group[0].devname);
    target->chaining = sys[1].group[0].chaining;
}

/***************************************************************************
 * diffconf_savethrl -- save throttles information
 **************************************************************************/
int
diffconf_savethrl(throttle_t **target) {
    int i;
    throttle_t *newthr, *curthr, **save;

    save = target;
    for (curthr = sys[0].throttles; curthr != NULL; curthr = curthr->n) {
        newthr = (throttle_t*)malloc(sizeof(throttle_t));
        if (newthr == NULL) {
            reporterr(ERRFAC, M_MALLOC, ERRCRIT, sizeof(throttle_t));
            return FALSE;
        }
        memset(newthr, 0x00, sizeof(throttle_t));
        *save = newthr;
        save = &(newthr->n);

        newthr->day_of_week_mask = curthr->day_of_week_mask;
        newthr->day_of_month_mask = curthr->day_of_month_mask;
        newthr->end_of_month_flag = curthr->end_of_month_flag;
        newthr->from = curthr->from;
        newthr->to = curthr->to;
        newthr->num_throttest = curthr->num_throttest;
        for (i = 0; i < curthr->num_throttest; i++) {
            newthr->throttest[i].measure_tok
             = curthr->throttest[i].measure_tok;
            newthr->throttest[i].relop_tok = curthr->throttest[i].relop_tok;
            newthr->throttest[i].value = curthr->throttest[i].value;
            newthr->throttest[i].valueflag = curthr->throttest[i].valueflag;
            strncpy(newthr->throttest[i].valuestring,
                    curthr->throttest[i].valuestring,
                    sizeof(newthr->throttest[i].valuestring));
            newthr->throttest[i].logop_tok = curthr->throttest[i].logop_tok;
        }
        newthr->num_actions = curthr->num_actions;
        for (i = 0; i < curthr->num_actions; i++) {
            newthr->actions[i].actionverb_tok
             = curthr->actions[i].actionverb_tok;
            newthr->actions[i].actionwhat_tok
             = curthr->actions[i].actionwhat_tok;
            newthr->actions[i].actionvalue = curthr->actions[i].actionvalue;
            strncpy(newthr->actions[i].actionstring,
                    curthr->actions[i].actionstring,
                    sizeof(newthr->actions[i].actionstring));
        }
    }
    *save = NULL;
    return TRUE;
}

/***************************************************************************
 * diffconf_savedev -- save device information
 **************************************************************************/
int
diffconf_savedev(diffdev_t **phead, diffdev_t **ptail) {
    diffdev_t *newent;
    sddisk_t *cur;

    for (cur = sys[0].group[0].headsddisk; cur != NULL; cur = cur->n) {
        newent = (diffdev_t*)malloc(sizeof(diffdev_t));
        if (newent == NULL) {
            reporterr(ERRFAC, M_MALLOC, ERRCRIT, sizeof(diffdev_t));
            return FALSE;
        }
        memset(newent, 0x00, sizeof(diffdev_t));

        strcpy(newent->sddevname, cur->sddevname);
        strcpy(newent->devname, cur->devname);
        strcpy(newent->mirname, cur->mirname);
	newent->dd_minor = cur->dd_minor;
	newent->dd_major = cur->dd_major;
	newent->md_minor = cur->md_minor;
	newent->md_major = cur->md_major;
        addchain((chainedtbl_t**)phead,
                 (chainedtbl_t**)ptail,
                 (chainedtbl_t*)newent);
    }
    return TRUE;
}

/***************************************************************************
 * addchain -- add entry to chain
 **************************************************************************/
void
addchain(chainedtbl_t **phead, chainedtbl_t **ptail, chainedtbl_t *entry) {
    if (*phead == NULL && *ptail == NULL) {
        entry->p = entry->n = NULL;
        *phead = *ptail = entry;
        return;
    }
    entry->p = *ptail;
    entry->n = (*ptail)->n;
    (*ptail)->n = entry;
    *ptail = entry;
    return;
}

/***************************************************************************
 * removechain -- remove entry from chain
 **************************************************************************/
void
removechain(chainedtbl_t **phead, chainedtbl_t **ptail, chainedtbl_t *entry) {
    chainedtbl_t *cur;
    for (cur = *phead; cur != NULL; cur = cur->n) {
        if (cur == entry) {
            if (entry == *phead) {
                *phead = entry->n;
            } else {
                entry->p->n = entry->n;
            }
            if (entry == *ptail) {
                *ptail = entry->p;
            } else {
                entry->n->p = entry->p;
            }
            entry->p = entry->n = NULL;
            return;
        }
    }
}

/*
 * Move buffer pointer to beginning of next line.
 * Make the buffer pointer point to the character following
 * EOL or to NULL, if end-of-buffer. Replaces the EOL with a NULL.
 *
 * Returns NULL, if parsing failed (end-of-buffer reached). 
 * Returns pointer to start-of-line, if parsing succeeded.
 * Making getline2 of cfg_intr.c public causes duplication problems with agent... 
 */
static char *getnextline (char **buffer, int *outlen)
{
    int  len;
    char *tempbuf;

    tempbuf = *buffer;
    if (tempbuf == NULL) {
	return NULL;
    }

    /* search for EOL or NULL */
    len = 0;
    while (1) {
	if (tempbuf[len] == '\n') {
	    tempbuf[len] = 0;
	    *buffer = &tempbuf[len+1];
	    *outlen = len;
	    break;
	} else if (tempbuf[len] == 0) {
	    /* must be done! */
	    *buffer = NULL;
	    if (len == 0) tempbuf = NULL;
	    *outlen = len;
	    break;
	}
	len++;
    }

    /* done */
    return tempbuf;
}

/***************************************************************************
 * read_key_value_from_file: same as cfg_get_key_value but the file name
 *                           is not hard coded in the function
 *                           and we check against buffer overflow;
 * Note: numeric strings are expected to be preceded by "=" and terminated by ";"
 Expected file format (example from Tracking Resolution cfg file):
 ###############
 # All numbers are in KB (resolutions are KBs per bit)
 LOW_RESOLUTION=64;
 MAX_HRDB_SIZE_LOW=4096;
 MEDIUM_RESOLUTION=32;
 MAX_HRDB_SIZE_MEDIUM=8192;
 HIGH_RESOLUTION=8;
 MAX_HRDB_SIZE_HIGH=16384;
 ###############
**************************************************************************/
int
read_key_value_from_file(char *file_path, char *key, char *value, int buffer_length, int stringval)
{
    int         fd, len, i;
    char        *ptr, *buffer, *temp, *line;
    struct stat statbuf;

    if (stat(file_path, &statbuf) != 0) {
        return KEY_FILE_NOT_FOUND;
    }

    /* open the file and look for the key */
    if ((fd = open(file_path, O_RDONLY)) == -1) {
        return KEY_FILE_OPEN_ERROR;
    }

    if ((buffer = (char*)malloc(2*statbuf.st_size)) == NULL) { /* TODO: Why allocate twice the size of the file? */
        close(fd);
        return KEY_MALLOC_ERROR;
    }

    /* read the entire file into the buffer. */
    if (read(fd, buffer, statbuf.st_size) != statbuf.st_size) {
        close(fd);
        free(buffer);
        return KEY_READ_ERROR;
    }
    close(fd);
    // The read value needs to be NULL terminated, otherwise, we'll keep looking for unfound values (and modify things) out of the allocated array.
    buffer[statbuf.st_size] = 0x0;
    temp = buffer;

    /* get lines until we find the right one */
    while ((line = getnextline(&temp, &len)) != NULL) {
        if (line[0] == '#') {
            continue;
        } else if ((ptr = strstr(line, key)) != NULL) {
            /* search for quotes, if this is a string */
            if (stringval) {
                while (*ptr) {
                    if (*ptr++ == '\"') {
                        i = 0;
                        while (*ptr && (*ptr != '\"') && (i++ < (buffer_length-1))) {
                            *value++ = *ptr++;
                        }
                        *value = 0;
                        free(buffer);
                        return KEY_OK;
                    }
                }
            } else {
                while (*ptr) {
                    if (*ptr++ == '=') {
                        i = 0;
                        while (*ptr && (*ptr != ';') && (i++ < (buffer_length-1))) {
                            *value++ = *ptr++;
                        }
                        free(buffer);
						*value = 0;	   // Terminate the string with null character
                        return KEY_OK;
                    }
                }
            }
        }
    }
    free(buffer);
    return KEY_NOT_FOUND;    
}


#ifdef STANDALONE_DEBUG 
int
main (int argc, char *argv[])
{
    int ret;
    int pmdflag, psflag, openflag;

    if (argc < 6) {
        fprintf(stderr,"Usage: %s config_file pmdflag openflag\n", argv[0]);
        EXIT(EXITANDDIE);
    }
    pmdflag = atoi(argv[2]);
    psflag = atoi(argv[3]);
    openflag = atoi(argv[4]);

    ret = readconfig(pmdflag, psflag, openflag, argv[1]);
    if (ret == -1) {
        fprintf(stderr,"Failed to open %s as input\n", argv[1]);
        EXIT(EXITANDDIE);
    }

    EXIT(EXITNORMAL);
}

#endif /* STANDALONE_DEBUG */

#ifdef TDMF_TRACE
void
testthrottles (void)
{
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    FILE* fd;
#elif defined(SOLARIS)  /* 1LG-ManyDEV  ManyLG-1DEV */ 
    int fd;
#endif
    LineState lls;
    LineState* ls;

    ls = &lls;
    lls.invalid = 1;
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    fd = fopen ("./test.cfg", "r");
#elif defined(SOLARIS)  /* 1LG-ManyDEV  ManyLG-1DEV */ 
    fd = open ("./test.cfg", O_RDONLY);
#endif
    if ( 0 == getline (fd, ls)) EXIT(EXITNORMAL);
    parse_throttles (fd, ls);
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    (void) fclose (fd);
#elif defined(SOLARIS)
    (void) close (fd);
#endif
}
#endif

