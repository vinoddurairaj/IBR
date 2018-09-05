/*
 * ftd_start.c - Start one or more logical groups
 *
 * Copyright (c) 1998 The FullTime Software, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 * This module initializes a logical group. It reads the config file
 * for the group and then checks the persistent store for state information.
 * If the PS doesn't have any data for this group, we add the group 
 * and its devices to the PS and assume the group is new one. If a device
 * doesn't exist in the PS, we assume it is a new device and set the 
 * appropriate defaults for it. 
 *
 * After reading/setting up of the group/device info from the PS, we 
 * load the driver with the group and device information, including
 * state and dirty bit maps. The device files for the group and its
 * devices are also created at this time. The logical group is now
 * ready for action.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <signal.h>
#include <macros.h>

#if defined(SOLARIS)
#include <sys/mkdev.h>
#endif

#include "cfg_intr.h"
#include "ps_intr.h"

#include "ftdio.h"
#include "ftdif.h"
#include "pathnames.h"
#include "config.h"
#include "ftd_cmd.h"
#include "errors.h"
#include "platform.h"
#include "devcheck.h"
#include "aixcmn.h"

static char *progname = NULL;
static char configpaths[512][32];

static autostart;
static grp_started;
extern char *paths;

char *argv0;

static int start_group(int group, int force, int autostart);
static int conf_lg(char *ps_name, char *group_name, int group,
                   unsigned int hostid, int *state, int *regen_hrdb);
static int conf_dev(char *ps_name, char *devname, char *sddevname, 
                    char *group_name, int group, int regen_hrdb,
                    ftd_dev_info_t *devp);

static void
usage(void)
{
    fprintf(stderr, "Usage: %s [-a | -g group_number] [-b]\n", progname);
    fprintf(stderr, "OPTIONS: -a => all groups\n");
    fprintf(stderr, "         -g => group number\n");
    fprintf(stderr, "         -b => don't set automatic start state\n");
}

/* this assumes 32-bits per int */ 
#define WORD_BOUNDARY(x)  ((x) >> 5) 
#define SINGLE_BIT(x)     (1 << ((x) & 31))
#define TEST_BIT(ptr,bit) (SINGLE_BIT(bit) & *(ptr + WORD_BOUNDARY(bit)))
#define SET_BIT(ptr,bit)  (*(ptr + WORD_BOUNDARY(bit)) |= SINGLE_BIT(bit))

/* use natural integer bit ordering (bit 0 = LSB, bit 31 = MSB) */
#define START_MASK(x)     (((unsigned int)0xffffffff) << ((x) & 31))
#define END_MASK(x)       (((unsigned int)0xffffffff) >> (31 - ((x) & 31)))

#if defined(HPUX)

char ftdctlpath[] = "/sbin/" QNM "ctl";
char control_daemon_psname[MAXPATHLEN];

/*-
 * control_daemon_init()
 *	
 * start a group's control daemon process
 */
static int
control_daemon_init(group)
	int group;
{
	int psfd;
	pid_t pid;
	char **ctldargv;
	char cur_cfg_path[32];
	pid_t oldCtldPid = 0;
	char ctmp[8];
	int pcnt;


	/* hold ps dev open across fork() */
	sprintf(cur_cfg_path, "%s%03d.cur", PMD_CFG_PREFIX, group);
	if (getpsname(group, control_daemon_psname) != 0) {
		reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, cur_cfg_path);
		return (-1);
	}
	if ( (psfd = open(control_daemon_psname, O_RDONLY, 0666)) < 0 ) {
		reporterr(ERRFAC, M_PSOPEN, ERRCRIT, control_daemon_psname);
		return (-1);
	}

	/* check if CTL_xxx is already running, if it is, kill it */
	sprintf(ctmp, "CTL_%03d", group);
	oldCtldPid = getprocessid(ctmp, 0, &pcnt);

	if ((pid = fork()) < 0) {
		/* fork bad */
		return(-1);
	} else {
		/* fork good */
		if (pid != 0) {
			/* parent */
			close(psfd);
			if (oldCtldPid > 0)
				kill(oldCtldPid, SIGKILL);
			return(0);
		}
		else {
			/* child */
			ctldargv = (char **) ftdmalloc(sizeof(char *) * 2);
			ctldargv[0] = (char *)ftdmalloc(strlen(ftdctlpath) + 1);
			sprintf(ctldargv[0], "CTL_%03d%", group);
			ctldargv[1] = (char*)NULL;
			execv(ftdctlpath, ctldargv);
			/* NOTREACHED */
		}
	}
} 

#endif /* defined(HPUX) */

/*
 * A very simple function. Just write out a new header. Everything on the
 * disk is wasted by the create function.
 */
