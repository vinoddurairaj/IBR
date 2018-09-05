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
/****************************************************************************
 * common.c - FullTime Data 
 *
 * (c) Copyright 1997, 1998 FullTime Software, Inc.
 *     All Rights Reserved
 *
 * This module implements the functionality for common FTD daemon routines 
 *
 ***************************************************************************/

#include <stdlib.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>       //for lib functions 'isdigit'
#include <sys/resource.h>
#include <unistd.h>
#include <strings.h>

#define  TDMF_TRACE_MAKER
#include "errors.h"
#include "config.h"
#include "pathnames.h"
#include "procinfo.h"
#include "ipc.h"
#include "ftdio.h"
#include "ftd_kern_ctypes.h"
#include "common.h"
#include "ftdif.h"
#undef	 TDMF_TRACE_MAKER

#ifdef  TDMF_TRACE
   #if ! defined(HPUX)
   #include "sys/procfs.h"
   #endif
#include "time.h"
#endif

#include "devcheck.h"
#ifdef NEED_SYS_MNTTAB
#include "ftd_mount.h"
#else
#include <sys/mnttab.h>
#endif

#if defined(SOLARIS) /* 1LG-ManyDEV */ 
extern int dummyfd;
#endif

/*
 * For IOCTL debugging output filer
 */
#if defined(SOLARIS)
#include "sys/dkio.h"
#endif
#if defined(_AIX)
#include "sys/lvdd.h"
#endif
#if defined(HPUX)
#include "sys/diskio.h"
#endif
#if defined(linux)
#include "linux/fs.h"
#endif

int lgcnt;

extern char *argv0;
extern char *paths;
extern int _pmd_state;
extern int _pmd_cpon;
extern int _pmd_cpstart;
extern int _pmd_configpath;
extern int _pmd_state_change;
extern int _pmd_verify_secondary; 
extern int _resuming_checksum_refresh;
extern int _pmd_refresh;

extern ftdpid_t p_cp[];
extern ftdpid_t r_cp[];

int compress_buf_len = 0;
char *compress_buf = NULL;
stat_buffer_t statBuf;
ftd_stat_t lgstat;

struct stat statbuf[1];

#define IHASH(x) do {iHash = (iHash << 4) ^ (x);} while(0)
#define OHASH(x) do {oHash = (oHash << 4) ^ (x);} while(0)
#define GUESS_TABLE_SIZE 65536

static unsigned short int iHash = 0;
static unsigned short int oHash = 0;
static unsigned char *InputGuessTable = NULL;
static unsigned char *OutputGuessTable = NULL;
static int iGuessTableInited = 0;
static int oGuessTableInited = 0;

statestr_ts statestr[] = {
    {0,       "NULL",    "invalid"},
    {FTDPMD,  "FTDPMD",  "pmd normal"},
    {FTDBFD,  "FTDBFD",  "back refresh"},
    {FTDRFD,  "FTDRFD",  "smart refresh"},
    {FTDRFDF, "FTDRFDF", "full refresh"},
    {FTDRFDC, "FTDRFDC", "checksum refresh"},
    {FTDRFDR, "FTDRFD.", "full refresh restart"}
};

#ifdef TDMF_TRACE
int ftd_debugFlag = \
                    FTD_DBG_DEFAULT;
#else
int ftd_debugFlag = 0;
#endif

char  debug_msg[128];   // For general tracing with reporterr, using M_GENMSG message code

// WI_338550 December 2017, implementing RPO / RTT
static int debug_RPO_timestamp_queue = 0;
static int RPO_timestamp_queue_num_of_entries = 0;
static int Max_RPO_timestamp_queue_num_of_entries_recorded_so_far = 0;
static unsigned long cumulative_RPO_timestamp_queue_entries_so_far = 0;

static void trace_RPO_timestamp_queue(int log_head_tail_and_size, unsigned long queue_head, unsigned long queue_tail, unsigned long queue_size)
{
    if( debug_RPO_timestamp_queue )
    {
        sprintf( debug_msg, "TRACING RPO_timestamp_queue_num_of_entries: %d\n", RPO_timestamp_queue_num_of_entries);
        reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg);
        sprintf( debug_msg, "TRACING cumulative_RPO_timestamp_queue_entries_so_far: %lu\n", cumulative_RPO_timestamp_queue_entries_so_far);
        reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg);
        sprintf( debug_msg, "TRACING Max_RPO_timestamp_queue_num_of_entries_recorded_so_far: %d\n", Max_RPO_timestamp_queue_num_of_entries_recorded_so_far);
        reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg);
        if( log_head_tail_and_size )
        {
            sprintf( debug_msg, "TRACING queue_head: %lu\n", queue_head);
            reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg);
            sprintf( debug_msg, "TRACING queue_tail: %lu\n", queue_tail);
            reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg);
            sprintf( debug_msg, "TRACING queue_size: %lu\n", queue_size);
            reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg);
        }
    }
}

#ifdef TDMF_TRACE
int
ftd_dbgCheckPrint(const char *mod, int cmd)
{
    int toprt = 0;
    int notprt = 1;

    if ((strcmp(mod, "config.c") == 0) &&
        (cmd == FTD_GET_LG_STATE_BUFFER) &&
        !(ftd_debugFlag & FTD_DBG_THROTD)) {
        return notprt;
    }

    if ((strcmp(mod, "throtd.c") == 0) &&
        !(ftd_debugFlag & FTD_DBG_THROTD)) {
        return notprt;
    }

    if ((strcmp(mod, "stat_intr.c") == 0) && 
        !(ftd_debugFlag & FTD_DBG_PMD_RMD)) {
        return notprt;
    }

    if ((strcmp(mod, "rmd.c") == 0) &&
        !(ftd_debugFlag & FTD_DBG_PMD_RMD)) {
        return notprt;
    }

#if defined(SOLARIS)
    if ((cmd == PIOCPSINFO) &&
        !(ftd_debugFlag & FTD_DBG_THROTD)) {
        return notprt;
    }

    if (((cmd == DKIOCINFO) || (cmd == DKIOCGVTOC)) &&
        !(ftd_debugFlag & FTD_DBG_IOCTL)) {
        return notprt;
    }
#endif
#if defined(_AIX)
    if ((cmd == LV_INFO) &&
        !(ftd_debugFlag & FTD_DBG_IOCTL)) {
        return notprt;
    }
#endif
#if defined(HPUX)
    if ((cmd == DIOC_CAPACITY) &&
        !(ftd_debugFlag & FTD_DBG_IOCTL)) {
        return notprt;
    }
#endif
#if defined(linux)
    if ((cmd == BLKGETSIZE) &&
        !(ftd_debugFlag & FTD_DBG_IOCTL)) {
        return notprt;
    }
#endif

    if (!(ftd_debugFlag & FTD_DBG_IOCTL)) {
        return notprt;
    }

    return toprt;
}

int
ftd_ioctl_trace(char *mod, char *funct, int cmd)
{
	int i;
	if (cmd) {
        if (ftd_dbgCheckPrint(mod, cmd)) {
            return (0);
        }

		i = (cmd & 0xFF)-1;
	 	
		if (i<0 || i >= MAX_TDMF_IOCTL) {
            DPRINTF("%d<-%d: %s/%s IOCTL [0x%08x]: ???\n",
                    getpid(), getppid(),mod, funct, cmd);
        } else {
            DPRINTF("%d<-%d: %s/%s() IOCTL [0x%08x]: %s \n",
                    getpid(), getppid(), mod, funct, cmd,
                    tdmf_ioctl_text_array[i].cmd_txt);
        }		
	}

	return (0);
}
#endif	    
/****************************************************************************
 * compress -- compress the buffer 
 ***************************************************************************/
int
compress(u_char *source, u_char *dest, int len)
{
    int i, bitmask;
    unsigned char *flagdest, flags, *orgdest;
    int olen;

    olen = 0;
    if (!oGuessTableInited) {
        OutputGuessTable = (unsigned char*)ftdcalloc(1, GUESS_TABLE_SIZE);
        memset(OutputGuessTable, 0, GUESS_TABLE_SIZE);
        oHash = 0;
        oGuessTableInited = 1;
    }

    orgdest = dest;
    while (len) {
        if ((++olen) > compress_buf_len) {
            /* -- keep output buffer from overflowing */
            compress_buf_len = 
                compress_buf_len + (compress_buf_len >> 1) + 1;
            compress_buf = (char*) ftdrealloc (compress_buf, 
                                               compress_buf_len);
            dest = (u_char*) compress_buf + (dest-orgdest);
            orgdest = (u_char*) compress_buf;
        }
        flagdest = dest++;
        flags = 0;			/* All guess wrong initially */
        for (bitmask = 1, i = 0; i < 8 && len; i++, bitmask <<= 1) {
            if (OutputGuessTable[oHash] == *source) {
                flags |= bitmask;	/* Guess was right - don't output */
            } else {
                OutputGuessTable[oHash] = *source;
                if ((++olen) > compress_buf_len) {
                    /* -- keep output buffer from overflowing */
                    compress_buf_len = 
                        compress_buf_len + (compress_buf_len >> 1) + 1;
                    compress_buf = (char*) ftdrealloc (compress_buf, 
                                                       compress_buf_len);
                    dest = (u_char*) compress_buf + (dest-orgdest);
                    orgdest = (u_char*) compress_buf;
                }
                *dest++ = *source;	/* Guess wrong, output char */
            }
            OHASH(*source++);
            len--;
        }
        *flagdest = flags;
    }

    return (dest - orgdest);
} /* compress */

