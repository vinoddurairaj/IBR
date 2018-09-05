/***************************************************************************
 * pmd.c - FullTime Data Primary Mirroring Daemon
 *
 * (c) Copyright 1996, 1997, 1998 FullTime Software, Inc.
 *     All Rights Reserved
 *
 * This module implements the high level functionality for the PMD
 *
 * History:
 *   10/14/96 - Steve Wahl - original code
 *
 ***************************************************************************/
#ifdef NEED_BIGINTS
#include "bigints.h"
#endif

#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/param.h>

#if defined(SOLARIS)
#include <sys/procfs.h>
#elif defined(HPUX)
#include <sys/pstat.h>
#endif

#include <stdio.h>

#ifdef NEED_SYS_MNTTAB
#include "ftd_mnttab.h"
#else
#include <sys/mnttab.h>
#endif


#include <fcntl.h>
#include "ftdio.h"
#include "errors.h"
#include "config.h"
#include "network.h"
#include "license.h"
#include "licprod.h"
#include "pathnames.h"
#include "common.h"
#include "process.h"
#include "ipc.h"

#ifdef DEBUG
FILE *dbgfd;
#endif

char *argv0;

int _pmd_retryidx = -1;
int _pmd_reconwait = 5;

extern int _pmd_hup;

static struct timeval skosh;
static char configpaths[MAXLG][32];

static sigset_t sigs;

extern char *paths;
extern int exitsuccess;
extern int exitfail;
extern char *_pmd_configpath;
extern int _pmd_state;
extern int _pmd_state_change;
extern int _pmd_rpipefd;
extern int _pmd_wpipefd;
extern int _pmd_restart;

extern int _pmd_cpstart;
extern int _pmd_cpstop;
extern int _pmd_cpon;
extern int _pmd_verify_secondary;

extern int refresh_started;

extern int refresh(void*);
extern int refresh_full(void*);
extern int backfresh(void*);
extern int dispatch(void*);
extern void *process_acks(void);

static void installsigaction(void);
static void sig_handler(int);

/*
 * We need to restart when we get a SIGPIPE on the connection.  Since
 * we have just one connection, we can do this by installing SIGPIPE
 * in the children and then just exiting when it fires.  This will
 * give the retry code the appearance of a recoverable action, which
 * is exactly what we want.
 */
/****************************************************************************
 * sig_handler -- PMD signal handler
 ***************************************************************************/
static void
sig_handler(int s)
{
    int lgnum;

    switch(s) {
    case SIGUSR1:
        /* -- stop the daemon - kill the state */
        if (_pmd_state == FTDRFD || _pmd_state == FTDRFDF) {
            reporterr(ERRFAC, M_RFDTERM, ERRINFO, argv0);
        } else if (_pmd_state == FTDBFD) {
            reporterr(ERRFAC, M_BFDTERM, ERRINFO, argv0);
        }
        initdvrstats();
        exit(EXITANDDIE);
    case SIGTERM:
        /* -- stop the daemon - don't kill the state - unless it's NORMAL */
        lgnum = atoi(_pmd_configpath+strlen(_pmd_configpath)-3);
        if (_pmd_state == FTDPMD) {
            clearmode(lgnum);
        }
        reporterr(ERRFAC, M_PMDEXIT, ERRINFO, argv0);
        initdvrstats();
        exit(EXITANDDIE);
    case SIGPIPE:
        reporterr(ERRFAC, M_NETBROKE, ERRWARN, argv0, othersys->name);
        exit (EXITNETWORK);
    default:
        /* error - do nothing */
        break;
    }
    return;
} /* sig_handler */

/****************************************************************************
 * installsigaction -- install signal actions for PMD 
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
        caction.sa_handler = sig_handler;
        caction.sa_mask = block_mask;
        caction.sa_flags = SA_RESTART;
        sigaction(csignal[i], &caction, NULL);
    } 
    return;
} /* installsigaction */

/****************************************************************************
 * processenviron -- process environment variables
 ***************************************************************************/
