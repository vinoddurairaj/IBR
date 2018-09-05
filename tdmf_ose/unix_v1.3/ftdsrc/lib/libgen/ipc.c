/**************************************************************************
 * ipc.c - FullTime Data 
 *
 * (c) Copyright 1998 FullTime Software, Inc. All Rights Reserved
 *
 * This module implements the functionality for daemon IPC
 *
 ***************************************************************************/

#ifdef NEED_BIGINTS
#include "bigints.h"
#endif

#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/un.h>
#include "errors.h"
#include "config.h"
#include "network.h"
#include "pathnames.h"
#include "process.h"
#include "ipc.h"

extern char *_pmd_configpath; 
extern int _pmd_state; 
extern int _pmd_state_change; 
extern int _pmd_wpipefd; 
extern int _pmd_rpipefd; 
extern int _pmd_restart; 
extern int _pmd_verify_secondary; 

int _pmd_hup;
extern int _pmd_cpon;
extern int _pmd_cpstart;
extern int _pmd_cpstop;

int rmd_pidcnt;
extern int _rmd_rpipefd;
extern int _rmd_wpipefd; 

extern int exitsuccess; 
extern int exitfail; 
extern int lgcnt; 

ftdpid_t p_cp[MAXLG];
ftdpid_t r_cp[MAXLG];

extern char *paths;
extern char *argv0;

char jrnpaths[MAXJRN][32];

struct stat statbuf[1];
struct timeval seltime[1];

struct sockaddr_un ftdsockaddr[1];
int ftdsrvsock;	
int rmdsrvsock;	
int ftdsock;	
int rmdsock;	
int len;

int targets[MAXLG];

static int pcnt;

/****************************************************************************
 * sig_action -- dispatch routine for ftd state changes 
 ***************************************************************************/
void
sig_action(int lgnum)
{
    char lgname[32];
    int action;
    int cnt;

    if ((action = ipcrecv_sig(_pmd_rpipefd, &cnt)) == -1) {
        exit(exitfail);
    }
    DPRINTF("\n*** sig_action: _pmd_state, action = %d, %d\n", _pmd_state, action);

    if (action == FTDQCPONP) {
        if (_pmd_cpstart) {
            reporterr(ERRFAC, M_CPSTARTAGAIN, ERRWARN, argv0);
            _pmd_cpstart = 0;
            ipcsend_ack(_pmd_wpipefd);
            return;
        } else if (_pmd_cpon) {
            reporterr(ERRFAC, M_CPONAGAIN, ERRWARN, argv0);
            ipcsend_ack(_pmd_wpipefd);
            return;
        }
        /* start transition into checkpoint mode */
        _pmd_cpstart = 1;

        return;
    } else if (action == FTDQCPOFFP) {
        if (_pmd_cpstop) {
            reporterr(ERRFAC, M_CPSTOPAGAIN, ERRWARN, argv0);
            _pmd_cpstop = 0;
            ipcsend_ack(_pmd_wpipefd);
            return;
        } else if (!_pmd_cpon) {
            reporterr(ERRFAC, M_CPOFFAGAIN, ERRWARN, argv0);
            ipcsend_ack(_pmd_wpipefd);
            return;
        }
        /* start transition out of checkpoint mode */
        _pmd_cpstop = 1;

        return;
    }

    /* state change and restart */
    switch(_pmd_state) {
    case FTDBFD:
        /* current process state is backfresh */
        if (action == FTDQBFD) {
            /* start backfresh */
            _pmd_state = FTDBFD;
            _pmd_state_change = 1;
            break;
        }
        break;
    case FTDRFD:
    case FTDRFDF:
        /* current process state is refresh */
        if (action == FTDQRFD) {
            /* re-start refresh */
            _pmd_state = FTDRFD;
            _pmd_state_change = 1;
        } else if (action == FTDQRFDF) {
            /* start FULL refresh */
            _pmd_state = FTDRFDF;
            _pmd_state_change = 1;
        } else if (action == FTDQRFDC) {
            /* verify secondary */
            _pmd_state = FTDRFD;
            _pmd_verify_secondary = 1;
            _pmd_state_change = 1;
        }
        break;
    default:
        /* current process state is normal */
        switch(action) {
        case FTDQHUP:
            /* re-init process */
            _pmd_hup = 1;
            _pmd_state_change = 1;
            break;
        case FTDQBFD:
            /* start backfresh */
            _pmd_state = FTDBFD;
            _pmd_state_change = 1;
            break;
        case FTDQRFD:
            /* start refresh */
            _pmd_state = FTDRFD;
            _pmd_state_change = 1;
            break;
        case FTDQRFDF:
            /* start FULL refresh */
            _pmd_state = FTDRFDF;
            _pmd_state_change = 1;
            break;
        case FTDQRFDC:
            /* verify secondary */
            _pmd_state = FTDRFD;
            _pmd_verify_secondary = 1;
            _pmd_state_change = 1;
            break;
        default:
            /* error - do nothing */
            break;
        }
        break;
    }

    return;
} /* sig_action */

