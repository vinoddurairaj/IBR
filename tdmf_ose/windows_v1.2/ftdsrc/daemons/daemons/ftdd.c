/***************************************************************************
 * ftdd.c - FullTime Data Master Daemon
 *
 * (c) Copyright 1998 FullTime Software, Inc. All Rights Reserved
 *
 ***************************************************************************
 */
#ifdef NEED_BIGINTS
#include "bigints.h"
#endif

#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/param.h>
#define SERVER_PORT 575
#if defined(SOLARIS)
#include <sys/procfs.h>
#elif defined(HPUX)
#include <sys/pstat.h>
#endif

#include <ctype.h>
#include <stdio.h>

#ifdef NEED_SYS_MNTTAB
#include "ftd_mnttab.h"
#else
#include <sys/mnttab.h>
#endif
#include <fcntl.h>

#include "errors.h"
#include "config.h"
#include "license.h"
#include "licprod.h"
#include "pathnames.h"
#include "common.h"
#include "process.h"
#include "ipc.h"
#include "network.h"
#include "devcheck.h"
#include "cfg_intr.h"

#ifdef DEBUG
FILE* dbgfd;
#endif

static pid_t pid;
static pid_throtd = -999;

char *argv0;

static char configpaths[MAXLG][32];
static struct timeval skosh;
static time_t currentts;
static char pmdversionstring[256];
static char rmdversionstring[256];

static listenport = 575;

char** environ = (char**)NULL;
int envcnt = 0;
char** childargv = (char**)NULL;
int childargc = 0;

char fmt[256];
char msg[256];
char *_pmd_configpath;
char *_rmd_configpath;

int _pmd_reconwait = 5;

extern char *paths;
sddisk_t *sd;

int restartcnts[MAXLG];

static int sigpipe[2]; 

extern ftdpid_t p_cp[];
extern ftdpid_t r_cp[];

extern int pidcnt;
extern int rmd_pidcnt;
extern int *rcnt; 
extern int lgcnt;

extern char **pmdenviron;
extern int pmdargc;
extern int pmdenvcnt;

char **pmdargv;

extern int ftdsrvsock;
extern int ftdsock;
extern int rmdsrvsock;
extern int rmdsock;

/****************************************************************************
 * reaper -- handle exiting children 
 ***************************************************************************/
