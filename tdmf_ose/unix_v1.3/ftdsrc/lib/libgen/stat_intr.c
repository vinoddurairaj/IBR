/***************************************************************************
 * stat.c - FullTime Data PMD/RMD statistics dumper 
 *
 * (c) Copyright 1998 FullTime Software, Inc. All Rights Reserved
 *
 * This module implements the functionality for statistics calculations 
 *
 ***************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#if !defined(_AIX)
#if defined(HPUX) 
#if (SYSVERS >= 1100)
#include <math.h>
#else /* (SYSVERS >= 1100) */
#include <nan.h>
#endif /* (SYSVERS >= 1100) */
#else /* defined(HPUX) */
#include <nan.h>
#endif /* defined(HPUX) */
#endif /* !defined(_AIX) */

#if defined(HPUX)
#include <sys/pstat.h>
#elif defined (SOLARIS)
#include <sys/procfs.h>
#endif
 
#include "errors.h"
#include "config.h"
#include "pathnames.h"
#include "process.h"
#include "ftdio.h"
#include "stat_intr.h"
#include "common.h"
#ifdef TDMF_TRACE
FILE *dbgfd;
#endif

static struct stat statbuf[1];
static char perfname[FILE_PATH_LEN];
static time_t statts;

extern char *argv0;

extern sddisk_t *get_lg_dev(group_t*, int);

devstat_l g_statl;

/****************************************************************************
 * dumpstats -- writes the current statistics to the statdump file
 ***************************************************************************/
void
dumpstats (machine_t *sys)
{
    devstat_l *statl;

    if (sys->tunables.statinterval < 1) {
        return;
    }
    (void)dump_pmd_stats(sys);
    return; 
} /* dumpstats */

/****************************************************************************
 * flushstats -- called by PMD, RMD to save stats in driver
 ***************************************************************************/
void
flushstats(int force)
{
    time_t currentts;

    time(&currentts);

    if (force 
    || (mysys->tunables.statinterval > 0 && 
        currentts >= (statts + mysys->tunables.statinterval))) {
        (void)dumpstats(mysys);
    }
} /* flushstats */

/****************************************************************************
 * initstats -- opens the stats dump file
 ***************************************************************************/
