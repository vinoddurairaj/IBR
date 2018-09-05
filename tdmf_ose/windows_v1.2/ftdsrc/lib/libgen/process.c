/***************************************************************************
 * process.c - FullTime Data Primary Mirroring Daemon
 *
 * (c) Copyright 1997, 1998 FullTime Software, Inc. All Rights Reserved
 *
 * This module implements the functionality for PMD process handling
 *
 ***************************************************************************/

#ifdef NEED_BIGINTS
#include "bigints.h"
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include "errors.h"
#include "config.h"
#include "network.h"
#include "pathnames.h"
#include "procinfo.h"
#include "ipc.h"
#include "process.h"

int lgcnt;
int *rcnt;
int pidcnt;
char **pmdenviron;
char *_pmd_configpath; 
int pmdenvcnt;
int pmdargc;
int _pmd_state; 
int _pmd_wpipefd; 
int _pmd_rpipefd; 
int _rmd_wpipefd; 
int _rmd_rpipefd; 
int _pmd_restart = 1; 

extern ftdpid_t p_cp[];

extern char *paths;
extern char *argv0;

struct stat statbuf[1];
struct timeval seltime[1];

/****************************************************************************
 * wait_child -- wait for child to contact parent 
 ***************************************************************************/
void
wait_child(int *cfgidx, int fd)
{
    struct timeval seltime[1];
    int rc;
    int tries;
    int maxtries;
    fd_set pfds[1];
    fd_set pfds_copy[1];
    int clen;
    int lgnum;
    int len;
    char lgname[32];
    char cstr[32];

    DPRINTF((ERRFAC,LOG_INFO,"\n*** wait_child: pidcnt, pipe fd = %d, %d\n",        pidcnt, fd));
    tries = 0;
    maxtries = 200;

    seltime->tv_sec = 0;
    seltime->tv_usec = 100000;

    FD_ZERO(pfds);
    FD_SET(fd, pfds);

    clen = strlen("_###_READY");
    *cfgidx = -1;

    while (1) {
        if (tries++ == maxtries) {
/*
            reporterr(ERRFAC, M_CHLDRESP, ERRWARN, argv0);
*/
            return;
        }
        *pfds_copy = *pfds;
        rc = select(fd+1, pfds_copy, NULL, NULL, seltime);
        DPRINTF((ERRFAC,LOG_INFO,"\n*** wait_child: select rc = %d\n", rc));
        switch(rc) {
        case -1:
            if (errno == EINTR || errno == EAGAIN) {
            } else {
                DPRINTF((ERRFAC,LOG_INFO,"\n*** wait_child: select error, rc = %s, %d\n",                    strerror(errno), rc));
            }
            break;
        case 0:
            continue;
        default:
            if (FD_ISSET(fd, pfds_copy)) {
                memset(cstr, 0, sizeof(cstr));
                rc = read(fd, cstr, clen);
                DPRINTF((ERRFAC,LOG_INFO,"\n*** master-child syncpipe read rc, text = %d, %s\n",                    rc, cstr));
                if (rc == 0) {
                } else if (rc != clen) {
/*
                    reporterr(ERRFAC, M_CHLDRESP, ERRWARN, argv0);
*/
                    return;
                }
                sscanf(cstr, "_%03d_READY",&lgnum);
                *cfgidx = numtocfg(lgnum);
                return;
            }
        }
    }
} /* wait_child */

/****************************************************************************
 * killpmds -- kill all pids in pids table 
 ****************************************************************************/
void
killpmds(void)
{
    int i;

    for (i = 0; i < MAXLG; i++) {
        if (p_cp[i].pid > 0) {
            kill(p_cp[i].pid, SIGTERM);
        }
    }
    return;
} /* killpmds */

/****************************************************************************
 * execpmd -- setup and execute a PMD for a logical group
 ***************************************************************************/
