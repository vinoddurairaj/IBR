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
 * rsync.c - FullTime Data
 *
 * (c) Copyright 1998 FullTime Software, Inc. All Rights Reserved
 *
 * This module implements the functionality for rsync
 *
 ***************************************************************************/

#ifdef NEED_BIGINTS
#include "bigints.h"
#endif
#include "platform.h"

#include <stdlib.h>
#include <sys/types.h>
#ifdef SOLARIS
#include <sys/vtoc.h>
#endif
#if defined(linux)
#include <linux/fs.h>
#endif
#include "errors.h"
#include "config.h"
#include "network.h"
#include "pathnames.h"
#include "procinfo.h"
#include "process.h"
#include "common.h"
#include "ftdif.h"
#include "md5.h"
#include "ps_intr.h"
#include "aixcmn.h"
#include "stat_intr.h"

#include <limits.h>


ioqueue_t *fifo;
int rsynccnt;
int refresh_started = 0;
int refresh_oflow;

extern char *compress_buf;
extern int compress_buf_len;

extern char *paths;
extern int lgcnt;
extern int _pmd_state;
extern int _pmd_cpon;
extern int _pmd_state_change;
extern int _pmd_refresh;
extern char *_pmd_configpath;
extern const int exitfail;
extern char *argv0;

extern int g_rfddone;
extern int net_writable;

/* The following is the max and min backward adjustment we can give to an interrupted Full Refresh, when it restarts.
   Min: equivalent of 64K * 4KB-pages, i.e. 256 MBytes expressed in this platform's sector size;
   Max: equivalent of 256K * 4KB-pages, i.e. 1024 MBytes (1 GB) expressed in this platform's sector size;
   purpose of binding between these min and max is to avoid values unreasonably high for huge devices and unreasonably low
   for very small devices.
   NOTE, however, that a parameter to the adjustment function may force complete restart at 0%
*/
#define MIN_FREFRESH_ADJUSTMENT ((64 * 1024 * 4096) >> DEV_BSHIFT)      /* 64K * 4KB-pages == 256 MBytes */
#define MAX_FREFRESH_ADJUSTMENT ((256 * 1024 * 4096) >> DEV_BSHIFT)     /* 256K * 4KB-pages == 1024 MBytes (1 GB) */

// NOTE: it is at Full Refresh completion that we registe a group's devices in the stats file,
//       so that device sizes won't be registered more than once (in case of interrupted
//       and restarted Full Refreh).
#define	REGISTER_DEVICES_AT_FR_START 0
#define	REGISTER_DEVICES_AT_FR_END   1

int _pmd_verify_secondary;

/**
 * Used to record that we're resuming a checksum refresh.
 *
 * The variable is set within setmode() and we detect that a checksum refresh is being
 * resumed whenever we cleanly start the PMD or restart a refresh and that the pstore _MODE was "CHECKSUM_REFRESH".
 *
 * When a checksum refresh is resumed, we have to make sure that all bits will not be fully dirtied again.
 */
int _resuming_checksum_refresh;
int chksum_flag;

time_t ts;
struct tm *tim;

int  copy_errno;

static struct iovec *iov = NULL;
static int iovlen = 0;

extern ioqueue_t * q_init (void);
extern void q_delete (ioqueue_t *q);

static dirtybit_array_t db[1];
static int sendblock(rsync_t *rsync);
int initiate_bitmap_refresh(int lgnum, group_t* group);
int refresh (int fullrefreshphase);
int refresh_full (int fullrefresh_restart);
int backfresh (void);
int flush_delta(void);
int sendrsyncdevs(rsync_t *rsync, int state);
int sendrsyncstart(int state);
int send_rfd_inco(int lgnum);
int send_rfd_co(int lgnum, int state);

static size_t buf_alignment = 8192;
static char *buf = NULL;
static int buflen = 0;

static void
init_buffer(const size_t min)
{
    if (min > buflen) {
        if (buf)
            ftdfree(buf);
        buflen = min;
        buf = (char *)ftdmemalign(buflen, buf_alignment);
    }
}

/*
 * get dirtybit arrays, an ioctl per dev, for portability...
 */
int
dbarr_ioc(int fd, int ioc, dirtybit_array_t *db, ftd_dev_info_t *dev)
{
    int i;
    int rc;
    int *intp;
    int numdevs;
    dirtybit_array_kern_t dbk;

    numdevs = db->numdevs;
    intp = (int *)(unsigned long)db->dbbuf;
    for(i=0; i < numdevs ;i++) {
        dbk.dev = (ftd_uint32_t)db->devs[i];
        dbk.dbbuf =  (ftd_uint64ptr_t)(unsigned long)intp;
        switch(ioc) {
        case FTD_GET_HRDBS:
        case FTD_SET_HRDBS:
        case FTD_MERGE_HRDBS:
            dbk.dblen32 = dev[i].hrdbsize32;
            break;
        case FTD_GET_LRDBS:
        case FTD_SET_LRDBS:
            dbk.dblen32 = dev[i].lrdbsize32;
            break;
        }
        dbk.state_change = db->state_change;
            rc = FTD_IOCTL_CALL(fd, ioc, &dbk);
        if (rc != 0)
            return(rc);
	db->dblen32=dbk.dblen32;

        switch(ioc) {
        case FTD_GET_HRDBS:
        case FTD_SET_HRDBS:
        case FTD_MERGE_HRDBS:
            intp += dev[i].hrdbsize32;
            break;
        case FTD_GET_LRDBS:
        case FTD_SET_LRDBS:
            intp += dev[i].lrdbsize32;
            break;
        }
    }
    return(0);
}

/****************************************************************************
 * get_hires_dirtybits -- get all high res dirtybit maps for group
 ***************************************************************************/
static int
get_hires_dirtybits(group_t *group, int lgnum, ftd_stat_t *lgp)
{
    sddisk_t *sd;
    ftd_dev_info_t *dev, *devp;
    stat_buffer_t sb;
    int maskoff;
    int masklen;
    int bytes;
    int len;
    int rc;
    int i;

    dev = (ftd_dev_info_t*)ftdmalloc(lgp->ndevs*sizeof(ftd_dev_info_t));
    sb.lg_num = lgnum;
    sb.addr = (ftd_uint64ptr_t)(unsigned long)dev;
    rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_DEVICES_INFO, &sb);
    if (rc != 0) {
        free(dev);
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_DEVICES_INFO",
            strerror(errno));
        return -1;
    }
    db->devs = (dev_t *)ftdmalloc(lgp->ndevs*sizeof(dev_t));
    len = 0;
    for (i = 0; i < lgp->ndevs; i++) {
        db->devs[i] = dev[i].cdev;
        len += dev[i].hrdbsize32;
    }
    if (db->dbbuf == (ftd_uint64ptr_t)(unsigned long)NULL) {
        db->dbbuf = (ftd_uint64ptr_t)(unsigned long)ftdmalloc(len*sizeof(int));
    }
    db->numdevs = lgp->ndevs;
    db->state_change = 0;

    rc = dbarr_ioc(group->devfd, FTD_GET_HRDBS, db, dev);
    if (rc != 0) {
        copy_errno = errno;
        free(dev);
        free(db->devs);
        free((char *)(unsigned long)db->dbbuf);
        if( copy_errno == ETIME )
        {
          /* Timeout error: treat as warning level, not critical, and print additionnal message */
          reporterr( ERRFAC, M_DRVERR, ERRWARN, "GET_HRDBS", strerror(copy_errno) );
          reporterr( ERRFAC, M_PENDIO_TIMEOUT, ERRWARN );
        }
        else
        {
          /* Other errors are displayed as critical */
          reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_HRDBS", strerror(copy_errno));
        }
        return -1;
    }
    /* point our devices at dbarray */
    for (sd = group->headsddisk; sd; sd = sd->n) {
        /* find device in dbarray dev list */
        maskoff = 0;
        ftd_trace_flow(FTD_DBG_SYNC, "db->numdevs = %d\n", db->numdevs);
        for (i = 0, devp = dev; i < db->numdevs; i++) {
            ftd_trace_flow(FTD_DBG_SYNC,
                    "sd->rdev, devp->cdev = %08x, %08x\n", sd->sd_rdev, devp->cdev);
            if (devp->cdev == sd->sd_rdev) {
                sd->dm.len = devp->hrdbsize32*sizeof(int)*8;
                sd->dm.res = 1<<devp->hrdb_res;
                sd->dm.bits = devp->hrdb_numbits;
                sd->dm.bits_used = 0;
                sd->dm.mask = (unsigned char*)(unsigned long)db->dbbuf+(maskoff*sizeof(int));
                break;
            }
            maskoff += devp->hrdbsize32;
            devp++;
        }
        if (i == db->numdevs) {
            free(dev);
            free(db->devs);
            free((char *)(unsigned long)db->dbbuf);
            reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, (int) sd->sd_rdev);
            return -1;
        }

        ftd_trace_flow(FTD_DBG_SYNC,
                "get_highres: found device: %s\n", sd->devname);
        ftd_trace_flow(FTD_DBG_SYNC,
                "maskoff = %d\n", maskoff);
        ftd_trace_flow(FTD_DBG_SYNC,
                "devp->hrdbsize32 = %d\n", devp->hrdbsize32);
        ftd_trace_flow(FTD_DBG_SYNC,
                "devp->hrdb_res = %d\n", devp->hrdb_res);
        ftd_trace_flow(FTD_DBG_SYNC,
                "devp->disksize = %lld\n", devp->disksize); 
        ftd_trace_flow(FTD_DBG_SYNC,
                "devp->hrdb_numbits = %d\n", devp->hrdb_numbits);
/*
dump bit map
        bytes = sd->dm.len/8;
        for (i=0;i<bytes;i++) {
            printf("%02x",(unsigned char)sd->dm.mask[i]);
        }
        printf("\n");
*/
    }
    masklen = 0;
    devp = dev;
    for (i = 0; i < db->numdevs; i++) {
        masklen += (devp++)->hrdbsize32;
    }
    chksum_flag = 1;

    if(!_resuming_checksum_refresh)
    {
        for (i = 0; i < masklen-1; i++) {
            if (~*((unsigned int *)
                   ((unsigned char *)(unsigned long)db->dbbuf + (i*sizeof(unsigned int))))) {
                chksum_flag = 0;
                break;
            }
        }
    }

    free(dev);
    free(db->devs);

    return 0;
} /* get_hires_dirtybits */

/****************************************************************************
 * set_dirtybits -- set all dirtybit maps for group
 ***************************************************************************/
int
set_dirtybits(group_t *group, int lgnum, ftd_stat_t *lgp)
{
    sddisk_t *sd;
    ftd_dev_info_t *dev;
    stat_buffer_t sb;
    dirtybit_array_t ldb[1];
    int len;
    int rc;
    int i;

    dev = (ftd_dev_info_t*)ftdmalloc(lgp->ndevs*sizeof(ftd_dev_info_t));
    sb.lg_num = lgnum;
    sb.addr = (ftd_uint64ptr_t)(unsigned long)dev;
    rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_DEVICES_INFO, &sb);
    if (rc != 0) {
        free(dev);
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_DEVICES_INFO",
            strerror(errno));
        return -1;
    }
    memset(ldb, 0, sizeof(*ldb));
    ldb->devs = (dev_t *)ftdmalloc(lgp->ndevs*sizeof(dev_t));
    len = 0;
    for (i = 0; i < lgp->ndevs; i++) {
        ldb->devs[i] = dev[i].cdev;
        len += dev[i].hrdbsize32;
    }
    ldb->numdevs = lgp->ndevs;
    ldb->dblen32 = len;
    ldb->dbbuf = (ftd_uint64ptr_t)(unsigned long)ftdmalloc(len*sizeof(int));
    memset((unsigned char*)(unsigned long)ldb->dbbuf, 0xff, len*sizeof(int));
    rc = dbarr_ioc(group->devfd, FTD_SET_LRDBS, ldb, dev);
    if (rc != 0) {
        free(dev);
        free(ldb->devs);
        free((char *)(unsigned long)ldb->dbbuf);
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "SET_LRDBS", strerror(errno));
        return -1;
    }
    rc = dbarr_ioc(group->devfd, FTD_SET_HRDBS, ldb, dev);
    if (rc != 0) {
        free(dev);
        free(ldb->devs);
        free((char *)(unsigned long)ldb->dbbuf);
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "SET_HRDBS", strerror(errno));
        return -1;
    }
    free(dev);
    free(ldb->devs);
    free((char *)(unsigned long)ldb->dbbuf);

    return 0;
} /* set_dirtybits */

/****************************************************************************
 * empty_bab -- send any BAB entries to remote
 ***************************************************************************/
void
empty_bab(void)
{
    dispatch();

    return;
} /* empty_bab */

/****************************************************************************
 * rsyncinit_dev -- initialize rsync device
 ***************************************************************************/
