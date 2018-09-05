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
 * rmd.c - FullTime Data Remote Mirroring Daemon
 *
 * (c) Copyright 1998 FullTime Software, Inc. All Rights Reserved
 *
 * This module implements the functionality for the RMD
 *
 * History:
 *   10/14/96 - Steve Wahl - original code
 *
 ***************************************************************************
 */
#ifdef NEED_BIGINTS
#include "bigints.h"
#endif

#include <stdlib.h>
#include <limits.h>
#include <signal.h>
#include "config.h"
#include "errors.h"
#include "network.h"
#include "license.h"
#include "licprod.h"
#include "common.h"
#include "ipc.h"
#include "process.h"
#include "stat_intr.h"
#include "pathnames.h"
#include "aixcmn.h"
#include "devcheck.h"
#ifdef TDMF_TRACE
FILE *dbgfd;
#endif
char *argv0 = "in.rmd";
char pgmname[128];
char *paths;

int _rmd_rfddone;

struct timeval skosh;

/* -- global variables */
rsync_t grsync;
ioqueue_t *fifo;

int rmdenvcnt;
int _rmd_sockfd;
char* _rmd_configpath;

#if defined(SOLARIS) /* 1LG-ManyDEV */
extern int dummyfd;
#endif
extern ftd_jrnpaths_t tmppaths;
extern ftd_jrnpaths_t *tjrnphp;

extern int _rmd_rpipefd;
extern int _rmd_wpipefd;
extern int _rmd_state;
extern int _rmd_apply;
extern int _rmd_jrn_fd;
extern ftd_jrnnum_t _rmd_jrn_cp_lnum;
extern int _rmd_jrn_state;
extern int _rmd_jrn_mode;
extern ftd_jrnnum_t _rmd_jrn_num;
extern int _rmd_cpstart;
extern int _rmd_cppend;
extern int _rmd_cpstop;
extern int _rmd_cpon;
extern int _rmd_jless;
extern u_longlong_t _rmd_jrn_offset;
extern u_longlong_t _rmd_jrn_size;

extern ftd_jrnpaths_t *jrnphp;

extern int max_elapsed_time;

extern int sendkill (int fd);
extern int chksumseg(rsync_t *rsync);
extern int q_put_syncrfd(ioqueue_t *fifo, headpack_t *header);
extern int q_put_data(ioqueue_t *fifo, headpack_t *header);
extern int q_put_datarfdf(ioqueue_t *fifo, headpack_t *header);
extern int q_put_syncbfd(ioqueue_t *fifo);
extern ioqueue_t * q_init (void);

static char msg[384];
static char fmt[384];
static int loc_datalen = 0;
static char *loc_databuf = NULL;

static sigset_t sigs;
static int lgnum;

/* WR 43376: to check if the RMD was shutdown uncleanly in the middle
   of a Full Refresh */
static int   unclean_shutdown;
extern int   remove_FR_watchdog_if_PMD_disconnets;
extern int   check_unclean_shutdown( int lgnum );
extern void  remove_unclean_shutdown_file( int lgnum );
extern void  create_unclean_shutdown_file( int lgnum );

static int   handshake_check_if_mirrors_mounted( char *return_devname, char *return_mountpoint );

extern void flag_corrupted_journal_and_stop_rmd(int lgnum);

/****************************************************************************
 * sig_handler -- RMD signal handler
 ***************************************************************************/
static void
sig_handler (int s)
{

    switch(s) {
    case SIGTERM:
        EXIT(EXITNORMAL);
    case SIGPIPE:
        EXIT(EXITNORMAL);
    case SIGUSR1:
        /* We come here at killrmds command; tell PMD to exit */
        sendkill(_rmd_sockfd);
        break;
    case SIGUSR2:
        /*
         * Toggle M_INTERNAL logging
         */
        log_internal = log_internal ? 0 : 1;
        break;
	case SIGQUIT:
	    // WR PROD7916
	    // Master daemon is telling us to quit. This normally comes from a PMD restart.
		// If we were in Full Refresh and the PMD did quit for some reason, we must
		// delete the Full Refresh watchdog file to avoid restarting the Full Refresh from 0%.
		// The fact that the PMD restarted makes it safe to restart Full Refresh where it
		// was interrupted since the PMD's recorded FR offset is one that has been acknowledged 
		// by the RMD.
        if( remove_FR_watchdog_if_PMD_disconnets )
		{
            remove_unclean_shutdown_file( lgnum );
            reporterr (ERRFAC, M_RMD_SIG_QUIT, ERRINFO, lgnum);
		}
	    // If in network bandwidth analysis mode, remove the fictitious config file
		if( _net_bandwidth_analysis )
		{
            remove_fictitious_cfg_files( lgnum, 0 );
		}

        EXIT(EXITNORMAL);
        break;
    default:
        break;
    }
    return;
} /* sig_handler */

/****************************************************************************
 * installsigaction -- install signal actions for RMD
 ***************************************************************************/
static void
installsigaction(void)
{
    int i;
    int j;
    int numsig;
    struct sigaction caction;
    sigset_t block_mask;
    static int csignal[] = { SIGUSR1, SIGTERM, SIGPIPE, SIGUSR2, SIGQUIT };

    sigemptyset(&block_mask);
    /* numsig = sizeof(csignal)/sizeof(*csignal); */ /* way too cute */
    numsig = 5;
    for (i = 0; i < numsig; i++) {
        for (j = 0; j < numsig; j++) {
            sigaddset(&block_mask, csignal[j]);
        }
        caction.sa_handler = sig_handler;
        caction.sa_mask = block_mask;
        caction.sa_flags = SA_RESTART;
        sigaction(csignal[i], &caction, NULL);
    }
    return;
} /* installsigaction */

/****************************************************************************
 * doauth -- RMD verify authentication, then open and process config file
 ***************************************************************************/
int
doauth (authpack_t* auth, time_t* ts)
{
    char realhostname[256];
    char localhost[256];
    mysys->isconnected = 1;
    mysys->sock = _rmd_sockfd;

    if (0 != initnetwork ()) {
        return (-2);
    }
    strcpy(realhostname, othersys->name);

    if (auth->len > sizeof(auth->auth)) {
        return (-3);
    }

    decodeauth(auth->len, auth->auth, *ts, othersys->hostid, othersys->name, sizeof(othersys->name),RIP);

    if (0 != strcmp(othersys->name, realhostname)) {
        /*
         * WR34640:
         * Add more checking to avoid failure on loopback case
         */
        if ((strcmp(othersys->name, "127.0.0.1") != 0) &&
            (strcmp(realhostname, "127.0.0.1") != 0)) {
            return (-3);
        } else {
            gethostname(localhost, sizeof(localhost));
        }

        if (strcmp(othersys->name, "127.0.0.1") == 0) {
            if (strcmp(realhostname, localhost) == 0) {
                return (0);
            } else {
                return (-3);
            }
        }

        if (strcmp(realhostname, "127.0.0.1") == 0) {
            if (strcmp(othersys->name, localhost) == 0) {
                return (0);
            } else {
                return (-3);
            }
        }
        return (-3);
    }
    return (0);
} /* doauth */

/****************************************************************************
 * doconfiginfo -- RMD verify a mirror device, set its device id
 ***************************************************************************/
int
doconfiginfo (rdevpack_t* rdev, time_t ts)
{
    headpack_t header[1];
    sddisk_t* sd;
    time_t nowts;
    time_t tsbias;

    if (mysys == NULL) {
        return -1;
    }
    (void) time (&nowts);
    tsbias = nowts - ts;
    mysys->tsbias = tsbias;
    othersys->tsbias = tsbias;

    for (sd = mysys->group->headsddisk; sd; sd = sd->n) {
        if (0 == strcmp(rdev->path, sd->mirname)) {
            if (sd->devid != rdev->devid) {
                geterrmsg (ERRFAC, M_DEVIDERR, fmt);
                sprintf (msg, fmt, rdev->devid, sd->devname);
                logerrmsg (ERRFAC, ERRCRIT, M_DEVIDERR, msg);
                header->ts = nowts;
                (void)senderr(mysys->sock, header, 0L, ERRCRIT, M_DEVIDERR, msg);
                return -1;
            }
            sd->dd_rdev = rdev->dd_rdev;
            sd->sd_rdev = rdev->sd_rdev;
            return 1;
        }
    }
    return -1;
} /* doconfiginfo */

/****************************************************************************
 * cmd_handshake -- make sure PMD/RMD agree
 ***************************************************************************/
