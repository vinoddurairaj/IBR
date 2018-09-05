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
#include "pathnames.h"
#include "aixcmn.h"
#ifdef TDMF_TRACE
FILE *dbgfd;
#endif

char *argv0 = "in.rmd";
char pgmname[128];
char *paths;

struct timeval skosh;

/* -- global variables */
rsync_t grsync;
ioqueue_t *fifo;

int rmdenvcnt;
int _rmd_sockfd;
char* _rmd_configpath;

extern int jrncnt;
extern char tmppaths[MAXLG][32];

extern int _rmd_rpipefd;
extern int _rmd_wpipefd;
extern int _rmd_state;
extern int _rmd_apply;
extern int _rmd_jrn_fd;
extern int _rmd_jrn_cp_lnum;
extern int _rmd_jrn_state;
extern int _rmd_jrn_mode;
extern int _rmd_jrn_num;
extern int _rmd_cpstart;
extern int _rmd_cppend;
extern int _rmd_cpstop;
extern int _rmd_cpon;
extern u_longlong_t _rmd_jrn_offset;
extern u_longlong_t _rmd_jrn_size;

extern char jrnpaths[][32];

extern int max_elapsed_time;

static char msg[256];
static char fmt[256];
static int loc_datalen = 0;
static char *loc_databuf = NULL;

static sigset_t sigs;
static int lgnum;

/****************************************************************************
 * sig_handler -- RMD signal handler 
 ***************************************************************************/
static void
sig_handler (int s) 
{
 
    switch(s) {
    case SIGTERM:
        exit(EXITANDDIE);
    case SIGPIPE:
        exit(EXITNETWORK);
    case SIGUSR1:
        /* tell PMD to die */
        sendkill(_rmd_sockfd);
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
    static int csignal[] = { SIGUSR1, SIGTERM, SIGPIPE };

    sigemptyset(&block_mask);
    numsig = sizeof(csignal)/sizeof(*csignal);

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

    mysys->isconnected = 1;
    mysys->sock = _rmd_sockfd;
    if (0 != initnetwork ()) {
        return (-2);
    }
    strcpy(realhostname, othersys->name);
    if (auth->len > sizeof(auth->auth)) {
        return (-3);
    }
    decodeauth (auth->len, auth->auth, *ts, othersys->hostid, 
        othersys->name, sizeof(othersys->name), othersys->ip);
    if (0 != strcmp(othersys->name, realhostname)) {
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

    /* -- perform license checking */
    if (LICKEY_OK != (response = get_license (RMDFLAG, &lickey))) {
        switch (response) {
        case LICKEY_FILEOPENERR:
            reporterr (ERRFAC, M_LICFILMIS, ERRCRIT, CAPQ);
            break;
        case LICKEY_BADKEYWORD:
            reporterr (ERRFAC, M_LICBADWORD, ERRCRIT, CAPQ);
            break;
        default:
            reporterr (ERRFAC, M_LICFILE, ERRCRIT, CAPQ);
        }
        exit (1);
    }
    if (lickey == NULL) {
        reporterr (ERRFAC, M_LICFILFMT, ERRCRIT, CAPQ);
        exit (1);
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
        exit (1);
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
            exit (EXITANDDIE);
        case -2:
            /* init network problem */
            geterrmsg (ERRFAC, M_NETPROB, msg);
            logerrmsg (ERRFAC, ERRCRIT, M_NETPROB, msg); 
            (void) senderr (fd, header, 0L, ERRCRIT, M_NETPROB, msg);
            exit (EXITNETWORK);
        case -3:
            /* authentication error */
            geterrmsg (ERRFAC, M_BADAUTH, fmt);
            sprintf (msg, fmt, othersys->name);
            logerrmsg (ERRFAC, ERRCRIT, M_BADAUTH, msg); 
            (void) senderr (fd, header, 0L, ERRCRIT, M_BADAUTH, msg);
            exit (EXITANDDIE);
        }
        exit (EXITANDDIE);
    }
    reporterr (ERRFAC, M_RMDSTART, ERRINFO, argv0);

    ack->data = _rmd_cpon;
    ack->devid = header->devid;
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
        if (readsock(mysys->sock, (char*)lrsync, sizeof(rsync_t)) == -1) {
            geterrmsg(ERRFAC, M_BADPROTO, msg);
            logerrmsg(ERRFAC, ERRCRIT, M_BADPROTO, msg);
            (void) senderr (fd, header, 0L, ERRWARN, M_BADPROTO, msg);
            exit (EXITNETWORK);
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
            exit (EXITNETWORK);
        }
        save_datap = rsync->cs.digest;
        rsync->cs.digest += digestlen;

        /* make sure the data buffer is large enough */
        if ((length = rsync->datalen) > loc_datalen) {
            loc_datalen = length; 
            loc_databuf = (char*)ftdrealloc(loc_databuf, loc_datalen);
        }
        rsync->data = loc_databuf;
        rsync->devfd = sd->mirfd;

        /* calculate checksums and insert into queue */
        if ((response = chksumseg(rsync)) == -1) {
            geterrmsg (ERRFAC, M_CHKSUM, fmt);
            sprintf (msg, fmt, rsync->devid);
            logerrmsg (ERRFAC, ERRWARN, M_CHKSUM, msg); 
            (void) senderr (fd, header, 0L, ERRWARN, M_CHKSUM, msg);
            exit(EXITNETWORK);
        }
        /* re-adjust digest pointer */
        rsync->cs.digest = save_datap;
    }
    if (q_put_syncrfd(fifo) == -1) {
        exit(EXITNETWORK);
    }
    return 0;
} /* cmd_rfdchksum */