/****************************************************************************
 * decompress -- decompress the buffer 
 ***************************************************************************/
int
decompress(u_char *source, u_char *dest, int len)
{
    int i, bitmask;
    unsigned char flags, *orgdest;

    if (!iGuessTableInited) {
        InputGuessTable = (unsigned char*)ftdcalloc(1, GUESS_TABLE_SIZE);
        memset(InputGuessTable, 0, GUESS_TABLE_SIZE);
        iHash = 0;
        iGuessTableInited = 1;
    }
    orgdest = dest;
    while (len) {
        flags = *source++;
        len--;
        for (i = 0, bitmask = 1; i < 8; i++, bitmask <<= 1) {
            if (flags & bitmask) {
                *dest = InputGuessTable[iHash];	/* Guess correct */
            } else {
                if (!len)
                    break;		/* we seem to be really done -- cabo */
                InputGuessTable[iHash] = *source;	/* Guess wrong */
                *dest = *source++;	/* Read from source */
                len--;
            }
            IHASH(*dest++);
        }
    }
    return (dest - orgdest);
} /* decompress */

/****************************************************************************
 * is_parent_master -- is parent the Master daemons (in.ftdd) ? 
 ***************************************************************************/
int
is_parent_master(void)
{
    proc_info_t ***p;
    pid_t pid, ppid;
    int pcnt;
    int i;

    ppid = getppid();
    pid = 0;
    if ((p = (proc_info_t ***)malloc(sizeof(proc_info_t **))) == NULL) {
        return 0;
    }
    *p = NULL;
    if ((pcnt = get_proc_names(FTDD, 0, p)) > 0) {
        for (i = 0; i < pcnt; i++) {
            pid = (*p)[i]->pid;
            if (pid == ppid) {
                del_proc_names(p, pcnt);
                free(p);
                return 1;
            }
        }
    }
    del_proc_names(p, pcnt);
    free(p);
    return 0;
} /* is_parent_master */

/****************************************************************************
 * get_lg_dev -- return device handle given group handle and target devid
 ***************************************************************************/
sddisk_t *
get_lg_dev(group_t *group, int devid)
{
	sddisk_t *sd;

	for (sd = group->headsddisk; sd; sd = sd->n) {
		if (devid == sd->devid) {
			return sd;
		}
	}		 
	return NULL;
} /* get_lg_dev */

/****************************************************************************
 * get_lg_rdev -- return device handle given group handle and target rdev
 ***************************************************************************/
sddisk_t *
get_lg_rdev(group_t *group, u_longlong_t rdev)
{
	sddisk_t *sd;

	for (sd = group->headsddisk; sd; sd = sd->n) {
		if (rdev == sd->dd_rdev) {
			return sd;
		}
	}		 
	return NULL;
} /* get_lg_rdev */

/****************************************************************************
 * getdevice -- return corresponding ftd device handle given rsync device 
 ***************************************************************************/
sddisk_t *
getdevice(rsync_t *rsync)
{
    group_t *group;
    sddisk_t *sd;

    group = mysys->group;
    for (sd = group->headsddisk; sd; sd = sd->n) {
        if (rsync->devid == sd->devid) {
            sd->rsync.size = rsync->size;
            return sd;
        }
    }		 
    return NULL;
} /* getdevice */

/****************************************************************************
 * syncgroup -- sync data to the devices in group.
 ***************************************************************************/
void
syncgroup(int force)
{
	group_t *group = mysys->group;
	sddisk_t *sd;

	for (sd = group->headsddisk; sd; sd = sd->n) {
		if (force || sd->dirty) {
			fsync(sd->mirfd);
			sd->dirty = 0;
		}
    	}
}

/****************************************************************************
 * ftdmalloc -- call malloc and report error on failure
***************************************************************************/
char *
ftdmalloc(int len) 
{
	char *mptr;

	if ((mptr = (char*)malloc(len)) == NULL) {
		reporterr(ERRFAC, M_MALLOC, ERRCRIT, len);
		EXIT(EXITANDDIE);
	}
	return mptr;
} /* ftdmalloc */

/****************************************************************************
 * ftdvalloc -- page-aligned malloc
***************************************************************************/
void *
ftdvalloc(size_t len)
{
    void *alloc = NULL;
#if defined(linux)
    errno = posix_memalign(&alloc, sysconf(_SC_PAGESIZE), len);
#else
    alloc = (void *)valloc(len);
#endif
	if (alloc == NULL) {
		reporterr(ERRFAC, M_VALLOC, ERRCRIT, len, get_error_str(errno));
		EXIT(EXITANDDIE);
	}
    return alloc;
}

/****************************************************************************
 * ftdmemalign -- aligned malloc
***************************************************************************/
void *
ftdmemalign(size_t len, size_t align)
{
    void *alloc = NULL;
#if defined(linux)
    errno = posix_memalign(&alloc, align, len);
#elif defined(SOLARIS)
    alloc = memalign(align, len);
#else
    alloc = (void *)valloc(len);
#endif
	if (alloc == NULL) {
		reporterr(ERRFAC, M_MEMALIGN, ERRCRIT, len, align, get_error_str(errno));
		EXIT(EXITANDDIE);
	}
    return alloc;
}

/****************************************************************************
 * ftdcalloc -- call calloc and report error on failure
***************************************************************************/
char *
ftdcalloc(int cnt, int len) 
{
	char *mptr;

	if ((mptr = (char*)calloc(cnt, len)) == NULL) {
		reporterr(ERRFAC, M_MALLOC, ERRCRIT, len);
		EXIT(EXITANDDIE);
	}
	return mptr;
} /* ftdcalloc */

/****************************************************************************
 * ftdrealloc -- call realloc and report error on failure
***************************************************************************/
char *
ftdrealloc(char *omptr, int len) 
{

	if ((omptr = (char*)realloc(omptr, len)) == NULL) {
		reporterr(ERRFAC, M_MALLOC, ERRCRIT, len);
		EXIT(EXITANDDIE);
	}
	return omptr;
} /* ftdrealloc */

/****************************************************************************
 * ftdfree -- call free
***************************************************************************/
void
ftdfree(char *ptr) 
{
    if (ptr != NULL) {
        free(ptr);
    }
    return;
}

/****************************************************************************
 * write_signal -- write the signal to the signal pipe 
 ***************************************************************************/
void
write_signal(int fd, int sig) 
{
	int save = errno;

	if (write(fd, &sig, sizeof(sig)) != sizeof(sig)) {
		reporterr(ERRFAC, M_SIGERR, ERRWARN, strerror(errno));
/*
*/
	}
	errno = save;
	return;
} /* write_signal*/

/****************************************************************************
 * update_lrdb -- update the LRDBs in the driver for the group
 ***************************************************************************/
int
update_lrdb(int lgnum)
{
    ftd_dev_t_t pass_lgnum = lgnum;

    /* update driver LRDBs for group */
    if (FTD_IOCTL_CALL(mysys->ctlfd, FTD_UPDATE_LRDBS, &pass_lgnum))
        return -1;

    return 0;
} /* update_lrdb */

/****************************************************************************
 * opendevs -- open each device in a group 
 ***************************************************************************/