int
reaper (void)
{
    time_t nowtime;
    time_t laststart[MAXLG];
    char pname[32];
    int status;
    int exitstatus;
    int exitsignal;
    int exitduetostatus;
    int exitduetosignal;
    int restartcount;
    int cfgidx;
    int lgnum;
    int tries;
    int maxtries;
    int j;

    tries = 0;
    maxtries = 3;

    restartcount = 0;
    while(1) {
        pid = waitpid(0, &status, WNOHANG);
        if (pid == -1) {
            switch(errno) {
            case EINTR:
                continue;
            case ECHILD:
                /* no more children */
                return 0;
            default:
                reporterr (ERRFAC, M_ABEXIT, ERRCRIT);
                pidcnt--;
                continue;
            }
        }
        if (pid == 0) {
            if (tries++ >= maxtries) {
                break;
            }
            usleep(100000);
            continue;
        }
        /* handle throtd relaunch, if necessary */
        if (pid == pid_throtd) {
            reporterr (ERRFAC, M_RETHROTD, ERRWARN);
            (void) throtdstart();  
            pidcnt--;
            continue;
        }
        /* reap mirror daemons */
        lgnum = -1;
        for (j = 0; j < MAXLG; j++) {
            if (p_cp[j].pid == pid) {
                lgnum = j;
                p_cp[j].pid = -1;
                if (p_cp[j].fd[0][STDIN] != -1) {
                    close(p_cp[j].fd[0][STDIN]);
                    p_cp[j].fd[0][STDIN] = -1;
                }
                if (p_cp[j].fd[0][STDOUT] != -1) {
                    close(p_cp[j].fd[0][STDOUT]);
                    p_cp[j].fd[0][STDOUT] = -1;
                }
                if (p_cp[j].fd[1][STDIN] != -1) {
                    close(p_cp[j].fd[1][STDIN]);
                    p_cp[j].fd[1][STDIN] = -1;
                }
                if (p_cp[j].fd[1][STDOUT] != -1) {
                    close(p_cp[j].fd[1][STDOUT]);
                    p_cp[j].fd[1][STDOUT] = -1;
                }
                error_tracef( TRACEINF, "reaper():PMD: %d, new pidcnt = %d", lgnum, pidcnt );
                break;
            } else if (r_cp[j].pid == pid) {
                r_cp[j].pid = -1;
                if (r_cp[j].fd[0][STDIN] != -1) {
                    close(r_cp[j].fd[0][STDIN]);
                    r_cp[j].fd[0][STDIN] = -1;
                }
                if (r_cp[j].fd[0][STDOUT] != -1) {
                    close(r_cp[j].fd[0][STDOUT]);
                    r_cp[j].fd[0][STDOUT] = -1;
                }
                if (r_cp[j].fd[1][STDIN] != -1) {
                    close(r_cp[j].fd[1][STDIN]);
                    r_cp[j].fd[1][STDIN] = -1;
                }
                if (r_cp[j].fd[1][STDOUT] != -1) {
                    close(r_cp[j].fd[1][STDOUT]);
                    r_cp[j].fd[1][STDOUT] = -1;
                }
                rmd_pidcnt--;
                error_tracef( TRACEINF, "reaper():RMD: %d, new rmd_pidcnt = %d", j, rmd_pidcnt );
            }
        }
        pidcnt--;
        if (lgnum == -1) {
            continue;
        }
        /* relaunch PMD if it exited with reasonable status */
        exitstatus = 0;
        exitsignal = 0;
        exitduetostatus = 0;
        exitduetosignal = 0;
        exitduetostatus = WIFEXITED(status);
        exitduetosignal = WIFSIGNALED(status);
        if (exitduetostatus) exitstatus = WEXITSTATUS(status);
        if (exitduetosignal) exitsignal = WTERMSIG(status);
#ifdef TDMF_TRACE
            fprintf(stderr, "FTDD child exited, exit status=%d signal=%d\n",
              exitstatus, exitsignal);
#endif

        if (exitsignal == 0) {
            if (exitstatus == EXITRESTART || exitstatus == EXITNETWORK) {
                if (pipe(p_cp[lgnum].fd[0]) < 0) {
                    reporterr(ERRFAC, M_PIPE, ERRWARN, strerror(errno));
                }
                if (pipe(p_cp[lgnum].fd[1]) < 0) {
                    reporterr(ERRFAC, M_PIPE, ERRWARN, strerror(errno));
                }
                pid = fork();
                switch (pid) {
                case -1:
                    reporterr (ERRFAC, M_FORK, ERRCRIT, strerror(errno));
                    exit(1);
                case 0: /* child */
                    /* close unused pipe descriptors */
                    close(p_cp[lgnum].fd[0][STDIN]);
                    p_cp[lgnum].fd[0][STDIN] = -1;
                    close(p_cp[lgnum].fd[1][STDOUT]);
                    p_cp[lgnum].fd[1][STDOUT] = -1;
                    (void) time (&nowtime);
                    if (laststart[lgnum] == 0
                    || (nowtime - laststart[lgnum]) < 10) {
                        rcnt[lgnum]++;
                    } else {
                        rcnt[lgnum] = 0;
                    }
                    laststart[lgnum] = nowtime;
                    if (exitstatus != EXITNETWORK && rcnt[lgnum] >= 5) {
                        reporterr (ERRFAC, M_2MANY, ERRCRIT, argv0);
                        exit (EXITANDDIE);
                    }
                    /* default to NORMAL-transfer state for restart */
                    pmdenviron[pmdenvcnt] = (char*)ftdmalloc(16*sizeof(char));
                    sprintf(pmdenviron[pmdenvcnt++], "_PMD_STATE=%d", FTDPMD);
                    pmdenviron[pmdenvcnt] = (char*)ftdmalloc(16*sizeof(char));
                    sprintf(pmdenviron[pmdenvcnt++], "_PMD_RECONWAIT=%d",_pmd_reconwait);
                    pmdenviron[pmdenvcnt] = (char*)ftdmalloc(17*sizeof(char));
                    sprintf(pmdenviron[pmdenvcnt++], "_PMD_RETRYIDX=%d", lgnum);
                    pmdenviron[pmdenvcnt] = (char*)ftdmalloc(16*sizeof(char));
                    sprintf(pmdenviron[pmdenvcnt++], "_PMD_RPIPEFD=%d",
                        p_cp[lgnum].fd[1][STDIN]);
                    pmdenviron[pmdenvcnt] = (char*)ftdmalloc(16*sizeof(char));
                    sprintf(pmdenviron[pmdenvcnt++], "_PMD_WPIPEFD=%d",
                        p_cp[lgnum].fd[0][STDOUT]);
                    pmdenviron[pmdenvcnt] = (char*)NULL;
                    execpmd(lgnum, pmdargv);
                default: /* parent */
                    /* close unused pipe descriptors */
                    close(p_cp[lgnum].fd[0][STDOUT]);
                    p_cp[lgnum].fd[0][STDOUT] = -1;
                    close(p_cp[lgnum].fd[1][STDIN]);
                    p_cp[lgnum].fd[1][STDIN] = -1;
                    restartcount++;
                    p_cp[lgnum].pid = pid;
                    wait_child(&cfgidx, p_cp[lgnum].fd[0][STDIN]);
                    pidcnt++;
                    break;
                } /* switch */
            } else {
                /* non-restart exitstatus */
                clearmode(lgnum);
            } /* exitstatus == */
        } else {
            /* non-zero exitsignal */
            if (exitsignal != SIGTERM) {
                sprintf(pname, "PMD_%03d",lgnum);
                reporterr (ERRFAC, M_SIGNAL, ERRCRIT, pname, exitsignal);
                clearmode(lgnum);
            }
        } /* exitsignal == 0 */
    } /* while */

    return 0;
} /* reaper */

