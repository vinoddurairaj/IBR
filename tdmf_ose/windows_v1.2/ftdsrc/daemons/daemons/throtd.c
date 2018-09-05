/***************************************************************************
 * throtd.c - FullTime Data throttle evaluation/performance calc  daemon
 *
 * (c) Copyright 1998 FullTime Software, Inc. All Rights Reserved
 *
 * This module implements the high level functionality for throtd
 *
 * $Id: throtd.c,v 1.2 2001/11/08 18:54:31 dturrin Exp $
 ***************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#if defined(SOLARIS)
#include <sys/procfs.h>
#elif defined(HPUX)
#include <sys/pstat.h>
#elif defined(_AIX)
#include <sys/../procinfo.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/un.h>
#include "errors.h"
#include "config.h"
#include "pathnames.h"
#include "process.h"
#include "ftdio.h"
#include "ps_intr.h"
#include "cfg_intr.h"
#include "stat_intr.h"
#include "network.h"
#include "ftd_cmd.h"
#include "aixcmn.h"

#ifdef DEBUG
FILE *dbgfd;
#endif

extern char *argv0;
extern int lgcnt;

char configpaths[MAXLG][32];
char prevconfigpaths[MAXLG][32];
static machine_t* configsys[MAXLG];
static machine_t* tsys[MAXLG];
static int configcount = 0;
static int masterfd = -1;


char *paths = (char*)configpaths;

int exitfail = EXITNORMAL;
int len;

#if !defined(SECTORSIZE)
#if defined(SOLARIS) || defined(_AIX)
#define SECTORSIZE 512
#elif defined(HPUX)
#define SECTORSIZE 1024
#endif
#endif /* !defined(SECTORSIZE) */

static int psfd = -1;

static
int
isgroupdead (int groupindex)
{
    int lgnum;
    stat_buffer_t statbuf;
    ftd_stat_t lgdrvstats;
    int err;
    
    /*  -- if not open, try to open the masterfd ftd device */
    if (masterfd == -1) {
        if ((masterfd = open(FTD_CTLDEV, O_RDWR)) < 0) {
            masterfd = -1;
            return (1);
        }
    }

    lgnum = cfgtonum(groupindex);
    /* get driver lg stats */
    statbuf.lg_num = lgnum;
    statbuf.dev_num = 0;
    statbuf.len = sizeof(ftd_stat_t);
    statbuf.addr = (char *)&lgdrvstats;
    if ((err = ioctl(masterfd, FTD_GET_GROUP_STATS, &statbuf)) < 0) {
        return (1);
    }
    return (0);
}

static
int
getstartedgroups (char buf[][32])
{
    int lgnum;
    stat_buffer_t statbuf;
    ftd_stat_t lgdrvstats;
    int err;
    int count;
    
    count=0;
    /*  -- if not open, try to open the masterfd ftd device */
    if (masterfd == -1) {
        if ((masterfd = open(FTD_CTLDEV, O_RDWR)) < 0) {
            masterfd = -1;
            return (1);
        }
    }

    for (lgnum=0; lgnum<1000; lgnum++) {
        /* get driver lg stats */
        statbuf.lg_num = lgnum;
        statbuf.dev_num = 0;
        statbuf.len = sizeof(ftd_stat_t);
        statbuf.addr = (char *)&lgdrvstats;
        if ((err = ioctl(masterfd, FTD_GET_GROUP_STATS, &statbuf)) >= 0) {
            sprintf (buf[count++], "p%03d.cur", lgnum);

        }
    }
    for (lgnum=count; lgnum<1000; lgnum++) {
        buf[lgnum][0] = '\0';
    }
    return (count);
}


/****************************************************************************
 * getstats -- compile a list of device stats 
 ***************************************************************************/
