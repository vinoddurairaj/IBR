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
 /*
 * (c) Copyright 1998 FullTime Software, Inc. All Rights Reserved
 *
 ***************************************************************************
 */
#ifdef NEED_BIGINTS
#include "bigints.h"
#endif

#include <stdlib.h>
#include <signal.h>
#include <limits.h>
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
#include <syslog.h>

#ifdef NEED_SYS_MNTTAB
#include "ftd_mount.h"
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
#include "procinfo.h"
#include "network.h"
#include "devcheck.h"
#include "cfg_intr.h"

#ifdef TDMF_TRACE
FILE* dbgfd;
#endif

#if defined(SOLARIS) /* 1LG-ManyDEV */ 
extern int dummyfd_mnt;
#endif

#include <netinet/tcp.h> 

/* wr15892 */
#if defined(HPUX) && (SYSVERS >= 1100)
#define DLKM
#endif
/* wr15892 */

static pid_t pid;
static pid_t pid_throtd = -999;

char *argv0;

static char configpaths[MAXLG][32];
static struct timeval skosh;
static time_t currentts;
static char pmdversionstring[256];
static char rmdversionstring[256];

static uint16_t listenport = 575;

static u_long boot_drive_migration = 0;
static u_long shutdown_source = 0;
static u_long keep_AIX_target_running = 0;

static int pmds_in_net_analysis_mode[MAXLG];
static int rmds_in_net_analysis_mode[MAXLG];

char** pmdenviron = (char**)NULL;
int pmdenvcnt = 0;
char** childargv = (char**)NULL;
int childargc = 0;

char *pcwd = NULL;
static char fmt[384];
static char msg[384];
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

int pmdargc;
char **pmdargv;

extern int ftdsrvsock;
extern int ftdsock;
extern int rmdsrvsock;
extern int rmdsock;

extern int sizediff[SIZE];
int throtdstart (void);

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
    int i, j;
/* add by PST 5 Dec 2002 for WR15229 */
    int rc;
    char ps_name[MAXPATHLEN];
    char group_name[MAXPATHLEN];
    char modestr[32];
    pid_hash_t *php;
/******************************/

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
#define USE_PID_HASH
#ifdef USE_PID_HASH
	php = pid_hash_find( pid );
	if (php)
	{
            const char *prefix = NULL;
	    switch(php->lgtbl)
	    {
		case LGARRY_PMD:
		    j = php->lgnum;
		    lgnum = j;
		    prefix = "pmd";
		    close_pipe_set(j, 1);
		    p_cp[j].pid = -1;
		    pid_hash_delete( php );
		    DPRINTF("\n*** reaping PMD: %d, new pidcnt = %d\n",j, pidcnt);
		    rm_rundir(pcwd, prefix, j, pid);
		    break;
		case LGARRY_RMD:
		    prefix = "rmd";
		    j = php->lgnum;
		    close_pipe_set(j, 0);
		    r_cp[j].pid = -1;
		    pid_hash_delete( php );
		    rmd_pidcnt--;
		    DPRINTF("\n*** reaping RMD: %d, new rmd_pidcnt = %d\n",j, rmd_pidcnt);
		    rm_rundir(pcwd, prefix, j, pid);
			if( rmds_in_net_analysis_mode[j] )
			{
                // If in network bandwidth analysis mode, remove the fictitious config files
	            remove_fictitious_cfg_files( j, 0 );
				rmds_in_net_analysis_mode[j] = 0;
			}
		    break;
		case LGARRY_RMDA:
		    prefix = "rmda";
		    j = php->lgnum;
		    rmda_pid[j] = -1;
		    pid_hash_delete( php );
		    rm_rundir(pcwd, prefix, j, pid);
		    break;
	    }
	}
#else
        for (j = 0; j < MAXLG; j++) {
            const char *prefix = NULL;
            if (p_cp[j].pid == pid) {
                lgnum = j;
                prefix = "pmd";
                close_pipe_set(j, 1);
                p_cp[j].pid = -1;
                DPRINTF("\n*** reaping PMD: %d, new pidcnt = %d\n",lgnum, pidcnt);
                rm_rundir(pcwd, prefix, j, pid);
                break;
            } else if (r_cp[j].pid == pid) {
                prefix = "rmd";
                close_pipe_set(j, 0);
                r_cp[j].pid = -1;
                rmd_pidcnt--;
                DPRINTF("\n*** reaping RMD: %d, new rmd_pidcnt = %d\n",j, rmd_pidcnt);
            } else if (rmda_pid[j] == pid)
                prefix = "rmda";
            if (prefix)
                rm_rundir(pcwd, prefix, j, pid);
        }