/****************************************************************************
 * dispatch_signal -- invoke actual signal action here 
 ***************************************************************************/
static void
dispatch_signal (int sig, int cnt)
{
    int lgnum;

    switch(sig) {
    case SIGHUP:
    case FTDQHUP:
        lgcnt = getconfigs(configpaths, 1, 1);
        pmdhup(sig, pmdargv);
        break;
    case FTDQBFD:
    case FTDQRFD:
    case FTDQRFDF:
    case FTDQRFDC:
        pmdrsync(sig, pmdargv);
        break;
    case FTDQAPPLY:
        /* get secondary config paths */
        lgcnt = getconfigs(configpaths, 2, 0);
        paths = (char*)configpaths;
        rmdapply(sig, pmdargv);
        break;
    case FTDQCP:
        checkpoint(cnt, pmdargv);
        break;
    default:
        break;
    }
    reaper();

    return;
} /* dispatch_signal */

/****************************************************************************
 * doversioncheck -- (RMD) negotiate protocol version with PMD
 ***************************************************************************/
int
doversioncheck (int fd, headpack_t *header, versionpack_t* version)
{
    struct stat statbuf[1];
    char configpath[MAXPATHLEN];
    int i;
    time_t tsdiff;
  
    /* -- figure out time differentials between PMD and RMD -- */
    (void) time(&currentts);
    tsdiff = currentts - version->pmdts;
    sys[0].tsbias = tsdiff;
    sys[1].tsbias = tsdiff;
  
    strcpy (pmdversionstring, "4.0.0");
    strcpy (rmdversionstring, "4.0.0");
    /* -- figure out RMD the version number put into the works by the Makefile */
#ifdef VERSION
    strcpy (rmdversionstring, VERSION);
    i = 0;
    /* -- eliminate beta, intermediate build information from version */
    while (pmdversionstring[i]) {
        if ((!(isdigit(pmdversionstring[i]))) && pmdversionstring[i] != '.') {
            pmdversionstring[i] = '\0';
            break;
        }
        i++;
    }
#endif
    /* -- capture the pmd's version number, do version mismatch checking here */
    strcpy (pmdversionstring, version->version);
 
    /* ===== protocol version mismatch checking goes here ===== */

    /* -- capture the name of the config file for this RMD instance, build
       a process name from it */
    memcpy(version->configpath, RMD_CFG_PREFIX, strlen(RMD_CFG_PREFIX)); 
    strcpy(&(version->configpath[5]), PATH_CFG_SUFFIX);
    sprintf(configpath, "%s/%s", PATH_CONFIG, version->configpath);
    _rmd_configpath = (char*)ftdmalloc(1+strlen(version->configpath));

    i = strlen(version->configpath) - 1;
    while (1) {
        if (i == 0 || version->configpath[i] == '.') {
            version->configpath[i] = '\0';
            break;
        }
        version->configpath[i] = '\0';
        i--;
    }
    strcpy(_rmd_configpath, version->configpath);
    if (stat(configpath, statbuf) == -1) {
        geterrmsg (ERRFAC, M_CFGFILE, fmt);
        sprintf (msg, fmt, argv0, configpath, strerror(errno));
        logerrmsg (ERRFAC, ERRWARN, M_CFGFILE, msg); 
        (void) senderr (fd, header, 0L, ERRWARN, M_CFGFILE, msg);
        return -1;
    }
    return (0);
} /* doversioncheck */