/****************************************************************************
 * pmdhup -- Restart PMDs
****************************************************************************/
void
pmdhup(int sig, char **pmdargv)
{
    pid_t pid;
    char pname[16];
    int lgnum;
    int i;

    memset(targets, 0, sizeof(targets));
    /* get targeted logical groups from socket */
    if (sig == FTDQHUP) {
        if (ipcrecv_lg_list(ftdsock) == -1) {
            return;
        }
    } else {
        for (i = 0; i < lgcnt; i++) {
            targets[cfgtonum(i)] = 1;
        }
    }
    for (i = 0; i < lgcnt; i++) {
        lgnum = cfgtonum(i);
        if (targets[lgnum]) {
            sprintf(pname, "PMD_%03d", lgnum);
            if ((pid = getprocessid(pname, 1, &pcnt)) > 0) {
                if (ipcsend_sig(p_cp[lgnum].fd[1][STDOUT], 0, &sig) == -1) {
                    return;
                }
            } else {
                pid = startpmd(lgnum, sig, pmdargv);
            }
        }
    }
    return;
} /* pmdhup */

/****************************************************************************
 * pmdrsync -- Start rsync operation (BFD/RFD)  
 ****************************************************************************/
void
pmdrsync(int sig, char **pmdargv)
{
    pid_t pid;
    char pname[16];
    int lgnum;
    int i;
    int j;

    memset(targets, 0, sizeof(targets));
    if (ipcrecv_lg_list(ftdsock) == -1) {
        return;
    }
    for (i = 0; i < lgcnt; i++) {
        lgnum = cfgtonum(i);
        if (targets[lgnum]) {
            sprintf(pname, "PMD_%03d", lgnum);
            if ((pid = getprocessid(pname, 1, &pcnt)) > 0) {
                if (ipcsend_sig(p_cp[lgnum].fd[1][STDOUT], 0, &sig) == -1) {
                    return;
                }
            } else {
                pid = startpmd(lgnum, sig, pmdargv);
            }
        }
    }
    return;
} /* pmdrsync */

/****************************************************************************
 * checkpoint -- perform a checkpoint action 
 ****************************************************************************/