int
rsyncinit_dev(sddisk_t *sd)
{
#ifdef SOLARIS
    struct dk_cinfo dkinfo;
    struct vtoc dkvtoc;
#endif
    stat_buffer_t sb;
    rsync_t *rsync;
    ftd_lg_info_t lginfo, *lgp;
    int lgnum;
    int cs_num;
    int rc;
    int openmode = O_RDONLY; /* All modes other than backfresh need to open for reading only. */
    
    rsync = &sd->rsync;
    if (rsync->devfd > 1) {
        (void) close(rsync->devfd);
        rsync->devfd = -1;
    }

    /*
     * WR 36420:: memory leak in pmd code, up to 500MB
     *            each used after 50 hours bab oflows
     * In case rsynccleanup() routine fails to clear
     * this memory we should free it up before 'memset'.
     * This was the main reason behind PMD memory leak.
     */
    if (rsync->deltamap.refp) {
    ftdfree((char *)rsync->deltamap.refp);
    rsync->deltamap.refp = NULL;
    }
    if (rsync->cs.digest) {
    ftdfree(rsync->cs.digest);
    rsync->cs.digest = NULL;
    }

    memset(rsync, 0, sizeof(rsync_t));

    rsync->devid = sd->devid;

    if( _net_bandwidth_analysis )
	{
        // In network bandwidth analysis, we simulate a Full Refresh without
		// physical device access nor driver or Pstore access.
	    rsync->size = sd->devsize = (u_longlong_t)0;
#if defined(linux)
	    rsync->blksize = 512;  // Block size default value (no physical access)
#endif
	    rsynccnt++;
	    return 0;
	}

    if (sd->devsize <= 0) {
       sd->devsize = (u_longlong_t)disksize(sd->devname);
       if (sd->devsize <= 0) {
            reporterr(ERRFAC, M_BADDEVSIZ, ERRCRIT, sd->devname);
            return -1;
        }
    }
    rsync->size = sd->devsize;

    if (_pmd_state == FTDRFD) {
        rsync->deltamap.cnt = 0;
        rsync->deltamap.refp =
            (ref_t*)ftdmalloc(sizeof(ref_t)*INIT_DELTA_SIZE);
        rsync->deltamap.size = INIT_DELTA_SIZE;
        cs_num = (mysys->tunables.chunksize/MINCHK)+2;
        rsync->cs.digestlen = 2*(cs_num*DIGESTSIZE);
        rsync->cs.digest = (char*)ftdmalloc(rsync->cs.digestlen);
        rsync->cs.cnt = 2;
        rsync->cs.num = cs_num;
    } else if (_pmd_state == FTDBFD) {
        sd->dm.res = MINCHK+1;
        rsync->deltamap.cnt = 0;
        rsync->deltamap.refp =
            (ref_t*)ftdmalloc(sizeof(ref_t)*INIT_DELTA_SIZE);
        rsync->deltamap.size = INIT_DELTA_SIZE;
        cs_num = (mysys->tunables.chunksize/MINCHK)+2;
        rsync->cs.digestlen = 2*(cs_num*DIGESTSIZE);
        rsync->cs.digest = (char*)ftdmalloc(rsync->cs.digestlen);
        rsync->cs.cnt = 2;
        rsync->cs.num = cs_num;
        
        openmode = O_RDWR; /* Backfresh is different than the other modes since we need to read and write. */
    } else if (_pmd_state == FTDRFDF) {
    }
    /* open the raw device */
#if defined(linux)
    // O_DIRECT is necessary when opening the source device to read it for refresh purposes.
    // If this flag isn't present, we can end up with the problems documented in WR #41530.
    openmode |= O_DIRECT;    
#endif
    
    if ((rsync->devfd = open(sd->devname, openmode)) == -1)
    {
        if (errno == EMFILE) {
            reporterr(ERRFAC, M_FILE, ERRCRIT,
                sd->devname, strerror(errno));
        }
        reporterr(ERRFAC, M_OPEN, ERRCRIT, sd->devname, strerror(errno));
        return -1;
    }
#if defined(linux)
{
    /*
     * need to fit the block boundary, or otherwise, it would hit
     * I/O error.
     */
    int blksize;
    if (FTD_IOCTL_CALL(rsync->devfd, BLKBSZGET, &blksize) < 0) {
        ftd_trace_flow(FTD_DBG_FLOW1,
                "Setting blocksize to 1024 bytes for device %s , errno %d\n", sd->devname, errno);
        /*
         * Hard coding blksize to 1024 might lead to problems on devices
         * where the block size is not 1024, but till all the appropriate
         * IOCTLs for querying devices for the block size are found,
         * this hard coding will have to do
         */
	blksize=1024;
    }
    if (blksize <= 0) {
        reporterr(ERRFAC, M_BADBLKBSZ, ERRCRIT, sd->devname);
    return -1;
    }

    rsync->size -= rsync->size % (blksize >> 9);
    rsync->blksize = blksize;

    (void)FTD_IOCTL_CALL(rsync->devfd, BLKFLSBUF, 0);
}
#endif

#ifdef SOLARIS
    /* check for meta-device */
#ifdef PURE
    memset(&dkinfo, 0, sizeof(dkinfo));
    memset(&dkvtoc, 0, sizeof(dkvtoc));
#endif
    if (FTD_IOCTL_CALL(rsync->devfd, DKIOCINFO, &dkinfo) < 0) {
        return -1;
    }
    if (FTD_IOCTL_CALL(rsync->devfd, DKIOCGVTOC, &dkvtoc) >= 0) {
        if (dkinfo.dki_ctype != DKC_MD
                && dkvtoc.v_part[dkinfo.dki_partition].p_start == 0) {
                        ftd_trace_flow(FTD_DBG_ERROR,
                                "Sector 0 write dissallowed for device: %s\n", sd->devname);
                sd->no0write = 1;
        }
    }
#endif
    ftd_trace_flow(FTD_DBG_FLOW1,
            "rsync->devname = %s, devfd = %d, offset = %d, size = %d\n",
            sd->devname, rsync->devfd, rsync->offset, rsync->size);

    /* tell RMD about device */
    if (_pmd_state == FTDBFD) {
        sendrsyncdevs(rsync, FTDBFD);
    }
    rsynccnt++;

    return 0;
} /* rsyncinit_dev */

/****************************************************************************
 * chk_baboverflow:
 *   check if driver transition to tracking while PMD is in smart refresh
 *
 * call only when PMD is in FTDRFD (refresh) or FTDPMD (normal)
 *   return 1: driver in tracking (FTD_MODE_TRACKING)
 *          0: driver is in refresh (FTD_MODE_REFRESH)
 *       else: other error
 ****************************************************************************
 */
int chk_baboverflow (group_t *group)
{
    sddisk_t *sd;
    stat_buffer_t sb;
    ftd_stat_t lgstat;
    ftd_state_t stb;
    int lgnum;
    int rc;
    char ps_name[MAXPATHLEN];
    char group_name[MAXPATHLEN];

    if (_pmd_state != FTDRFD && _pmd_state != FTDPMD) {
    /* should not be called in mode other than FTDRFD, do not
         * change any state.
         */
        return 0;
    }

    lgnum = cfgpathtonum(mysys->configpath);
    FTD_CREATE_GROUP_NAME(group_name, lgnum);

    /* get group info */
    memset(&sb, 0, sizeof(stat_buffer_t));
    sb.lg_num = lgnum;
    sb.len = sizeof(ftd_stat_t);
    sb.addr = (ftd_uint64ptr_t)(unsigned long)&lgstat;
    rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_GROUP_STATS, &sb);
    if (rc != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_GROUP_STATS",
            strerror(errno));
        return -1;
    }

    /* Has BAB overflow occurred during smart refresh for this group? */
    switch(lgstat.state) {
    case FTD_MODE_TRACKING:
    /* case FTD_MODE_CHECKPOINT_JLESS: */
         return 1;
    default:
         return 0;
    }
}

/****************************************************************************
 * sendrfdstart -- send INCO to RMD
 ***************************************************************************/
int
sendrfdstart(void)
{
    headpack_t header[1];
    headpack32_t header32[1];

    memset(header, 0, sizeof(headpack_t));
    memset(header32, 0, sizeof(headpack32_t));
    header->magicvalue = MAGICHDR;
    header->cmd = CMDMSGINCO;
    header->ackwanted = 0;
    if (!rmd64)
    {
	converthdrfrom64to32 (header, header32);
    	if (-1 == writesock(mysys->sock, (char*)header32, sizeof(headpack32_t))) {
        	EXIT (exitfail);
    	}
    }
    else
    {
    	if (-1 == writesock(mysys->sock, (char*)header, sizeof(headpack_t))) {
        	EXIT (exitfail);
    	}
    }
    return 0;
} /* sendrfdstart */

/****************************************************************************
 * sendrfdend -- send CO to RMD
 ***************************************************************************/
int
sendrfdend(void)
{
    headpack_t header[1];
    headpack32_t header32[1];

    memset(header, 0, sizeof(headpack_t));
    memset(header32, 0, sizeof(headpack32_t));
    header->magicvalue = MAGICHDR;
    header->cmd = CMDMSGCO;
    header->ackwanted = 0;
    if (!rmd64)
    {
        converthdrfrom64to32 (header, header32);
    	if (-1 == writesock(mysys->sock, (char*)header32, sizeof(headpack32_t))) {
        	EXIT (exitfail);
    	}
    }
    else
    {
    	if (-1 == writesock(mysys->sock, (char*)header, sizeof(headpack_t))) {
        	EXIT (exitfail);
    	}
    }
    return 0;
} /* sendrfdend */

/****************************************************************************
 * rsyncinit -- initialize rsync
 * Passed -r option as flag
 ***************************************************************************/
int
rsyncinit(group_t *group, int fullrefreshphase, int fullrefresh_restart)
{
    sddisk_t *sd;
    stat_buffer_t sb;
    ftd_stat_t lgstat;
    ftd_state_t stb;
    static int firsttime = 1;
    int lgnum;
    int rc;
    char ps_name[MAXPATHLEN];
    char group_name[MAXPATHLEN];
#if defined(linux)
    int fd;
    ftd_param_t  trk_pend_info;
 
    memset(&trk_pend_info, 0, sizeof(ftd_param_t));
#endif
    
    lgnum = cfgpathtonum(mysys->configpath);
    FTD_CREATE_GROUP_NAME(group_name, lgnum);

    refresh_started = 0;
    refresh_oflow = 0;
    chksum_flag = 0;

    if (firsttime) {
        memset(db, 0, sizeof(dirtybit_array_t));
        firsttime = 0;
    }

    if( _net_bandwidth_analysis )
	{
        // In network bandwidth analysis, we simulate a Full Refresh without
		// physical device access nor driver or Pstore access.
        sendrsyncstart(_pmd_state);
        refresh_started = 1;
	    rsynccnt = 0;
	    for (sd = group->headsddisk; sd; sd = sd->n) {
	        rsyncinit_dev(sd);
	    }
	    if ((fifo = (ioqueue_t*)q_init()) == NULL) {
	        return FTD_ERR;
	    }
	    /* dump initial stats */
	    savestats(1, _net_bandwidth_analysis);

	    return FTD_OK;
	}

    /* get group info */
    memset(&sb, 0, sizeof(stat_buffer_t));
    sb.lg_num = lgnum;
    sb.len = sizeof(ftd_stat_t);
    sb.addr = (ftd_uint64ptr_t)(unsigned long)&lgstat;
    rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_GROUP_STATS, &sb);
    if (rc != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_GROUP_STATS",
            strerror(errno));
        return FTD_ERR;
    }
    if (_pmd_state == FTDRFD) {
        switch(lgstat.state) {
        case FTD_MODE_NORMAL:
        case FTD_MODE_PASSTHRU:
            /* kick driver into REFRESH mode */
            stb.lg_num = lgnum;
            stb.state = FTD_MODE_REFRESH;
            rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_GROUP_STATE, &stb);
            if (rc != 0)
            {
                copy_errno = errno;
                /* If called with fullrefreshphase argument == 0 and error occurs,
                   the PMD will exit and we go to Tracking mode. If due to pending IOs completions
                   timeout, log additionnal message to indicate driver saturation. */
                if( (copy_errno == ETIME) && (fullrefreshphase == 0) )
                {
                  /* Timeout error: treat as warning level, not critical, and print additionnal message */
                  reporterr( ERRFAC, M_DRVERR, ERRWARN, "SET_GROUP_STATE", strerror(copy_errno) );
                  reporterr( ERRFAC, M_PENDIO_TIMEOUT, ERRWARN );
                }
                else
                {
                  /* Other errors are displayed as critical */
                  reporterr(ERRFAC, M_DRVERR, ERRCRIT, "SET_GROUP_STATE", strerror(copy_errno));
                }
                return FTD_ERR;
            }
            ps_set_group_state(mysys->pstore, group_name, stb.state);
            
            if( lgstat.state == FTD_MODE_PASSTHRU )
            {
                // Reset the RPO variables for RPO reporting: no valid RPO exists
                group->QualityOfService.OldestInconsistentIOTimestamp = 0;
                group->QualityOfService.LastConsistentIOTimestamp = 0;
                give_RPO_stats_to_driver( group );
            }

            if (lgstat.state == FTD_MODE_NORMAL) {
                if (_pmd_verify_secondary) {
                    /* refresh -c */

                    if (!_resuming_checksum_refresh)
                    {
                        if ((rc = set_dirtybits(group, lgnum, &lgstat)) == -1) {
                            return FTD_ERR;
                        }
                    }
                    _pmd_verify_secondary = 0;
                }
            }
            /* all bits will be set */
            if ((rc = get_hires_dirtybits(group, lgnum, &lgstat)) == -1) {
                return FTD_ERR;
            }
            break;
        case FTD_MODE_REFRESH:
            /* driver was previously in refresh mode */
            if ((rc = get_hires_dirtybits(group, lgnum, &lgstat)) == -1) {
                return FTD_ERR;
            }
            break;
        case FTD_MODE_FULLREFRESH:
            /* driver was previously in full refresh mode, start smart refresh phase */
            /* kick driver into REFRESH mode */
            stb.lg_num = lgnum;
            stb.state = FTD_MODE_REFRESH;
            rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_GROUP_STATE, &stb);
            if (rc != 0)
            {
                copy_errno = errno;
                /* If called with fullrefreshphase argument == 0 and error occurs,
                   the PMD will exit and we go to Tracking mode. If due to pending IOs completions
                   timeout, log additionnal message to indicate driver saturation. */
                if( (copy_errno == ETIME) && (fullrefreshphase == 0) )
                {
                  /* Timeout error: treat as warning level, not critical, and print additionnal message */
                  reporterr( ERRFAC, M_DRVERR, ERRWARN, "SET_GROUP_STATE", strerror(copy_errno) );
                  reporterr( ERRFAC, M_PENDIO_TIMEOUT, ERRWARN );
                }
                else
                {
                  /* Other errors are displayed as critical */
                  reporterr(ERRFAC, M_DRVERR, ERRCRIT, "SET_GROUP_STATE", strerror(copy_errno));
                }
                return FTD_ERR;
            }
            ps_set_group_state(mysys->pstore, group_name, stb.state);
            if ((rc = get_hires_dirtybits(group, lgnum, &lgstat)) == -1) {
                return FTD_ERR;
            }
            break;
        case FTD_MODE_TRACKING:
            empty_bab();
            /* kick driver into REFRESH mode */
            stb.lg_num = lgnum;
            stb.state = FTD_MODE_REFRESH;
            rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_GROUP_STATE, &stb);
            if (rc != 0)
            {
                copy_errno = errno;
                /* If called with fullrefreshphase argument == 0 and error occurs,
                   the PMD will exit and we go to / stay in Tracking mode. If due to pending IOs completions
                   timeout, log additionnal message to indicate driver saturation. */
                if( (copy_errno == ETIME) && (fullrefreshphase == 0) )
                {
                  /* Timeout error: treat as warning level, not critical, and print additionnal message */
                  reporterr( ERRFAC, M_DRVERR, ERRWARN, "SET_GROUP_STATE", strerror(copy_errno) );
                  reporterr( ERRFAC, M_PENDIO_TIMEOUT, ERRWARN );
                }
                else
                {
                  /* Other errors are displayed as critical */
                  reporterr(ERRFAC, M_DRVERR, ERRCRIT, "SET_GROUP_STATE", strerror(copy_errno));
                }
                return FTD_ERR;
            }
            ps_set_group_state(mysys->pstore, group_name, stb.state);

            if (_pmd_verify_secondary) {
                /* refresh -c */

                if (!_resuming_checksum_refresh)
                {
                    if ((rc = set_dirtybits(group, lgnum, &lgstat)) == -1) {
                        return FTD_ERR;
                    }
                }
                _pmd_verify_secondary = 0;
            }
            if (get_hires_dirtybits(group, lgnum, &lgstat) == -1) {
                return FTD_ERR;
            }
            /*
             * set this flag so that if we overflow while emptying the
             * BAB the correct thing happens. The state machine in
             * this app is in dire need of some work.
             */
            refresh_oflow = 1;
            break;
        case FTD_MODE_CHECKPOINT_JLESS:
            /*
             * We will not execute refresh since RMD side is in checkpoint and
             * no journal file to hold the resync data.
             */
            ftd_trace_flow(FTD_DBG_FLOW1,
                    "FTDRFD driver(cpon+jless) jless(%d) _pmd_cpon(%d)\n",
                    GET_LG_JLESS(mysys), _pmd_cpon);
            if (!GET_LG_JLESS(mysys) || !_pmd_cpon) {
                ftd_trace_flow(FTD_DBG_FLOW1, "set driver state to tracking\n");
                stb.lg_num = lgnum;
                stb.state = FTD_MODE_TRACKING;
                rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_GROUP_STATE, &stb);
                if (rc != 0) {
                    reporterr(ERRFAC, M_DRVERR, ERRCRIT, "SET_GROUP_STATE",
                            strerror(errno));
                    return FTD_ERR;
                }
            }

            return FTD_NO_OP;
        default:
            reporterr(ERRFAC, M_PSBADSTATE, ERRCRIT, lgstat.state);
            return FTD_ERR;
        }

        if (fullrefreshphase == 0) {
            /*
             *Changed the existing design wherein INCO/CO sentinels are sent
             *through BAB to a new design wherein INCO/CO sentinels are sent directly
             *through socket. This change was implemented to avoid the problem of PMD
             *not being able to put INCO/CO into the BAB because of BAB overflow.
             */
            if (!rmd64) {
                send_rfd_inco(lgnum);
                /* the INCO marker needs sent/ACK'd */
                empty_bab();
            } else {
            	sendrfdstart();
            	empty_bab();
            }
        } else {
            sendrsyncstart(_pmd_state);
            empty_bab();
        }
        refresh_started = 1;
    } else if (_pmd_state == FTDRFDF) {
        if (lgstat.state == FTD_MODE_CHECKPOINT_JLESS) {
            /*
             * We will not execute full refresh since RMD side is in checkpoint
             * and no journal file to hold the resync data.
             */
            ftd_trace_flow(FTD_DBG_FLOW1,
                    "FTDRFDF driver(cpon+jless) jless(%d) _pmd_cpon(%d)\n",
                    GET_LG_JLESS(mysys), _pmd_cpon);
            if (!GET_LG_JLESS(mysys) || !_pmd_cpon) {
                ftd_trace_flow(FTD_DBG_FLOW1, "set driver state to tracking\n");
                stb.lg_num = lgnum;
                stb.state = FTD_MODE_TRACKING;
                rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_GROUP_STATE, &stb);
                if (rc != 0) {
                    reporterr(ERRFAC, M_DRVERR, ERRCRIT, "SET_GROUP_STATE",
                        strerror(errno));
                    return FTD_ERR;
                }
            }

            return FTD_NO_OP;
        }
        /* kick driver into refresh mode */
        stb.lg_num = lgnum;
        stb.state = FTD_MODE_FULLREFRESH;

        /* For full refresh restart, set the stb.fullrefresh_restart to 1,
         * or for normal full refresh, set the stb.fullrefresh_restart to 0 */
        stb.fullrefresh_restart = fullrefresh_restart;

        rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_GROUP_STATE, &stb);
        if (rc != 0) {
            copy_errno = errno;
            if( copy_errno == ETIME )
            {
               /* Timeout error: treat as warning level, not critical, and print additionnal message */
               reporterr(ERRFAC, M_DRVERR, ERRWARN, "SET_GROUP_STATE", strerror(copy_errno));
               reporterr( ERRFAC, M_PENDIO_TIMEOUT, ERRWARN );
            }
            else
            {
               /* Other errors are displayed as critical */
               reporterr(ERRFAC, M_DRVERR, ERRCRIT, "SET_GROUP_STATE", strerror(copy_errno));
            }
            return FTD_ERR;
        }
        ps_set_group_state(mysys->pstore, group_name, stb.state);

        sendrsyncstart(_pmd_state);
        if (flush_bab(mysys->ctlfd, lgnum) < 0) {
            /*
             * can't flush so empty - this will cause
             * us to send redundant blocks (ie. bab + all dev blocks)
             */
            empty_bab();
        }
        refresh_started = 1;
    } else if (_pmd_state == FTDBFD) {
        sendrsyncstart(_pmd_state);
    }
    rsynccnt = 0;
    for (sd = group->headsddisk; sd; sd = sd->n) {
        rsyncinit_dev(sd);
    }
    if ((fifo = (ioqueue_t*)q_init()) == NULL) {
        return FTD_ERR;
    }
    /* dump initial stats to driver */
    savestats(1, _net_bandwidth_analysis);

    return FTD_OK;
} /* rsyncinit */