/****************************************************************************
 * readfirst_fourbytes --
 *  read first four bytes from connect to determine whether or not this is
 *  a pmd request
 ***************************************************************************/
static int
readfirst_fourbytes (int fd, unsigned char *cmd)
{
    unsigned char *savecmd;
    int response;
    int len;
    int rc;

    savecmd = cmd;
    len = 4;

    /*
     * read first 4 characters to see if this is a valid request 
     * this is a blocking read
     */
    while (len > 0) {
        rc = read(fd, (char*)cmd, sizeof(char)*4);
        if (rc == -1) {
            /* error */
            if (errno == EINTR
            || errno == EAGAIN) {
                continue;
            } else {
                return -1;
            }
        } else if (rc == 0) {
            /* connection closed */
            return -1;
        } else {
            len -= rc;
            cmd += rc;
        }
    }
    cmd = savecmd;
    if (0 == strncmp((char*)cmd, "ftd ", 4)) {
        return 0;
    }

    return 1;
} /* readfirst_fourbytes */

/****************************************************************************
 * readfirstitem --  read first item from the network 
 ***************************************************************************/
static int
readfirstitem (int fd, int pmdrequest, unsigned char *cmd)
{
    headpack_t *header;
    versionpack_t version;
    ackpack_t ack;
    char tbuf[256];
    char* errbuf;
    char sizestring[256];
    int numerrmsgs;
    int errbufsize;
    int erroffset;
    int response;
    int sock;
    int isconnected;
    int lgnum;
    int i;
 
    sock = fd;
    if (!pmdrequest) {
        i = 3;
        while (1) {
            i++;
            if (-1 == (response = readsock(fd, (char*)&cmd[i], sizeof(char)))) {
                return (1);
            }
            if (cmd[i] == '\n' || cmd[i] == '\0' || i == 255) {
                cmd[i] = '\0';
                break;
            }
        }
        if (0 == strncmp ((char *) cmd, " get error messages", 22)) {
            i = 23;
            while (isspace(cmd[i])) i++;
            sscanf ((char*)cmd, "%d", &erroffset);
            getlogmsgs (&errbuf, &errbufsize, &numerrmsgs, &erroffset);
            if (errbufsize > 0) {  
                sprintf (sizestring, "%d %d\n", numerrmsgs, erroffset);
                write (sock, (void*)sizestring, strlen(sizestring));
                write (sock, (void*)"{", 1);
                write (sock, (void*)errbuf, strlen(errbuf));
                write (sock, (void*)"}\n", 2);
                free (errbuf);
            } else {
                sprintf (sizestring, "0 %d\n{}\n", erroffset);
                write (sock, (void*) sizestring, strlen(sizestring));
            }
            return (1);
        } else if (-999 == process_proc_request (sock, (char*) cmd)) {
            /* -- see if this is a process info request, otherwise, a device info
               request (it will return a "0\n" message on error) */
     
            (void) process_dev_info_request (sock, (char*) cmd);
        }
        /* -- ftd command serviced, return */
        return (1);
    }
    /* end if ftd command */
 
    /* Must be a message for the RMD */
    header = (headpack_t*)cmd;
    /* Make sure this is a valid rmd message */
    if (header->magicvalue != MAGICHDR) {
        if (mysys != NULL) 
            reporterr (ERRFAC, M_HDRMAGIC, ERRWARN);
        return (1);
    }

    switch (header->cmd) {
    case CMDVERSION:
        if (-1 == (response = readsock(fd, (char*)&version, 
            sizeof(versionpack_t)))) {
            return (1);
        }
        if (0 > (response = doversioncheck (fd, header, &version))) {
            return 1;
        }
        sock = fd;
        isconnected = 1;
        break;
    case CMDNOOP:
        return (1);
    case CMDEXIT:
        exit (EXITNORMAL); 
    default:
        geterrmsg (ERRFAC, M_BADCMD, fmt);
        sprintf (msg, fmt, errno, header->devid, header->cmd);
        logerrmsg (ERRFAC, ERRWARN, M_BADCMD, msg); 
        (void) senderr (fd, header, 0L, ERRWARN, M_BADCMD, msg);
        return (-1);
    }

    /* -- return the RMD protocol version number for the 1st packet exchange */
    if (header->cmd == CMDVERSION) {
        ack.data = strlen (rmdversionstring);
    } else {
        ack.data = 0L;
    }
    /* -- see if ack is requested for this packet, if so, send it */
    if (header->ackwanted) {
        response = sendack (fd, header, &ack);
        if (response != 1) return (response);
        if (header->cmd == CMDVERSION) {
            if (!isconnected) return (1);
            if (fd != sock) return (1);
            if (-1 == writesock(fd, rmdversionstring, strlen(rmdversionstring))) 
                return (-1);
        }
    }
    if (header->cmd == CMDVERSION) {
        /* -- set up for exec'ing the RMD by finishing setting private
           environment variables, private argv list, and then exec'ing */
    
        environ = (char**)ftdmalloc(7*sizeof(char*));
        envcnt = 0;
 
        if (_rmd_configpath == (char*)NULL || strlen(_rmd_configpath) == 0) {
            sprintf (tbuf, "_RMD_CONFIGPATH=in.rmd");
        } else {
            sprintf (tbuf, "_RMD_CONFIGPATH=%s", _rmd_configpath);
        }
        lgnum = cfgpathtonum(_rmd_configpath);
        environ[envcnt] = (char*)ftdmalloc(sizeof(char)*(strlen(tbuf)));
        strcpy(environ[envcnt++], tbuf);
        environ[envcnt] = (char*)ftdmalloc(sizeof(char)*23);
        sprintf (environ[envcnt++], "_RMD_SOCKFD=%d", fd);
        environ[envcnt] = (char*)ftdmalloc(16*sizeof(char));
        sprintf(environ[envcnt++], "_RMD_RPIPEFD=%d",
            r_cp[lgnum].fd[1][STDIN]);
        environ[envcnt] = (char*)ftdmalloc(16*sizeof(char));
        sprintf(environ[envcnt++], "_RMD_WPIPEFD=%d",
            r_cp[lgnum].fd[0][STDOUT]);
        environ[envcnt] = (char*)NULL;

        childargv = (char**)ftdmalloc(sizeof(char*)*2);
        childargv[0] = (char*)ftdmalloc(4+strlen(_rmd_configpath));
        sprintf (childargv[0], "RMD_%s", &(_rmd_configpath[1]));

        childargv[1] = (char*)NULL;
        childargc = 1;

        (void)execve(RMD_PATH, childargv, environ);

        reporterr(ERRFAC, M_EXEC, ERRCRIT, childargv[0], strerror(errno));
        return -1;
    }

} /* readfirstitem */

