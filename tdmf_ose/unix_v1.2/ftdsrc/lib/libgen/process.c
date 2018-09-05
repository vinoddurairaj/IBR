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


#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
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
pid_t rmda_pid[MAXLG];
char **pmdenviron;
int pmdenvcnt;
char *_pmd_configpath; 
int pmdargc;
extern int _pmd_state; 
int _pmd_wpipefd; 
int _pmd_rpipefd; 
int _rmd_wpipefd; 
int _rmd_rpipefd; 
int _pmd_restart = 1; 
int _rmd_jless; 

extern ftdpid_t p_cp[];
extern ftdpid_t r_cp[];

extern char *paths;
extern char *argv0;

static int rundir_fd = -1;

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

    ftd_trace_flow(FTD_DBG_IO, "pidcnt, pipe fd = %d, %d\n",  pidcnt, fd);
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
        ftd_trace_flow(FTD_DBG_IO, "select rc = %d\n", rc);
        switch(rc) {
        case -1:
            if (errno == EINTR || errno == EAGAIN) {
            } else {
                ftd_trace_flow(FTD_DBG_IO, "select error, rc = %s, %d\n", strerror(errno), rc);
            }
            break;
        case 0:
#if defined(linux)
            seltime->tv_sec = 0;
            seltime->tv_usec = 100000;
#endif /* defined(linux) */
            continue;
        default:
            if (FD_ISSET(fd, pfds_copy)) {
                memset(cstr, 0, sizeof(cstr));
                rc = read(fd, cstr, clen);
                ftd_trace_flow(FTD_DBG_IO, "master-child syncpipe read rc, text = %d, %s\n", rc, cstr);
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
#if defined(linux)
        seltime->tv_sec = 0;
        seltime->tv_usec = 100000;
#endif /* defined(linux) */
    }
} /* wait_child */

ftd_uint32_t 
pid_hash(const void *key, size_t keylen)
{
    ftd_uint32_t hash = *((ftd_uint32_t *)((pid_t *)key));
    hash = ~hash + (hash << 15);
    hash ^= (hash >> 12);
    hash += (hash << 2);
    hash ^= (hash >> 4);
    hash *= 2057;
    hash ^= (hash >> 16);
    return hash;
}


/****************************************************************************
 * pid_hash_add -- creates pid element
 ****************************************************************************/
   
static hash_ctx   *pid_hash_ctx = NULL;
static hash_table *pid_hash_tbl = NULL;

pid_hash_t *
pid_hash_add( pid_t pid, int lgnum, lgarry_e lgtbl )
{
    pid_hash_t *php;
    int ret;
    if (pid_hash_ctx == NULL)
    {
	pid_hash_ctx = hash_ctx_new();
	if (pid_hash_ctx == NULL || pid_hash_ctx->err) {
	    /* TODO Error processing hash_log_error(ctx->err); */
	    EXIT(EXITANDDIE);
	}
	pid_hash_ctx->hash_fn = pid_hash;
	pid_hash_ctx->free_value_fn = NULL;
	pid_hash_ctx->free_key_fn = NULL;
	pid_hash_tbl = hash_new( pid_hash_ctx, 4 * MAXLG );
	if (pid_hash_tbl == NULL || pid_hash_ctx->err) {
	    /* TODO Error processing hash_log_error(ctx->err); */
	    EXIT(EXITANDDIE);
	}
    }
    
    php = (pid_hash_t *)ftdmalloc( sizeof( pid_hash_t ));
    php->pid = pid;
    php->lgnum = lgnum;
    php->lgtbl = lgtbl;
    ret = hash_put( pid_hash_tbl, &php->pid, sizeof php->pid, php );
    /*TODO Error Processing */
    return php;
}

/****************************************************************************
 * pid_hash_find -- lookups pid from pid hash
 ****************************************************************************/

pid_hash_t *
pid_hash_find( pid_t pid )
{
    pid_hash_t *  php = NULL;
    if (pid_hash_tbl) {
	php = hash_get( pid_hash_tbl, &pid, sizeof (pid_t) );
        /* TODO error processing? */
    }
    return php;
}

/****************************************************************************
 * pid_has_delete -- remove pid from pid hash
 ****************************************************************************/