/****************************************************************************
 * rsynccleanup -- cleanup rsync
 ***************************************************************************/
int
rsynccleanup(int mode)
{
    group_t *group;
    sddisk_t *sd;
    stat_buffer_t sb;
    ftd_lg_info_t lginfo;
    ftd_state_t stb;
    char group_name[MAXPATHLEN];
    char ps_name[MAXPATHLEN];
    char *devbuf;
    char *modestr;
    int lgnum;
    int rc;

    q_delete(fifo);

    group = mysys->group;
    lgnum = cfgpathtonum(mysys->configpath);

    if (db->dbbuf) {
        /* free the bit map */
        free((char *)(unsigned long)db->dbbuf);
        db->dbbuf = (ftd_uint64ptr_t)(unsigned long)NULL;
    }
    memset(&sb, 0, sizeof(stat_buffer_t));
    sb.lg_num = lgnum;
    sb.len = sizeof(ftd_lg_info_t);
    sb.addr = (ftd_uint64ptr_t)(unsigned long)&lginfo;
    rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_GROUPS_INFO, &sb);
    if (rc != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_GROUPS_INFO",
            strerror(errno));
        return -1;
    }
    devbuf = ftdcalloc(1, lginfo.statsize);
    /* reset driver stats */
    for (sd = group->headsddisk; sd; sd = sd->n) {
        /* clear out driver stats */
        sb.lg_num = lgnum;
        sb.dev_num = sd->sd_rdev;
        sb.len = lginfo.statsize;
        sb.addr = (ftd_uint64ptr_t)(unsigned long)devbuf;
        rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_DEV_STATE_BUFFER, &sb);
        if (rc != 0) {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "SET_DEV_STATE_BUFFER",
                strerror(errno));
            free(devbuf);
            return -1;
        }
        /* close raw disk device */
        if (sd->rsync.devfd > 0) {
            close(sd->rsync.devfd);
        }
        /* free rsync buffers */
        if (sd->rsync.cs.digest) {
            ftdfree(sd->rsync.cs.digest);
            sd->rsync.cs.digest = NULL;
        }
        if (sd->rsync.deltamap.refp) {
            ftdfree((char *)sd->rsync.deltamap.refp);
            sd->rsync.deltamap.refp = NULL;
        }
        sd->stat.rfshoffset = 0;
        sd->stat.rfshdelta = 0;
        memset(&sd->rsync, 0, sizeof(rsync_t));
    }
    free(devbuf);

    /* free global iov structure */
    if (iovlen) {
        free(iov);
        iov = NULL;
        iovlen = 0;
    }
    /* update the LRDBs for the group */
    update_lrdb(lgnum);

    /*  New full refresh behavior - don't go  into normal mode,
        still have to do smart refresh phase */
    if (_pmd_state!=FTDRFDF) {
        /* reset driver mode */
        stb.lg_num = lgnum;
        stb.state = FTD_MODE_NORMAL;
        rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_GROUP_STATE, &stb);
        if (rc != 0 && errno != EINVAL) {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "SET_GROUP_STATE",
                strerror(errno));
            free(devbuf);
            return -1;
        }
        /* reset pstore mode */
        FTD_CREATE_GROUP_NAME(group_name, lgnum);
        modestr = "NORMAL";
        rc = ps_set_group_key_value(mysys->pstore, group_name, "_MODE:", modestr);
        if (rc != PS_OK) {
            reporterr(ERRFAC, M_PSWTDATTR, ERRWARN, group_name, ps_name);
            return -1;
        }
        ps_set_group_state(mysys->pstore, group_name, stb.state);
    }
    return 0;
} /* rsynccleanup */

/****************************************************************************
 * setblock -- set a local buffer to send to the RMD in network analysis mode
 * note: the buffer is filled with a byte that gets incremented at every call
 *       except that we don't fill with 0x00s (from 0x01 to 0xff)
 ***************************************************************************/
int
setblock(rsync_t *rsync)
{
    int length;
	static char pattern = 0;

    length = (rsync->length << DEV_BSHIFT);
    if( ++pattern == 0 )
	    pattern = 1;
	memset(rsync->data, pattern, length);

    return length;
} /* readblock */

/****************************************************************************
 * readblock -- read a block from the data device (PMD) mirror (RMD)
 ***************************************************************************/
int
readblock(rsync_t *rsync)
{
    sddisk_t *sd;
    u_longlong_t offset;
    u_longlong_t rc;
    int length;
    int devbytes;

    offset = rsync->offset;
    offset <<= DEV_BSHIFT;
    length = (rsync->length << DEV_BSHIFT);

    if ((sd = getdevice(rsync)) == NULL) {
        reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, rsync->devid);
        EXIT(EXITANDDIE);
    }

    ftd_trace_flow(FTD_DBG_SYNC,
            "\n*** [%s] reading %d bytes @ offset %llu\n",
            argv0, length, offset);

    if (llseek(rsync->devfd, offset, SEEK_SET) == -1) {
        if (ISPRIMARY(mysys)) {
            reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
                argv0, sd->devname, offset, strerror(errno));
        } else {
            reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
                argv0, sd->mirname, offset, strerror(errno));
        }
        EXIT(EXITANDDIE);
    }
    if ((devbytes = ftdread(rsync->devfd, rsync->data, length)) == -1) {
        if (ISPRIMARY(mysys)) {
            reporterr(ERRFAC, M_READERR1, ERRCRIT,
                sd->devname, rsync->data, offset, length, strerror(errno));
        } else {
            reporterr(ERRFAC, M_READERR1, ERRCRIT,
                sd->mirname, rsync->data, offset, length, strerror(errno));
        }
        EXIT(EXITANDDIE);
    }

    return devbytes;
} /* readblock */

/****************************************************************************
 * dirtyseg -- inspect a device segment for 'dirty'ness
 ***************************************************************************/
int
dirtyseg(unsigned char *mask, u_longlong_t offset, u_longlong_t length, int res)
{
    u_longlong_t startbit;
    u_longlong_t endbit;
    u_longlong_t endbyte;
    u_longlong_t i;
    unsigned int *wp;
    int rc;

    ftd_trace_flow(FTD_DBG_FLOW3,
            "res, offset, length %d, [%llu-%llu]\n",
            res, offset, length);

    rc = 0;

    if (res == 0) {
        return -1;
    }

    /* scan this 'segment' of bits */
    endbyte = (u_longlong_t)(offset+length-1);
    startbit = (u_longlong_t)(offset/res);
    endbit = (u_longlong_t)(endbyte/res);

    ftd_trace_flow(FTD_DBG_FLOW3,
            "offset, endbyte, startbit, endbit = %llu, %llu, %llu, %llu\n",
            offset, endbyte, startbit, endbit);

    for (i = startbit; i <= endbit; i++) {
        /* find the word */
        ftd_trace_flow(FTD_DBG_FLOW3,
                "i, word = %llu, %llu\n",i, (i>>5));
        wp = (unsigned int*)((unsigned int*)mask+(i>>5));

        /* test the bit */
        if((*wp) & (0x00000001 << (i % (sizeof(*wp) * 8)))) {
            rc = 1;
            break;
        }

    }
    ftd_trace_flow(FTD_DBG_FLOW3, "returning %d\n", rc);

    return rc;
} /* dirtyseg */

/****************************************************************************
 * rsyncwrite_bfd -- backfresh - write to data device
 ***************************************************************************/
int
rsyncwrite_bfd(rsync_t *lrsync)
{
#ifndef SUPPORT_BACKFRESH
    reporterr(ERRFAC, M_BKFRSH_NOTAVAIL, ERRINFO);
    return( -1 );
#endif
    sddisk_t *sd;
    char *devname;
    u_longlong_t offset;
    u_longlong_t length;
    u_longlong_t vsize;
    int len32;
    int rc;
    int devbytes;
    int fd;

    if ((sd = getdevice(lrsync)) == NULL) {
        reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, lrsync->devid);
        EXIT(EXITANDDIE);
    }
    fd = sd->rsync.devfd;
    devname = sd->devname;

    devbytes = 0;
    offset = lrsync->offset;
    offset <<= DEV_BSHIFT;
    vsize = sd->devsize * (u_longlong_t) DEV_BSIZE;
    length = lrsync->datalen;

    DPRINTF("\n*** rsyncwrite_bfd: writing %llu bytes @ offset: %llu\n",         length, offset);
    if (offset == 0 && sd->no0write) {
        DPRINTF("\n*** skipping sector 0 write for device: %s\n",            sd->devname);
        offset += DEV_BSIZE;
        length -= DEV_BSIZE;
        lrsync->data += DEV_BSIZE;
    }
    /* simulate a successful write if offset is beyond end of device */
    if (offset >= vsize) {
        return ((int)length);
    }
    if (llseek(fd, offset, SEEK_SET) == -1) {
        reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
            argv0, devname, offset, strerror(errno));
        EXIT(EXITANDDIE);
    }
    /* truncate writes that extend beyond the end of the device */
    if ((offset+length) > vsize) {
        length = vsize - offset;
    }
    len32 = length;
    rc = ftdwrite(fd, lrsync->data, len32);
    if (rc != len32) {
       reporterr(ERRFAC, M_WRITEERR, ERRCRIT,
            argv0, devname, lrsync->devid,
            offset, length, strerror(errno));
        EXIT(EXITANDDIE);
    }
    devbytes = rc;
    DPRINTF("\n@@@ rsyncwrite_bfd: returning %d\n",devbytes);

    return devbytes;
} /* rsyncwrite_bfd */