/****************************************************************************
 * cmd_bfdchksum -- backfresh checksum
 ***************************************************************************/
int
cmd_bfdchksum(int fd, headpack_t *header)
{
    group_t *group;
    sddisk_t *sd;
    int response;
    int length;
    int digestlen;

    rsync_t lrsync[1];
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
        if (readsock(mysys->sock, (char*)lrsync, sizeof(rsync_t)) == -1) {
            geterrmsg(ERRFAC, M_BADPROTO, msg);
            logerrmsg(ERRFAC, ERRCRIT, M_BADPROTO, msg);
            (void) senderr (fd, header, 0L, ERRWARN, M_BADPROTO, msg);
            exit (EXITNETWORK);
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
            exit (EXITNETWORK);
        }
        save_datap = rsync->cs.digest;
        rsync->cs.digest += digestlen;

        /* make sure the data buffer is large enough */
        if ((length = rsync->datalen) > loc_datalen) {
            loc_datalen = length; 
            loc_databuf = (char*)ftdrealloc(loc_databuf, loc_datalen);
        }
        rsync->data = sd->rsync.data = loc_databuf;
        rsync->devfd = sd->mirfd;

        /* calculate checksums and insert into queue */
        if ((response = chksumseg(rsync)) == -1) {
            geterrmsg (ERRFAC, M_CHKSUM, fmt);
            sprintf (msg, fmt, rsync->devid);
            logerrmsg (ERRFAC, ERRWARN, M_CHKSUM, msg); 
            (void) senderr (fd, header, 0L, ERRWARN, M_CHKSUM, msg);
            exit(EXITNETWORK);
        }
        /* re-adjust digest pointer */
        rsync->cs.digest = save_datap;
    }
    if (q_put_syncbfd(fifo) == -1) {
        exit(EXITNETWORK);
    }
    return 0;
} /* cmd_bfdchksum */

/****************************************************************************
 * cmd_chkconfig -- make sure PMD/RMD agree
 ***************************************************************************/