void 
pid_hash_delete( pid_hash_t *php )
{
    hash_del( pid_hash_tbl, &php->pid, sizeof php->pid );
    ftdfree( (void *)php );
}

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
    pmdenviron[pmdenvcnt] = (char*)ftdmalloc((16+strlen(_pmd_configpath)+1)*sizeof(char));
    sprintf(pmdenviron[pmdenvcnt++], "%s%s", "_PMD_CONFIGPATH=", _pmd_configpath);
    pmdenviron[pmdenvcnt] = (char*)NULL;
    if (pmdargv[0]) {
        free(pmdargv[0]);
	pmdargv[0] = NULL;
    }
    pmdargv[0] = (char*)ftdmalloc((4+strlen(_pmd_configpath)+1)*sizeof(char));
    for (i = 0; i < pmdenvcnt; i++) {
        if (!pmdenviron[i]) continue;
        if (!strncmp(pmdenviron[i], "_PMD_STATE=", strlen("PMD_STATE="))) {
            sscanf(pmdenviron[i], "_PMD_STATE=%d", &_pmd_state);
            break;
        }
    }
    sprintf(pmdargv[0], "PMD_%s", &(_pmd_configpath[1]));

    /* overlay this process with the logical group PMD */
    ch_rundir("pmd", lgnum);
    execve(PMD_PATH, pmdargv, pmdenviron);
    reporterr(ERRFAC, M_EXEC, ERRCRIT, pmdargv[0], strerror(errno));
    EXIT(EXITANDDIE);
    return 0; // Just to avoid compilation warnings.
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
    pmdenviron[pmdenvcnt] = (char*)ftdmalloc((16+strlen(_pmd_configpath)+1)*sizeof(char));
    sprintf(pmdenviron[pmdenvcnt++], "%s%s", "_RMD_CONFIGPATH=", _pmd_configpath);
    pmdenviron[pmdenvcnt] = (char*)ftdmalloc((12+1)*sizeof(char));
    sprintf(pmdenviron[pmdenvcnt++], "%s%d", "_RMD_APPLY=", 1);
    if (log_internal)
        pmdenviron[pmdenvcnt++] = strdup(LOGINTERNAL "=1");
    if (save_journals)
        pmdenviron[pmdenvcnt++] = strdup(SAVEJOURNALS "=1");
    pmdenviron[pmdenvcnt] = (char*)NULL;
    if (pmdargv[0]) {
        free(pmdargv[0]);
	pmdargv[0] = NULL;
    }
    pmdargv[0] = (char*)ftdmalloc((5+strlen(_pmd_configpath)+1)*sizeof(char));
    sprintf(pmdargv[0], "RMDA_%s", &(_pmd_configpath[1]));
    /* overlay this process with the logical group RMDA */
    ftd_trace_flow(FTD_DBG_FLOW1, "starting %s\n", RMD_PATH);
    ch_rundir("rmda", lgnum);
    execve(RMD_PATH, pmdargv, pmdenviron);
    reporterr(ERRFAC, M_EXEC, ERRCRIT, pmdargv[0], strerror(errno));
    EXIT(EXITANDDIE);
    return 0; // Just to avoid compilation warnings.
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
  int i, j;
  
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
   /* FRF - Set the pmd state to FTDRFDR, when launrefresh -ra command has issued */
  case FTDQRFDR:
    state = FTDRFDR;
    break;
  case FTDQNETANALYSIS:
    state = FTDNETANALYSIS;
    break;
  default:
    /* error */
    return -1;
  }
  /* init the pipes */
  close_pipe_set(lgnum, 1);
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
    EXIT(EXITANDDIE);
  case 0:
    /* child - PMD */
    /* close unused pipe descriptors */
    close_pipes(lgnum, 1);
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
    if (log_internal)
        pmdenviron[pmdenvcnt++] = strdup(LOGINTERNAL "=1");
    pmdenviron[pmdenvcnt] = (char*)NULL;
    execpmd(lgnum, pmdargv);
    break;
  default:
    /* close unused pipe descriptors */
    if (p_cp[lgnum].fd[0][STDOUT] != -1) {
      close(p_cp[lgnum].fd[0][STDOUT]);
      p_cp[lgnum].fd[0][STDOUT] = -1;
    }
    if (p_cp[lgnum].fd[1][STDIN] != -1) {
      close(p_cp[lgnum].fd[1][STDIN]);
      p_cp[lgnum].fd[1][STDIN] = -1;
    }
    rcnt[lgnum] = 0;
    p_cp[lgnum].pid = pid;
    pid_hash_add( pid, lgnum, LGARRY_PMD );
    wait_child(&cfgidx, p_cp[lgnum].fd[0][STDIN]);
    pidcnt++;
    break;	
  }	
  return pid;
} /* startpmd */