static int
create_ps(char *ps_name, int *max_dev)
{
    unsigned long long  size, err;
    char                raw_name[MAXPATHLEN];
    daddr_t             dsize;
    ps_version_1_attr_t attr;

    /* convert the block device name to the raw device name */
#if defined(HPUX)
    if (is_logical_volume(ps_name)) {
        convert_lv_name(raw_name, ps_name, 1);
/* FIXME: for logical volumes, should we make sure it has contiguous blocks? */
    } else {
        force_dsk_or_rdsk(raw_name, ps_name, 1);
    }
#else
    force_dsk_or_rdsk(raw_name, ps_name, 1);
#endif

    /* stat the pstore device and figure out the maximum number of devices */
    if ((dsize = disksize(raw_name)) < 0) {
        reporterr(ERRFAC, M_STAT, ERRCRIT, raw_name, strerror(errno));
        return -1;
    }

/* FIXME: the 3*MAXPATHLEN fudge is not kosher, but it works. The fudge
 * takes into account the table entry sizes for devices and groups, but
 * the table entry sizes may change in the future. Fix this by asking 
 * the pstore interface how much space each device uses and how much space
 * each group uses.
 */
    /* the size of one device assuming one device per group */
    size = FTD_PS_LRDB_SIZE + FTD_PS_HRDB_SIZE + FTD_PS_DEVICE_ATTR_SIZE +
           FTD_PS_GROUP_ATTR_SIZE + 3*MAXPATHLEN;

    size = size / DEV_BSIZE;
/* BOGUSITY: assume the pstore starts at 16K from the start of the device */
    dsize = (daddr_t)((dsize - 32) / size);
    if (dsize > FTD_MAX_DEVICES) {
        dsize = FTD_MAX_DEVICES;
    }

    *max_dev = dsize;

    /* create a new pstore with the defaults */
    attr.max_dev = dsize;
    attr.max_group = dsize;
    attr.lrdb_size = FTD_PS_LRDB_SIZE;
    attr.hrdb_size = FTD_PS_HRDB_SIZE;
    attr.group_attr_size = FTD_PS_GROUP_ATTR_SIZE;
    attr.dev_attr_size = FTD_PS_DEVICE_ATTR_SIZE;

    if ((err = ps_create_version_1(ps_name, &attr)) != PS_OK) {
        reporterr(ERRFAC, M_PSCREATE, ERRCRIT, ps_name, err);
        return -1;
    }

    return 0;
}

static void
set_bits(unsigned int *ptr, int x1, int x2)
{
    unsigned int *dest;
    unsigned int mask, word_left, n_words;

    word_left = WORD_BOUNDARY(x1);
    dest = ptr + word_left;
    mask = START_MASK(x1);
    if ((n_words = (WORD_BOUNDARY(x2) - word_left)) == 0) {
        mask &= END_MASK(x2);
        *dest |= mask;
        return;
    }
    *dest++ |= mask;
    while (--n_words > 0) {
        *dest++ = 0xffffffff;
    }
    *dest |= END_MASK(x2);
    return;
}

static int
get_lrdb(char *ps_name, char *dev_name, unsigned int size,
    int **lrdb, unsigned int *lrdb_bits)
{
    int  ret;
    int *buffer;

    /* allocate a buffer */
    if ((buffer = (int *)malloc(size)) == NULL) {
        return PS_MALLOC_ERROR;
    }
    ret = ps_get_lrdb(ps_name, dev_name, (caddr_t)buffer, size, lrdb_bits);
    if (ret != PS_OK) {
        free(buffer);
        return ret;
    }
    *lrdb = buffer;
    
    return PS_OK;
}

static int
get_hrdb(char *ps_name, char *dev_name, unsigned int size,
    int **hrdb, unsigned int *hrdb_bits)
{
    int  ret;
    int *buffer;

    /* allocate a buffer */
    if ((buffer = (int *)malloc(size)) == NULL) {
        return PS_MALLOC_ERROR;
    }
    ret = ps_get_hrdb(ps_name, dev_name, (caddr_t)buffer, size, hrdb_bits);
    if (ret != PS_OK) {
        free(buffer);
        return ret;
    }
    *hrdb = buffer;
    
    return PS_OK;
}