void
initstats (machine_t *mysys)
{
    FILE* hdrfd;
    char fil[FILE_PATH_LEN];
    char fil2[FILE_PATH_LEN];
    char filhdr[FILE_PATH_LEN];
#if defined(HPUX)
    char procname[FILE_PATH_LEN];
#endif
    char *t;
    char *cfg_prefix;
    int l;

    if (mysys->tunables.statinterval < 1) {
        return;
    }
    if (0 != stat (PATH_VAR_OPT, statbuf)) {
        (void) mkdir (PATH_VAR_OPT, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    if (0 != stat (PATH_RUN_FILES, statbuf)) {
        (void) mkdir (PATH_RUN_FILES, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    l = strlen(mysys->configpath);
    t = mysys->configpath + (l - 7);
    if (ISPRIMARY(mysys)) {
        cfg_prefix = "p";
    } else {
        cfg_prefix = "s";
    }
    sprintf(filhdr, "%s/%s%3.3s.phd", PATH_RUN_FILES, cfg_prefix, t);
    sprintf(perfname, "%s/%s%3.3s.prf", PATH_RUN_FILES, cfg_prefix, t);
    sprintf(fil, "%s", perfname);
    sprintf(fil2, "%s.1", perfname);
    (void) unlink (fil2);
    (void) unlink (filhdr);
    (void) rename (fil, fil2);

    /* -- print a "table of contents" for performance file created */
    if ((hdrfd = fopen (filhdr, "w")) == NULL) {
        if (errno == EMFILE) {
            reporterr (ERRFAC, M_FILE, ERRCRIT, filhdr, strerror(errno));
        }
        return;
    }
    if (ISPRIMARY(mysys)) {
        fprintf (hdrfd, "\"ts\" x\n");
        fprintf (hdrfd, "\"dummy\" 0\n");
        fprintf (hdrfd, "\"Device\" n\n");
        fprintf (hdrfd, "\"Xfer Actual KBps\" y\n");
        fprintf (hdrfd, "\"Xfer Effective KBps\" y\n");
        fprintf (hdrfd, "\"Entries in BAB\" y\n");
        fprintf (hdrfd, "\"Sectors in BAB\" y\n");
        fprintf (hdrfd, "\"Percent Done\" y\n");
        fprintf (hdrfd, "\"Percent BAB Full\" y\n");
        fprintf (hdrfd, "\"Driver Mode\" 0\n");
        fprintf (hdrfd, "\"Read KBps\" y\n");
        fprintf (hdrfd, "\"Write KBps\" y\n");
        
    } else {
        fprintf (hdrfd, "\"ts\" x\n");
        fprintf (hdrfd, "\"dummy1\" 0\n");
        fprintf (hdrfd, "\"Device\" n\n");
        fprintf (hdrfd, "\"Xfer Actual KBps\" y\n");
        fprintf (hdrfd, "\"Xfer Effective KBps\" y\n");
        fprintf (hdrfd, "\"Entries per sec recvd\" y\n");
        fprintf (hdrfd, "\"Entry age recvd\" y\n");
        fprintf (hdrfd, "\"Entry age in journal\" y\n");
        fflush (hdrfd);
    }
    (void) fclose (hdrfd);
    if ((mysys->statfd = fopen(fil, "w")) == NULL) {
        if (errno == EMFILE) {
            reporterr (ERRFAC, M_FILE, ERRCRIT, filhdr, strerror(errno));
        }
    }
    /* DPRINTF("opened stats performance file %s\n", fil); */
    (void)time(&statts);
#if defined(HPUX)
    sprintf (procname, "%05d", (int) getpid());
    if ((mysys->procfd = open (procname, O_RDONLY)) == -1) {
        if (errno == EMFILE) {
            reporterr (ERRFAC, M_FILE, ERRCRIT, filhdr, strerror(errno));
        }
    }
#endif
    return;
} /* initstats */

/****************************************************************************
 * savestats -- called by PMD, RMD to save stats into driver memory
 ***************************************************************************/
int
savestats(int force)
{
    static ftd_lg_info_t lg;
    static ftd_lg_info_t *lgp = NULL;
    static char *devbuf = NULL;
    static int firsttime = 1;
    static int lgnum = -1;
    group_t *group;
    sddisk_t *sd;
    stat_buffer_t sb;
    devstat_t *devstatp, *statp;
    int rc;

    group = mysys->group;

    if (firsttime) {
        lgnum = cfgpathtonum(mysys->configpath);
        if (ISPRIMARY(mysys)) {
            initdvrstats();
            memset(&sb, 0, sizeof(stat_buffer_t));
            sb.lg_num = lgnum;
            sb.len = sizeof(ftd_lg_info_t);
            sb.addr = (ftd_uint64ptr_t)&lg;
            rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_GROUPS_INFO, &sb);
            if (rc != 0) {
#ifdef DEBUG_THROTTLE
                reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_GROUPS_INFO", 
                    strerror(errno));
#endif /* DEBUG_THROTTLE */
                return -1;
            }
            lgp = (ftd_lg_info_t*)&lg;
            devbuf = ftdmalloc(lgp->statsize);
        } else {
            (void)initstats(mysys);
        }
        firsttime = 0;
    }
    if (ISPRIMARY(mysys)) {
        for (sd = group->headsddisk; sd; sd = sd->n) {
            memset(&sb, 0, sizeof(stat_buffer_t));
            sb.lg_num = lgnum;
            sb.dev_num = sd->sd_rdev;
            sb.len = lgp->statsize;
            sb.addr = (ftd_uint64ptr_t)devbuf;
            rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_DEV_STATE_BUFFER, &sb);
            if (rc != 0) {
#ifdef DEBUG_THROTTLE
                reporterr(ERRFAC, M_DRVERR, ERRWARN, "GET_DEV_STATE_BUFFER", 
                    strerror(errno));
#endif /* DEBUG_THROTTLE */
                free(devbuf);
            }
            /* add our stats to stats returned from shared memory */
            devstatp = (devstat_t*)sb.addr;
            statp = &sd->stat;

            devstatp->devid = statp->devid;
            devstatp->a_tdatacnt += statp->a_tdatacnt;
            devstatp->a_datacnt += statp->a_datacnt;
            devstatp->e_tdatacnt += statp->e_tdatacnt;
            devstatp->e_datacnt += statp->e_datacnt;
            devstatp->entries += statp->entries;

            devstatp->rfshoffset = statp->rfshoffset;
            devstatp->rfshdelta = statp->rfshdelta;
#ifdef TDMF_TRACE
            fprintf(stderr,"\n*** device = %s\n",sd->sddevname);
            fprintf(stderr,"\n*** devstatp->devid = %d\n",devstatp->devid);
            fprintf(stderr,"\n*** devstatp->a_datacnt = %d\n",devstatp->a_datacnt);
            fprintf(stderr,"\n*** devstatp->e_datacnt = %d\n",devstatp->e_datacnt);
            fprintf(stderr,"\n*** devstatp->entries = %d\n",devstatp->entries);
            fprintf(stderr,"\n*** devstatp->rfshdelta = %d\n",devstatp->rfshdelta);
            fprintf(stderr,"\n*** devstatp->rfshoffset = %d\n",devstatp->rfshoffset);
#endif
            rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_DEV_STATE_BUFFER, &sb);
            if (rc != 0) {
#ifdef DEBUG_THROTTLE
                reporterr(ERRFAC, M_DRVERR, ERRWARN, "SET_DEV_STATE_BUFFER", 
                    strerror(errno));
#endif /* DEBUG_THROTTLE */
                free(devbuf);
            }
            /* reset internal counters */
            statp->a_tdatacnt = 0;
            statp->a_datacnt = 0;
            statp->e_tdatacnt = 0;
            statp->e_datacnt = 0;
            statp->entries = 0;
        }
/*
        free(devbuf);
*/
    } else {
        flushstats(force);
    }
    return;
} /* savestats */