int
execpmd(int lgnum, char **pmdargv)
{
    int cfgidx;
    int len;
    int i;

    cfgidx = numtocfg(lgnum);

    _pmd_configpath = (char*)ftdmalloc(32);
    len = strlen(paths+(cfgidx*32))-4;
    strncpy(_pmd_configpath, paths+(cfgidx*32), len);
    _pmd_configpath[len] = 0;
    pmdenviron[pmdenvcnt] = (char*)ftdmalloc((16+strlen(_pmd_configpath))*sizeof(char));
    sprintf(pmdenviron[pmdenvcnt++], "%s%s", "_PMD_CONFIGPATH=", _pmd_configpath);
    pmdenviron[pmdenvcnt] = (char*)NULL;
    if (pmdargv[0]) {
        free(pmdargv[0]);
    }
    pmdargv[0] = (char*)ftdmalloc((4+strlen(_pmd_configpath))*sizeof(char));
    for (i = 0; i < pmdenvcnt; i++) {
        if (!pmdenviron[i]) continue;
        if (!strncmp(pmdenviron[i], "_PMD_STATE=", strlen("PMD_STATE="))) {
            sscanf(pmdenviron[i], "_PMD_STATE=%d", &_pmd_state);
            break;
        }
    }
    sprintf(pmdargv[0], "PMD_%s", &(_pmd_configpath[1]));
    pmdargv[1] = (char*)NULL;

    /* overlay this process with the logical group PMD */
    execve(PMD_PATH, pmdargv, pmdenviron);
    reporterr(ERRFAC, M_EXEC, ERRCRIT, pmdargv[0], strerror(errno));
    exit (EXITANDDIE);
} /* execpmd */

/****************************************************************************
 * execapply -- setup and execute a RMD apply process for a logical group
 ***************************************************************************/
int
execapply(int lgnum, char **pmdargv)
{
    int cfgidx;
    int len;

    cfgidx = numtocfg(lgnum);

    _pmd_configpath = (char*)ftdmalloc(32);
    len = strlen(paths+(cfgidx*32))-4;
    strncpy(_pmd_configpath, paths+(cfgidx*32), len);
    _pmd_configpath[len] = 0;
    pmdenviron[pmdenvcnt] = (char*)ftdmalloc((16+strlen(_pmd_configpath))*sizeof(char));
    sprintf(pmdenviron[pmdenvcnt++], "%s%s", "_RMD_CONFIGPATH=", _pmd_configpath);
    pmdenviron[pmdenvcnt] = (char*)ftdmalloc(12*sizeof(char));
    sprintf(pmdenviron[pmdenvcnt++], "%s%d", "_RMD_APPLY=", 1);
    pmdenviron[pmdenvcnt] = (char*)NULL;
    if (pmdargv[0]) {
        free(pmdargv[0]);
    }
    pmdargv[0] = (char*)ftdmalloc((4+strlen(_pmd_configpath))*sizeof(char));
    sprintf(pmdargv[0], "RMDA_%s", &(_pmd_configpath[1]));
    pmdargv[1] = (char*)NULL;
    /* overlay this process with the logical group RMDA */
    DPRINTF((ERRFAC,LOG_INFO,"\n*** execapply: starting %s\n",RMD_PATH));
    execve(RMD_PATH, pmdargv, pmdenviron);
    reporterr(ERRFAC, M_EXEC, ERRCRIT, pmdargv[0], strerror(errno));
    exit (EXITANDDIE);
} /* execapply */

/****************************************************************************
 * startpmd -- start a PMD for a logical group 
 ***************************************************************************/
