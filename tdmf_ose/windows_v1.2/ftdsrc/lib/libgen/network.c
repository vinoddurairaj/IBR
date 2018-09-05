/**************************************************************************
 * network.c - FullTime Data Network communications module
 *
 * (c) Copyright 1998 FullTime Software, Inc. All Rights Reserved
 *
 * This module defines the functions for implementing network communications
 * between the PMD and RMD daemons.
 *
 * History:
 *   10/14/96 - Steve Wahl	- original code
 *
 ***************************************************************************
 */
#ifdef NEED_BIGINTS
#include "bigints.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stropts.h>
#include <poll.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/signal.h>

#include "ftdio.h"
#include "errors.h"
#include "config.h"
#include "network.h"
#include "pathnames.h"
#include "process.h"
#include "platform.h"
#include "common.h"
#include "aixcmn.h"

char *databuf = NULL;
int datalen = 0;

ioqueue_t *fifo;

int g_rfddone = 0;
int g_ackhup = 0;

int max_elapsed_time = 30;
int elapsed_time = 0;

int _pmd_cpstart;
int _pmd_cpstop;
int _pmd_cpon;
int _pmd_state;
int _pmd_state_change;
char *_pmd_configpath;

int exitsuccess;
int exitfail;

static struct timeval skosh;

static u_char *cbuf = NULL;
static int clen = 0;
static int net_write_timeo = 0;

extern sddisk_t *get_lg_dev(group_t*, int);
extern int compress(u_char*, u_char*, int);

extern char *argv0;

static char pmdversionstring[256];
static char rmdversionstring[256];

EntryFunc entry_null;
extern EntryFunc get_counts;

extern float compr_ratio;

extern int compress_buf_len;
extern char* compress_buf;

int net_writable = 1;

int net_write_vector(int fd, struct iovec *iov, int iovcnt, int role);
int process_acks(void);

/*
 * set_tcp_send_low -- set TCP send low water mark 
 */
int
set_tcp_send_low(void) 
{
#if defined(_AIX)
    size_t len; 
#else /* defined(_AIX) */
    int len;
#endif /* defined(_AIX) */
    int max_packet, n, rc;

    len = sizeof(int); 
    rc = getsockopt(mysys->sock, SOL_SOCKET, SO_SNDBUF,
#if defined(_AIX)
        (void *)&max_packet, 
#else /* defined(_AIX) */
        (char*)&max_packet, 
#endif /* defined(_AIX) */
        &len);
    DPRINTF((ERRFAC,LOG_INFO,"\n*** TCP send buffer size = %d\n",max_packet));
    if (rc == -1) {
        return -1;
    }
    rc = setsockopt(mysys->sock, SOL_SOCKET, SO_SNDLOWAT,
#if defined(_AIX)
        (void*)&max_packet, sizeof(int));
#else /* defined(_AIX) */
        (char*)&max_packet, sizeof(int));
#endif

    len = sizeof(int);
    getsockopt(mysys->sock, SOL_SOCKET, SO_SNDLOWAT,
        (char*)&n, &len);
    DPRINTF((ERRFAC,LOG_INFO,"\n*** TCP send lowater = %d\n",n));
    return 0;
}

/*
 * set_tcp_send_timeo -- set TCP send timeout
 */
int
set_tcp_send_timeo(void) 
{
    struct timeval timeo[1];
#if defined(_AIX)
    size_t len; 
#else /* defined(_AIX) */
    int len;
#endif /* defined(_AIX) */

    timeo->tv_usec = 100000;
    timeo->tv_sec = 0;

    setsockopt(mysys->sock, SOL_SOCKET, SO_SNDTIMEO,
        (char*)timeo, sizeof(struct timeval));
#ifdef TDMF_TRACE
    len = sizeof(struct timeval);
    getsockopt(mysys->sock, SOL_SOCKET, SO_SNDTIMEO,
#if defined(_AIX)
        (void *)timeo,
#else /* defined(_AIX) */
        (char*)timeo,
#endif /* defined(_AIX) */
        &len);
    fprintf(stderr,"\n*** TCP send timeout = %d:%d\n",
        timeo->tv_sec, timeo->tv_usec);
#endif
    return 0;
}

/*
 * set_tcp_nonb -- set TCP socket to non-blocking 
 */
int
set_tcp_nonb(void) 
{
    int val;

    /* get the socket state */
    val = fcntl(mysys->sock, F_GETFL, 0);

    /* set the socket to non-blocking */
    fcntl(mysys->sock, F_SETFL, val | O_NONBLOCK);
}

/****************************************************************************
 * initnetwork -- initialize the network interface
 ***************************************************************************/
int
initnetwork (void) 
{
    int retval;
    int lineno;

    lineno = 0;
    /* -- verify our hostname and/or IP address */
    retval = isthisme (mysys->name, mysys->ip);
    switch (retval) {
    case -1:
        reporterr (ERRFAC, M_BADHOSTNAM, ERRCRIT, mysys->configpath, lineno, 
            mysys->name);
        return (1);
    case -2:
        reporterr (ERRFAC, M_BADIPADDR, ERRCRIT, mysys->configpath, lineno,
            mysys->name, iptoa(mysys->ip));
        return (1);
    case -3:
        reporterr (ERRFAC, M_BADNAMIP, ERRCRIT, mysys->configpath, lineno,
            mysys->name, iptoa(mysys->ip));
        return (1);
    case 0: return (0);
    case 1: return (0);
    }

    return (1);
}

/****************************************************************************
 * createconnection -- establishes a connection with a remote daemon
 *                     returns 0 = success, 1 = failure
 ***************************************************************************/