/****************************************************************************
 * flush_delta -- flush the compiled list of deltas to the network
 ***************************************************************************/
int
flush_delta(void)
{
    rsync_t lrsync[1];
    sddisk_t *sd;
    rsync_t *rsync;
    int rc;
    int i;

    for (sd = mysys->group->headsddisk; sd; sd = sd->n) {
        rsync = &sd->rsync;
        memcpy(lrsync, rsync, sizeof(rsync_t));

        for (i = 0; i < rsync->deltamap.cnt; i++) {
            /* send local data to remote */
            lrsync->offset = rsync->deltamap.refp[i].offset;
            lrsync->length = rsync->deltamap.refp[i].length;
            lrsync->datalen = lrsync->length << DEV_BSHIFT;
            lrsync->delta += lrsync->length;

            DPRINTF("\n*** flush_delta: device: %s, offset: %d length: %d\n",sd->sddevname, lrsync->offset,  lrsync->length);

            if (readblock(lrsync) == -1) {
                return -1;
            }
            sendblock(lrsync);
        }
        rsync->deltamap.cnt = 0;
        if (ISPRIMARY(mysys)) {
            eval_netspeed();
        }
/*
        savestats(0, _net_bandwidth_analysis);
*/
    }

    return 0;
} /* flush_delta */

/****************************************************************************
 * add_delta_to_list -- save offset/length of delta block
 ***************************************************************************/
int
add_delta_to_list(rsync_t *rsync, ref_t *ref)
{

    if (++rsync->deltamap.cnt > rsync->deltamap.size) {
        /* increase list size */
        rsync->deltamap.size *= 2;
        rsync->deltamap.refp =
            (ref_t*)ftdrealloc((char*)rsync->deltamap.refp,
                rsync->deltamap.size*sizeof(ref_t));
    }
    memcpy(&rsync->deltamap.refp[rsync->deltamap.cnt-1],
        ref, sizeof(ref_t));

    return 0;
} /* add_delta_to_list */

/****************************************************************************
 * chksumdiff -- compare local/remote checksums and create delta buffer
 ***************************************************************************/
int
chksumdiff(void)
{
    sddisk_t *sd;
    rsync_t *rsync;
    rsync_t *ackrsync;
    ref_t ref[1];
    unsigned char *rptr;
    unsigned char *lptr;
    u_longlong_t length;
    u_longlong_t offset;
    u_longlong_t bytesleft;
    int byteoff;
    int digestlen;
    int res;
    int num;
    int i;
    int j;

    for (sd = mysys->group->headsddisk; sd; sd = sd->n) {
        rsync = &sd->rsync;
        ackrsync = &sd->ackrsync;
        if (ackrsync->cs.seglen == 0) {
            ackrsync->cs.seglen = -1;
            continue;
        }
        digestlen = ackrsync->cs.num*DIGESTSIZE;
        num = ackrsync->cs.num;
        res = MINCHK;

        lptr = (unsigned char*)ackrsync->cs.digest;
        rptr = (unsigned char*)ackrsync->cs.digest+digestlen;

        length = ackrsync->cs.seglen;
        ackrsync->cs.seglen = 0;
        offset = ackrsync->cs.segoff;

        /* create delta data buffer to send to remote */
        for (i = 0; i < num; i++) {
            byteoff = i*res;
            bytesleft = (length < res ? length: res);
/*
#ifdef TDMF_TRACE
            fprintf(stderr,"\n*** [%d]  local digest: ",i);
            for (j = 0; j < DIGESTSIZE; j++) {
                fprintf(stderr,"%02x",lptr[j]);
            }
            fprintf(stderr,"\n*** [%d] remote digest: ", i);
            for (j = 0; j < DIGESTSIZE; j++) {
                fprintf(stderr,"%02x",rptr[j]);
            }
#endif
*/
            if (memcmp(lptr, rptr, DIGESTSIZE)) {
                /* checksums are different - bump delta length */
                ackrsync->cs.seglen += bytesleft;
            } else {
                /* checksums are the same */
                if (ackrsync->cs.seglen) {
                    /* save offset, length - in sectors */
                    ref->offset = offset;
                    ref->length = ackrsync->cs.seglen >> DEV_BSHIFT;
                    add_delta_to_list(rsync, ref);
                    ackrsync->cs.seglen = 0;
                }
                /* bump offset - past this identical checksum */
                offset = ackrsync->cs.segoff +
                    ((byteoff+bytesleft) >> DEV_BSHIFT);
            }
            lptr += DIGESTSIZE;
            rptr += DIGESTSIZE;
            length -= bytesleft;
        }
        if (ackrsync->cs.seglen) {
            /* save offset, length - in sectors */
            ref->offset = offset;
            ref->length = ackrsync->cs.seglen >> DEV_BSHIFT;
            add_delta_to_list(rsync, ref);
            ackrsync->cs.seglen = 0;
        }
    }

    return 0;
} /* chksumdiff */

/****************************************************************************
 * chksumseg -- compute checksums on given sector range
 ***************************************************************************/
int
chksumseg(rsync_t *rsync)
{
    MD5_CTX context;
    unsigned char *dataptr;
    unsigned char *digestptr;
    unsigned int length;
    unsigned int left;
    unsigned int bytes;
    int res;
    int rc;

    /* read the device */
    if ((rc = readblock(rsync)) == -1) {
        return -1;
    }
    digestptr = (unsigned char*)rsync->cs.digest;
    dataptr = (unsigned char*)rsync->data;

    res = MINCHK;
    length = (((unsigned int)rsync->length) << DEV_BSHIFT);

    for (left = length; left > 0; left -= res) {
        if (left < res) {
            bytes = res = left;
        } else {
            bytes = res;
        }
        MD5Init(&context);
        MD5Update(&context, (unsigned char*)dataptr, bytes);
        MD5Final((unsigned char*)digestptr, &context);
        digestptr += DIGESTSIZE;
        dataptr += bytes;
    }

    return 1;
} /* chksumseg */

/****************************************************************************
 * process_seg -- process data segment - RFD
 ***************************************************************************/
int
process_seg (rsync_t *rsync)
{
    rsync_t lrsync[1];
    u_longlong_t bytelen;
    int devbytes = 0;
    int rc;

    memcpy(lrsync, rsync, sizeof(rsync_t));
    /* modify some state for checksum function */
    lrsync->offset = rsync->cs.dirtyoff;
    lrsync->length = rsync->cs.dirtylen >> DEV_BSHIFT;
    lrsync->datalen = lrsync->cs.seglen = rsync->cs.dirtylen;

    /* bytelen will always be <= CHUNKSIZE */
    bytelen = lrsync->datalen;
    lrsync->cs.num = bytelen%MINCHK ? (bytelen/MINCHK)+1: bytelen/MINCHK;
    lrsync->cs.cnt = 2;

    if (chksum_flag) {
        /* build local checksum buffer */
        if ((rc = chksumseg(lrsync)) == -1) {
            EXIT(EXITANDDIE);
        }
        /* save offset, length for send */
        rsync->cs.num = lrsync->cs.num;
        rsync->cs.segoff = rsync->cs.dirtyoff;
        rsync->cs.seglen = rsync->cs.dirtylen;
    } else {
        /*
         * This is an attempt at optimization. just send delta.
         */
        if ((devbytes = readblock(lrsync)) == -1) {
            EXIT(EXITANDDIE);
        }
        sendblock(lrsync);

        rsync->cs.segoff = rsync->cs.dirtyoff;
        rsync->cs.seglen = rsync->cs.dirtylen;
    }
    /* reset dirty len accumulator */
    rsync->cs.dirtyoff = rsync->offset;
    rsync->cs.dirtylen = 0;

    return 0;
} /* process_seg */

/****************************************************************************
 * sendrfdchk -- send a RFD sync request to RMD
 ***************************************************************************/
int
sendrfdchk(void)
{
    sddisk_t *sd;
    rsync_t *rsync;
    rsync32_t rsync32array[1024];
    headpack_t header[1];
    headpack32_t header32[1];
    int digestlen;
    int iovcnt;
    int rc = 0;
    int count = 0;

    memset(header, 0, sizeof(header));
    memset(header32, 0, sizeof(header32));
    memset(rsync32array, 0, 1024*sizeof(rsync32_t));

    header->magicvalue = MAGICHDR;
    header->cmd = CMDRFDCHKSUM;
    header->ackwanted = 1;

    if (iov == NULL) {
        iov = (struct iovec*)ftdmalloc(sizeof(struct iovec));
        iovlen = 1;
    }
    if (!rmd64)
    {
        converthdrfrom64to32 (header, header32);
        iov[0].iov_base = (void*)header32;
        iov[0].iov_len = sizeof(headpack32_t);
    }
    else
    {
    	iov[0].iov_base = (void*)header;
    	iov[0].iov_len = sizeof(headpack_t);
    }

    iovcnt = 1;

    for (sd = mysys->group->headsddisk; sd; sd = sd->n) {
        rsync = &sd->rsync;
        if (rsync->cs.seglen == 0) {
            continue;
        }
        digestlen = rsync->cs.num*DIGESTSIZE;

        if (++iovcnt > iovlen) {
            iovlen *= 2;
            iov = (struct iovec*)ftdrealloc((char*)iov,
                iovlen*sizeof(struct iovec));
        }
	if (!rmd64)
        {
	        convertrsyncfrom64to32 (rsync, &rsync32array[count]);
                iov[iovcnt-1].iov_base = (void*)&rsync32array[count];
                iov[iovcnt-1].iov_len = sizeof(rsync32_t);
                count++;
        }
        else
        {
        	iov[iovcnt-1].iov_base = (void*)rsync;
        	iov[iovcnt-1].iov_len = sizeof(rsync_t);
        }
        if (++iovcnt > iovlen) {
            iovlen *= 2;
            iov = (struct iovec*)ftdrealloc((char*)iov,
                iovlen*sizeof(struct iovec));
        }
        iov[iovcnt-1].iov_base = (void*)rsync->cs.digest;
        iov[iovcnt-1].iov_len = digestlen;

        sd->stat.a_tdatacnt += sizeof(rsync_t)+digestlen;
        sd->stat.e_tdatacnt += sizeof(rsync_t)+digestlen;

        if (iovcnt >= 10) {
            if (!rmd64)
            {
            	header32->len = (iovcnt-1)/2;
            }
            else
            {
            	header->len = (iovcnt-1)/2;
            }
            // Increment Sequencer value for each net_write_vector() call
            Sequencer_Inc(&mysys->group->SendSequencer);
            // WI_338550 December 2017, implementing RPO / RTT
            update_RTT_based_on_SentSequencer(mysys->group);
            header->lgsn = mysys->group->SendSequencer;
            ftd_trace_flow(FTD_DBG_SEQCER, "sendrfdchk CMDRFDCHKSUM %d lgsn = %d\n", ISSECONDARY(mysys), header->lgsn);
            rc = net_write_vector(mysys->sock, iov, iovcnt, ROLEPRIMARY);
            iovcnt = 1;
		    // PROD8354: restore iov[0] information as it may have been altered by net_write_vector
            if (!rmd64)
            {
                iov[0].iov_base = (void*)header32;
                iov[0].iov_len = sizeof(headpack32_t);
            }
            else
            {
    	        iov[0].iov_base = (void*)header;
    	        iov[0].iov_len = sizeof(headpack_t);
            }
        }

    } // end of for loop

    if (iovcnt > 1) {
	if (!rmd64)
        {
        	header32->len = (iovcnt-1)/2;
        }
        else
        {
        	header->len = (iovcnt-1)/2;
	}
        // Increment Sequencer value for each net_write_vector() call
        Sequencer_Inc(&mysys->group->SendSequencer);
        // WI_338550 December 2017, implementing RPO / RTT
        update_RTT_based_on_SentSequencer(mysys->group);
        header->lgsn = mysys->group->SendSequencer;
        ftd_trace_flow(FTD_DBG_SEQCER, "sendrfdchk CMDRFDCHKSUM %d lgsn = %d\n", ISSECONDARY(mysys), header->lgsn);
        rc = net_write_vector(mysys->sock, iov, iovcnt, ROLEPRIMARY);
    }
    /* reset segment offset, length */
    for (sd = mysys->group->headsddisk; sd; sd = sd->n) {
        rsync = &sd->rsync;
        rsync->cs.segoff = 0;
        rsync->cs.seglen = 0;
    }
    
    return rc;
} /* sendrfdchk */

/****************************************************************************
 * sendbfdchk -- send a BFD sync request to RMD
 ***************************************************************************/