int
cmd_handshake(int fd, headpack_t *header)
{
    ackpack_t ack[1];
    authpack_t auth;
    char **lickey;
    int response;
    char devname[MAXPATH], mountp[MAXPATH];

    /* -- perform license checking */
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if (LICKEY_OK != (response = get_license (RMDFLAG, &lickey))) {
#elif defined(SOLARIS) /* 1LG-ManyDEV */
    if (LICKEY_OK != (response = get_license_manydev (RMDFLAG, &lickey))) {
#endif
        switch (response) {
        case LICKEY_FILEOPENERR:
            reporterr (ERRFAC, M_LICFILMIS, ERRCRIT, CAPQ);
            break;
        case LICKEY_BADKEYWORD:
            reporterr (ERRFAC, M_LICBADWORD, ERRCRIT, CAPQ);
            break;
	    case LICKEY_NOT_ALLOWED:
            reporterr (ERRFAC, M_LICNOTALLOW, ERRCRIT, CAPQ);
            break;
        default:
            reporterr (ERRFAC, M_LICFILE, ERRCRIT, CAPQ);
        }
        /* Tell PMD that this RMD does not have a valid license (WR 43583) */
        send_no_lic_msg(_rmd_sockfd);
        sleep( 5 );
        EXIT(EXITANDDIE);
    }
    if (lickey == NULL) {
        reporterr (ERRFAC, M_LICFILFMT, ERRCRIT, CAPQ);
        send_no_lic_msg(_rmd_sockfd);
        sleep( 5 );
        EXIT(EXITANDDIE);
    }
    if (LICKEY_OK != (response = check_key (lickey, np_crypt_key_rmd,
        NP_CRYPT_KEY_LEN))) {
        switch (response) {
        case LICKEY_NULL:
        case LICKEY_EMPTY:
            reporterr (ERRFAC, M_LICFILE, ERRCRIT, CAPQ);
            break;
        default:
            reporterr (ERRFAC, M_LICGENERR, ERRCRIT, PATH_CONFIG "/" CAPQ ".lic");
        }
        send_no_lic_msg(_rmd_sockfd);
        sleep( 5 );
        EXIT(EXITANDDIE);
    }
    /* -- license is ok, proceed */
    if (-1 == (response = readsock(fd, (char*)&auth, sizeof(authpack_t)))) {
        return (response);
    }
    if (0 > (response = doauth (&auth, &header->ts))) {
        switch (response) {
        case -1:
            /* config file problem */
            geterrmsg (ERRFAC, M_CFGPROB, msg);
            logerrmsg (ERRFAC, ERRCRIT, M_CFGPROB, msg);
            (void) senderr (fd, header, 0L, ERRCRIT, M_CFGPROB, msg);
            EXIT(EXITANDDIE);
        case -2:
            /* init network problem */
            geterrmsg (ERRFAC, M_NETPROB, msg);
            logerrmsg (ERRFAC, ERRCRIT, M_NETPROB, msg);
            (void) senderr (fd, header, 0L, ERRCRIT, M_NETPROB, msg);
            EXIT(EXITNETWORK);
        case -3:
            /* authentication error */
            geterrmsg (ERRFAC, M_BADAUTH, fmt);
            sprintf (msg, fmt, othersys->name);
            logerrmsg (ERRFAC, ERRCRIT, M_BADAUTH, msg);
            (void) senderr (fd, header, 0L, ERRCRIT, M_BADAUTH, msg);
            EXIT(EXITANDDIE);
        }
        EXIT(EXITANDDIE);
    }
    reporterr (ERRFAC, M_RMDSTART, ERRINFO, argv0);

    /* WR PROD4508 and HIST WR 38281: if checkpoint is off and a checkpoint transition 
       is not in progress, check if any device of this RMD is mounted. If so, send a message to 
       the PMD and tell the PMD to exit. We do not allow any replication activities while a target 
       device is mounted. But we do allow mounting target devices in checkpoint mode: in this case 
       we would not check.
	*/
    if( !_rmd_cpon && !_rmd_cppend )
	{
        /* Not in checkpoint mode nor transitionning to checkpoint. 
           We refuse replication activities if target devices are mounted. */
        if( (handshake_check_if_mirrors_mounted( devname, mountp )) != 0 )
        {
            /* Log the error locally on the RMD */
            reporterr (ERRFAC, M_HSHK_TGTMOUNTED, ERRCRIT, devname, mountp );
            /* Send the error message to the PMD */
            if (geterrmsg (ERRFAC, M_HSHK_TGTMOUNTED, fmt) == 0)
            {
                sprintf (msg, fmt, devname, mountp);
            }
            else
            {
                /* Note: if this RMD code is delivered on an older release as a hotfix and the
                   errors.msg file is not delivered with it, protect against not finding the message 
                   in the error facility and failing the sprintf; if the message is not there, use 
                   the default message prepared by geterrmsg() with the message name included, 
                   which will indicate what happens (same as what reporterr() does).  */
                strcpy (msg, fmt);
            }
            (void) senderr (fd, header, 0L, ERRCRIT, M_HSHK_TGTMOUNTED, msg);
            EXIT(EXITANDDIE);
        }
	}
    ack->data = _rmd_cpon;
    ack->devid = header->devid;

    if (GET_LG_JLESS(mysys)) {
        reporterr(ERRFAC, M_JLESS, ERRINFO);
    }

    /* Check if a previous Full Refresh was uncleanly interrupted (system crash, power failure, etc...).
       If so, warn the PMD so that it can check on the commands that will be given by the user
       and decide if these commands are accepted or not. */
    ack->mirco = 0;
    if( unclean_shutdown )
    {
      ack->mirco = UNCLEAN_SHUTDOWN_RMD;
    }
    else if( check_corrupted_journal_flag(lgnum) )
    {
        ack->mirco = RMDA_CORRUPTED_JOURNAL;
    }

    if ((response = sendack(fd, header, ack)) == -1) {
        return -1;
    }
    return 0;
} /* cmd_handshake */

/*
 * cmd_rfdchksum -- refresh checksum
 */
int
cmd_rfdchksum(int fd, headpack_t *header)
{
    group_t *group;
    sddisk_t *sd;

    rsync_t lrsync[1];
    rsync32_t lrsync32[1];
    int response;
    int length;
    int digestlen;

    rsync_t *rsync;
    char *save_datap;
    int i;

    group = mysys->group;

    /* clear checksum state */
    for (sd = group->headsddisk; sd; sd = sd->n) {
        sd->rsync.cs.cnt = 0;
        sd->rsync.cs.num = 0;
        sd->rsync.cs.seglen = 0;
    }

    for (i = 0; i < header->len; i++) {
        /* read rsync data structure from the network */
        if (!pmd64)
        {
        	if (readsock(mysys->sock, (char*)lrsync32, sizeof(rsync32_t)) == -1) {
            		geterrmsg(ERRFAC, M_BADPROTO, msg);
            		logerrmsg(ERRFAC, ERRCRIT, M_BADPROTO, msg);
            		(void) senderr (fd, header, 0L, ERRWARN, M_BADPROTO, msg);
            		EXIT(EXITNETWORK);
        	}
	        memset(lrsync, 0, sizeof(rsync_t));
                convertrsyncfrom32to64(lrsync32, lrsync);

        }
	else
        {
        	if (readsock(mysys->sock, (char*)lrsync, sizeof(rsync_t)) == -1) {
            		geterrmsg(ERRFAC, M_BADPROTO, msg);
            		logerrmsg(ERRFAC, ERRCRIT, M_BADPROTO, msg);
            		(void) senderr (fd, header, 0L, ERRWARN, M_BADPROTO, msg);
            		EXIT(EXITNETWORK);
        	}
        }
        if ((sd = get_lg_dev(group, lrsync->devid)) == NULL) {
            geterrmsg (ERRFAC, M_BADDEVID, fmt);
            sprintf (msg, fmt, lrsync->devid);
            logerrmsg (ERRFAC, ERRWARN, M_BADDEVID, msg);
            (void) senderr (fd, header, 0L, ERRWARN, M_BADDEVID, msg);
            return (-1);
        }

        rsync = &sd->rsync;
        rsync->devid = lrsync->devid;
        rsync->offset = lrsync->cs.segoff;
        rsync->length = lrsync->cs.seglen >> DEV_BSHIFT;
        rsync->datalen = lrsync->cs.seglen;
        rsync->cs.cnt = lrsync->cs.cnt;
        rsync->cs.num = lrsync->cs.num;
        rsync->cs.segoff = lrsync->cs.segoff;
        rsync->cs.seglen = lrsync->cs.seglen;

        sd->stat.a_tdatacnt += sizeof(rsync_t);
        sd->stat.e_tdatacnt += sizeof(rsync_t);

        /* make sure the digest buffer is large enough */
        digestlen = rsync->cs.num*DIGESTSIZE;
        if ((length = 2*digestlen) > rsync->cs.digestlen) {
            rsync->cs.digestlen = length;
            rsync->cs.digest =
              (char*)ftdrealloc(rsync->cs.digest, rsync->cs.digestlen);
        }
        /* read digest buffer from the network */
        if (readsock(mysys->sock, (char*)rsync->cs.digest, digestlen) == -1) {
            geterrmsg(ERRFAC, M_BADPROTO, msg);
            logerrmsg(ERRFAC, ERRCRIT, M_BADPROTO, msg);
            (void) senderr (fd, header, 0L, ERRWARN, M_BADPROTO, msg);
            EXIT(EXITNETWORK);
        }
        save_datap = rsync->cs.digest;
        rsync->cs.digest += digestlen;

        /* make sure the data buffer is large enough */
        if ((length = rsync->datalen) > loc_datalen) {
            const int blksize = 8192;
            loc_datalen = length;
            ftdfree(loc_databuf);
            loc_databuf = (char*)ftdmemalign(loc_datalen, blksize);
        }
        rsync->data = loc_databuf;
        rsync->devfd = sd->mirfd;

        /* calculate checksums and insert into queue */
        if ((response = chksumseg(rsync)) == -1) {
            geterrmsg (ERRFAC, M_CHKSUM, fmt);
            sprintf (msg, fmt, rsync->devid);
            logerrmsg (ERRFAC, ERRWARN, M_CHKSUM, msg);
            (void) senderr (fd, header, 0L, ERRWARN, M_CHKSUM, msg);
            EXIT(EXITNETWORK);
        }
        /* re-adjust digest pointer */
        rsync->cs.digest = save_datap;
    }
    if (q_put_syncrfd(fifo, header) == -1) {
        EXIT(EXITNETWORK);
    }
    return 0;
} /* cmd_rfdchksum */

/****************************************************************************
 * cmd_bfdchksum -- backfresh checksum
 ***************************************************************************/
int
cmd_bfdchksum(int fd, headpack_t *header)
{
#ifndef SUPPORT_BACKFRESH
    reporterr(ERRFAC, M_BKFRSH_NOTAVAIL, ERRINFO);
    return( -1 );
#endif
    group_t *group;
    sddisk_t *sd;
    int response;
    int length;
    int digestlen;

    rsync_t lrsync[1];
    rsync32_t lrsync32[1];
    rsync_t *rsync;
    char *save_datap;
    int i;

    group = mysys->group;

    /* clear checksum state */
    for (sd = group->headsddisk; sd; sd = sd->n) {
        sd->ackrsync.cs.cnt = 0;
        sd->ackrsync.cs.num = 0;
        sd->ackrsync.cs.seglen = 0;
    }

    for (i = 0; i < header->len; i++) {
        /* read rsync data structure from the network */
	if (!pmd64)
        {
        	if (readsock(mysys->sock, (char*)lrsync32, sizeof(rsync32_t)) == -1) {
            		geterrmsg(ERRFAC, M_BADPROTO, msg);
            		logerrmsg(ERRFAC, ERRCRIT, M_BADPROTO, msg);
            		(void) senderr (fd, header, 0L, ERRWARN, M_BADPROTO, msg);
            		EXIT(EXITNETWORK);
        	}
                memset(lrsync, 0, sizeof(rsync_t));
                convertrsyncfrom32to64(lrsync32, lrsync);
	}
	else
        {
        	if (readsock(mysys->sock, (char*)lrsync, sizeof(rsync_t)) == -1) {
            		geterrmsg(ERRFAC, M_BADPROTO, msg);
            		logerrmsg(ERRFAC, ERRCRIT, M_BADPROTO, msg);
            		(void) senderr (fd, header, 0L, ERRWARN, M_BADPROTO, msg);
            		EXIT(EXITNETWORK);
        	}
        }
        if ((sd = get_lg_dev(group, lrsync->devid)) == NULL) {
            geterrmsg (ERRFAC, M_BADDEVID, fmt);
            sprintf (msg, fmt, lrsync->devid);
            logerrmsg (ERRFAC, ERRWARN, M_BADDEVID, msg);
            (void) senderr (fd, header, 0L, ERRWARN, M_BADDEVID, msg);
            return (-1);
        }
        rsync = &sd->ackrsync;
        rsync->devid = lrsync->devid;
        rsync->offset = lrsync->cs.segoff;
        rsync->length = lrsync->cs.seglen >> DEV_BSHIFT;
        rsync->datalen = lrsync->cs.seglen;
        rsync->cs.cnt = lrsync->cs.cnt;
        rsync->cs.num = lrsync->cs.num;
        rsync->cs.segoff = lrsync->cs.segoff;
        rsync->cs.seglen = lrsync->cs.seglen;

        sd->stat.a_tdatacnt += sizeof(rsync_t);
        sd->stat.e_tdatacnt += sizeof(rsync_t);

        /* make sure the digest buffer is large enough */
        digestlen = rsync->cs.num*DIGESTSIZE;
        if ((length = 2*digestlen) > rsync->cs.digestlen) {
            rsync->cs.digestlen = length;
            rsync->cs.digest =
              (char*)ftdrealloc(rsync->cs.digest, rsync->cs.digestlen);
        }
        /* read digest buffer from the network */
        if (readsock(mysys->sock, (char*)rsync->cs.digest, digestlen) == -1) {
            geterrmsg(ERRFAC, M_BADPROTO, msg);
            logerrmsg(ERRFAC, ERRCRIT, M_BADPROTO, msg);
            (void) senderr (fd, header, 0L, ERRWARN, M_BADPROTO, msg);
            EXIT(EXITNETWORK);
        }
        save_datap = rsync->cs.digest;
        rsync->cs.digest += digestlen;

        /* make sure the data buffer is large enough */
        if ((length = rsync->datalen) > loc_datalen) {
            const int blksize = 8192;
            loc_datalen = length;
            ftdfree(loc_databuf);
            loc_databuf = (char*)ftdmemalign(loc_datalen, blksize);
        }
        rsync->data = sd->rsync.data = loc_databuf;
        rsync->devfd = sd->mirfd;

        /* calculate checksums and insert into queue */
        if ((response = chksumseg(rsync)) == -1) {
            geterrmsg (ERRFAC, M_CHKSUM, fmt);
            sprintf (msg, fmt, rsync->devid);
            logerrmsg (ERRFAC, ERRWARN, M_CHKSUM, msg);
            (void) senderr (fd, header, 0L, ERRWARN, M_CHKSUM, msg);
            EXIT(EXITNETWORK);
        }
        /* re-adjust digest pointer */
        rsync->cs.digest = save_datap;
    }
    if (q_put_syncbfd(fifo) == -1) {
        EXIT(EXITNETWORK);
    }
    return 0;
} /* cmd_bfdchksum */

/****************************************************************************
 * cmd_chkconfig -- make sure PMD/RMD agree
 ***************************************************************************/
u_longlong_t
cmd_chkconfig(int fd, headpack_t *header)
{
    ackpack_t ack[1];
    group_t *group;
    sddisk_t *sd;
    rdevpack_t rdev;
    struct stat statbuf;
    char path[MAXPATH];
    int response;
    u_longlong_t dsksize = 0;

    group = mysys->group;

    if (-1 == (response = readsock(fd, (char*)&rdev, sizeof(rdevpack_t)))) {
        return (response);
    }
    if (-1 == (response = doconfiginfo (&rdev, header->ts))) {
        geterrmsg (ERRFAC, M_MIRMISMTCH, fmt);
        sprintf (msg, fmt, mysys->configpath);
        logerrmsg (ERRFAC, ERRCRIT, M_MIRMISMTCH, msg);
        (void) senderr (fd, header, 0L, ERRCRIT, M_MIRMISMTCH, msg);
        EXIT(EXITANDDIE);
    }
    for (sd = group->headsddisk; sd; sd = sd->n) {
        if (0 == strcmp(rdev.path, sd->mirname)) {
           if ( _rmd_jrn_mode == JRN_ONLY ) {
              dsksize = UNSIGNED_LONG_LONG_MAX;
           }
           else
           {
              if( ((dsksize = sd->mirsize) == (u_longlong_t)0) || (sd->mirfd < 0) )
              {
                  // If the device is not already opened by the RMD or its size is unknown, get the disk size.
                  // WR PROD3919 / PROD3973: calling disksize() while the device is already opened Exclusive would fail;
                  //    also: avoid checking for negative return from disksize() as it is unsigned.
		          dsksize = disksize(sd->mirname);
              }
			  if ((dsksize == (u_longlong_t) 0) || (dsksize == UNSIGNED_LONG_LONG_MAX))
              {
                reporterr (ERRFAC, M_BADDEVSIZ, ERRCRIT, sd->mirname); 
                exit(EXITANDDIE);
			  }
           }
           break;
        }
    }
    /* Make sure that the rmd for this is enabled.  If not, then report err */
    strcpy(path, mysys->configpath);
    strcpy(path + strlen(path) - 4, ".off");
    if (stat(path, &statbuf) == 0) {
        geterrmsg (ERRFAC, M_MIRDISABLE, fmt);
        sprintf (msg, fmt, path);
        logerrmsg (ERRFAC, ERRCRIT, M_MIRDISABLE, msg);
        (void) senderr (fd, header, 0L, ERRCRIT, M_MIRDISABLE, msg);
        EXIT(EXITANDDIE);
    }
    sd->rsync.devid = header->devid;

    /* -- return mirror volume size for a configuration query */
    ack->data = dsksize;
    ack->devid = header->devid;
    if (sendack(fd, header, ack) == -1) {
        return -1;
    } 
    return dsksize;
} /* cmd_chkconfig */

/****************************************************************************
 * cmd_write -- BAB write to mirror(s)
 ***************************************************************************/
int
cmd_write(int fd, headpack_t *header)
{
    if (q_put_data(fifo, header) == -1) {
        EXIT(EXITNETWORK);
    }
    return 0;
} /* cmd_write */

/****************************************************************************
 * cmd_rfdfwrite -- full-refresh write to mirror
 ***************************************************************************/
int
cmd_rfdfwrite(int fd, headpack_t *header)
{

    /* read the data from the network into the queue */
    if (q_put_datarfdf(fifo, header) == -1) {
        EXIT(EXITNETWORK);
    }
    return 0;
} /* cmd_rfdfwrite */

/****************************************************************************
 * cmd_bfddevs -- put device into backfresh
 ***************************************************************************/
int
cmd_bfddevs(int fd, headpack_t *header)
{
#ifndef SUPPORT_BACKFRESH
    reporterr(ERRFAC, M_BKFRSH_NOTAVAIL, ERRINFO);
    return( -1 );
#endif
    ackpack_t ack[1];
    group_t *group;
    sddisk_t *sd;
    u_longlong_t offset;
    int cs_num;

    group = mysys->group;

    if ((sd = get_lg_dev(group, header->devid)) == NULL) {
        geterrmsg (ERRFAC, M_DEVID, fmt);
        sprintf (msg, fmt, argv0, header->devid);
        logerrmsg (ERRFAC, ERRCRIT, M_DEVID, msg);
        (void) senderr (fd, header, 0L, ERRCRIT, M_DEVID, msg);
        return (-1);
    }
    /* Now, seek to the appropriate offset */
    ftd_trace_flow(FTD_DBG_FLOW1,
            "\n*** BFD device: %s:%d restart sector offset = %ld\n",
            sd->devname, sd->mirfd, header->offset);
    offset = (((u_longlong_t)header->offset) << DEV_BSHIFT);
    if (llseek(sd->mirfd, offset, SEEK_SET) == -1) {
        geterrmsg (ERRFAC, M_SEEKERR, fmt);
        sprintf (msg, fmt, argv0, sd->mirname, offset, strerror(errno));
        logerrmsg (ERRFAC, ERRWARN, M_SEEKERR, msg);
        (void) senderr (fd, header, 0L, ERRWARN, M_SEEKERR, msg);
        return (-1);
    }
    sd->dm.res = header->data;
    sd->rsync.devfd = sd->mirfd;
    sd->rsync.devid = sd->devid;

    sd->rsync.deltamap.cnt = 0;
    sd->rsync.deltamap.refp =
        (ref_t*)ftdmalloc(sizeof(ref_t)*INIT_DELTA_SIZE);
    sd->rsync.deltamap.size = INIT_DELTA_SIZE;
    cs_num = (mysys->tunables.chunksize/MINCHK)+2;
    sd->rsync.cs.digestlen = 2*(cs_num*DIGESTSIZE);
    sd->rsync.cs.digest = (char*)ftdmalloc(sd->rsync.cs.digestlen);
    sd->rsync.cs.cnt = 2;
    sd->rsync.cs.num = cs_num;

    memset(ack, 0, sizeof(ackpack_t));
    ack->devid = header->devid;
    ack->data = ACKBFDCMD;
    if (sendack(fd, header, ack) == -1) {
        EXIT(EXITNETWORK);
    }
    return 0;
} /* cmd_bfddevs */

/****************************************************************************
 * cmd_bfdeve --
 ***************************************************************************/
int
cmd_bfddeve(int fd, headpack_t *header)
{
#ifndef SUPPORT_BACKFRESH
    reporterr(ERRFAC, M_BKFRSH_NOTAVAIL, ERRINFO);
    return( -1 );
#endif
    group_t *group;
    sddisk_t *sd;

    group = mysys->group;

    if ((sd = get_lg_dev(group, header->devid)) == NULL) {
        geterrmsg (ERRFAC, M_DEVID, fmt);
        sprintf (msg, fmt, argv0, header->devid);
        logerrmsg (ERRFAC, ERRCRIT, M_DEVID, msg);
        (void) senderr (fd, header, 0L, ERRCRIT, M_DEVID, msg);
        return (-1);
    }
    if (sd->rsync.deltamap.refp) {
        free(sd->rsync.deltamap.refp);
    }
    if (sd->rsync.cs.digest) {
        free(sd->rsync.cs.digest);
    }
    memset(&sd->rsync, 0, sizeof(rsync_t));
    return 0;
} /* cmd_bfddeve */

/****************************************************************************
 * cmd_bfdend -- take group out of backfresh
 ***************************************************************************/
int
cmd_bfdend(int fd, headpack_t *header)
{
#ifndef SUPPORT_BACKFRESH
    reporterr(ERRFAC, M_BKFRSH_NOTAVAIL, ERRINFO);
    return( -1 );
#endif
    ackpack_t ack[1];
    sddisk_t *sd;

    sprintf(fmt, "%s/%03d.off", PATH_RUN_FILES, header->devid);
    UNLINK(fmt);
    _rmd_state = FTDPMD;

    memset(ack, 0, sizeof(ackpack_t));
    ack->devid = header->devid;
    ack->data = ACKBFDCMD;
    if (sendack(fd, header, ack) == -1) {
        EXIT(EXITNETWORK);
    }
    return 0;
} /* cmd_bfdend */

/****************************************************************************
 * cmd_rfddevs -- put device into refresh
 ***************************************************************************/
int
cmd_rfddevs(int fd, headpack_t *header)
{
    ackpack_t ack[1];
    group_t *group;
    sddisk_t *sd;
    u_longlong_t offset;
    int cs_num;

    group = mysys->group;

    if ((sd = get_lg_dev(group, header->devid)) == NULL) {
        geterrmsg (ERRFAC, M_BADDEVID, fmt);
        sprintf (msg, fmt, header->devid);
        logerrmsg (ERRFAC, ERRWARN, M_BADDEVID, msg);
        (void) senderr (fd, header, 0L, ERRWARN, M_BADDEVID, msg);
        return (-1);
    }
    /* Now, seek to the appropriate offset */
    ftd_trace_flow(FTD_DBG_FLOW1,
            "\n*** RFD device: %s restart sector offset = %ld\n",
            sd->devname, header->offset);
    offset = (((u_longlong_t)header->offset) << DEV_BSHIFT);
    if (llseek(sd->mirfd, offset, SEEK_SET) == -1) {
        geterrmsg (ERRFAC, M_BADOFF, fmt);
        sprintf (msg, fmt, sd->mirname, offset, strerror(errno));
        logerrmsg (ERRFAC, ERRWARN, M_BADOFF, msg);
        (void) senderr (fd, header, 0L, ERRWARN, M_BADOFF, msg);
        return (-1);
    }
    sd->dm.res = header->data;

    /* allocate an initial digest buffer for the device */
    cs_num = (mysys->tunables.chunksize/MINCHK)+2;
    sd->rsync.cs.digest = (char*)ftdmalloc(2*cs_num*DIGESTSIZE);

    memset(ack, 0, sizeof(ackpack_t));
    ack->devid = header->devid;
    ack->data = ACKRFDCMD;
    if (sendack(fd, header, ack) == -1) {
        EXIT(EXITNETWORK);
    }
    return 0;
} /* cmd_rfddevs */

/****************************************************************************
 * cmd_rfddeve -- take device out of refresh
 ***************************************************************************/
int
cmd_rfddeve(int fd, headpack_t *header)
{
    ackpack_t ack[1];
    group_t *group;
    sddisk_t *sd;

    group = mysys->group;

    if ((sd = get_lg_dev(group, header->devid)) == NULL) {
        geterrmsg (ERRFAC, M_BADDEVID, fmt);
        sprintf (msg, fmt, header->devid);
        logerrmsg (ERRFAC, ERRWARN, M_BADDEVID, msg);
        (void) senderr (fd, header, 0L, ERRWARN, M_BADDEVID, msg);
        return (-1);
    }
    close(sd->mirfd);
    sd->mirfd = -1;

    memset(ack, 0, sizeof(ackpack_t));
    ack->devid = header->devid;
    ack->data = ACKRFDCMD;
    if (sendack(fd, header, ack) == -1) {
        EXIT(EXITNETWORK);
    }
    return 0;
} /* cmd_rfddeve */

/****************************************************************************
 * cmd_bfdfstart -- start backfresh
 ***************************************************************************/
int
cmd_bfdstart(int fd, headpack_t *header)
{
#ifndef SUPPORT_BACKFRESH
    reporterr(ERRFAC, M_BKFRSH_NOTAVAIL, ERRINFO);
    return( -1 );
#endif
    ackpack_t ack[1];

    _rmd_state = FTDBFD;

    memset(ack, 0, sizeof(ackpack_t));
    ack->devid = header->devid;
    ack->data = ACKBFDCMD;
    if (sendack(fd, header, ack) == -1) {
        EXIT(EXITNETWORK);
    }

    closedevs(-1);
    /* re-open devices EXCL */
    if (verify_rmd_entries(mysys->group, mysys->configpath, 1) == -1) {
        EXIT(EXITANDDIE);
    }
    return 0;
} /* cmd_bfdstart */

/****************************************************************************
 * cmd_rfdfstart -- start full refresh
 ***************************************************************************/
int
cmd_rfdfstart(int fd, headpack_t *header)
{
    ackpack_t ack[1];
	u_long    refresh_state;   // WR PROD6755

    stop_rmdapply();
    /*
     * WR36133: PMD shall not send full refresh request to RMD when checkpoint
     *          is on due to possible journal space shortage. Add defensive
     *          codes here, do not delete .p
     */
    nuke_journals(DELETE_JOURNAL_ALL);
    remove_corrupted_journal_flag(lgnum);

    closedevs(-1);
    /* re-open devices EXCL */
    if( !_net_bandwidth_analysis )
	{
	    if (verify_rmd_entries(mysys->group, mysys->configpath, 1) == -1) {
	        EXIT(EXITANDDIE);
	    }
	}

    _rmd_jrn_state = JRN_CO;
    if (get_lg_checkpoint()) {
        _rmd_jrn_mode = JRN_ONLY;
    } else {
        _rmd_jrn_mode = MIR_ONLY;
    }
    _rmd_state = FTDRFD;

    /* WR PROD6755: take note of the Refresh state, i.e. Full or Smart, to determine 
       if we create a Full Refresh watchdog file or not (we want the watchdog only in Full mode.
	*/
    refresh_state =	header->data;

    memset(ack, 0, sizeof(ackpack_t));
    ack->devid = header->devid;
    ack->data = ACKRFDCMD;
    if (sendack(fd, header, ack) == -1) {
        EXIT(EXITNETWORK);
    }

    /* WR43376: if Full Refresh or Full Refresh Restart (not Smart Refresh)
       create a watchdog file; if a crash (such as hard reboot) of RMD occurs
       during Full Refresh, this watchdog will permit detecting it at restart;
       also: take note that the watchdog can be removed if the PMD decides to
       interrupt the Full Refresh itself, for instance with a killrefresh
       (because in this case the interruption is caused by the PMD)
    */
    if( (refresh_state == FTDRFDF || refresh_state == FTDRFDR) && !_net_bandwidth_analysis )  // WR PROD6755
	{
        create_unclean_shutdown_file( lgnum );
        remove_FR_watchdog_if_PMD_disconnets = 1;
	}

    return 0;
} /* cmd_rfdfstart */

/****************************************************************************
 * cmd_rfdfend -- end full refresh
 ***************************************************************************/
int
cmd_rfdfend(int fd, headpack_t *header)
{
    ackpack_t ack[1];

    /* WR43376: Full Refresh completed, clean state; remove unclean-shutdown flag file */
    remove_unclean_shutdown_file( lgnum );
    unclean_shutdown = 0;
    remove_FR_watchdog_if_PMD_disconnets = 0;  /* Watchdog already removed here */

    // If in network bandwidth analysis mode, remove the fictitious config file
	if( _net_bandwidth_analysis )
	{
        remove_fictitious_cfg_files( lgnum, 0 );
	}

    _rmd_state = FTDPMD;
    close_journal(mysys->group);

    memset(ack, 0, sizeof(ackpack_t));
    ack->devid = header->devid;
    ack->data = ACKRFDCMD;
    if (sendack(fd, header, ack) == -1) {
        EXIT(EXITNETWORK);
    }
    return 0;
} /* cmd_rfdfend */

/****************************************************************************
 * cmd_msgco -- end refresh, send acknowledgement to PMD
 ***************************************************************************/
int
cmd_msgco(int fd, headpack_t *header)
{
    ackpack_t ack[1];

    /* WR PROD6443: Refresh completed, clean state; remove unclean-shutdown flag file */
    remove_unclean_shutdown_file( lgnum );
    unclean_shutdown = 0;
    remove_FR_watchdog_if_PMD_disconnets = 0;  /* Watchdog already removed here */

    memset(ack, 0, sizeof(ackpack_t));
    ack->devid = header->devid;
    ack->data = ACKMSGCO;
    ack->mirco = 1;

    if (sendack(fd, header, ack) == -1) {
        EXIT(EXITNETWORK);
    }

    return 0;
} /* cmd_msgco */

/****************************************************************************
 * cmd_hup -- PMD was interupted
 ***************************************************************************/
int
cmd_hup(int fd, headpack_t *header)
{
    ackpack_t ack[1];

    memset(ack, 0, sizeof(ackpack_t));
    ack->devid = header->devid;
    ack->data = ACKHUP;
    if (sendack(fd, header, ack) == -1) {
        EXIT(EXITNETWORK);
    }
    return 0;
} /* cmd_hup */

/****************************************************************************
 * cmd_cponerr -- PMD could not transition to checkpoint
 ***************************************************************************/
int
cmd_cponerr(int fd, headpack_t *header)
{
    _rmd_cpstart = 0;
    return 0;
} /* cmd_cponerr */

/****************************************************************************
 * cmd_cpofferr -- PMD could not transition from checkpoint
 ***************************************************************************/
int
cmd_cpofferr(int fd, headpack_t *header)
{
    _rmd_cpstop = 0;
    return 0;
} /* cmd_cpofferr */

/****************************************************************************
 * cmd_noop -- send a NOOP ACK to the PMD
 ***************************************************************************/
int
cmd_noop(int fd, headpack_t *header)
{
    ackpack_t ack[1];

    memset(ack, 0, sizeof(ackpack_t));
    ack->acktype = ACKNOOP;
    ack->devid = header->devid;
    ack->data = ACKNOOP;
    if (sendack(fd, header, ack) == -1) {
        EXIT(EXITNETWORK);
    }
    return 0;
} /* cmd_noop */

/****************************************************************************
 * cmd_exit -- RMD go away
 ***************************************************************************/
void
cmd_exit(int fd, headpack_t *header)
{
    group_t *group;
    sddisk_t *sd;
    sddisk_t *sdnext;

    group = mysys->group;

    for (sd = group->headsddisk; sd;) {
        sdnext = sd->n;
        (void)close(sd->mirfd);
        free(sd);
        sd = sdnext;
    }
    free (loc_databuf);
    close(mysys->sock);
    reporterr (ERRFAC, M_RMDEXIT, ERRINFO, argv0);
    EXIT(EXITNORMAL);
} /* cmd_exit */

/****************************************************************************
  Tell PMD that this RMD has been uncleanly shutdown (crash) 
  during a Full Refresh, and no Full Refresh has been redone
  to completion since then (WR43376)
***************************************************************************/
void send_unclean_shutdown_state_to_PMD(  int fd, headpack_t *header  )
{
    ackpack_t ack[1];

    memset(ack, 0, sizeof(ackpack_t));
    ack->acktype = ACKNORM;
    ack->data = UNCLEAN_SHUTDOWN_RMD;

    if (sendack(fd, header, ack) == -1) 
    {
        EXIT(EXITNETWORK);
    }
    return;
}


/****************************************************************************
 * handshake_check_if_mirrors_mounted: check if any target (mirror) device of
 * this group is mounted upon starting the RMD. This is used at initial PMD-RMD
 * handshake when we are not in checkpoint mode (hence we cannot allow
 * replication of a group with mounted target devices).
 * Return: 0 if no device mounted; 1 otherwise and name of device mounted 
 * and its mountpoint.
 * NOTE: we stop at the first mounted device found.
***************************************************************************/
static int handshake_check_if_mirrors_mounted( char *return_devname, char *return_mountpoint )
{
    char devname[MAXPATH], mountp[MAXPATH];
    sddisk_t *sd;
    group_t *group;

    group = mysys->group;

    for (sd = group->headsddisk; sd; sd = sd->n)
    {
       force_dsk_or_rdsk(devname, sd->mirname, 0);     /* Force block device name */
       if (dev_mounted(devname, mountp))
       {
         break;
       }
    }

    if( sd )
    {
       /* Found a mounted mirror device */
       strcpy( return_devname, devname );
       strcpy( return_mountpoint, mountp );
       return(1);
    }
    else
    {
       return(0);
    }
}

/****************************************************************************
 * cmd_check_if_mirrors_mounted: check if any target (mirror) device of
 * a group is mounted. This is used by the PMD to determine if a Full,
 * Restartable Full, or Smart Refresh can be launched.
***************************************************************************/
#ifdef CHECK_IF_RMD_MIRRORS_MOUNTED
void cmd_check_if_mirrors_mounted( int fd, headpack_t *header )
{
    char devname[MAXPATH], mountp[MAXPATH];
    ackpack_t ack[1];
    sddisk_t *sd;
    group_t *group;

    group = mysys->group;

    memset(ack, 0, sizeof(ackpack_t));
    ack->acktype = ACKNORM;
    ack->devid = header->devid;

    for (sd = group->headsddisk; sd; sd = sd->n)
    {
       force_dsk_or_rdsk(devname, sd->mirname, 0);     /* Force block device name */
       if (dev_mounted(devname, mountp))
       {
         break;
       }
    }

    if( sd )
    {
       /* Found a mounted mirror device */
       reporterr (ERRFAC, M_RMD_TGTMOUNTED, ERRCRIT, devname, mountp );
       ack->data = RFDF_TGT_MNTED;       /* Signal RMD target device mounted to PMD */
    }
    else
    {
       ack->data = RFDF_TGT_NOTMNTED;    /* Signal RMD target device NOT mounted to PMD */
    }

    if (sendack(fd, header, ack) == -1) 
    {
        EXIT(EXITNETWORK);
    }
   return;
}
#endif /* of CHECK_IF_RMD_MIRRORS_MOUNTED */


/****************************************************************************
* ackclearbits -- ACK to PMD
***************************************************************************/
int
ackclearbits(int fd, headpack_t *header)
{
    ackpack_t ack[1];
	group_t *group = mysys->group;

    // PROD7131: target crash during Normal with IO and BAB overflows was seen to cause data mismatches.
	// sync the buffers to this group's devices and to any opened journal before acknowledging to PMD; this acknowledge
	// will make the PMD proceed with clearing the dirty blocks bitmap.
    syncgroup(1);
    if(group->jrnfd >= 0)
	{
	  fsync(group->jrnfd);
	}
	sleep(2); // Without this sleep, got checksum mismatches at test of target crash

    memset(ack, 0, sizeof(ackpack_t));
    ack->devid = header->devid;
    ack->data = ACKCLEARBITS;

    if (sendack(fd, header, ack) == -1) {
        EXIT(EXITNETWORK);
    }

    return 0;
} /* ackclearbits */

/****************************************************************************
 * readitem -- RMD read an item from the network or a stream
 *
 * NOTE: as of TDMF IP 2.8.0, all the commands related to backfresh are
 *       deactivated; if any of them is received here, the PMD will get
 *       a "bad command" response.
 ***************************************************************************/
int
readitem (int fd)
{
    headpack_t header;
    headpack32_t header32;
    ackpack_t ack;
    int response;
    int lgnum;

    group_t *group;

    group = mysys->group;
    lgnum = cfgpathtonum(mysys->configpath);

    if (!pmd64)
    {
    	if (-1 == (response = readsock(fd, (char*)&header32, sizeof(headpack32_t)))) {
        	return response;
    	}
	converthdrfrom32to64(&header32, &header);
    }
    else
    {
    	if (-1 == (response = readsock(fd, (char*)&header, sizeof(headpack_t)))) {
        	return response;
    	}
    }

    if ((header.cmd != CMDNOOP) || (ftd_debugFlag & FTD_DBG_FLOW3)) {
        ftd_trace_flow(FTD_DBG_FLOW1,
                "readitem: header.magicvalue = %08lx, header.cmd = %d\n",
                header.magicvalue, header.cmd);
    }

    if (header.magicvalue != MAGICHDR) {
        if (mysys != NULL)
            reporterr (ERRFAC, M_HDRMAGIC, ERRWARN);
        return -1;
    }
    if (header.cmd != CMDNOOP)
        ftd_debugf("%d", "%s(%d) %s %s",
            fd,
            get_cmd_str(header.cmd),
            header.len,
            get_jrn_mode_str(_rmd_jrn_mode),
            get_jrn_state_str(_rmd_jrn_state));
    memset(&ack, 0, sizeof(ack));

    switch (header.cmd) {
    case CMDHANDSHAKE:
        cmd_handshake(fd, &header);
        break;
    case CMDCHKCONFIG:
        response = cmd_chkconfig(fd, &header);
         break;
    case CMDNOOP:
        cmd_noop(fd, &header);
        break;
    case CMDHUP:
        cmd_hup(fd, &header);
        break;
    case CMDCPONERR:
        cmd_cponerr(fd, &header);
        break;
    case CMDCPOFFERR:
        cmd_cpofferr(fd, &header);
        break;
    case CMDRFDCHKSUM:
        cmd_rfdchksum(fd, &header);
        break;
#ifdef SUPPORT_BACKFRESH
    case CMDBFDCHKSUM:
        cmd_bfdchksum(fd, &header);
        break;
#endif
    case CMDWRITE:
        cmd_write(fd, &header);
        break;
    case CMDRFDFWRITE:
	    _net_bandwidth_analysis = 0;
        cmd_rfdfwrite(fd, &header);
        break;
    case CMDRFDFSWRITE:
	    _net_bandwidth_analysis = 1; // Full Refresh simulation for Network bandwidth evaluation
        header.cmd = CMDRFDFWRITE;
        cmd_rfdfwrite(fd, &header);
        break;
    case CMDRFDFSTART:
        cmd_rfdfstart(fd, &header);
        break;
    case CMDRFDEND:
#if defined(linux)
	/*
	 * after a smart or full refresh, sync data into the disks as
	 * the mirror disks are open in async mode on linux
	 */
	syncgroup(1);
#endif
        break;
    case CMDRFDFEND:
        cmd_rfdfend(fd, &header);
#if defined(linux)
	syncgroup(1);
#endif
        break;
#ifdef SUPPORT_BACKFRESH
    case CMDBFDSTART:
        cmd_bfdstart(fd, &header);
        break;
    case CMDBFDDEVS:
        cmd_bfddevs(fd, &header);
        break;
    case CMDBFDEND:
        cmd_bfdend(fd, &header);
        break;
    case CMDBFDDEVE:
        cmd_bfddeve(fd, &header);
        break;
#endif
    case CMDRFDDEVS:
        cmd_rfddevs(fd, &header);
        break;
    case CMDRFDDEVE:
        cmd_rfddeve(fd, &header);
        break;
    case CMDEXIT:
        cmd_exit(fd, &header);
        break;
    case CMDMSGINCO:
        /* WR PROD6443: the verification that was done here against starting a refresh after
           an unclean shutdown during a Full Refresh is now done via the startup handshake between
           PMD and RMD.
        */
        if (check_corrupted_journal_flag(lgnum))
        {
            nuke_journals(DELETE_JOURNAL_ALL);
            remove_corrupted_journal_flag(lgnum);
        }

        if (GET_LG_JLESS(mysys)) {
             ftd_trace_flow(FTD_DBG_FLOW1,
                  "ignore MSG_INCO due to JLESS is on.\n");
        } else {
            // Now that we have smarter smart refreshes, we do not want to delete existing .i journal entries,
            // as their content will not be sent again.
             if ((group->jrnfd = new_journal(lgnum, JRN_INCO, 0)) == -1) {
                  return -1;
             }
             closedevs(O_RDONLY);
             _rmd_state = FTDRFD;
             _rmd_rfddone = 0;
        }
        break;
    case CMDMSGCO:
        _rmd_rfddone = 1;
        cmd_msgco(fd, &header);
        if (!GET_LG_JLESS(mysys)) {
             close_journal(group);
             rename_journals(JRN_CO);
             if ((group->jrnfd = new_journal(lgnum, JRN_CO, 1)) == -1) {
                  return -1;
             }
             if (!_rmd_cpon) {
                  /* only apply if we are NOT in checkpoint */
                  closedevs(O_RDONLY);
                  start_rmdapply(0);
                  _rmd_jrn_mode = JRN_AND_MIR;
             }
             _rmd_jrn_state = JRN_CO;
             _rmd_state = FTDPMD;
        }
        break;
    case CMDCLEARBITS:
        /* Send back an ack. */
        ackclearbits(fd, &header);
        break;
#ifdef CHECK_IF_RMD_MIRRORS_MOUNTED
    case CMDCKMIRSMTD:
        /* WR PROD4508 and HIST wr38281: */
        cmd_check_if_mirrors_mounted(fd, &header);
        break;
#endif
    default:
        geterrmsg (ERRFAC, M_BADCMD, fmt);
        sprintf (msg, fmt, errno, header.devid, header.cmd);
        logerrmsg (ERRFAC, ERRWARN, M_BADCMD, msg);
        (void) senderr (fd, &header, 0L, ERRWARN, M_BADCMD, msg);
        return -1;
    } /* switch */

    return 1;
} /* readitem */

/****************************************************************************
 * cp_action -- RMD checkpoint actions
 ***************************************************************************/
int
cp_action (int fd)
{
    ackpack_t ack[1];
    headpack_t header[1];
    char lgname[32];
    int response;
    int action;
    int cnt;

    if ((action = ipcrecv_sig(fd, &cnt)) == -1) {
	    // WR PROD7916
        // This error has been seen to occur after a long network outage, upon network interface
		// coming back. If this occurs, the RMD must remove its Full Refresh watchdog file since
		// the FRefresh offset recorded by the PMD is one that the RMD has effectively acknowledged,
		// and thus FReresh can resume where it was interrupted.
        if( remove_FR_watchdog_if_PMD_disconnets )
		{
            remove_unclean_shutdown_file( lgnum );
            reporterr (ERRFAC, M_RMD_IPCERR_QUIT, ERRINFO, lgnum);
		}
	    // If in network bandwidth analysis mode, remove the fictitious config file
		if( _net_bandwidth_analysis )
		{
            remove_fictitious_cfg_files( lgnum, 0 );
		}

        EXIT(EXITNETWORK);
    }

    ftd_trace_flow(FTD_DBG_FLOW1, "action: %d, jless: %d\n",
                    action, GET_LG_JLESS(mysys));

    /* check for checkpoint transition */
    if (action == FTDQCPOFFS) {
        /* should we transition OUT of checkpoint ? */
        sprintf(lgname, "s%03d", lgnum);
        ftd_trace_flow(FTD_DBG_FLOW1,
                        "FTDQCPOFFS _rmd_cpstop: %d, _rmd_cpon: %d, _rmd_cpstart: %d\n",
                        _rmd_cpstop, _rmd_cpon, _rmd_cpstart);
        if (_rmd_state == FTDRFD || _rmd_state == FTDRFDF) {
            reporterr(ERRFAC, M_RFDCPOFF, ERRWARN, argv0);
            _rmd_cpstop = 0;
            return 1;
        } else if (_rmd_state == FTDBFD) {
            reporterr(ERRFAC, M_BFDCPOFF, ERRWARN, argv0);
            _rmd_cpstop = 0;
            return 1;
        }
        if (_rmd_cpstop) {
            reporterr(ERRFAC, M_CPSTOPAGAIN, ERRWARN, lgname);
            return 1;
        } else if (!_rmd_cpon && !_rmd_cpstart) {
            /* if (!GET_LG_JLESS(mysys)) {  */
            reporterr(ERRFAC, M_CPOFFAGAIN, ERRWARN, lgname);
            return 1;
        }
        if (_rmd_cpon || _rmd_cpstart) {
            /* notify PMD that we are going out of checkpoint mode */
            memset(header, 0, sizeof(headpack_t));
            ack->data = ACKCPSTOP;
            if ((response = sendack(_rmd_sockfd, header, ack)) == -1) {
                return -1;
            }
        }
        _rmd_cpstop = 1;
    } else if (action == FTDQCPONS) {
        /* should we transition IN to checkpoint ? */
        sprintf(lgname, "s%03d", lgnum);
        ftd_trace_flow(FTD_DBG_FLOW1,
                        "FTDQCPONS _rmd_cpstop: %d, _rmd_cpon: %d, _rmd_cpstart: %d\n",
                        _rmd_cpstop, _rmd_cpon, _rmd_cpstart);
        if (_rmd_state == FTDRFD || _rmd_state == FTDRFDF) {
            reporterr(ERRFAC, M_RFDCPON, ERRWARN, argv0);
            _rmd_cpstart = 0;
            return 1;
        } else if (_rmd_state == FTDBFD) {
            reporterr(ERRFAC, M_BFDCPON, ERRWARN, argv0);
            _rmd_cpstop = 0;
            return 1;
        }
        if (_rmd_cpstart) {
            reporterr(ERRFAC, M_CPSTARTAGAIN, ERRWARN, lgname);
            return 1;
        } else if (_rmd_cpon) {
            reporterr(ERRFAC, M_CPONAGAIN, ERRWARN, lgname);
            return 1;
        }
        /* notify PMD that we are going to checkpoint mode */
        memset(header, 0, sizeof(headpack_t));
        ack->data = ACKCPSTART;
        if ((response = sendack(_rmd_sockfd, header, ack)) == -1) {
            return -1;
        }
        _rmd_cpstart = 1;
    } else {
        /* unknown action */
    }
    return 1;
} /* cp_action */

/****************************************************************************
 * processrequests -- process PMD requests
 ***************************************************************************/
void
processrequests(void *args)
{
    headpack_t header[1];
    ackpack_t ack[1];
    struct timeval seltime[1];
    fd_set readfds[1];
    fd_set readfds_copy[1];
    char *jrnname;
    time_t now, lastts;
    int deltatime, elapsed_time;
    int connect_timeo;
    int lstate, lmode;
    ftd_jrnnum_t lnum;
    int lgnum;
    int len;
    int nselect;
    int response;
    int transition;
    int rc;

    lgnum = cfgpathtonum(mysys->configpath);

    elapsed_time = 0;
    lastts = 0;
    connect_timeo = 5;

    FD_ZERO(readfds);
    FD_SET(_rmd_rpipefd, readfds);
    FD_SET(_rmd_sockfd, readfds);

    *readfds_copy = *readfds;
    nselect = _rmd_rpipefd > _rmd_sockfd ? _rmd_rpipefd: _rmd_sockfd;
    nselect++;

    seltime->tv_usec = 0;
    seltime->tv_sec = 1;

    memset(header, 0, sizeof(headpack_t));

    response = 1;
    while (response == 1) {
        *readfds_copy = *readfds;

        rc = select(nselect, readfds_copy, NULL, NULL, seltime);

        ftd_trace_flow(FTD_DBG_FLOW3, "select: %d\n", rc);

        switch(rc) {
        case -1:
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            } else if (errno == 0 || errno == ENOENT) {
                reporterr(ERRFAC, M_RMDEXIT, ERRINFO, argv0);
                EXIT(EXITANDDIE);
            } else {
                reporterr(ERRFAC, M_SOCKRERR, ERRCRIT, argv0, strerror(errno));
                EXIT(EXITANDDIE);
            }
            break;
        case 0:
            /* nothing to read */
            time(&now);
            if (lastts > 0) {
                deltatime = now - lastts;
            } else {
                deltatime = 0;
            }
            elapsed_time += deltatime;
            lastts = now;
            break;
        default:
            if (FD_ISSET(_rmd_rpipefd, readfds_copy)) {
                response = cp_action(_rmd_rpipefd);
            } else if (FD_ISSET(_rmd_sockfd, readfds_copy)) {
                if ((response = readitem(_rmd_sockfd)) != 1) {
                    closeconnection ();
                    continue;
                }
                elapsed_time = 0;
                lastts = 0;
            } else {
                /* nothing to read on target descriptors */
                time(&now);
                if (lastts > 0) {
                    deltatime = now - lastts;
                } else {
                    deltatime = 0;
                }
                elapsed_time += deltatime;
                lastts = now;
            }
            break;
        }
        /* maybe network channel is down */
        if (elapsed_time > max_elapsed_time) {
            int save_errno;
            if (net_test_channel(connect_timeo) <= 0) 
            {
                // Check for errno EALREADY (Operation already in progress)
                // in which case it is not a NETBROKE condition. It may be that a connect()
                // operation is still in progress.
				// PROD10185: check also for status EISCONN (got this status on Solaris, meaning
				// "Transport endpoint is already connected").
				save_errno = errno;
                if( (save_errno != EALREADY) && (save_errno != EISCONN) )
                {
                   reporterr(ERRFAC, M_NETBROKE, ERRWARN,
                       argv0, othersys->name, get_error_str(save_errno));
				   // If a full refresh was in progress, it is safe to remove the FRefresh watchdog file
				   // since this is not an RMD crash. The PMD can resume the Full Refresh at the last offset
				   // the RMD has acknowledged and that the PMD has effectively received (PROD7916).
                   if( remove_FR_watchdog_if_PMD_disconnets )
                       remove_unclean_shutdown_file( lgnum );

				    // If in network bandwidth analysis mode, remove the fictitious config file
					if( _net_bandwidth_analysis )
					{
                        remove_fictitious_cfg_files( lgnum, 0 );
					}

                   EXIT(EXITANDDIE);
                }
                else
                {
                   reporterr (ERRFAC, M_COMMS_TIMEOUT, ERRINFO, lgnum );
                }
            }
            elapsed_time = 0;
            lastts = 0;
        }

        /* should we dump stats ? */
        dump_rmd_stats(mysys);

        if (_rmd_cppend) {
            /* we are waiting to transition to cp mode */
            if (get_journals(jrnphp, 1, 1) <= 0) {
                transition = 1;
            } else {
		jrnname = jrnpath_get(jrnphp, 0);
                parse_journal_name(jrnname, &lnum, &lstate, &lmode);
                len = strlen(jrnname);
                transition = jrnname[len-1] == 'i' || lnum > _rmd_jrn_cp_lnum;
            }
            if (transition) {
                /* don't care if this fails! - transition to cp regardless */
                exec_cp_cmd(lgnum, POST_CP_ON, 0);
                reporterr(ERRFAC, M_CPON, ERRINFO, argv0);
                /* send same msg to PMD */
                memset(header, 0, sizeof(headpack_t));
                ack->data = ACKCPON;
                sendack(mysys->sock, header, ack);
                ftd_trace_flow(FTD_DBG_FLOW1,
                        "_rmd_cppend: %d -> 0\n", _rmd_cppend);
                _rmd_cppend = 0;
                if (!GET_LG_JLESS(mysys)) {
                    ftd_trace_flow(FTD_DBG_FLOW1,
                            "_rmd_cpon: %d -> 1\n", _rmd_cpon);
                    _rmd_cpon = 1;
                    _rmd_jrn_mode = JRN_ONLY;
                } else {
                    _rmd_cpon = 1;
                    closedevs(-1);
                }
            }
        } else if (_rmd_jrn_mode == JRN_AND_MIR
	         /* TODO heavy expense for existence test */
		 && get_journals(tjrnphp, 1, 1) <= 0) {
            /* no more journals - go back to mirroring */
            stop_rmdapply();
            closedevs(-1);
            /* Close the last opened journal (WR PROD4619) */
			if( mysys->group->jrnfd != -1 )
            {
              close_journal(mysys->group);
			}

            /* re-open devices EXCL */
            if (verify_rmd_entries(mysys->group, mysys->configpath, 1) == -1) {
                EXIT(EXITANDDIE);
            }
            _rmd_jrn_state = JRN_CO;
            _rmd_jrn_mode = MIR_ONLY;
        }
#if defined(linux)
        seltime->tv_usec = 0;
        seltime->tv_sec = 1;
#endif
    }

} /* processrequests */