/****************************************************************************
 * initdvrstats -- called by PMD to initailize stats in driver memory
 ***************************************************************************/
int
initdvrstats(void)
{
    group_t *group;
    sddisk_t *sd;
    stat_buffer_t sb;
    ftd_lg_info_t lg, *lgp;
    devstat_t devstatp[1];
    char *devbuf;
    int lgnum;
    int rc;

    group = mysys->group;
    lgnum = cfgpathtonum(mysys->configpath);

    memset(devstatp, 0, sizeof(devstat_t));
    memset(&sb, 0, sizeof(stat_buffer_t));

    sb.lg_num = lgnum;
    sb.len = sizeof(ftd_lg_info_t);
    sb.addr = (ftd_uint64ptr_t)&lg;

    rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_GROUPS_INFO, &sb);
    if (rc != 0) {
#ifdef DEBUG_THROTTLE
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_GROUPS_INFO", 
            strerror(errno));
#endif /* DEBUG_THROTTLE */
        return -1;
    }
    lgp = (ftd_lg_info_t*)sb.addr;
    devbuf = ftdcalloc(1, lgp->statsize);

    for (sd = group->headsddisk; sd; sd = sd->n) {
        sb.lg_num = lgnum;
        sb.dev_num = sd->sd_rdev;
        sb.len = lgp->statsize;
        sb.addr = (ftd_uint64ptr_t)devbuf;
        rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_DEV_STATE_BUFFER, &sb);
        if (rc != 0) {
#ifdef DEBUG_THROTTLE
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "SET_DEV_STATE_BUFFER", 
                strerror(errno));
#endif /* DEBUG_THROTTLE */
            free(devbuf);
            return -1;
        }
        memset(&sd->stat, 0, sizeof(devstat_t));
        sd->stat.devid = sd->devid;
    }
    free(devbuf);
    return 0;
} /* initdvrstats */