int
opendevs(int lgnum)
{
    group_t *group;
    sddisk_t *sd;
    char devname[MAXPATH];
    int chainmask;

	group = mysys->group;

    /* open the system control device */
    if (ISPRIMARY(mysys)) {
        if ((mysys->ctlfd = open(FTD_CTLDEV, O_RDWR)) == -1) {
            reporterr(ERRFAC, M_OPEN, ERRCRIT, FTD_CTLDEV, strerror(errno));
            return -1;
        }
        /* open the logical group device */
        FTD_CREATE_GROUP_NAME(group->devname, lgnum);
        if ((group->devfd = open(group->devname, O_RDWR)) == -1) {
            reporterr(ERRFAC, M_LGDOPEN, ERRCRIT, group->devname, strerror(errno));
            return -1;
        }
        /* get major/minor dev numbers */
        for (sd = group->headsddisk; sd; sd = sd->n) {
            force_dsk_or_rdsk(devname, sd->devname, 0); 
            if (stat(devname, statbuf) == -1) {
                reporterr(ERRFAC, M_STAT, ERRWARN, devname, strerror(errno));
                return -1;
            }
            sd->dd_rdev = statbuf->st_rdev;
            force_dsk_or_rdsk(devname, sd->sddevname, 1); 
            if (stat(devname, statbuf) == -1) {
                reporterr(ERRFAC, M_STAT, ERRWARN, devname, strerror(errno));
                return -1;
            }
            sd->sd_rdev = statbuf->st_rdev;
        }
    } else {
        const int syncmask = O_SYNC;
        /* get major/minor dev numbers */
        for (sd = group->headsddisk; sd; sd = sd->n) {
            if (stat(sd->mirname, statbuf) == -1) {
                reporterr(ERRFAC, M_STAT, ERRWARN, sd->mirname, strerror(errno));
                return -1;
            }
            if (group->chaining == 1) {
                chainmask = 0;
            } else {
                chainmask = O_EXCL;
            }
            if ((sd->mirfd = open(sd->mirname, O_RDWR | syncmask | O_LARGEFILE | chainmask)) == -1) {
                reporterr(ERRFAC, M_MIROPEN, ERRCRIT, sd->mirname, argv0, strerror(errno));
                return -1;
            }
        }
    }
    return 0;
} /* opendevs */

/****************************************************************************
 * closedevs -- close each device in a group 
 ***************************************************************************/
void
closedevs(int modebits)
{
    group_t *group;
    sddisk_t *sd;

    group = mysys->group;

    if (ISPRIMARY(mysys)) {
        close(mysys->ctlfd);
        close(group->devfd);
    } else {
        for (sd = group->headsddisk; sd; sd = sd->n) {
            if (sd->mirfd >= 0) {
                close(sd->mirfd);
                sd->mirfd = -1;
            }
            if (modebits != -1) {
                if ((sd->mirfd = open(sd->mirname, O_LARGEFILE | modebits)) == -1) {
                    reporterr(ERRFAC, M_MIROPEN, ERRCRIT, sd->mirname, argv0, strerror(errno));
                    EXIT(EXITANDDIE);
                }
                /* SECONDARY - for refresh */ 
                sd->rsync.devfd = sd->mirfd;
            }
        }
    }
    return;
} /* closedevs */

/****************************************************************************
 * cfgpathtoname -- prune group name from a configpath 
 ***************************************************************************/
char *
cfgpathtoname(char *configpath)
{
    static char retstr[16];
    int len = strlen(configpath) - strlen(".cfg");

    strncpy(retstr, &configpath[len - 3], 3);
    retstr[3] = '\0';
    return retstr;    /* return remaining filename */
} /* cfgpathtoname */

/****************************************************************************
 * cfgpathtonum -- convert a configpath to logical group number 
 ***************************************************************************/
int
cfgpathtonum(char *configpath)
{
    int lgnum;
    char* p;
    int i;

    lgnum = 0;
    p = strrchr(configpath, '/');
    if (!p)
        p = configpath;
    else
        p++;
    for (i=1; i<4; i++) {
        if (isdigit(p[i])) {
            lgnum *= 10;
            lgnum += (int) (p[i] - '0');
        }
    }
    return (lgnum);
} /* cfgpathtonum */

/****************************************************************************
 * cfgtonum -- convert a config index to logical group number
 ***************************************************************************/
int
cfgtonum(int cfgidx)
{
    return (cfgpathtonum(paths+(cfgidx*32)));
} /* cfgtonum */

/****************************************************************************
 * numtocfg -- convert a logical group number to its config index
 ***************************************************************************/
int
numtocfg(int lgnum)
{
    int num;
    int i;
    
    i = 0;
    while (strlen(paths+(i*32))) {
        num = cfgtonum(i);
        if (num == lgnum) {
            return i;
        }
        i++;
    }
    return -1;
} /* numtocfg */

/****************************************************************************
 * getprocessid -- gets process id given name 
 ****************************************************************************/
pid_t
getprocessid(char *name, int exactflag, int *pcnt)
{
	static proc_info_t ***p = NULL;
	pid_t pid;

	pid = 0;
	if (!p && (p = (proc_info_t***)malloc(sizeof(proc_info_t**))) == NULL) {
        reporterr(ERRFAC, M_NOMEM, ERRWARN, name);
		return 0;
	}
	*p = NULL;
	if ((*pcnt = get_proc_names(name, exactflag, p)) > 0) {
		pid = (*p)[0]->pid;
	}
	del_proc_names(p, *pcnt);
	return pid;
} /* getprocessid */

/****************************************************************************
 * ftd_getidlist -- gets process id list 
 ****************************************************************************/
int ftd_getidlist(pid_t *pidlist)
{  
    return ftd_get_proc_id(pidlist);
}

/****************************************************************************
 * get_started_groups_list -- returns a list of started logical group numbers
 ****************************************************************************/
int
get_started_groups_list (int* list, int* size)
{
  /* 
     NOTE:  size should be set to the size of the list array coming into
     this function.  it will be set to the number of logical
     groups started (and whose numbers are now in list) on return.
     A caller should size the list argument to MAXLG to be safe. 
  */ 

  ftd_lg_map_t lgmap;
  ftd_uchar_t lgs[MAXLG];
  int err;
  int i;
  int count;
  int masterfd = -1;

  /* initialize stuff */
  count = 0;
  lgmap.lg_map = (ftd_uint64ptr_t)(unsigned long)lgs;
  lgmap.count = 0;
  lgmap.lg_max = 0;
  for (i=0; i<*size; i++)
    list[i] = -1;
  for (i=0; i<MAXLG; i++)
    lgs[i] = FTD_STOP_GROUP_FLAG;
  /* obtain the master control device file descriptor, if necessary */
  if (masterfd == -1) {
    if ((masterfd = open(FTD_CTLDEV, O_RDWR)) < 0) {
      *size=0;
      return (0);
    }
  }
  /* fetch the started group map from the driver */
  if ((err = ioctl(masterfd, FTD_GET_GROUP_STARTED, &lgmap)) < 0) {
    reporterr(ERRFAC, M_IOCTLERR, ERRCRIT, "FTD_GET_GROUP_STARTED", 
	      strerror(errno));
    close(masterfd);
    return(-1);
  }
  close(masterfd);
  /* populate the list with started logical group numbers */
  for (i=0; i<MAXLG; i++) {
    if (lgs[i] == FTD_STARTED_GROUP_FLAG)
      if (count < *size)
	list[count++] = i;
  }
  *size = count;
  return(0);
}

/****************************************************************************
 * get_driver_mode -- returns the group mode from the driver
 ****************************************************************************/
int
get_driver_mode(int lgnum)
{
    ftd_state_t lgstate;
    int ctlfd;

    if ((ctlfd = open(FTD_CTLDEV, O_RDWR)) == -1) {
        reporterr(ERRFAC, M_OPEN, ERRCRIT, FTD_CTLDEV, strerror(errno));
        return -1;
    }
    /* get current driver mode */
    lgstate.lg_num = lgnum;
    if (FTD_IOCTL_CALL(ctlfd, FTD_GET_GROUP_STATE, &lgstate))
		{
        reporterr(ERRFAC, M_DRVERR, ERRWARN, "GET_GROUP_STATE", strerror(errno));
        close(ctlfd);
        return -1;
    	}
    close(ctlfd);
    return lgstate.state;
} /* get_driver_mode */

/****************************************************************************
 * get_pstore_mode -- returns the group mode from the pstore 
 ****************************************************************************/
int
get_pstore_mode(int lgnum)
{
    char modestr[32];
    char ps_name[MAXPATHLEN];
    char group_name[MAXPATHLEN];
    int mode = 0;
    int rc;

    FTD_CREATE_GROUP_NAME(group_name, lgnum);
    if (GETPSNAME(lgnum, ps_name) == -1) {
        sprintf(group_name, "%s/%s%03d.cfg",
            PATH_CONFIG, PMD_CFG_PREFIX, lgnum);
        reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, group_name);
        return -1;
    }
    
    rc = ps_get_group_key_value(ps_name, group_name, "_MODE:", modestr);

    if (rc != PS_OK) {
        return -1;
    }

    if (!strcmp(modestr, "NORMAL")) {
        mode = FTDPMD;
    } else if (!strcmp(modestr, "BACKFRESH")) {
        mode = FTDBFD;
    } else if (!strcmp(modestr, "REFRESH")) {
        mode = FTDRFD;
    } else if (!strcmp(modestr, "FULL_REFRESH")) {
        mode = FTDRFDF;
    } else if (!strcmp(modestr, "CHECKSUM_REFRESH")) {
        mode = FTDRFDC;
    } else if (!strlen(modestr)) {
        mode = FTDPMD;
    }  

    ftd_trace_flow(FTD_DBG_FLOW12, "lg(%d) pstore mode(%d) %s\n", 
            lgnum, mode, modestr);

    return mode;
} /* get_pstore_mode */