/****************************************************************************
 * acceptconnect -- Wait for a client to attach to this server
 ***************************************************************************/
void
acceptconnect (void)
{
    ackpack_t ipc_ack[1];
    headpack_t *header;
    struct sockaddr_in myaddr;
    struct sockaddr_in client_addr;
    pid_t pid;
    fd_set read_set[1];
    struct servent *port;
    unsigned char cmd[256];
    char tcpwinsize[32];
    int recfd;
    int socket_fd;
#if defined(_AIX)
    size_t length;
#else  /* defined(_AIX) */
    int length;
#endif /* defined(_AIX) */
    int len;
    int nselect;
    int pmdrequest;
    int lgnum;
    int cnt;
    int sig;
    int rc;
    int n;
    int i;

    pid = -1;
    pidcnt = 0;
    rmd_pidcnt = 0;

    /* create control socket for use by ftd utilities to peform operations
       such as launching pmds, refresh, backfresh, etc. */
    if ((ftdsrvsock = createftdsock()) == -1) {
        exit(1);
    }
    if ((rmdsrvsock = creatermdsock()) == -1) {
        exit(1);
    }
    if ((port = getservbyname(FTDD, "tcp"))) {
        listenport = ntohs(port->s_port);
    } else {
        listenport = (int) SERVER_PORT;
    }
    skosh.tv_sec = 10;
    skosh.tv_usec = 0;
     
    /* create main tcp socket for listening for pmd connections and 
       status information queries */

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        if (errno == EMFILE) {
            reporterr (ERRFAC, M_FILE, ERRCRIT, "", strerror(errno));
        }
        reporterr (ERRFAC, M_SOCKFAIL, ERRCRIT, strerror(errno));
        exit (EXITNORMAL);
    }
    n = 1;

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&n,
        sizeof(int)) < 0) {
        reporterr (ERRFAC, M_SOCKOP, ERRCRIT, strerror(errno));
        exit (EXITNORMAL);
    }
    memset((char*)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(listenport);

    len = sizeof(myaddr);
    if (bind(socket_fd, (struct sockaddr*)&myaddr, len) < 0) {
        reporterr (ERRFAC, M_BINDERR, ERRCRIT, listenport);
        exit (EXITNORMAL);
    }

    /* set the TCP window from the value in the ftd.conf file */
    if ((cfg_get_key_value( "tcp_window_size", tcpwinsize, 
                           CFG_IS_NOT_STRINGVAL)) == CFG_OK) {
        n = atoi(tcpwinsize);
        if ((n > (256 * 1024)) || (n < 0)) {
            n = 256 * 1024;
        }
    } else {
        /* wasn't there, just default to max value */
        n = 256*1024;
    }
    setsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, (char*)&n, sizeof(int));
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF, (char*)&n, sizeof(int));

    if (listen(socket_fd, 5) < 0) {
        reporterr (ERRFAC, M_LISTEN, ERRCRIT);
        exit (EXITNORMAL);
    }
    while (1) {
        /* get primary config paths */
        lgcnt = getconfigs(configpaths, 1, 1);
        paths = (char*)configpaths;

        FD_ZERO(read_set);
        FD_SET(sigpipe[0], read_set);
        FD_SET(ftdsrvsock, read_set);
        FD_SET(rmdsrvsock, read_set);
        FD_SET(socket_fd, read_set);

        nselect = ftdsrvsock > rmdsrvsock ? ftdsrvsock: rmdsrvsock;
        nselect = socket_fd > nselect ? socket_fd: nselect;
        nselect = sigpipe[0] > nselect ? sigpipe[0]: nselect;

        /* listen on child pipes too */
        for (i = 0; i < MAXLG; i++) {
            if (p_cp[i].fd[0][STDIN] == -1) {
                continue;
            }
            FD_SET(p_cp[i].fd[0][STDIN], read_set);
            nselect = nselect > p_cp[i].fd[0][STDIN] ? nselect: p_cp[i].fd[0][STDIN];
        }
        if (ftdsock > 0) {
            close(ftdsock);
        }
        if (rmdsock > 0) {
            close(rmdsock);
        }
        ftdsock = -1;
        rmdsock = -1;
        nselect++;

        n = select(nselect, read_set, NULL, NULL, NULL);
        switch(n) {
        case -1:
            break;
        case 0:
            break;
        default:
            if (FD_ISSET(ftdsrvsock, read_set)) {
                /* ftd utility is trying to connect with us */
                if ((ftdsock = getftdsock()) == -1) {
                    break;
                }
                /* process the message  */
                if ((sig = ipcrecv_sig(ftdsock, &cnt)) > 0) {
                    dispatch_signal(sig, cnt);
                }
            } else if (FD_ISSET(rmdsrvsock, read_set)) {
                /* rmd is trying to connect with us */
                if ((rmdsock = getrmdsock()) == -1) {
                    break;
                }
                /* process the message  */
                if ((sig = ipcrecv_sig(rmdsock, &cnt)) > 0) {
                    dispatch_signal(sig, cnt);
                }
            } else if (FD_ISSET(sigpipe[0], read_set)) {
                /* handle signal */
                if (read(sigpipe[0], &sig, sizeof(sig)) == sizeof(sig)) {
                    dispatch_signal(sig, 0);
                }
            } else if (FD_ISSET(socket_fd, read_set)) {
                /* a pmd is attempting to connect with us, or someone is attempting
                   to query device information from us */
                length = sizeof(client_addr);
	
                /* accept the connection */
                if ((recfd=accept(socket_fd, 
                    (struct sockaddr*)&client_addr, &length)) < 0) {
                    reporterr (ERRFAC, M_ACCEPT, ERRCRIT, recfd, strerror(errno));
                    exit (EXITNORMAL);
                }
                /* read 1st four bytes */
                memset(cmd, 0, sizeof(cmd));
                pmdrequest = readfirst_fourbytes(recfd, cmd);
                if (pmdrequest == -1) {
                    close(recfd);
                    continue;
                } else if (pmdrequest == 1) {
                    /* read rest of header */
                    (void)memcpy((void*)&header,(void*)cmd,sizeof(char)*4);
                    /* We already read the first four bytes above, read in the rest to
                       fill in the header structure */
                    if (-1 == (rc = readsock(recfd, (char*)&cmd[4], sizeof(headpack_t)-4))) {
                        close(recfd);
                        continue;
                    }
                    header = (headpack_t*)cmd;
                    /* get secondary config paths */
                    lgcnt = getconfigs(configpaths, 2, 0);
                    paths = (char*)configpaths;
                    lgnum = header->data;
                    /* open - communication channel for RMD */
                    if (pipe(r_cp[lgnum].fd[0]) < 0) {
                        /* should the master report errors ? */
                        reporterr (ERRFAC, M_PIPE, ERRWARN, strerror(errno));
                    }
                    if (pipe(r_cp[lgnum].fd[1]) < 0) {
                        /* should the master report errors ? */
                        reporterr (ERRFAC, M_PIPE, ERRWARN, strerror(errno));
                    }
                }
                pid = fork();
                switch (pid) {
                case -1:
                    reporterr(ERRFAC, M_FORK, ERRCRIT, strerror(errno));
                    exit (EXITNORMAL);
                case 0:
                    /* child - RMD */
                    /* close unused pipe descriptors */
                    if (pmdrequest) {
                        close(r_cp[lgnum].fd[0][STDIN]);
                        r_cp[lgnum].fd[0][STDIN] = -1;
                        close(r_cp[lgnum].fd[1][STDOUT]);
                        r_cp[lgnum].fd[1][STDOUT] = -1;
                    }
                    close(socket_fd);
                    if (setsockopt(recfd, SOL_SOCKET, SO_KEEPALIVE,(char*)&n,sizeof(int))) {
                        reporterr (ERRFAC, M_SOCKOP, ERRCRIT, recfd, strerror(errno));
                        close(recfd);
                        exit(0);
                    }
                    /* read 1st message, and decide if we need to spawn a child rmd or
                       satisfy some other request */
                    if (-1 == readfirstitem(recfd, pmdrequest, cmd)) {
                        exit(-1);
                    } 
                    /* if we get here then we were not an RMD */
                    close(recfd);
                    exit(0);
                default:
                    /* parent */
                    close(recfd);
                    if (pmdrequest) {
                        if (lgnum != -1) {
                            /* close unused pipe descriptors */
                            close(r_cp[lgnum].fd[0][STDOUT]);
                            r_cp[lgnum].fd[0][STDOUT] = -1;
                            close(r_cp[lgnum].fd[1][STDIN]);
                            r_cp[lgnum].fd[1][STDIN] = -1;
                            r_cp[lgnum].pid = pid;
                        }
                        rmd_pidcnt++;
                    }
                    pidcnt++;
                }                   		
            } else {
                /* check child communication pipes */
                for (i = 0; i < MAXLG; i++) {
                    if (p_cp[i].fd[0][STDIN] == -1) {
                        continue;
                    }
                    if (FD_ISSET(p_cp[i].fd[0][STDIN], read_set)) {
                        /* get msg from child process */
                        if (p_cp[i].fd[0][STDIN] > 0) {
                            ipcrecv_ack(p_cp[i].fd[0][STDIN]);
                        }
                        if (ftdsock > 0) {
                            ipcsend_ack(ftdsock);
                        }
                    }
                }
            }
        }
    }

} /* acceptconnect */