void
dump_pmd_stats (machine_t *sys)
{
    time_t statts;
    struct tm *tim;
    group_t *group;
    sddisk_t *sd;
    char fil[FILE_PATH_LEN];
    char fil2[FILE_PATH_LEN];
    char *t;
    int l;
    char *cfg_prefix;
    double pctbab;

    (void)time(&statts);
    tim = localtime(&statts);

    if (sys->statfd == (FILE*)NULL) {
        return;
    }
    fprintf(sys->statfd, "%02d:%02d:%02d ", tim->tm_hour, tim->tm_min,
            tim->tm_sec);

    group = sys->group;

    for (sd = group->headsddisk; sd; sd = sd->n) {
        if (sd->throtstats.actualkbps <= 0.0) sd->throtstats.actualkbps = 0.0;
        if (sd->throtstats.effectkbps <= 0.0) sd->throtstats.effectkbps = 0.0;
        if (sd->throtstats.entries < 0) sd->throtstats.entries = 0;
        if (sd->throtstats.sectors < 0) sd->throtstats.sectors = 0;
#if defined(HPUX)
        if (ISNANF(sd->throtstats.pctdone))
#elif defined(_AIX)
        if (finite(sd->throtstats.pctdone) == 0)
#else
        if (IsNANorINF(sd->throtstats.pctdone)) 
#endif
            sd->throtstats.pctdone = 0.0;
        if (sd->throtstats.pctdone <= 0.0) sd->throtstats.pctdone = 0.0;
        if (sd->throtstats.pctdone >100.0) sd->throtstats.pctdone = 0.0;
        pctbab = (double) sd->throtstats.pctbab;
#if defined(HPUX)
        if (ISNANF(pctbab))
#elif defined(_AIX)
        if (finite(pctbab) == 0)
#else
        if (IsNANorINF(pctbab))
#endif
            pctbab = 0.0;
        if (pctbab<0.0) pctbab = 0.0;
        if (pctbab>100.0) pctbab = 100.0;

        if (sd->throtstats.local_kbps_read <= 0.0)
            sd->throtstats.local_kbps_read = 0.0;
        if (sd->throtstats.local_kbps_written <= 0.0)
            sd->throtstats.local_kbps_written = 0.0;

        fprintf(sys->statfd, " || %s %6.2f %6.2f %d %d %6.2f %6.2f %d %6.2f %6.2f",
                sd->sddevname,
                sd->throtstats.actualkbps,
                sd->throtstats.effectkbps,
                sd->throtstats.entries,
                sd->throtstats.sectors,
                sd->throtstats.pctdone,
                pctbab,
                sys->group->throtstats.drvmode,
                sd->throtstats.local_kbps_read,
                sd->throtstats.local_kbps_written);
        
        fflush(sys->statfd);
    }
    fprintf(sys->statfd, "\n");
    fflush(sys->statfd);
    if (sys->tunables.maxstatfilesize < ftell(sys->statfd)) {
        (void) fclose (sys->statfd);
        l = strlen(mysys->configpath);
        t = sys->configpath + (l - 7);
        cfg_prefix = "p";
        sprintf(perfname, "%s/%s%3.3s.prf", PATH_RUN_FILES, cfg_prefix, t);
        sprintf(fil, "%s", perfname);
        sprintf(fil2, "%s.1", perfname);
        (void) unlink (fil2);
        (void) rename (fil, fil2);
        if ((sys->statfd = fopen(fil, "w")) == NULL) {
            if (errno == EMFILE) {
                reporterr (ERRFAC, M_FILE, ERRCRIT, fil, strerror(errno));
            }
        }
    }

  return;
} /* dump_pmd_stats */

/****************************************************************************
 * dump_rmd_stats -- writes the current statistics to the statdump file
 ***************************************************************************/
void
dump_rmd_stats (machine_t *sys)
{
    time_t statts;
    static time_t lastts;
    static int firsttime = 1;
    struct tm *tim;
    group_t *group;
    sddisk_t *sd;
    char fil[FILE_PATH_LEN];
    char fil2[FILE_PATH_LEN];
    char *cfg_prefix;
    char *t;
    float actualkbps;
    float effectkbps;
    float entries;
    int deltasec;
    int l;

    (void)time(&statts);
    if (firsttime) {
        lastts = statts - sys->tunables.statinterval;
        firsttime = 0;
    }
    deltasec = statts - lastts;

    if (deltasec < 10) {
        return;
    }
    tim = localtime(&statts);

    if (sys->statfd == (FILE*)NULL) {
        return;
    }
    fprintf(sys->statfd, "%02d:%02d:%02d ", tim->tm_hour, tim->tm_min,
            tim->tm_sec);

    group = sys->group;

    for (sd = group->headsddisk; sd; sd = sd->n) {
        /* calculate rmd stats */
        actualkbps = ((sd->stat.a_tdatacnt * 1.0) / (deltasec * 1.0)) / 1024.0;
        effectkbps = ((sd->stat.e_tdatacnt * 1.0) / (deltasec * 1.0)) / 1024.0;
        entries = ((sd->stat.entries * 1.0) / deltasec);
        fprintf(sys->statfd, " || %s %6.2f %6.2f %6.2f %d %d",
                sd->mirname,
                actualkbps,
                effectkbps,
                entries,
                sd->stat.entage,
                sd->stat.jrnentage);
        fflush(sys->statfd);
        sd->stat.a_tdatacnt = 0;
        sd->stat.e_tdatacnt = 0;
        sd->stat.entries = 0;
    }
    fprintf(sys->statfd, "\n");
    fflush(sys->statfd);
    if (sys->tunables.maxstatfilesize < ftell(sys->statfd)) {
        (void) fclose (sys->statfd);
        l = strlen(mysys->configpath);
        t = sys->configpath + (l - 7);
        cfg_prefix = "s";
        sprintf(perfname, "%s/%s%3.3s.prf", PATH_RUN_FILES, cfg_prefix, t);
        sprintf(fil, "%s", perfname);
        sprintf(fil2, "%s.1", perfname);
        (void) unlink (fil2);
        (void) rename (fil, fil2);
        if ((sys->statfd = fopen(fil, "w")) == NULL) {
            if (errno == EMFILE) {
                reporterr (ERRFAC, M_FILE, ERRCRIT, fil, strerror(errno));
            }
        }
    }
    lastts = statts;
    return;
} /* dump_rmd_stats */