/****************************************************************************
 * clearmode -- clear group state in the pstore/driver
 ****************************************************************************/
int
clearmode(int lgnum, int reset_RPO_stats)
{
    int ctlfd = -1;
    ftd_state_t lgstate;
    char *modestr = "ACCUMULATE";
    char ps_name[MAXPATHLEN];
    char group_name[MAXPATHLEN];
    int target_drv_state = 0;
    int save_drv_state;
    int save_pstore_state;
    int chg_drv_state;
    int chg_pstore_state;
    int rc;
    int count=0;
    ftd_dev_t_t group=lgnum;
    group_t *group_ptr;

    // WI_338550 December 2017, implementing RPO / RTT
    if( reset_RPO_stats )
    {
        group_ptr = mysys->group;

        // Clear RPO queue
        if (group_ptr->pChunkTimeQueue != NULL)
            Chunk_Timestamp_Queue_Destroy(group_ptr->pChunkTimeQueue);
        Chunk_Timestamp_Queue_Clear(group_ptr->pChunkTimeQueue);
        
        // WI_338550 December 2017, implementing RPO / RTT
        // Invalidate RTT sequencer tag (this may not be necessary but we add it
        // in case RTT data would be given to the driver at some point)
        SequencerTag_Invalidate(&group_ptr->RTTComputingControl.iSequencerTag);
        
        // Reset RPO stats
        // Feb8: Set this one to -1 to tell the driver to keep its own value; the driver may have set it.
        // Feb9: there seems to be a glitch in there and after killpmd the OldestInconsistentIOTimestamp becomes garbage; for
        // the moment all calls to clearmode() do not activate the RPO reset mode (turned off).
        // Feb 10: set a special value that the driver will recognize and tell it not to change OldestInconsistentIOTimestamp.
        group_ptr->QualityOfService.OldestInconsistentIOTimestamp = DO_NOT_CHANGE_OLDEST_INCONSISTENT_IO_TIMESTAMP;
        group_ptr->QualityOfService.RTT = 0;
        give_RPO_stats_to_driver( group_ptr );
    }
    
    FTD_CREATE_GROUP_NAME(group_name, lgnum);
    chg_drv_state = 1;
    chg_pstore_state = 1;

    save_drv_state = get_driver_mode(lgnum);

    /* get current pstore mode */
    save_pstore_state = get_pstore_mode(lgnum);

    switch(save_pstore_state) {
    case FTDPMD:
	/* going to tracking, if killpmds is called --- START */ 
        target_drv_state = FTD_MODE_TRACKING; 
        break;
	/* going to tracking, if killpmds is called --- END */
    case FTDBFD:
        modestr = "ACCUMULATE";
        chg_drv_state = 0;
        break;
    case FTDRFD:
    case FTDRFDC:
    case FTDRFDF:
        chg_pstore_state = 0;
        target_drv_state = FTD_MODE_TRACKING;
        break;
    default:
        chg_drv_state = 0;
        break;
    }
    if (chg_drv_state) {
        if ((ctlfd = open(FTD_CTLDEV, O_RDWR)) == -1) {
            reporterr(ERRFAC, M_OPEN, ERRCRIT, FTD_CTLDEV, strerror(errno));
            return -1;
        }
        /* set driver mode */
        lgstate.lg_num = lgnum;
        lgstate.state = target_drv_state;
        if (FTD_IOCTL_CALL(ctlfd, FTD_SET_GROUP_STATE, &lgstate))
			 {
            if (errno != EINVAL) {
                reporterr(ERRFAC, M_DRVERR, ERRWARN, "SET_GROUP_STATE", 
                    strerror(errno));
                close(ctlfd);
                return -1;
            }
        }

	/* going to tracking, if killpmds is called --- START */ 
	FTD_IOCTL_CALL(ctlfd, FTD_UPDATE_HRDBS, &group);
        while ( (FTD_IOCTL_CALL(ctlfd, FTD_CLEAR_BAB, &group) == EAGAIN)
            &&  (count < 1)) {
                 sleep(1);
                 count++;
        }
	/* going to tracking, if killpmds is called --- START */ 

        close(ctlfd);
        if (GETPSNAME(lgnum, ps_name) != 0) {
            reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, group_name);
            return (-1);
        }
    ps_set_group_state(ps_name, group_name, target_drv_state);
   }
    /* set pstore mode */
    if (chg_pstore_state) {
        if (GETPSNAME(lgnum, ps_name) != 0) {
            reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, group_name);
            return (-1);
        }
        rc = ps_set_group_key_value(ps_name, group_name, "_MODE:", modestr);
        if (rc != PS_OK) {
            reporterr(ERRFAC, M_PSWTDATTR, ERRWARN, group_name, ps_name);
            close(ctlfd);
            return -1;
        }
    }
    return 0;
} /* clearmode */

/****************************************************************************
 * setmode -- set group mode in the pstore/driver
 ***************************************************************************/