int
sendbfdchk(void)
{
#ifndef SUPPORT_BACKFRESH
    reporterr(ERRFAC, M_BKFRSH_NOTAVAIL, ERRINFO);
    return( -1 );
#endif
    sddisk_t *sd;
    rsync_t *rsync;
    rsync32_t rsync32array[1024];
    headpack_t header[1];
    headpack32_t header32[1];
    int digestlen;
    int iovcnt = 0;
    int rc = 0;
    int count = 0;

    memset(header, 0, sizeof(header));
    memset(header32, 0, sizeof(header32));
    memset(rsync32array, 0, 1024*sizeof(rsync32_t));

    header->magicvalue = MAGICHDR;
    header->cmd = CMDBFDCHKSUM;
    header->ackwanted = 1;

    if (iov == NULL) {
        iov = (struct iovec*)ftdmalloc(sizeof(struct iovec));
        iovlen = 1;
    }
    if (!rmd64)
    {
	converthdrfrom64to32 (header, header32);
    	iov[0].iov_base = (void*)header32;
    	iov[0].iov_len = sizeof(headpack32_t);
    }
    else
    {
    	iov[0].iov_base = (void*)header;
    	iov[0].iov_len = sizeof(headpack_t);
    }
    iovcnt = 1;

    for (sd = mysys->group->headsddisk; sd; sd = sd->n) {
        rsync = &sd->rsync;
        if (rsync->cs.seglen == 0) {
            continue;
        }
        digestlen = rsync->cs.num*DIGESTSIZE;

        if (++iovcnt > iovlen) {
            iovlen *= 2;
            iov = (struct iovec*)ftdrealloc((char*)iov,
                iovlen*sizeof(struct iovec));
        }
	if (!rmd64)
        {
		convertrsyncfrom64to32 (rsync, &rsync32array[count]);
        	iov[iovcnt-1].iov_base = (void*)&rsync32array[count];
        	iov[iovcnt-1].iov_len = sizeof(rsync32_t);
		count++;
        }
        else
        {
        	iov[iovcnt-1].iov_base = (void*)rsync;
        	iov[iovcnt-1].iov_len = sizeof(rsync_t);
        }
        if (++iovcnt > iovlen) {
            iovlen *= 2;
            iov = (struct iovec*)ftdrealloc((char*)iov,
                iovlen*sizeof(struct iovec));
        }
        iov[iovcnt-1].iov_base = (void*)rsync->cs.digest;
        iov[iovcnt-1].iov_len = digestlen;

        sd->stat.a_tdatacnt += sizeof(rsync_t)+digestlen;
        sd->stat.e_tdatacnt += sizeof(rsync_t)+digestlen;

        if (iovcnt >= 10) {
	        if (!rmd64)
            {
            	header32->len = (iovcnt-1)/2;
            }
            else
            {
            	header->len = (iovcnt-1)/2;
            }
            rc = net_write_vector(mysys->sock, iov, iovcnt, ROLEPRIMARY);
            iovcnt = 1;
		    // PROD8354: restore iov[0] information as it may have been altered by net_write_vector
            if (!rmd64)
            {
    	        iov[0].iov_base = (void*)header32;
    	        iov[0].iov_len = sizeof(headpack32_t);
            }
            else
            {
    	        iov[0].iov_base = (void*)header;
    	        iov[0].iov_len = sizeof(headpack_t);
            }
        }
    } // end of for loop
    if (iovcnt > 1) {
	if (!rmd64)
	{
        	header32->len = (iovcnt-1)/2;
	}
	else
        {
        	header->len = (iovcnt-1)/2;
	}
        rc = net_write_vector(mysys->sock, iov, iovcnt, ROLEPRIMARY);
    }
    for (sd = mysys->group->headsddisk; sd; sd = sd->n) {
        rsync = &sd->rsync;
        /* reset segment offset, length */
        rsync->cs.segoff = 0;
        rsync->cs.seglen = 0;
    }
    return rc;
} /* sendbfdchk */

/****************************************************************************
 * block_all_zero -- compare data block against all zeros
 ***************************************************************************/
#define _ROUND_UP(v, i) (((((unsigned long)(v)) + (((unsigned long)(i)) - 1)) / \
                        ((unsigned long)(i))) * ((unsigned long)(i)))
int
block_all_zero(rsync_t *rsync)
{
    typedef unsigned long long chunk_t;
    char *p = (char *)rsync->data;
    const char *const end = &p[rsync->datalen];
    chunk_t *chunk = (chunk_t *)_ROUND_UP(rsync->data, sizeof(*chunk));

    while (p < (char *)chunk)
        if (*p++) return 0;
    while ((char *)chunk < &end[-sizeof(*chunk)])
        if (*chunk++) return 0;
    p = (char *)chunk;
    while (p < end)
        if (*p++) return 0;

    return 1;
} /* block_all_zero */

/****************************************************************************
 * sendblock -- send a data block to remote
 ***************************************************************************/
static int
sendblock(rsync_t *rsync)
{
    headpack_t header[1];
    headpack32_t header32[1];
    ackpack_t ack[1];
    ackpack32_t ack32[1];
    sddisk_t *sd;        
    char *datap;
    int length;
    int cnt;
    int rc;

    struct iovec iov[3];

    if ((sd = getdevice(rsync)) == NULL) {
        reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, rsync->devid);
        EXIT(EXITANDDIE);
    }
    memset(header, 0, sizeof(header));
    memset(header32, 0, sizeof(header32));
    memset(iov, 0, sizeof(struct iovec));

    header->magicvalue = MAGICHDR;
	if( _net_bandwidth_analysis )
	{
        header->cmd = CMDRFDFSWRITE;
	}
	else
	{
        header->cmd = CMDRFDFWRITE;
	}
    header->devid = rsync->devid;
    header->offset = rsync->offset;
    header->decodelen = rsync->length << DEV_BSHIFT;
    header->len = rsync->datalen;

/*
    if (_pmd_state == FTDRFDF) {
        header->ackwanted = 1;
    } else {
        header->ackwanted = 0;
    }
*/
    // We now always request the acks, even when in checksum refresh in order to maintain the rsync->refreshed_sectors.
    header->ackwanted = 1;
    
    if (!ISSECONDARY(mysys))
    {
        Sequencer_Inc(&mysys->group->SendSequencer);
        // WI_338550 December 2017, implementing RPO / RTT
        update_RTT_based_on_SentSequencer(mysys->group);
        header->lgsn = mysys->group->SendSequencer;
        ftd_trace_flow(FTD_DBG_SEQCER, "sendblock CMDRFDFWRITE ackwanted = %d lgsn = %d\n", header->ackwanted, header->lgsn);
    }

/*
*/
    if (ISSECONDARY(mysys)) {
        /* precede header with an ACK */
        memset(ack, 0, sizeof(ackpack_t));
        memset(ack32, 0, sizeof(ackpack32_t));
        ack->magicvalue = MAGICACK;
        ack->acktype = ACKNORM;
        ack->data = ACKBFDDELTA;
        ack->devid = header->devid;
        if (!pmd64)
	{
                converthdrfrom64to32 (header, header32);
                convertackfrom64to32 (ack, ack32);
        	iov[0].iov_base = (void*)ack32;
        	iov[0].iov_len = sizeof(ackpack32_t);
        	iov[1].iov_base = (void*)header32;
        	iov[1].iov_len = sizeof(headpack32_t);
        }
        else
        {
        	iov[0].iov_base = (void*)ack;
        	iov[0].iov_len = sizeof(ackpack_t);
        	iov[1].iov_base = (void*)header;
        	iov[1].iov_len = sizeof(headpack_t);
        }
        cnt = 3;
    } else {
	if (!rmd64)
        {
		converthdrfrom64to32 (header, header32);
        	iov[0].iov_base = (void*)header32;
        	iov[0].iov_len = sizeof(headpack32_t);
        }
        else
        {
        	iov[0].iov_base = (void*)header;
        	iov[0].iov_len = sizeof(headpack_t);
	}
        cnt = 2;
    }
    if (block_all_zero(rsync)) {
        if ((!rmd64) || (!pmd64))
        {
        	header32->data = BLOCKALLZERO;
        }
        else
        {
        	header->data = BLOCKALLZERO;
        }

        /* bump network data counts */
        sd->stat.a_datacnt += sizeof(headpack_t);
        sd->stat.a_tdatacnt += sizeof(headpack_t);

        rc = net_write_vector(mysys->sock, iov, cnt-1, ISSECONDARY(mysys));
    } else {
        if (mysys->tunables.compression) {
            /* compress data */
            if ((length = rsync->datalen) > compress_buf_len) {
                compress_buf_len = length + (length >> 1) + 1;
                ftdfree(compress_buf);
                compress_buf = (char*)ftdmemalign(compress_buf_len, buf_alignment);
            }
            length = compress((u_char*)rsync->data,
                (u_char*)compress_buf, rsync->datalen);
            datap = compress_buf;
        } else {
            datap = rsync->data;
            length = rsync->datalen;
        } 
        if ((!rmd64) || (!pmd64))
        {
        	header32->compress = mysys->tunables.compression;
        	header32->len = length;
        }
        else
        {
        	header->compress = mysys->tunables.compression;
        	header->len = length;
        }

        iov[cnt-1].iov_base = (void*)datap;
        iov[cnt-1].iov_len = length;

        /* bump network data counts */
        sd->stat.a_datacnt += header->len;
        sd->stat.a_tdatacnt += header->len + sizeof(headpack_t);

        rc = net_write_vector(mysys->sock, iov, cnt, ISSECONDARY(mysys));
    }
    /* bump rsync delta count */
    sd->rsync.delta += rsync->length;

    return 0;
} /* sendblock */

/****************************************************************************
 * sendrsyncstart -- tell RMD to put itself into rsync mode
 ***************************************************************************/
int
sendrsyncstart(int state)
{
    headpack_t header[1];
    headpack32_t header32[1];

    memset(header, 0, sizeof(headpack_t));
    memset(header32, 0, sizeof(headpack32_t));
    header->magicvalue = MAGICHDR;
    header->data = (u_long)state;  // WR PROD6755: tell the RMD which Phase of Refresh we do
    header->cmd = state = state == FTDBFD ? CMDBFDSTART: CMDRFDFSTART;
    header->ackwanted = 1;
    if (!rmd64)
    {
	converthdrfrom64to32 (header, header32);
    	if (-1 == writesock(mysys->sock, (char*)header32, sizeof(headpack32_t))) {
        	EXIT(exitfail);
    	}
    }
    else
    {
    	if (-1 == writesock(mysys->sock, (char*)header, sizeof(headpack_t))) {
        	EXIT(exitfail);
    	}
    }
    return 0;
} /* sendrsyncstart */

/****************************************************************************
 * sendrsyncdevs -- tell RMD to put this device into refresh mode
 ***************************************************************************/
int
sendrsyncdevs(rsync_t *rsync, int state)
{
    headpack_t header[1];
    headpack32_t header32[1];
    sddisk_t *sd;

    if ((sd = getdevice(rsync)) == NULL) {
        reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, rsync->devid);
        EXIT(EXITANDDIE);
    }
    memset(header, 0, sizeof(header));
    memset(header32, 0, sizeof(header32));
    header->magicvalue = MAGICHDR;
    header->cmd = state == FTDBFD ? CMDBFDDEVS: CMDRFDDEVS;
    header->devid = rsync->devid;
    header->offset = rsync->offset;
    header->data = sd->dm.res;
    header->ackwanted = 1;
    if (!rmd64)
    {
        converthdrfrom64to32 (header, header32);
    	if (-1 == writesock(mysys->sock, (char*)header32, sizeof(headpack32_t))) {
        	EXIT(exitfail);
    	}
    }
    else
    {
    	if (-1 == writesock(mysys->sock, (char*)header, sizeof(headpack_t))) {
        	EXIT(exitfail);
    	}
    }
    return 0;
} /* sendrsyncdevs */

/****************************************************************************
 * sendrsyncdeve -- tell RMD to take this device out of rsync mode
 ***************************************************************************/
int
sendrsyncdeve(rsync_t *rsync, int state)
{
    headpack_t header[1];
    headpack32_t header32[1];

    memset(header, 0, sizeof(header));
    memset(header32, 0, sizeof(header32));
    header->magicvalue = MAGICHDR;
    header->cmd = state == FTDBFD ? CMDBFDDEVE: CMDRFDDEVE;
    header->devid = rsync->devid;
    header->ackwanted = 1;
    if (!rmd64)
    {
        converthdrfrom64to32 (header, header32);
        if (-1 == writesock(mysys->sock, (char*)header32, sizeof(headpack32_t))) {
                EXIT(exitfail);
        }
    }
    else
    {
    	if (-1 == writesock(mysys->sock, (char*)header, sizeof(headpack_t))) {
        	EXIT(exitfail);
    	}
    }
    return 0;
} /* sendrsyncdeve */

/****************************************************************************
 * sendrsyncend -- tell RMD to take this logical group out of rsync mode
 ***************************************************************************/
int
sendrsyncend(int lgnum, int state)
{
    headpack_t header[1];
    headpack32_t header32[1];

    memset(header, 0, sizeof(header));
    memset(header32, 0, sizeof(header32));
    header->magicvalue = MAGICHDR;
    switch(state) {
    case FTDBFD:
        header->cmd = CMDBFDEND;
        break;
    case FTDRFD:
        header->cmd = CMDRFDEND;
        break;
    case FTDRFDF:
        header->cmd = CMDRFDFEND;
        break;
    default:
        break;
    }
    header->devid = lgnum;
    header->ackwanted = 0;
    if (!rmd64)
    {
        converthdrfrom64to32 (header, header32);
    	if (-1 == writesock(mysys->sock, (char*)header32, sizeof(headpack32_t))) {
        	EXIT(exitfail);
    	}
    }
    else
    {
    	if (-1 == writesock(mysys->sock, (char*)header, sizeof(headpack_t))) {
        	EXIT(exitfail);
    	}
    }
    return 0;
} /* sendrsyncend */

/****************************************************************************
 * devs_to_read -- any devices left to read ?
***************************************************************************/
int
devs_to_read (void)
{
    group_t *group;
    sddisk_t *sd;
    int ret = 0;

    group = mysys->group;
    for (sd = group->headsddisk; sd; sd = sd->n) {
        if (!sd->rsync.done) {
           ret++;
        }
    }
    return ret;
} /* devs_to_read */

#if defined(linux)
/*
 * Under Linux 2.4 transfer sizes, and  the  alignment  of
 * user buffer and file offset must all be multiples of the logical
 * block size of the file system.  Under  Linux  2.6  alignment  to
 * 512-byte boundaries suffices.
 */
char *
adjust_data_ptr(char *data_ptr, int blksize)
{
    unsigned long ul;
    ul = (unsigned long)data_ptr;
    ul = ((ul + (blksize - 1)) / blksize) * blksize;
    return (char *)ul;
}

#endif