/****************************************************************************
 * sig_handler -- master daemon signal handler
 ***************************************************************************/
static void
sig_handler(int s)
{
    switch(s) {
    case SIGTERM:
        killpmds();
        exit(0);
    default:
        break;
    }
    write_signal(sigpipe[1], s);

    return;
} /* sig_handler */

/****************************************************************************
 * installsigaction -- installs signal actions and creates signal pipe
 ***************************************************************************/
static void
installsigaction(void)
{
    int i;
    int numsig;
    struct sigaction msaction;
    sigset_t block_mask;
    static int msignal[] = { SIGHUP, SIGCHLD, SIGTERM };

    if (pipe(sigpipe) < 0) {
        reporterr(ERRFAC, M_PIPE, ERRWARN, strerror(errno));
    }
    sigemptyset(&block_mask);
    numsig = sizeof(msignal)/sizeof(*msignal);

    for (i = 0; i < numsig; i++) {
        msaction.sa_handler = sig_handler;
        msaction.sa_mask = block_mask;
        msaction.sa_flags = SA_RESTART;
        sigaction(msignal[i], &msaction, NULL);
    } 
    return;
} /* installsigaction */

/****************************************************************************
 * usage -- print usage message and exit 
 ***************************************************************************/
void 
usage(int argc, char **argv) 
{
    printf ("%s usage:\n", argv0);
    printf ("%s [options]\n", argv0);
    printf ("  %s \\\n", argv0);
    printf ("  %s -version\n", argv0);
    printf ("  %s -help\n", argv0);

    exit (EXITNORMAL);
} /* usage */