#endif
        pidcnt--;
        if (lgnum == -1) {
            continue;
        }
        /* relaunch PMD if it exited with reasonable status unless it was in network bandwidth analysis mode,
           simulating a Full Refreh */
        exitstatus = 0;
        exitsignal = 0;
        exitduetostatus = 0;
        exitduetosignal = 0;
        exitduetostatus = WIFEXITED(status);
        exitduetosignal = WIFSIGNALED(status);
        if (exitduetostatus) exitstatus = WEXITSTATUS(status);
        if (exitduetosignal) exitsignal = WTERMSIG(status);
        fprintf(stderr, "[%d]FTDD child(%d) exited, exit status=%d signal=%d\n",
            getpid(), pid, exitstatus, exitsignal);

        if (exitsignal == 0) {
            if( (exitstatus == EXITRESTART || exitstatus == EXITNETWORK) && !pmds_in_net_analysis_mode[lgnum] ) {
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
                    EXIT(EXITANDDIE);
                case 0: /* child */
                    /* close unused pipe descriptors */
                     close_pipes(lgnum, 1);
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
                        EXIT(EXITANDDIE);
                    }
                    /* mod by PST 5 Dec 2002 for WR15229 */
                    /* When Full-refresh disterved by rmd stop or Network trable */
                    /* Restart Full-refresh again                                */
                    /* Create group name from lgnum for Pstore read */

                    FTD_CREATE_GROUP_NAME(group_name, lgnum);

                    /* Get Pstore's Path(= /dev/rdsk/.... ) */
                    if (GETPSNAME(lgnum, ps_name) == -1) {
                        sprintf(group_name, "%s/%s%03d.cfg",
                                    PATH_CONFIG, PMD_CFG_PREFIX, lgnum);
                        reporterr(ERRFAC, M_CFGPS, ERRCRIT, argv0, group_name);
                        EXIT(EXITANDDIE);
                    }
                    /* Read Pstore and get "_MODE"keys value. */
                    rc = ps_get_group_key_value(ps_name, group_name, "_MODE:", modestr);
                    if (rc != PS_OK) {
                        syslog (LOG_ERR,"reaper : _MODE not found in Pstore\n");
                        EXIT(EXITANDDIE);
                    }
                    if ( strcmp( modestr, "FULL_REFRESH" ) == 0  ) {
                        /* if full_refresh was going, restart full refresh state for restart */
                        pmdenviron[pmdenvcnt] = (char*)ftdmalloc(16*sizeof(char));
                        /* FRF: When PMD is killed due to some reason during full refresh
                         * phase, start the full refresh from the place where it is
                         * last stopped ( not from zero) by setting pmd state to
                         * FTDRFDR */
                        sprintf(pmdenviron[pmdenvcnt++], "_PMD_STATE=%d", FTDRFDR); 
                        reporterr(ERRFAC, M_PMDRERUNF, ERRWARN);
                    }
                    else {
                        /* default to NORMAL-transfer state for restart */
                        pmdenviron[pmdenvcnt] = (char*)ftdmalloc(16*sizeof(char));
                        sprintf(pmdenviron[pmdenvcnt++], "_PMD_STATE=%d", FTDPMD);
                        reporterr(ERRFAC, M_PMDRERUN, ERRWARN);
                    }
                    /************************************************************************/
                    pmdenviron[pmdenvcnt] = (char*)ftdmalloc((16+1)*sizeof(char));
                    sprintf(pmdenviron[pmdenvcnt++], "_PMD_RECONWAIT=%d",_pmd_reconwait);
                    pmdenviron[pmdenvcnt] = (char*)ftdmalloc((17+1)*sizeof(char));
                    sprintf(pmdenviron[pmdenvcnt++], "_PMD_RETRYIDX=%d", lgnum);
                    pmdenviron[pmdenvcnt] = (char*)ftdmalloc((16+1)*sizeof(char));
                    sprintf(pmdenviron[pmdenvcnt++], "_PMD_RPIPEFD=%d",
                        p_cp[lgnum].fd[1][STDIN]);
                    pmdenviron[pmdenvcnt] = (char*)ftdmalloc((16+1)*sizeof(char));
                    sprintf(pmdenviron[pmdenvcnt++], "_PMD_WPIPEFD=%d",
                        p_cp[lgnum].fd[0][STDOUT]);
                    if (log_internal)
                        pmdenviron[pmdenvcnt++] = strdup(LOGINTERNAL "=1");
                    pmdenviron[pmdenvcnt] = (char*)NULL;

                    ftd_trace_flow(FTD_DBG_FLOW1, "start PMD\n");

                    GETCONFIGS(configpaths, 1, 1);
                    paths = (char*)configpaths;
                    execpmd(lgnum, pmdargv);
                default: /* parent */
                    /* close unused pipe descriptors */
                    if (p_cp[lgnum].fd[0][STDOUT] != -1) {
                        close(p_cp[lgnum].fd[0][STDOUT]);
                        p_cp[lgnum].fd[0][STDOUT] = -1;
                    }
                    if (p_cp[lgnum].fd[1][STDIN] != -1) {
                        close(p_cp[lgnum].fd[1][STDIN]);
                        p_cp[lgnum].fd[1][STDIN] = -1;
                    }
                    restartcount++;
                    p_cp[lgnum].pid = pid;
		    pid_hash_add( pid, lgnum, LGARRY_PMD );
                    wait_child(&cfgidx, p_cp[lgnum].fd[0][STDIN]);
                    pidcnt++;
                    break;
                } /* switch */
            } else {
                /* non-restart exitstatus */
                reporterr (ERRFAC, M_PMDNORERUN, ERRWARN, exitstatus);
				if( !pmds_in_net_analysis_mode[lgnum] )
				{
                    clearmode(lgnum, 0);
				}
				else
				{
                    remove_fictitious_cfg_files( lgnum, 1 );
				}
				pmds_in_net_analysis_mode[lgnum] = 0;
            } /* exitstatus == */
        } else {
            /* non-zero exitsignal */
            if (exitsignal != SIGTERM) {
                sprintf(pname, "PMD_%03d",lgnum);
                reporterr (ERRFAC, M_SIGNAL, ERRCRIT, pname, exitsignal);
				if( !pmds_in_net_analysis_mode[lgnum] )
				{
                    clearmode(lgnum, 0);
				}
				else
				{
                    // If in network bandwidth analysis mode, remove the fictitious config files
                    remove_fictitious_cfg_files( lgnum, 1 );
				}

				pmds_in_net_analysis_mode[lgnum] = 0;
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
        lgcnt = GETCONFIGS(configpaths, 1, 1);
        pmdhup(sig, pmdargv);
        break;
    case FTDQBFD:
    case FTDQRFD:
    case FTDQRFDF:
    case FTDQRFDC:
    case FTDQRFDR: /* FRF */
        lgcnt = GETCONFIGS(configpaths, 1, 1); 
        pmdrsync(sig, pmdargv, pmds_in_net_analysis_mode);
        break;
    case FTDQNETANALYSIS: /* Full Refresh simulation for Network bandwidth evaluation */
        lgcnt = GETCONFIGS(configpaths, 1, 0); 
        pmdrsync(sig, pmdargv, pmds_in_net_analysis_mode);
        break;
    case FTDQAPPLY:
        /* get secondary config paths */
        lgcnt = GETCONFIGS(configpaths, 2, 0);
        paths = (char*)configpaths;
        rmdapply(sig, pmdargv, rmda_pid);
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
doversioncheck (int fd, headpack32_t *header32, versionpack_t* version)
{
    struct stat statbuf[1];
    char configpath[MAXPATHLEN];
    int i;
    time_t tsdiff;
    headpack_t header[1];
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
    if (strncmp (pmdversionstring, "2.5.0.0", strlen(pmdversionstring))>=0)
    {
        pmd64 = 1;
    }
    else
    {
        pmd64 = 0;
    }
    memset (sizediff, 0, SIZE);
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
/*
        logerrmsg (ERRFAC, ERRWARN, M_CFGFILE, msg); 
this causes SIGSEGV - no time to find out why so replace
with call to reporterr for now which seems to work ???
*/
        reporterr(ERRFAC, M_CFGFILE, ERRWARN, argv0, configpath, strerror(errno));
        memset (header, 0, sizeof (header));
        converthdrfrom32to64 (header32, header);
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
readfirstitem (int fd, int pmdrequest, unsigned char *cmd, int read_RMD_config_file)
{
    headpack_t header[1];
    headpack32_t *header32;
    versionpack_t version;
	cfgfilepack_t cfg_file;
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
    int i;
    pid_t pid;
	char cmdline[80];
	int rc;
	int lgnum;
	int cfg_fd;
    struct stat statbuf;

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
    header32 = (headpack32_t*)cmd;
    /* Make sure this is a valid rmd message */
    if (header32->magicvalue != MAGICHDR) {
        if (mysys != NULL) 
            reporterr (ERRFAC, M_HDRMAGIC, ERRWARN);
        return (1);
    }

    switch (header32->cmd) {
    case CMDVERSION:
        if (-1 == (response = readsock(fd, (char*)&version, 
            sizeof(versionpack_t)))) {
            return (1);
        }
	    lgnum = (int)header32->data;
        if( read_RMD_config_file )
		{
			// Save the cfg file for the RMD: sxxx.cfg
			sprintf(tbuf, "%s/s%3d.cfg", PATH_CONFIG, lgnum);
		    if( stat(tbuf, &statbuf) == 0 ) // Check if the file exists; we do not want to overwrite
			{
                reporterr( ERRFAC, M_NETCFGEXISTS, ERRCRIT, tbuf );
		        geterrmsg (ERRFAC, M_NETCFGEXISTS, fmt);
		        sprintf (msg, fmt, tbuf);
		        memset (header, 0, sizeof (header));
		        converthdrfrom32to64 (header32, header);
		        (void) senderr (fd, header, 0L, ERRCRIT, M_NETCFGEXISTS, msg);
				return( 1 );
			}
			else
			{
				if ((cfg_fd = open(tbuf, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0)
				{
			        rc = errno;
	                reporterr( ERRFAC, M_NETCFGOPENERR, ERRCRIT, tbuf, strerror(rc) );
			        geterrmsg (ERRFAC, M_NETCFGOPENERR, fmt);
			        sprintf (msg, fmt, tbuf, strerror(rc));
			        memset (header, 0, sizeof (header));
			        converthdrfrom32to64 (header32, header);
			        (void) senderr (fd, header, 0L, ERRCRIT, M_NETCFGOPENERR, msg);
					return( 1 );
				}
				else
				{
					if (write(cfg_fd, &(version.cfg_file.file_data), version.cfg_file.file_length) != version.cfg_file.file_length)
				    {
				        rc = errno;
				        close(cfg_fd);
				        unlink( tbuf );
		                reporterr( ERRFAC, M_NETCFGWRITERR, ERRCRIT, tbuf, strerror(rc) );
				        geterrmsg (ERRFAC, M_NETCFGWRITERR, fmt);
				        sprintf (msg, fmt, tbuf, strerror(rc));
				        memset (header, 0, sizeof (header));
				        converthdrfrom32to64 (header32, header);
				        (void) senderr (fd, header, 0L, ERRCRIT, M_NETCFGWRITERR, msg);
						return( 1 );
					}
					else
					{
				        close(cfg_fd);
					}
				}
			}
		}
        if (0 > (response = doversioncheck (fd, header32, &version))) {
            return 1;
        }
        sock = fd;
        isconnected = 1;
        break;
    case CMDNOOP:
        return (1);
    case CMDEXIT:
        EXIT(EXITNORMAL); 
    case CMDFAILOVER:
	    lgnum = (int)header32->data;
		if( boot_drive_migration )
		{
		    // Note: the shutdown_source option is for Linux only, and the keep_AIX_target_running option is for AIX only
			//       so they are mutually exclusive and neither of them is mandatory
		    if( !shutdown_source && !keep_AIX_target_running )
			{
		        sprintf(cmdline, "%s/dtcfailover -g%d -r -b -q 1>/dev/null 2> /dev/null", PATH_BIN_FILES, lgnum);
		    }
			else
			{
			    // Coming here means that one (and only one) of shutdown_source and keep_AIX_target_running is true
			    if( shutdown_source )
				{
			        sprintf(cmdline, "%s/dtcfailover -g%d -r -b -q -s 1>/dev/null 2> /dev/null", PATH_BIN_FILES, lgnum);
				}
				if( keep_AIX_target_running )
				{
			        sprintf(cmdline, "%s/dtcfailover -g%d -r -b -q -k 1>/dev/null 2> /dev/null", PATH_BIN_FILES, lgnum);
			    }
			}
		}
		else  // Not boot drive migration
		{
		    sprintf(cmdline, "%s/dtcfailover -g%d -r -q 1>/dev/null 2> /dev/null", PATH_BIN_FILES, lgnum);
		}
		rc = system(cmdline);
        if( rc != 0 )
		{
            reporterr(ERRFAC, M_TARGET_FO_ERR, ERRCRIT, lgnum, rc);
		}
        ack.data = (u_long)rc;
		break;
    default:
        geterrmsg (ERRFAC, M_BADCMD, fmt);
        sprintf (msg, fmt, errno, header32->devid, header32->cmd);
        logerrmsg (ERRFAC, ERRWARN, M_BADCMD, msg); 
        memset (header, 0, sizeof (header));
        converthdrfrom32to64 (header32, header);
        (void) senderr (fd, header, 0L, ERRWARN, M_BADCMD, msg);
        return (-1);
    }

    /* -- return the RMD protocol version number for the 1st packet exchange */
    if (header32->cmd == CMDVERSION) {
        ack.data = strlen (rmdversionstring);
    } else {
        ack.data = 0L;
    }
    /* -- see if ack is requested for this packet, if so, send it */
    if (header32->ackwanted) {
        memset (header, 0, sizeof (header));
        rmd64 = 0;
        converthdrfrom32to64 (header32, header);
        response = sendack (fd, header, &ack);
        if (response != 1) return (response);
        if (header32->cmd == CMDVERSION) {
            if (!isconnected) return (1);
            if (fd != sock) return (1);
            if (-1 == writesock(fd, rmdversionstring, strlen(rmdversionstring))) 
                return (-1);
        }
    }
    if( header32->cmd == CMDFAILOVER )
    {
	    return(1);
	}
    if (header32->cmd == CMDVERSION) {
        int lgnum;
        /* -- set up for exec'ing the RMD by finishing setting private
           pmdenvironment variables, private argv list, and then exec'ing */
  
        if (_rmd_configpath == (char*)NULL || strlen(_rmd_configpath) == 0) {
            sprintf (tbuf, "_RMD_CONFIGPATH=in.rmd");
            lgnum = -1;
        } else {
            sprintf (tbuf, "_RMD_CONFIGPATH=%s", _rmd_configpath);
            lgnum = cfgpathtonum(_rmd_configpath);
        }
        pmdenviron[pmdenvcnt++] = strdup(tbuf);

        pmdenviron[pmdenvcnt] = (char*)ftdmalloc(sizeof(char)*23);
        sprintf (pmdenviron[pmdenvcnt++], "_RMD_SOCKFD=%d", fd);
        pmdenviron[pmdenvcnt] = (char*)ftdmalloc((16+1)*sizeof(char));
        sprintf(pmdenviron[pmdenvcnt++], "_RMD_RPIPEFD=%d",
            r_cp[lgnum].fd[1][STDIN]);
        pmdenviron[pmdenvcnt] = (char*)ftdmalloc((16+1)*sizeof(char));
        sprintf(pmdenviron[pmdenvcnt++], "_RMD_WPIPEFD=%d",
            r_cp[lgnum].fd[0][STDOUT]);
        pmdenviron[pmdenvcnt] = (char*)ftdmalloc((13)*sizeof(char));
        sprintf(pmdenviron[pmdenvcnt++], "_RMD_JLESS=%d",
            version.jless);
        pmdenviron[pmdenvcnt] = (char*)ftdmalloc(13*sizeof(char));
        sprintf(pmdenviron[pmdenvcnt++], "_RMD_PMD64=%d", pmd64);
        pmdenviron[pmdenvcnt] = (char*)ftdmalloc(13*sizeof(char));
        sprintf(pmdenviron[pmdenvcnt++], "_RMD_RMD64=%d", rmd64);
        if (log_internal)
            pmdenviron[pmdenvcnt++] = strdup(LOGINTERNAL "=1");
        if (save_journals)
            pmdenviron[pmdenvcnt++] = strdup(SAVEJOURNALS "=1");
        pmdenviron[pmdenvcnt] = (char*)NULL;

        childargv = (char**)ftdmalloc(sizeof(char*)*2);
        childargv[0] = (char*)ftdmalloc(4+strlen(_rmd_configpath));
        sprintf (childargv[0], "RMD_%s", &(_rmd_configpath[1]));

        /* kill it if it's currently running */
        if ((pid = getprocessid(childargv[0], 1, &response)) > 0)
        {
		    // Send a signal to the RMD to do whatever cleanup is needed
		    // and to exit after (WR PROD7916)
            kill(pid, SIGQUIT);
			sleep( 1 );

			// Check that the RMD does exit (timeout is 10 seconds).
			// It would be abnormal that the timeout be reached since signals
			// are interrupts, but we put a safety check here.
			for( i = 0; i < 5; i++ )
			{
			    if( (pid = getprocessid(childargv[0], 1, &response)) > 0 )
				{
				    // RMD still there
					sleep( 2 );
				}
				else
				{
				    break;
				}
			}
			if( pid > 0 )
			{
			    // Timeout reached on RMD exit; send the SIGKILL signal
                reporterr(ERRFAC, M_RMD_QUIT_TMOUT, ERRWARN);
				kill(pid, SIGKILL);
			}
        }

        childargv[1] = (char*)NULL;
        childargc = 1;

#ifdef TDMF_TRACE
        for (i = 0; i < pmdenvcnt; i++) {
            if (pmdenviron[i] != NULL) {
                ftd_trace_flow(FTD_DBG_FLOW1, "%s\n", pmdenviron[i]);
            }
        }
#endif

        ch_rundir("rmd", lgnum);
        (void)execve(RMD_PATH, childargv, pmdenviron);

        reporterr(ERRFAC, M_EXEC, ERRCRIT, childargv[0], strerror(errno));
        return -1;
    }
    return 1;
} /* readfirstitem */

/****************************************************************************
 * acceptconnect -- Wait for a client to attach to this server
 ***************************************************************************/
void
acceptconnect (void)
{
    ackpack_t ipc_ack[1];
    headpack32_t *header32;
    struct sockaddr_in myaddr;
    struct sockaddr_in client_addr;
#if !defined(FTD_IPV4)
    struct sockaddr_storage ss;
    struct addrinfo *ai, *res;
    struct addrinfo hints;
    int ret = -1;
    int retval = -1;
    char tmpport[16] ;
#endif 
    pid_t pid;
    fd_set read_set[1];
    struct servent *port;
    unsigned char cmd[256];
    char tcpwinsize[32];
    int recfd;
    int socket_fd;
#if defined(_AIX) || defined(linux)
    size_t length;
#else  /* defined(_AIX) */
    int length;
#endif /* defined(_AIX) */
    int len;
    int nselect;
    int pmdrequest;
    int lgnum = -1;
    int cnt;
    int sig;
    int rc;
    int n;
    int i, j;
    int nooftries=0;
    int fd;
    int family;
    unsigned int fdtype_selector=0;
    int firstlg, lastlg;
    int fdselect;
	int read_RMD_config_file;
    char bad_data_err_msg[100];

    pid = -1;
    pidcnt = 0;
    rmd_pidcnt = 0;

    /* create control socket for use by ftd utilities to peform operations
       such as launching pmds, refresh, backfresh, etc. */
    if ((ftdsrvsock = createftdsock()) == -1) {
        EXIT(EXITANDDIE);
    }
    if ((rmdsrvsock = creatermdsock()) == -1) {
        EXIT(EXITANDDIE);
    }
    if ((port = getservbyname(FTDD, "tcp"))) {
        listenport = ntohs(port->s_port);
    } else {
        listenport = (int) SERVER_PORT;
    }
    skosh.tv_sec = 10;
    skosh.tv_usec = 0;
   
    /* create main IPv4 tcp socket for listening for pmd connections and 
       status information queries */
    
#if defined(FTD_IPV4)
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        if (errno == EMFILE) {
            reporterr (ERRFAC, M_FILE, ERRCRIT, "", strerror(errno));
        } else
            reporterr (ERRFAC, M_SOCKFAIL, ERRCRIT, strerror(errno));
        EXIT(EXITNORMAL);
    }
    n = 1;

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&n,
        sizeof(int)) < 0) {
        reporterr (ERRFAC, M_SOCKOP, ERRCRIT, strerror(errno));
        EXIT(EXITNORMAL);
    }
    memset((char*)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(listenport);

    len = sizeof(myaddr);
    if (bind(socket_fd, (struct sockaddr*)&myaddr, len) < 0) {
        reporterr (ERRFAC, M_BINDERR, ERRCRIT, listenport);
        EXIT(EXITNORMAL);
    }
#else
    /* create main IPv6 tcp socket for listening for pmd connections and
       status information queries */
    memset(&hints, '\0', sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(tmpport, sizeof(tmpport),"%d", listenport);
    ret = getaddrinfo(NULL, tmpport, &hints, &ai);
    if  (ret != 0)
       reporterr(ERRFAC, M_SOCKFAIL, ERRCRIT, gai_strerror(ret));
    res = ai;
#if defined(_AIX) || defined(linux)
    while (res)
    {
      if (res->ai_family == AF_INET6){
           socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
         if (!(socket_fd < 0))
            {
              if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(int)) == 0) {
                 if (bind(socket_fd, res->ai_addr, res->ai_addrlen) == 0)
                  {
                    retval = 0;
                    break;
              }
           }
        }
      }
      res = res->ai_next;
    }

    if (res == NULL){
      res = ai;
      while (res) {
        if (res->ai_family == AF_INET){
          socket_fd = socket(AF_INET, res->ai_socktype, res->ai_protocol);
           if (!(socket_fd < 0)) {
             if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(int)) == 0) {
                if (bind(socket_fd, res->ai_addr, res->ai_addrlen) == 0) {
                    retval = 0;
                    break;
                  }
               }
            }
         }
         res = res->ai_next;
      }
    }
#else 
    while (res)
    {
     socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
     if (!(socket_fd < 0))
     {
       if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(int)) == 0) {
          if (bind(socket_fd, res->ai_addr, res->ai_addrlen) == 0)
          {
                retval = 0;
                break;
          }
        }
     }
     res = res->ai_next;
   }