int 
setmode(int lgnum, int mode, int doing_reboot_autostart) 
{
    group_t *group;
    sddisk_t *sd;
    ftd_state_t lgstate;
    char modestr[32];
    char devname[MAXPATH];
    char mountp[MAXPATH];
    int target_drv_state;
    int save_drv_state;
    int chg_drv_state;
    int rc = 0;
    char group_name[MAXPATHLEN];
    int pstore_mode;
	int fd;
	int openmode;

    group = mysys->group;
    FTD_CREATE_GROUP_NAME(group_name, lgnum);

    /* get current driver mode */
    lgstate.lg_num = lgnum;
    if (FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_GROUP_STATE, &lgstate))
    {
        reporterr(ERRFAC, M_DRVERR, ERRWARN, "GET_GROUP_STATE", strerror(errno));
        return -1;
    }
    save_drv_state = lgstate.state;
    chg_drv_state = 1;

    ftd_trace_flow(FTD_DBG_FLOW1 | FTD_DBG_FLOW12,
            "current lg(%d) PMD mode: %d, drv state: %d\n",
            lgnum, mode, lgstate.state);

    _resuming_checksum_refresh = 0;
    
    switch(mode) {
    case FTDPMD:
        if (save_drv_state == FTD_MODE_PASSTHRU) {
            reporterr(ERRFAC, M_PASSTHRU, ERRWARN, argv0);
            EXIT(EXITANDDIE);
        } else if (save_drv_state == FTD_MODE_TRACKING) {
            pstore_mode = get_pstore_mode(lgnum);
            if (pstore_mode == FTDRFDF) {
                _pmd_state = FTDRFDF;
            } else if (pstore_mode == FTDRFDC) {
                _pmd_state = FTDRFD;
                _pmd_verify_secondary = 1;
                _resuming_checksum_refresh = 1;
            } else if (pstore_mode == FTDBFD) {
                _pmd_state = FTDBFD;
            } else {
                _pmd_state = FTDRFD;
            }
            return 1;
        } else if (save_drv_state == FTD_MODE_REFRESH) {
            pstore_mode = get_pstore_mode(lgnum);
            if (pstore_mode == FTDRFDF) {
                _pmd_state = FTDRFDF;
            } else if (pstore_mode == FTDRFDC) {
                _pmd_state = FTDRFD;
                _pmd_verify_secondary = 1;
                _resuming_checksum_refresh = 1;
            } else if (pstore_mode == FTDBFD) {
                _pmd_state = FTDBFD;
            } else {
                _pmd_state = FTDRFD;
            }
            return 1;
        } else if (save_drv_state == FTD_MODE_BACKFRESH) {
             _pmd_state = FTDBFD;
              return 1;
        } else if (save_drv_state == FTD_MODE_CHECKPOINT_JLESS) {
            ftd_trace_flow(FTD_DBG_FLOW1,
                    "_pmd_cpon(%d), _pmd_cpstart(%d)\n",
                    _pmd_cpon, _pmd_cpstart);
            if (GET_LG_JLESS(mysys) && _pmd_cpon) {
                return 0;
            }
        }
        strcpy(modestr, "NORMAL");
        target_drv_state = FTD_MODE_NORMAL;
        break;
    case FTDBFD:
        if (_pmd_cpon) {
            reporterr(ERRFAC, M_MIROFFLINE, ERRWARN, argv0, "Backfresh");
            return -1;
        }
        /* backfresh - if the dev is mounted then we can't run */
		 // NOTE: dev_mounted return values
		 //         1 == device mounted and *mountp is the mount point
		 //         0 == device not mounted and no error
		 //         else: an error occurred 
        for (sd = group->headsddisk; sd; sd = sd->n)
        {
            force_dsk_or_rdsk(devname, sd->sddevname, 0); // Check dtc device name first
            if (dev_mounted(devname, mountp) == 1) {
                reporterr(ERRFAC, M_BFDDEV, ERRCRIT,
                    _pmd_configpath, devname, mountp);
                return -1;
            }
            force_dsk_or_rdsk(devname, sd->devname, 0);	 // Check real device name second
            if (dev_mounted(devname, mountp) == 1) {
                reporterr(ERRFAC, M_BFDDEV, ERRCRIT,
                    _pmd_configpath, devname, mountp);
                return -1;
            }
#if defined(linux)
            // WR PROD12927
            // On some releases of Linux (such as RHEL 6.4), there may be problems
			// identifying the mount point of a device, even though that device would be mounted.
			// Therefore, in addition to attempting mount verification, we double-check by
			// temporarily opening the device with O_EXCL attribute (Exclusive access).
            openmode = (O_RDWR | O_DIRECT | O_EXCL);
		    if ((fd = open(sd->devname, openmode)) == -1)
		    {
                reporterr(ERRFAC, M_BFDDEV_NOTEXCL, ERRCRIT, _pmd_configpath, sd->devname, strerror(errno));
                return -1;
		    }
			close( fd );
#endif
        }
        if (save_drv_state == FTD_MODE_REFRESH) {
            reporterr(ERRFAC, M_BADMODE, ERRWARN, lgnum, "REFRESH", "BACKFRESH");
            return -1;
        }
        strcpy(modestr, "BACKFRESH");
/*
        target_drv_state = FTD_MODE_PASSTHRU;
*/
        target_drv_state = FTD_MODE_BACKFRESH;
        break;
    case FTDRFD:
        if (save_drv_state == FTD_MODE_BACKFRESH) {
            reporterr(ERRFAC, M_BADMODE, ERRWARN, lgnum, "BACKFRESH", "REFRESH");
            return -1;
        } else if (save_drv_state == FTD_MODE_CHECKPOINT_JLESS) {
            if (GET_LG_JLESS(mysys) && _pmd_cpon) {
                return 0;
            }
        } else {
            if (save_drv_state == FTD_MODE_REFRESH
            || save_drv_state == FTD_MODE_NORMAL
            || save_drv_state == FTD_MODE_TRACKING) {
                ftd_trace_flow(FTD_DBG_FLOW1 | FTD_DBG_FLOW12,
                    "lg(%d) PMD mode: %d, drv state: %d chg_drv_state %d->0\n",
                    lgnum, mode, lgstate.state, chg_drv_state);
                chg_drv_state = 0;

                if (get_pstore_mode(lgnum) == FTDRFDC)
                {
                    _pmd_verify_secondary = 1;
                    _resuming_checksum_refresh = 1;
                }
                
            }
        }
        if (get_pstore_mode(lgnum) == FTDRFDF) {
/*
 *           mode = FTDRFDF;
 */
        /*
         * We need to differentiate here betweeen a full refresh phase 2
         * during which we need to start smart refresh and the case where
         * smart refresh was accidently kicked off after a previous
         * full refresh operation was left incomplete.
         */
             if (_pmd_refresh)
             {
               strcpy(modestr, "REFRESH");
             }
             else
             {
                if ( _pmd_verify_secondary == 0 )    /* WR PROD4677: always allow Checksum Refresh */
                {
                    /* WR PROD6443: IF WE ARE NOT IN A REBOOT AUTOSTART MODE,
                       do not automatically relaunch a Full Refresh at 0% if a previous 
                       Full Refresh did not complete. Log a message indicating to the user what the options are
                       in order to recover from the interruption, and then exit.
                       Options for the user:
                       1) if the Full Refresh interruption was caused by an unclean shutdown (crash, power failure, etc) on the RMD,
                          the user can launch a Full Refresh (not Restartable Full (-r), but straight Full Refresh (-f) at 0%)
                          or launch a Checksum Refresh (launchrefresh -c)
                          NOTE: this situation (1) is handled elsewhere in the code, in handshake operations between PMD and RMD;
                       2) if the Full Refresh interruption was caused by a clean exit of the RMD or a network loss,
                          the user can launch a Restarted Full Refresh (launchrefresh -r, resuming at offset where it left)
                          or launch a Checksum Refresh (-c); case 2 is handled here, but again not in an automated fashion; 
                          we just log a message telling the user what his options are.
                    */
					if( doing_reboot_autostart )
					{
                        _pmd_state = FTDRFDF;
                        reporterr(ERRFAC,M_RFDFRERUN,ERRWARN);
                        strcpy(modestr, "FULL_REFRESH");
					}
					else
					{
                        reporterr(ERRFAC, M_CLEAN_FR_RESTRT, ERRWARN);
                        return(-1);
					}
                }
             }
        } else {
            strcpy(modestr, "REFRESH");
        }
        if (_pmd_verify_secondary)
        {
            strcpy(modestr, "CHECKSUM_REFRESH");
        }
        target_drv_state = FTD_MODE_REFRESH;
        break;
    case FTDRFDF:
        if (save_drv_state == FTD_MODE_BACKFRESH) {
            reporterr(ERRFAC, M_BADMODE, ERRWARN, lgnum, "BACKFRESH", "REFRESH");
            return -1;
        } else if (save_drv_state == FTD_MODE_CHECKPOINT_JLESS) {
            if (GET_LG_JLESS(mysys) && _pmd_cpon) {
                return 0;
            }
        }
        strcpy(modestr, "FULL_REFRESH");
        target_drv_state = FTD_MODE_REFRESH; /* ???
                                              * Driver state does set to
                                              * FTD_MODE_FULLREFRESH later
                                              * but why not here?
                                              */
        break;
    default:
        return -1;
    }
    ftd_trace_flow(FTD_DBG_FLOW1,
            "target_drv_state = %d, modestr = %s, rc = %d\n",
            target_drv_state, modestr, rc);

    if (chg_drv_state) {
        /* get current driver mode - in case something fails */
        lgstate.lg_num = lgnum;
        if (FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_GROUP_STATE, &lgstate)) {
            reporterr(ERRFAC, M_DRVERR, ERRWARN, "GET_GROUP_STATE", 
            strerror(errno));
            return -1;
        }
        save_drv_state = lgstate.state;
        /* set driver mode */
        lgstate.lg_num = lgnum;
        lgstate.state = target_drv_state;

        ftd_trace_flow(FTD_DBG_FLOW1, "change lg(%d) drv state from %d to %d\n",
            lgnum, save_drv_state, target_drv_state);

        if (FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_GROUP_STATE, &lgstate)) {
            if (errno != EINVAL) {
                reporterr(ERRFAC, M_DRVERR, ERRWARN, "SET_GROUP_STATE", 
                strerror(errno));
                return -1;
            }
        }
        ps_set_group_state(mysys->pstore, group_name, target_drv_state);
        if (target_drv_state == FTD_MODE_BACKFRESH) {
            rc = flush_bab(mysys->ctlfd, lgnum);
        }
    }
    /* set pstore mode */
    rc = ps_set_group_key_value(mysys->pstore, group->devname, "_MODE:", modestr);
    if (rc != PS_OK) {
        reporterr(ERRFAC, M_PSWTDATTR, ERRWARN, group->devname, mysys->pstore);
    }

    return 0;
} /* setmode */

/****************************************************************************
  setmaxfiles -
  Set max number of open file descriptors
 ****************************************************************************/
void
setmaxfiles(void)
{
 struct rlimit limit;

 limit.rlim_cur=(rlim_t) MAXFILES;
 limit.rlim_max=(rlim_t) MAXFILES;
 setrlimit(RLIMIT_NOFILE, &limit);
}

/****************************************************************************
 * dev_mounted -- is device mounted ?
 * return: 1 == device mounted and *mountp is the mount point
 *         0 == device not mounted and no error
 *         else: an error occurred 
 ***************************************************************************/