static int
start_group(int group, int force, int autostart)
{
    int ret, ctlfd, i, state, regen;
    int n;  
    char copybuf[BUFSIZ];
    char cfg_path[32];
    char cur_cfg_path[32];
    char full_cfg_path[MAXPATHLEN];
    char full_cur_cfg_path[MAXPATHLEN];
    char group_name[MAXPATHLEN];
    char ps_name[MAXPATHLEN];
    char startstr[32];
    sddisk_t *temp;
    ftd_state_t st;
    struct stat statbuf;
    char tunesetpath[MAXPATHLEN];
    char **setargv;
    pid_t childpid;
    int chldstat;
    int err;
    int infile, outfile;
    int max_dev;
    ps_version_1_attr_t attr;

	ftd_dev_info_t		*devp;

    /* pXXX.cfg is the user created version of the configuration, while
       pXXX.cur is the private copy of the configuration that will be used
       by future readconfigs so that other utilities will be working with
       the same version of the configuration that we start with here.  
       This allows the user to edit the pXXX.cfg files without affecting 
       anything until a subsequent start is run. */
    
    sprintf(cfg_path, "%s%03d.cfg", PMD_CFG_PREFIX, group);
    sprintf(cur_cfg_path, "%s%03d.cur", PMD_CFG_PREFIX, group);
    sprintf(full_cfg_path, "%s/%s", PATH_CONFIG, cfg_path);
    sprintf(full_cur_cfg_path, "%s/%s", PATH_CONFIG, cur_cfg_path);
    /* unlink the old .cur file in case we starting after an unorderly 
       shutdown.  (ftdstop should remove this file otherwise.) */
    unlink(full_cur_cfg_path);

    /* now copy the .cfg file for this group to .cur */
    if ((infile = open (full_cfg_path, O_RDONLY, 0)) != -1) {
        if ((outfile = creat (full_cur_cfg_path,  S_IRUSR | S_IWUSR)) != -1) {
            while (( n = read(infile, copybuf, BUFSIZ)) > 0) {
                if (write (outfile, copybuf, n) != n) {
                    fprintf(stderr, 
                            "Failed copy of %s to %s - write failed.\n", 
                            cfg_path, cur_cfg_path);
                    exit (-1);
                }
            }
            fchmod(outfile, S_IRUSR | S_IRGRP);
            close (outfile);
        } else {
            fprintf(stderr, "Failed copy of %s to %s - couldn't create %s\n", 
                    cfg_path, cur_cfg_path, cur_cfg_path);
            exit (-1);
        }
        close (infile);
    } else {
        fprintf(stderr, "Failed copy of %s to %s - couldn't open %s\n", 
                    full_cfg_path, full_cur_cfg_path, full_cfg_path);
    }
    
    /* open the cfg file */
    if ((ret = readconfig(1, 0, 0, cur_cfg_path)) < 0) {
        reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, cur_cfg_path);
        return (-1);
    }

    /* create the group name */
    FTD_CREATE_GROUP_NAME(group_name, group);

    if (getpsname(group, ps_name) != 0) {
        reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, cur_cfg_path);
        return (-1);
    }

    /* has this pstore been created? */
    ret = ps_get_version_1_attr(ps_name, &attr);
    if (ret != PS_OK) {
        /* no, so create it */
        ret = create_ps(ps_name, &max_dev);
        if (ret == 0) {
            reporterr(ERRFAC, M_PSCREAT_SUCCESS, ERRINFO, ps_name, group, max_dev);     
        } else {
            reporterr(ERRFAC, M_PSCREATE, ERRCRIT, ps_name, group);
            exit(-1);
        }
    }

    /* open the driver and create the group and devices! */
    /* create the group */
    if ((ret = conf_lg(ps_name, group_name, group, mysys->hostid, &state, &regen)) < 0) {
        return (-1);
    }

    /* backfresh mode means we don't add anything to the driver */
    if (state == FTD_MODE_BACKFRESH) {
        /* don't do anything here */
    }

	devp = (ftd_dev_info_t*)malloc(FTD_MAX_DEVICES*sizeof(ftd_dev_info_t));

    /* create the devices */
    for (temp = mysys->group->headsddisk, i=0; temp != NULL; temp = temp->n, i++) {
#if defined(HPUX)
	/*-
	 * deprecate the f*king control daemon.
	 * here, open each local device. 
	 * on return from this function, we'll fork(2).
	 * the forked process will persist until kill(2)'ed
	 * by the stop program...
	 */
	if( (temp->devid = open(temp->devname, O_RDONLY, 0666))  < 0 ) {
        	reporterr(ERRFAC, M_OPEN, ERRCRIT, temp->devname, strerror(errno));
            	return (-1);
        }
#endif /* defined(HPUX) */
        if (conf_dev(ps_name, temp->devname, temp->sddevname, group_name, 
                 group, regen, devp) < 0) {
			free(devp);
            return (-1);
        }
    }

	free(devp);

    ctlfd = open(FTD_CTLDEV, O_RDWR);
    if (ctlfd < 0) {
        reporterr(ERRFAC, M_FILE, ERRCRIT, FTD_CTLDEV, strerror(errno));
        return (-1);
    }

    st.lg_num = group;
    st.state = state;
    if (ioctl(ctlfd, FTD_SET_GROUP_STATE, &st) < 0) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't set group state", 
            strerror(errno));
        return (-1) ;
    }

    /* set the shutdown state of the group to FALSE */
    ps_set_group_shutdown(ps_name, group_name, 0);

    /* configtool may not have been able to set the tunables in 
       the pstore because start had not been run, so it stores
       them in a temp file that we'll read in and delete now. */
    sprintf(tunesetpath, PATH_CONFIG "/settunables%d.tmp",group);
    if (stat(tunesetpath, &statbuf) != -1) {
        setargv = (char **) ftdmalloc (sizeof(char *) * 2);
        setargv[0] = (char *) ftdmalloc (sizeof(tunesetpath));
        strcpy (setargv[0], tunesetpath);
        setargv[1] = (char *) NULL;
        childpid = fork ();
        switch (childpid) {
        case 0 : 
            /* child */
            execv(tunesetpath, setargv);
        default:
            wait(&chldstat);
            unlink(tunesetpath);
            free (setargv[0]);
            free (setargv);
        }
    }

    /* get tunables that driver needs to know about. */
    if (gettunables(group_name, 1, 0) < 1) {
        fprintf(stderr, "Couldn't retrieve tunable paramaters.");
        return -1;
    }   
    
    /* set the tunables in the driver */
    if (setdrvrtunables(group, &sys[0].tunables)!=0) {
        return -1;
    }
    /* set autostart flag in pstore */ 
    if (!autostart) {
        strcpy(startstr, "yes");
        err = ps_set_group_key_value(mysys->pstore, mysys->group->devname,
            "_AUTOSTART:", startstr);
        if (err != PS_OK) {
            reporterr(ERRFAC, M_PSWTGATTR, ERRCRIT, mysys->group->devname, ps_name);
            return -1;
        }
    }
    return (0);
}