void 
processenviron (char** envp) 
{
    int len;
    int pmdenvcnt;
	
    pmdenvcnt = 0;
    while (envp[pmdenvcnt]) {
        if (0 == strncmp(envp[pmdenvcnt], "_PMD_", 5)) {
            if (0 == strncmp(envp[pmdenvcnt], "_PMD_CONFIGPATH=", 16)) { 
                len = (strlen(envp[pmdenvcnt])-15)*sizeof(char);
                _pmd_configpath = (char*) ftdmalloc(len);
                sscanf(&(envp[pmdenvcnt])[16], "%s", _pmd_configpath);
            } else if (0 == strncmp(envp[pmdenvcnt], "_PMD_STATE=", 11)) { 
                sscanf(&(envp[pmdenvcnt])[11], "%d", &_pmd_state);
            } else if (0 == strncmp(envp[pmdenvcnt], "_PMD_RETRYIDX=", 14)) { 
                sscanf(&(envp[pmdenvcnt])[14], "%d", &_pmd_retryidx);
            } else if (0 == strncmp(envp[pmdenvcnt], "_PMD_RESTART=", 13)) { 
                sscanf(&(envp[pmdenvcnt])[13], "%d", &_pmd_restart);
            } else if (0 == strncmp(envp[pmdenvcnt], "_PMD_RECONWAIT=", 15)) { 
                sscanf(&(envp[pmdenvcnt])[15], "%d", &_pmd_reconwait);
            } else if (0 == strncmp(envp[pmdenvcnt], "_PMD_WPIPEFD=", 13)) { 
                sscanf(&(envp[pmdenvcnt])[13], "%d", &_pmd_wpipefd);
            } else if (0 == strncmp(envp[pmdenvcnt], "_PMD_RPIPEFD=", 13)) { 
                sscanf(&(envp[pmdenvcnt])[13], "%d", &_pmd_rpipefd);
            }
        }
        pmdenvcnt++;
    }
    return;
} /* processenviron */

/****************************************************************************
 * re_init -- re-initialize state 
 ***************************************************************************/
int
re_init (int lgnum, char *cfgpath)
{
    sddisk_t *sd;
    int savesock;
    int saveconnect;
    int rc;

    savesock = mysys->sock;
    saveconnect = mysys->isconnected;

    closedevs(-1);
    /* re-read the config file and pstore */
    if ((rc = readconfig(1, 1, 0, cfgpath)) != 0) {
        exit(EXITANDDIE);
    }
    if (opendevs(lgnum) == -1) {
        exit(EXITANDDIE);
    }
    mysys->sock = savesock;
    mysys->isconnected = saveconnect;

    return 0;
    
} /* re_init */

/****************************************************************************
 * main -- PMD
 ***************************************************************************/
