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
#include "errors.h"
#include "config.h"
#include "network.h"
#include "pathnames.h"
#include "procinfo.h"
#include "process.h"
#include "common.h"
#include "md5.h"
#include "ps_intr.h"
#include "aixcmn.h"

#include <limits.h>

ioqueue_t *fifo;
int rsynccnt;
int refresh_started;
int refresh_oflow;

extern char *compress_buf;
extern int compress_buf_len;

extern char *paths;
extern int lgcnt;
extern int _pmd_state;
extern int _pmd_cpon;
extern int _pmd_state_change;
extern char *_pmd_configpath;
extern int exitsuccess;
extern int exitfail;
extern char *argv0;

extern int g_rfddone;
extern int net_writable;

int _pmd_verify_secondary;

int _rmd_jrn_state;
int _rmd_jrn_mode;
int _rmd_jrn_num;

int chksum_flag;

time_t ts; 
struct tm *tim;

static struct iovec *iov = NULL;
static int iovlen = 0;

static dirtybit_array_t db[1];
static int sendblock(rsync_t *rsync);

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
	intp = (int *)db->dbbuf;
	for(i=0; i < numdevs ;i++) {
		dbk.dev = (ftd_uint32_t)db->devs[i];
		dbk.dbbuf =  (ftd_uint64ptr_t) intp;
		switch(ioc) {
		case FTD_GET_HRDBS:
		case FTD_SET_HRDBS:
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
		intp += dev[i].hrdbsize32;
	}
	return(0);
}

/****************************************************************************
 * get_hires_dirtybits -- get all high res dirtybit maps for group
 ***************************************************************************/
int 
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
    sb.addr = (ftd_uint64ptr_t)dev;
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
    if (db->dbbuf == NULL) {
        db->dbbuf = (ftd_uint64ptr_t)ftdmalloc(len*sizeof(int));
    }
    db->numdevs = lgp->ndevs;
    db->state_change = 0;

    rc = dbarr_ioc(group->devfd, FTD_GET_HRDBS, db, dev);
    if (rc != 0) {
        free(dev);
        free(db->devs);
        free((char *)db->dbbuf);
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_HRDBS", strerror(errno));
        return -1;
    }
    /* point our devices at dbarray */
    for (sd = group->headsddisk; sd; sd = sd->n) {
        /* find device in dbarray dev list */
        maskoff = 0;
        DPRINTF("\n*** db->numdevs = %d\n",db->numdevs);
        for (i = 0; i < db->numdevs; i++) {
            devp = dev+i;
            DPRINTF("\n*** sd->rdev, devp->cdev = %08x, %08x\n",                sd->sd_rdev,devp->cdev);
            if (devp->cdev == sd->sd_rdev) {
                break;
            }
            maskoff += devp->hrdbsize32; 
        }
        if (i == db->numdevs) {
            free(dev);
            free(db->devs);
            free((char *)db->dbbuf);
            reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, (int) sd->sd_rdev);
            return -1;
        }
        sd->dm.len = devp->hrdbsize32*sizeof(int)*8;
        sd->dm.res = 1<<devp->hrdb_res;
        sd->dm.bits = devp->hrdb_numbits;
        sd->dm.bits_used = 0;
        sd->dm.mask = (unsigned char*)db->dbbuf+(maskoff*sizeof(int));
#ifdef TDMF_TRACE
        fprintf(stderr,"\n*** get_highres: found device: %s\n",sd->devname);
        fprintf(stderr,"\n*** maskoff = %d\n",maskoff);
        fprintf(stderr,"\n*** devp->hrdbsize32 = %d\n",devp->hrdbsize32);
        fprintf(stderr,"\n*** devp->hrdb_res = %d\n",devp->hrdb_res);
        fprintf(stderr,"\n*** devp->disksize = %d\n",devp->disksize);
        fprintf(stderr,"\n*** devp->hrdb_numbits = %d\n",devp->hrdb_numbits);
#endif
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
    for (i = 0; i < db->numdevs; i++) {
        devp = dev+i;
        masklen += devp->hrdbsize32; 
    }
    chksum_flag = 1;
    for (i = 0; i < masklen-1; i++) {
        if (~*((unsigned int *)
               ((unsigned char *)db->dbbuf + (i*sizeof(unsigned int))))) {
            chksum_flag = 0;
            break;
        }
    }