int
cmd_chkconfig(int fd, headpack_t *header)
{
    ackpack_t ack[1];
    group_t *group;
    sddisk_t *sd;
    rdevpack_t rdev;
    struct stat statbuf;
    char path[MAXPATH];
    int response;
    int disksize;

    group = mysys->group;

    if (-1 == (response = readsock(fd, (char*)&rdev, sizeof(rdevpack_t)))) {
        return (response);
    }
    if (-1 == (response = doconfiginfo (&rdev, header->ts))) {
        geterrmsg (ERRFAC, M_MIRMISMTCH, fmt);
        sprintf (msg, fmt, mysys->configpath);
        logerrmsg (ERRFAC, ERRCRIT, M_MIRMISMTCH, msg); 
        (void) senderr (fd, header, 0L, ERRCRIT, M_MIRMISMTCH, msg);
        exit (EXITANDDIE);
    }
    for (sd = group->headsddisk; sd; sd = sd->n) {
        if (0 == strcmp(rdev.path, sd->mirname)) {
            if (_rmd_jrn_mode == MIR_ONLY) {
                if ((disksize = fdisksize(sd->mirfd)) == (u_long) -1) {
                    reporterr (ERRFAC, M_MIRSIZ, ERRWARN, sd->mirname, 
                        mysys->configpath, strerror(errno));
                }
            } else {
                disksize = INT_MAX;
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
        exit (EXITANDDIE);
    }
    sd->rsync.devid = header->devid;

    /* -- return mirror volume size for a configuration query */
    ack->data = disksize;
    ack->devid = header->devid;
    if (sendack(fd, header, ack) == -1) {
        return -1;
    } 
    return disksize;
} /* cmd_chkconfig */

/****************************************************************************
 * cmd_write -- BAB write to mirror(s)
 ***************************************************************************/
int
cmd_write(int fd, headpack_t *header)
{
    if (q_put_data(fifo, header) == -1) {
        exit(EXITNETWORK);
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
        exit(EXITNETWORK);
    }
    return 0;
} /* cmd_rfdfwrite */

/****************************************************************************
 * cmd_bfddevs -- put device into backfresh
 ***************************************************************************/
int
cmd_bfddevs(int fd, headpack_t *header)
{
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
    DPRINTF("\n*** BFD device: %s:%d restart sector offset = %d\n", sd->devname, sd->mirfd, header->offset);
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
        exit(EXITNETWORK);
    }
    return 0;
} /* cmd_bfddevs */

/****************************************************************************
 * cmd_bfdeve --  
 ***************************************************************************/
int
cmd_bfddeve(int fd, headpack_t *header)
{
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
    ackpack_t ack[1];
    sddisk_t *sd;

    sprintf(fmt, "%s/%03d.off", PATH_RUN_FILES, header->devid);
    unlink(fmt);
    _rmd_state = FTDPMD;

    memset(ack, 0, sizeof(ackpack_t));
    ack->devid = header->devid;
    ack->data = ACKBFDCMD;
    if (sendack(fd, header, ack) == -1) {
        exit(EXITNETWORK);
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
    DPRINTF("\n*** RFD device: %s restart sector offset = %d\n",sd->devname, header->offset);
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
        exit(EXITNETWORK);
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

    memset(ack, 0, sizeof(ackpack_t));
    ack->devid = header->devid;
    ack->data = ACKRFDCMD;
    if (sendack(fd, header, ack) == -1) {
        exit(EXITNETWORK);
    }
    return 0;
} /* cmd_rfddeve */

/****************************************************************************
 * cmd_bfdfstart -- start backfresh 
 ***************************************************************************/
int
cmd_bfdstart(int fd, headpack_t *header)
{
    ackpack_t ack[1];

    _rmd_state = FTDBFD;

    memset(ack, 0, sizeof(ackpack_t));
    ack->devid = header->devid;
    ack->data = ACKBFDCMD;
    if (sendack(fd, header, ack) == -1) {
        exit(EXITNETWORK);
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

    stop_rmdapply();
    nuke_journals();

    closedevs(-1);
    /* re-open devices EXCL */
    if (verify_rmd_entries(mysys->group, mysys->configpath, 1) == -1) {
        exit(EXITANDDIE);
    }
    _rmd_jrn_state = JRN_CO;
    _rmd_jrn_mode = MIR_ONLY;
    _rmd_state = FTDRFD;

    memset(ack, 0, sizeof(ackpack_t));
    ack->devid = header->devid;
    ack->data = ACKRFDCMD;
    if (sendack(fd, header, ack) == -1) {
        exit(EXITNETWORK);
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

    _rmd_state = FTDPMD;
    close_journal(mysys->group); 

    memset(ack, 0, sizeof(ackpack_t));
    ack->devid = header->devid;
    ack->data = ACKRFDCMD;
    if (sendack(fd, header, ack) == -1) {
        exit(EXITNETWORK);
    }
    return 0;
} /* cmd_rfdfend */

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
        exit(EXITNETWORK);
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
        exit(EXITNETWORK);
    }
    return 0;
} /* cmd_noop */

/****************************************************************************
 * cmd_exit -- RMD go away
 ***************************************************************************/
int
cmd_exit(int fd, headpack_t *header)
{
    group_t *group;
    sddisk_t *sd;
    sddisk_t *sdnext;

    group = mysys->group;

    for (sd = group->headsddisk; sd; sd = sd->n) {
        sdnext = sd->n;
        (void)close(sd->mirfd);
        free(sd);
        sd = sdnext;
    }
    free (loc_databuf);
    close(mysys->sock);
    reporterr (ERRFAC, M_RMDEXIT, ERRINFO, argv0);
    exit (EXITNORMAL);
} /* cmd_exit */

/****************************************************************************
 * readitem -- RMD read an item from the network or a stream
 ***************************************************************************/
int
readitem (int fd)
{
    headpack_t header;
    ackpack_t ack;
    int response;
    int lgnum;

    group_t *group;

    group = mysys->group;
    lgnum = cfgpathtonum(mysys->configpath);

    if (-1 == (response = readsock(fd, (char*)&header, sizeof(headpack_t)))) {
        return response;
    }
    DPRINTF("\n*** readitem: header.magicvalue = %08x",header.magicvalue);
    DPRINTF("\n*** readitem: header.cmd = %d",header.cmd);

    if (header.magicvalue != MAGICHDR) {
        if (mysys != NULL) 
            reporterr (ERRFAC, M_HDRMAGIC, ERRWARN);
        return -1;
    }
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
    case CMDBFDCHKSUM:
        cmd_bfdchksum(fd, &header);
        break;
    case CMDWRITE:
        cmd_write(fd, &header);
        break;
    case CMDRFDFWRITE:
        cmd_rfdfwrite(fd, &header);
        break;
    case CMDRFDFSTART:
        cmd_rfdfstart(fd, &header);
        break;
    case CMDRFDEND:
        break;
    case CMDRFDFEND:
        cmd_rfdfend(fd, &header);
        break;
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
    case CMDRFDDEVS:
        cmd_rfddevs(fd, &header);
        break;
    case CMDRFDDEVE:
        cmd_rfddeve(fd, &header);
        break;
    case CMDEXIT:
        cmd_exit(fd, &header);
        break;
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
        return -1;
    }
    /* check for checkpoint transition */
    if (action == FTDQCPOFFS) {
        /* should we transition OUT of checkpoint ? */
        sprintf(lgname, "s%03d", lgnum);
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
        } else if (!_rmd_cpon
        && !_rmd_cpstart) {
            reporterr(ERRFAC, M_CPOFFAGAIN, ERRWARN, lgname);
            return 1;
        }
        if (_rmd_cpon
        || _rmd_cpstart) {
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
void *
processrequests(void *args)
{
    headpack_t header[1];
    ackpack_t ack[1];
    struct timeval seltime[1];
    fd_set readfds[1];
    fd_set readfds_copy[1];
    time_t now, lastts;
    int deltatime, elapsed_time;
    int connect_timeo;
    int lnum, lstate, lmode;
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
 
    do {
        *readfds_copy = *readfds;
        response = 1;

        rc = select(nselect, readfds_copy, NULL, NULL, seltime);

        switch(rc) {
        case -1:
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            } else if (errno == 0 || errno == ENOENT) {
                reporterr(ERRFAC, M_RMDEXIT, ERRINFO, argv0);
                exit(EXITANDDIE);
            } else {
                reporterr(ERRFAC, M_SOCKRERR, ERRCRIT, argv0, strerror(errno));
                exit(EXITANDDIE);
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
                if ((response = cp_action(_rmd_rpipefd)) == -1) {
                    /* error */
                    response = -1;
                }
            } else if (FD_ISSET(_rmd_sockfd, readfds_copy)) {
                if ((response = readitem(_rmd_sockfd)) != 1) {
                    closeconnection ();
                    exit (EXITNORMAL);
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
            if (net_test_channel(connect_timeo) <= 0) {
                reporterr(ERRFAC, M_NETBROKE, ERRWARN,
                    argv0, othersys->name);
                exit(EXITANDDIE);
            }
            elapsed_time = 0;
            lastts = 0;
        }

        /* should we dump stats ? */
        dump_rmd_stats(mysys);

        if (_rmd_cppend) {
            transition = 0;
            /* we are waiting to transition to cp mode */
            if (get_journals(jrnpaths, 1, 1) == 0) {
                transition = 1;
            }
            parse_journal_name(jrnpaths[0], &lnum, &lstate, &lmode);
            len = strlen(jrnpaths[0]);
            if (jrnpaths[0][len-1] == 'i'
            || lnum > _rmd_jrn_cp_lnum) {
                transition = 1;
            }
            if (transition) {
                /* don't care if this fails! - transition to cp regardless */
                exec_cp_cmd(lgnum, POST_CP_ON, 0);
                reporterr(ERRFAC, M_CPON, ERRINFO, argv0); 
                /* send same msg to PMD */
                memset(header, 0, sizeof(headpack_t));
                ack->data = ACKCPON;
                sendack(mysys->sock, header, ack); 
                _rmd_cppend = 0;
                _rmd_cpon = 1;
                _rmd_jrn_mode = JRN_ONLY;
            }
        } else {
            /* if we were jrnling/mirrng - can we go back to mirrng only ? */
            if (_rmd_jrn_mode == JRN_AND_MIR) {
                if ((jrncnt = get_journals(tmppaths, 1, 1)) == 0) { 
                    /* no more journals - go back to mirroring */
                    stop_rmdapply();
                    closedevs(-1);
                    /* re-open devices EXCL */
                    if (verify_rmd_entries(mysys->group, mysys->configpath, 1) == -1) {
                        exit(EXITANDDIE);
                    }
                    _rmd_jrn_state = JRN_CO;
                    _rmd_jrn_mode = MIR_ONLY;
                }
            }
        }
    } while (response == 1);

} /* processrequests */

/****************************************************************************
 * processjournals -- Process journals
 ***************************************************************************/
void*
processjournals (void *args)
{
    group_t *group;
    jrnheader_t jrnheader[1];
    int lnum, lstate, lmode;
    int lgnum;
    int rc;
    int n;

    lgnum = cfgpathtonum(mysys->configpath);
    group = mysys->group;

    _rmd_jrn_cp_lnum = -1;

    while(1) {
        if ((jrncnt = get_journals(jrnpaths, 1, 1)) == 0) {
            /* no more journals */
            reporterr (ERRFAC, M_RMDAEND, ERRINFO, argv0);
            closedevs(-1);
            sleep(2);
            exit(EXITANDDIE);
        }
        parse_journal_name(jrnpaths[0], &lnum, &lstate, &lmode);
        /* if lnum > checkpoint journal num - done */
        if (_rmd_jrn_cp_lnum >= 0
        && lnum > _rmd_jrn_cp_lnum) {
            reporterr (ERRFAC, M_RMDAEND, ERRINFO, argv0);
            closedevs(-1);
            exit(EXITANDDIE);
        }
        _rmd_jrn_num = lnum;
        _rmd_jrn_state = lstate;
        _rmd_jrn_mode = lmode;

        sprintf(group->journal, "%s/%s",group->journal_path, jrnpaths[0]);
        if ((group->jrnfd = open(group->journal, O_RDWR)) == -1) {
            reporterr (ERRFAC, M_FILE, ERRCRIT, group->journal, strerror(errno)); 
            reporterr (ERRFAC, M_RMDAPPLY, ERRCRIT, argv0); 
            exit (EXITANDDIE); 
        } 
        n = sizeof(jrnheader_t);
        if ((rc = ftdread(group->jrnfd, jrnheader, n)) != n) {
            reporterr (ERRFAC, M_JRNHEADER, ERRCRIT,
                group->journal, strerror(errno)); 
            reporterr (ERRFAC, M_RMDAPPLY, ERRCRIT, argv0); 
            exit (EXITANDDIE); 
        }
        if (jrnheader->magicnum != MAGICJRN) {
            reporterr (ERRFAC, M_JRNMAGIC, ERRCRIT, group->journal); 
            reporterr (ERRFAC, M_RMDAPPLY, ERRCRIT, argv0); 
            exit (EXITANDDIE); 
        }
        rc = apply_journal(group);
        if (rc != 0) {
            reporterr (ERRFAC, M_RMDAPPLY, ERRCRIT, argv0); 
            exit(EXITANDDIE);
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
    int lnum, lstate, lmode, pcnt;
    int save_chaining;
    int response;
    int len;

    /* reset open file desc limit */
    setmaxfiles();
    if (initerrmgt (ERRFAC) < 0)  {
        exit (1); 
    }

    rmdenvcnt = 0;
    while (environ[rmdenvcnt]) {
        if (0 == strncmp(environ[rmdenvcnt], "_RMD_", 5)) {
            if (0 == strncmp(environ[rmdenvcnt], "_RMD_SOCKFD=", 12)) {
                sscanf (&environ[rmdenvcnt][12], "%d", &_rmd_sockfd);
            } else if (0 == strncmp(environ[rmdenvcnt], "_RMD_CONFIGPATH=", 16)) {
                len = strlen(environ[rmdenvcnt])*sizeof(char);
                _rmd_configpath = (char*)ftdmalloc(len);
                sscanf (&environ[rmdenvcnt][16], "%s", _rmd_configpath);
            } else if (0 == strncmp(environ[rmdenvcnt], "_RMD_APPLY=", 11)) {
                sscanf (&environ[rmdenvcnt][11], "%d", &_rmd_apply);
            } else if (0 == strncmp(environ[rmdenvcnt], "_RMD_RPIPEFD=", 13)) {
                sscanf (&environ[rmdenvcnt][13], "%d", &_rmd_rpipefd);
            } else if (0 == strncmp(environ[rmdenvcnt], "_RMD_WPIPEFD=", 13)) {
                sscanf (&environ[rmdenvcnt][13], "%d", &_rmd_wpipefd);
            }
        }
        rmdenvcnt++;
    }
    _rmd_state = FTDPMD;
    if (_rmd_apply) {
        sprintf(pgmname,"RMDA_%s",_rmd_configpath+1);
    } else {
        installsigaction();
        sigprocmask(0, NULL, &sigs);
        sprintf(pgmname,"RMD_%s",_rmd_configpath+1);
    }
    argv0 = pgmname;

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
        exit (EXITANDDIE);
    }
    response = readconfig (0, 0, 0, configpath);
    if (response == -1) {
        exit(EXITANDDIE);
    }
    lgnum = cfgpathtonum(configpath);

    if (init_journal() == -1) {
        reporterr(ERRFAC, M_JRNINIT, ERRWARN, configpath);
    }
    if (_rmd_apply) {
        /* this is an RMDA */
        if (get_journals(jrnpaths, 1, 1) == 0) {
            exit(0);
        }
        /* don't open EXCL since RMD need RDONLY to mirrors */
        save_chaining = mysys->group->chaining;
        mysys->group->chaining = 1;
        if (verify_rmd_entries(mysys->group, configpath, 1) == -1) {
            exit(EXITANDDIE);
        }
        mysys->group->chaining = save_chaining;
        
        reporterr (ERRFAC, M_RMDASTART, ERRINFO, argv0);
        processjournals(NULL);
    } else {
        /* this is an RMD */
        if (!is_parent_master()) {
            reporterr(ERRFAC, M_NOMASTER, ERRCRIT, argv0);
            exit (EXITANDDIE);
        }
        if (_rmd_jrn_mode == MIR_ONLY) {
            DPRINTF("\n*** %s opening mirrors EXCL\n",argv0);
            closedevs(-1);
            /* if mirroring - open mirrors */
            if (verify_rmd_entries(mysys->group, configpath, 1) == -1) {
                exit(EXITANDDIE);
            }
        } else if (_rmd_jrn_mode == JRN_AND_MIR) {
            if (_rmd_jrn_state == JRN_CO) {
                if ((mysys->group->jrnfd =
                    new_journal(lgnum, _rmd_jrn_state, 1)) == -1) {
                    exit(EXITANDDIE);
                }
            }
            if (_rmd_jrn_state == JRN_CO) {
                if (!_rmd_cpon) {
                    /* coherent journals exist - and we're not in cp */
                    closedevs(O_RDONLY);
                    start_rmdapply();
                }
            }

        } else if (_rmd_jrn_mode == JRN_ONLY) {
            if ((mysys->group->jrnfd =
                new_journal(lgnum, _rmd_jrn_state, 1)) == -1) {
                exit(EXITANDDIE);
            }
            if (_rmd_cppend) {
                /* appliable journals exist - cp mode pending */
                closedevs(O_RDONLY);
                /* apply coherent data up to cp */
                start_rmdapply(1);
            } else {
                stop_rmdapply();
                if (!_rmd_cpon) {
                    /* not in cp and not transitioning to cp */
                    nuke_journals();
                }
            }
        } else {
            reporterr(ERRFAC, M_JRNINIT, ERRCRIT, argv0);
            exit(EXITANDDIE);
        }
        if ((fifo = (ioqueue_t*)q_init()) == NULL) {
            reporterr (ERRFAC, M_RMDINIT, ERRCRIT, strerror(errno)); 
            exit(EXITANDDIE);
        }
        (void)initstats(mysys);
        (void*)processrequests(NULL);
    }
    exit(0);
} /* main */