#endif
   freeaddrinfo(ai);
   if (retval == -1)
   {
         reporterr (ERRFAC, M_BINDERR, ERRCRIT, listenport);
         EXIT(EXITNORMAL);
         close(socket_fd);
   }

   n = 1;
#endif /* defined(FTD_IPV4) */

    /* set the TCP window from the value in the ftd.conf file */
    if ((cfg_get_key_value( "tcp_window_size", tcpwinsize, CFG_IS_NOT_STRINGVAL)) == CFG_OK)
    {
        n = atoi(tcpwinsize);
    }
    else
    {
        /* wasn't there, just default to the default value */
        n = DEFAULT_TCP_WINDOW_SIZE;
    }

#ifdef _AIX /*TCP_RFC1323 support */
    //   The TCP_RFC1323 option is only available on AIX.
    //  We'll only enable the RFC1323 improvements if we request a buffer larger than 64k and know we'll need the window scale option.
    if (n > (64*1024) )
    {
        int on = 1;
        setsockopt(socket_fd, IPPROTO_TCP, TCP_RFC1323, (char*)&on, sizeof(on));
    }
#endif /* TCP_RFC1323 */

    if (n > 0)
    {
        setsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, (char*)&n, sizeof(int));
        setsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF, (char*)&n, sizeof(int));
    }
        
    if (listen(socket_fd, 5) < 0) {
        reporterr (ERRFAC, M_LISTEN, ERRCRIT);
        EXIT(EXITNORMAL);
    }
    while (1) {
        /* get primary config paths */
        lgcnt = GETCONFIGS(configpaths, 1, 1);
        paths = (char*)configpaths;

        FD_ZERO(read_set);
        FD_SET(sigpipe[0], read_set);
        FD_SET(ftdsrvsock, read_set);
        FD_SET(rmdsrvsock, read_set);
        FD_SET(socket_fd, read_set);

        nselect = ftdsrvsock > rmdsrvsock ? ftdsrvsock: rmdsrvsock;
        nselect = socket_fd > nselect ? socket_fd: nselect;
        nselect = sigpipe[0] > nselect ? sigpipe[0]: nselect;

	firstlg = -1;
	lastlg  = -1;
        /* listen on child pipes too */
        for (i = 0; i < MAXLG; i++) {
            if (p_cp[i].fd[0][STDIN] == -1) {
                continue;
            }
	    if (firstlg < 0) firstlg = i;
	    lastlg = i + 1;
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
        if (n != -1 && n != 0) {
	    fdselect = -1;
	    for (j = 0; j < 5 && fdselect < 0; j++)
	    {
		switch ((fdtype_selector + j) % 5)
		{
		    case 0:
			if (FD_ISSET(ftdsrvsock, read_set)) fdselect = 0;
			break;
		    case 1:
			if (FD_ISSET(rmdsrvsock, read_set)) fdselect = 1;
			break;
		    case 2:
			if (FD_ISSET(sigpipe[0], read_set)) fdselect = 2;
			break;
		    case 3:
			if (FD_ISSET(socket_fd, read_set)) fdselect = 3;
			break;
		    case 4:
			for (i = firstlg; i < lastlg; i++)
			{
			    if (p_cp[i].fd[0][STDIN] == -1) {
				continue;
			    }
			    if (FD_ISSET(p_cp[i].fd[0][STDIN], read_set)) {
				fdselect = 4;
				break;
			    }
			}
			break;
		}
	    }

	    DPRINTF("\n*** fdselect = %d fdtype_selector = %d\n", fdselect, fdtype_selector);
	    switch (fdselect)
	    {
	      case -1:
		DPRINTF("\n*** fdselect did not get a value\n" );
		break;
              case 0: /* (FD_ISSET(ftdsrvsock, read_set) */
                /* ftd utility is trying to connect with us */
                if ((ftdsock = getftdsock()) == -1) {
                    goto exit_func;
                }
                /* process the message  */
                if ((sig = ipcrecv_sig(ftdsock, &cnt)) > 0) {
                    dispatch_signal(sig, cnt);
                }
		break;
              case 1: /* (FD_ISSET(rmdsrvsock, read_set) */
                /* rmd is trying to connect with us */
                if ((rmdsock = getrmdsock()) == -1) {
                    goto exit_func;
                }
                /* process the message  */
                if ((sig = ipcrecv_sig(rmdsock, &cnt)) > 0) {
                    dispatch_signal(sig, cnt);
                }
		break;
              case 2: /* (FD_ISSET(sigpipe[0], read_set) */
                /* handle signal */
                if (read(sigpipe[0], &sig, sizeof(sig)) == sizeof(sig)) {
                dispatch_signal(sig, 0);
                }
		break;
              case 3: /* (FD_ISSET(socket_fd, read_set) */
                /* a pmd is attempting to connect with us, or someone is attempting
                   to query device information from us */
#if defined(FTD_IPV4)
                   length = sizeof(client_addr);
                   #define client client_addr
#else
                   length = sizeof(ss);
                   #define client ss
#endif /* defined(FTD_IPV4)*/
                /* accept the connection */  
                 if ((recfd=accept(socket_fd, 
                    (struct sockaddr*)&client, &length)) < 0)
                        {
                    reporterr (ERRFAC, M_ACCEPT, ERRCRIT, errno, strerror(errno));
                    /* Error handling for ENOBUF problem */
                    if (errno == ENOBUFS && nooftries++ < 3) {
                        usleep(100000);
                        continue;
                    } else
                        EXIT(EXITNORMAL);
                } else
                    nooftries=0;
                /* read 1st four bytes */
                memset(cmd, 0, sizeof(cmd));
                pmdrequest = readfirst_fourbytes(recfd, cmd);
                if (pmdrequest == -1) {
                    close(recfd);
                    continue;
                } else if (pmdrequest == 1) {
                    /* read rest of header */
                    /* We already read the first four bytes above, read in the rest to
                       fill in the header structure */
                    if (-1 == (rc = readsock(recfd, (char*)&cmd[4], sizeof(headpack32_t)-4))) {
                        close(recfd);
                        continue;
                    }
                    header32 = (headpack32_t*)cmd;
                    /* WR 41856: validate header */
                    if( header32->magicvalue != MAGICHDR )
                    {
                        reporterr (ERRFAC, M_ACCEPT, ERRWARN, 0,
                                   "unexpected data received on dtc master daemon port (port scanning would be a possible cause).\n"); /* WR PROD6036 */
                        close(recfd);
                        continue;
                    }
                    /* get secondary config paths */
                    lgcnt = GETCONFIGS(configpaths, 2, 0);
                    paths = (char*)configpaths;
					if( header32->cmd == CMDFAILOVER )
					{
					    // The command CMDFAILOVER uses bits of the data field as flags; save them and clear them
						boot_drive_migration = header32->data & CMDFO_BOOT_DRIVE;
						header32->data &= ~CMDFO_BOOT_DRIVE;
						shutdown_source = header32->data & CMDFO_SHUTDOWN_SOURCE;
						header32->data &= ~CMDFO_SHUTDOWN_SOURCE;
						keep_AIX_target_running = header32->data & CMDFO_KEEP_AIX_TARGET_RUNNING;
						header32->data &= ~CMDFO_KEEP_AIX_TARGET_RUNNING;
					}
                    read_RMD_config_file = 0;
					if( header32->cmd == CMDVERSION )
					{
					    // The command CMDVERSION uses bit 16 of the data field as a flag indicating if a config 
					    // file is appended to the command for the RMD; save it and clear it. The presence of this file
						// indicates that the RMD will be launched in network bandwidth analysis mode (Full Refresh
						// simulation).
						read_RMD_config_file = header32->data & CMDVER_READ_RMD_CFG_FILE;
						header32->data &= ~CMDVER_READ_RMD_CFG_FILE;
					}
                    lgnum = header32->data;
					rmds_in_net_analysis_mode[lgnum] = read_RMD_config_file;

                    /* WR 41856: validate lgnum */
                    if( (lgnum < 0) || (lgnum >= MAXLG) )
                    {
                        sprintf( bad_data_err_msg, "Invalid group number: 0x%x; cmd field: 0x%x\n", lgnum, header32->cmd );
                        reporterr (ERRFAC, M_ACCEPT, ERRWARN, 0, bad_data_err_msg);
                        close(recfd);
                        continue;
                    }
                    close_pipe_set(lgnum, 0);
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
                    EXIT(EXITNORMAL);
                case 0:
                    /* child - RMD */
                    /* close unused pipe descriptors */
                     /* if (pmdrequest) { */
                    close_pipes(lgnum, 0);
                    /* } */
                    close(socket_fd);
                    if (setsockopt(recfd, SOL_SOCKET, SO_KEEPALIVE,(char*)&n,sizeof(int))) {
                        reporterr (ERRFAC, M_SOCKOP, ERRCRIT, recfd, strerror(errno));
                        close(recfd);
                        EXIT(EXITNORMAL);
                    }
                    /* read 1st message, and decide if we need to spawn a child rmd or
                       satisfy some other request */
                    if (-1 == readfirstitem(recfd, pmdrequest, cmd, read_RMD_config_file)) {
                        EXIT(-1);
                    }
                     
                    /* if we get here then we were not an RMD */
                    close(recfd);
                    EXIT(EXITNORMAL);
                default:
                    /* parent */
                    close(recfd);
                    if (pmdrequest) {
                        if (lgnum != -1) {
                            /* close unused pipe descriptors */
                            if (r_cp[lgnum].fd[0][STDOUT] != -1) {
                              close(r_cp[lgnum].fd[0][STDOUT]);
                              r_cp[lgnum].fd[0][STDOUT] = -1;
                            }
                            if (r_cp[lgnum].fd[1][STDIN] != -1) {
                              close(r_cp[lgnum].fd[1][STDIN]);
                              r_cp[lgnum].fd[1][STDIN] = -1;
                              /* mod by PST '02/12/16 for WR15335,WR15337*/ 
                              /* r_cp[lgnum].fd[0][STDIN] = -1;          */
                              /*******************************************/
                            }
                            r_cp[lgnum].pid = pid;
			    pid_hash_add( pid, lgnum, LGARRY_RMD );
                        }
                        rmd_pidcnt++;
                    }
                    pidcnt++;
                }                                   
		break;
              case 4:  /* (FD_ISSET(p_cp[i].fd[0][STDIN], read_set) */
                /* check child communication pipes */
                for (i = firstlg; i < lastlg; i++) {
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
	fdtype_selector++;
    }
exit_func: ;
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
        rm_rundir(pcwd, QNM, -1, getpid());
        EXIT(EXITNORMAL);
    case SIGALRM:
        return;
    case SIGUSR2:
        /*
         * Toggle M_INTERNAL logging
         */
        log_internal = log_internal ? 0 : 1;
        break;
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
    int numsig, val;
    struct sigaction msaction;
    sigset_t block_mask;
    static int msignal[] = { SIGALRM, SIGHUP, SIGCHLD, SIGTERM, SIGUSR2 };

    if (pipe(sigpipe) < 0) {
        reporterr(ERRFAC, M_PIPE, ERRWARN, strerror(errno));
    }
        /* set write end of sigpipe to non-blocking */
        val = fcntl(sigpipe[STDOUT_FILENO], F_GETFL, 0);
        fcntl(sigpipe[STDOUT_FILENO], F_SETFL, val | O_NONBLOCK);
        
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
    printf ("  %s -log_internal\n", argv0);
    printf ("  %s -save_journals\n", argv0);
    printf ("  %s -help\n", argv0);

    EXIT(EXITNORMAL);
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
      EXIT(EXITNORMAL);
    } else if (!strcmp("-rundir", argv[i])) {
        if ((i + 1) < argc && argv[i + 1][0] != '-') {
            pcwd = argv[i + 1];
            i++;
        }
    } else if (!strcmp("-save_journals", argv[i])) {
        save_journals = 1;
    } else if (!strcmp("-log_internal", argv[i])) {
        log_internal = 1;
    } else {
      usage(argc, argv);
    }
    i++;
  }
} /* processargs */
 
/****************************************************************************
 * processpmdenviron -- process pmdenvironment variables
 ***************************************************************************/
void 
processpmdenviron (char** envp) 
{
    /* count pmdenvironment variables */
    pmdenvcnt = 0;
    while (envp[pmdenvcnt]) {
        ftd_trace_flow(FTD_DBG_FLOW1, "%s\n", envp[pmdenvcnt]);
        pmdenvcnt++;                
    }
    return;
} /* processpmdenviron */

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
        EXIT(EXITNORMAL);
    case 0:
        throtargv = (char **) ftdmalloc(2*sizeof(char*));
        throtargv[0] = (char*) ftdmalloc (strlen("throtd")+1);
        throtargv[1] = (char*)NULL;
        strcpy(throtargv[0], "throtd");
        
        execv(PATH_BIN_FILES "/throtd", throtargv);
        reporterr(ERRFAC, M_EXEC, ERRCRIT, PATH_BIN_FILES "/throtd", strerror(errno));
        EXIT(EXITANDDIE);
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
        EXIT(EXITNORMAL);
    }
    setsid();
    chdir("/");
    umask(0);

    return 0;
} /* daemon_init */