/****************************************************************************
 * processargs -- process startup arguments
 ***************************************************************************/
void 
processargs (int argc, char** argv) 
{
  int i;
       
  i = 1;
  
  while (i < argc) {
    if (0 == strcmp("-version", argv[i])) {
      fprintf (stderr, "Version " VERSION "\n");
      exit (EXITNORMAL);
    } else {
      usage(argc, argv);
    }
    i++;
  }
} /* processargs */
 
/****************************************************************************
 * processenviron -- process environment variables
 ***************************************************************************/
void 
processenviron (char** envirp) 
{
    /* count environment variables */
    pmdenvcnt = 0;
    while (envirp[pmdenvcnt]) {
        pmdenvcnt++;		
    }
    return;
} /* processenviron */

/****************************************************************************
 * throtdstart -- start throttle evaluation daemon 
 ***************************************************************************/
int 
throtdstart (void) 
{
    char **throtargv;
    pid_t pid;
    int pcnt;

    while ((pid = getprocessid("throtd", 1, &pcnt)) > 0) {
        kill(pid, SIGKILL);
    }
    pid_throtd = fork();
    switch (pid_throtd) {
    case -1:
        reporterr(ERRFAC, M_FORK, ERRCRIT, strerror(errno));
        exit (EXITNORMAL);
    case 0:
        throtargv = (char **) ftdmalloc(2*sizeof(char*));
        throtargv[0] = (char*) ftdmalloc (strlen("throtd")+1);
        throtargv[1] = (char*)NULL;
        strcpy(throtargv[0], "throtd");
        
        execv(PATH_BIN_FILES "/throtd", throtargv);
        reporterr(ERRFAC, M_EXEC, ERRCRIT, PATH_BIN_FILES "/throtd", strerror(errno));
        exit(1);
    default:
        /* parent */
        pidcnt++;
        return 0;
    }
}                   		 