int
getstats (group_t *group, int lgnum, char *configpath, int deltasec)
{
    struct stat sb[1];
    sddisk_t *sd;
    devstat_t *statp;
    stat_buffer_t statbuf;
    pid_t pmdpid;
    int err;
    int pcnt;
    int procfd;
    char devname[MAXPATHLEN];
    char devbuffer[FTD_PS_DEVICE_ATTR_SIZE];
    devthrotstat_t *devthrotstats;
    lgthrotstat_t *lgthrotstats;
    ftd_stat_t lgdrvstats;
    disk_stats_t devdrvstats;
    char procfile[MAXPATHLEN] = "";
    char ps_name[MAXPATHLEN] = "";     /* path to persistent store */
    char procname[MAXPATHLEN] =  "";
    char pmdgrpno[4];
    long totalsects;
    int statbuflen;
#if defined(SOLARIS)
    struct prpsinfo ps;
#elif defined(HPUX)
    struct pst_status ps;
#endif

    lgthrotstats = &(group->throtstats);

    /* find out where the pstore is */
   
    /* get driver lg stats */
    statbuf.lg_num = lgnum;
    statbuf.dev_num = 0;
    statbuf.len = sizeof(ftd_stat_t);
    statbuf.addr = (char *)&lgdrvstats;

    if ((err = ioctl(masterfd, FTD_GET_GROUP_STATS, &statbuf)) < 0) {
#ifdef DEBUG_THROTTLE
        fprintf(stderr,"FTD_GET_GROUP_STATS ioctl: error = %d\n", errno);
#endif /* DEBUG_THROTTLE */
        return (0);
    }
    lgthrotstats->pctbab = (int) ((((double)lgdrvstats.bab_used) * 100.0) / 
        (((double)lgdrvstats.bab_used) + ((double)lgdrvstats.bab_free)));
lgthrotstats->entries = lgdrvstats.wlentries;
    lgthrotstats->sectors = lgdrvstats.bab_used / SECTORSIZE;
    /* get the driver mode */
    if (lgdrvstats.state == FTD_MODE_PASSTHRU) 
        lgthrotstats->drvmode = DRVR_PASSTHRU;
    if (lgdrvstats.state == FTD_MODE_NORMAL)
        lgthrotstats->drvmode = DRVR_NORMAL;
    if (lgdrvstats.state == FTD_MODE_TRACKING)
        lgthrotstats->drvmode = DRVR_TRACKING;
    if (lgdrvstats.state == FTD_MODE_REFRESH)
        lgthrotstats->drvmode = DRVR_REFRESH;
    if (lgdrvstats.state == FTD_MODE_BACKFRESH)
        lgthrotstats->drvmode = DRVR_BACKFRESH;
    strncpy (pmdgrpno, configpath+1, 3);
    pmdgrpno[3] = '\0';
  
    sprintf(procname, "PMD_%s", pmdgrpno);
   
    /* figure out whether pmd is running and get pid here */
    if ((pmdpid = getprocessid(procname, 1, &pcnt)) <= 0) {
        lgthrotstats->pmdup = FALSE;
        lgthrotstats->pid = -1;
    } else {
        lgthrotstats->pmdup = TRUE;
        lgthrotstats->pid = pmdpid;

#if defined(SOLARIS)
        sprintf (procfile,"/proc/%05d", (int) pmdpid);
        if ((procfd = open (procfile, O_RDONLY))==-1) {
#ifdef DEBUG_THROTTLE
            fprintf(stderr, "Couldn't open proc file: %s",procfile);
#endif /* DEBUG_THROTTLE */
            lgthrotstats->percentcpu = 0;
            close (procfd);
        } else {
            (void) ioctl (procfd, PIOCPSINFO, (char*)&ps);
            lgthrotstats->percentcpu  = (int)((((ps.pr_pctcpu && 0x0000ffff) * 1.0) / 327.68) + 0.5);
            close (procfd);
        }
#elif defined(HPUX)
        if (pstat_getproc(&ps, sizeof(ps), 0, pmdpid) == -1) {
#ifdef DEBUG_THOTTLE
            fprintf(stderr, "pstat_getproc failed for pid %d\n",pmdpid);
#endif /* DEBUG_THROTTLE */
            lgthrotstats->percentcpu=0;
        } else {
            lgthrotstats->percentcpu = (int) ((((ps.pst_pctcpu && 0x0000ffff) * 1.0) / 327.68) + 0.5);
        }
#elif defined(_AIX)
{
	/*- 
	 * _AIX doesn't hand you the equivalent of %CPU. 
	 * so...compute it from various times...
	 */
	int ncpu;
	int pctcpu;
	unsigned long now;
	unsigned long ttime;
	unsigned long etime;
	struct procsinfo pb;

	pid_t pididx=pmdpid;

	struct fdsinfo *fb = (struct fdsinfo *) 0;

	if(getprocs(&pb, sizeof(pb), fb, 0, &pididx, 1) ==  1) {
		now = (unsigned long)time(0);
		ncpu = sysconf(_SC_NPROCESSORS_CONF);
		ttime = now - pb.pi_start;
		etime = pb.pi_utime + pb.pi_stime;
		pctcpu = (100 * etime) / ttime;
		pctcpu = pctcpu / ncpu;
		lgthrotstats->percentcpu = pctcpu;
	}
	else {
		/* getprocs(3) bummer */
		lgthrotstats->percentcpu = 0;
	}
}
#endif
    }
    /* We'll calculate the lg % done by the lowest device %, so init to 100 */
    lgthrotstats->pctdone = 100;
    statbuflen = FTD_PS_DEVICE_ATTR_SIZE;
    lgthrotstats->actualkbps = 0;
    lgthrotstats->effectkbps = 0;
    lgthrotstats->local_kbps_read = 0.0;
    lgthrotstats->local_kbps_written = 0.0;

    /* get stats for each device */

    sd = group->headsddisk;
    while (sd) {
        force_dsk_or_rdsk(devname, sd->sddevname, 1);
        stat(devname, sb);
        sd->sd_rdev = sb->st_rdev;

        statbuf.lg_num = lgnum;
        statbuf.dev_num = sd->sd_rdev;
        statbuf.len = FTD_PS_DEVICE_ATTR_SIZE;
        statbuf.addr = devbuffer;
 
        /* get the pmds stat info for the device */
        if((err = ioctl(masterfd, FTD_GET_DEV_STATE_BUFFER, &statbuf)) < 0) {
#ifdef TDMF_TRACE
            fprintf(stderr,"FTD_GET_DEV_STATE_BUFFER ioctl: error = %d\n",errno);
#endif /* DEBUG THROTTLE */
            return (0); 
        }
        /* stats from pmd are in statp */
        statp = (devstat_t*)devbuffer;

        statbuf.len = sizeof(disk_stats_t);
        statbuf.addr = (char*)&devdrvstats;

        /* get the drvr stat info for the device */
        if((err = ioctl(masterfd, FTD_GET_DEVICE_STATS, &statbuf)) < 0) {
#ifdef DEBUG_THROTTLE
            fprintf(stderr,"FTD_GET_DEV_DEVOCE_STATS ioctl: error = %d\n",errno);
#endif /* DEBUG_THROTTLE */
            return (0);
        }
        /* calculate dev stats */
        devthrotstats = &(sd->throtstats);
        /* device reads/writes --> sum to group reads/writes */
        devthrotstats->dev_ptotread_sectors = devthrotstats->dev_ctotread_sectors;
        devthrotstats->dev_ptotwrite_sectors = devthrotstats->dev_ctotwrite_sectors;
        devthrotstats->dev_ctotread_sectors = devdrvstats.sectorsread;
        devthrotstats->dev_ctotwrite_sectors = devdrvstats.sectorswritten;
        if (devthrotstats->dev_ptotread_sectors == 0L) {
            devthrotstats->dev_ptotread_sectors = devthrotstats->dev_ctotread_sectors;
            devthrotstats->dev_ptotwrite_sectors = devthrotstats->dev_ctotwrite_sectors;
        }
        devthrotstats->local_kbps_read = ((((double)(int)(devthrotstats->dev_ctotread_sectors - devthrotstats->dev_ptotread_sectors)) * (double)DEV_BSIZE) / 1024.0) / (double) mysys->tunables.statinterval;
        devthrotstats->local_kbps_written = ((((double)(int)(devthrotstats->dev_ctotwrite_sectors - devthrotstats->dev_ptotwrite_sectors)) * (double)DEV_BSIZE) / 1024.0) / (double) mysys->tunables.statinterval;
        lgthrotstats->local_kbps_read += devthrotstats->local_kbps_read;
        lgthrotstats->local_kbps_written += devthrotstats->local_kbps_written;
        /* -- other per device statistics from driver */
        devthrotstats->sectors = devdrvstats.wlsectors;
        devthrotstats->entries = devdrvstats.wlentries;
        devthrotstats->pctbab = (devdrvstats.wlsectors * SECTORSIZE * 100.0) / ((lgdrvstats.bab_used + lgdrvstats.bab_free) * 1.0);
        devthrotstats->actualkbps = ((statp->a_tdatacnt * 1.0) / 
                                     (deltasec * 1.0)) / 1024.0;
        devthrotstats->effectkbps = ((statp->e_tdatacnt * 1.0) /  
                                     (deltasec * 1.0)) / 1024.0;
        /*  calculate percent done  */
        totalsects = (long)sd->devsize;
        devthrotstats->pctdone = (( statp->rfshoffset * 1.0) /
                                  (totalsects * 1.0)) * 100.0;

       
        /* reset the pmds devstat info since it has been read*/
        statbuf.len = FTD_PS_DEVICE_ATTR_SIZE;
        statbuf.addr = devbuffer;
        statp->a_tdatacnt = statp->e_tdatacnt = statp->e_datacnt = 
            statp->a_datacnt = statp->entries = 0;

        if((err = ioctl(masterfd, FTD_SET_DEV_STATE_BUFFER,  &statbuf)) < 0) {
#ifdef DEBUG_THROTTLE
            fprintf(stderr,"FTD_SET_DEV_STATE_BUFFER ioctl: error = %d\n", errno);
#endif /* DEBUG_THROTTLE */
            return (0);
        }
        /* lg % done should be equal to the lowest device percentage */
        if (devthrotstats->pctdone < lgthrotstats->pctdone) {
            lgthrotstats->pctdone = devthrotstats->pctdone;
        }
        lgthrotstats->actualkbps = lgthrotstats->actualkbps + devthrotstats->actualkbps;
        lgthrotstats->effectkbps = lgthrotstats->effectkbps + devthrotstats->effectkbps;
        sd = sd->n;
    }
    return (1);
} /* getstats */