/****************************************************************************
 * startapply -- start a RMD apply process for a logical group 
 ***************************************************************************/
pid_t
startapply(int lgnum, int sig, char **pmdargv)
{
  pid_t pid;
  char pname[32];
  int status;
  int pcnt;
  int i, j;
  
  sprintf(pname, "RMDA_%03d", lgnum);
  if ((pid = getprocessid(pname , 1, &pcnt)) > 0) {
    return 0;
  }
  pid = fork();
  switch(pid) {
  case -1:
    reporterr(ERRFAC, M_FORK, ERRCRIT, strerror(errno));
    EXIT(EXITANDDIE);
  case 0:
    /* child - RMD */
    /* close unused pipe descriptors */
    close_pipes(lgnum, 0);
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
  int i, j;
  
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
    EXIT(EXITANDDIE);
  case 0:
    /* close _ALL_ open pipes */
    close_pipes(-1, 0);
    largv = (char**)ftdcalloc(2, sizeof(char*));
    largv[0] = strdup(command);
    largv[1] = NULL;
    execv(execpath, largv);
    reporterr(ERRFAC, M_EXEC, ERRWARN, largv[0], strerror(errno));
    EXIT(EXITANDDIE);
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
      ftd_trace_flow(FTD_DBG_IO, "%s exitstatus = %d\n", command, exitstatus);
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
    int i, j;

    strcpy(execpath, "/usr/sbin/lockfs");
    pid = fork();
    switch(pid) {
    case -1:
        reporterr(ERRFAC, M_FORK, ERRCRIT, strerror(errno));
        EXIT(EXITANDDIE);
    case 0:
      /* child -- close _ALL_ open pipes */
      close_pipes(-1, 0);
      largv = (char**)ftdcalloc(4, sizeof(char*));
      largv[0] = execpath;
      largv[1] = lockflag ? "-w": "-u";
      largv[2] = strdup(fs);
      largv[3] = NULL;
      execv(execpath, largv);
      reporterr(ERRFAC, M_EXEC, ERRWARN, largv[0], strerror(errno));
      EXIT(EXITANDDIE);
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
            ftd_trace_flow(FTD_DBG_IO, "%s exitstatus = %d\n", execpath, exitstatus);
            if (exitstatus == 0) {
                return 0;
            }
            reporterr(ERRFAC, M_CPEXEC, ERRWARN, argv0, execpath);
            return -1;
        }
        return pid;
    }	
} /* exec_fs_sync */

/****************************************************************************
 * close_pipes -- close all pipes except for lgnum, then prune lgnum 
 *                according to whether pmd or rmd pipes should be retained
 ***************************************************************************/
void
close_pipe_set (int lgnum, int pmdflag)
{
  /* close the four pipe file descriptors for a logical group
     pmd or rmd */
  if (pmdflag) {
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
  } else {
    if (r_cp[lgnum].fd[0][STDIN] != -1) {
      close(r_cp[lgnum].fd[0][STDIN]);
      r_cp[lgnum].fd[0][STDIN] = -1;
    }
    if (r_cp[lgnum].fd[0][STDOUT] != -1) {
      close(r_cp[lgnum].fd[0][STDOUT]);
      r_cp[lgnum].fd[0][STDOUT] = -1;
    }
    if (r_cp[lgnum].fd[1][STDIN] != -1) {
      close(r_cp[lgnum].fd[1][STDIN]);
      r_cp[lgnum].fd[1][STDIN] = -1;
    }
    if (r_cp[lgnum].fd[1][STDOUT] != -1) {
      close(r_cp[lgnum].fd[1][STDOUT]);
      r_cp[lgnum].fd[1][STDOUT] = -1;
    }
  }
  return;
}