/****************************************************************************
 * daemon_init -- initialize process as a daemon 
 ***************************************************************************/
int
daemon_init(void) 
{
    pid_t pid;

    if ((pid = fork()) < 0) {
        return -1;
    } else if (pid != 0) {
        exit (0);
    }
    setsid();
    chdir("/");
    umask(0);

    return 0;
} /* daemon_init */

/****************************************************************************
 * main
 *     - process command line arguments, environment variables
 *     - install signal handler 
 *     - create man control server socket 
 *     - accept connections forever
 ***************************************************************************/
int
main (int argc, char** argv, char **environ)
{
    pid_t pid;
    int pcnt; 
    int i; 

    argv0 = strdup(argv[0]);

    if (initerrmgt(ERRFAC) < 0) {
        exit(1);
    }
    /* -- Make sure we're not already running -- */
    if ((pid = getprocessid(FTDD, 0, &pcnt)) > 0) {
        if (pid != getpid() || pcnt > 1) {
            reporterr(ERRFAC, M_RUNAGAIN, ERRWARN, FTDD);
            exit(0);
        }
    } 
    /* -- Make sure we're root -- */
    if (geteuid()) {
        reporterr (ERRFAC, M_NOTROOT, ERRCRIT);
        exit(1);
    }
    /* init global structures not used in master */
    mysys = othersys = NULL;
    paths = (char*)configpaths;

    for (i = 0; i < MAXLG; i++) {
        p_cp[i].fd[0][STDIN] = -1;
        p_cp[i].fd[1][STDIN] = -1;
        p_cp[i].fd[0][STDOUT] = -1;
        p_cp[i].fd[1][STDOUT] = -1;
        p_cp[i].pid = -1;
        r_cp[i].fd[0][STDIN] = -1;
        r_cp[i].fd[1][STDIN] = -1;
        r_cp[i].fd[0][STDOUT] = -1;
        r_cp[i].fd[1][STDOUT] = -1;
        r_cp[i].pid = -1;
    }
    rcnt = restartcnts;

    /* process environment */
    processenviron (environ);

    pmdenviron = (char**)ftdmalloc((pmdenvcnt+10)*sizeof(char*));
    i = 0;
    while (environ[i]) {
        pmdenviron[i] = (char*)ftdmalloc((1+strlen(environ[i]))*sizeof(char));
        strcpy(pmdenviron[i], environ[i]);
        i++;	
    }
    pmdenviron[i] = (char*)NULL;
    pmdargv = (char**)ftdcalloc(2, sizeof(char**));

    /* install signal handler */
    installsigaction();

    /* get cmd line args */
    processargs (argc, argv);

    /* make this a daemon */
    daemon_init();

    /* set group id */
    setpgid(getpid(),getpid());

    /* Fire off throtd */
    (void)throtdstart();

    /* Wait for connection */
    acceptconnect();
 
    exit(0);
} /* main */