/****************************************************************************
   Push Full Refresh offset completion back to 0% fr current group,
   to restart completely upon relaunch
***************************************************************************/
void reset_FRefresh_back_to_0( void )
{
    group_t       *group;
    sddisk_t      *sd;
    rsync_t       *rsync = NULL;
    int           pstore_error;
	int           message_already_logged;

    if( !mysys )
    {
       /* Minimum initializations not performed; cannot be in middle of Full Refresh: do nothing */
       return;
    }

    group = mysys->group;

    message_already_logged = 0;

    /* For all the devices of this group... */
    for (sd = group->headsddisk; sd; sd = sd->n)
    {
       rsync = &sd->rsync;

       /* check if a FRefresh has been interrupted on this device */
       pstore_error = (ps_get_device_entry(mysys->pstore,sd->sddevname,&rsync->ackoff) != PS_OK);

       if( pstore_error )
       {
           sprintf( debug_msg, "reset_FRefresh_back_to_0(): error accessing pstore to obtain previous FRefresh percent completion\n" );
           reporterr(ERRFAC, M_GENMSG, ERRWARN, debug_msg);

	   /* Cannot say that a Full Refresh was interrupted; use a different message, 
	      but still force restart offset to 0% in case a Full Refresh would be relaunched
	   */
           sprintf( debug_msg, "Resetting next Full Refresh start offset to 0 as safety measure\n" );
           reporterr(ERRFAC, M_GENMSG, ERRWARN, debug_msg);
           ps_set_device_entry( mysys->pstore, sd->sddevname, 0 );  /* Write to pstore */
       }
       else /* No pstore error */
       {
          /* If current FRefresh offset saved in pstore == 0, Full Refresh complete or not started: 
             nothing to do for current device; else display a message indicating forcing back to 0% 
             and adjust pstore entry; note: log the message only once for the group, since all the devices
			 with non-zero offset will be rewinded to 0 for next Full Refresh (WR PROD6757)
          */
          if( rsync->ackoff != 0 )
          {
		     if( !message_already_logged )
			 {
                reporterr(ERRFAC, M_RFD_OFFSETTO0, ERRWARN);
				message_already_logged = 1;
			 }
             ps_set_device_entry( mysys->pstore, sd->sddevname, 0 );  /* Write to pstore */
   	      }
          /* else: no pstore_error and current offset already == 0; nothing to do for this device */
       }

       rsync->offset = rsync->ackoff = 0;

    } /*... for ... */

    return;
}




/****************************************************************************
  Function to adjust the last known good full refresh offset of each device
  of a group backward to make sure we cover blocs that might have been lost
  due to an RMD crash; note: if the requested percentage_backward adjustment
  is greater than the curret offset done, clear current offset to 0; if
  current offset == 0 (FRefresh completed or not started), exit.
  Also, if the percentage requested gives a very large number of blocks,
  limit this to a maximum (for instance if calculated on the basis of a
  terabyte device); same thing if number of blocks is too small, bind it
  to a minimum. Note: it is possible to force complete restart at 0%
  via parameter force_complete_restart
***************************************************************************/
void adjust_FRefresh_offset_backward( double percentage_backward, int force_complete_restart )
{
    int           int_current_percentage_done;
    int           int_device_percent_adjustment;
    int           int_new_offset_percentage;
    group_t       *group;
    sddisk_t      *sd;
    rsync_t       *rsync = NULL;
    u_longlong_t  offset_adjustment;

    if( !mysys )
    {
       /* Minimum initializations not performed; cannot be in middle of Full Refresh: do nothing */
       return;
    }

    if( force_complete_restart )
    {
       reset_FRefresh_back_to_0();
       return;                     /* <-- Exit point if force_complete_restart */
    }

    group = mysys->group;

    /* For all the devices of this group... */
    for (sd = group->headsddisk; sd; sd = sd->n)
    {
       rsync = &sd->rsync;

       /* check if a FRefresh has been interrupted */
       if( ps_get_device_entry(mysys->pstore,sd->sddevname,&rsync->ackoff) != PS_OK )
       {
          sprintf( debug_msg, "adjust_FRefresh_offset_backward(): error accessing pstore to obtain previous FRefresh percent completion\n" );
          reporterr(ERRFAC, M_GENMSG, ERRWARN, debug_msg);
          reset_FRefresh_back_to_0(); /* Force FRefresh offset back to 0% */
          return;                     /* <-- Exit point if pstore read error */
       }

       rsync->offset = rsync->ackoff;
       /* If current FRefresh offset saved in pstore == 0, Full Refresh complete or not started: 
          nothing to do for current device; else display a message indicating adjusting percent 
          completion backward and update pstore entry
       */
       if( rsync->offset != 0 )
       {
          sprintf( debug_msg, "Full Refresh abnormally interrupted; adjusting percentage completion backward as safety measure\n" );
          reporterr(ERRFAC, M_GENMSG, ERRWARN, debug_msg);

          /* We need the device size to calculate percentage adjustment */
          if (sd->devsize <= 0)
          {
             sd->devsize = (u_longlong_t)disksize(sd->devname);
             if (sd->devsize <= 0) 
             {
               reporterr(ERRFAC, M_BADDEVSIZ, ERRWARN, sd->devname);
               reset_FRefresh_back_to_0(); /* Force FRefresh offset back to 0% */
	       return;                     /* <-- Exit point if device size unknown */
             }
          }

          int_current_percentage_done = 100 - (int)( ((double)(sd->devsize - rsync->offset) * 100.0) / (double)(sd->devsize) );

          offset_adjustment = (u_longlong_t)(percentage_backward  * (double)(sd->devsize));

          if( offset_adjustment > MAX_FREFRESH_ADJUSTMENT )
             offset_adjustment = MAX_FREFRESH_ADJUSTMENT;
          else if( offset_adjustment < MIN_FREFRESH_ADJUSTMENT )
             offset_adjustment = MIN_FREFRESH_ADJUSTMENT;

          if( rsync->offset <= offset_adjustment )
          {
             /* Set offset to 0 to avoid underflow */
             ps_set_device_entry( mysys->pstore, sd->sddevname, 0 );
             int_new_offset_percentage = 0;
          }
          else
          {
             rsync->offset -= offset_adjustment;
             ps_set_device_entry( mysys->pstore, sd->sddevname, rsync->offset );
             int_device_percent_adjustment = (int)( ((double)offset_adjustment * 100.0) / (double)(sd->devsize) );
             int_new_offset_percentage = int_current_percentage_done - int_device_percent_adjustment;
          }
          sprintf( debug_msg, "%s: Full Refresh percentage done: %d%%, adjusted backward to %d%%\n", 
                   sd->sddevname, int_current_percentage_done, int_new_offset_percentage );
          reporterr(ERRFAC, M_GENMSG, ERRINFO, debug_msg);
       } /*... if rsync_offset != 0... */
    } /*... for ... */

    return;
}

/****************************************************************************
 * refresh_full -- get data from data device(s) and write to mirror
 * Passed -r option as flag
***************************************************************************/
int
refresh_full (int fullrefresh_restart)
{
    group_t *group;
    sddisk_t *sd;
    rsync_t *rsync = NULL;
    static time_t currts = 0;
    static time_t lastts = 0;
    static time_t FullRefresh_start_ts = 0;
    static time_t last_stat_ts = 0;
    static int save_chunksize =0;
    int deltatime, time_since_start;
    int devcnt;
    int moredevs;
    int done;
    int lgnum;
    int devbytes;
    u_longlong_t rsyncsize;
    u_longlong_t bytesleft;
    int rc;
	int all_at_offset_0;
	int net_analysis_duration;
    u_longlong_t total_KBs_transferred = 0;

    group = mysys->group;
	
    lgnum = atoi(_pmd_configpath+strlen(_pmd_configpath)-3);

    if( !_net_bandwidth_analysis )
	{
        reporterr(ERRFAC, M_RFDPHASE1START, ERRINFO, argv0);
		// Record information in the Product Usage tracking file
        if( (update_product_usage_tracking_file( mysys, lgnum, 1, 0, REGISTER_DEVICES_AT_FR_START )) < 0 )
		{
            reporterr(ERRFAC, M_PRODUSE_FSTART, ERRWARN, lgnum);
		}
	}

    if ((rc = rsyncinit(group, 0,fullrefresh_restart)) != FTD_OK) {
        return rc;
    }

    devcnt = moredevs = rsynccnt;
    done = 0;

    g_rfddone = 0;

    /* If the launchrefresh command is run with -r -a/g# command, then
     * get the last completed full refresh amount from the pstore (device wise)
     * and assign to the rsync structure offset and ackoff values
      */
	/* WR PROD6757: check if all devices in the group are rewinded to offset 0 for Full Refresh restart.
	   If so, we will log only one message for the whole group. At the same time, preserve the restart offset
	   in the sd->rsync structure for each device (DO NOT break this loop upon finding a non-zero offet).
    */
    if(fullrefresh_restart)
    {
	    all_at_offset_0	= 1;   /* Assume all devices rewinded to 0 */
        for (sd = group->headsddisk; sd; sd = sd->n)
        {
            rsync = &sd->rsync;
            ps_get_device_entry(mysys->pstore,sd->sddevname,&rsync->ackoff);
            rsync->offset = rsync->ackoff;
			if(rsync->ackoff != 0)
			{
			    all_at_offset_0 = 0;
			} 
		}
		if( all_at_offset_0 )
		{
		    /* All devices restart at offset 0: log only one message for whole group */
            reporterr( ERRFAC, M_RFD_GRP_AT_0, ERRINFO );
		}
	}

    for (sd = group->headsddisk; sd; sd = sd->n)
    {
        if(fullrefresh_restart)
        {
		    /* WR PROD6757: if not all restarting at 0, show next offset per device */
			if( (!all_at_offset_0) && (sd->devsize > 0))
			{
			    rsync = &sd->rsync;
                reporterr( ERRFAC, M_RFDOFFSET, ERRINFO, sd->devname, rsync->offset, (int)((rsync->offset*100.00) / sd->devsize) );
			}
        }
        else
        {
		    /* Not restarting Full Refresh: reset next offset to 0 */
            ps_set_device_entry(mysys->pstore, sd->sddevname,0);
        }
    }
    /* refresh all devices for the logical group */

    if( _net_bandwidth_analysis )
	{
	    // Full Refresh simulation for network bandwidth analysis
        time(&FullRefresh_start_ts);
		last_stat_ts = FullRefresh_start_ts;
		time_since_start = 0;

        if (rc = gettunables_from_net_analysis_file( &net_analysis_duration ) < 0 )
        {
            reporterr(ERRFAC, M_NETTUNABLE_ERR, ERRCRIT, PATH_CONFIG, "SFTKdtc_net_analysis_parms.txt", rc);
            EXIT(EXITANDDIE);
		}
        reporterr(ERRFAC, M_RFDSIMSTART, ERRINFO, net_analysis_duration, argv0);

	    init_buffer(mysys->tunables.chunksize);
	    rsyncsize = mysys->tunables.chunksize;
	    while( time_since_start < net_analysis_duration )
	    {
	        if (_pmd_state_change)
	            return 0;
	        if (mysys->tunables.chunkdelay > 0)
	        {
	            usleep(mysys->tunables.chunkdelay*1000);
	        }
	        
            time(&currts);
			time_since_start = (currts - FullRefresh_start_ts);
	        for (sd = group->headsddisk; sd; sd = sd->n)
	        {
	            rsync = &sd->rsync;
	            rsync->data = buf;

                process_acks();

	            rsync->length = (rsyncsize >> DEV_BSHIFT);
			    // In network bandwidth analysis mode, use a local RAM buffer (no device access)
                devbytes = setblock(rsync);

	            rsync->datalen = devbytes;
	            rsync->length = (devbytes >> DEV_BSHIFT);

	            /* update stats before the write */
	            sd->stat.e_datacnt += rsync->datalen;
	            sd->stat.e_tdatacnt += rsync->datalen;
		        sd->throtstats.actualkbps += ((float)rsync->datalen / 1024.0);
		        sd->throtstats.effectkbps += ((float)rsync->datalen / 1024.0);
	            sendblock(rsync);

	            eval_netspeed();

	            time(&currts);
	            deltatime = (currts-last_stat_ts);
	            if (deltatime >= mysys->tunables.statinterval) {
				    // Compute per-second stats and save to prf file
					total_KBs_transferred += sd->throtstats.actualkbps;  // Cumulative KBs transferred
					sd->throtstats.actualkbps /= ((float)deltatime);
					sd->throtstats.effectkbps /= ((float)deltatime);
					sd->throtstats.pctdone= (float)((float)time_since_start / (float)net_analysis_duration) * 100.0;
	                savestats(0, _net_bandwidth_analysis);
	                save_csv_stats(net_analysis_duration, sd->sddevname); // csv format for Network planner tool
	                last_stat_ts = currts;
			        sd->throtstats.actualkbps = 0.0;
			        sd->throtstats.effectkbps = 0.0;
	            }
	        }
	        process_acks();
	        time(&currts);
	        time_since_start = (currts - FullRefresh_start_ts);
	    } // ... of while ...

        save_network_analysis_total( mysys, net_analysis_duration, total_KBs_transferred, group->headsddisk->sddevname );

        sendrsyncend(lgnum, _pmd_state);
	    
        reporterr(ERRFAC, M_RFDSIMEND, ERRINFO, argv0);

        /* In Full Refresh simulation mode for Network bandwidth evaluation,
           remove the fictitious config files and exit */
        remove_fictitious_cfg_files( lgnum, 1 );

        EXIT(EXITANDDIE);
	}

    // Here is the logic for the real Full Refresh
    while (!done) {
        if (_pmd_state_change) {
            return 0;
        }
        time(&currts);
        deltatime = (currts-lastts);
        if (deltatime >= 1) {
            gettunables(group->devname, 0, 0);
            /* NEW full refresh, don't empty BAB here
                since we're in tracking mode*/
            lastts = currts;
        }
        /* chunksize changed */
        init_buffer(mysys->tunables.chunksize);
        save_chunksize = mysys->tunables.chunksize;

        if (mysys->tunables.chunkdelay > 0)
        {
            usleep(mysys->tunables.chunkdelay*1000);
        }
        
        for (sd = group->headsddisk; sd; sd = sd->n) {
            rsync = &sd->rsync;
            rsync->data = buf;

            ftd_trace_flow(FTD_DBG_REFRESH,
                    "\n*** refresh_f: offset, ackoff, delta, size = %d, %d, %d, %d\n",
                    rsync->offset, rsync->ackoff, rsync->delta, rsync->size);

            if (rsync->offset >= rsync->size) {
                if (!rsync->done && rsync->ackoff >= rsync->size) {
                    sd->stat.rfshoffset = rsync->size;
                    savestats(1, _net_bandwidth_analysis);
                    rsync->done = 1;
                    moredevs--;
                    if (devs_to_read() == 0) {
                        sendrsyncend(lgnum, _pmd_state);
                    }
                } else {
                    process_acks();
                    if (!moredevs) {
                        done = 1;
                    }
                }
                continue;
            }
            bytesleft = (rsync->size-rsync->offset);
            bytesleft <<= DEV_BSHIFT;
            if (bytesleft < mysys->tunables.chunksize) {
                rsyncsize = bytesleft;
            } else {
                rsyncsize = mysys->tunables.chunksize;
            }
            rsync->length = (rsyncsize >> DEV_BSHIFT);
            if ((devbytes = readblock(rsync)) == -1) {
	                EXIT(EXITANDDIE);
			}
            /* set length to actual length returned from read */
            rsync->datalen = devbytes;
            rsync->length = (devbytes >> DEV_BSHIFT);

            /* update stats before the write */
            sd->stat.rfshoffset = rsync->ackoff;
            sd->stat.rfshdelta = rsync->delta;
            sd->stat.e_datacnt += rsync->datalen;
            sd->stat.e_tdatacnt += rsync->datalen;

            /* Store the completed full refresh amount in pstore device wise */
            ps_set_device_entry(mysys->pstore, sd->sddevname,rsync->ackoff);

            sendblock(rsync);

            /* bump device offset */
            rsync->offset += rsync->length;

            eval_netspeed();

            time(&currts);
            deltatime = (currts-last_stat_ts);
            if (deltatime >= 1) {
                savestats(0, _net_bandwidth_analysis);
                last_stat_ts = currts;
            }
        }
        process_acks();
    }
    reporterr(ERRFAC, M_RFDPHASE1END, ERRINFO, argv0);

    /* force a final net kbps dump to driver memory */
    for (sd = group->headsddisk; sd; sd = sd->n) {
        sd->stat.rfshoffset = rsync->size;
        sd->stat.rfshdelta = rsync->delta;
        /* clear out internal rsync stats */
        sd->rsync.offset = 0;
        sd->rsync.ackoff = 0;
        sd->rsync.delta = 0;
    }
    savestats(1, _net_bandwidth_analysis);
    rsynccleanup(_pmd_state);

    /* New FULL refresh behavior requires smart refresh after full*/
    reporterr(ERRFAC, M_RFDPHASE2START, ERRINFO, argv0);

    _pmd_state = FTDRFD;
    while (refresh(1) == 1) {
        _pmd_state_change = 0;
    }

    /* Once the full refresh is complete, set the pstore ackoff value to zero */
    for (sd = group->headsddisk; sd; sd = sd->n) {
        ps_set_device_entry(mysys->pstore, sd->sddevname,sd->rsync.ackoff);
    }

    /* Only transition to FTDPMD (normal) mode if no BAB overflow ocurred.
     * If BABOFLOW occurred, PMD remains in FTDRFD (smart refresh) mode.
     * Another round of smart refresh will be kick off.
     */
    if (chk_baboverflow(group) == 1) {
        _pmd_refresh = 1; /* indicates that an overflow occurred */
        /* bab overflowed, retained the smart refresh mode */
        reporterr(ERRFAC, M_BABOFLOW2, ERRWARN, lgnum);
        ++BAB_oflow_counter;    /* WR 43926 */
        // We're not able to clean up and merge the bitmaps at this point for these reasons:
        // -) The driver's been put in normal state which turned off the DualBitmaps mechanism.
        // -) The ack offsets have been reset to 0.
        // -) The bitmap refresh mechanism (historical bitmap) might be going on in parallel.
        
        // WI_338550 December 2017, implementing RPO / RTT
        // Invalidate RTT sequencer tag (as is done on Windows)
        SequencerTag_Invalidate(&group->RTTComputingControl.iSequencerTag);
    } else {
        /* no bab overflow, transition to normal mode */
        _pmd_state = FTDPMD;
    }

    return 0;
} /* refresh_full */