/*
*/
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
    ftd_dev_info_t *dev, *devp;
    stat_buffer_t sb;
    dirtybit_array_t ldb[1];
    int len;
    int rc;
    int i;

    dev = (ftd_dev_info_t*)ftdmalloc(lgp->ndevs*sizeof(ftd_dev_info_t));
    sb.lg_num = lgnum;
    sb.addr = (ftd_uint64ptr_t)dev;
    rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_DEVICES_INFO, &sb);
    if (rc != 0) {
        free(dev);
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_DEVICES_INFO", 
            strerror(errno));
        return -1;
    }
    memset(ldb, 0, sizeof(dirtybit_array_t));
    ldb->devs = (dev_t *)ftdmalloc(lgp->ndevs*sizeof(dev_t));
    len = 0;
    for (i = 0; i < lgp->ndevs; i++) {
        ldb->devs[i] = dev[i].cdev;
        len += dev[i].hrdbsize32;
    } 
    ldb->numdevs = lgp->ndevs;
    ldb->dblen32 = len;
    ldb->dbbuf = (ftd_uint64ptr_t)ftdmalloc(len*sizeof(int));
    memset((unsigned char*)ldb->dbbuf, 0xff, len*sizeof(int));
    rc = dbarr_ioc(group->devfd, FTD_SET_LRDBS, ldb, dev);
    if (rc != 0) {
        free(dev);
        free(ldb->devs);
        free((char *)ldb->dbbuf);
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "SET_LRDBS", strerror(errno));
        return -1;
    }
    rc = dbarr_ioc(group->devfd, FTD_SET_HRDBS, ldb, dev);
    if (rc != 0) {
        free(dev);
        free(ldb->devs);
        free((char *)ldb->dbbuf);
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "SET_HRDBS", strerror(errno));
        return -1;
    }
    free(dev);
    free(ldb->devs);
    free((char *)ldb->dbbuf);

    return 0;
} /* set_dirtybits */

/****************************************************************************
 * empty_bab -- send any BAB entries to remote
 ***************************************************************************/
void
empty_bab(int pempty)
{
    dispatch(NULL);

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
    devstat_t *devp;
    rsync_t *rsync;
    ftd_lg_info_t lginfo, *lgp;
    int lgnum;
    int cs_num;
    int rc;

    rsync = &sd->rsync;
    memset(rsync, 0, sizeof(rsync_t));

    rsync->devid = sd->devid;
    rsync->size = disksize(sd->devname);

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
    } else if (_pmd_state == FTDRFDF) {
    }
    /* open the raw device */
    if ((rsync->devfd = open(sd->devname, O_RDWR)) == -1) {
        if (errno == EMFILE) {
            reporterr(ERRFAC, M_FILE, ERRCRIT,
                sd->devname, strerror(errno));
        }
        reporterr(ERRFAC, M_OPEN, ERRCRIT, sd->devname, strerror(errno));
        return -1;
    }
#ifdef SOLARIS
    /* check for meta-device */
#ifdef PURE
    memset(&dkinfo, 0, sizeof(dkinfo));
    memset(&dkvtoc, 0, sizeof(dkvtoc));
#endif
    if (FTD_IOCTL_CALL(rsync->devfd, DKIOCINFO, &dkinfo) < 0) {
        return -1;
    }
    if (FTD_IOCTL_CALL(rsync->devfd, DKIOCGVTOC, &dkvtoc) < 0) {
        return -1;
    }
    if (dkinfo.dki_ctype != DKC_MD
    && dkvtoc.v_part[dkinfo.dki_partition].p_start == 0) {
        DPRINTF( "Sector 0 write dissallowed for device: %s\n",             sd->devname);
        sd->no0write = 1;
    }
#endif
    DPRINTF("\n*** rsync->devname, devfd, offset, size = %s, %d, %d, %d\n",         sd->devname, rsync->devfd, rsync->offset, rsync->size);

    /* tell RMD about device */
    if (_pmd_state == FTDBFD) {
        sendrsyncdevs(rsync, FTDBFD);
    }
    rsynccnt++;

    return 0;
} /* rsyncinit_dev */

