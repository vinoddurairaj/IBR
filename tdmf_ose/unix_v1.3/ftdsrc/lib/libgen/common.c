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
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>

#define  TDMF_TRACE_MAKER
#include "errors.h"
#include "config.h"
#include "pathnames.h"
#include "procinfo.h"
#include "ipc.h"
#include "ftdio.h"
#include "common.h"
#undef	  TDMF_TRACE_MAKER

#include "devcheck.h"
#ifdef NEED_SYS_MNTTAB
#include "ftd_mnttab.h"
#else
#include <sys/mnttab.h>
#endif

int lgcnt;

extern char *argv0;
extern char *paths;
extern int _pmd_state;
extern int _pmd_cpon;
extern int _pmd_configpath;
extern int _pmd_state_change;

extern ftdpid_t p_cp[];
extern ftdpid_t r_cp[];

int compress_buf_len = 0;
char *compress_buf = NULL;

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

#ifdef TDMF_TRACE
int ftd_ioctl_trace(char *mod, char *funct, int cmd)
{
	int i;
	if (cmd)
		{
		i = (cmd & 0xFF)-1;
		if (i<0 || i >= MAX_TDMF_IOCTL)
			{DPRINTF("%s/%s IOCTL [0x%08x]: ???",mod,funct,cmd);}				
		else
	 	    {DPRINTF("%s/%s() IOCTL [0x%08x]: %s ", mod,funct, cmd,tdmf_ioctl_text_array[i].cmd_txt);}		
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
    pid_t pid, ppid;
    int pcnt;
    int i;

    for (i = 0; i < 10; i++) {
        /* there is a race - ftdd forks but process not exec'd yet */
        pid = getprocessid(FTDD, 0, &pcnt);
        ppid = getppid();
        DPRINTF("\n@@@ is_parent_master: FTDD pid = %d\n",pid);
        DPRINTF("\n@@@ is_parent_master: parent pid = %d\n",ppid);
        if (pid > 0 && ppid == pid) {
           break;
        }
    }
    if (i == 10) {
        return 0;
    }
    return 1;
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
get_lg_rdev(group_t *group, int rdev)
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
 * ftdmalloc -- call malloc and report error on failure
***************************************************************************/
char *
ftdmalloc(int len) 
{
	char *mptr;

	if ((mptr = (char*)malloc(len)) == NULL) {
		reporterr(ERRFAC, M_MALLOC, ERRCRIT, len);
		exit(EXITANDDIE);
	}
	return mptr;
} /* ftdmalloc */

/****************************************************************************
 * ftdcalloc -- call calloc and report error on failure
***************************************************************************/
char *
ftdcalloc(int cnt, int len) 
{
	char *mptr;

	if ((mptr = (char*)calloc(cnt, len)) == NULL) {
		reporterr(ERRFAC, M_MALLOC, ERRCRIT, len);
		exit(EXITANDDIE);
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
		exit(EXITANDDIE);
	}
	return omptr;
} /* ftdrealloc */

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
    int rc;
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
#if defined(SOLARIS) || defined(_AIX)
            force_dsk_or_rdsk(devname, sd->devname, 0); 
#elif defined(HPUX)
            if (strncmp(sd->devname, "/dev/" QNM "/",9)==0) {
                force_dsk_or_rdsk(devname, sd->devname, 0);
            } else {
                convert_lv_name(devname, sd->devname, 0); 
            }
#endif
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
            if ((sd->mirfd = open(sd->mirname, O_RDWR | O_SYNC | chainmask)) == -1) {
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
                if ((sd->mirfd = open(sd->mirname, modebits)) == -1) {
                    reporterr(ERRFAC, M_MIROPEN, ERRCRIT, sd->mirname, argv0, strerror(errno));
                    exit(EXITANDDIE);
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
    static char *retstr = NULL;
    char lgname[128];
    int len;

    if (!retstr) {
        retstr = ftdcalloc(1, 32); 
    } else {
        memset(retstr, 0, 32);
    }
    len = strlen(configpath)-4;          /* lop off .cfg */
    memset(lgname, 0, sizeof(lgname));
    strncpy(lgname, configpath, len);
    strcpy(retstr, &lgname[strlen(lgname)-3]);
    return retstr;    /* return remaining filename */
} /* cfgpathtoname */

/****************************************************************************
 * cfgpathtonum -- convert a configpath to logical group number 
 ***************************************************************************/
int
cfgpathtonum(char *configpath)
{
    char lgname[128];
    int lgnum;
    int len;
    char* p;
    int i;

    lgnum = 0;
    p = configpath;
    for (i=(strlen(configpath)-1); i>=0; i--) {
        if (configpath[i] == '/') {
            p = &(configpath[i+1]);
            break;
        }
    }
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
	if (!p && (p = (proc_info_t***)malloc(sizeof(proc_info_t***))) == NULL) {
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
        reporterr(ERRFAC, M_CFGPS, ERRCRIT, group_name);
        return -1;
    }
    rc = ps_get_group_key_value(ps_name, group_name, "_MODE:", modestr);
    if (rc != PS_OK) {
        return -1;
    }
    DPRINTF("\n*** get_pstore_mode: modestr = %s\n",modestr);
    if (!strcmp(modestr, "NORMAL")) {
        mode = FTDPMD;
    } else if (!strcmp(modestr, "BACKFRESH")) {
        mode = FTDBFD;
    } else if (!strcmp(modestr, "REFRESH")) {
        mode = FTDRFD;
    } else if (!strcmp(modestr, "FULL_REFRESH")) {
        mode = FTDRFDF;
    } else if (!strlen(modestr)) {
        mode = FTDPMD;
    }  
    return mode;
} /* get_pstore_mode */

/****************************************************************************
 * clearmode -- clear group state in the pstore/driver
 ****************************************************************************/
int
clearmode(int lgnum)
{
    int ctlfd;
    ftd_state_t lgstate;
    char *modestr = "ACCUMULATE";
    char ps_name[MAXPATHLEN];
    char group_name[MAXPATHLEN];
    int target_drv_state;
    int save_drv_state;
    int save_pstore_state;
    int chg_drv_state;
    int chg_pstore_state;
    int rc;

    FTD_CREATE_GROUP_NAME(group_name, lgnum);
    chg_drv_state = 1;
    chg_pstore_state = 1;

    save_drv_state = get_driver_mode(lgnum);

    /* get current pstore mode */
    save_pstore_state = get_pstore_mode(lgnum);

    switch(save_pstore_state) {
    case FTDBFD:
    case FTDPMD:
        modestr = "ACCUMULATE";
        chg_drv_state = 0;
        break;
    case FTDRFD:
    case FTDRFDF:
        chg_pstore_state = 0;
        target_drv_state = FTD_MODE_REFRESH;
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
setmode(int lgnum, int mode) 
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
    int rc;
    char ps_name[MAXPATHLEN];
    char group_name[MAXPATHLEN];

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

    switch(mode) {
    case FTDPMD:
        if (save_drv_state == FTD_MODE_PASSTHRU) {
            reporterr(ERRFAC, M_PASSTHRU, ERRWARN, argv0);
            exit (EXITANDDIE);
        } else if (save_drv_state == FTD_MODE_TRACKING) {
            if (get_pstore_mode(lgnum) == FTDRFDF) {
                _pmd_state = FTDRFDF;
            } else {
                _pmd_state = FTDRFD;
            }
            return 1;
        } else if (save_drv_state == FTD_MODE_REFRESH) {
            if (get_pstore_mode(lgnum) == FTDRFDF) {
                _pmd_state = FTDRFDF;
            } else {
                _pmd_state = FTDRFD;
            }
            return 1;
        } else if (save_drv_state == FTD_MODE_BACKFRESH) {
             _pmd_state = FTDBFD;
              return 1;
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
        for (sd = group->headsddisk; sd; sd = sd->n) {
            force_dsk_or_rdsk(devname, sd->sddevname, 0);
            if (dev_mounted(devname, mountp)) {
                reporterr(ERRFAC, M_BFDDEV, ERRCRIT,
                    _pmd_configpath, devname, mountp);
                return -1;
            }
            force_dsk_or_rdsk(devname, sd->devname, 0);
            if (dev_mounted(devname, mountp)) {
                reporterr(ERRFAC, M_BFDDEV, ERRCRIT,
                    _pmd_configpath, devname, mountp);
                return -1;
            }
        }
        if (save_drv_state == FTD_MODE_REFRESH) {
            reporterr(ERRFAC, M_BADMODE, ERRWARN, "REFRESH", "BACKFRESH");
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
            reporterr(ERRFAC, M_BADMODE, ERRWARN, "BACKFRESH", "REFRESH");
            return -1;
        } else {
            if (save_drv_state == FTD_MODE_REFRESH
            || save_drv_state == FTD_MODE_NORMAL
            || save_drv_state == FTD_MODE_TRACKING) {
                chg_drv_state = 0;
            }
        }
        if (get_pstore_mode(lgnum) == FTDRFDF) {
            mode = FTDRFDF;
            strcpy(modestr, "FULL_REFRESH");
        } else {
            strcpy(modestr, "REFRESH");
        }
        target_drv_state = FTD_MODE_REFRESH;
        break;
    case FTDRFDF:
        if (save_drv_state == FTD_MODE_BACKFRESH) {
            reporterr(ERRFAC, M_BADMODE, ERRWARN, "BACKFRESH", "REFRESH");
            return -1;
        }
        strcpy(modestr, "FULL_REFRESH");
        target_drv_state = FTD_MODE_REFRESH;
        break;
    default:
        return -1;
    }
    DPRINTF("\n*** setmode: target_drv_state, modestr, rc = %d, %s, %d\n",target_drv_state, modestr, rc);

    if (chg_drv_state) {
        /* get current driver mode - in case something fails */
        lgstate.lg_num = lgnum;
        if (FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_GROUP_STATE, &lgstate))
			 {
            reporterr(ERRFAC, M_DRVERR, ERRWARN, "GET_GROUP_STATE", 
            strerror(errno));
            return -1;
        	}
        save_drv_state = lgstate.state;
        /* set driver mode */
        lgstate.lg_num = lgnum;
        lgstate.state = target_drv_state;
        if (FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_GROUP_STATE, &lgstate))
			 {
            if (errno != EINVAL) 
            	{
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
 ***************************************************************************/
int
dev_mounted(char *devname, char *mountp)
{
    struct mnttab mp;
    struct mnttab mpref;
    char bdevname[MAXPATHLEN];
    FILE *fp;
    int rc;
    int i;

#if !defined(_AIX)
    fp = fopen ("/etc/mnttab", "r");
#else /* !defined(_AIX) */
    fp = (FILE *)NULL;
#endif /* !defined(_AIX) */

    force_dsk_or_rdsk(bdevname, devname, 0); 
    if (fp != (FILE*)NULL) {
        rewind(fp);
        mpref.mnt_special = bdevname;
        mpref.mnt_mountp = (char*)NULL;
        mpref.mnt_fstype = (char*)NULL;
        mpref.mnt_mntopts = (char*)NULL;
        mpref.mnt_time = (char*)NULL;
        if (0 == getmntany (fp, &mp, &mpref)) {
            if ((char*)NULL != mp.mnt_mountp && 0 < strlen(mp.mnt_mountp)) {
#if !defined(_AIX)
                fclose(fp);
#endif /* !defined(_AIX) */
                strcpy(mountp, mp.mnt_mountp);
                return 1;
            }
        }
    }
#if !defined(_AIX)
    fclose(fp);
#endif /* !defined(_AIX) */
    return 0;
} /* dev_mounted */

/****************************************************************************
 * encodeauth -- encode an authorization handshake string
 ***************************************************************************/
void 
encodeauth (time_t ts, char* hostname, u_long hostid, u_long ip,
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