void
checkpoint(int cnt, char **pmdargv)
{
    ackpack_t ipcack[1];
    pid_t pid;
    char path[MAXPATHLEN];
    char tmppaths[MAXLG][32];
    char pname[16];
    char prefix[16];
    char *jrnpath;
    int jrnpath_len;
    int prefix_len;
    int jrncnt;
    int pipefd;
    int cp_found;
    int lgnum;
    int cfgidx;
    int sig;
    int rc;
    int i;
    int j;

    for (i = 0; i < cnt; i++) {
            if ((sig = ipcrecv_sig(ftdsock, &rc)) == -1) {
                return; 
            } 
            if ((lgnum = ipcrecv_lg(ftdsock)) == -1) {
                return; 
            } 
            cfgidx = numtocfg(lgnum);
            switch(sig) {
            case FTDQCPONP:
                /* turn checkpoint on from primary */ 
                sprintf(pname, "PMD_%03d", lgnum);
                if ((pid = getprocessid(pname, 1, &pcnt)) > 0) {
                    if (ipcsend_sig(p_cp[lgnum].fd[1][STDOUT], 0, &sig) == -1) {
                        continue;
                    }
                } else {
                    reporterr(ERRFAC, M_NOPMD, ERRWARN, paths+(cfgidx*32), pname);
                    /* let ftdcheckpoint know to stop waiting */
                    ipcsend_ack(ftdsock);
                }
                break;
            case FTDQCPONS:
                /* turn checkpoint on from secondary */ 
                lgcnt = GETCONFIGS(tmppaths, 2, 0);
                paths = (char*)tmppaths;
                cfgidx = numtocfg(lgnum);
                sprintf(pname, "RMD_%03d", lgnum);
                if ((pid = getprocessid(pname, 0, &pcnt)) > 0) {
                    if (ipcsend_sig(r_cp[lgnum].fd[1][STDOUT], 0, &sig) == -1) {
                        continue;
                    }
                } else {
                    reporterr(ERRFAC, M_NORMD, ERRWARN, paths+(cfgidx*32), pname);
                }
                break;
            case FTDQCPOFFP:
                /* turn checkpoint off from primary */ 
                sprintf(pname, "PMD_%03d", lgnum);
                if ((pid = getprocessid(pname, 1, &pcnt)) > 0) {
                    if (ipcsend_sig(p_cp[lgnum].fd[1][STDOUT], 0, &sig) == -1) {
                       continue; 
                    }
                } else {
                    reporterr(ERRFAC, M_NOPMD, ERRWARN, paths+(cfgidx*32), pname);
                    ipcsend_ack(ftdsock);
                }
                break;
            case FTDQCPOFFS:
                /* turn checkpoint off from secondary */ 
                lgcnt = GETCONFIGS(tmppaths, 2, 0);
                paths = (char*)tmppaths;
                cfgidx = numtocfg(lgnum);
                sprintf(pname, "RMD_%03d", lgnum);
                if ((pid = getprocessid(pname, 1, &pcnt)) > 0) {
                    if (ipcsend_sig(r_cp[lgnum].fd[1][STDOUT], 0, &sig) == -1) {
                        continue;
                    }
                } else {
                    if (readconfig(0, 0, 0, paths+(cfgidx*32)) == -1) {
                        reporterr(ERRFAC, M_CPOFFERR, ERRWARN, pname);
                        break;
                    }
                    /* clobber checkpoint file */
                    jrncnt = get_journals(jrnpaths, 0, 0);
                    if (jrncnt > 0) {
                        sprintf(prefix, "j%03d", lgnum);
                        prefix_len = strlen(prefix);
                        cp_found = 0;
                        for (j = 0; j < jrncnt; j++) {
                            if (strncmp(jrnpaths[j], prefix, prefix_len)) {
                                continue;
                            }
                            if (cp_file(jrnpaths[j], CP_ON)) {
                                sprintf(path, "%s/%s", mysys->group->journal_path, jrnpaths[j]);
                                unlink(path);
                                cp_found = 1;
                            }         
                        }
                        if (!cp_found) {
                            reporterr(ERRFAC, M_CPOFFAGAIN, ERRWARN, pname);
                            break;
                        } else {
                            reporterr(ERRFAC, M_CPOFF, ERRWARN, pname);
                            break;
                        }
                    } else if (jrncnt == -1) {
                        reporterr(ERRFAC, M_CPOFFERR, ERRWARN, pname);
                        break;
                    } else if (jrncnt == 0) {
                        reporterr(ERRFAC, M_CPOFFAGAIN, ERRWARN, pname);
                        break;
                    }
                }
                break;
            default:
                break;
            }
    }

    return;
} /* checkpoint */

/****************************************************************************
 * rmdapply -- Start RMD apply process
 ****************************************************************************/
void
rmdapply(int sig, char **pmdargv)
{
    pid_t pid;
    char pname[16];
    int lgnum;
    int i;

    memset(targets, 0, sizeof(targets));
    if (ipcrecv_lg_list(rmdsock) == -1) {
        return;
    }
    for (i = 0; i < lgcnt; i++) {
        lgnum = cfgtonum(i);
        DPRINTF("\n*** rmdapply: targets[%d] = %d\n", lgnum, targets[lgnum]);
        if (targets[lgnum]) {
            sprintf(pname, "RMDA_%03d", lgnum);
            if ((pid = getprocessid(pname, 1, &pcnt)) <= 0) {
                pid = startapply(lgnum, sig, pmdargv);
            }
        }
    }
    return;
} /* rmdapply */