/****************************************************************************
 * rsyncinit -- initialize rsync 
 ***************************************************************************/
int 
rsyncinit(group_t *group) 
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

    lgnum = cfgpathtonum(mysys->configpath);
    FTD_CREATE_GROUP_NAME(group_name, lgnum);
 
    refresh_started = 0;
    refresh_oflow = 0;
    chksum_flag = 0;

    if (firsttime) {
        memset(db, 0, sizeof(dirtybit_array_t)); 
        firsttime = 0;
    }
    /* get group info */
    memset(&sb, 0, sizeof(stat_buffer_t));
    sb.lg_num = lgnum;
    sb.len = sizeof(ftd_stat_t);
    sb.addr = (ftd_uint64ptr_t)&lgstat;
    rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_GROUP_STATS, &sb);
    if (rc != 0) {
        reporterr(ERRFAC, M_DRVERR, ERRCRIT, "GET_GROUP_STATS", 
            strerror(errno));
        return -1;
    }
    if (_pmd_state == FTDRFD) {
        switch(lgstat.state) {
        case FTD_MODE_NORMAL:
        case FTD_MODE_PASSTHRU:
            /* kick driver into REFRESH mode */
            stb.lg_num = lgnum;
            stb.state = FTD_MODE_REFRESH;
            rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_GROUP_STATE, &stb);
            if (rc != 0) {
                reporterr(ERRFAC, M_DRVERR, ERRCRIT, "SET_GROUP_STATE", 
                    strerror(errno));
                return -1;
            }
       ps_set_group_state(mysys->pstore, group_name, stb.state);
         
            if (lgstat.state == FTD_MODE_NORMAL) {
                if (_pmd_verify_secondary) {
                    /* refresh -c */
                    if ((rc = set_dirtybits(group, lgnum, &lgstat)) == -1) {
                        return -1;
                    }
                    _pmd_verify_secondary = 0;
                }
            }
            /* all bits will be set */
            if ((rc = get_hires_dirtybits(group, lgnum, &lgstat)) == -1) {
                return -1;
            }
            break;
        case FTD_MODE_REFRESH:
            /* driver was previously in refresh mode */
            if ((rc = get_hires_dirtybits(group, lgnum, &lgstat)) == -1) {
                return -1;
            }
            break;
        case FTD_MODE_TRACKING:
            empty_bab(0);
            /* kick driver into REFRESH mode */
            stb.lg_num = lgnum;
            stb.state = FTD_MODE_REFRESH;
            rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_GROUP_STATE, &stb);
            if (rc != 0) {
                reporterr(ERRFAC, M_DRVERR, ERRCRIT, "SET_GROUP_STATE", 
                    strerror(errno));
                return -1;
            }
        ps_set_group_state(mysys->pstore, group_name, stb.state);
 
            if (_pmd_verify_secondary) {
                /* refresh -c */
                if ((rc = set_dirtybits(group, lgnum, &lgstat)) == -1) {
                    return -1;
                }
                _pmd_verify_secondary = 0;
            }
            if (get_hires_dirtybits(group, lgnum, &lgstat) == -1) {
                return -1;
            }
            /*
             * set this flag so that if we overflow while emptying the
             * BAB the correct thing happens. The state machine in 
             * this app is in dire need of some work.
             */
            refresh_oflow = 1;
            break;
        default:
            reporterr(ERRFAC, M_PSBADSTATE, ERRCRIT, lgstat.state);
            return -1;
        } 
        /* insert a marker into bab to tell RMD to go INCO */
        send_rfd_inco(lgnum);
        /* the INCO marker needs sent/ACK'd */
        empty_bab(0);
        refresh_started = 1;
    } else if (_pmd_state == FTDRFDF) {
        /* kick driver into refresh mode */
        stb.lg_num = lgnum;
        stb.state = FTD_MODE_REFRESH;
        rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_GROUP_STATE, &stb);
        if (rc != 0) {
            reporterr(ERRFAC, M_DRVERR, ERRCRIT, "SET_GROUP_STATE", 
                strerror(errno));
            return -1;
        }
        ps_set_group_state(mysys->pstore, group_name, stb.state);
        
        sendrsyncstart(_pmd_state);
        if (flush_bab(mysys->ctlfd, lgnum) < 0) {
            /*
             * can't flush so empty - this will cause
             * us to send redundant blocks (ie. bab + all dev blocks)
             */
            empty_bab(0); 
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
        return -1;
    }
    /* dump initial stats to driver */
    savestats(1);

    return 0;
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
        free((char *)db->dbbuf);
        db->dbbuf = NULL;
    }
    memset(&sb, 0, sizeof(stat_buffer_t));
    sb.lg_num = lgnum;
    sb.len = sizeof(ftd_lg_info_t);
    sb.addr = (ftd_uint64ptr_t)&lginfo;
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
        sb.addr = (ftd_uint64ptr_t)devbuf;
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
            free(sd->rsync.cs.digest);
        }
        if (sd->rsync.deltamap.refp) {
            free(sd->rsync.deltamap.refp);
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

    /* reset driver mode */
    stb.lg_num = lgnum;
    stb.state = FTD_MODE_NORMAL;
    rc = FTD_IOCTL_CALL(mysys->ctlfd, FTD_SET_GROUP_STATE, &stb);
    if (rc != 0) {
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
    return 0;
} /* rsynccleanup */

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
        exit (EXITANDDIE);
    }
    DPRINTF("\n*** readblock: [%s] reading %d bytes @ offset %llu\n",      argv0, length, offset);

    if (llseek(rsync->devfd, offset, SEEK_SET) == -1) {
        if (ISPRIMARY(mysys)) {
            reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
                argv0, sd->devname, offset, strerror(errno));
        } else {
            reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
                argv0, sd->mirname, offset, strerror(errno));
        }
        exit (EXITANDDIE);
    }
    if ((devbytes = ftdread(rsync->devfd, rsync->data, length)) == -1) {
        if (ISPRIMARY(mysys)) {
            reporterr(ERRFAC, M_READERR, ERRCRIT,
                sd->devname, offset, length, strerror(errno));
        } else {
            reporterr(ERRFAC, M_READERR, ERRCRIT,
                sd->mirname, offset, length, strerror(errno));
        }
        exit (EXITANDDIE);
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

    DPRINTF(  "\n*** dirtyseg: res, offset, length %d, [%llu-%llu]\n",	         res, offset, length);

    rc = 0;

    if (res == 0) {
        return -1;
    }

    /* scan this 'segment' of bits */
    endbyte = (u_longlong_t)(offset+length-1);
    startbit = (u_longlong_t)(offset/res);
    endbit = (u_longlong_t)(endbyte/res); 

    DPRINTF(        "\n*** offset, endbyte, startbit, endbit = %llu, %llu, %llu, %llu\n",         offset, endbyte, startbit, endbit);

    for (i = startbit; i <= endbit; i++) {
        /* find the word */
        DPRINTF("\n*** i, word = %llu, %llu\n",i, (i>>5));
        wp = (unsigned int*)((unsigned int*)mask+(i>>5));

        /* test the bit */
#if defined(_AIX)
        	if((*wp) & (0x00000001 << (i % (sizeof(*wp) * NBPB))))
#else /* defined(_AIX) */
        if ((*wp)&(0x01<<(i&0xffffffff)))
#endif /* defined(_AIX) */
        {
            rc = 1;
            break;
        }

    }
    DPRINTF( "\n*** dirtyseg: returning %d\n",      rc);

    return rc;
} /* dirtyseg */