int
createconnection (int reporterrflag)
{
    struct hostent *hp;
    struct sockaddr_in myaddr; 
    struct sockaddr_in servaddr;
    int len;
    int n;
    int result;

    if (mysys->isconnected) {
        if (reporterrflag) reporterr (ERRFAC, M_CNCTED, ERRWARN);
        return 0;
    }
    if ((mysys->sock = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        if (errno == EMFILE) {
            reporterr (ERRFAC, M_FILE, ERRCRIT, "socket", strerror(errno));
        }
        reporterr (ERRFAC, M_SOCKFAIL, ERRCRIT, strerror(errno));
        return 1;
    }
    memset((char*)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htons(INADDR_ANY);
    myaddr.sin_port = htons(0);

    len = sizeof(myaddr);
    if (bind(mysys->sock, (struct sockaddr *) &myaddr, len) < 0) {
        reporterr (ERRFAC, M_SOCKBIND, ERRCRIT, strerror(errno));
        close(mysys->sock);
        return 1;
    }
    memset ((char*)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(othersys->secondaryport);
  
    /* -- if the other system host name is empty or gethostbyname fails
     *    then try gethostbyaddr */
    hp = 0;
    if (othersys->ip != 0L) {
        hp = gethostbyaddr((char*)&othersys->ip, sizeof(u_long), AF_INET);
    } else if (strlen(othersys->name) > 0) {
        hp = gethostbyname(othersys->name);
    }
    if (hp == 0) {
        reporterr (ERRFAC, M_HOSTLKUP, ERRCRIT, othersys->name);
        close(mysys->sock);
        return 1;
    }
    /* set the TCP SNDBUF, RCVBUF to the maximum allowed */
    n = 256*1024;
    setsockopt(mysys->sock, SOL_SOCKET, SO_SNDBUF, (char*)&n, sizeof(int));
    setsockopt(mysys->sock, SOL_SOCKET, SO_RCVBUF, (char*)&n, sizeof(int));

    memcpy((caddr_t)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);
    if (0 > (result = connect(mysys->sock,
        (struct sockaddr*)&servaddr,sizeof(servaddr)))) {
        if (reporterrflag) {
            reporterr (ERRFAC, M_CONERR, ERRWARN,
                hp->h_name, othersys->secondaryport, strerror(errno));
        }
        close(mysys->sock);
        return 1;
    }
    n = 1;
    if (setsockopt(mysys->sock, SOL_SOCKET, SO_KEEPALIVE,(char*)&n,sizeof(int))) {
        reporterr (ERRFAC, M_SOCKOP, ERRCRIT, strerror(errno));
        close(mysys->sock);
        return (1);
    }
    mysys->isconnected = 1;

    return (0);
} /* createconnection */

/****************************************************************************
 * closeconnection -- severs the connection between two daemons orderly
 ***************************************************************************/
void
closeconnection (void)
{
    headpack_t header;
    sddisk_t* sd;
    sddisk_t* nextsd;

    if (mysys == NULL) {
        return;
    }
    if (mysys->isconnected) {
        if (ISPRIMARY(mysys)) {
            header.magicvalue = MAGICHDR;
            header.cmd = CMDEXIT;
            (void)time(&header.ts);
            header.ackwanted = 0;
            (void)writesock(mysys->sock, (char*)&header, sizeof(headpack_t));
            close(mysys->sock);
            mysys->isconnected = 0;
        }
    }
    sd = mysys->group->headsddisk; 
    while(sd) {
        nextsd = sd->n;
        close(sd->rsync.devfd);
        free (sd);
        sd = nextsd;
    }
    sd = othersys->group->headsddisk;
    while (sd) {
        nextsd = sd->n;
        close(sd->rsync.devfd);
        free (sd);
        sd = nextsd;
    }
    mysys->isconnected = 0;

    if (ISSECONDARY(mysys)) {
        reporterr (ERRFAC, M_RMDEXIT, ERRINFO, argv0);
        exit (EXITNORMAL);
    }
} /* closeconnection */

/****************************************************************************
 * sendhup -- send a hup packet to the RMD 
 ***************************************************************************/
int
sendhup(void) 
{
    headpack_t header[1];

    memset(header, 0, sizeof(header));
    header->magicvalue = MAGICHDR;
    header->cmd = CMDHUP;
    header->ackwanted = 1;
    if (-1 == writesock(mysys->sock, (char*)header, sizeof(headpack_t))) {
        exit (EXITNETWORK);
    }
    return 0;
} /* sendhup */
  
/****************************************************************************
 * send_cp_err -- send a checkpoint error packet to the RMD 
 ***************************************************************************/
int
send_cp_err(int msgtype) 
{
    headpack_t header[1];

    memset(header, 0, sizeof(header));
    header->magicvalue = MAGICHDR;
    header->cmd = msgtype;
    header->ackwanted = 1;
    if (-1 == writesock(mysys->sock, (char*)header, sizeof(headpack_t))) {
        exit (EXITNETWORK);
    }
    return 0;
} /* send_cp_err */
  
/****************************************************************************
 * send_cp_err_ack -- send a checkpoint error packet to the PMD 
 ***************************************************************************/
int
send_cp_err_ack(int msgtype) 
{
    ackpack_t ack[1];

    memset(ack, 0, sizeof(ackpack_t));
    ack->magicvalue = MAGICACK;
    ack->acktype = ACKNORM;
    ack->data = msgtype;
    if (-1 == writesock(mysys->sock, (char*)ack, sizeof(ackpack_t))) {
        exit (EXITNETWORK);
    }
    return 0;
} /* send_cp_err */
  
/****************************************************************************
 * sendnoop -- send a noop packet to the RMD if no data in writelog 
 ***************************************************************************/
int
sendnoop(void) 
{
    headpack_t header[1];

    memset(header, 0, sizeof(header));
    header->magicvalue = MAGICHDR;
    header->cmd = CMDNOOP;
    header->ackwanted = 1;

    if (-1 == writesock(mysys->sock, (char*)header, sizeof(headpack_t))) {
        exit (EXITNETWORK);
    }

    return 0;
} /* sendnoop */
  
/****************************************************************************
 * send_cp_on -- write checkpoint-start entry to journal
 ***************************************************************************/
int
send_cp_on(int lgnum) 
{
    group_t *group;
    stat_buffer_t sb;
    char msgbuf[DEV_BSIZE];
    int maxtries;
    int rc;
    int i;

    group = mysys->group;
    memset(&sb, 0, sizeof(stat_buffer_t));

    sprintf(msgbuf, "%s|%s", MSG_CPON, "");
    sb.lg_num = lgnum;
    sb.len = sizeof(msgbuf);
    sb.addr = msgbuf;

    maxtries = 10;
    for (i = 0; i < maxtries; i++) {
        rc = ioctl(group->devfd, FTD_SEND_LG_MESSAGE, &sb);
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
} /* send_cp_on */

/****************************************************************************
 * send_cp_off -- write checkpoint-stop entry to journal
 ***************************************************************************/
int
send_cp_off(int lgnum) 
{
    group_t *group;
    stat_buffer_t sb;
    char msgbuf[DEV_BSIZE];
    int maxtries;
    int rc;
    int i;

    group = mysys->group;
    memset(&sb, 0, sizeof(stat_buffer_t));

    sprintf(msgbuf, "%s|%s", MSG_CPOFF, "");
    sb.lg_num = lgnum;
    sb.len = sizeof(msgbuf);
    sb.addr = msgbuf;

    maxtries = 10;
    for (i = 0; i < maxtries; i++) {
        rc = ioctl(group->devfd, FTD_SEND_LG_MESSAGE, &sb);
        if (rc != 0) {
            if (errno == EAGAIN) {
                usleep(200000);
                continue;
            }
        } else {
            break;
        }
        reporterr(ERRFAC, M_SENDLGMSG, ERRCRIT);
        exit(EXITANDDIE);
    }
    return 0;
} /* send_cp_off */

/****************************************************************************
 * sendkill -- send an kill ACK from RMD to the PMD
 ***************************************************************************/
int
sendkill (int fd)
{
    headpack_t header[1];
    ackpack_t ack[1];

    memset(header, 0, sizeof(headpack_t));
    memset(ack, 0, sizeof(ackpack_t));
    ack->data = ACKKILLPMD;
    sendack(fd, header, ack);

    return 0;
} /* sendkill */

/****************************************************************************
 * sendack -- send an ACK from RMD to the PMD
 ***************************************************************************/
int
sendack (int fd, headpack_t *header, ackpack_t *ack)
{
    ackpack_t lack;
    int rc;
  
    memset(&lack, 0, sizeof(ackpack_t));

    /* -- send the ACK */
    lack.acktype = ACKNORM;
    lack.magicvalue = MAGICACK;
    lack.devid = header->devid;
    lack.ts = header->ts;
    lack.ackoff = ack->ackoff;
    lack.data = ack->data;
    lack.mirco = ack->mirco;

    DPRINTF((ERRFAC,LOG_INFO, "\n*** sendack: ack->acktype, ack->data = %d, %d\n",         lack.acktype, lack.data));
 
    rc = writesock(fd, (char*)&lack, sizeof(ackpack_t));

    return rc;
} /* sendack */

/****************************************************************************
 * senderr -- send an ERR to the PMD
 ***************************************************************************/
int
senderr (int fd, headpack_t *header, u_long data, u_long class, char *name, char *msg)
{
    ackpack_t ack;
    errpack_t err;

    skosh.tv_sec = 10;
    skosh.tv_usec = 0;

    /* -- fake sending of the ack if the unit isn't the socket */
    if (NULL != mysys && 0 == mysys->isconnected) {
        return 1;
    }
    /* -- send the ACK */
    ack.magicvalue = MAGICACK;
    ack.acktype = ACKERR;
    ack.ts = header->ts;
    ack.data = data;
    if (-1 == writesock(fd, (char*)&ack, sizeof(ackpack_t))) {
        return -1;
    }
    err.magicvalue = MAGICERR;
    err.errcode = class;
    strcpy (err.errkey, name);
    if (strlen(msg) > 255) {
        msg[255] = '\0';
    }
    strcpy (err.msg, msg);
    (void) writesock(fd, (char*)&err, sizeof(errpack_t));
    DPRINTF((ERRFAC,LOG_INFO, "senderr -- sent message over the socket\n"));
    (void) select (0, NULL, NULL, NULL, &skosh); 
    return 0;
} /* senderr */

/****************************************************************************
 * sendversion -- send protocol version checking information
 ***************************************************************************/
int
sendversion (int fd)
{
    headpack_t header;
    versionpack_t version;
    int response;
    ackpack_t ack;
    int i;
    int retval;

    memset(&header, 0, sizeof(header));
    memset(&version, 0, sizeof(version));

    /* -- fake out if this isn't going to the socket */
    if (!mysys->isconnected) {
        return (1);
    }
    if (fd != mysys->sock) {
        return (1);
    }
    /* -- get the system information */
    header.magicvalue = MAGICHDR;
    header.cmd = CMDVERSION;
    header.ackwanted = 1;
    header.devid = 0;
    header.len = 0;
    header.offset = 0;
    header.data = cfgpathtonum(mysys->configpath);
    (void) time(&header.ts);
    strcpy (version.configpath, 
        &(othersys->configpath[strlen(othersys->configpath)-8]));
    (void) time(&version.pmdts);
    /* -- set default protocol version numbers */
    strcpy (pmdversionstring, "4.0.0");
    strcpy (rmdversionstring, "4.0.0");
    strcpy (version.version, pmdversionstring);
    /* -- process the version number put into the works by the Makefile */
#ifdef VERSION
    strcpy (pmdversionstring, VERSION);
    i = 0;
    /* -- eliminate beta, intermediate build information from version */
    while (pmdversionstring[i]) {
        if ((!(isdigit(pmdversionstring[i]))) && pmdversionstring[i] != '.') {
            pmdversionstring[i] = '\0';
            break;
        }
        i++;
    }
    strcpy (version.version, pmdversionstring);
#endif
    /* -- send header and version packets */
    if (-1 == writesock (fd, (char*)&header, sizeof(headpack_t))) return (-1);
    (void) time(&version.pmdts);
    if (-1 == writesock (fd, (char*)&version, sizeof(versionpack_t))) return (-1);
    while (1 != (response = checkresponse (fd, &ack))) {
        if (0 >= response) {
            exit (EXITANDDIE);
        }
    }
    /* -- read the version number of the RMD -- */
    retval = readsock (fd, rmdversionstring, (int)ack.data);
    if (retval <= 0) return (retval);
    rmdversionstring[(int)ack.data] = '\0';
    /* -- put version specific processing here */

    return (1);
} /* sendversion */

/****************************************************************************
 * sendhandshake -- send a handshake message 
 ***************************************************************************/
int
sendhandshake (int fd)
{
    headpack_t header;
    authpack_t auth;
    ackpack_t ack;
    time_t ts;
    u_long hostid;
    int response;
    int len;

    memset(&header, 0, sizeof(header));
    memset(&auth, 0, sizeof(auth));
    /* -- fake out if this isn't going to the socket */
    if (!mysys->isconnected) {
        return 1;
    }
    if (fd != mysys->sock) {
        return 1;
    }
    /* -- get the system information */
    time(&ts);
    len = 256;
    hostid = mysys->hostid;
    encodeauth (ts, mysys->name, hostid, mysys->ip, &auth.len, auth.auth);
    strcpy (auth.configpath, &(mysys->configpath[strlen(mysys->configpath)-8]));
    header.magicvalue = MAGICHDR;
    header.cmd = CMDHANDSHAKE;
    header.ts = ts;
    header.ackwanted = 1;
    header.devid = 0;
    header.len = 0;
    header.offset = 0;
    /* -- send header and auth packets */
    if (-1 == writesock (fd, (char*)&header, sizeof(headpack_t))) return -1;
    if (-1 == writesock (fd, (char*)&auth, sizeof(authpack_t))) return -1;
    while (1 != (response = checkresponse (fd, &ack))) {
        if (0 >= response) {
            exit (EXITANDDIE);
        }
    }
    _pmd_cpon = ack.data;
    return 1;
} /* sendhandshake */

/****************************************************************************
 * sendconfiginfo -- send a configuration of a remote mirror device for
 *                   verification
 ***************************************************************************/
int
sendconfiginfo (int fd)
{
    headpack_t header;
    ackpack_t ack;
    rdevpack_t rdev;
    sddisk_t* sd;
    daddr_t locsize;
    int response;

    memset(&header, 0, sizeof(header));
    memset(&rdev, 0, sizeof(rdev));

    /* -- fake out if this isn't going to the socket */
    if (!mysys->isconnected || fd != mysys->sock) {
        return (1);
    }
    /* -- walk through devices, send believed mirror device, 
       for size of the device */
    for (sd = mysys->group->headsddisk; sd; sd = sd->n) {
        if ((locsize = disksize(sd->devname)) < 0) {
            reporterr (ERRFAC, M_LOCDK,ERRWARN,sd->devname,mysys->configpath,strerror(errno));
            return -1;
        }
        /* -- send the mirror device name to the remote system for verification */
        header.magicvalue = MAGICHDR;
        header.cmd = CMDCHKCONFIG;
        (void)time(&header.ts);
        header.ackwanted = 1;
        header.devid = sd->devid;
        header.len = 0;
        header.offset = 0;

        rdev.devid = sd->devid;
        rdev.dd_rdev = sd->dd_rdev;
        rdev.sd_rdev = sd->sd_rdev;

        (void) strcpy(rdev.path, sd->mirname);
        rdev.len = strlen(sd->mirname);
        if (-1 == (response = writesock (fd, (char*)&header, 
            sizeof(headpack_t)))) return (response);
        if (-1 == (response = writesock (fd, (char*)&rdev, 
            sizeof(rdevpack_t)))) return (response);
        while (1 != (response = checkresponse (fd, &ack))) {
            if (0 >= response) {
                exit (EXITANDDIE);
            }
        }
        /* -- check if remote device size is less than local data disk */
        if (locsize > (daddr_t)ack.data) {
            reporterr (ERRFAC, M_MIR2SMAL, ERRCRIT, mysys->configpath, 
                sd->devname, sd->mirname, locsize, (daddr_t)ack.data);
            return (-1);
        }
        /* -- move on to the next device for this group */
    }
    return 1;
} /* sendconfiginfo */

/****************************************************************************
 * sendchunk -- send a journal chunk to the RMD
 ***************************************************************************/
int
sendchunk (int fd, group_t *group)
{
    struct iovec iov[2];
    headpack_t header;
    static char *save_chunk_addr;
    int length;
    int rc;

    memset(&header, 0, sizeof(headpack_t));

    /* create a header for the current entry */
    header.magicvalue = MAGICHDR;
    header.cmd = CMDWRITE;
    header.devid = 0;
    header.ts = group->ts;

#ifdef CRASH_ON_BAD_SIZE
    if (group->size & 0x7)
        *(char *)0 = 0;
#endif

    header.ackwanted = 1;
    header.compress = mysys->tunables.compression;
    header.decodelen = group->size;

    save_chunk_addr = group->chunk;

    if (mysys->tunables.compression) {
        if ((length = group->size) > compress_buf_len) {
            compress_buf_len = length + (length >> 1) + 1;
            compress_buf = (char*)ftdrealloc((void*)compress_buf, 
                                           compress_buf_len);  
        }
        /* predictor compression */
        group->len =
            compress((u_char*)group->chunk, (u_char*)compress_buf, 
                group->size);
        group->chunk = compress_buf;
        if (group->size) {
            compr_ratio =
                (float)((float)group->len/(float)group->size);
        } else {
            compr_ratio = 1;
        }
    } else {
        group->len = group->size;
    }
    header.len = group->len;
    DPRINTF((ERRFAC,LOG_INFO,        "\n*** sendchunk: writing header, magicvalue = %08x\n",	       header.magicvalue));
    DPRINTF((ERRFAC,LOG_INFO,        "\n*** sendchunk: writing %d bytes @ addr %d to socket\n",       header.len, group->chunk));

    /* send the header and the data */
    iov[0].iov_base = (void*)&header;
    iov[0].iov_len = sizeof(headpack_t);
    iov[1].iov_base = (void*)group->chunk;
    iov[1].iov_len = header.len;

    rc = net_write_vector(fd, iov, 2, ROLEPRIMARY);
    group->chunk = save_chunk_addr;
 
    return rc;
} /* sendchunk */

/****************************************************************************
 * checkresponse -- check if an ACK or ERR is pending to be read, if so, read
 *                  and process it (0=nothing waiting, -1=ERR, >0=ACK)
 ***************************************************************************/
int
checkresponse (int fd, ackpack_t* ack) 
{
    errpack_t err;
    int retval;
    int retries;

sddisk_t *sd;

    retries = 0;
    /* -- fake out networked ACK if not reading from the network */
    if (!mysys->isconnected) {
        return 1;
    }
    if (mysys->sock != fd) {
        return 1;
    }
    memset(ack, 0, sizeof(ackpack_t));
    /* -- see if there is something waiting to be read */
    retval = readsock (fd, (char*)ack, sizeof(ackpack_t));
    if (retval <= 0) {
        return retval;
    }
    /* -- process the ACK packet */
    if (ack->magicvalue != MAGICACK) {
        reporterr (ERRFAC, M_ACKMAGIC, ERRCRIT, _pmd_configpath);
        return -1;
    }
    if (ack->acktype != ACKNOOP
    && ack->acktype != ACKNORM
    && ack->acktype != ACKERR) {
        reporterr (ERRFAC, M_ACKCMD, ERRCRIT, _pmd_configpath);
        return -1;
    }
    if (ack->acktype == ACKNOOP) {
        return 2;
    }
    if (ack->acktype == ACKERR) {
        /* -- get error packet */
        retval = readsock (fd, (char*)&err, sizeof(errpack_t));
        if (retval < 0) return retval;
        if (retval == 0) {
            retries++;
            if (retries >= 20) {
                reporterr (ERRFAC, M_ERRMIS, ERRCRIT, _pmd_configpath);
                return -1;
            }
        }
        reporterr (ERRFAC, M_RMDERR, ERRWARN, err.msg);
        if (err.errcode == ERRINFO) {
            return 0;
        }
        return -1;
    } 

    return 1;
} /* checkresponse */

/****************************************************************************
 * readsock -- read from a socket (-1=err, 0=nothing pending, 1=OK)
 ***************************************************************************/
int
readsock (int fd, char* buf, int len)
{
    fd_set rset;
    time_t now, lastts;
    int deltatime, elapsed_time;
    int connect_timeo;
    int sofar;
    int rc;

    skosh.tv_sec = 0L;
    skosh.tv_usec = 100000L;
 
    elapsed_time = 0;
    connect_timeo = 5;
 
    /* read the message */ 
    sofar = 0;
    while (len > 0) {
        FD_ZERO(&rset);
        FD_SET(fd, &rset);

        rc = select(fd+1, &rset, NULL, NULL, &skosh);

        if (rc == -1) {
            if (errno == EINTR
            || errno == EAGAIN) {
                continue;
            } else {
                return -1;
            }
        } else if (rc == 0 || !FD_ISSET(fd, &rset)) {
            /*
             *  nothing to read on socket
             *  continue unless network channel
             *  has been severed
             */
            time(&now);
            if (lastts > 0) {
                deltatime = now - lastts;
            } else {
                deltatime = 0;
            }
            elapsed_time += deltatime;
            if (elapsed_time > max_elapsed_time) {
                if (net_test_channel(connect_timeo) <= 0) {
                    reporterr (ERRFAC, M_NETTIMEO, ERRWARN,
                        mysys->name, othersys->name);
                    exit (EXITNETWORK);
                }
            }
            /* try again */
            elapsed_time = 0;
            lastts = now;
            continue;
        }
        elapsed_time = lastts = 0;

        sofar = read(fd, buf, len);
        if (sofar == -1) {
            /* -- real errors, report error and return */
            if (errno == EFAULT || errno == EIO || errno == EISDIR || 
                errno == ENOLINK || errno == ENXIO) {
                reporterr (ERRFAC, M_SOCKRERR, ERRWARN, argv0, strerror(errno));
                return (-1);
            } else {
                /* -- soft errors, sleep, then retry the read -- */
                (void) select (0, NULL, NULL, NULL, &skosh);
            }
            continue;
        } 
        if (sofar == 0) {
            goto netbroke;
        }
        len -= sofar;
        buf += sofar;
    }

    return 1;

netbroke:

    reporterr (ERRFAC, M_NETBROKE, ERRWARN, argv0, othersys->name);
    exit (EXITNETWORK);

} /* readsock */

/****************************************************************************
 * netread -- read a structure from a network socket (-1=err, 
 *            0=nothing pending, 1=OK)
 ***************************************************************************/
int
netread (int fd, char* buf, int len)
{
    int sofar;
    struct pollfd fds[1];
    u_long nfds;
    int timeout;
    int pollretval;
    int count;
    ackpack_t noopack;
    int retval;
    int onesec;

    skosh.tv_sec = 0L;
    skosh.tv_usec = 50000L;
    nfds = 1L;
    timeout = 50;
    fds[0].fd = fd;
    fds[0].events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI | POLLERR |
        POLLHUP | POLLNVAL;
    fds[0].revents = 0;
    noopack.magicvalue = MAGICACK;
    noopack.acktype = ACKNOOP;
    noopack.ts = (time_t)0;
    noopack.data = 0L;

    /* -- see if there is something waiting to be read */
    /* -- read the message */ 
    sofar = 0;
    count = 0;
    if (timeout == 0) {
        onesec = 1;
    } else {
        onesec = ((1000 / timeout) > 0) ? (1000 / timeout) : 1 ;
    }
    while (len > 0) {
        while (0 == (pollretval = poll (fds, nfds, timeout))) {
            count++;
            if (count >= 600) {
                /* -- send a gratuitous unsolicited no-op ACK to test connect */
                count = 0;
                retval = writesock(fd, (char*)&noopack, sizeof(ackpack_t));
                if (-1 == retval){
                    reporterr (ERRFAC, M_SOCKWERR, ERRWARN,
                        argv0, strerror(errno));
                }
            }
        }
        count = 0;
        if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            if (errno == 0 || errno == ENOENT) {
                reporterr (ERRFAC, M_RMDEXIT, ERRINFO, argv0);
            } else {
                reporterr (ERRFAC, M_SOCKRERR, ERRWARN, argv0, strerror(errno));
            }
            exit (EXITANDDIE);
        }
        sofar = read(fd, buf, len);
        if (sofar == -1) {
            /* -- real errors, report error and die */
            if (errno == EFAULT || errno == EIO || errno == EISDIR || 
                errno == ENOLINK || errno == ENXIO) {
                reporterr (ERRFAC, M_SOCKRERR, ERRWARN, argv0, strerror(errno));
                exit (EXITANDDIE);
            } else {
                /* -- soft errors, sleep, then retry the read -- */
                (void) select (0, NULL, NULL, NULL, &skosh);
            }
            continue;
        } 
        if (sofar == 0) {
            reporterr (ERRFAC, M_NETBROKE, ERRWARN, argv0, othersys->name);
            exit (EXITNETWORK);
        }
        len -= sofar;
        buf += sofar;
    }

    return 1;
} /* netread */

/****************************************************************************
 * checksock -- check the given file descriptor for I/O pending 
 ***************************************************************************/
int
checksock (int fd, int op, struct timeval *seltime)
{
    ackpack_t noopack;
    fd_set fds;
    int rc;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    if (op == 1) {
        /* see if descriptor can be written to */
        rc = select(fd+1, NULL, &fds, NULL, seltime);
    } else if (op == 0) {
        /* see if there is something waiting to be read */
        rc = select(fd+1, &fds, NULL, NULL, seltime);
    } else if (op == 2) {
        /* see if connection is alive */
        noopack.magicvalue = MAGICACK;
        noopack.acktype = ACKNOOP;
        noopack.ts = (time_t)0;
        noopack.data = 0L;
        rc = writesock(fd, (char*)&noopack, sizeof(ackpack_t));
        if (rc <= 0) {
            return -1;
        } else {
            return 1;
        }
    }
    if (rc <= 0 || !FD_ISSET(fd, &fds)) {
        return 0;
    }

    return 1;
} /* checksock */

/****************************************************************************
 * writesock -- write a structure to a socket (-1=err, 1=OK)
 ***************************************************************************/
int
writesock (int fd, char* buf, int len)
{
    int sofar;

    skosh.tv_sec = 0L;
    skosh.tv_usec = 500000L;
    sofar = 0;
    while (len > 0) {
        /* We have to be connected in order to do this */
        sofar = write(fd, buf, len);
        if (sofar == -1) {
            /* -- real errors, report error and return */
            if (errno == EFAULT || errno == EIO || errno == EPIPE || 
                errno == ENXIO || errno == EBADF || errno == ERANGE ||
                errno == EFBIG || errno == ENOSPC) {
                reporterr (ERRFAC, M_SOCKWERR, ERRWARN, argv0, strerror(errno));
                return (-1);
            } else {
                /* -- soft errors, sleep, then retry the write -- */
                (void) select(0, NULL, NULL, NULL, &skosh);
            }
            continue;
        }
        len -= sofar;
        buf += sofar;
    }
    return (1);
} /* writesock */

/*
 * net_test_channel -- try to connect to other end
 */
int 
net_test_channel(int connect_timeo)
{
    struct hostent *hp;
    struct sockaddr_in servaddr;
	fd_set rset, wset;
	struct timeval tval;
    char *error;
	int flags, n, nsec;
    int sock;
#if defined(_AIX)
    size_t len; 
#else /* defined(_AIX) */
    int len;
#endif /* defined(_AIX) */

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        /* can't create a socket - maybe we'll get it next time around */
        return 1;
    }
	flags = fcntl(sock, F_GETFL, 0);
	fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    memset ((char*)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(othersys->secondaryport);
  
    /*
     * if the other system host name is empty or gethostbyname fails
     * then try gethostbyaddr
     */
    hp = 0;
    if (othersys->ip != 0L) {
        hp = gethostbyaddr((char*)&othersys->ip, sizeof(u_long), AF_INET);
    } else if (strlen(othersys->name) > 0) {
        hp = gethostbyname(othersys->name);
    }
    if (hp == 0) {
        close(sock);
        return 1;
    }
    memcpy((caddr_t)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);

	error = "";
    if ((n = connect(sock, (struct sockaddr*)&servaddr,
        sizeof(servaddr))) < 0) {
		if (errno != EINPROGRESS) {
            close(sock);
			return -1;
        }
    }
	if (n == 0) {
		goto done;	/* connect completed immediately */
    }
    nsec = connect_timeo;

	FD_ZERO(&rset);
	FD_SET(sock, &rset);
	wset = rset;
	tval.tv_sec = nsec;
	tval.tv_usec = 0;

	if ((n = select(sock+1, &rset, &wset, NULL, nsec ? &tval : NULL)) == 0) {
		close(sock);		/* timeout */
		errno = ETIMEDOUT;
		return 0;
	}
	if (FD_ISSET(sock, &rset) || FD_ISSET(sock, &wset)) {
		len = sizeof(error);
		if (getsockopt(sock, SOL_SOCKET, SO_ERROR, error, &len) < 0)
			return(-1);			/* Solaris pending error */
	} else {
        return -1;
    }

done:

	if (strlen(error)) {
		close(sock);		/* just in case */
		errno = atoi(error);
		return(-1);
	}
    close(sock);
 
    return 1;
} /* net_test_channel */

/****************************************************************************
 * process_acks -- PMD - process ACKs and do the right thing 
 ***************************************************************************/
int 
process_acks(void)
{
    struct timeval seltime[1];
    headpack_t header[1];
    ackpack_t ack[1];
    rsync_t rrsync[1];
    rsync_t *rsync;
    rsync_t *ackrsync;
    group_t *group;
    sddisk_t* sd;
    static int buflen = 0;
    static char *buf = NULL;
    static int zerolen = 0;
    static char *zerobuf = NULL;
    static int firsttime = 1;
    static time_t ts, lastts;
    int deltatime;
    int datalen;
    int length;
    int ret;
    int lgnum;
    int cnt;
    int connect_timeo;
    int ackcount;
    int rc;
    int i;

    group = mysys->group;
    lgnum = cfgpathtonum(mysys->configpath);

    seltime->tv_sec = 0;
    seltime->tv_usec = 0;
   
    (void)time(&ts);
    ackcount = 0;

    if (firsttime) {
        lastts = ts;
        elapsed_time = 0;
        firsttime = 0;
    }
    deltatime = ts - lastts;
    connect_timeo = 5;

    while (1) {
        ret = checksock(mysys->sock, 0, seltime);
        if (ret == 0) {
            elapsed_time += deltatime;
            if (elapsed_time > max_elapsed_time) {
                if (net_test_channel(connect_timeo) == 0) {
                    reporterr (ERRFAC, M_NETTIMEO, ERRWARN,
                        mysys->name, othersys->name);
                    exit(EXITNETWORK);
                } else {
                    /* channel still there - rmd must be busy! */
                    elapsed_time = 0;
                }
            }
            lastts = ts;
            return 0;
        }
        elapsed_time = 0;
        lastts = ts;

        if (ackcount++ >= 2) {
            return ackcount;
        }
        rc = checkresponse(mysys->sock, ack);
        DPRINTF((ERRFAC,LOG_INFO,               "\n*** checkresponse rc, ack->acktype, ack->data = %d, %d, %d\n",               rc, ack->acktype, ack->data));
        if (rc == -1) {
            exit(EXITANDDIE);
        }
        if (ack->data == ACKNOOP) {
            DPRINTF((ERRFAC,LOG_INFO,"\n*** ACKNOOP received\n"));
            return ackcount;
        }
        if (ack->acktype == ACKNORM) {
            if (ack->data == ACKBFDDELTA
            || ack->data == ACKRFD
            || ack->data == ACKRFDF) {
                if ((sd = get_lg_dev(group, ack->devid)) == NULL) {
                    reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, ack->devid);
                    exit(EXITANDDIE);
                    return ackcount;
                }
                rsync = &sd->rsync;
                ackrsync = &sd->ackrsync;
            }
        } else {
            return ackcount;
        }

        switch(ack->data) {
        case ACKRFDCHKSUM:
            DPRINTF((ERRFAC,LOG_INFO,"\n*** ACKRFDCHKSUM\n"));
            for (sd = group->headsddisk; sd; sd = sd->n) {
                sd->ackrsync.cs.cnt = 0;
                sd->ackrsync.cs.num = 0;
                sd->ackrsync.cs.seglen = 0;
            }
            cnt = ack->ackoff;

            for (i = 0; i < cnt; i++) {
                memset(rrsync, 0, sizeof(rsync_t));
                rc = readsock(mysys->sock, (char*)rrsync, sizeof(rsync_t));
                if (rc != 1) {
                    exit(exitfail);
                }  
                if ((sd = get_lg_dev(group, rrsync->devid)) == NULL) {
                    reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, rrsync->devid);
                    exit(EXITANDDIE);
                    return;
                }
                rsync = &sd->rsync;
                ackrsync = &sd->ackrsync;

                ackrsync->cs.cnt = rrsync->cs.cnt;
                ackrsync->cs.num = rrsync->cs.num;
                ackrsync->cs.segoff = rrsync->cs.segoff;
                ackrsync->cs.seglen = rrsync->cs.seglen;

                length = ackrsync->cs.cnt*ackrsync->cs.num*DIGESTSIZE;
                if (length > ackrsync->cs.digestlen) {
                    ackrsync->cs.digest = ftdrealloc(ackrsync->cs.digest, length);
                    ackrsync->cs.digestlen = length;
                } 
                /* read remote checksum digest into local digest buffer */
                rc = readsock(mysys->sock, ackrsync->cs.digest, length);
                if (rc != 1) {
                    exit(exitfail);
                }
                /* bump ackoff for device */
                rsync->ackoff = rrsync->offset+rrsync->length;
/*
*/
                sd->stat.rfshoffset = rsync->ackoff;
            }  
            /* compare local/remote checksum digests */
            if ((rc = chksumdiff()) == -1) {
                reporterr (ERRFAC, M_CHKSUMDIFF, ERRCRIT, argv0);
                exit (EXITANDDIE);
            }
            break;
        case ACKBFDDELTA:
            /* read the header packet from network */
            rc = readsock(mysys->sock, (char*)header, sizeof(headpack_t));
            if (1 != rc) {
                exit (exitfail);
            }  
            DPRINTF((ERRFAC,LOG_INFO,                 "\n*** ACKBFDDELTA: header->len, header->data = %d, %d\n",                header->len, header->data));

            if (header->len) {
                if ((datalen = header->len) > buflen) {
                    buflen = datalen;
                    buf = (char*)ftdrealloc(buf, buflen);
                }
                if (header->data == BLOCKALLZERO) {
                    if (header->len > zerolen) {
                        zerolen = header->len;
                        zerobuf = (char*)ftdrealloc(zerobuf, zerolen);
                        memset(zerobuf, 0, zerolen);
                    }
                    header->data = (u_long)zerobuf;
                } else {
                    if (1 != (rc = readsock(mysys->sock, buf, header->len))) {
                        exit (exitfail);
                    }  
                    if (header->compress) {
                        /* decompress the data */
                        if ((length = header->decodelen) > clen) {
                            clen = length;
                            cbuf = (u_char*)ftdrealloc((char*)cbuf, clen);  
                        }
                        header->len =
                          decompress((u_char*)buf, (u_char*)cbuf, header->len);
                        header->data = (u_long)cbuf;
                    } else {
                        header->data = (u_long)buf;
                    }
                }
                memset(rrsync, 0, sizeof(rsync_t));
                rrsync->devid = header->devid;
                rrsync->offset = header->offset;
                rrsync->datalen = header->len;
                rrsync->length = rrsync->datalen >> DEV_BSHIFT;
                rrsync->data = (char*)header->data;

                rrsync->size = rsync->size;

                if ((rc = rsyncwrite_bfd(rrsync)) < 0) {
                    exit (EXITANDDIE);
                }
            }
            rsync->delta += rrsync->length;
            sd->stat.rfshdelta = rsync->delta;
            sd->stat.rfshoffset = rsync->ackoff;
            DPRINTF((ERRFAC,LOG_INFO,                   "\n*** ACKBFDDELTA: datalen, delta, size = %d, %d, %d\n",                 datalen, rsync->delta, rsync->size));
            break;
        case ACKRFDF:
            /* full-refresh write ACK */
            rsync->ackoff += ack->ackoff;
            sd->stat.rfshoffset = rsync->ackoff;
            break;
        case ACKRFD:
            /* chksum/HRDB write ACK */
            rsync->ackoff += ack->ackoff;
            sd->stat.rfshoffset = rsync->ackoff;
            break;
        case ACKBFD:
            cnt = ack->ackoff;
            /* read ACK offsets */
            for (i = 0; i < cnt; i++) {
                rc = readsock(mysys->sock, (char*)rrsync, sizeof(rsync_t));
                if (rc != 1) {
                    DPRINTF((ERRFAC,LOG_INFO,"\n*** readsock returned: %d\n", rc));
                    exit(exitfail);
                }  
                if ((sd = get_lg_dev(group, rrsync->devid)) == NULL) {
                    reporterr(ERRFAC, M_DEVID, ERRCRIT, argv0, rrsync->devid);
                    exit(EXITANDDIE);
                    return;
                }
                /* bump ackoff for device */
                rsync = &sd->rsync;
                rsync->ackoff = rrsync->offset+rrsync->length;
            }  
            break;
        case ACKCHUNK:
            DPRINTF((ERRFAC,LOG_INFO,"\n*** ACKCHUNK: ack->mirco = %d\n",                ack->mirco));
            if (ack->mirco) {
                g_rfddone = 1;
            }
            if (ack->cpon) {
                _pmd_cpon = ack->cpon;
                _pmd_cpstart = 0;
            }
            if (1 != (migrate(ack->ackoff))) {
                reporterr (ERRFAC, M_MIGRATE, ERRCRIT, group->devname, 
                    strerror(errno));
                closeconnection();
                exit (EXITRESTART);
            }
            break;
        case ACKHUP:
            DPRINTF((ERRFAC,LOG_INFO,"\n*** ACKHUP\n"));
            g_ackhup = 1;
            break;
        case ACKCPSTART:
            DPRINTF((ERRFAC,LOG_INFO,"\n*** ACKCPSTART\n"));
            _pmd_cpstart = 2; /* cp on pending state */
            break;
        case ACKCPSTOP:
            DPRINTF((ERRFAC,LOG_INFO,"\n*** ACKCPSTOP\n"));
            _pmd_cpstop = 2; /* cp off pending state */
            break;
        case ACKCPON:
            DPRINTF((ERRFAC,LOG_INFO,"\n*** ACKCPON\n"));
            _pmd_cpon = 1;
            _pmd_cpstart = 0; /* no longer pending */
            reporterr(ERRFAC, M_CPON, ERRINFO, argv0);
            break;
        case ACKCPOFF:
            DPRINTF((ERRFAC,LOG_INFO,"\n*** ACKCPOFF\n"));
            _pmd_cpon = 0;
            _pmd_cpstop = 0; /* not longer pending */
            reporterr(ERRFAC, M_CPOFF, ERRINFO, argv0);
            break;
        case ACKCPONERR:
            DPRINTF((ERRFAC,LOG_INFO,"\n*** ACKCPONERR\n"));
            _pmd_cpon = 0;
            _pmd_cpstart = 0; /* not longer pending */
            reporterr(ERRFAC, M_CPONERR, ERRWARN, argv0);
            break;
        case ACKCPOFFERR:
            DPRINTF((ERRFAC,LOG_INFO,"\n*** ACKCPOFFERR\n"));
            _pmd_cpstop = 0; /* not longer pending */
            reporterr(ERRFAC, M_CPOFFERR, ERRWARN, argv0);
            break;
        case ACKKILLPMD:
            DPRINTF((ERRFAC,LOG_INFO,"\n*** ACKKILLPMD\n"));
            reporterr(ERRFAC, M_PMDEXIT, ERRINFO, argv0);
            exit (EXITANDDIE);
            break;
        default:
            break;
        }
    }

} /* process_acks */

/****************************************************************************
 * eval_netspeed -- evaluate network thruput against 'netmaxkbps' 
 ***************************************************************************/
int
eval_netspeed(void)
{
    group_t *group;
    sddisk_t *sd;
    struct timeval seltime[1];
    static float prev_kbps = 0.0;
    static float kbps = 0.0;
    static time_t currentts = 0;
    static time_t lastts = 0;
    static unsigned int netsleep = 0;
    static int firsttime = 1;
    int netmaxkbps;
    int deltasec;
    u_longlong_t a_tdatacnt;
    u_longlong_t a_datacnt;
    u_longlong_t e_tdatacnt;
    u_longlong_t e_datacnt;

    netmaxkbps = mysys->tunables.netmaxkbps;

    if (netmaxkbps < 0) {
        return 0;
    }
    if (firsttime) {
        time(&currentts);
        lastts = currentts;
        firsttime = 0;
    } else {
        lastts = currentts;
        time(&currentts);
    }
    deltasec = currentts - lastts;

    if (deltasec <= 0) {
        return 0;
    }
    group = mysys->group;
    prev_kbps = kbps;

    a_tdatacnt = 0;
    a_datacnt = 0;
    e_tdatacnt = 0;
    e_datacnt = 0;

    for (sd = group->headsddisk; sd; sd = sd->n) {
        a_tdatacnt += sd->stat.a_tdatacnt;
        a_datacnt += sd->stat.a_datacnt;
        e_tdatacnt += sd->stat.e_tdatacnt;
        e_datacnt += sd->stat.e_datacnt;
    }
    /* calculate kbps */
    DPRINTF((ERRFAC,LOG_INFO,     "\n*** eval_netspeed: e_datacnt, e_tdatacnt, deltasec = %llu, %llu, %d\n",	        a_datacnt, a_tdatacnt, deltasec));
    kbps = ((a_tdatacnt*1.0)/(deltasec*1.0))/1024.0;
    DPRINTF((ERRFAC,LOG_INFO,"\n*** eval_netspeed: kbps, netmaxkbps = %6.2f, %6.2f\n",         (float) kbps, (float) netmaxkbps));
    if (kbps > netmaxkbps) {
        netsleep += 500000;
    } else if (kbps < netmaxkbps) {
        if (netsleep > 500000) {
            netsleep -= 500000;
        } else  {
            netsleep = 0;
        }
    }
    if (netsleep < 1000000) {
        seltime->tv_sec = 0;
        seltime->tv_usec = netsleep;
    } else {
        seltime->tv_sec = netsleep/1000000;
        seltime->tv_usec = netsleep%1000000;
    }
    DPRINTF((ERRFAC,LOG_INFO,         "\n### kbps = %6.2f, sleeping for %d\n", kbps, netsleep));

    select(0, NULL, NULL, NULL, seltime);

    return 0;
} /* eval_netspeed */

/*
 * net_write_vector -- write a message vector to the network
 */
int
net_write_vector(int fd, struct iovec *iov, int iovcnt, int role)
{
    struct timeval seltime[1];
    int rc, cnt, len, len1, tlen, i;
    int deltatime;
    time_t now, starttime;
    int iov_offset;
    int writecnt, save_iov_len;
    int write_timeo;
    struct iovec *liov;
    caddr_t iov_base, save_iov_base;

    len = 0;
    /* get total write length */
    for (i = 0; i < iovcnt; i++) {
        len += iov[i].iov_len;
    } 
    tlen = len;
    time(&starttime);

    seltime->tv_sec = 0;
    seltime->tv_usec = 10000;

    writecnt = 0;
    /* if we can't write for 30 minutes - give up ? */ 
    write_timeo = 1800; /* 30 minutes */

    while (1) {
        if (!checksock(fd, 1, seltime)) {
            /*
             * net not writable
             * see if anything to read 
             */
            net_writable = 0;
        } else {
            if (writecnt) {
                /*
                 * partial write complete
                 * adjust vector and retry
                 */
                len1 = 0;
                for (i = 0; i < iovcnt; i++) {
                    if ((len1 += iov[i].iov_len) > writecnt) {
                        break;
                    }
                } 
                iov_base = iov[i].iov_base; 
                iov_offset = (writecnt - (len1 - iov[i].iov_len));
                
                /* save base, len */
                save_iov_base = iov[i].iov_base;
                save_iov_len = iov[i].iov_len;

                iov[i].iov_base = iov_base+iov_offset; 
                iov[i].iov_len -= iov_offset; 

                liov = &iov[i];
                cnt = iovcnt - i;
            } else {
                i = 0;
                liov = iov;
                cnt = iovcnt;
                save_iov_base = iov[i].iov_base;
                save_iov_len = iov[i].iov_len;
            }
               
            rc = writev(fd, liov, cnt);

            if (rc == -1) {
                /* real errors, report error and return */
                if (errno == EFAULT
                || errno == EIO
                || errno == EPIPE
                || errno == ENXIO
                || errno == EBADF
                || errno == ERANGE
                || errno == EFBIG
                || errno == ENOSPC) {
                    reporterr (ERRFAC, M_SOCKWERR, ERRWARN,
                        argv0, strerror(errno));
                    return -1;
                } else {
                    /* soft errors, read, then retry the write */
                    net_writable = 0;
                }
            } else if (rc < len) { 
                writecnt += rc;
                len -= rc;
            } else {
                /* write completed */
                writecnt = tlen;
                net_writable = 1;
                return 0;
            }

            iov[i].iov_base = save_iov_base;
            iov[i].iov_len = save_iov_len;
            if (writecnt == tlen) {
                break;
            }
        }
        /* read net to free up remote in case it's blocked */
        time(&now);
        deltatime = now-starttime;
        if (deltatime > 1) {
            if (role == ROLEPRIMARY) {
                process_acks();
            }
        }
        if (deltatime > write_timeo) {
            /* report error and die */
            reporterr (ERRFAC, M_SOCKWERR, ERRWARN,
                argv0, "network write timed out");
            exit(EXITANDDIE);
        }
    }

} /* net_write_vector */


