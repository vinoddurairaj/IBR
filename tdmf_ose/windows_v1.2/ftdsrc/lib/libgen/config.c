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

#define TRUE  1
#define FALSE 0

#ifdef DEBUG_THROTTLE
FILE* throtfd;
FILE* oldthrotfd;
#endif /* DEBUG_THROTTLE */

/* -- structure used for reading and parsing a line from the config file */
typedef struct _line_state {
    int  invalid;       /* bogus boolean. */
    int  lineno;
    char line[256];	/* storage for parsed line */
    char readline[256]; /* unparsed line */
    /* next four elements used for parsing throttles */
    char word[256];
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
u_long    *myips;              /* array of IP addresses for this machine */
int       myipcount;           /* number of IP addresses for this machine */

static char ipstring[20];

char *paths;
static int lgnum;

extern char *argv0;

/* -- tunable parameters globals -- */

int    chunksize = CHUNKSIZE; 
int    statinterval = STATINTERVAL; 
int    maxstatfilesize = MAXSTATFILESIZE;      
int    stattofileflag = STATTOFILEFLAG;      
int    secondaryport = SECONDARYPORT;
int    syncmode = SYNCMODE;
int    syncmodedepth = SYNCMODEDEPTH;
int    syncmodetimeout = SYNCMODETIMEOUT;
int    compression = COMPRESSION;      
int    tcpwindowsize = TCPWINDOWSIZE;      
int    netmaxkbps = NETMAXKBPS;
u_long chunkdelay = CHUNKDELAY;
int    iodelay = IODELAY;
int    bablow = BABLOW;
int    babhigh = BABHIGH;

int verify_rmd_entries(group_t *group, char *path, int openflag);
static int getline (FILE *fd, LineState *ls);
static void get_word (FILE *fd, LineState *ls);
static void drain_to_eol (FILE* fd, LineState* ls);
static void forget_throttle (throttle_t* throttle);
static int verify_pmd_entries(sddisk_t *curdev, char *path, int openflag);
extern int parse_throttles (FILE* fd, LineState *ls);

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
int getpsname (int lgnum, char *ps_name)
{
    FILE *fd;
    char cfg_path[MAXPATHLEN];
    LineState lstate; 
    int error;
    sprintf(cfg_path, "%s/%s%03d.cfg", PATH_CONFIG, PMD_CFG_PREFIX, lgnum);

    if ((fd = fopen(cfg_path, "r")) == NULL) {
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
        if (strcmp("PSTORE:", lstate.key) == 0) {
            strcpy(ps_name, lstate.p1);
            DPRINTF((ERRFAC,LOG_INFO,"getpsname: %s ***\n", ps_name));
            fclose(fd);
            return 0;
        }
    }
    return -1;
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
        DPRINTF((ERRFAC,LOG_INFO,"Error: Failed to open master device: %s.\n",FTD_CTLDEV));
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

    /* set iodelay in driver */
    if (set_iodelay(master, lgnum, tunables->iodelay)!=0) {
        reporterr(ERRFAC, M_SETFAIL, ERRCRIT, "iodelay in driver", lgnum);
        close(master);
        return (-1);
    }

    /* set syncmodetimeout in driver */
    if (set_sync_timeout(master, lgnum, tunables->syncmodetimeout)!=0) {
        reporterr(ERRFAC, M_SETFAIL, ERRCRIT,
                  "sync mode timeout in driver", lgnum);
        close(master);
        return (-1);
    }

    /* make driver memory match pstore */
    memset(&statbuf, 0, sizeof(stat_buffer_t)); 
    statbuf.lg_num = lgnum;
    statbuf.len = FTD_PS_DEVICE_ATTR_SIZE;
    statbuf.addr = (char*)tunables;

    /* set these tunables in driver memory */
    if ((err = ioctl(master, FTD_SET_LG_STATE_BUFFER, &statbuf)) < 0) {
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
int set_tunables_from_buf(char *buffer, tunable_t *tunables, int size)
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
        if (verify_and_set_tunable(key, value, tunables)!=0) {
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
int verify_and_set_tunable(char *key, char *value, tunable_t *tunables) {
   
    int intval;
    int bablow, babhigh;

    /* FIXME: these verifications are also done in throttle.c - the max and
       min values need to be be shared through defines */
    if (strcmp("CHUNKSIZE:", key) == 0) {
        intval = atoi(value);
        if ( intval < 32 || intval > 10240) {
            reporterr(ERRFAC, M_CHUNKSZ, ERRCRIT, intval, 10240, 32);
            return -1;
        }
        tunables->chunksize = intval * 1024;
    } else if (strcmp("STATINTERVAL:", key) == 0) {
        intval = atoi(value);
        if ( intval < 0 || intval > 86400) {
            reporterr(ERRFAC, M_STATINT, ERRCRIT, intval, 3600, 1);
            return -1;
        }
        tunables->statinterval = intval;
    } else if (strcmp("MAXSTATFILESIZE:", key) == 0) {
        intval = atoi(value);
        if ( intval > 32000 || intval < 0 ) {
            reporterr(ERRFAC, M_STATSIZE, ERRCRIT, intval, 32000, 1);
            return -1;
        }
        tunables->maxstatfilesize = intval * 1024;
    } else if (strcmp("LOGSTATS:", key) == 0) {
        if (strcmp (value, "on") == 0 || strcmp (value, "ON")==0 || strcmp (value, "1") == 0) {
            tunables->stattofileflag = 1;
        } else if (strcmp(value, "off") == 0 || strcmp(value, "OFF") == 0 || strcmp (value, "0") == 0) {
            tunables->stattofileflag = 0;
        } else {
            reporterr(ERRFAC, M_LOGSTATS, ERRCRIT);
            return -1;
        }
    } else if (strcmp("SYNCMODEDEPTH:", key) == 0) {
        intval = atoi(value);
        if ( intval == 0 ) {
			/* DTurrin - Sept. 24th, 2001
			   99999999 is now the max value that can be entered
			   in the dialog box.                                     */
            reporterr(ERRFAC, M_SYNCDEPTH, ERRCRIT, intval, 99999999, 1);
            return -1;
        }
        tunables->syncmodedepth = intval;
    } else if (0 == strcmp("SYNCMODETIMEOUT:", key)) {
        intval = atoi(value);
        if ( intval < 1 ) {
			/* DTurrin - Sept. 24th, 2001
			   99999999 is now the max value that can be entered
			   in the dialog box.                                     */
            reporterr(ERRFAC, M_SYNCTIMEOUT, ERRCRIT, intval, 99999999, 1);
            return -1;
        }
        tunables->syncmodetimeout = intval;
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
        intval = atoi(value);
        if (intval <= 0 && intval != -1) {
            reporterr(ERRFAC, M_NETMAXERR, ERRCRIT, intval, INT_MAX, 1);
            return -1;
        }
        tunables->netmaxkbps = atoi(value);
    } else if (strcmp("IODELAY:", key) == 0) {
        intval = atoi(value);
        if (intval < 0) {
            reporterr(ERRFAC, M_IODELAY, ERRCRIT, intval, 500000, 0);
            return -1;
        }
        if (intval > 500000) {
            reporterr(ERRFAC, M_IODELAY, ERRCRIT, intval, 500000, 0);
            return -1;
        }
        tunables->iodelay = atoi(value);
    } else if (strcmp("CHUNKDELAY:", key) == 0) {
        intval = atoi (value);
        if (intval < 0) {
            reporterr(ERRFAC, M_CHUNKDELAY, ERRCRIT, intval, INT_MAX, 0);
            return -1;
        }
        tunables->chunkdelay = atoi(value);
    } else if (strcmp("BABLOW:", key) == 0) {
        intval = atoi(value);
        babhigh = tunables->babhigh == -1 ? 100: tunables->babhigh;
        if (intval < 0 || intval > babhigh) {
            reporterr(ERRFAC, M_BABLOW, ERRCRIT,
                intval, babhigh, 0);
            return -1;
        }
        tunables->bablow = intval;
    } else if (strcmp("BABHIGH:", key) == 0) {
        intval = atoi(value);
        bablow = tunables->bablow == -1 ? 100: tunables->bablow;
        if (intval < bablow || intval > 100) {
            reporterr(ERRFAC, M_BABLOW, ERRCRIT,
                intval, 100, bablow);
            return -1;
        }
        tunables->babhigh = intval;
    }  else { 
        return -1;
    }
    return 0;
}
/***********************************************************************
  settunable -  sets a tunable paramter given a <KEY>: and value.
**********************************************************************/
int settunable (int lgnum, char *inkey, char *invalue) 
{
    char  *inbuffer, *temp, *outbuffer;
    char  group_name[MAXPATHLEN];
    char  line[MAXPATHLEN];
    char  ps_name[MAXPATHLEN];
    int   num_ps;
    int  i, j, ret, found, linelen, len;
    tunable_t tunables;
    ps_version_1_attr_t attr;
    char *ps_key[FTD_MAX_KEY_VALUE_PAIRS];
    char *ps_value[FTD_MAX_KEY_VALUE_PAIRS];

    if (inkey[strlen(inkey) - 1] != ':') {
        reporterr(ERRFAC, M_BADKEY, ERRCRIT, inkey, "settunable");
        exit(1);
    }

    FTD_CREATE_GROUP_NAME(group_name, lgnum);

    if (getpsname(lgnum, ps_name) != 0) {
        i = numtocfg(lgnum);
        sprintf(line, "%s/p%03d.cfg", PATH_CONFIG, lgnum);
        reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, line);
        exit(1);
    }

    /* read the persistent store for info on this group */
    ret = ps_get_version_1_attr(ps_name, &attr);
    if (ret != PS_OK) {
        reporterr(ERRFAC, M_PSRDHDR, ERRCRIT, ps_name);
        exit(1);
    }

    /* allocate an input buffer and output buffer for the parameters */
    if (((inbuffer = (char *)malloc(attr.group_attr_size)) == NULL) ||
        ((outbuffer = (char *)calloc(attr.group_attr_size, 1)) == NULL)) {
        reporterr(ERRFAC, M_MALLOC, ERRCRIT, attr.group_attr_size);
        exit(1);
    }
    ret = ps_get_group_attr(ps_name, group_name, inbuffer, attr.group_attr_size);
    if (ret != PS_OK) {
        reporterr(ERRFAC, M_PSRDGATTR, ERRCRIT, group_name, ps_name);
        exit(1);
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
    if (getdrvrtunables(lgnum, &tunables) < 0) {
        return -1;
    }
    /* parse them and set the appropriate tunable fields */
    if (set_tunables_from_buf(outbuffer, &tunables, attr.group_attr_size) 
        != 0) {
        return (-1);
    }

    /* set the driver specific tunables and driver buffer area for 
       throtd to pmd IPC */
    if (setdrvrtunables(lgnum, &tunables)!=0) {
        return (-1);
    }

    /* dump new buffer to the ps */
    ret = ps_set_group_attr(ps_name, group_name, outbuffer, attr.group_attr_size);
    if (ret != PS_OK) {
        return (-1);
    }
 
    free(inbuffer);
    free(outbuffer);

    return 0; 
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
        DPRINTF((ERRFAC,LOG_INFO,"Error: Failed to open master device: %s.\n",            FTD_CTLDEV));
        return -1;
    }
    lgbuf = ftdmalloc(FTD_PS_GROUP_ATTR_SIZE);

    memset(&statbuf, 0, sizeof(stat_buffer_t)); 
    statbuf.lg_num = lgnum;
    statbuf.len = FTD_PS_GROUP_ATTR_SIZE;
    statbuf.addr = (char*)lgbuf;

    /* read from driver buffer */
    if ((err = ioctl(master, FTD_GET_LG_STATE_BUFFER, &statbuf)) < 0) {
/*
printf("\n$$$ GET group state err: %d\n",err);
*/
        free(lgbuf);
        close(master);
        return -1;
    }
    memcpy(tunables, statbuf.addr, sizeof(tunable_t));
/*
printf("\n$$$ tunables->bablow: %d\n",tunables->bablow);
*/
    free(lgbuf);
    close(master);

    return 0;
} /* getdrvrtunables */

/***************************************************************************
 * gettunables - get tunables from persistent store
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

    lgnum = cfgpathtonum(sys[0].configpath);

    /* open the system control device */
    if ((master = open(FTD_CTLDEV, O_RDWR)) < 0) {
        reporterr(ERRFAC, M_CTLOPEN, ERRCRIT,
            argv0, strerror(errno), FTD_CTLDEV);
        DPRINTF((ERRFAC,LOG_INFO,"Error: Failed to open master device: %s.\n",FTD_CTLDEV));
        return(NULL);
    }

    if (psflag) {
        if (getpsname(lgnum, ps_name) !=0 ) {
            sprintf(ps_name, "%s/p%03d.cfg", PATH_CONFIG, lgnum);
            reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, ps_name);
            return -1;
        }
        /* get header info */
        if ((rc = ps_get_version_1_attr(ps_name, &attr))!=PS_OK) {
            reporterr(ERRFAC, M_PSRDGATTR, ERRCRIT, group_name, ps_name);
            close(master);
            return -1;
        }
        
        buff_len = attr.dev_attr_size;
        
        retbuffer = (char *) ftdmalloc(buff_len*sizeof(char));
        
        /* get the tunables */
        if (ps_get_group_attr(ps_name, group_name, retbuffer, buff_len) != PS_OK) {
            reporterr(ERRFAC, M_PSRDGATTR, ERRCRIT, group_name, ps_name);
            free(retbuffer);
            close(master);
            return -1;
        }
        buf_ptr = retbuffer;

        /* get current driver tunables */
        getdrvrtunables(lgnum, &sys[0].tunables);

        /* parse them and set the appropriate machine_t fields */
        set_tunables_from_buf(buf_ptr, &(sys[0].tunables), buff_len);
        memcpy(&(sys[1].tunables), &(sys[0].tunables), sizeof(tunable_t));

        /* make driver memory match pstore */
        memset(&statbuf, 0, sizeof(stat_buffer_t)); 
        statbuf.lg_num = lgnum;
        statbuf.len = FTD_PS_GROUP_ATTR_SIZE;
        statbuf.addr = (char*)&sys[0].tunables;

        /* set these tunables in driver memory */
        if ((err = ioctl(master, FTD_SET_LG_STATE_BUFFER, &statbuf)) < 0) {
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
 * isthime - figures out if a hostname and/or IP address is this machine
 *             1 = this hostname / ip addr pertains to this machine
 *             0 = this hostname / ip addr does not pertain to this machine
 *            -1 = unresolvable host name
 *            -2 = unresolvable IP addr
 *            -3 = hostname and IP addr conflict
 *        this function requires that getnetconfcount and getnetconfs has
 *        been previously called.
 **************************************************************************/
int
isthisme (char* hostnodename, u_long ip)
{
    int nameretval;
    int ipretval;
    struct hostent *hp;
    char **p;
    char **q;
    struct in_addr in;
    u_long hostnameip;
    int j;
    
    nameretval = -2;
    ipretval = -2;
    hostnameip = 0L;
    
    if ((hostnodename == (char*) NULL || strlen(hostnodename) == 0) &&
        ip == 0L) return (-3);
    if (ip == 0xffffffff) return(-2);
    /* -- check out the node name, if it exists */
    if (hostnodename != (char*) NULL && strlen(hostnodename) > 0) {
        hp = sock_gethostbyname(hostnodename);
        if (hp == NULL) 
            return (-1);
        nameretval = 0;
        (void) memcpy(&in.s_addr, *(hp->h_addr_list), sizeof(in.s_addr));
        if (0 == strcmp(hostnodename, hp->h_name)) {
            hostnameip = in.s_addr;
            for (j=0; j<myipcount; j++) {
                if (myips[j] == in.s_addr) {
                    nameretval = 1;
                    break;
                }
            }
        } else {
            for (p = hp->h_addr_list; *p != 0; p++) {
                (void) memcpy(&in.s_addr, *p, sizeof(in.s_addr));
                for (q = hp->h_aliases; *q != 0; q++) {
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
        hp = gethostbyaddr ((char*)&ip, sizeof(ip), AF_INET);
        if (hp == NULL) {
            return (-2);
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
        hp = sock_gethostbyname(hostnodename);
        (void) memcpy(&in.s_addr, *(hp->h_addr_list), sizeof(in.s_addr));
        if (ip != in.s_addr) 
            return (-3);
    }
    if (nameretval == -2 && ipretval == -2)
        return (-3);
    if (nameretval != -2 && ipretval == -2)
        return (nameretval);
    if (nameretval == -2 && ipretval != -2)
        return (ipretval);
    if (nameretval != ipretval)
        return (-3);
    return (nameretval);
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
int
getconfigs (char paths[][32], int who, int startflag) 
{
    DIR* dfd;
    struct dirent* dent;
    char line[512];
    char ps_name[MAXPATHLEN];
    char group_name[MAXPATHLEN];
    char startstr[32];
    char cfg_path_suffix[4];
    char *cfg_path_prefix;
    int rc;
    int i;

    i = 0;
    dfd = opendir(PATH_CONFIG);
    if (dfd == (DIR*)NULL) {
        if (errno == EMFILE) {
            reporterr(ERRFAC, M_FILE, ERRCRIT, PATH_CONFIG, strerror(errno));
        }
        sprintf(line, "[error opening directory %s]", PATH_CONFIG);
        reporterr (ERRFAC, M_CFGFILE, ERRCRIT,
            argv0, line, strerror(errno));
        exit (EXITANDDIE);
    }
    /* -- get ip addresses for this machine */
    myipcount = 0;
    myipcount = getnetconfcount();
    myips = (u_long*) NULL;
    if (myipcount > 0) {
        myips = (u_long*) malloc ((sizeof(u_long)) * myipcount);
        getnetconfs (myips);
    }
    if (startflag) {
        strcpy(cfg_path_suffix, "cur");
    } else {
        strcpy(cfg_path_suffix, PATH_CFG_SUFFIX);
    }

    if (ISPRIMARY(mysys)) {
        cfg_path_prefix = "p";
    } else {
        cfg_path_prefix = "s";
    }
 
    while (NULL != (dent = readdir(dfd))) {
        /* XXX BIG NOTE!!! - change this test when config file naming opens up */
        if (8 != strlen(dent->d_name)) {
            continue;
        }
        if (0 == (strncmp(&(dent->d_name[5]), cfg_path_suffix, 3))) { 
            if (who == 1) {
                if (0 == strncmp(dent->d_name, PMD_CFG_PREFIX, strlen(PMD_CFG_PREFIX))) {
                    if (startflag) {
                        lgnum = cfgpathtonum(dent->d_name);
                        if (getpsname(lgnum, ps_name) !=0 ) {
/*
                            sprintf(line, "%s/%s%03d.%s",
                                PATH_CONFIG, cfg_path_prefix, 
                                lgnum, cfg_path_suffix);
                            reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, 
                                line);
*/
/*
                            (void) closedir (dfd);
                            return -1;
*/
                            continue;
                        }
                        
                        FTD_CREATE_GROUP_NAME(group_name, lgnum);
                        /* check _AUTOSTART in pstore */
                        rc = ps_get_group_key_value(ps_name, group_name, "_AUTOSTART:", startstr);
                        if (rc != PS_OK) {
                            continue;
                        }
                        if (strcmp(startstr, "yes")) {
                            continue;
                        }
                    }
                    strcpy (paths[i++], dent->d_name);
                    paths[i][0] = '\0';
                    continue;
                }
            } else if (who == 2) {
                if (0 == strncmp(dent->d_name, RMD_CFG_PREFIX, strlen(RMD_CFG_PREFIX))) {
                    strcpy (paths[i++], dent->d_name);
                    paths[i][0] = '\0';
                    continue;
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
    sys[0].iodelayms = 0L;
    sys[0].setiodelayflag = 0;
    sys[0].ip = 0L;
    sys[0].secondaryport = secondaryport;
    sys[0].sock = -1;
    sys[0].isconnected = 0;
    sys[0].lgsnmaster = 0;
    sys[0].statfd = (FILE*) NULL;
 
    sys[0].tunables.chunksize = chunksize * 1024;
    sys[0].tunables.statinterval = statinterval;
    sys[0].tunables.maxstatfilesize = maxstatfilesize * 1024;
    sys[0].tunables.stattofileflag = stattofileflag;
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
    sys[0].tunables.bablow = bablow;
    sys[0].tunables.babhigh = babhigh;

    memset(sys[0].group, 0, sizeof(group_t));
    
    sys[1] = sys[0];
    
    /* -- get ip addresses for this machine */
    myipcount = 0;
    myipcount = getnetconfcount();
    myips = (u_long*) NULL;
    if (myipcount > 0) {
        myips = (u_long*) malloc ((sizeof(u_long)) * myipcount);
        getnetconfs (myips);
    }
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
getline (FILE *fd, LineState *ls)
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
        if (fgets(line, 256, fd) == NULL) {
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
  DPRINTF((ERRFAC,LOG_INFO,"LINE(%d): = %s\n", ls->lineno - 1, ls->readline)));
  */
    return TRUE;
}

/***************************************************************************
 * get_word -- obtains the next word from the config file's line / file
 *              (and handles continuation characters)
 *
 **************************************************************************/
static void
get_word (FILE* fd, LineState* ls)
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
drain_to_eol (FILE* fd, LineState* ls)
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

    sign = 1;
    ls->valueflag = 0;
    if (0 == (len = strlen(ls->word))) return (0);
    digitflag = 1;
    value = 0;
    j=0;
    while (ls->word[j] == '-') {
        sign = -1;
        j++;
    }
    for (i=j; i<len; i++) {
        if ('0' <= ls->word[i] && '9' >= ls->word[i]) {
            value = (value * 10) + (int) (ls->word[i] - '0');
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
parse_system(FILE *fd, LineState *ls)
{
    machine_t *currentsys;
    group_t *group;

    /* we don't read a line; we just parse the one we have */
    if (strcmp("PRIMARY", ls->p2) == 0) {
        currentsys = &sys[0];
    } else if (strcmp("SECONDARY", ls->p2) == 0) {
        currentsys = &sys[1];
    } else {
        reporterr(ERRFAC, M_CFGERR, ERRCRIT, sys[0].configpath, ls->lineno);
        return -1; 
    }
    currentsys->name[0] = 0;
    currentsys->hostid = 0;
    currentsys->ip = 0;

    group = currentsys->group;
    group->sync_depth = (unsigned int) -1;
    FTD_CREATE_GROUP_NAME(group->devname, lgnum);
    group->chaining = 0;

    while (1) {
        if (!getline(fd, ls)) {
            break;
        }
        if (strcmp("HOST:", ls->key) == 0) {
            strcpy(currentsys->name, ls->p1);
        } else if (strcmp("PSTORE:", ls->key) == 0) {
            strcpy(group->pstore, ls->p1);
        } else if (strcmp("JOURNAL:", ls->key) == 0) {
            strcpy(group->journal_path, ls->p1);
        } else if (0 == strcmp("SECONDARY-PORT:", ls->key )) {
            currentsys->secondaryport = atoi(ls->p1);
        } else if (0 == strcmp("CHAINING:", ls->key )) {
            if ((strcmp("on", ls->p1) == 0 ) || (strcmp("ON",ls->p1)==0) ) {
                group->chaining = 1;
            } else {
                group->chaining = 0;
            }
        } else {
            ls->invalid = FALSE;
            break;
        }
    }
    /* we must have either the name or the IP address for the host */
    if ((strlen(currentsys->name) == 0) && (currentsys->ip == 0)) {
        reporterr(ERRFAC, M_BADNAMIP, ERRCRIT, currentsys->configpath, 
            ls->lineno, currentsys->name, iptoa(currentsys->ip));
        return -1; 
    }
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
parse_profile(FILE *fd, LineState *ls)
{
    group_t      *group;
    sddisk_t     disk, *curdev;
    char         *rest;
    int          moreprofiles = TRUE;

    while (moreprofiles) {
        moreprofiles=FALSE;
        memset(&disk, 0, sizeof(sddisk_t));
        group = sys[0].group;
      
        while (1)  {
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
          } else if (0 == strcmp(CAPPRODUCTNAME_TOKEN "-DEVICE:", ls->key)) {
            strcpy(disk.sddevname, ls->p1);
            disk.devid = parseftddevnum(disk.sddevname);
          } else if (0 == strcmp("DATA-DISK:", ls->key)) {
            strcpy(disk.devname, ls->p1);
            disk.devsize = disksize(disk.devname);
          } else if (0 == strcmp("MIRROR-DISK:", ls->key)){
            strcpy (disk.mirname, ls->p1);
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
parse_throttles (FILE* fd, LineState *ls) 
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
                if (ls->word == "") break;
                tok = ls->word;
		throttle->num_throttest++;
		if (throttle->num_throttest > 16) {
		    reporterr (ERRFAC, M_THROT2TST, ERRWARN, 
                               sys[0].configpath, ls->lineno);
                    state = 5;
                    forget_throttle (throttle);
                    drain_to_eol (fd, ls);
                    if (0 == getline(fd, ls)) return (0);
                    continue;
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
                    if (0 == getline(fd, ls)) return (0);
                    continue;
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
                    if (0 == getline(fd, ls)) return (0);
                    continue;
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
                    if (0 == getline(fd, ls)) return (0);
                    continue;
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
                    if (0 == getline(fd, ls)) return (0);
                    continue;
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
                    if (0 == getline(fd, ls)) return (0);
                    continue;
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
 * readconfig -- process and parse a configuration file
 **************************************************************************/
int
readconfig(int pmdflag, int pstoreflag, int openflag, char *filename)
{
    FILE *fd;
    LineState lstate;
    int result;
    int error;
    int tryquietly;

    sprintf(sys[0].configpath, "%s/%s", PATH_CONFIG, filename);
    if ((fd = fopen(sys[0].configpath, "r")) == NULL) {
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
            if ((result = parse_system (fd, &lstate)) < 0) {
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
    fclose(fd);

    if (pstoreflag > 0) { 
        tryquietly = 0;
        if (pstoreflag == 2) {
            tryquietly = 1;
        }
        /* retrieve the tunables from the persistent store */
        if (gettunables(sys[0].group->devname, 1, tryquietly) == -1) {
            return -1;
        }
    }
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
    /* if the rmd is calling, then switch mysys and othersys */
    if (!pmdflag) {
        mysys = &sys[1];
        othersys = &sys[0];
    } else {
        mysys = &sys[0];
        othersys = &sys[1];
    }

    return (error);
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
    return 0;
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
    int i;

    curdev = group->headsddisk;
    while (curdev != NULL) {
        /* stat the mirror device and make sure it is a char device. */
        if (stat(curdev->mirname, &statbuf) != 0) {
            strcpy (emsg, strerror(errno));
            reporterr (ERRFAC, M_MIRSTAT, ERRCRIT, curdev->mirname, emsg);
            return (-1);
        }
        if (!S_ISCHR(statbuf.st_mode)) {
            reporterr (ERRFAC, M_MIRTYPERR, ERRCRIT, curdev->mirname,
                path, 0);
            return (-1);
        }
        curdev->no0write = 0;
        curdev->mirfd = -1;

        /* open the mirror device */
        if (openflag) {
            open_tries = 10;
            for (i = 0; i < open_tries; i++) {
                /* kluge to get around a race with RMDA */
                DPRINTF((ERRFAC,LOG_INFO,                   "\n*** verify_rmd_entries: open try #: %d\n",i));
                save_errno = 0;
                if (group->chaining != 1) {
                    curdev->mirfd = open(curdev->mirname, O_RDWR | O_SYNC | O_EXCL);
                } else {
                    curdev->mirfd = open(curdev->mirname, O_RDWR | O_SYNC );
                }

                if (curdev->mirfd < 0) {
                    if (errno == EMFILE) {
                        reporterr (ERRFAC, M_FILE, ERRCRIT, curdev->mirname, 
                            strerror(errno));
                    } else {
                        save_errno = errno;
                    }
                    sleep(1);
                } else {
                    break;
                }
            }
            if (curdev->mirfd < 0 && i == open_tries) {
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
                if (ioctl(curdev->mirfd, DKIOCINFO, &dkinfo) < 0) {
                    break;
                }
                if (ioctl(curdev->mirfd, DKIOCGVTOC, &dkvtoc) < 0) {
                    break;
                }
                if (dkinfo.dki_ctype != DKC_MD
                && dkvtoc.v_part[dkinfo.dki_partition].p_start == 0) {
                    DPRINTF((ERRFAC,LOG_INFO, "Sector 0 write dissallowed for device: %s\n",	                        curdev->mirname));
                    curdev->no0write = 1;
                }
            } while (0);	
#endif /* SOLARIS */
        }
        curdev = curdev->n;
    }
    return 0;
}

#ifdef STANDALONE_DEBUG 
int
main (int argc, char *argv[])
{
    int ret;
    int pmdflag, psflag, openflag;

    if (argc < 6) {
        fprintf(stderr,"Usage: %s config_file pmdflag openflag\n", argv[0]);
        exit(1);
    }
    pmdflag = atoi(argv[2]);
    psflag = atoi(argv[3]);
    openflag = atoi(argv[4]);

    ret = readconfig(pmdflag, psflag, openflag, argv[1]);
    if (ret == -1) {
        fprintf(stderr,"Failed to open %s as input\n", argv[1]);
        exit(1);
    }

    exit(0);
}

#endif /* STANDALONE_DEBUG */

#ifdef DEBUG
void
testthrottles (void)
{
    FILE* fd;
    LineState lls;
    LineState* ls;

    ls = &lls;
    lls.invalid = 1;
    fd = fopen ("./test.cfg", "r");
    if ( 0 == getline (fd, ls)) exit(0);
    parse_throttles (fd, ls);
    (void) fclose (fd);
}
#endif