static int
conf_lg(char *ps_name, char *group_name, int group, unsigned int hostid,
        int *state, int *regen_hrdb)
{
    char                *buffer;
    ftd_lg_info_t       info;
    int                 ctlfd, ret;
    stat_buffer_t       sb;
    ps_group_info_t     group_info;
    ps_version_1_attr_t attr;
    struct stat         statbuf;
#if defined(HPUX)
    ftd_devnum_t        dev_num;
    dev_t               dev;
#endif

    *regen_hrdb = 0;
    grp_started = 0;

    /* get the header to find out the size of the attribute buffer */
    ret = ps_get_version_1_attr(ps_name, &attr);
    if (ret != PS_OK) {
        reporterr(ERRFAC, M_PSRDHDR, ERRCRIT, ps_name);
        return -1;
    }

    if ((buffer = (char *)calloc(attr.group_attr_size, 1)) == NULL) {
        reporterr(ERRFAC, M_MALLOC, ERRCRIT, attr.group_attr_size);
        return -1;
    }

    /* get the group info to see if this group exists */
    group_info.name = NULL;
    ret = ps_get_group_info(ps_name, group_name, &group_info);

    if (ret == PS_GROUP_NOT_FOUND) {
        /* if attributes don't exist, add the group to the persistent store */
        group_info.name = group_name;
        group_info.hostid = hostid;
        ret = ps_add_group(ps_name, &group_info);
        if (ret != PS_OK) {
            /* we're hosed */
            reporterr(ERRFAC, M_PSADDGRP, ERRCRIT, group_name, ps_name);
            free(buffer);
            return -1;
        }

        /* set default attributes */
        strcpy(buffer, FTD_DEFAULT_TUNABLES);
        ret = ps_set_group_attr(ps_name, group_name, buffer, attr.group_attr_size);
        if (ret != PS_OK) {
            /* we're hosed */
            ps_delete_group(ps_name, group_name);
            reporterr(ERRFAC, M_PSWTGATTR, ERRCRIT, group_name, ps_name);
            free(buffer);
            return -1;
        }

        /* set the group state, too */
        ret = ps_set_group_state(ps_name, group_name, FTD_MODE_PASSTHRU);

        if (ret != PS_OK) {
            /* we're hosed */
            ps_delete_group(ps_name, group_name);
            reporterr(ERRFAC, M_PSWTGATTR, ERRCRIT, group_name, ps_name);
            free(buffer);
            return -1;
        }
        *state = FTD_MODE_PASSTHRU;

    } else if (ret == PS_OK) {
        /* see if the group was shutdown properly */
        if (group_info.shutdown != 1) {
            /* shutdown isn't needed for PASSTHRU mode */

            /* BACKFRESH doesn't add anything to the driver */
            if (group_info.state == FTD_MODE_BACKFRESH) {
                *state = FTD_MODE_BACKFRESH;
            }
            else if(group_info.state == FTD_MODE_PASSTHRU) {
                 *state = FTD_MODE_PASSTHRU;
            }
            else {
                /* regenerate the high res dirty bits */
                *regen_hrdb = 1;
                /* mode must be set to TRACKING */
                *state = FTD_MODE_TRACKING;
                ps_set_group_state(ps_name, group_name, FTD_MODE_TRACKING);
            }
        } else {
            *state = group_info.state;
        }
    } else {
        /* other error. we're hosed. */
        reporterr(ERRFAC, M_PSRDGATTR, ERRCRIT, group_name, ps_name);
        free(buffer);
        return -1;
    }

    ctlfd = open(FTD_CTLDEV, O_RDWR);
    if (ctlfd < 0) {
        reporterr(ERRFAC, M_FILE, ERRCRIT, FTD_CTLDEV, strerror(errno));
        free(buffer);
        return -1;
    }

    if (stat(ps_name, &statbuf) != 0) {
        reporterr(ERRFAC, M_PSSTAT, ERRCRIT, ps_name);
        close(ctlfd);
        free(buffer);
        return(-1);
    }
    
#if defined(_AIX)
    /* 
     * _AIX (struct stat *)p->st_dev is the device of 
     * the parent directory of this node: "/". ouch. 
     */
    info.persistent_store = statbuf.st_rdev;
#else /* defined(_AIX) */
    info.persistent_store = statbuf.st_rdev;
#endif /* defined(_AIX) */
    
#if defined(HPUX)

    if (ioctl(ctlfd, FTD_GET_DEVICE_NUMS, &dev_num) != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't get device numbers", 
            strerror(errno));
        close(ctlfd);
        free(buffer);
        return -1;
    }

    ftd_make_group(group, dev_num.c_major, &dev);
    info.lgdev = dev;        /* both the major and minor numbers */