int
startpmd(int lgnum, int sig, char **pmdargv)
{
    pid_t pid;
    int state;
    int cfgidx;

    switch(sig) {
    case FTDQHUP:
        state = FTDPMD;
        break;
    case FTDQBFD:
        state = FTDBFD;
        break;
    case FTDQRFD:
        state = FTDRFD;
        break;
    case FTDQRFDC:
        state = FTDRFDC;
        break;
    case FTDQRFDF:
        state = FTDRFDF;
        break;
    default:
        /* error */
        return -1;
    }
    /* init the pipes */
    if (p_cp[lgnum].fd[0][STDIN] != -1) {
        close(p_cp[lgnum].fd[0][STDIN]);
        p_cp[lgnum].fd[0][STDIN] = -1;
    }
    if (p_cp[lgnum].fd[0][STDOUT] != -1) {
        close(p_cp[lgnum].fd[0][STDOUT]);
        p_cp[lgnum].fd[0][STDOUT] = -1;
    }
    if (p_cp[lgnum].fd[1][STDIN] != -1) {
        close(p_cp[lgnum].fd[1][STDIN]);
        p_cp[lgnum].fd[1][STDIN] = -1;
    }
    if (p_cp[lgnum].fd[1][STDOUT] != -1) {
        close(p_cp[lgnum].fd[1][STDOUT]);
        p_cp[lgnum].fd[1][STDOUT] = -1;
    }
    if (pipe(p_cp[lgnum].fd[0]) < 0) {
        reporterr(ERRFAC, M_PIPE, ERRWARN, strerror(errno));
        return -1;
    }	
    if (pipe(p_cp[lgnum].fd[1]) < 0) {
        reporterr(ERRFAC, M_PIPE, ERRWARN, strerror(errno));
        return -1;
    }	
    pid = fork();
    switch(pid) {
    case -1:
        reporterr(ERRFAC, M_FORK, ERRCRIT, strerror(errno));
        exit(1);
    case 0:
        /* child - PMD */
        /* close unused pipe descriptors */
        close(p_cp[lgnum].fd[0][STDIN]);
        p_cp[lgnum].fd[0][STDIN] = -1;
        close(p_cp[lgnum].fd[1][STDOUT]);
        p_cp[lgnum].fd[1][STDOUT] = -1;
        pmdenviron[pmdenvcnt] = (char*)ftdmalloc(14*sizeof(char));
        sprintf(pmdenviron[pmdenvcnt++], "_PMD_RESTART=%d", 0);
        pmdenviron[pmdenvcnt] = (char*)ftdmalloc(16*sizeof(char));
        sprintf(pmdenviron[pmdenvcnt++], "_PMD_RECONWAIT=%d", 0);
        pmdenviron[pmdenvcnt] = (char*)ftdmalloc(16*sizeof(char));
        sprintf(pmdenviron[pmdenvcnt++], "_PMD_STATE=%d", state);
        pmdenviron[pmdenvcnt] = (char*)ftdmalloc(16*sizeof(char));
        sprintf(pmdenviron[pmdenvcnt++], "_PMD_RPIPEFD=%d",
            p_cp[lgnum].fd[1][STDIN]);
        pmdenviron[pmdenvcnt] = (char*)ftdmalloc(16*sizeof(char));
        sprintf(pmdenviron[pmdenvcnt++], "_PMD_WPIPEFD=%d",
            p_cp[lgnum].fd[0][STDOUT]);
        pmdenviron[pmdenvcnt] = (char*)NULL;
        execpmd(lgnum, pmdargv);
        break;
    default:
        /* close unused pipe descriptors */
        close(p_cp[lgnum].fd[0][STDOUT]);
        p_cp[lgnum].fd[0][STDOUT] = -1;
        close(p_cp[lgnum].fd[1][STDIN]);
        p_cp[lgnum].fd[1][STDIN] = -1;
        rcnt[lgnum] = 0;
        p_cp[lgnum].pid = pid;
        wait_child(&cfgidx, p_cp[lgnum].fd[0][STDIN]);
        pidcnt++;
        break;	
    }	
    return pid;
} /* startpmd */

/****************************************************************************
 * startapply -- start a RMD apply process for a logical group 
 ***************************************************************************/
int
startapply(int lgnum, int sig, char **pmdargv)
{
    pid_t pid;
    char pname[32];
    int status;
    int pcnt;

    sprintf(pname, "RMDA_%03d", lgnum);
    if ((pid = getprocessid(pname , 1, &pcnt)) > 0) {
        return 0;
    }
    pid = fork();
    switch(pid) {
    case -1:
        reporterr(ERRFAC, M_FORK, ERRCRIT, strerror(errno));
        exit(1);
    case 0:
        execapply(lgnum, pmdargv);
    default:
        pidcnt++;
        break;	
    }	
    return pid;
} /* startapply */