int
main (int argc, char** argv, char **environ)
{
    char path[FILE_PATH_LEN];
    char **lickey;
    int firsttry;
    int result;
    int lgnum;
    int tries;
    int cfgidx;
    int rc;

    void *voidptr = NULL;
    argv0 = strdup(argv[0]);

    if (initerrmgt(ERRFAC) < 0) { 
	exit (1);
    }

    /* init signal handler */
    installsigaction();

    if (!is_parent_master()) {
        reporterr(ERRFAC, M_NOMASTER, ERRCRIT, argv0);
        exit (EXITANDDIE);
    }
    processenviron(environ);

    getconfigs(configpaths, 1, 1);
    paths = (char*)configpaths;

    sprintf(path, "%s.cur", _pmd_configpath);
    lgnum = cfgpathtonum(path);
    cfgidx = numtocfg(lgnum);

    /* notify master - as soon as we have enough state info */
    tell_master(1, lgnum);

    skosh.tv_sec = 10;
    skosh.tv_usec = 0;

    /* -- perform license checking */
    if (LICKEY_OK != (result = get_license (PMDFLAG, &lickey))) {
        switch (result) {
        case LICKEY_FILEOPENERR:
            reporterr (ERRFAC, M_LICFILMIS, ERRCRIT, "DTC");
            break;
        case LICKEY_BADKEYWORD:
            reporterr (ERRFAC, M_LICBADWORD, ERRCRIT, "DTC");
            break;
        default:
            reporterr (ERRFAC, M_LICFILE, ERRCRIT, "DTC");
        }
        exit (EXITANDDIE);
    }
    if (lickey == NULL) {
        reporterr (ERRFAC, M_LICFILFMT, ERRCRIT, "DTC");
        exit (EXITANDDIE);
    }
    result = check_key(lickey, np_crypt_key_pmd, NP_CRYPT_KEY_LEN);
    if (LICKEY_OK != result) { 
        switch (result) {
        case LICKEY_NULL:
        case LICKEY_EMPTY:
            reporterr (ERRFAC, M_LICFILE, ERRCRIT, "DTC");
            break;
        default:
            reporterr (ERRFAC, M_LICGENERR, ERRCRIT);
        }
        exit (EXITANDDIE);
    }

    exitsuccess = exitfail = EXITRESTART;
    _pmd_verify_secondary = 0;

    /* _pmd_state is passed down to us from the master (in.ftd) */
    switch(_pmd_state) {
    case FTDBFD:
    case FTDRFD:
    case FTDRFDF:
        break;
    case FTDRFDC:
        _pmd_state = FTDRFD;
        _pmd_verify_secondary = 1;
        break;
    default:
        break;
    }

    if ((rc = readconfig(1, 1, 0, path)) != 0) {
        exit(EXITANDDIE);
    }
    if (opendevs(lgnum) == -1) {
        exit (EXITANDDIE);
    }
    if (0 != (rc = initnetwork ())) {
        exit(EXITANDDIE);
    }
    /* get actual BAB size */
    mysys->group->babsize = get_bab_size(mysys->ctlfd);

    _pmd_cpstart = 0;
    _pmd_cpstop = 0;
    _pmd_cpon = 0;
    _pmd_hup = 0;

    if (_pmd_retryidx >= 0) {
        /* this is a restart */
        /* -- sleep 'reconwait' seconds before reconnecting */
        (void)sleep(_pmd_reconwait);
    }
    tries = 0;
    firsttry = 1;
    while (1) {
        rc = 0;
        if (!(rc = createconnection (firsttry))) {
            break;
        }
        (void) select(0, NULL, NULL, NULL, &skosh);
        if (++tries == 90) {
            reporterr(ERRFAC, M_RECONN, ERRWARN);
            tries = 0;
        }
        firsttry = 0;
    }
    if (1 != (rc = sendversion (mysys->sock))) {
        exit(EXITANDDIE);
    }
    if (1 != (rc = sendhandshake (mysys->sock))) {
        exit(EXITANDDIE);
    }
    if (1 != (rc = sendconfiginfo (mysys->sock))) {
        exit(EXITANDDIE);
    }

    /* set default TCP low water mark */
    set_tcp_send_low();
    
    /* set socket to non-blocking */
    set_tcp_nonb();
    
    /* save signal mask */
    sigprocmask(0, NULL, &sigs);

    while (1) {
        if ((rc = flush_net(mysys->sock)) != 0) {
            reporterr(ERRFAC, M_FLUSHNET, ERRWARN);
            exit(EXITANDDIE); 
        }
        _pmd_state_change = 0;
        if (_pmd_hup) {
            re_init(lgnum, path);
            _pmd_hup = 0;
        }
        /* set group state in pstore/driver */
        rc = setmode(lgnum, _pmd_state);
        if (rc == -1) {
            exit(EXITANDDIE);
        } else if (rc == 1) {
            continue;
        }
        /* initialize statistics */
        if (savestats(1) == -1) {
            exit(EXITANDDIE);
        }
        switch(_pmd_state) {
        case FTDBFD:
            reporterr(ERRFAC, M_BFDSTART, ERRINFO, argv0);
            if ((rc = backfresh(voidptr)) == -1) {
                exit(EXITANDDIE);
            }
            _pmd_state = FTDPMD;
            break;
        case FTDRFD:
            reporterr(ERRFAC, M_RFDSTART, ERRINFO, argv0);
            if ((rc = refresh(voidptr)) == -1) {
                exit(EXITANDDIE);
            }
            refresh_started = 0;
/*
            _pmd_state = FTDPMD;
*/
            break;
        case FTDRFDF:
            reporterr(ERRFAC, M_RFDSTART, ERRINFO, argv0);
            if ((rc = refresh_full(voidptr)) == -1) {
                exit(EXITANDDIE);
            }
            refresh_started = 0;
/*
            _pmd_state = FTDPMD;
*/
            break;
        case FTDPMD:
            reporterr(ERRFAC, M_PMDSTART, ERRINFO, argv0);
            if ((rc = dispatch(voidptr)) == -1) {
                exit(EXITANDDIE);
            }
            break;
        default:
            break;
        }
        /* reset signal mask */
        sigprocmask(SIG_SETMASK, &sigs, NULL);
    }
} /* main */