/****************************************************************************
 * rsyncwrite_bfd -- backfresh - write to data device 
 ***************************************************************************/
int
rsyncwrite_bfd(rsync_t *lrsync)
{
    sddisk_t *sd;
    char *devname;
    u_longlong_t offset;
    u_longlong_t length;
    int len32;
    int rc;
    int devbytes;
    int fd;

    if ((sd = getdevice(lrsync)) == NULL) {
        reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, lrsync->devid); 
        exit (EXITANDDIE);
    }
    fd = sd->rsync.devfd;
    devname = sd->devname;

    devbytes = 0;
    offset = lrsync->offset;
    offset <<= DEV_BSHIFT;
    length = lrsync->datalen;

    DPRINTF("\n*** rsyncwrite_bfd: writing %llu bytes @ offset: %llu\n",         length, offset);
    if (offset == 0 && sd->no0write) {
        DPRINTF("\n*** skipping sector 0 write for device: %s\n",            sd->devname);
        offset += DEV_BSIZE;
        length -= DEV_BSIZE;
        lrsync->data += DEV_BSIZE;
    } 
    if (llseek(fd, offset, SEEK_SET) == -1) {
        reporterr(ERRFAC, M_SEEKERR, ERRCRIT,
            argv0, devname, offset, strerror(errno));
        exit (EXITANDDIE);
    }
    len32 = length;
    rc = ftdwrite(fd, lrsync->data, len32);
    if (rc != len32) {
        reporterr(ERRFAC, M_WRITEERR, ERRCRIT,
            argv0, devname, lrsync->devid,
            offset, length, strerror(errno));
        exit (EXITANDDIE);
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
        savestats(0);
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
            exit (EXITANDDIE);
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
            exit (EXITANDDIE);
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
    headpack_t header[1];
    int digestlen;
    int iovcnt;
    int rc;

    memset(header, 0, sizeof(header));

    header->magicvalue = MAGICHDR;
    header->cmd = CMDRFDCHKSUM;
    header->ackwanted = 1;

    if (iov == NULL) {
        iov = (struct iovec*)ftdmalloc(sizeof(struct iovec));
        iovlen = 1;
    }
    iov[0].iov_base = (void*)header;
    iov[0].iov_len = sizeof(headpack_t);

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
        iov[iovcnt-1].iov_base = (void*)rsync;
        iov[iovcnt-1].iov_len = sizeof(rsync_t);
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
            header->len = (iovcnt-1)/2;
            rc = net_write_vector(mysys->sock, iov, iovcnt, ROLEPRIMARY);
            iovcnt = 1;
        }
    }
    if (iovcnt > 1) {
        header->len = (iovcnt-1)/2;
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
    sddisk_t *sd;
    rsync_t *rsync;
    headpack_t header[1];
    int digestlen;
    int iovcnt = 0;
    int rc;

    memset(header, 0, sizeof(header));

    header->magicvalue = MAGICHDR;
    header->cmd = CMDBFDCHKSUM;
    header->ackwanted = 1;

    if (iov == NULL) {
        iov = (struct iovec*)ftdmalloc(sizeof(struct iovec));
        iovlen = 1;
    }
    iov[0].iov_base = (void*)header;
    iov[0].iov_len = sizeof(headpack_t);
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
        iov[iovcnt-1].iov_base = (void*)rsync;
        iov[iovcnt-1].iov_len = sizeof(rsync_t);
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
            header->len = (iovcnt-1)/2;
            rc = net_write_vector(mysys->sock, iov, iovcnt, ROLEPRIMARY);
            iovcnt = 1;
        }
    }
    if (iovcnt > 1) {
        header->len = (iovcnt-1)/2;
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
int
block_all_zero(rsync_t *rsync)
{
    int i;

    for (i = 0; i < rsync->datalen; i++) {
        if (rsync->data[i]) {
            return 0;
        }
    }
    return 1;
} /* block_all_zero */

/****************************************************************************
 * sendblock -- send a data block to remote 
 ***************************************************************************/
static int
sendblock(rsync_t *rsync)
{
    headpack_t header[1];
    ackpack_t ack[1];
    sddisk_t *sd;	
    char *datap;
    int length;
    int cnt;
    int rc;

    struct iovec iov[3];

    if ((sd = getdevice(rsync)) == NULL) {
        reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, rsync->devid); 
        exit (EXITANDDIE);
    }
    memset(header, 0, sizeof(header));
    memset(iov, 0, sizeof(struct iovec));

    header->magicvalue = MAGICHDR;
    header->cmd = CMDRFDFWRITE;
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
    header->ackwanted = 1;
    if (chksum_flag) {
        header->ackwanted = 0;
    }
/*
*/
    if (ISSECONDARY(mysys)) {
        /* precede header with an ACK */
        memset(ack, 0, sizeof(ackpack_t));
        ack->magicvalue = MAGICACK;
        ack->acktype = ACKNORM;
        ack->data = ACKBFDDELTA;
        ack->devid = header->devid;
        iov[0].iov_base = (void*)ack;
        iov[0].iov_len = sizeof(ackpack_t);
        iov[1].iov_base = (void*)header;
        iov[1].iov_len = sizeof(headpack_t);
        cnt = 3;
    } else {
        iov[0].iov_base = (void*)header;
        iov[0].iov_len = sizeof(headpack_t);
        cnt = 2;
    }
    if (block_all_zero(rsync)) {
        header->data = BLOCKALLZERO;

        /* bump network data counts */
        sd->stat.a_datacnt += sizeof(headpack_t);
        sd->stat.a_tdatacnt += sizeof(headpack_t);

        rc = net_write_vector(mysys->sock, iov, cnt-1, ROLEPRIMARY);
    } else {
        if (mysys->tunables.compression) {
            /* compress data */
            if ((length = rsync->datalen) > compress_buf_len) {
                compress_buf_len = length + (length >> 1) + 1;
                compress_buf =
                (char*)ftdrealloc(compress_buf, compress_buf_len);
            }
            length = compress((u_char*)rsync->data,
                (u_char*)compress_buf, rsync->datalen);
            datap = compress_buf;
        } else {
            datap = rsync->data;
            length = rsync->datalen;
        } 
        header->compress = mysys->tunables.compression;
        header->len = length;

        iov[cnt-1].iov_base = (void*)datap;
        iov[cnt-1].iov_len = length;

        /* bump network data counts */
        sd->stat.a_datacnt += header->len;
        sd->stat.a_tdatacnt += header->len + sizeof(headpack_t);

        rc = net_write_vector(mysys->sock, iov, cnt, ROLEPRIMARY);
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

    memset(header, 0, sizeof(headpack_t));
    header->magicvalue = MAGICHDR;
    header->cmd = state = state == FTDBFD ? CMDBFDSTART: CMDRFDFSTART;
    header->ackwanted = 1;
    if (-1 == writesock(mysys->sock, (char*)header, sizeof(headpack_t))) {
        exit (exitfail);
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
    sddisk_t *sd;

    if ((sd = getdevice(rsync)) == NULL) {
        reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, rsync->devid); 
        exit (EXITANDDIE);
    }
    memset(header, 0, sizeof(header));
    header->magicvalue = MAGICHDR;
    header->cmd = state == FTDBFD ? CMDBFDDEVS: CMDRFDDEVS;
    header->devid = rsync->devid;
    header->offset = rsync->offset;
    header->data = sd->dm.res;
    header->ackwanted = 1;
    if (-1 == writesock(mysys->sock, (char*)header, sizeof(headpack_t))) {
        exit (exitfail);
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

    memset(header, 0, sizeof(header));
    header->magicvalue = MAGICHDR;
    header->cmd = state == FTDBFD ? CMDBFDDEVE: CMDRFDDEVE;
    header->devid = rsync->devid;
    header->ackwanted = 1;
    if (-1 == writesock(mysys->sock, (char*)header, sizeof(headpack_t))) {
        exit (exitfail);
    }
    return 0;
} /* sendrsyncdeve */

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
    sb.addr = (ftd_uint64ptr_t)msgbuf;

    maxtries = 10;
    for (i = 0; i < maxtries; i++) {
        rc = FTD_IOCTL_CALL(group->devfd, FTD_SEND_LG_MESSAGE, &sb);
        if (rc != 0) {
            if (errno == EAGAIN) {
                usleep(200000);
                continue;
            }
            reporterr(ERRFAC, M_SENDLGMSG, ERRCRIT);
            exit(EXITANDDIE);
        } else {
            break;
        }
    }
    return 0;
} /* send_rfd_inco */

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
    sb.addr = (ftd_uint64ptr_t)msgbuf;

    maxtries = 10;
    for (i = 0; i < maxtries; i++) {
        rc = FTD_IOCTL_CALL(group->devfd, FTD_SEND_LG_MESSAGE, &sb);
        if (rc != 0) {
            if (errno == EAGAIN) {
                usleep(200000);
                continue;
            }
            reporterr(ERRFAC, M_SENDLGMSG, ERRCRIT);
            exit(EXITANDDIE);
        } else {
            break;
        }
    }
    return 0;
} /* send_rfd_co */