/****************************************************************************
 * exec_cp_cmd -- exec given checkpoint command 
 ***************************************************************************/
int
exec_cp_cmd(int lgnum, char *cmd_prefix, int pmdflag)
{
    struct stat statbuf[1];
    char command[MAXPATHLEN];
    char execpath[MAXPATHLEN];
    char **largv;
    pid_t pid, rpid;
    int status;
    int exitstatus;

    if (pmdflag) {
        sprintf(command, "%sp%03d.sh", cmd_prefix, lgnum);
        sprintf(execpath, "%s/%s", PATH_CONFIG, command);
    } else {
        sprintf(command, "%ss%03d.sh", cmd_prefix, lgnum);
        sprintf(execpath, "%s/%s", PATH_CONFIG, command);
    }
    /* if file doesn't exist - return success */
    if (stat(execpath, statbuf) == -1) {
        return 0;
    }
    pid = fork();
    switch(pid) {
    case -1:
        reporterr(ERRFAC, M_FORK, ERRCRIT, strerror(errno));
        exit(1);
    case 0:
        largv = (char**)ftdcalloc(2, sizeof(char*));
        largv[0] = strdup(command);
        largv[1] = NULL;
        execv(execpath, largv);
        reporterr(ERRFAC, M_EXEC, ERRWARN, largv[0], strerror(errno));
        exit(1);
    default:
        /* wait for command to complete - check return status */
        while ((rpid = waitpid(pid, &status, 0)) <= 0) {
            if (rpid == pid) {
                break;
            } 
            if (rpid == -1) {
                if (errno == ECHILD) break;
            }
        }
        if (WIFEXITED(status)) {
            exitstatus = WEXITSTATUS(status);
            DPRINTF((ERRFAC,LOG_INFO,"\n*** %s exitstatus = %d\n",command,exitstatus));
            if (exitstatus == 0) {
                return 0;
            }
            reporterr(ERRFAC, M_CPEXEC, ERRWARN, argv0, command);
            return -1;
        }
        return pid;
    }	
} /* exec_cp_cmd */

/****************************************************************************
 * exec_fs_sync -- exec OS sync command to sync filesystems 
 ***************************************************************************/
int
exec_fs_sync(char *fs, int lockflag)
{
    char execpath[MAXPATHLEN];
    char **largv;
    pid_t pid, rpid;
    int status;
    int exitstatus;

    strcpy(execpath, "/usr/sbin/lockfs");
    pid = fork();
    switch(pid) {
    case -1:
        reporterr(ERRFAC, M_FORK, ERRCRIT, strerror(errno));
        exit(1);
    case 0:
        largv = (char**)ftdcalloc(4, sizeof(char*));
        largv[0] = execpath;
        largv[1] = lockflag ? "-w": "-u";
        largv[2] = strdup(fs);
        largv[3] = NULL;
        execv(execpath, largv);
        reporterr(ERRFAC, M_EXEC, ERRWARN, largv[0], strerror(errno));
        exit(1);
    default:
        /* wait for command to complete - check return status */
        while ((rpid = waitpid(pid, &status, 0)) <= 0) {
            if (rpid == pid) {
                break;
            }
            if (rpid == -1) {
                if (errno == ECHILD) break;
            }
        }
        if (WIFEXITED(status)) {
            exitstatus = WEXITSTATUS(status);
            DPRINTF((ERRFAC,LOG_INFO,"\n*** %s exitstatus = %d\n",                  execpath, exitstatus));
            if (exitstatus == 0) {
                return 0;
            }
            reporterr(ERRFAC, M_CPEXEC, ERRWARN, argv0, execpath);
            return -1;
        }
        return pid;
    }	
} /* exec_fs_sync */