#elif defined(SOLARIS) || defined(_AIX)
    {
        struct stat csb;
        fstat(ctlfd, &csb);
        info.lgdev = makedev(major(csb.st_rdev), group | FTD_LGFLAG);
    }
#endif

    info.statsize = attr.group_attr_size; 
    if (ioctl(ctlfd, FTD_NEW_LG, &info)) {
        if (errno == EADDRINUSE){
            grp_started = 1;
            close(ctlfd);
            free(buffer);
            return -1;
        } else {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't add group", 
                strerror(errno));
            close(ctlfd);
            free(buffer);
            return -1;
        }
    }

#if defined(SOLARIS) || defined(_AIX)
    ftd_make_group(group, info.lgdev);
#endif

    /* get the attributes of the group */
    ret = ps_get_group_attr(ps_name, group_name, buffer, attr.group_attr_size);

    /* add the attributes to the driver */
    sb.lg_num = group; /* the minor number of the group */
    sb.dev_num = 0;
    sb.len = attr.group_attr_size;
    sb.addr = (ftd_uint64ptr_t)buffer;
    if (ioctl(ctlfd, FTD_SET_LG_STATE_BUFFER, &sb)) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't set group stats", 
            strerror(errno));
        close(ctlfd);
        free(buffer);
        return -1;
    }

    free(buffer);
    close(ctlfd);
    return 0;
}

/* FIXME:  This code is a mess */
static int
conf_dev(char *ps_name, char *devname, char *sddevname, char *group_name,
	int lgnum, int regen_hrdb, ftd_dev_info_t *devp)
{
    stat_buffer_t       sbuf;
    ps_version_1_attr_t attr;
    ps_dev_info_t       dev_info;
    int                 ctlfd, lgfd, index, ret;
    ftd_dev_info_t      info;
    struct stat         sb;
    dev_t               cdev, bdev;
    char                *buffer;
    int                 *lrdb, *hrdb;
    unsigned int        hrdb_bits, lrdb_bits;
    dirtybit_array_t    dbarray;
    char                raw[MAXPATHLEN], block[MAXPATHLEN];
#if defined(HPUX)
    ftd_devnum_t        dev_num;
#endif
	int					need_regen = 0;

    /* make sure the device exists */
    if (stat(devname, &sb)) {
        reporterr(ERRFAC, M_STAT, ERRCRIT, devname, strerror(errno));
        return -1;
    }

    /* get the header to find out the max size of the dirty bitmaps */
    ret = ps_get_version_1_attr(ps_name, &attr);
    if (ret != PS_OK) {
        /* we're hosed. */
        return -1;
    }

    if ((buffer = (char *)calloc(attr.dev_attr_size, 1)) == NULL) {
        reporterr(ERRFAC, M_MALLOC, ERRCRIT, attr.dev_attr_size);
        return -1;
    }

    /* create the device names */
    strcpy(raw, sddevname);
    force_dsk_or_rdsk(block, raw, 0);

    /* get the attributes of the device */
    ret = ps_get_device_attr(ps_name, raw, buffer, attr.dev_attr_size);
    if (ret == PS_OK) {
/* FIXME: get the state information, if any. */
        if (get_lrdb(ps_name, raw, attr.lrdb_size, &lrdb, &lrdb_bits) != PS_OK) {
            /* we're hosed */
            free(buffer);
            return -1;
        }
        if (get_hrdb(ps_name, raw, attr.hrdb_size, &hrdb, &hrdb_bits) != PS_OK) {
            /* we're hosed */
            free(lrdb);
            free(buffer);
            return -1;
        }
        /* if the hrdb is bogus, create a new one */
        if (regen_hrdb) {
			need_regen = 1;
        }
    } else if (ret == PS_DEVICE_NOT_FOUND) {
        /* if attributes don't exist, add the device to the persistent store */
        dev_info.name = raw;

/* FIXME: how do we override/calculate these values? */
        dev_info.num_lrdb_bits = attr.lrdb_size * 8;
        dev_info.num_hrdb_bits = attr.hrdb_size * 8;
        ret = ps_add_device(ps_name, &dev_info);
        if (ret != PS_OK) {
            /* we're hosed */
            free(buffer);
            return -1;
        }

        /* set up default LRDB and HRDB */
        if ((lrdb = (int *)malloc(attr.lrdb_size)) == NULL) {
            free(buffer);
            reporterr(ERRFAC, M_MALLOC, ERRCRIT, attr.lrdb_size);
            return -1;
        }
        if ((hrdb = (int *)malloc(attr.hrdb_size)) == NULL) {
            reporterr(ERRFAC, M_MALLOC, ERRCRIT, attr.hrdb_size);
            free(lrdb);
            free(buffer);
            return -1;
        }
        memset((caddr_t)lrdb, 0xff, attr.lrdb_size);
        memset((caddr_t)hrdb, 0xff, attr.hrdb_size);
        ret = ps_set_lrdb(ps_name, raw, (caddr_t)lrdb, attr.lrdb_size);
        if (ret != PS_OK) {
            /* we're hosed */
            ps_delete_device(ps_name, raw);
            free(lrdb);
            free(hrdb);
            free(buffer);
            return -1;
        }
        ret = ps_set_hrdb(ps_name, raw, (caddr_t)hrdb, attr.hrdb_size);
        if (ret != PS_OK) {
            /* we're hosed */
            ps_delete_device(ps_name, raw);
            free(lrdb);
            free(hrdb);
            free(buffer);
            return -1;
        }

/* FIXME: set up default attributes, if any. */
        sprintf(buffer, "DEFAULT_DEVICE_ATTRIBUTES: NONE\n");
        ret = ps_set_device_attr(ps_name, raw, buffer, attr.dev_attr_size);
        if (ret != PS_OK) {
            /* we're hosed */
            ps_delete_device(ps_name, raw);
            free(lrdb);
            free(hrdb);
            free(buffer);
            return -1;
        }
    } else {
        /* other error. we're hosed. */
        free(buffer);
        return -1;
    }

    ctlfd = open(FTD_CTLDEV, O_RDWR);
    if (ctlfd < 0) {
        reporterr(ERRFAC, M_OPEN, ERRCRIT, FTD_CTLDEV, strerror(errno));
        free(lrdb);
        free(hrdb);
        free(buffer);
        return -1;
    }

/* new code begin- for multiple pstores ... */

   if(ioctl(ctlfd, FTD_CTL_ALLOC_MINOR, &index) < 0) {
   printf("\n error getting the minor number ");
   free(lrdb);
   free(hrdb);
   free(buffer);
   return -1;
   } 
/* new code ends..... */

    lgfd = open(group_name, O_RDWR);
    if (lgfd < 0) {
        reporterr(ERRFAC, M_OPEN, ERRCRIT, group_name, strerror(errno));
        free(lrdb);
        free(hrdb);
        free(buffer);
        close(ctlfd);
        return -1;
    }

#if defined(HPUX)

    if (ioctl(ctlfd, FTD_GET_DEVICE_NUMS, &dev_num) != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't get device numbers", 
            strerror(errno));
        free(lrdb);
        free(hrdb);
        free(buffer);
        close(lgfd);
        close(ctlfd);
        return -1;
    }
    /* 
     * On HP_UX we create the devices first because we need the device numbers
     * for the special device files before we call the ioctl to add the
     * device to the logical group. The Solaris driver creates device 
     * files during the ioctl (or it preallocates them) with some weird 
     * name (e.g. /device/pseudo/ftd@0:raw,0), so we end up creating a 
     * symbolic link with a more reasonable name 
     */
    ftd_make_devices(raw, block, dev_num.c_major, dev_num.b_major, index, 
                     &cdev, &bdev);

