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
#include "ftd_mount.h"
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
#include "ftdif.h"
#include "stat_intr.h"

#ifdef TDMF_TRACE
FILE *dbgfd;
#endif

char *argv0;

int _pmd_retryidx = -1;
int _pmd_reconwait = 5;

#if defined(SOLARIS) /* 1LG-ManyDEV */ 
extern int dummyfd;
#endif
extern int _pmd_hup;

extern int check_commands_due_to_unclean_RMD_shutdown; /* WR PROD6443 */
extern int in_reboot_autostart_mode(int lgnum);        /* WR PROD6443 */

extern void adjust_FRefresh_offset_backward( double percentage_backward, int force_complete_restart );
extern void remove_reboot_autostart_file(int group);
extern int ftd_lg_merge_hrdbs(group_t* group, int lgnum);

static struct timeval skosh;
static char configpaths[MAXLG][32];

static sigset_t sigs;

extern char *paths;
extern const int exitfail;
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
extern int _net_bandwidth_analysis;

extern int refresh_started;
extern statestr_ts statestr[];

extern int refresh(int);
extern int refresh_full(int);
extern int backfresh(void);
extern int dispatch(void);

static void installsigaction(void);
static void sig_handler(int);

/**
 * @brief Calls ftd_lg_merge_hrdbs() if we are in the middle of a refresh.
 *
 * Meant to be registered with atexit() and used to ensure that
 * the ongoing refresh will be resumable regardless of the reason of why
 * it is aborted.
 */