extern int chk_baboverflow(group_t *group);
extern statestr_ts statestr[];

/************************************************************************************//**

    Merge the input hrdb with the driver hrdb.map.next into driver hrdb for this group.

\param     group The group's definition.
\param     lgnum The group's number.

\pre       rsyncinit() and get_hires_dirtybits() must have been previously called since
           the global db array is used.

\return    -1 in case of error, otherwise 0.

****************************************************************************************/
int ftd_lg_merge_hrdbs(group_t* group, int lgnum)
{
    int             rc  = -1;
    sddisk_t *sd = NULL;
    ftd_dev_info_t *dev = NULL;
    stat_buffer_t sb = {0};
    int i;
    
    ftd_trace_flow(FTD_DBG_REFRESH, "Entered ftd_lg_merge_hrdbs()\n");

    dev = (ftd_dev_info_t*)ftdmalloc(db->numdevs*sizeof(ftd_dev_info_t));
    sb.lg_num = lgnum;
    sb.addr = (ftd_uint64ptr_t)(unsigned long)dev;
    rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_DEVICES_INFO, &sb);
    if (rc != 0) {
        free(dev);
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_DEVICES_INFO",
            strerror(errno));
        return -1;
    }

    db->devs = (dev_t *)ftdmalloc(db->numdevs*sizeof(dev_t));
    for (i = 0; i < db->numdevs; i++) {
        db->devs[i] = dev[i].cdev;
    }
    
    // Clear the bitmap for the part of the refresh that was successfully transmitted to the remote side
    for (sd = group->headsddisk; sd; sd = sd->n)
    {
        // The bits in the bitmap are indexed in the following manner:
        // -) The bitmap is actually contained within an array of 32 bit integers.
        // -) Within each integer, the bits are ordered from the least significant bit to the most significant bit.
        //
        // So dirty bits 0 and 31 are within the 1st integer at 0x00000001 and 0x80000000 respectively.
        // Dirty bits 32 and 63 are within the 2nd integer, at 0x00000001 to 0x80000000 as well.
        // And so on....
        //
        // See also dirtyseg() and the driver's ftd_set_bits() functions for references.
        
        // This represents the part of the volume known to have been refreshed and treated by the RMD.
        ftd_uint64_t refreshed_bytes = sd->rsync.refreshed_sectors << DEV_BSHIFT;

        // The resolution (dm.res) is in dirty bytes per dirty bits.
        ftd_uint64_t bits_of_bitmap_that_can_be_cleared = refreshed_bytes / sd->dm.res;

        ftd_uint64_t number_of_32_bit_ints_to_clear = bits_of_bitmap_that_can_be_cleared / 32; // Number of ints that can be fully cleared.
        ftd_uint32_t number_of_remaining_bits_to_clear = bits_of_bitmap_that_can_be_cleared % 32; // Partial bits that need to be cleared in the last int.

        ftd_uint32_t mask_to_clear_remaining_bits = 0xFFFFFFFF << number_of_remaining_bits_to_clear;

        ftd_uint32_t* bitmap_array_of_32_bit_ints = (ftd_uint32_t*)sd->dm.mask;

        // Clear the last partial integer.
        bitmap_array_of_32_bit_ints[number_of_32_bit_ints_to_clear] &= mask_to_clear_remaining_bits;

        // Clear the full ints.
        memset(sd->dm.mask, 0, number_of_32_bit_ints_to_clear * sizeof(unsigned int));
          
        ftd_trace_flow(FTD_DBG_REFRESH,
                       "Refresh will resume for device %s at offset: %llu, res: %d, clear length: %llu bits",
                       sd->devname,
                       refreshed_bytes,
                       sd->dm.res,
                       bits_of_bitmap_that_can_be_cleared);
    }
    
    // The global db array has already been allocated and initialized by rsyncinit, so we'll directly reuse it.
    rc = dbarr_ioc(group->devfd, FTD_MERGE_HRDBS, db,  dev);

    if (rc != 0)
    {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "dbarr_ioc/FTD_MERGE_HRDBS", strerror(errno));
    }
    
    ftd_trace_flow(FTD_DBG_REFRESH, "ftd_lg_merge_hrdbs returned: %d\n", rc);

    free(dev);
    free(db->devs);
    
    return rc;
}