/****************************************************************************
 * main -- throtd process main
 ****************************************************************************/
void
main (int argc, char **argv) {

    char configpath[FILE_PATH_LEN];
    time_t ts;
    time_t cfgchkts;
    int elapsedtime;
    int lgnum;
    int i, j, k;
    int t;
    int stopflag;
    FILE *stashed_statfd;
    char ps_name[MAXPATHLEN] = "";
    char group_name[MAXPATHLEN];
    struct stat sbuf;
    stat_buffer_t statbuf;
    ftd_stat_t lgdrvstats;
    int err;
    sddisk_t* sd;
    sddisk_t* nsd;
    lgthrotstat_t t_throtstats;
    ps_version_1_attr_t attr;    /* persistent store header */
    struct timeval wakeupcall;
    int getpstoreflag;
    int rc;
    char startstr[32];
    int cfgcnt;

    wakeupcall.tv_sec = 1;
    wakeupcall.tv_usec = 0; 
    cfgchkts = (time_t) 0;
    getpstoreflag = 1;

    argv0 = strdup(argv[0]);
#ifdef DEBUG_THROTTLE
    throtfd = fopen ( PATH_RUN_FILES "/throtdump.dbg", "w+");
#endif /* DEBUG_THROTTLE */

    if (initerrmgt ("FTD") < 0) {
        exit(1);
    }

    /* -- initialize the arrays of critical configuration for all groups */
    for (i=0; i<MAXLG; i++) {
        prevconfigpaths[i][0] = '\0';
        configpaths[i][0] = '\0';
        configsys[i] = (machine_t*) NULL;
        tsys[i] = (machine_t*) NULL;
    }
    configcount = 0;
    
    /* run forever and evaluate throttles, dump stats */
    while (1) {  
        /*  -- if not open, try to open the masterfd ftd device */
        if (masterfd == -1) {
            if ((masterfd = open(FTD_CTLDEV, O_RDWR)) < 0) {
                masterfd = -1;
                fflush (stdout);
                select (NULL, NULL, NULL, NULL, &wakeupcall); 
                continue;
            }
        }
        /* -- see if any groups have been stopped */
        stopflag = 0;
        for (i=0; i<configcount; i++) {
            if (isgroupdead(i)) {
                stopflag = 1;
            }
        }
        /* -- check the config files for primary */
        (void) time (&ts); /* get a clean new timestamp with each group */
        if (stopflag || ((ts-cfgchkts) >= 10)) {
            cfgchkts = ts;
/*             lgcnt = getconfigs(configpaths, 1, 1); */
            lgcnt = getstartedgroups(configpaths);
            /* -- get rid of groups that have gone away since last time */
            k = 0;
            for (j=0; j<configcount; j++) {
                t = -1;
                for (i=0; i<lgcnt; i++) {
                    if (0 == (strcmp(prevconfigpaths[j], configpaths[i]))) {
                        t = i;
                        break;
                    }
                }
                if (t < 0) {
                    k = 1;
                    prevconfigpaths[j][0] = '\0';
                    fclose (configsys[j]->statfd);
                    sd = configsys[j]->group->headsddisk;
                    while (sd) {
                        nsd = sd->n;
                        free(sd);
                        sd = nsd;
                    }
                    free (configsys[j]);
                }
            }
            /* -- compact any gaps in group lists after group removal */
            if (k) {
                i = -1;
                for (j=0; j<configcount; j++) {
                    if (0 < strlen(prevconfigpaths[j])) {
                        i++;
                        if (i != j) {
                            strcpy (prevconfigpaths[i], prevconfigpaths[j]);
                            configsys[i] = configsys[j];
                        }
                    }
                }
                for (j=i+1; j<configcount; j++) {
                    prevconfigpaths[j][0] = '\0';
                    configsys[j] = (machine_t*) NULL;
                }
                configcount = i+1;
            }
            /* -- insert new groups that appeared since last time */
            k = 0;
            for (j=0; j<lgcnt; j++) {
                t = -1;  /* assume match, disprove that it exist, if can */
                for (i=0; i<configcount; i++) {
                    if (0 == (strcmp(configpaths[j], prevconfigpaths[i]))) {
                        t = i;
                        break;
                    } 
                }
                if (t == -1) {
                    /* -- new group detected */
                    k = 1;
                    tsys[j] = (machine_t*) ftdmalloc (sizeof(machine_t));
                    memset ((void*)tsys[j], 0, sizeof(machine_t));
                    memcpy ((void*)mysys, (void*)tsys[j], 
                            sizeof(machine_t));
                    initconfigs();
                    (void) readconfig (1, 2, 0, configpaths[j]);
                    lgnum = cfgtonum(j);
                    FTD_CREATE_GROUP_NAME(group_name, lgnum);
                    gettunables (group_name, 0, 1);
                    initstats(mysys);
                    memcpy ((void*)tsys[j], (void*)mysys, sizeof(machine_t));
                } else {
                    tsys[j] = configsys[t];
                }
            }
            configcount = lgcnt;
            /* -- if any new groups added, update working name/structure */
            if (k) {
                for (j=0; j<configcount; j++) {
                    configsys[j] = tsys[j];
                    strcpy (prevconfigpaths[j], configpaths[j]);
                }
            }
        }
        /* -- now process each group */
        for (i=0; i<configcount; i++) {
            (void) time (&ts); /* get a clean new timestamp with each group */
            elapsedtime = ts - configsys[i]->group->throtstats.statts;
            if ((configsys[i]->tunables.statinterval > 0) && 
                (configsys[i]->tunables.statinterval <= elapsedtime)) {
                configsys[i]->group->throtstats.statts = ts;
                memcpy((void*)mysys, (void*)configsys[i], sizeof(machine_t));

                /* we need to get the tunables each iteration to catch any 
                   changes.  We specify 0 for the pstore argument because we
                   don't need to hit the pstore since the driver should have
                   a copy in memory */
                if (isgroupdead(i)) {
                    memcpy ((void*)configsys[i], (void*)mysys, 
                            sizeof(machine_t));
                    continue;
                }                    
                lgnum = cfgtonum(i);
                FTD_CREATE_GROUP_NAME(group_name, lgnum);
                gettunables (group_name, 0, 1);
                /* get driver lg stats */
                statbuf.lg_num = lgnum;
                statbuf.dev_num = 0;
                statbuf.len = sizeof(ftd_stat_t);
                statbuf.addr = (char *)&lgdrvstats;
                if ((err = ioctl(masterfd, FTD_GET_GROUP_STATS, &statbuf)) < 0) {
                    fflush (stdout);
                    memcpy ((void*)configsys[i], (void*)mysys, 
                            sizeof(machine_t));
                    continue; /* go to next group */
                }
                if (!(err = getstats(mysys->group, lgnum, configpaths[i], elapsedtime))) {
                    fflush (stdout);
                    memcpy ((void*)configsys[i], (void*)mysys, 
                            sizeof(machine_t));
                    continue;
                    /* reporterr (ERRFAC, M_PERFERR, ERRCRIT); */
                }
                if (mysys->tunables.stattofileflag) {
                    (void)dumpstats(mysys);
                }
                (void) eval_throttles();
                memcpy ((void*)configsys[i], (void*)mysys, 
                        sizeof(machine_t));
            }
        }	
        fflush (stdout);
        /* check again in a second */
        select (NULL, NULL, NULL, NULL, &wakeupcall); 
    }     
} 