/****************************************************************************
 * start_rmdapply -- signal master to start RMD apply process
 ****************************************************************************/
void
start_rmdapply(int waitflag)
{
    int sock;
    int lgnum;
    int sig;
 
    /* waitflag was not implemented because the master deamon 
       cannot afford to wait for a single child while other
       children may need its services. Need some other way to do it
    */ 
    /* start an apply process for this group */
    if ((sock = connecttomaster(FTDD, 1)) == -1) {
        reporterr (ERRFAC, M_RMDINIT, ERRCRIT, strerror(errno)); 
        exit (EXITANDDIE);        
    }
    lgnum = cfgpathtonum(mysys->configpath);
    sig = FTDQAPPLY;
    if (ipcsend_sig(sock, 0, &sig) == -1) {
        reporterr (ERRFAC, M_RMDINIT, ERRCRIT, strerror(errno)); 
        exit (EXITANDDIE);        
    }
    memset(targets, 0, sizeof(targets));
    targets[lgnum] = 1;
    if (ipcsend_lg_list(sock) == -1) {
        reporterr (ERRFAC, M_RMDINIT, ERRCRIT, strerror(errno)); 
        exit (EXITANDDIE);        
    }     
    close(sock);
    return;
} /* start_rmdapply */

/****************************************************************************
 * stop_rmdapply -- kill RMDA
 ****************************************************************************/
void
stop_rmdapply(void)
{
    pid_t pid;
    char pname[32];
    int pcnt;
    int lgnum;

    lgnum = cfgpathtonum(mysys->configpath);
    sprintf(pname, "RMDA_%03d", lgnum);
    if ((pid = getprocessid(pname, 1, &pcnt)) > 0) {
        kill(pid, SIGTERM);
    } 
    return;
} /* stop_rmdapply */

/****************************************************************************
 * connecttomaster -- ftd utility make a socket connection to Master PMD 
 ***************************************************************************/
int
connecttomaster(char* masterpath, int rmdflag)
{
    char *sock_path;
    int sock;
    int len;
    int tries;
    int maxtries;

    tries = 0;
    maxtries = 20;
    seltime->tv_sec = 0;
    seltime->tv_usec = 500000;

    if (getprocessid(masterpath, 1, &pcnt) <= 0) {
        return -1;
    }
    if (rmdflag) {
        sock_path = RMD_SOCKPATH;
    } else {
        sock_path = FTD_SOCKPATH;
    }
    while (1) {
        if (tries++ == maxtries) {
            reporterr(ERRFAC, M_NORESP, ERRCRIT);
            exit(1);
        }
        if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
            if (errno == EMFILE) {
                reporterr(ERRFAC, M_FILE, ERRCRIT, "socket", strerror(errno));
            }
            reporterr(ERRFAC, M_PMDFAIL, ERRCRIT, strerror(errno));
            exit(1);
        }
        memset(ftdsockaddr, 0, sizeof(ftdsockaddr));
        ftdsockaddr->sun_family = AF_UNIX;
        strcpy(ftdsockaddr->sun_path, sock_path);
#if defined(_AIX)
        len = SUN_LEN(ftdsockaddr);
#else  /* defined(_AIX) */
        len = strlen(ftdsockaddr->sun_path) + sizeof(ftdsockaddr->sun_family);
#endif /* defined(_AIX) */
        if (connect(sock, (struct sockaddr*)ftdsockaddr, len) == -1) {
            select(NULL, NULL, NULL, NULL, seltime);
            close(sock);
            continue;
        }
        break;
    }
    return sock;
} /* connecttomaster */

/****************************************************************************
 * getftdsock -- accept a socket connection from ftd utility 
 ***************************************************************************/
int
getftdsock(void)
{
#if defined(_AIX)
    size_t len;
#else  /* defined(_AIX) */
    int len;
#endif /* defined(_AIX) */
    struct sockaddr_un cliaddr[1];

    if (ftdsock > 0) {
        close(ftdsock);
        ftdsock = 0;
    }
    len = sizeof(cliaddr);
    if ((ftdsock = accept(ftdsrvsock, (struct sockaddr*)cliaddr, &len)) == -1) {
        reporterr(ERRFAC, M_ACCPTCONN, ERRCRIT, argv0, strerror(errno));
        return -1;
    }
    return ftdsock;
} /* getftdsock */