int
dev_mounted(char *devname, char *mountp)
{
#if defined(linux)
#define DTC_MNTTAB "/etc/mtab"
    char *dm_ptr = NULL;
	char dm_minor_string[16];
	char dmsetup_cmd[MAXPATHLEN];
	char mapper_device_name[MAXPATHLEN];
	FILE *f;
	int  j;
#else
#define DTC_MNTTAB "/etc/mnttab"
#endif
    struct mnttab mp;
    struct mnttab mpref;
    char bdevname[MAXPATHLEN];
    FILE *fp = (FILE *)NULL;
    int rc = 0;
#if defined(SOLARIS)
    int  tmpfd;
#endif  /* defined(SOLARIS) */

#if defined(SOLARIS)
    tmpfd = fcntl(dummyfd, F_DUPFD, 0);
    close(dummyfd);
#endif /* defined(SOLARIS) */
#if !defined(_AIX)
    fp = fopen (DTC_MNTTAB, "r");
    if (fp == (FILE*)NULL)
        return rc;
    rewind (fp);
#endif /* !defined(_AIX) */
    force_dsk_or_rdsk (bdevname, devname, 0);
#if defined(linux)
    /* 
        force_dsk_or_rdsk (bdevname, devname, 0); just copy devname into bdevname
        dev_mounted() cannot detect mounted disk when devname is a symbolic link
        instead of the block device itself in /etc/mtab
    */
    {
        struct stat sb;
        int rc = 0;
   
        memset(bdevname, 0, MAXPATHLEN);
   
        if ((rc = lstat(devname, &sb)) != -1) {
            rc = readlink(devname, bdevname, sb.st_size + 1);
        }

        if (rc == -1) {
    	    force_dsk_or_rdsk (bdevname, devname, 0);
	    }
    }
#endif

    mpref.mnt_special = bdevname;
    mpref.mnt_mountp = (char*)NULL;
    mpref.mnt_fstype = (char*)NULL;
    mpref.mnt_mntopts = (char*)NULL;
    mpref.mnt_time = (char*)NULL;
    if (0 == getmntany (fp, &mp, &mpref)) {
        if ((char*)NULL != mp.mnt_mountp && 0 < strlen (mp.mnt_mountp)) {
            strcpy (mountp, mp.mnt_mountp);
            rc = 1;
        }
    }
#if defined(linux)
	// WR PROD12927
	// On Linux, it may happen that device multipathing will cause returning a "/dev/dm-" device name from readlink()
	// and this device multipath name not be the one registered in the mount table, but its related
	// /dev/mapper device is instead.
	// So here we double-check if that was the case.
	if( rc != 1 )
	{
	    if( (dm_ptr = strstr( bdevname, "/dm-" )) != NULL )
		{
		    // Found a possible	dm device link; extract its minor number substring
			strncpy( dm_minor_string, &(dm_ptr[4]), 16 );
			// Format the commnad to get the mapper link
			// NOTE: dmsetup returns the following type of ouput where the last digit is the minor number (example):
			// vg01-xx4        (253:3)
			// vg02-xx3        (253:2)
			// VolGroup00-LogVol01     (253:0)
			// VolGroup00-LogVol00     (253:1)
			sprintf( dmsetup_cmd, "/sbin/dmsetup ls | /bin/grep \":%s)\" | /bin/awk '{print $1}'", dm_minor_string );
			if( (f = popen(dmsetup_cmd, "r")) != NULL )
			{
			    strcpy( mapper_device_name, "/dev/mapper/" );
			    if( fgets( &(mapper_device_name[12]), MAXPATHLEN-12, f ) != NULL)
			    {
				    // Terminate the string with null character
					for( j = 0; j < MAXPATHLEN; j++ )
					{
					    if( mapper_device_name[j] == '\0' )
						    break;
					    if( mapper_device_name[j] == '\n' )
						{
						    mapper_device_name[j] = '\0';
						    break;
						}
					}
			        // Check for mapper device name in mount table
				    mpref.mnt_special = mapper_device_name;
				    mpref.mnt_mountp = (char*)NULL;
				    mpref.mnt_fstype = (char*)NULL;
				    mpref.mnt_mntopts = (char*)NULL;
				    mpref.mnt_time = (char*)NULL;
					rewind( fp );
				    if (0 == getmntany (fp, &mp, &mpref))
				    {
				        if ((char*)NULL != mp.mnt_mountp && 0 < strlen (mp.mnt_mountp))
				        {
				            strcpy (mountp, mp.mnt_mountp);
				            rc = 1;
				        }
				    }
			    }
			    pclose(f);
			}
		}
	}
#endif

#if !defined(_AIX)
    fclose (fp);
#endif /* !defined(_AIX) */
#if defined(SOLARIS)
    dup2(tmpfd, dummyfd);
    close(tmpfd);
#endif /* defined(SOLARIS) */
    return rc;
} /* dev_mounted */

/****************************************************************************
 * encodeauth -- encode an authorization handshake string
 ***************************************************************************/
void 
encodeauth(time_t ts, char* hostname, u_long hostid, u_long ip,
		 int* encodelen, char* encode)
{
  encodeunion key1, key2;
  int i, j;
  u_char k;
  u_char t;
  encodeunion *kp;

  key1.ul = ((u_long) ts) ^ ((u_long) hostid);
  key2.ul = ((u_long) ts) ^ ((u_long) ip);
  
  j = 0;
  i = 0;
  while (i < strlen(hostname)) {
    kp = ((i%8) > 3) ? &key1 : &key2;
    k = kp->uc[i%4];
    t = (u_char) (0x000000ff & ((u_long)k ^ (u_long) hostname[i++]));
    sprintf (&(encode[j]), "%02x", t);
    j += 2;
  }
  encode[j] = '\0';
  *encodelen = j;
}

/****************************************************************************
 * decodeauth -- decode an authorization handshake string
 ***************************************************************************/
void decodeauth (int encodelen, char* encodestr, time_t ts, u_long hostid,
		 char* hostname, size_t hostname_len, u_long ip)
{
  encodeunion key1, key2;
  int i;
  u_char k;
  u_char t;
  int j;
  int temp;
  encodeunion *kp;

  key1.ul = ((u_long) ts) ^ ((u_long) hostid);
  key2.ul = ((u_long) ts) ^ ((u_long) ip);

  i = 0;
  j = 0;
  while (j < encodelen && i < hostname_len - 1) {
    kp = ((i%8) > 3) ? &key1 : &key2;
    k = kp->uc[i%4];
    sscanf ((encodestr+j), "%2x", &temp);
    t = (unsigned char) (0x000000ff & temp);
    hostname[i++] = (char)  (0x0000007f & ((u_long)k ^ (u_long)t));
    j += 2;
  }
  hostname[i] = '\0';
}


#if defined(SOLARIS)  /* 1LG-ManyDEV */
#include "license.h"
#include "licprod.h"

int
ftd_feof(int fd)
{
    int  ret;
    char buf[1];

    if((ret = read(fd, buf, 1)) <= 0){
	return 1;
    }
    lseek(fd, -1, SEEK_CUR);

    return 0;
}

char *
ftd_fgets(char *line, int size, int fd)
{
    int  i = 0, ret;
    static int offset = 0;
    char *buf;
    
    if (size <= 0) {
        return (NULL);
    }
    
    buf = (char *)malloc( size );
    memset(line, 0, size);
    
    if((ret = read(fd, buf, (size-1))) <= 0){
        free(buf);
        return (NULL);
    }
    
    do{
        line[i] = buf[i];
	if( buf[i] == '\n'|| buf[i] == '\0' ){
	        break;
	}
	i++;
    }while( i < ret );

    if(i == ret){
        free(buf);
	return (line);
    }
    
    offset = ret - (i+1);
    lseek(fd, -offset, SEEK_CUR);
    free(buf);
    return (line);
}

/*-----------------------------------------------------------------------*/
int
compare_license_keys (char *lic1, char *lic2, int max_length)
{
	int j, n, match;

	match = 1;
	for ( j = 0, n = max_length-1; j < max_length; j++, n-- )
	{
		if( lic1[j] != (char)(lic2[n] + 1) )
		{
			match = 0;
			break;
		}
	}

	return match;

}




int
get_license_manydev (int pmdflag, char ***lickeyaddr)
{

    int fd;
    struct stat statbuf[1];
    char line[256];
    char* ptr;
    char* key;
    char **lickey;
    int keycnt;
    char fname[256];
    char keyword[32];
	char TDMFIP280_free_for_all_license[LENGTH_TDMFIP_280_FREEFORALL_LICKEY+1];
	int found_free_for_all_license;

	strcpy( TDMFIP280_free_for_all_license, TDMFIP_280_FREEFORALL_LICKEY );
	TDMFIP280_free_for_all_license[LENGTH_TDMFIP_280_FREEFORALL_LICKEY] = NULL;

    sprintf (fname, "%s/%s.lic", PATH_CONFIG, CAPQ);
    sprintf (keyword, "%s_LICENSE:", CAPQ);

    if ( -1 == (fd = open (fname, O_RDONLY))){
        return LICKEY_FILEOPENERR;
    } else {
        stat(fname, statbuf);
    }
    if (statbuf->st_size <= 0) {
        return -1;
    }
    keycnt = 0;
    while (!ftd_feof(fd) && NULL != ftd_fgets (line, 255, fd)) {
        if (0 < strlen(line)) {
            if ('#' != line[0] && 
                ' ' != line[0] && 
                '\t' != line[0] && 
                '\n' != line[0]) {
                if (0 == strncmp(line, keyword, 12)) {
                    ptr = &line[12];
                } else {
                    ptr = line;
                }
                while (ptr[0] != '\0' && ptr[0] != '\n' && 
                       (ptr[0] == ' ' || ptr[0] == '\t')) ptr++;
                key = (char*)malloc(256);
                if (!keycnt) {
                    lickey = (char**)malloc(sizeof(char*));
                } else {
                    lickey = (char**)realloc(lickey, (keycnt+1)*sizeof(char*));
                }
                lickey[keycnt] = key;
                strcpy (key, ptr);
                keycnt++;
            }
        }
        memset(line, 0, sizeof(line));
    }
    /* null-terminate the key list */
    lickey = (char**)realloc(lickey, (keycnt+1)*sizeof(char*));
    lickey[keycnt] = NULL;
    *lickeyaddr = lickey;
    close(fd);
	/* In TDMFIP 2.9.0, reject the free-for-all permanent license that was installed in release 2.8.0 */
	found_free_for_all_license = compare_license_keys( *lickey, TDMFIP280_free_for_all_license, LENGTH_TDMFIP_280_FREEFORALL_LICKEY );
	if( found_free_for_all_license )
    {
	    return LICKEY_NOT_ALLOWED;
    }
    return LICKEY_OK;
}
#endif