/* wr15892 */
#if defined(HPUX) && !defined(DLKM)
/****************************************************************************
 * check_sf --  check dtc special file
 ***************************************************************************/
void
check_sf(void)
{
    struct stat statbuf;
    FILE *fp;
    char cmd[128];
    char data[128];
    char drvname[64];
    char drvinfo[64];

    /* ctl file check */
    if (stat(FTD_CTLDEV, &statbuf) == 0) {
        /* file exist */
        return;
    }
    
    /* file not found. driver install check */
    fp = popen("/usr/sbin/lsdev | /bin/grep dtc | awk '{print $3,  $4}'","r");
    if (fp == NULL)
    {
        /* lsdev exec error */
        return;
    }
    else
    {
        for ( ; fgets(data, sizeof(data), fp) != 0 ;)
        {
            sscanf(data,"%s %s",drvname,drvinfo);
            if ((strcmp(drvname,"dtc") == 0) &&
                (strcmp(drvinfo,"pseudo") == 0))
            {
                /* dtc driver install  */        
                /* create special file */
                sprintf(cmd, "/etc/mksf -d %s -m 0xffffff -r %s",
                                                           QNM,FTD_CTLDEV);
                system(cmd);
                break;
            }

        }
        pclose(fp);
    }
    return;

}
#endif
/* wr15892 */