static void merge_hrdbs_if_in_refresh(void)
{
    if (_pmd_state == FTDRFD && refresh_started)
    {
        int lgnum = atoi(_pmd_configpath+strlen(_pmd_configpath)-3);
        if (ftd_lg_merge_hrdbs(mysys->group, lgnum) != 0)
        {
            reporterr(ERRFAC, M_MERGE_FAILED, ERRWARN);
        }
    }
} 

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

        if( !_net_bandwidth_analysis )
            initdvrstats();
        EXIT(EXITNORMAL);
    case SIGTERM:
        /* -- stop the daemon - don't kill the state - unless it's NORMAL */
        lgnum = atoi(_pmd_configpath+strlen(_pmd_configpath)-3);
        if (_pmd_state == FTDPMD) {
            clearmode(lgnum, 0);
        }
        reporterr(ERRFAC, M_PMDEXIT, ERRINFO, argv0);

        if( !_net_bandwidth_analysis )
            initdvrstats();
        EXIT(EXITNORMAL);
    case SIGPIPE:
        reporterr(ERRFAC, M_NETBROKE, ERRWARN, argv0, othersys->name, "received SIGPIPE");

        // When detecting a SIGPIPE, it's important to make sure we'll recover using a smart refresh.
        // Before we detect that there's no one on the other end, it's possible that the PMD has already
        // drained the bab into a dead connection and migrated its data.
        // When that's the case, the contents of the BAB is lost and we need a refresh to recover.
        // C.F. WR PROD00006836.
        lgnum = atoi(_pmd_configpath+strlen(_pmd_configpath)-3);

        if( !_net_bandwidth_analysis )
            clearmode(lgnum, 0);

        /* WR PROD6443: we no longer call adjust_FRefresh_offset_backward(); if an unclean 
           Full Refresh interruption has occurred on the RMD (ex.: crash, power failure, etc) 
           this is now reported by the RMD upon initial handshake; if clean exit on RMD (not a crash),
           a Resumed Full Refresh is now permitted (launchrefresh -r), or a Checksum Refresh as well.
		*/
        EXIT(EXITNETWORK);
    case SIGUSR2:
        /*
         * Toggle M_INTERNAL logging
         */
        log_internal = log_internal ? 0 : 1;
        break;
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
    static int csignal[] = { SIGUSR1, SIGTERM, SIGPIPE, SIGUSR2 };

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
    int pmdenvcnt;
	pmdenvcnt = 0;	

    while (envp[pmdenvcnt]) {
        if (0 == strncmp(envp[pmdenvcnt], "_PMD_", 5)) {
            if (0 == strncmp(envp[pmdenvcnt], "_PMD_CONFIGPATH=", 16)) { 
                _pmd_configpath = strdup(envp[pmdenvcnt] + 16);
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
        ftd_trace_flow(FTD_DBG_FLOW1, "%s\n", envp[pmdenvcnt]);
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
        EXIT(EXITANDDIE);
    }
    if (opendevs(lgnum) == -1) {
        EXIT(EXITANDDIE);
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
    int i;
    char *p;
    ps_group_info_t group_info;
    char group_name[MAXPATHLEN];
    ftd_state_t lgstate;
    int saved_pmd_cpon;
    char pname[16];
    int addrflag;
	int smart_refresh_requested = 0;
    int doing_reboot_autostart = 0;
    int rmda_corrupted_journal_found = 0;
    struct stat    sb;
    group_t *group;
    
    /* This flag is used only in case of refresh_full() function to
     * identify the -r used in the launchrefresh command to restart
     * the full refresh from the place where it stopped last time
     * The refresh() and backfresh() function take this as dummy
     * variable */
    int fullrefresh_restart = 0;

    void *voidptr = NULL;

    FTD_TIME_STAMP(FTD_DBG_FLOW1, "%s start\n", argv[0]);
    if (ftd_debugFlag & FTD_DBG_FLOW1) {
        for (i = 0; i < argc; i++) {
            ftd_trace_flow(FTD_DBG_FLOW1, "%d/%d: %s\n", i, argc, argv[i]);
        }
    }

    putenv("LANG=C");

    argv0 = strdup(argv[0]);

    /* -- Make sure we're root -- */            /* WR16793 */
    if (geteuid() != 0) {                       /* WR16793 */
        EXIT(EXITANDDIE);                       /* WR16793 */				 
   	}                                           /* WR16793 */

    /* reset open file desc limit */ 
    setmaxfiles();
    if (initerrmgt(ERRFAC) < 0) { 
		EXIT(EXITANDDIE);
    }

    setsyslog_prefix("%s", argv[0]);
    p = getenv(LOGINTERNAL);
    if (p)
        log_internal = atoi(p);
    reporterr(ERRFAC, M_INTERNAL, ERRINFO, "pmd started");

    log_command(argc, argv);  /* trace command line in dtcerror.log */

	if( stat(SFTKDTC_SERVICES_DISABLE_FILE, &sb) == 0 )
	{
	    reporterr(ERRFAC, M_SFTK_DISABLED, ERRINFO, SFTKDTC_SERVICES_DISABLE_FILE);
        EXIT(EXITANDDIE);
	}

#if defined(SOLARIS)  /* 1LG-ManyDEV */ 
    /*Dummy open for gethostbyname  / gethostbyaddr */
    if ((dummyfd = open("/dev/null", O_RDONLY)) == -1) {
	    reporterr(ERRFAC, M_FILE, ERRWARN, "/dev/null", strerror(errno));
     }
#endif

    /* init signal handler */
    installsigaction();

    if (!is_parent_master()) {
        reporterr(ERRFAC, M_NOMASTER, ERRCRIT, argv0);
        EXIT(EXITANDDIE);
    }
    processenviron(environ);

    /* Check if the PMD is launched for network bandwidth analysis (simulating Full Refresh) */
	_net_bandwidth_analysis = (_pmd_state == FTDNETANALYSIS);

	if( !_net_bandwidth_analysis )
	{
        atexit(merge_hrdbs_if_in_refresh);
	}
    
    GETCONFIGS(configpaths, 1, 1);
    paths = (char*)configpaths;

    sprintf(path, "%s.cur", _pmd_configpath);
    lgnum = cfgpathtonum(path);
    cfgidx = numtocfg(lgnum);

    /* notify master - as soon as we have enough state info */
    tell_master(1, lgnum);

    skosh.tv_sec = 10;
    skosh.tv_usec = 0;

    /* -- perform license checking */
#if defined(HPUX) ||  defined(_AIX) || defined(linux)
    if (LICKEY_OK != (result = get_license (PMDFLAG, &lickey))) {
#elif defined(SOLARIS) /* 1LG-ManyDEV */
    if (LICKEY_OK != (result = get_license_manydev (PMDFLAG, &lickey))) {
#endif
        switch (result) {
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
        EXIT(EXITANDDIE);
    }

    if (lickey == NULL) {
        reporterr (ERRFAC, M_LICFILFMT, ERRCRIT, CAPQ);
        EXIT(EXITANDDIE);
    }
    result = check_key(lickey, np_crypt_key_pmd, NP_CRYPT_KEY_LEN);
    if (LICKEY_OK != result) { 
        switch (result) {
        case LICKEY_NULL:
        case LICKEY_EMPTY:
            reporterr (ERRFAC, M_LICFILE, ERRCRIT, CAPQ);
            break;
        default:
            reporterr (ERRFAC, M_LICGENERR, ERRCRIT, PATH_CONFIG "/" CAPQ ".lic");
        }
        EXIT(EXITANDDIE);
    }

    _pmd_verify_secondary = 0;

    /* _pmd_state is passed down to us from the master (in.ftd) */
    switch(_pmd_state) {
    case FTDRFD:
	/* Smart refresh requested; take note for checking against previous RMD crash in Full Refresh */
	    smart_refresh_requested = 1;
		break;
    case FTDBFD:
    case FTDRFDF:
        break;
	case FTDNETANALYSIS:
		_pmd_state = FTDRFDF;
		break;
    case FTDRFDC:
        _pmd_state = FTDRFD;
        _pmd_verify_secondary = 1;
        break;
    case FTDRFDR: /* FRF restart */
        _pmd_state = FTDRFDF;
        fullrefresh_restart = 1;
        break;
    default:
        break;
    }

    ftd_trace_flow(FTD_DBG_FLOW1, "_pmd_state(%d)\n", _pmd_state);

    if ((rc = readconfig(1, 1, 0, path)) != 0) {
        EXIT(EXITANDDIE);
    }

    showtunables(&mysys->tunables);
    SET_LG_JLESS(mysys, ((mysys->tunables.use_journal == 0) ? 1 : 0));

    FTD_CREATE_GROUP_NAME(group_name, lgnum);
    _pmd_cpon = 0;

	if( !_net_bandwidth_analysis )
	{
	    if ((rc = ps_get_group_info(mysys->pstore, group_name, &group_info)) != PS_OK) {
	        EXIT(EXITANDDIE);
	    }

	    _pmd_cpon = group_info.checkpoint;

	    ftd_trace_flow(FTD_DBG_FLOW15,
	                   "set mysys jless(%d) cpon(%d) ps_state(%d)\n", 
	                   GET_LG_JLESS(mysys), _pmd_cpon, group_info.state);

	    // WR3426 - Don't allow to a full Refresh request when checkpoint mode is on. 
	    //          Problem occured when journal space ran out of space and pmd was in tracking mode.
	    if ( (_pmd_cpon) && (_pmd_state == FTDRFDF))
	    {
	        reporterr(ERRFAC, M_INVALIDOP, ERRWARN,lgnum, statestr[_pmd_state].desc, "checkpoint is on. Need to exit from checkpoint mode before.");
	        EXIT(EXITANDDIE);
	    }
	    if (opendevs(lgnum) == -1) {
	        EXIT(EXITANDDIE);
	    }
	}

    if (0 != (rc = initnetwork ())) {
        EXIT(EXITANDDIE);
    }
    
    // WI_338550 December 2017, implementing RPO / RTT
    group = mysys->group;
    // WI_338550 December 2017, implementing RPO / RTT
    if (mysys->tunables.use_journal == 0)
    {
        mysys->group->QualityOfService.WriteOrderingMaintained = 0;
    }
    else
    {
        mysys->group->QualityOfService.WriteOrderingMaintained = 1;
    }
    
    /* get actual BAB size */
	if( !_net_bandwidth_analysis )
	    mysys->group->babsize = get_bab_size(mysys->ctlfd);
	else
	    mysys->group->babsize = 0;

    _pmd_cpstart = 0;
    _pmd_cpstop = 0;
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
#if defined(linux)
        skosh.tv_sec = 10;
        skosh.tv_usec = 0;
#endif
    }

	if( !_net_bandwidth_analysis )
	{
		/* Check if we are in autostart mode after reboot (WR PROD6443) */
		if( (doing_reboot_autostart = in_reboot_autostart_mode(lgnum)) )
		{
		    remove_reboot_autostart_file(lgnum);
	        if (get_pstore_mode(lgnum) == FTDRFDF)
	        {
			   /* Full Refresh was interrupted before reboot.
			      Reset Full Refresh completion offset to 0; this is done in case a Full Refresh was interrupted uncleanly
			      (by a shutdown (which would not be normal, though, having a user command a shutdown during a Full Refresh)
			      or a crash of the source server) and in case a NETBROKE would occur before the actual relaunch
			      of the Full Refresh; then the master daemon could relaunch the PMD in a Restartable Full Refresh mode, to 
			      make it resume at the offset where it was interrupted. A NETBROKE could occur for the first time in the initial
			      handshake with the RMD (just below here); that's why we rewind the offset here, before initial handshake.
			      If the cause of the reboot was a hard crash on the source server, we don't know what offset was recorded 
			      last in the pstore and what we want here is a complete Full Refresh. We don't want the master daemon to 
			      make the PMD do a Restartable Full Refresh at the last offset left in the pstore at time of a crash.
			   */
	           adjust_FRefresh_offset_backward( FREFRESH_BACKWARD_ADJUSTMENT, 1 );
			}
		}
	}

    if (1 != (rc = sendversion (mysys->sock, _net_bandwidth_analysis))) {
        EXIT(EXITANDDIE);
    }

    saved_pmd_cpon = _pmd_cpon;

    if (1 != (rc = sendhandshake (mysys->sock, &rmda_corrupted_journal_found))) {
        EXIT(EXITANDDIE);
    }

    /* WR PROD6443: check if RMD crashed during a Full Refresh (which the RMD reports in
       the sendhandshake() command, WHICH MUST PRECEDE THIS VERIFICATION); if crash reported,
       validate the command this PMD has been passed down from in.dtc; not all commands
	   are acceptable after a crash occured in the middle of a Full Refresh, and if a Resumed Full Refresh
	   is requested, we must restart it at offset 0.
    */
    if( check_commands_due_to_unclean_RMD_shutdown )
	{
	    if( fullrefresh_restart )
	    {
            adjust_FRefresh_offset_backward( FREFRESH_BACKWARD_ADJUSTMENT, 1 );
	    }
	    else if( smart_refresh_requested )   /* Add other rejected command tests here if applicable */
		{
            reporterr (ERRFAC, M_RMD_REP_CRASH, ERRCRIT, lgnum );
            EXIT(EXITANDDIE);
		}
	}

    if (rmda_corrupted_journal_found)
    {
        // Only accept full refresh or chksum refresh commands.
        if (! (_pmd_state == FTDRFDF || _pmd_verify_secondary) )
        {
            reporterr (ERRFAC, M_RMDA_CORR_JRN, ERRCRIT, lgnum );
            EXIT(EXITANDDIE);
        }
    }
    
    /*
     * WR36133: _pmd_cpon set to the value of RMD _rmd_cpon in sendhandshake().
     *          If that does not match what PMD & PSTORE have, we trust RMD for
     *          now.
     */
    if ((saved_pmd_cpon != _pmd_cpon) && !_net_bandwidth_analysis)  {
        reporterr (ERRFAC, M_CPOVERWRITE, ERRWARN, lgnum, (_pmd_cpon) ? "ON" : "OFF");
        
        sprintf(pname, "PMD_%03d", lgnum);
        reporterr(ERRFAC, (_pmd_cpon) ? M_CPON : M_CPOFF, ERRWARN, pname);
        ps_set_group_checkpoint(mysys->pstore, group_name, _pmd_cpon);
    }

	if( !_net_bandwidth_analysis )
	{
	    if (1 != (rc = sendconfiginfo (mysys->sock))) {
	        EXIT(EXITANDDIE);
	    }
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
            EXIT(EXITANDDIE); 
        }
        _pmd_state_change = 0;
        if (_pmd_hup) {
            re_init(lgnum, path);
            _pmd_hup = 0;
        }

		if( !_net_bandwidth_analysis )
		{
	        if (GET_LG_JLESS(mysys) && _pmd_cpon) {
	            /*
	             * WR36133: For group which state is in CHECKPOINT+JLESS we want to 
	             *          avoid full refresh & smart refeesh.  
	             */
	            lgstate.lg_num = lgnum;
	            if (FTD_IOCTL_CALL(mysys->ctlfd, FTD_GET_GROUP_STATE, &lgstate)) {
	                reporterr(ERRFAC, M_DRVERR, ERRWARN, "GET_GROUP_STATE", strerror(errno));
	                EXIT(EXITANDDIE);
	            }

	            if (lgstate.state != FTD_MODE_CHECKPOINT_JLESS) {
	                if ((rc = set_drv_state_checkpoint_jless(lgnum)) != FTD_OK) {
	                    EXIT(EXITANDDIE);
	                }
	            }

	            switch(_pmd_state) {
	            case FTDRFDF:
	            case FTDRFD:
	            case FTDBFD:
	                reporterr(ERRFAC, M_INVALIDOP, ERRWARN,
	                          lgnum, statestr[_pmd_state].desc, "checkpoint is on");
	                _pmd_state = FTDPMD;
	                break;
	            default:
	                break;
	            }
	        } else if (!GET_LG_JLESS(mysys) && _pmd_cpon) {
	            /*
	             * WR36133: For group which state is in CHECKPOINT we want to 
	             *          avoid full refresh. We don't want to blow up journal
	             *          partition.  
	             */
	            switch(_pmd_state) {
	            case FTDRFDF:
	            case FTDBFD:
	                reporterr(ERRFAC, M_INVALIDOP, ERRWARN,
	                          lgnum, statestr[_pmd_state].desc, "checkpoint is on");
	                _pmd_state = FTDPMD;
	                break;
	            default:
	                break;
	            }
	        }
	        /* set group state in pstore/driver */
	        rc = setmode(lgnum, _pmd_state, doing_reboot_autostart);

            // WI_338550 December 2017, implementing RPO / RTT
            // Initialize RPO / RTT stats in the driver
            // Note: the actual initial values are set in config.c parse_system()
            give_RPO_stats_to_driver( group );
		}
		else
		{
		    // Network analysis mode, force trigger of state initiation
			rc = 0;
		}

		/* Reset the reboot flag */
		doing_reboot_autostart = 0;

        if (rc == -1) {
            EXIT(EXITANDDIE);
        } else if (rc == 1) {
            /* no mode change to pstore/driver */
            continue;
        }
        /* initialize statistics */
        if (savestats(1, _net_bandwidth_analysis) == -1) {
            EXIT(EXITANDDIE);
        }
        

        /* trigger actions for the new states */
        switch(_pmd_state) {
        case FTDBFD:
            // WI_338550 December 2017, implementing RPO / RTT
            // Give RPO stats to the driver (as is done on Windows)
            give_RPO_stats_to_driver( group );
            reporterr(ERRFAC, M_BFDSTART, ERRINFO, argv0);
            if ((rc = backfresh()) == -1) {
                EXIT(EXITANDDIE);
            }
            _pmd_state = FTDPMD;
            break;
        case FTDRFD:
#ifdef CHECK_IF_RMD_MIRRORS_MOUNTED
            /* Send a command to the RMD to check that its mirror devs are not mounted. The response
               will be received in the process_acks() call, and an error situation will be handled
               there (if mirror mounted)  (wr38281) */
            ask_if_RMD_mirrors_mounted(); 
#endif
            reporterr(ERRFAC, M_RFDSTART, ERRINFO, argv0);
            if ((rc = refresh(0)) == -1) {
                EXIT(EXITANDDIE);
            }
            refresh_started = 0;
/*
            _pmd_state = FTDPMD;
*/
            break;
        case FTDRFDF:
#ifdef CHECK_IF_RMD_MIRRORS_MOUNTED
            /* Send a command to the RMD to check that its mirror devs are not mounted. The response
               will be received in the process_acks() call, and an error situation will be handled
               there (if mirror mounted)  (wr38281) */
            ask_if_RMD_mirrors_mounted();
#endif
			if( !_net_bandwidth_analysis )
			{
	            reporterr(ERRFAC, M_RFDSTART, ERRINFO, argv0);
			}
            /* FRF - Passed the -r option to refresh_full function */
            if ((rc = refresh_full(fullrefresh_restart)) == -1) {
                EXIT(EXITANDDIE);
            }
            refresh_started = 0;
            /* FRF - reset the flag to zero */
            fullrefresh_restart = 0;
/*
            _pmd_state = FTDPMD;
*/
            break;
        case FTDPMD:
            reporterr(ERRFAC, M_PMDSTART, ERRINFO, argv0);
            if ((rc = dispatch()) == -1) {
                EXIT(EXITANDDIE);
            }
            break;
        default:
            break;
        }
        /* reset signal mask */
        sigprocmask(SIG_SETMASK, &sigs, NULL);
    }
} /* main */