/****************************************************************************
 * sendrsyncend -- tell RMD to take this logical group out of rsync mode
 ***************************************************************************/
int
sendrsyncend(int lgnum, int state) 
{
    headpack_t header[1];

    memset(header, 0, sizeof(header));
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
    if (-1 == writesock(mysys->sock, (char*)header, sizeof(headpack_t))) {
        exit (exitfail);
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

/****************************************************************************
 * refresh_full -- get data from data device(s) and write to mirror 
***************************************************************************/
int
refresh_full (void *args)
{
    group_t *group;
    sddisk_t *sd;
    rsync_t *rsync;
    static int buflen = 0;
    static char *buf = NULL;
    static time_t currts = 0;
    static time_t lastts = 0;
    static time_t last_stat_ts = 0;
    static int save_chunksize =0;
    int deltatime;
    int devcnt;
    int moredevs;
    int done;
    int lgnum;
    int devbytes;
    u_longlong_t rsyncsize;
    u_longlong_t bytesleft;
    int rc;

    group = mysys->group;

    if (rsyncinit(group) == -1) {
        return -1;
    }
    lgnum = atoi(_pmd_configpath+strlen(_pmd_configpath)-3);
    devcnt = moredevs = rsynccnt;
    done = 0;

    g_rfddone = 0;

    /* refresh all devices for the logical group */
    while (!done) {
        if (_pmd_state_change) {
            return 0;
        }
        time(&currts);
        deltatime = (currts-lastts);
        if (deltatime >= 1) {
            gettunables(group->devname, 0, 0);
            /* empty BAB here */
            (void)empty_bab(0);
            lastts = currts;
        }
        /* chunksize changed */
        if (mysys->tunables.chunksize > save_chunksize) {
            /* re-allocate data buffer */
            buflen = mysys->tunables.chunksize;
            buf = ftdrealloc(buf, buflen);
        }
        save_chunksize = mysys->tunables.chunksize;

        for (sd = group->headsddisk; sd; sd = sd->n) {
            rsync = &sd->rsync;
            rsync->data = buf;

            DPRINTF( "\n*** refresh_f: offset, ackoff, delta, size = %d %d, %d, %d\n",rsync->offset, rsync->ackoff, rsync->delta, rsync->size);

            if (rsync->offset >= rsync->size) {
                if (!rsync->done && rsync->ackoff >= rsync->size) {
                    sd->stat.rfshoffset = rsync->size;
                    savestats(1);
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
                exit (EXITANDDIE);
            }
            /* set length to actual length returned from read */
            rsync->datalen = devbytes;
            rsync->length = (devbytes >> DEV_BSHIFT);

            /* update stats before the write */
            sd->stat.rfshoffset = rsync->ackoff;
            sd->stat.rfshdelta = rsync->delta;
            sd->stat.e_datacnt += rsync->datalen;
            sd->stat.e_tdatacnt += rsync->datalen;

            sendblock(rsync);

            /* bump device offset */
            rsync->offset += rsync->length;

            eval_netspeed();
           
            time(&currts);
            deltatime = (currts-last_stat_ts);
            if (deltatime >= 1) {
                savestats(0);
                last_stat_ts = currts;
            }
        }
        process_acks();
    }
    reporterr(ERRFAC, M_RFDEND, ERRINFO, argv0);

    /* force a final net kbps dump to driver memory */
    for (sd = group->headsddisk; sd; sd = sd->n) {
        sd->stat.rfshoffset = rsync->size;
        sd->stat.rfshdelta = rsync->delta;
        /* clear out internal rsync stats */
        sd->rsync.offset = 0;
        sd->rsync.ackoff = 0;
        sd->rsync.delta = 0;
    }
    savestats(1);
    rsynccleanup(_pmd_state);

    _pmd_state = FTDPMD;

    return 0;
} /* refresh_full */

/****************************************************************************
 * refresh -- get data from data device(s) and write to mirror 
***************************************************************************/
int
refresh (void *args)
{
    group_t *group;
    sddisk_t *sd;
    rsync_t *rsync;
    static int buflen = 0;
    static char *buf = NULL;
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

    group = mysys->group;

    if (rsyncinit(group) == -1) {
        return -1;
    }
    lgnum = atoi(_pmd_configpath+strlen(_pmd_configpath)-3);
    devcnt = moredevs = rsynccnt;
    done = 0;

    g_rfddone = 0;

    /* refresh all devices for the logical group */
    while (!done) {
        if (_pmd_state_change) {
            return 0;
        }
        time(&currts);
        deltatime = (currts-lastts);
        if (deltatime >= 1) {
            gettunables(group->devname, 0, 0);
            /* empty BAB here */
            (void)empty_bab(0);
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
        /* chunksize changed */
        if (mysys->tunables.chunksize > buflen) {
            /* re-allocate data buffer */
            buflen = mysys->tunables.chunksize;
            buf = ftdrealloc(buf, buflen);
        }
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

            DPRINTF("\n*** refresh: offset, ackoff, delta, size = %d, %d, %d, %d\n", rsync->offset, rsync->ackoff, rsync->delta, rsync->size);

            if (rsync->offset >= rsync->size) {
                if (rsync->cs.dirtylen) {
                    process_seg(rsync);
                }
                if (!rsync->done && rsync->ackoff >= rsync->size) {
                    sd->stat.rfshoffset = rsync->size;
                    savestats(0);
                    moredevs--;
                    rsync->done = 1;
                } else {
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
                len = sd->dm.res;
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
                savestats(0);
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
        process_acks();
    }
    /* flush deltas - one last time */
    if (chksum_flag) {
        flush_delta();
    }
    /* insert a go-coherent msg into bab */
    send_rfd_co(lgnum, FTDRFD);

    sleeptime = 500000;
    while (1) {
        if (net_writable) {
            empty_bab(0);
        }
        if (_pmd_state_change) {
            return 0;
        }
        /* if g_rfddone is set then a MSG_CO was ACK'd by the RMD */ 
        process_acks();
        if (g_rfddone) {
            break;
        } 
        usleep(sleeptime);
        if ((timeslept += sleeptime) >= 20*sleeptime) {
            break;
        }
    }
    refresh_started = 0;
    reporterr(ERRFAC, M_RFDEND, ERRINFO, argv0);

    /* force a final net kbps dump to driver memory */
    for (sd = group->headsddisk; sd; sd = sd->n) {
        sd->stat.rfshoffset = rsync->size;
        sd->stat.rfshdelta = rsync->delta;
        /* clear out internal rsync stats */
        sd->rsync.offset = 0;
        sd->rsync.ackoff = 0;
        sd->rsync.delta = 0;
    }
    savestats(1);
    rsynccleanup(_pmd_state);

    _pmd_state = FTDPMD;

    return 0;
} /* refresh */

/****************************************************************************
 * backfresh -- get data from secondary device(s) and write to primary 
 ***************************************************************************/
int
backfresh (void *args)
{
    group_t *group;
    sddisk_t *sd;
    rsync_t *rsync;
    static int buflen = 0;
    static char *buf = NULL;
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

    if (rsyncinit(group) == -1) {
        return -1;
    }
    lgnum = atoi(_pmd_configpath+strlen(_pmd_configpath)-3);
    devcnt = moredevs = rsynccnt;

    /* backfresh all devices for the logical group */
    while (moredevs) {
        if (_pmd_state_change) {
            return 0;
        }
        gettunables(group->devname, 0, 0);

        if (mysys->tunables.chunksize > buflen) {
            /* re-allocate data buffer */
            buflen = mysys->tunables.chunksize;
            buf = ftdrealloc(buf, buflen);
        }
        cs_num = (mysys->tunables.chunksize/MINCHK)+2;
        digestlen = 2*(cs_num*DIGESTSIZE);
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
                    savestats(1);
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
                exit (EXITANDDIE);
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
                savestats(0);
                last_stat_ts = currts;
            }
        }
        /* request remote checksums - just this segment */
        sendbfdchk();
        process_acks();
    }
    sendrsyncend(lgnum, _pmd_state);
    reporterr(ERRFAC, M_BFDEND, ERRINFO, argv0);

    /* force a final net kbps dump to driver memory */
    for (sd = group->headsddisk; sd; sd = sd->n) {
        sd->stat.rfshoffset = rsync->ackoff;
        sd->stat.rfshdelta = rsync->delta;
        /* clear out internal rsync stats */
        sd->rsync.offset = 0;
        sd->rsync.ackoff = 0;
        sd->rsync.delta = 0;
    }
    savestats(1);
    rsynccleanup(_pmd_state);

    return 0;
} /* backfresh */