/****************************************************************************
 * refresh -- get data from data device(s) and write to mirror
***************************************************************************/
int
refresh (int fullrefreshphase)
{
    group_t *group;
    sddisk_t *sd;
    rsync_t *rsync = NULL;
    static time_t currts = 0;
    static time_t lastts = 0;
    static time_t last_stat_ts = 0;
    u_longlong_t byteoff;
    u_longlong_t bytelen;
    u_longlong_t bytesleft;
    int timeslept, sleeptime;
    int deltatime;
    int devcnt;
    int moredevs;
    int done;
    int lgnum;
    int cs_num;
    static int save_chunksize = 0;
    int len;
    int digestlen;
    int i;
    int rc;

    ftd_trace_flow(FTD_DBG_FLOW1,
            "start refresh type %d\n", fullrefreshphase);

    group = mysys->group;

    if ((rc = rsyncinit(group, fullrefreshphase,0)) != FTD_OK) {
        return rc;
    }
    lgnum = atoi(_pmd_configpath+strlen(_pmd_configpath)-3);
    devcnt = moredevs = rsynccnt;
    done = 0;

    g_rfddone = 0;

    /* refresh all devices for the logical group */
    while (!done) {
        if (_pmd_state_change) {
            return 1;
        }
        time(&currts);
        deltatime = (currts-lastts);
        if (deltatime >= 1) {
            gettunables(group->devname, 0, 0);
            /* empty BAB here */
            (void)empty_bab();
            lastts = currts;
        }
        if (save_chunksize
        && mysys->tunables.chunksize > save_chunksize) {
            /* process data in buffer */
            for (sd = group->headsddisk; sd; sd = sd->n) {
                if (sd->rsync.cs.dirtylen > 0) {
                    process_seg(&sd->rsync);
                }
            }
            if (chksum_flag) {
                sendrfdchk();
            }
        }
        /* reallocate if chunksize changed  (or first time alloc)*/
        init_buffer(mysys->tunables.chunksize);

        cs_num = (mysys->tunables.chunksize/MINCHK)+2;
        digestlen = 2*(cs_num*DIGESTSIZE);
        save_chunksize = mysys->tunables.chunksize;

        for (sd = group->headsddisk; sd; sd = sd->n) {
            rsync = &sd->rsync;
            rsync->data = buf;
            if (digestlen > rsync->cs.digestlen) {
                rsync->cs.digest = ftdrealloc(rsync->cs.digest, digestlen);
            }
            rsync->cs.cnt = 2;
            rsync->cs.num = cs_num;
            rsync->cs.digestlen = digestlen;

            ftd_trace_flow(FTD_DBG_REFRESH,
                    "\n*** refresh: offset, ackoff, delta, size = %d, %d, %d, %d\n",
                    rsync->offset, rsync->ackoff, rsync->delta, rsync->size);

            if (rsync->offset >= rsync->size) {
                if (rsync->cs.dirtylen > 0) {
                    process_seg(rsync);
                }
                if (!rsync->done && rsync->ackoff >= rsync->size) {
                    sd->stat.rfshoffset = rsync->size;
                    savestats(0, _net_bandwidth_analysis);
                    moredevs--;
                    rsync->done = 1;
                } else {
                    // Here, we are in one of these two situations:

                    // -) We know we've sent all of the current device's dirty segments to the PMD and we are waiting to receive the related acks. (rsync->done)
                    // -) We are completely done, and are just waiting for other devices to complete. (!rsync->done)

                    if (!rsync->done)
                    {
                        // If we don't explicitely ask to process the acks in this situation, we end up only handling a limited number of acks
                        // periodically.  (Look above and below for deltatime and calls to empty_bab()/process_acks()).
                        
                        // This by itself wouldn't be so bad, but when combined with the fact that process_acks() never processes more
                        // than a limited amount acks each time it is called, this explains the extraneously slow smart refresh ack processing
                        // that was noticed in previous versions.
                        
                        // The decision to call empy_bab() at this particular location was based upon the following factors:
                        // -) Location: Equivalent code for the full refresh does something similar at this location as well.
                        // -) Using empty_bab() rather than process_acks():  In this situation, it was judged a good thing to check the
                        //    bab more often than once per second.
                        //    This currently works because empty_bab() calls dispatch() which calls process_acks().
                        // -) Start actively processing the acks after a single device is done sending its segments rather than all devices:
                        //    No valid justification to do so only after all devices are done sending their segments have been found and
                        //    doing so would have added even more complexity to the actual logic.
                        // -) CPU usage: This actual modification will still loop tightly, but we'll only do so in vain during the round
                        //    trip of the last IO of all devices sent.
                        //    The alternative of relying on some sleeping mechanism wasn't used since we have no way at this particular
                        //    location to know if there is some task to be processed by empty_bab() or even if the last call to
                        //    empty_bab() did process something or not.
                        // -) Only when !rsync->done: There's no need to do so otherwise, and in some rare circumstances,
                        //    (C.F. WR PROD00008574), calling empty_bab()/process_acks() when other devices of the group weren't finished
                        //    slowed down too much the refresh process.
                        empty_bab();
                    }
                    
                    if (!moredevs) {
                        done = 1;
                    }
                }
                continue;
            }
            bytesleft = (rsync->size-rsync->offset);
            bytesleft <<= DEV_BSHIFT;

            if (chksum_flag) {
                len = mysys->tunables.chunksize;
            } else {
                len = (sd->dm.res < 
		(unsigned int) mysys->tunables.chunksize) ? sd->dm.res : mysys->tunables.chunksize;
            }
            if (bytesleft < len) {
                rsync->length = (bytesleft >> DEV_BSHIFT);
            } else {
                rsync->length = (len >> DEV_BSHIFT);
            }
            byteoff = rsync->offset;
            byteoff <<= DEV_BSHIFT;
            bytelen = rsync->length;
            bytelen <<= DEV_BSHIFT;

            /* test dirty */
            if (dirtyseg(sd->dm.mask, byteoff, bytelen, sd->dm.res) > 0) {
                /* will the next segment overflow the buf ? */
                if (rsync->cs.dirtylen > 0
                && (rsync->cs.dirtylen+bytelen)
                    >= mysys->tunables.chunksize) {
                    process_seg(rsync);
                }
                /* bump contiguous dirty length */
                rsync->cs.dirtylen += bytelen;
            } else {
                /* clean segment found - dump current buf and start over */
                if (rsync->cs.dirtylen > 0) {
                    process_seg(rsync);
                }
                rsync->cs.dirtyoff = (byteoff+bytelen) >> DEV_BSHIFT;
                rsync->ackoff += rsync->length;
            }
            /* bump device offsets */
            rsync->offset += rsync->length;
/*
            rsync->ackoff += rsync->length;
*/

            /* update stats */
            sd->stat.rfshoffset = rsync->ackoff;
            sd->stat.rfshdelta = rsync->delta;
            sd->stat.e_datacnt += (rsync->length << DEV_BSHIFT);
            sd->stat.e_tdatacnt += (rsync->length << DEV_BSHIFT);

            eval_netspeed();

            time(&currts);
            deltatime = (currts-last_stat_ts);
            if (deltatime >= 1) {
                savestats(0, _net_bandwidth_analysis);
                process_acks();
                last_stat_ts = currts;
            }
        }
        /* send checksums for the group - if we are chksumming */
        if (chksum_flag) {
            sendrfdchk();
        }
        /* flush deltas */
        if (chksum_flag && net_writable) {
            flush_delta();
        }

        /*
         * WR36345: if BAB overflow during refresh, restart refresh
         */
        if (chk_baboverflow(group) == 1) {
            reporterr(ERRFAC, M_BABOFLOW2, ERRWARN, lgnum);
            _pmd_refresh = 1; /* indicates that an overflow occurred */
            _pmd_state_change = 1;

            // WI_338550 December 2017, implementing RPO / RTT
            // Invalidate RTT sequencer tag (as is done on Windows)
            SequencerTag_Invalidate(&group->RTTComputingControl.iSequencerTag);

            // Smarter Smart-Refresh
            // =====================
            // Since we clear each bit of our local HRDB every time we acknowledge a refresh packet,
            // at exit we can update the driver hrdb with our local hrdb and merge it with the 
            // historical hrdb (map.next) that was initialise and start at the begining of 
            // Smart-Refresh to make sure we do not miss any bits during Smart Refresh.
            if (ftd_lg_merge_hrdbs(group, lgnum) != 0)
            {
                reporterr(ERRFAC, M_MERGE_FAILED, ERRWARN);
            }
            
            ++BAB_oflow_counter;    /* WR 43926 */
            return 2;
        }
    }
    /* flush deltas - one last time */
    if (chksum_flag) {
        flush_delta();
    }

    /* if there is no BAB overflow:
     *    2nd phase of full-refresh: send FTDRFDF end command.
     */
    if (fullrefreshphase == 0 && chk_baboverflow(group) != 1) {
        /* insert a go-coherent msg into bab */
        if (!rmd64)
        {
            send_rfd_co(lgnum, FTDRFD);
        }
    } else if (chk_baboverflow(group) != 1) {
        sendrsyncend(lgnum, FTDRFDF);
    }
    sleeptime = 500000;
    timeslept = 0;

    /* Skip the following if we're in the 2nd phase of the full refresh
       (since we aren't journalling) otherwise loop forever waiting for
        MSG_CO from RMD or a state change.
       If BAB overflowed, we do not do sendrfdend, hence, no ACK expected.
    */
    if (chk_baboverflow(group) != 1) {
      while (!fullrefreshphase) {
        if (net_writable) {
            empty_bab();
        }
        if (_pmd_state_change) {
            return 0;
        }
        if (timeslept == 0) {
            /*
             *Changed the existing design wherein INCO/CO sentinels are sent
             *through BAB to a new design wherein INCO/CO sentinels are sent directly
             *through socket. This change was implemented to avoid the problem of PMD
             *not being able to put INCO/CO into the BAB because of BAB overflow.
             */
            if (rmd64)
            {
                sendrfdend();
            }

        }
        /* if g_rfddone is set then a MSG_CO was ACK'd by the RMD */
        process_acks();
        if (g_rfddone) {
            break;
        }
        usleep(sleeptime);
        if ((timeslept += sleeptime) >= 120*sleeptime) {
            syslog(LOG_WARNING, "[%s]: no ack for MSG_CO", argv0);
            break;
        }
      } /* while ... */
    }
    refresh_started = 0;

    if (fullrefreshphase == 0) {
        reporterr(ERRFAC, M_RFDEND, ERRINFO, argv0);
    } else {
        reporterr(ERRFAC, M_RFDPHASE2END, ERRINFO, argv0);
		// Record FullRefresh completion in the Product Usage tracking file
        if( (update_product_usage_tracking_file( mysys, lgnum, 0, 1, REGISTER_DEVICES_AT_FR_END )) < 0 )
		{
            reporterr(ERRFAC, M_PRODUSE_FEND, ERRWARN, lgnum);
		}
    }
    // WI_338550 December 2017, implementing RPO / RTT
    ftd_lg_set_consistency_point_reached(group);
    // Give RPO stats to the driver
    give_RPO_stats_to_driver( group );

    /* force a final net kbps dump to driver memory */
    for (sd = group->headsddisk; sd; sd = sd->n) {
        sd->stat.rfshoffset = rsync->size;
        sd->stat.rfshdelta = rsync->delta;
        /* clear out internal rsync stats */
        sd->rsync.offset = 0;
        sd->rsync.ackoff = 0;
        sd->rsync.delta = 0;
    }
    savestats(1, _net_bandwidth_analysis);

    /*
     * WR37215: Fix timing window where PMD sets the driver to NORMAL mode
     */
    if (rsynccleanup(_pmd_state)) {
            _pmd_state_change = 1;
            return 2;
    }


    /* Only transition to FTDPMD (normal) mode if no BAB overflow ocurred.
     * If BABOFLOW occurred, PMD remains in FTDRFD (smart refresh) mode.
     * Another round of smart refresh will be kick off.
     *
     * 2nd phase of full refresh has identical checking.
     */
    if (fullrefreshphase != 1 && chk_baboverflow(group) == 1) {
        /* bab overforwed, retain the existing smart refresh medhphase != 1 */
        reporterr(ERRFAC, M_BABOFLOW2, ERRWARN, lgnum);
        ++BAB_oflow_counter;   /* WR 43926 */
        _pmd_refresh = 1; /* indicates overflow occurred */
        // We're not able to clean up and merge the bitmaps at this point for these reasons:
        // -) The call to rsynccleanup() puts the driver in normal state which turns off the DualBitmaps mechanism.
        // -) The ack offsets have been reset to 0.

        // WI_338550 December 2017, implementing RPO / RTT
        // Invalidate RTT sequencer tag (as is done on Windows)
        SequencerTag_Invalidate(&group->RTTComputingControl.iSequencerTag);
    } else {
        /* no bab overflow, transition to Normal mode */
        _pmd_state = FTDPMD;
        /* After a transition from Refresh to Normal, we initiate the refresh mechanism for the bitmaps. */
        initiate_bitmap_refresh(lgnum, group); // Not checking return code, since there's no clear way to report any error,
                                               // and that a failure cannot cause any real harm.
    }

    return 0;
} /* refresh */

/****************************************************************************
 * backfresh -- get data from secondary device(s) and write to primary
 ***************************************************************************/
int
backfresh (void)
{
#ifndef SUPPORT_BACKFRESH
    reporterr(ERRFAC, M_BKFRSH_NOTAVAIL, ERRINFO);
    return( -1 );
#endif
    group_t *group;
    sddisk_t *sd;
    rsync_t *rsync = NULL;
    static time_t currts = 0;
    static time_t last_stat_ts = 0;
    u_longlong_t bytesleft;
    int deltatime;
    int digestlen;
    int devcnt;
    int moredevs;
    int cs_num;
    int lgnum;
    int len;
    int num;
    int rc;

    group = mysys->group;

    if ((rc = rsyncinit(group, 0,0)) != FTD_OK) {
        return rc;
    }
    lgnum = atoi(_pmd_configpath+strlen(_pmd_configpath)-3);
    devcnt = moredevs = rsynccnt;

    /* backfresh all devices for the logical group */
    while (moredevs) {
        if (_pmd_state_change) {
            return 0;
        }
        gettunables(group->devname, 0, 0);

        init_buffer(mysys->tunables.chunksize);
        cs_num = (mysys->tunables.chunksize/MINCHK)+2;
        digestlen = 2*(cs_num*DIGESTSIZE);
        
        if (mysys->tunables.chunkdelay > 0)
        {
            usleep(mysys->tunables.chunkdelay*1000);
        }        
        
        for (sd = group->headsddisk; sd; sd = sd->n) {
            rsync = &sd->rsync;
            rsync->data = buf;
            if (digestlen > rsync->cs.digestlen) {
                rsync->cs.digest = ftdrealloc(rsync->cs.digest, digestlen);
            }
            rsync->cs.digestlen = digestlen;

            DPRINTF("\n*** backfresh: offset, ackoff, delta, size = %d, %d, %d, %d\n", rsync->offset, rsync->ackoff,  rsync->delta, rsync->size);

            if (rsync->offset >= rsync->size) {
                if (!rsync->done && rsync->ackoff >= rsync->size) {
                    sd->stat.rfshoffset = rsync->size;
                    savestats(1, _net_bandwidth_analysis);
                    moredevs--;
                    rsync->done = 1;
                } else {
                    process_acks();
                }
                continue;
            }
            rsync->datalen = mysys->tunables.chunksize;
            bytesleft = (rsync->size - rsync->offset);
            bytesleft <<= DEV_BSHIFT;
            if (bytesleft < rsync->datalen) {
                rsync->datalen = bytesleft;
            }
            rsync->length = (rsync->datalen >> DEV_BSHIFT);
            DPRINTF("\n*** bytesleft, datalen, rsync->length = %llu, %d, %d\n",  bytesleft,rsync->datalen,rsync->length);

            num = (rsync->datalen%MINCHK) ? (rsync->datalen/MINCHK)+1:
                (rsync->datalen/MINCHK);
            rsync->cs.cnt = 2;
            rsync->cs.num = num;
            rsync->cs.segoff = rsync->offset;
            rsync->cs.seglen = rsync->datalen;

            /* compute checksums */
            if ((rc = chksumseg(rsync)) == -1) {
                EXIT(EXITANDDIE);
            }
            sd->stat.rfshoffset = rsync->ackoff;
            sd->stat.rfshdelta = rsync->delta;
            sd->stat.e_datacnt += rsync->datalen;
            sd->stat.e_tdatacnt += rsync->datalen;

            rsync->offset += (rsync->datalen >> DEV_BSHIFT);

            eval_netspeed();

            time(&currts);
            deltatime = (currts-last_stat_ts);
            if (deltatime >= 1) {
                savestats(0, _net_bandwidth_analysis);
                last_stat_ts = currts;
            }
        }
        /* request remote checksums - just this segment */
        sendbfdchk();
        process_acks();
    }
    sendrsyncend(lgnum, _pmd_state);
    // WI_338550 December 2017, implementing RPO / RTT
    give_RPO_stats_to_driver( group );
    
    reporterr(ERRFAC, M_BFDEND, ERRINFO, argv0);

    /*to overcome mounting problem after backfresh*/
#if defined(linux)
    (void)sync();
#endif /* defined(linux) */
    /* force a final net kbps dump to driver memory */
    for (sd = group->headsddisk; sd; sd = sd->n) {
        sd->stat.rfshoffset = rsync->ackoff;
        sd->stat.rfshdelta = rsync->delta;
        /* clear out internal rsync stats */
        sd->rsync.offset = 0;
        sd->rsync.ackoff = 0;
        sd->rsync.delta = 0;
    }
    savestats(1, _net_bandwidth_analysis);
    rsynccleanup(_pmd_state);

    return 0;
} /* backfresh */

/****************************************************************************
 * send_rfd_co -- write refresh-end entry to BAB
 ***************************************************************************/
int
send_rfd_co(int lgnum, int state)
{
    group_t *group;
    stat_buffer_t sb;
    char msgbuf[DEV_BSIZE];
    int maxtries;
    int rc;
    int i;

    group = mysys->group;
    memset(&sb, 0, sizeof(stat_buffer_t));

    if (state == FTDRFD) {
        sprintf(msgbuf, "%s|%s", MSG_CO, "");
    } else if (state == FTDRFDF) {
        sprintf(msgbuf, "%s|%s", MSG_CO, "");
    }

    sb.lg_num = lgnum;
    sb.len = sizeof(msgbuf);
    sb.addr = (ftd_uint64ptr_t)(unsigned long)msgbuf;

    maxtries = 10;
    for (i = 0; i < maxtries; i++) {
        rc = FTD_IOCTL_CALL(group->devfd, FTD_SEND_LG_MESSAGE, &sb);
        if (rc != 0) {
            if (errno == EAGAIN) {
                usleep(200000);
                continue;
            }
            reporterr(ERRFAC, M_SENDLGMSG, ERRCRIT, get_error_str(errno));
            EXIT(EXITANDDIE);
        } else {
            break;
        }
    }
    return 0;
} /* send_rfd_co */

/****************************************************************************
 * send_rfd_inco -- write refresh-start entry to BAB
 ***************************************************************************/
int
send_rfd_inco(int lgnum)
{
    group_t *group;
    stat_buffer_t sb;
    char msgbuf[DEV_BSIZE];
    int maxtries;
    int rc;
    int i;

    group = mysys->group;
    memset(&sb, 0, sizeof(stat_buffer_t));

    if (_pmd_state == FTDRFD) {
        sprintf(msgbuf, "%s|%s", MSG_INCO, "");
    } else if (_pmd_state == FTDRFDF) {
        sprintf(msgbuf, "%s|%s", MSG_INCO, "");
    }
    sb.lg_num = lgnum;
    sb.len = sizeof(msgbuf);
    sb.addr = (ftd_uint64ptr_t)(unsigned long)msgbuf;

    maxtries = 10;
    for (i = 0; i < maxtries; i++) {
        rc = FTD_IOCTL_CALL(group->devfd, FTD_SEND_LG_MESSAGE, &sb);
        if (rc != 0) {
            if (errno == EAGAIN) {
                usleep(200000);
                continue;
            }
            reporterr(ERRFAC, M_SENDLGMSG, ERRCRIT, get_error_str(errno));
            EXIT(EXITANDDIE);
        } else {
            break;
        }
    }
   return 0;
} /* send_rfd_inco */