#elif defined(SOLARIS) || defined(_AIX)
    {
        struct stat csb;
        fstat(ctlfd, &csb);
        cdev = makedev(major(csb.st_rdev), index);
        bdev = cdev;
    }
#endif
    info.lgnum = lgnum;
    info.cdev = cdev;
    info.bdev = bdev;
    info.localcdev = sb.st_rdev;
    info.disksize = disksize(devname);
    if (info.disksize == -1) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't get disk size", 
            strerror(errno));
        free(lrdb);
        free(hrdb);
        free(buffer);
        close(lgfd);
        close(ctlfd);
        return -1;
    }
    info.lrdbsize32 = attr.lrdb_size / 4;      /* number of 32-bit words */
    info.hrdbsize32 = attr.hrdb_size / 4; /* number of 32-bit words */
    info.statsize = attr.dev_attr_size; 
    ps_get_lrdb_offset(ps_name, raw, &info.lrdb_offset);

    if (ioctl(ctlfd, FTD_NEW_DEVICE, &info)) {
        if (grp_started && errno == EADDRINUSE) {
        } else {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't add new device", 
                strerror(errno));
            free(lrdb);
            free(hrdb);
            free(buffer);
            close(lgfd);
            close(ctlfd);
            return -1;
        }
    }

#if defined(SOLARIS) || defined(_AIX)
    ftd_make_devices(raw, block, bdev);
#endif

    /* add the attributes to the driver */
    sbuf.lg_num = lgnum; /* by luck: the minor number of the group */
    sbuf.dev_num = cdev; /* was: index */
    sbuf.len = attr.dev_attr_size;
    sbuf.addr = (ftd_uint64ptr_t)buffer;
    if (ioctl(ctlfd, FTD_SET_DEV_STATE_BUFFER, &sbuf)) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't set group stats", 
            strerror(errno));
        close(lgfd);
        close(ctlfd);
        free(lrdb);
        free(hrdb);
        free(buffer);
        return -1;
    }