void 
close_pipes(int lgnum, int pmdflag)
{
  int i, j;
  
  for (i=0; i<MAXLG; i++) {
    /* NOTE -- setting lgnum to -1 will close each and every pipe */
    if (i == lgnum) {
      if (pmdflag) {
	/* close the two out of four unused pipes open for this PMD */
	if (p_cp[i].fd[0][STDIN] != -1) {
	  close(p_cp[i].fd[0][STDIN]);
	  p_cp[i].fd[0][STDIN] = -1;
	}
	if (p_cp[i].fd[1][STDOUT] != -1) {
	  close(p_cp[i].fd[1][STDOUT]);
	  p_cp[i].fd[1][STDOUT] = -1;
	}
        /* close all RMD pipes for this group */
	close_pipe_set(i, 0);
      } else {
	/* close the two out of four unused pipes open for this RMD */
	if (r_cp[i].fd[0][STDIN] != -1) {
	  close(r_cp[i].fd[0][STDIN]);
	  r_cp[i].fd[0][STDIN] = -1;
	}
	if (r_cp[i].fd[1][STDOUT] != -1) {
	  close(r_cp[i].fd[1][STDOUT]);
	  r_cp[i].fd[1][STDOUT] = -1;
	}
        /* close all PMD pipes for this group */
	close_pipe_set(i, 1);    
      }
    } else {
      /* close all PMD and RMD pipes for a logical group */
      close_pipe_set(i, 1);
      close_pipe_set(i, 0);
    }
  }
  return;
}
/* end close_pipes */

void
close_rundir(void)
{
    if (rundir_fd >= 0)
        close(rundir_fd);
}

int
open_rundir(const char *rundir)
{
    int fd = open(rundir, O_RDONLY);
    int ret = -1;
    int N;
    char buf[LINE_MAX];
    struct stat stat;
    if (fd < 0) {
        snprintf(buf, sizeof(buf), "open(%s, O_RDONLY) failed with %s", rundir, get_error_str(errno));
        reporterr(ERRFAC, M_INTERNAL, ERRWARN, buf);
        goto RETN;
    }
    if (fstat(fd, &stat) < 0) {
        snprintf(buf, sizeof(buf), "open_rundir(%s): fstat(%d) failed with %s", rundir, fd, get_error_str(errno));
        reporterr(ERRFAC, M_INTERNAL, ERRWARN, buf);
        goto RETN;
    }
    if (!S_ISDIR(stat.st_mode)) {
        errno = ENOTDIR;
        snprintf(buf, sizeof(buf), "open_rundir(%s): %s", rundir, get_error_str(errno));
        reporterr(ERRFAC, M_INTERNAL, ERRWARN, buf);
        goto RETN;
    }
    if (fchdir(fd) >= 0)
        ret = 0;
    rundir_fd = fd;
RETN:
    N = snprintf(buf, sizeof(buf), "rundir = ");
    getcwd(&buf[N], sizeof(buf) - N);
    reporterr(ERRFAC, M_INTERNAL, ERRINFO, buf);
    return ret;
}

#ifndef NAME_MAX
#define NAME_MAX 255
#endif
void
ch_rundir(const char *subdir, const int lgnum)
{
    int fd = -1;
    char buf[NAME_MAX];
    int N;
    if (rundir_fd < 0 || fchdir(rundir_fd) < 0)
        goto RETN;
    if (lgnum >= 0)
        snprintf(buf, sizeof(buf), "%s_%.3d", subdir, lgnum);
    else
        strcpy(buf, subdir);
    fd = open(buf, O_RDONLY);
    if (fd < 0) {
        if (mkdir(buf, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) >= 0)
            fd = open(buf, O_RDONLY);
        else
            goto RETN;
    }
    if (fchdir(fd) >= 0) {
        if (mkdir((sprintf(buf, "%d", getpid()), buf),
                  S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) >= 0)
            chdir(buf);
    }
RETN:
    N = snprintf(buf, sizeof(buf), "PWD=");
    if (getcwd(&buf[N], sizeof(buf) - N)) {
        putenv(buf);
    } else
        snprintf(buf, sizeof(buf), "getcwd(%#lx, %d) failed with %s\n",
                 (unsigned long)&buf[N], sizeof(buf) - N, get_error_str(errno));
    reporterr(ERRFAC, M_INTERNAL, ERRINFO, buf);
    if (fd >= 0)
        close(fd);
}

void
rm_rundir(const char *root, const char *subdir, const int lgnum, const pid_t pid)
{
    char rundir[MAXPATHLEN];
    char *slash = rundir;
    if (lgnum < 0)
        snprintf(rundir, sizeof(rundir), "%s/%s/%d", root, subdir, pid);
    else
        snprintf(rundir, sizeof(rundir), "%s/%s_%.3d/%d", root, subdir, lgnum, pid);
    chdir("../..");
    if (!rmdir(rundir)) {
        slash = strrchr(rundir, '/');
        *slash = '\0';
        rmdir(rundir);
    } else {
        int err = errno;
        ftd_debugf("%s, %s, %d, %u", "rmdir(%s) failed %s\n",
            root, subdir, lgnum, pid, rundir, get_error_str(-err));
    }
}