/*
 * return the value converted from the specified characters.
 */
int
ftd_strtol(char *str)
{
	char	*p;
	int	val;

	val = (int)strtol(str, &p, 10);
	if (errno == ERANGE || p == str || *p != '\0') {
		return (-1);
	}

	return (val);
}

void
ftd_time_stamp
(
    char    *file,
    char    *func,
    char    *fmt,
    ...
)
{
#ifdef TDMF_TRACE
    va_list args;
    char    msg[256];
    time_t  statts;
    struct  tm *tim;

    (void)time(&statts);
    tim = localtime(&statts);

    DPRINTF("%s", asctime(tim));
    DPRINTF("%d<-%d: ", getpid(), getppid());
    DPRINTF("%s/%s: ", file, func);

    bzero((void *)msg, sizeof(char)*256);
    va_start(args, fmt);
    vsprintf(msg, fmt, args);
    va_end(args);

    DPRINTF("%s", msg);
#endif /* TDMF_TRACE */

    return;
}

int
ftd_get_lg_group_stat(int lgnum, stat_buffer_t *statBuf, int display)
{
    int master;
    int err;

    bzero((void *)statBuf, sizeof(*statBuf));

    statBuf->lg_num = lgnum;
    statBuf->dev_num = 0;
    statBuf->len = sizeof(lgstat);
    statBuf->addr = (ftd_uint64ptr_t)(unsigned long)&lgstat;

	if (0) {
		fprintf(stderr, "get_lg_group_stat(%d, %p, %d):\n"
						"    statBuf{lg_num = %llx, dev_num= %llx, len= %d, addr= %llx\n",
						lgnum, statBuf, display,
						statBuf->lg_num, statBuf->dev_num,
						statBuf->len, statBuf->addr);
	}
    if ((master = open(FTD_CTLDEV, O_RDWR)) < 0) {
        reporterr(ERRFAC, M_DEVOPEN, ERRINFO);
        fprintf(stderr, "Error: Failed to open master device: %s. \n"
                "Has " QNM " driver been added?\n", 
                FTD_CTLDEV);
        return -1;
    }

    if ((err = FTD_IOCTL_CALL(master, FTD_GET_GROUP_STATS, statBuf)) < 0) {
        fprintf(stderr, "FTD_GET_GROUP_STATS ioctl: error = %d\n", errno);
        return err;
    }

    if (display == TRUE) {
        fprintf(stderr, "\nDriver logic group %d state: "
                "\nloadtimesecs = %d,\nloadtimesystics = %d,"
                "\nwlentries = %d, \nwlsectors = %llu, \nbab_free = %d,"
                "\nbab_used = %d, \nstate = %d, \nndevs = %d,"
                "\nsync_depth = %u, \nsync_timeout = %u, \niodelay = %u\n",
                lgnum, lgstat.loadtimesecs, lgstat.loadtimesystics,
                lgstat.wlentries, lgstat.wlsectors, lgstat.bab_free,
                lgstat.bab_used, lgstat.state, lgstat.ndevs, lgstat.sync_depth,
                lgstat.sync_timeout, lgstat.iodelay);
    }

    close(master);

    return 0;
}

int showtunables (tunable_t *tunables)
{
    ftd_trace_flow(FTD_DBG_FLOW1,
            "\nchunksize = %d, \nstatinterval = %d, \nmaxstatfilesize = %d, "
            "\ntracethrottle = %d, \nsyncmode = %d, \nsyncmodedepth = %d, "
            "\nsyncmodetimeout = %d, \ncompression = %d, \ntcpwindowsize = %d, "
            "\nnetmaxkbps = %d, \nchunkdelay = %d \nuse_journal = %d\n",
            tunables->chunksize, tunables->statinterval,
            tunables->maxstatfilesize, tunables->tracethrottle,
            tunables->syncmode, tunables->syncmodedepth,
            tunables->syncmodetimeout, tunables->compression,
            tunables->tcpwindowsize, tunables->netmaxkbps, tunables->chunkdelay,
            tunables->use_journal);

    return 0;
}

int ftd_dump_drv_info (int lgnum)
{
    tunable_t   tunables;

    bzero((void *)&tunables, sizeof(tunables));

    /* get current driver tunables */
    if (getdrvrtunables(lgnum, &tunables) == -1) {
        ftd_trace_flow(FTD_DBG_FLOW1,
                "Driver is not ready, lgnum: %d\n", lgnum);
    } else {
        ftd_trace_flow(FTD_DBG_FLOW1,
                "Driver logic group %d tunables: \n", lgnum);
        showtunables(&tunables);
    }
    ftd_get_lg_group_stat(lgnum, &statBuf, (ftd_debugFlag & FTD_DBG_FLOW1));

    return 0;
}

//-----------------------------------------------------
// WI_338550 December 2017, implementing RPO / RTT
int give_RPO_stats_to_driver( group_t *group )
{
    stat_buffer_t sb;
    ftd_stat_t lgstat;
    int lgnum;
    int rc;

    lgnum = cfgpathtonum(mysys->configpath);

    /* get group info */
    memset(&sb, 0, sizeof(stat_buffer_t));
    sb.lg_num = lgnum;
    sb.len = sizeof(ftd_stat_t);
    
    lgstat.OldestInconsistentIOTimestamp = group->QualityOfService.OldestInconsistentIOTimestamp;
    lgstat.LastConsistentIOTimestamp = group->QualityOfService.LastConsistentIOTimestamp;
    lgstat.RTT = group->QualityOfService.RTT;
    lgstat.previous_non_zero_RTT = group->QualityOfService.previous_non_zero_RTT;
    lgstat.WriteOrderingMaintained = group->QualityOfService.WriteOrderingMaintained;
    lgstat.average_of_most_recent_RTTs = group->QualityOfService.average_of_most_recent_RTTs;
    lgstat.network_chunk_size_in_bytes = mysys->tunables.chunksize;
    // Note: LastIOTimestamp is not given to the driver; it is controlled by the driver

#ifdef  ENABLE_RPO_DEBUGGING
    sprintf( debug_msg, ">>> TRACING in give_RPO_stats_to_driver, lgstat.OldestInconsistentIOTimestamp %lu\n", lgstat.OldestInconsistentIOTimestamp);
    reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg);
    sprintf( debug_msg, ">>> TRACING in give_RPO_stats_to_driver, lgstat.LastConsistentIOTimestamp %lu\n", lgstat.LastConsistentIOTimestamp);
    reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg);
    sprintf( debug_msg, ">>> TRACING in give_RPO_stats_to_driver, lgstat.RTT %lu\n", lgstat.RTT);
    reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg);
    sprintf( debug_msg, ">>> TRACING in give_RPO_stats_to_driver, lgstat.WriteOrderingMaintained %lu\n", lgstat.WriteOrderingMaintained);
    reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg);
    sprintf( debug_msg, ">>> TRACING in give_RPO_stats_to_driver, lgstat.network_chunk_size_in_bytes %lu\n", lgstat.network_chunk_size_in_bytes);
    reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg);
    sprintf( debug_msg, ">>> TRACING in give_RPO_stats_to_driver, lgstat.previous_non_zero_RTT %lu\n", lgstat.previous_non_zero_RTT);
    reporterr (ERRFAC, M_GENMSG, ERRWARN, debug_msg);
#endif

    sb.addr = (ftd_uint64ptr_t)(unsigned long)&lgstat;
    rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_GROUP_RPO_STATS, &sb);
    if (rc != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "FTD_SET_GROUP_RPO_STATS",
            strerror(errno));
        return -1;
    }
    return rc;
}