/****************************************************************************
 * processjournals -- Process journals
 ***************************************************************************/
void
processjournals (void *args)
{
    group_t *group;
    jrnheader_t jrnheader[1];
    int lstate, lmode;
    ftd_jrnnum_t lnum;
    char * jrnname;
    int lgnum;
    int rc;
    int n;

    lgnum = cfgpathtonum(mysys->configpath);
    group = mysys->group;

    _rmd_jrn_cp_lnum = -1;

    while(1) {
        if ((get_journals(jrnphp, 1, 1)) <= 0) {
            /* no more journals */
            reporterr (ERRFAC, M_RMDAEND, ERRINFO, argv0);
            closedevs(-1);
            sleep(2);
            break;
        }
	jrnname = jrnpath_get( jrnphp, 0 );
        parse_journal_name(jrnname, &lnum, &lstate, &lmode);
        /* if lnum > checkpoint journal num - done */
        if (_rmd_jrn_cp_lnum >= 0
        && lnum > _rmd_jrn_cp_lnum) {
            reporterr (ERRFAC, M_RMDAEND, ERRINFO, argv0);
            closedevs(-1);
            break;
        }
        _rmd_jrn_num = lnum;
        _rmd_jrn_state = lstate;
        _rmd_jrn_mode = lmode;

        sprintf(group->journal, "%s/%s",group->journal_path, jrnname);
        if ((group->jrnfd = open(group->journal, O_RDWR)) == -1) {
            reporterr (ERRFAC, M_FILE, ERRCRIT, group->journal, strerror(errno));
            reporterr (ERRFAC, M_RMDAPPLY, ERRCRIT, argv0);
            EXIT(EXITANDDIE);
        }
        n = sizeof(jrnheader_t);
        if ((rc = ftdread(group->jrnfd, (char *)jrnheader, n)) != n) {

            // We weren't able to read the complete journal header.
            // Confirm it's because the file is too small.
            if (rc < sizeof(jrnheader_t))
            {
                // The journal begins with an incomplete journal header.
                reporterr(ERRFAC, M_JRNINCHEADER, ERRWARN, group->journal);
                // Just delete and skip the complete journal.
                UNLINK(group->journal);
                close(group->jrnfd);
                group->jrnfd = -1;
                continue;
            }

            reporterr (ERRFAC, M_JRNHEADER, ERRCRIT,
                group->journal);
            reporterr (ERRFAC, M_RMDAPPLY, ERRCRIT, argv0);
            EXIT(EXITANDDIE);
        }
        if (jrnheader->magicnum != MAGICJRN) {
            reporterr (ERRFAC, M_JRNMAGIC, ERRCRIT, group->journal);
            reporterr (ERRFAC, M_RMDAPPLY, ERRCRIT, argv0);
            flag_corrupted_journal_and_stop_rmd(lgnum);
            EXIT(EXITANDDIE);
        }
        rc = apply_journal(group);
        if (rc != 0) {
            reporterr (ERRFAC, M_RMDAPPLY, ERRCRIT, argv0);
            EXIT(EXITANDDIE);
        }
    }
} /* processjournals  */