#if 0
printf("\n$$$ start: lrdb = \n");
{
unsigned char lrdbc[1024];
int i;

memcpy(lrdbc, lrdb, 1024);
for(i=0;i<1024;i++) {
	printf("%02x",lrdbc[i]);
}
printf("\n");
}
#endif
    /* set the LRDB in the driver */
    dbarray.numdevs = 1;
    {
    dev_t tempdev[2];
    ftd_dev_info_t devinfo;
    tempdev[0] = cdev;
    tempdev[1] = 0x12345678;
    dbarray.devs = (dev_t *)&tempdev[0];
    dbarray.dblen32 = info.lrdbsize32;
    dbarray.dbbuf = (ftd_uint64ptr_t)lrdb;
    devinfo.lrdbsize32 = dbarray.dblen32;
    if (dbarr_ioc(lgfd, FTD_SET_LRDBS, &dbarray, &devinfo)) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't set lrdb", 
            strerror(errno));
        close(lgfd);
        close(ctlfd);
        free(lrdb);
        free(hrdb);
        free(buffer);
        return -1;
    }
    }

    /* set the HRDB in the driver */
	/* we have bitsize set (by new_device) at this point */
	if (need_regen) {
        unsigned int value, expansion;
        int          i, j, k, num32, bitnum;

		sbuf.lg_num = lgnum;
		sbuf.addr = (ftd_uint64ptr_t)devp;
		if (ioctl(ctlfd, FTD_GET_DEVICES_INFO, &sbuf) != 0) {
			reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_DEVICES_INFO", 
				strerror(errno));
			return -1;
		}
		for(i=0;i<FTD_MAX_DEVICES;i++) {
			if (cdev == devp[i].cdev) {
				break;
			}
		}
		if (i == FTD_MAX_DEVICES) {
			/* this can't happen - we added the dev above */
			reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_DEVICES_INFO", 
				strerror(errno));
			return -1;
		}

		/* this should be a power of 2 */
		expansion = 1 << (devp[i].lrdb_res - devp[i].hrdb_res);
#if 0
		printf("\n$$$ disk size, lrdb_numbits, hrdb_numbits, lrdb_res, hrdb_res, expansion = %d, %d, %d, %d, %d, %d\n",
			devp[i].disksize,
			devp[i].lrdb_numbits,
			devp[i].hrdb_numbits,
			devp[i].lrdb_res,
			devp[i].hrdb_res, expansion); 
#endif
		/* zero it out */
		memset(hrdb, 0, attr.hrdb_size);

        /* lrdb_numbits to words + 1, hrdb_size (bytes) to words / expansion */
        num32 = min( (devp[i].lrdb_numbits / 32) + 1, (attr.hrdb_size / 4) / expansion );

		/* march thru bitmap one 32-bit word at a time */
		for (j = 0; j < num32; j++) {
			/* blow off empty words */
			if ((value = lrdb[j]) == 0) continue;

			/* test each bit */
			bitnum = j * expansion * 32;
			for (k = 0; k < 32; k++) {
				if ((value >> k) & 1) {
					set_bits((unsigned int *)hrdb, bitnum + k*expansion,
						bitnum + ((k+1)*expansion)-1);
				}
			} 
		}
	}
#if 0
printf("\n$$$ start: hrdb = \n");
{
unsigned char lrdbc[1024];
int i;

memcpy(lrdbc, hrdb, 1024);
for(i=0;i<1024;i++) {
	printf("%02x",lrdbc[i]);
}
printf("\n");
}
#endif
    dbarray.numdevs = 1;
    {
    dev_t tempdev=cdev;
    ftd_dev_info_t devinfo;
    dbarray.devs = (dev_t *)&tempdev;
    dbarray.dblen32 = info.hrdbsize32;
    dbarray.dbbuf = (ftd_uint64ptr_t)hrdb;
    devinfo.hrdbsize32 = dbarray.dblen32;
    if (dbarr_ioc(lgfd, FTD_SET_HRDBS, &dbarray, &devinfo)) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "can't set hrdb", 
            strerror(errno));
        close(lgfd);
        close(ctlfd);
        free(lrdb);
        free(hrdb);
        free(buffer);
        return -1;
    }
    }
    free(lrdb);
    free(hrdb);
    free(buffer);
    close(ctlfd);
    close(lgfd);
    return 0;

}

/*
 * starts the driver, if needed
 *
static int
start_driver(char *psname)
{
    struct stat sb;
    int ctlfd;
    
    if (stat(psname, &sb))
        return -1;
    ctlfd = open(FTD_CTLDEV, O_RDWR);
    if (ctlfd < 0) {
        reporterr(ERRFAC, M_FILE, ERRCRIT, FTD_CTLDEV, strerror(errno));
        return (-1);
    }
    if (ioctl(ctlfd, FTD_SET_PERSISTENT_STORE, &sb.st_rdev) && errno != EBUSY) {
        reporterr(ERRFAC, M_FILE, ERRCRIT, psname, strerror(errno));
        return -1;
    }
    return 0;
}*/