/****************************************************************************
 * getrmdsock -- accept a socket connection from an RMD
 ***************************************************************************/
int
getrmdsock(void)
{
#if defined(_AIX)
    size_t len;
#else  /* defined(_AIX) */
    int len;
#endif /* defined(_AIX) */
    struct sockaddr_un cliaddr[1];

    if (rmdsock) {
        close(rmdsock);
        rmdsock = 0;
    }
    len = sizeof(cliaddr);
    if ((rmdsock = accept(rmdsrvsock, (struct sockaddr*)cliaddr, &len)) == -1) {
        reporterr(ERRFAC, M_ACCPTCONN, ERRCRIT, argv0, strerror(errno));
        return -1;
    }
    return rmdsock;
} /* getrmdsock */

/****************************************************************************
 * createftdsock -- Create a UNIX domain socket - for utilities 
 ***************************************************************************/
int
createftdsock(void)
{
    int len;

    memset(ftdsockaddr, 0, sizeof(ftdsockaddr));
    ftdsockaddr->sun_family = AF_UNIX;
    strcpy(ftdsockaddr->sun_path, FTD_SOCKPATH);

    if ((ftdsrvsock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        if (errno == EMFILE) {
            reporterr(ERRFAC, M_FILE, ERRCRIT, "socket", strerror(errno));
        }
        reporterr(ERRFAC, "FTDSOCK", ERRCRIT);
        return -1;
    }
    unlink(FTD_SOCKPATH);
#if defined(_AIX)
    len = SUN_LEN(ftdsockaddr);
#else  /* defined(_AIX) */
    len = strlen(FTD_SOCKPATH) + sizeof(ftdsockaddr->sun_family);
#endif /* defined(_AIX) */
    if (bind(ftdsrvsock, (struct sockaddr*)ftdsockaddr, len) == -1) {
        reporterr(ERRFAC, M_PMDBIND, ERRCRIT, strerror(errno));
        return -1;
    }
    if (listen(ftdsrvsock, 5) == -1) {
        reporterr(ERRFAC, M_PMDLISTEN, ERRCRIT, strerror(errno));
        return -1;
    }

    return ftdsrvsock;
} /* createftdsock */

/****************************************************************************
 * creatermdsock -- Create a UNIX domain socket - for RMDs
 ***************************************************************************/
int
creatermdsock(void)
{
    int len;

    memset(ftdsockaddr, 0, sizeof(ftdsockaddr));
    ftdsockaddr->sun_family = AF_UNIX;
    strcpy(ftdsockaddr->sun_path, RMD_SOCKPATH);

    if ((rmdsrvsock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        if (errno == EMFILE) {
            reporterr(ERRFAC, M_FILE, ERRCRIT, "socket", strerror(errno));
        }
        reporterr(ERRFAC, "FTDSOCK", ERRCRIT);
        return -1;
    }
    unlink(RMD_SOCKPATH);
#if defined(_AIX)
    len = SUN_LEN(ftdsockaddr);
#else  /* defined(_AIX) */
    len = strlen(RMD_SOCKPATH) + sizeof(ftdsockaddr->sun_family);
#endif /* defined(_AIX) */
    if (bind(rmdsrvsock, (struct sockaddr*)ftdsockaddr, len) == -1) {
        reporterr(ERRFAC, M_PMDBIND, ERRCRIT, strerror(errno));
        return -1;
    }
    if (listen(rmdsrvsock, 5) == -1) {
        reporterr(ERRFAC, M_PMDLISTEN, ERRCRIT, strerror(errno));
        return -1;
    }
    return rmdsrvsock;
} /* creatermdsock */

/****************************************************************************
 * tell_master -- child notify master PMD so it doesn't wait forever
 ***************************************************************************/
void
tell_master(int pmdflag, int lgnum)
{
    char readybuf[32];
    int cfgidx;
    int rc;
    int fd;

    sprintf(readybuf, "_%03d_READY",lgnum);
    cfgidx = numtocfg(lgnum);
    if (pmdflag) {
        fd = _pmd_wpipefd;
    } else {
        fd = r_cp[lgnum].fd[0][STDOUT];
    }
    if ((rc = write(fd, readybuf, strlen(readybuf))) == -1) {
        reporterr(ERRFAC, M_IPCWERR, ERRWARN, argv0, fd, strerror(errno));
    }
    return;
} /* tell_master */

/****************************************************************************
 * ipcrecv_sig -- read state information from IPC channel 
 ****************************************************************************/
int
ipcrecv_sig(int fd, int *cnt)
{
    ipcpack_t header[1];
    char errbuf[32];
    int n;
    int sig; 
    int rc;

    memset(header, 0, sizeof(ipcpack_t));
    n = sizeof(ipcpack_t);
    if ((rc = read(fd, (char*)header, n)) != n) {
        if (rc == 0) {
            return -1;
        }
        if (rc == -1) {
            reporterr(ERRFAC, M_IPCRERR, ERRCRIT, argv0, fd, strerror(errno));
        }
        rc = -1;
    }
    if (header->msgtype != FTDQSIG) {
        sprintf(errbuf, "unexpected msg id: %d",header->msgtype);
        reporterr(ERRFAC, M_IPCRERR, ERRCRIT, argv0, fd, errbuf);
        rc = -1;
    }
    n = sizeof(int);
    if ((rc = read(fd, (char*)&sig, n)) != n) {
        if (rc == 0) {
            return -1;
        }
        if (rc == -1) {
            reporterr(ERRFAC, M_IPCRERR, ERRCRIT, argv0, fd, strerror(errno));
        }
        rc = -1;
    }
    *cnt = header->cnt;
    rc = sig;
    return rc;	
} /* ipcrecv_sig */

/****************************************************************************
 * ipcrecv_lg -- read state information from IPC channel 
 ****************************************************************************/
int
ipcrecv_lg(int fd)
{
    ipcpack_t header[1];
    char errbuf[32];
    int n;
    int lgnum; 
    int rc;

    memset(header, 0, sizeof(ipcpack_t));
    n = sizeof(ipcpack_t);
    if ((rc = read(fd, (char*)header, n)) != n) {
        if (rc == 0) {
            return -1;
        }
        if (rc == -1) {
            reporterr(ERRFAC, M_IPCRERR, ERRCRIT, argv0, fd, strerror(errno));
        }
 
        rc = -1;
    }
    if (header->msgtype != FTDQLG) {
        sprintf(errbuf, "unexpected msg id: %d",header->msgtype);
        reporterr(ERRFAC, M_IPCRERR, ERRCRIT, argv0, fd, errbuf);
        rc = -1;
    }
    n = sizeof(int);
    if ((rc = read(fd, (char*)&lgnum, n)) != n) {
        if (rc == 0) {
            return -1;
        }
        if (rc == -1) {
            reporterr(ERRFAC, M_IPCRERR, ERRCRIT, argv0, fd, strerror(errno));
        }
        rc = -1;
    }
    rc = lgnum;
    return rc;	
} /* ipcrecv_lg */

/****************************************************************************
 * ipcrecv_lg_list -- read state information from IPC channel 
 ****************************************************************************/
int
ipcrecv_lg_list(int fd)
{
    ipcpack_t header[1];
    int local_targets[MAXLG];
    int len;
    int n;
    int i;
    int rc;

    n = sizeof(ipcpack_t);
    if ((rc = read(fd, (char*)header, n)) != n) {
        if (rc == 0) {
            return -1;
        }
        if (rc == -1) {
            reporterr(ERRFAC, M_IPCRERR, ERRCRIT, argv0, fd, strerror(errno));
        }
        return -1;
    }
    if (header->msgtype != FTDQLGLIST) {
        reporterr(ERRFAC, M_IPCRERR, ERRCRIT, argv0, fd, strerror(errno));
        return -1;
    }
    len = header->cnt*sizeof(int);
    if (read(fd, (char*)local_targets, len) != len) {
        reporterr(ERRFAC, M_IPCRERR, ERRCRIT, argv0, fd, strerror(errno));
        if (rc == 0) {
            return -1;
        }
        return -1;
    }
    for (i = 0; i < header->cnt; i++) {
        targets[local_targets[i]] = 1; 
    }
    return 0;	
} /* ipcrecv_lg_list */

/****************************************************************************
 * ipcrecv_ack -- read ack information from IPC channel 
 ****************************************************************************/
int
ipcrecv_ack(int fd)
{
    ipcpack_t header[1];
    int n;
    int rc;

    n = sizeof(ipcpack_t);
    if ((rc = read(fd, (char*)header, n)) != n) {
        if (rc == 0) {
            return -1;
        }
        if (rc == -1) {
            reporterr(ERRFAC, M_IPCRERR, ERRCRIT, argv0, fd, strerror(errno));
        }
        return -1;
    }
    if (header->msgtype != FTDQACK) {
        reporterr(ERRFAC, M_IPCRERR, ERRCRIT, argv0, fd, strerror(errno));
        return -1;
    }
    return 0;	
} /* ipcrecv_ack */

/****************************************************************************
 * ipcsend_ack -- write state information onto IPC channel 
 ****************************************************************************/
int
ipcsend_ack(int fd)
{
    ipcpack_t header[1];
    int n;
    int rc;

    memset(header, 0, sizeof(ipcpack_t));

    header->msgtype = FTDQACK;
    n = sizeof(ipcpack_t);
    if ((rc = write(fd, (char*)header, n)) != n) {
        if (rc == -1) {
            reporterr(ERRFAC, M_IPCWERR, ERRWARN, argv0, fd, strerror(errno));
        }
        return -1;
    }
    return 0;
} /* ipcsend_ack */

/****************************************************************************
 * ipcsend_sig -- write state information onto IPC channel 
 ****************************************************************************/
int
ipcsend_sig(int fd, int cnt, int *sig)
{
    ipcpack_t header[1];
    int n;
    int rc;

    memset(header, 0, sizeof(header));
    header->msgtype = FTDQSIG;
    header->cnt = cnt;
    header->len = sizeof(int);
    n = sizeof(header);
    if ((rc = write(fd, (char*)header, n)) != n) {
        if (rc == -1) {
            reporterr(ERRFAC, M_IPCWERR, ERRWARN, argv0, fd, strerror(errno));
        }
        return -1;
    }
    if ((rc = write(fd, (char*)sig, sizeof(int))) != sizeof(int)) {
        reporterr(ERRFAC, M_IPCWERR, ERRWARN, argv0, fd, strerror(errno));
        return -1;
    }
    return 0;
} /* ipcsend_sig */

/****************************************************************************
 * ipcsend_lg -- write state information onto IPC channel 
 ****************************************************************************/
int
ipcsend_lg(int fd, int lgnum)
{
    ipcpack_t header[1];
    int n;
    int rc;

    memset(header, 0, sizeof(header));
    header->msgtype = FTDQLG;
    header->len = sizeof(char);

    n = sizeof(header);
    if ((rc = write(fd, (char*)header, n)) != n) {
        reporterr(ERRFAC, M_IPCWERR, ERRWARN, argv0, fd, strerror(errno));
        return -1;
    }
    if ((rc = write(fd, &lgnum, sizeof(int))) != sizeof(int)) {
        reporterr(ERRFAC, M_IPCWERR, ERRWARN, argv0, fd, strerror(errno));
        return -1;
    }
    return 0;
} /* ipcsend_lg */

/****************************************************************************
 * ipcsend_lg_list -- write state information onto IPC channel 
 ****************************************************************************/
int
ipcsend_lg_list(int fd)
{
    ipcpack_t header[1];
    int lglist[MAXLG];
    int n;
    int i;
    int rc;

    memset(header, 0, sizeof(header));
    header->msgtype = FTDQLGLIST;
    header->len = sizeof(char);

    /* count target lgs */
    memset(lglist, 0, sizeof(lglist));
    for (i = 0; i < MAXLG; i++) {
        if (targets[i]) {
            lglist[header->cnt++] = i;
        }
    }
    n = sizeof(header);
    if ((rc = write(fd, (char*)header, n)) != n) {
        reporterr(ERRFAC, M_IPCWERR, ERRWARN, argv0, fd, strerror(errno));
        return -1;
    }
    n = header->cnt*sizeof(int);
    if ((rc = write(fd, lglist, n)) != n) {
        reporterr(ERRFAC, M_IPCWERR, ERRWARN, argv0, fd, strerror(errno));
        return -1;
    }
    return 0;
} /* ipcsend_lg_list */