// WI_338550 December 2017, implementing RPO / RTT
/************************************************************************************//**

            Returns the timestamp of the oldest IO not applied on the target yet.

\param      lgp 

\return     

****************************************************************************************/
ftd_uint64_t ftd_lg_get_oldest_inconsistent_timestamp(group_t* lgp)
{
    return lgp->QualityOfService.OldestInconsistentIOTimestamp;
}

// WI_338550 December 2017, implementing RPO / RTT
/************************************************************************************//**

            Return the timestamp of the last IO successfully applied on the target.

\param      lgp 

\return     

****************************************************************************************/
ftd_uint64_t ftd_lg_get_last_consistency_point_timestamp(group_t* lgp)
{
    return lgp->QualityOfService.LastConsistentIOTimestamp;
}

// WI_338550 December 2017, implementing RPO / RTT
/************************************************************************************//**

            Indicate that this group has reached a consistency point.

\param      group 

****************************************************************************************/
void ftd_lg_set_consistency_point_reached(group_t *group)
{
    time_t now;
    // We can never reach a consistency point when write ordering is not maintained
    if (group->QualityOfService.WriteOrderingMaintained)
    {
        time(&now);
        group->QualityOfService.LastConsistentIOTimestamp = now;
        group->QualityOfService.OldestInconsistentIOTimestamp = 0;
    }
}

// WI_338550 December 2017, implementing RPO / RTT
/************************************************************************************//**

            Indicate that this group has lost the consistency.

\param      lgp 

****************************************************************************************/
void ftd_lg_set_consistency_point_lost(group_t* lgp)
{
    time_t now;

    // By setting the timestamp to 0, we indicate that there is no consistency point available
    lgp->QualityOfService.LastConsistentIOTimestamp = 0;
    // Also indicate the time at which the consistency was lost (only if not previously set)
    if( ( (lgp->QualityOfService.OldestInconsistentIOTimestamp == 0) || (lgp->QualityOfService.OldestInconsistentIOTimestamp == -1) ) &&
        lgp->QualityOfService.WriteOrderingMaintained)
        {
        time(&now);
        lgp->QualityOfService.OldestInconsistentIOTimestamp = now;
        }
    give_RPO_stats_to_driver( lgp );
}

// WI_338550 December 2017, implementing RPO / RTT
/************************************************************************************//**

            Indicate that this group has an invalid consistency point.

\param      lgp 

****************************************************************************************/
void ftd_lg_set_consistency_point_invalid(group_t* lgp)
{
    // By setting both timestamps to 0, we indicate that it is impossible to determine the consistency point
    lgp->QualityOfService.LastConsistentIOTimestamp = 0;
    lgp->QualityOfService.OldestInconsistentIOTimestamp = 0;
    give_RPO_stats_to_driver( lgp );
}

// WI_338550 December 2017, implementing RPO / RTT
/************************************************************************************//**

            This is called upon receiving the confirmation of a chunk's application on 
            the secondary.  It updates the internal consistent and inconsistent timestamps.

\param      group 

****************************************************************************************/
void ftd_lg_update_consistency_timestamps(group_t* group)
{
    ftd_uint32_t lgnum;
    ftd_uint64_t Timestamp;
    ftd_int32_t  lg_state;

    Timestamp = Chunk_Timestamp_Queue_Head(group->pChunkTimeQueue)->NewestChunkTimestamp;
    /* get current driver mode */
    lgnum = cfgpathtonum(mysys->configpath);
    lg_state = get_driver_mode(lgnum);
    if( lg_state != -1 )
    {
        if ((Timestamp != LG_CONSISTENCY_POINT_TIMESTAMP_INVALID) &&
            (lg_state == FTD_MODE_NORMAL) &&
            group->QualityOfService.WriteOrderingMaintained)
        {
            // Update the last consistent IO time
            group->QualityOfService.LastConsistentIOTimestamp = Timestamp;
        }
        // And remove it from the queue once it's acknowledged
        Chunk_Timestamp_Queue_Pop(group->pChunkTimeQueue);

        // Update the oldest inconsistent IO time with the new queue content
        if (lg_state == FTD_MODE_NORMAL)
        {
            if (Chunk_Timestamp_Queue_IsEmpty(group->pChunkTimeQueue))
            {
                // This indicates that there is no inconsistent data
                group->QualityOfService.OldestInconsistentIOTimestamp = 0;
                ftd_lg_set_consistency_point_reached(group);
            }
            else
            {
                Timestamp = Chunk_Timestamp_Queue_Head(group->pChunkTimeQueue)->OldestChunkTimestamp;
                if (Timestamp != LG_CONSISTENCY_POINT_TIMESTAMP_INVALID &&
                    group->QualityOfService.WriteOrderingMaintained)
                {
                    group->QualityOfService.OldestInconsistentIOTimestamp = Timestamp;
                }
            }
        }
        give_RPO_stats_to_driver( group );
    }
    else
    {
        reporterr (ERRFAC, M_GENMSG, ERRWARN, "ftd_lg_update_consistency_timestamps: error getting group mode\n");
    }
}
// WI_338550 December 2017, implementing RPO / RTT
queue_t* Chunk_Timestamp_Queue_Create()
{
    return (queue_t*)malloc(sizeof(queue_t));
}

void Chunk_Timestamp_Queue_Destroy(queue_t* pQueue)
{
    free(pQueue->pBuffer);
    free(pQueue);
}

void Chunk_Timestamp_Queue_Init(queue_t* pQueue, u_long Size)
{
    pQueue->pBuffer = (ftd_uint64_t*)calloc(Size, sizeof(chunk_timestamp_queue_entry_t));
    if (pQueue->pBuffer == NULL)
        {
        reporterr(ERRFAC, M_MALLOC, (Size * sizeof(chunk_timestamp_queue_entry_t)));
        }
    pQueue->Head = pQueue->Tail = 0;
    pQueue->Size = Size;
}

void Chunk_Timestamp_Queue_Push(queue_t* pQueue, chunk_timestamp_queue_entry_t* pData)
{
    static int logged_no_space_message = 0;
    
    chunk_timestamp_queue_entry_t* pEntry = (chunk_timestamp_queue_entry_t*)pQueue->pBuffer;
    pEntry[pQueue->Tail] = *pData;
    pQueue->Tail = (pQueue->Tail + 1) % pQueue->Size;
    ++RPO_timestamp_queue_num_of_entries;
    ++cumulative_RPO_timestamp_queue_entries_so_far;
    if( RPO_timestamp_queue_num_of_entries > Max_RPO_timestamp_queue_num_of_entries_recorded_so_far )
    {
        Max_RPO_timestamp_queue_num_of_entries_recorded_so_far = RPO_timestamp_queue_num_of_entries;
    }
    trace_RPO_timestamp_queue(1, pQueue->Head, pQueue->Tail, pQueue->Size);
    if ((pQueue->Tail == pQueue->Head) && !logged_no_space_message)
        {
        reporterr(ERRFAC, M_GENMSG, ERRWARN, "Not enough space in RPO timestamp queue");
        logged_no_space_message = 1;
        }
}

void Chunk_Timestamp_Queue_Pop(queue_t* pQueue)
{
    if( Chunk_Timestamp_Queue_IsEmpty(pQueue) )
    {
        // Check if the pop function is called on an empty queue; this may happen for instance if we get a BAB overflow,
        // in which case we clear the timestamp queue and we still can receive acknowledgements from the RMD for previously sent
        // chunks
        return;
    }
    pQueue->Head = (pQueue->Head + 1) % pQueue->Size;
    --RPO_timestamp_queue_num_of_entries;
    trace_RPO_timestamp_queue(1, pQueue->Head, pQueue->Tail, pQueue->Size);
}

chunk_timestamp_queue_entry_t* Chunk_Timestamp_Queue_Head(queue_t* pQueue)
{
    chunk_timestamp_queue_entry_t* pEntry = (chunk_timestamp_queue_entry_t*)pQueue->pBuffer;
    return &pEntry[pQueue->Head];
}

chunk_timestamp_queue_entry_t* Chunk_Timestamp_Queue_Tail(queue_t* pQueue)
{
    chunk_timestamp_queue_entry_t* pEntry = (chunk_timestamp_queue_entry_t*)pQueue->pBuffer;
    return &pEntry[pQueue->Tail];
}

void Chunk_Timestamp_Queue_Clear(queue_t* pQueue)
{
    pQueue->Head = pQueue->Tail = 0;
    RPO_timestamp_queue_num_of_entries = 0;
}

int Chunk_Timestamp_Queue_IsEmpty(queue_t* pQueue)
{
    if (pQueue->Head == pQueue->Tail)
    {
        return 1;
    }
    return 0;
}