int 
main(int argc, char *argv[])
{
    int i, fd, group, all, force, lgcnt, pid, pcnt, chpid;
#if defined(_AIX)
    int ch;
#else  /* defined(_AIX) */
    char ch;
#endif /* defined(_AIX) */
    char **setargv;
    int exitcode;
#if defined(HPUX)
    struct stat statbuf; 
    char cmd[128];
#endif
    progname = argv[0];
    if (argc < 2) {
        goto usage_error;
    }
    argv0 = argv[0];
    all = 0;
    force = 0;
    group = -1;
    
    setmaxfiles();
    autostart = 0;
    /* process all of the command line options */
    opterr = 0;
    while ((ch = getopt(argc, argv, "afbg:")) != EOF) {
        switch(ch) {
        case 'a':
            if (group != -1) {
                goto usage_error;
            }
            all = 1;
            break;
        case 'f':
            force = 1;
            break;
        case 'g':
            if (all != 0) {
                goto usage_error;
            }
            group = strtol(optarg, NULL, 0);
            break;
        case 'b':
            autostart = 1;
            break;
        default:
            goto usage_error;
        }
    }
    if ((all == 0) && (group < 0)) {
        goto usage_error;
    }
    if (initerrmgt(ERRFAC) != 0) {
       /* NO need to exit here because it causes HP-UX */
       /* replication devices' startup at boot-time to fail */
       /* PRN# 498       */
    }
#if defined(HPUX) && defined(OLD_CONTROL_DAEMON)
    if ((pid = getprocessid(QNM "ctl", 0, &pcnt)) > 0) {
        /* signal the 'hold open' process so that it opens only our currently
           started groups */
    } else {
        /* if it isn't there, it should be, so exec it. */
        setargv = (char **) ftdmalloc (sizeof(char *) * 2);
        setargv[0] = (char *) ftdmalloc (sizeof(ftdctlpath));
        strcpy (setargv[0], ftdctlpath);
        setargv[1] = (char *) NULL;
        chpid = fork();
        switch (chpid) {
        case 0 :
            /* child */
            execv(ftdctlpath, setargv);
        default:
	    break;
        }
    }

    /* verify ctld is started */
    for (i = 0; i < 5; i++) {
	if (getprocessid(QNM "ctl", 0, &pcnt) > 0) 
	   break;
	sleep(1);
    }
    if (i >= 5) {
	/* ctld is not started, don't start group */
        reporterr(ERRFAC, M_CTLDDOWN1, ERRCRIT);
	exit(1);
    }

#endif



#if defined(HPUX)
      if (stat(FTD_CTLDEV, &statbuf) != 0) {
        sprintf(cmd, "/etc/mksf -d %Q% -m 0xffffff -r %s", FTD_CTLDEV);
        system(cmd);
    }
#endif
    /* config.c bogusity */
    paths = (char*)configpaths;

    exitcode = 0;
    if (all != 0) {
        if (autostart) {
            /* get paths of previously started groups */
            lgcnt = getconfigs(configpaths, 1, 1);
            if (lgcnt == 0) {
                exit(1);
            }
        } else {
            /* get paths of all groups */
            lgcnt = getconfigs(configpaths, 1, 0);
            if (lgcnt == 0) {
                reporterr(ERRFAC, M_NOCONFIG, ERRWARN);
                exit(1);
            }
        }
        for (i = 0; i < lgcnt; i++) {
            group = cfgtonum(i);
            if (start_group(group, force, autostart)!=0) {
                reporterr(ERRFAC, M_STARTGRP, ERRCRIT, group);
                exitcode = -1;
            } 
#if defined(HPUX)
            else {
                /*-
                 * instantiate a per-group control daemon
                 */
                 control_daemon_init(group);
                 {
                      int z;
                      sddisk_t *temp; 
                      for (temp = mysys->group->headsddisk, z=0; temp != NULL; temp = temp->n, z++) {
                           close(temp->devid);
                      }
                 }
             }
#endif /* defined(HPUX) */
        }
    } else {
        if (start_group(group, force, autostart)!=0) {
            reporterr(ERRFAC, M_STARTGRP, ERRCRIT, group);
            exitcode = -1;
        } 
#if defined(HPUX)
        else  {
		/*-
	 	 * instantiate a per-group control daemon
	 	 */
		control_daemon_init(group);
		{
		    int z;
		    sddisk_t *temp;
		    for (temp = mysys->group->headsddisk, z=0; temp != NULL; temp = temp->n, z++) {
		        close(temp->devid);
		    }
		}
        } 
#endif /* defined(HPUX) */
    }
#if defined(HPUX) && defined(OLD_CONTROL_DAEMON)
    /* .cur file is built, send signal to dtcctl */
    if ((pid = getprocessid(QNM "ctl", 0, &pcnt)) > 0) {
        /* signal the 'hold open' process so that it opens only our currently
           started groups */
        kill (pid, SIGUSR1);
    }
    else {
        reporterr(ERRFAC, M_CTLDDOWN, ERRCRIT);
    }
#endif /* defined(HPUX) && defined(OLD_CONTROL_DAEMON) */
    exit(exitcode);
    return exitcode; /* for stupid compiler */
usage_error:
    usage();
    exit(1);
    return 0; /* for stupid compiler */
}

/* The Following 3 lines of code are intended to be a hack 
   only for HP-UX 11.0 . 
   HP-UX 11.0 compile complains about not finding the below
   function when they are staticaly linked - also we DON'T want
   to link it dynamically. So for the time being - ignore these
   functions while linking. */ 
#if defined(HPUX) && (SYSVERS >= 1100)
  shl_load () {}
  shl_unload () {}
  shl_findsym () {}
#endif
