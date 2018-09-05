/*
 * ftd_stop.c - Stop one or more logical groups
 *
 * Copyright (c) 1999 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#include "ftd_port.h"
#include "ftd_error.h"
#include "ftd_config.h"
#include "ftd_proc.h"
#include "ftd_ps.h"
#include "ftd_lg.h"
#include "ftd_sock.h"

#if defined(_WINDOWS)
#include "ftd_devlock.h"
#endif

static int autostart;

static void
usage(char **argv)
{
	fprintf(stderr, "Usage: %s [-a | -g group_number] [-s]\n", argv[0]);
	fprintf(stderr, "OPTIONS: -a => all groups\n");
	fprintf(stderr, "         -g => group number\n");
	fprintf(stderr, "         -s => don't clear autostart in pstore\n");
}

int 
main(int argc, char *argv[])
{
	int		all, group, lgcnt;
	char	ftdctlpath[MAXPATHLEN];
#if !defined(_WINDOWS)
	int		pcnt;
	char	procname[MAXPATHLEN];
#endif

#if defined(_AIX)
	int   ch;
#else /* defined(_AIX) */
	char  ch;
#endif /* defined(_AIX) */
	ftd_lg_cfg_t	**cfgpp, *cfgp;
#if defined(HPUX)
	ftd_proc_t		*procp;
#endif
	ftd_lg_t		*lgp;
	LList			*cfglist = NULL;

////------------------------------------------
#if defined(_WINDOWS)
	void ftd_util_force_to_use_only_one_processor();
	ftd_util_force_to_use_only_one_processor();
#endif
//------------------------------------------

	if (argc < 2) {
		goto usage_error;
	}

	sprintf(ftdctlpath, "%s/%sctl", PATH_BIN_FILES, QNM);

	all = 0;
	group = -1;
	autostart = 1;

	/* process all of the command line options */
	while ((ch = getopt(argc, argv, "asg:")) != -1) {
		switch(ch) {
		case 'a':
			if (group != -1) {
				goto usage_error;
			}
			all = 1;
			break;
		case 'g':
			if (all != 0) {
				goto usage_error;
			}
			group = strtol(optarg, NULL, 0);
			break;
		case 's':
			autostart = 0; 
			break;
		default:
			goto usage_error;
		}
	}
	if ((all == 0) && (group < 0)) {
		goto usage_error;
	}

#if defined(_WINDOWS)
	if (ftd_sock_startup() == -1) {
		error_tracef( TRACEERR, "Stop:main()", "Calling ftd_sock_startup()" );
		goto errexit;
	}
	if (ftd_dev_lock_create() == -1) {
		error_tracef( TRACEERR, "Stop:main()", "Calling ftd_dev_lock_create()" );
		goto errexit;
	}
#endif

	if (ftd_init_errfac(
#if !defined(_WINDOWS)
		CAPQ, 
#else
		PRODUCTNAME,
#endif		
		argv[0], NULL, NULL, 0, 1) == NULL) {
		error_tracef( TRACEERR, "Stop:main()", "Calling ftd_init_errfac()" );
		goto errexit;
	}
	// create config file list
	if ((cfglist = ftd_config_create_list()) == NULL) {
		error_tracef( TRACEERR, "Stop:main()", "Calling ftd_config_create_list()" );
		goto errexit;
	}

	// get all primary config files
	if (ftd_config_get_primary_started(PATH_CONFIG, cfglist) < 0) {
		error_tracef( TRACEERR, "Stop:main()", "Calling ftd_config_get_primary_started()" );
		goto errexit;
	}

	// if none exist then exit	
	if ((lgcnt = SizeOfLL(cfglist)) == 0) {
		reporterr(ERRFAC, M_NOGROUP, ERRWARN);
		goto errexit;
	}

	if (all != 0) {
		ForEachLLElement(cfglist, cfgpp) {

			cfgp = *cfgpp;

			if ((lgp = ftd_lg_create()) == NULL) {
				error_tracef( TRACEERR, "Stop:main()", "Calling ftd_lg_create()" );
				goto errexit;
			}

			if (ftd_lg_init(lgp, cfgp->lgnum, ROLEPRIMARY, 1) < 0) {
				ftd_lg_delete(lgp);
				goto errexit;
			}

			if (ftd_lg_rem(lgp, autostart, 0) < 0) {
				reporterr(ERRFAC, M_STOPGRP, ERRCRIT, cfgp->lgnum);
				ftd_lg_delete(lgp);
				goto errexit;
			}

			ftd_lg_delete(lgp);
		}
	} else {

		if ((lgp = ftd_lg_create()) == NULL) {
			error_tracef( TRACEERR, "Stop:main()", "Calling ftd_lg_create()" );
			goto errexit;
		}

		if (ftd_lg_init(lgp, group, ROLEPRIMARY, 1) < 0) {
			ftd_lg_delete(lgp);
			error_tracef( TRACEERR, "Stop:main()", "Calling ftd_lg_init()" );
			goto errexit;
		}

		if (ftd_lg_rem(lgp, autostart, 0) < 0) {
			reporterr(ERRFAC, M_STOPGRP, ERRCRIT, group);
			ftd_lg_delete(lgp);
			goto errexit;
		}

		ftd_lg_delete(lgp);
	}
#if defined(HPUX)
	if ((pid = proc_get_pid(QNM "ctl", 0, &pcnt)) > 0) {
		/*
		 * signal the 'hold open' process so that it opens only our currently
		 * started groups
	 	 */
		kill(pid, SIGUSR1);
	} else {
		/* if it isn't there, it should be, so exec it. */
		procp = ftd_proc_create();

		strcpy(procp->procp->command, ftdctlpath);

		ftd_proc_exec(procp, 1);
		
		ftd_proc_delete(procp);
	}
#endif

	if (cfglist) {
		ftd_config_delete_list(cfglist);
	}
	ftd_delete_errfac();

#if defined(_WINDOWS)
	ftd_dev_lock_delete();
	ftd_sock_cleanup();
#endif

#if defined(_WINDOWS) && defined(_DEBUG)
	printf("\nPress any key to continue....\n");
	while(!_kbhit())
		Sleep(1);
#endif

	exit(0);
	return 0; /* for stupid compiler */

usage_error:
    usage(argv);

errexit:

	if (cfglist) {
		ftd_config_delete_list(cfglist);
	}
	ftd_delete_errfac();

#if defined(_WINDOWS)
	ftd_dev_lock_delete();
    ftd_sock_cleanup();
#endif

#if defined(_WINDOWS) && defined(_DEBUG)
	printf("\nPress any key to continue....\n");
	while(!_kbhit())
		Sleep(1);
#endif

	exit(1);
    return 0; /* for stupid compiler */
}