/****************************************************************************
 * main
 *     - process command line arguments, pmdenvironment variables
 *     - install signal handler 
 *     - create man control server socket 
 *     - accept connections forever
 ***************************************************************************/
int
main (int argc, char** argv, char **envp)
{
    pid_t pid;
    int pcnt; 
    int i;
    char cwd[PATH_MAX + 1];
    pcwd = getcwd(cwd, sizeof(cwd) - 1);

    FTD_TIME_STAMP(FTD_DBG_FLOW1, "%s start\n", argv[0]);
    putenv("LANG=C");

    argv0 = strdup(argv[0]);
/*
    {
       volatile int f=1;
       while(f);
    }
*/
    /* reset open file desc limit */
    setmaxfiles();

    if (initerrmgt(ERRFAC) < 0) {
        EXIT(EXITANDDIE);
    }

    reporterr(ERRFAC, M_INTERNAL, ERRINFO, FTDD " started");
    log_command(argc, argv);  /* trace command line in dtcerror.log */

#if defined(SOLARIS) /* 1LG-ManyDEV */     
    if ((dummyfd_mnt = open("/dev/null", O_RDONLY)) == -1) {
        reporterr(ERRFAC, M_FILE, ERRWARN, "/dev/null", strerror(errno));
    }
#endif
    /* -- Make sure we're not already running -- */
    if ((pid = getprocessid(FTDD, 0, &pcnt)) > 0) {
        if (pid != getpid() || pcnt > 1) {
            reporterr(ERRFAC, M_RUNAGAIN, ERRWARN, FTDD);
            EXIT(EXITNORMAL);
        }
    } 
    /* -- Make sure we're root -- */
    if (geteuid()) {
        reporterr (ERRFAC, M_NOTROOT, ERRCRIT);
        EXIT(EXITANDDIE);
    }
    /* init global structures not used in master and some local flags */
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
		pmds_in_net_analysis_mode[i] = 0;
		rmds_in_net_analysis_mode[i] = 0;
    }
    rcnt = restartcnts;

    /* process pmdenvironment */
    processpmdenviron (envp);

    pmdenviron = (char**)ftdcalloc((pmdenvcnt+10), sizeof(char*));
    i = 0;
    while (envp[i]) {
        pmdenviron[i] = strdup(envp[i]);
        i++;        
    }
    pmdenviron[i] = (char*)NULL;
    pmdargv = (char**)ftdcalloc(2, sizeof(char**));

    for (i = 0; i < (sizeof(rmda_pid) / sizeof(rmda_pid[0])); i++)
        rmda_pid[i] = (pid_t)(-1);

    /* install signal handler */
    installsigaction();

/* wr15892 */
#if defined(HPUX) && !defined(DLKM)
    check_sf();
#endif
/* wr15892 */

    /* get cmd line args */
    processargs (argc, argv);

    /* make this a daemon */
    daemon_init();
    setsyslog_prefix("%s", argv0);
    if(!pcwd) pcwd = getenv("PWD");
    if (pcwd && open_rundir(pcwd) >= 0)
        ch_rundir(QNM, -1);

    /* set group id */
    setpgid(getpid(),getpid());

    /* Fire off throtd */
    (void)throtdstart();

    /* Wait for connection */
    acceptconnect();
 
    close_rundir();
    EXIT(EXITNORMAL);
    return 0; // To avoid a compiler warning.
} /* main */