/****************************************************************************
 * main -- Remote mirror daemon - RMD
 ****************************************************************************/
int
main (int argc, char** argv, char** environ)
{
    headpack_t header;
    ackpack_t ack;
    struct stat statbuf;
    char configpath[MAXPATH];
    char buf[MAXPATH];
    char msg[MAXPATH];
    char fmt[MAXPATH];
    int lstate, lmode, pcnt;
    ftd_jrnnum_t lnum;
    int save_chaining;
    int response;
    int i;
    char *p;

    FTD_TIME_STAMP(FTD_DBG_FLOW1, "%s start\n", argv[0]);
    for (i = 0; i < argc; i++) {
        ftd_trace_flow(FTD_DBG_FLOW1, "%d/%d: %s\n", i, argc, argv[i]);
    }

    putenv("LANG=C");

    /* -- Make sure we're root -- */            /* WR16793 */
    if (geteuid() != 0) {                       /* WR16793 */
        EXIT(EXITANDDIE);                       /* WR16793 */
    }                                           /* WR16793 */

    /* reset open file desc limit */
    setmaxfiles();
    if (initerrmgt (ERRFAC) < 0)  {
        EXIT(EXITANDDIE);
    }

    log_command(argc, argv);  /* trace command line in dtcerror.log */

#if defined(SOLARIS)  /* 1LG-ManyDEV */
    /*Dummy open for gethostbyname / gethostbyaddr */
    if ((dummyfd = open("/dev/null", O_RDONLY)) == -1) {
	    reporterr(ERRFAC, M_FILE, ERRWARN, "/dev/null", strerror(errno));
    }
#endif

    rmdenvcnt = 0;
    while (environ[rmdenvcnt]) {
        if (0 == strncmp(environ[rmdenvcnt], "_RMD_", 5)) {
            if (0 == strncmp(environ[rmdenvcnt], "_RMD_SOCKFD=", 12)) {
                sscanf (&environ[rmdenvcnt][12], "%d", &_rmd_sockfd);
            } else if (0 == strncmp(environ[rmdenvcnt], "_RMD_CONFIGPATH=", 16)) {
                _rmd_configpath = strdup(environ[rmdenvcnt] + 16);
            } else if (0 == strncmp(environ[rmdenvcnt], "_RMD_APPLY=", 11)) {
                sscanf (&environ[rmdenvcnt][11], "%d", &_rmd_apply);
            } else if (0 == strncmp(environ[rmdenvcnt], "_RMD_RPIPEFD=", 13)) {
                sscanf (&environ[rmdenvcnt][13], "%d", &_rmd_rpipefd);
            } else if (0 == strncmp(environ[rmdenvcnt], "_RMD_WPIPEFD=", 13)) {
                sscanf (&environ[rmdenvcnt][13], "%d", &_rmd_wpipefd);
            } else if (0 == strncmp(environ[rmdenvcnt], "_RMD_JLESS=", 11)) {
                sscanf (&environ[rmdenvcnt][11], "%d", &_rmd_jless);
            } else if (0 == strncmp(environ[rmdenvcnt], "_RMD_PMD64=", 11)) { 
                sscanf (&environ[rmdenvcnt][11], "%d", &pmd64); 
            } else if (0 == strncmp(environ[rmdenvcnt], "_RMD_RMD64=", 11)) { 
                sscanf (&environ[rmdenvcnt][11], "%d", &rmd64); 
            }
        }
        ftd_trace_flow(FTD_DBG_FLOW1, "%s\n", environ[rmdenvcnt]);
        rmdenvcnt++;
    }
    SET_LG_JLESS(mysys, _rmd_jless);
    ftd_trace_flow(FTD_DBG_FLOW1,
                    "set mysys jless: %d\n", GET_LG_JLESS(mysys));

    _rmd_state = FTDPMD;
    if (_rmd_apply) {
        sprintf(pgmname,"RMDA_%s",_rmd_configpath+1);
    } else {
        installsigaction();
        sigprocmask(0, NULL, &sigs);
        sprintf(pgmname,"RMD_%s",_rmd_configpath+1);
    }
    argv0 = pgmname;
    setsyslog_prefix("%s", pgmname);
    p = getenv(LOGINTERNAL);
    if (p)
        log_internal = atoi(p);
    p = getenv(SAVEJOURNALS);
    if (p)
        save_journals = atoi(p);
    reporterr(ERRFAC, M_INTERNAL, ERRINFO, _rmd_apply ? "rmda started" : "rmd started");

    /* -- if the config file doesn't exist, report and send an error */
    sprintf(configpath, "%s.cfg", _rmd_configpath);
    sprintf(buf, "%s/%s.cfg", PATH_CONFIG, _rmd_configpath);
    if (0 != stat (buf, &statbuf)) {
        geterrmsg (ERRFAC, M_CFGFILE, fmt);
        sprintf (msg, fmt, argv0, buf, strerror(errno));
        logerrmsg (ERRFAC, ERRCRIT, M_CFGFILE, msg);
        memset ((char*)&header, 0, sizeof(header));
        header.ts = time((time_t*)NULL);
        (void) senderr (mysys->sock, &header, 0L, ERRCRIT, M_CFGFILE, msg);
        (void) sleep ((unsigned)2);
        EXIT(EXITANDDIE);
    }
    response = readconfig (0, 0, 0, configpath);
    if (response == -1) {
        EXIT(EXITANDDIE);
    }

    showtunables(&mysys->tunables);

    lgnum = cfgpathtonum(configpath);

    /*
     * in init_journal() set:
     * _rmd_jrn_state
     * _rmd_jrn_mode
     * _rmd_jrn_num
     * _rmd_cpon
     * _rmd_cppend
     */
    if (init_journal() == -1) {
        reporterr(ERRFAC, M_JRNINIT, ERRWARN, configpath);
    }

    if (_rmd_apply) {
        /* this is an RMDA */
        if (get_journals(jrnphp, 1, 1) <= 0) {
            EXIT(EXITNORMAL);
        }
        /* don't open EXCL since RMD need RDONLY to mirrors */
        save_chaining = mysys->group->chaining;
        mysys->group->chaining = 1;
        if (verify_rmd_entries(mysys->group, configpath, 1) == -1) {
            EXIT(EXITANDDIE);
        }
        mysys->group->chaining = save_chaining;

        reporterr (ERRFAC, M_RMDASTART, ERRINFO, argv0);
        processjournals(NULL);
    } else {
        /* this is an RMD */
        if (!is_parent_master()) {
            reporterr(ERRFAC, M_NOMASTER, ERRCRIT, argv0);
            EXIT(EXITANDDIE);
        }
        if (GET_LG_JLESS(mysys)) {
            /*
             * - JLESS is set, PMD/RMD pair must be just restarted.
             */
            ftd_trace_flow(FTD_DBG_FLOW1,
                    "jrn_mode: %d, jrn_state: %d, jrn_num: %lld, cp_num: %lld\n",
                    _rmd_jrn_mode, _rmd_jrn_state, _rmd_jrn_num, _rmd_jrn_cp_lnum);
            /*
             * When not in checkpoint:
             * If *.i, *.c are left over, apply them, because with the smarter smart refreshes, their content will not be sent again.
             */
            if (!_rmd_cpon &&
                !_rmd_cppend &&
                get_journals(jrnphp, 1,0) > 0)
            {
                memset(&header, 0x0, sizeof(header));
                header.ts = time(NULL);
                
                geterrmsg (ERRFAC, M_CANNOT_JLESS, msg);
                logerrmsg (ERRFAC, ERRCRIT, M_CANNOT_JLESS, msg);
                mysys->isconnected = 1; // Needed hack to enable senderr to behave properly.
                senderr (_rmd_sockfd, &header, 0L, ERRCRIT, M_CANNOT_JLESS, msg);

                if (is_rmdapply_running() == 0)
                {
                    rename_journals(JRN_CO);
                    start_rmdapply(0);
                }
                EXIT(EXITANDDIE);
            }
            
            _rmd_jrn_state = _rmd_jrn_state ? _rmd_jrn_state: JRN_CO;
            _rmd_jrn_mode = MIR_ONLY;
            _rmd_jrn_num = 0;
            /*_rmd_cpon = 0;*/ /* _rmd_cpon is set by init_journal(), and
                                  since nuke_journals() will not delete .p,
                                  do not reset _rmd_cpon here. */
            _rmd_cppend = 0;

            ftd_trace_flow(FTD_DBG_FLOW1,
                    "Reset due to checkpoint on, jrn_mode: %d, jrn_state: %d, "
                    "jrn_num: %lld, cp_num: %lld\n",
                    _rmd_jrn_mode, _rmd_jrn_state, _rmd_jrn_num, _rmd_jrn_cp_lnum);
        }
        if (_rmd_jrn_mode == MIR_ONLY) {
            ftd_trace_flow(FTD_DBG_FLOW1,
                    "\n*** %s opening mirrors EXCL\n", argv0);
            closedevs(-1);
            /* if mirroring - open mirrors */
			if( !_net_bandwidth_analysis )
			{
	            if (verify_rmd_entries(mysys->group, configpath, 1) == -1) {
			        sddisk_t *curdev;
			        int errstat;
			        char devname[MAXPATH], mountp[MAXPATH];

			        curdev = mysys->group->headsddisk;
		            force_dsk_or_rdsk(devname, curdev->mirname, 0);
			        // Check the status returned by dev_mounted; a non-zero status may be returned
			        // when the device is not, in fact, mounted (which case we have seen on Linux when attempting
			        // to replicate a whole disk), then the RMD sent garbage to the PMD as mount point in the error msg.
			        if( (errstat = dev_mounted(devname, mountp) == 1) )
			        {
			            reporterr(ERRFAC, M_HSHK_TGTMOUNTED, ERRWARN, devname, mountp);
					}
			        /* Send an error message to the PMD */
				    if( errstat == 1)
					{
					    // Device is mounted
				        if (geterrmsg (ERRFAC, M_HSHK_TGTMOUNTED, fmt) == 0)
						{
			                sprintf (msg, fmt, devname, mountp);
						}
						else
						{
				            strcpy (msg, fmt);
						}
					}
					else
					{
					    // Device busy but mount point unknown
				        if (geterrmsg (ERRFAC, M_MIROPEN, fmt) == 0)
						{
			                sprintf (msg, fmt, argv0, devname, "open failed but could not determine if dev mounted nor what the mount point would be.");
						}
						else
						{
				            strcpy (msg, fmt);
						}
					}
			        memset (&header, 0, sizeof(header));
			        header.ts = time((time_t*)NULL);
			        mysys->isconnected = 1;
			        if ((errstat = senderr (_rmd_sockfd, &header, 0L, ERRCRIT,
						M_HSHK_TGTMOUNTED, msg)) != 0) {
			            char tmp[265];
			            sprintf (tmp, "status = %d", errstat);
			            reporterr (ERRFAC, M_MIROPEN, ERRCRIT,
				      "senderr", tmp, strerror(errno));
			        }
			        (void) sleep ((unsigned)2);
			        EXIT(EXITANDDIE);
	            }
			}
        } else if (_rmd_jrn_mode == JRN_AND_MIR) {
            /*
             * - smart refresh
             * - checkpoint off
             */
            if (_rmd_jrn_state == JRN_CO) {
                if ((mysys->group->jrnfd =
                    new_journal(lgnum, _rmd_jrn_state, 1)) == -1) {
                    EXIT(EXITANDDIE);
                }
            }
            if (_rmd_jrn_state == JRN_CO) {
                if (!_rmd_cpon) {
                    /* coherent journals exist - and we're not in cp */
                    closedevs(O_RDONLY);
                    start_rmdapply(0);
                }
            }

        } else if (_rmd_jrn_mode == JRN_ONLY) {
            /*
             * - checkpoint on, or incoherent journal entries found.
             */
/* Creation of new_journal is commented out as continuous killing pmd and
 * re-launching creates .c file everytime, when cp is on.
 *          if ((mysys->group->jrnfd =
 *              new_journal(lgnum, _rmd_jrn_state, 0)) == -1) {
 *              EXIT(EXITANDDIE);
 *           }
 */
            if (_rmd_cppend) {
                /* appliable journals exist - cp mode pending */
                closedevs(O_RDONLY);
                /* apply coherent data up to cp */
                start_rmdapply(1);
            } else {
                stop_rmdapply();
                if (!_rmd_cpon) {
                    /* not in cp and not transitioning to cp */
                    // We found incoherent journals remaining...
                    // Now that we have smarter smart refreshes, we do not want to delete existing .i journal entries,
                    // as their content will not be sent again.
                }
            }
        } else {
            reporterr(ERRFAC, M_JRNINIT, ERRCRIT, argv0);
            EXIT(EXITANDDIE);
        }
        if ((fifo = (ioqueue_t*)q_init()) == NULL) {
            reporterr (ERRFAC, M_RMDINIT, ERRCRIT, strerror(errno));
            EXIT(EXITANDDIE);
        }
        (void)initstats(mysys);

        /* WR 43376: check if the RMD was shutdown uncleanly during a Full Refresh, in which case 
           we issue a warning because some of the data acknowledged to the PMD before the shutdown
           might not have been physically written to disk due to async IO or hard disk caching;
           then prompt for verification that a Full Refresh completed automatically at reboot.
           ALSO: take note of this unclean state; if no Full Refresh is performed in this run and
           the RMD exits due to a PMD disconnect, leave the watchdog in place for the next run; if
           a Full Refresh is done, the watchdog will be removed at the end of the refresh
        */
        unclean_shutdown = check_unclean_shutdown( lgnum );
        if( unclean_shutdown )
        {
           remove_FR_watchdog_if_PMD_disconnets = 0;
           reporterr (ERRFAC, M_RMD_UNCLEAN_SD, ERRWARN, lgnum );
        }
        else
        {
           remove_FR_watchdog_if_PMD_disconnets = 1;
        }
        processrequests(NULL);
    }
    EXIT(EXITNORMAL);
    return 0; // To avoid a compiler warning.
} /* main */
