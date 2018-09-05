/*
 * ftd_kill.c - ftd kill command
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
#include "ftd_config.h"
#include "ftd_lg.h"
#include "ftd_proc.h"
#include "ftd_lg_states.h"
#include "ftd_error.h"
#include "ftd_sock.h"

#if defined(_WINDOWS)
#include "ftd_devlock.h"
#endif

static int			all = FALSE;
static int			group = FALSE;
static int			silent = FALSE;

static int			targets[FTD_MAX_GRP_NUM];
static char			*argv0;
static LList		*cfglist;
static ftd_lg_cfg_t	**cfgpp;

/*
 * usage -- This is how the program is used
 */
static void
usage(void)
{
	fprintf(stderr,"\nUsage: %s <options>\n\n", argv0);
	
	fprintf(stderr,"\
	One of the following two options is mandatory:\n\
	\t-g <group_num>\t: Stop mirroring the group\n\
	\t-a            \t: Stop mirroring all groups\n\n\
	\t-h            \t: Print this listing\n");
    
	return;
}

/*
 * proc_argv -- process argument vector 
 */
static int
proc_argv(int argc, char **argv)
{
    unsigned long	lgnum = 0;
    int				ch;

	argv0 = strdup(argv[0]);

	// At least one argument is required 
	if (argc < 1) {
		usage();
		return -1;
	}
	// Determine what options were specified 
	while ((ch = getopt(argc, argv, "ag:")) != -1) {
		switch (ch) {
		case 'a':
			// change the operation mode of all devices in all logical groups 
			if (group) {
				usage();
				return -1;
			}
			all = TRUE;
            ForEachLLElement(cfglist, cfgpp) {
				targets[(*cfgpp)->lgnum] = 1;
			}
			break;
		case 'g':
			// change the operation mode of all devices in a logical group 
			if (all) {
				usage();
				return -1;
			}
			group = TRUE;
			lgnum = strtol(optarg, NULL, 0);
			// DTurrin - Sept 19th, 2001
			// Added an extra check to make sure
			// that <group_num> is between 0 and 999.
			if ((lgnum < 0) || (lgnum > 999))
			{
				fprintf(stderr,"\nError: <group_num> must be between 0 and 999.\n");
				usage();
				return -1;
			}
			targets[lgnum] = 1;
			break;
		default:
			usage();
			return -1;
		}
	}
	if (!(all || group)) {
		usage();
		return -1;
	}

	return 0;
}

#if defined(_WINDOWS)
/*
 * send_message
 */
static int
send_message(ftd_header_t *header)
{
	int				rc, tries, maxtries, portnum;
	//char			hostname[MAXHOST];
	ftd_header_t	ack;	
	ftd_sock_t		*fsockp = NULL;

	maxtries = 3;
	tries = 0;

	// initialize the socket object
	//gethostname(hostname, sizeof(hostname));

	if (!(portnum = ftd_sock_get_port(FTD_MASTER))) {
		portnum = FTD_SERVER_PORT;
	}

	header->magicvalue = MAGICHDR;
	header->cli = (HANDLE)1;

	while (tries < maxtries) {
		// create an ipc socket and connect it to the local master ipc listener
		if ((fsockp = ftd_sock_create(FTD_SOCK_GENERIC)) == NULL) {
			goto errret;
		}

		if (ftd_sock_init(fsockp, "localhost", "localhost", LOCALHOSTIP, LOCALHOSTIP,
			SOCK_STREAM, AF_INET, 1, 1) < 0)
		{
			goto errret;
		}

		FTD_SOCK_PORT(fsockp) = portnum;

		rc = ftd_sock_connect_nonb(fsockp, fsockp->sockp->port, 1, 0, 1);
		
		if (rc == 1) {
			// got it
		} else if (rc == 0) {
			
			if (++tries == maxtries) {
				reporterr (ERRFAC, M_SOCKCONNECT, ERRWARN, 
					FTD_SOCK_LHOST(fsockp), 
					FTD_SOCK_RHOST(fsockp), 
					FTD_SOCK_PORT(fsockp), 
					sock_strerror(ETIMEDOUT));
				
				goto errret;
			}
			
			continue;
		} else {
			goto errret;
		}

		// send the message
		if (FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"send_message",fsockp, header) < 0) {
			goto errret;
		}
				 
		// recv the ack 
		if ((rc = FTD_SOCK_RECV_HEADER(__FILE__ ,"send_message",fsockp, &ack)) < 0) {
			goto errret;
		} else if (rc == 0) {
			// connection to master down
			goto errret;
		}
		error_tracef( TRACEINF5, "send_message():ack.msgtpe = %s",tdmf_socket_msg[ack.msgtype-1].cmd_txt );

		
		if (ack.msgtype == FTDACKERR) {
			tries++;
		} else {
			break;
		}

		ftd_sock_delete(&fsockp);
	}

	if (tries >= maxtries) {
	
	}

	ftd_sock_delete(&fsockp);

	// data field contains group running state
	return ack.msg.lg.data;

errret:

	ftd_sock_delete(&fsockp);

	return -1;
}
#endif

/*
 * main 
 */
int main(int argc, char **argv)
{
	char			pname[16];
#if !defined(_WINDOWS)
	int				pcnt, i;
#else
	ftd_header_t	header;
#endif

////------------------------------------------
	void ftd_util_force_to_use_only_one_processor();
	ftd_util_force_to_use_only_one_processor();
//------------------------------------------

	if (ftd_init_errfac(
#if !defined(_WINDOWS)
		CAPQ, 
#else
		PRODUCTNAME,
#endif		
		argv[0], NULL, NULL, 0, 1) == NULL) {
		goto errexit;
	}

#if !defined(_WINDOWS)
	// Make sure we are root 
	if (geteuid()) {
		reporterr(ERRFAC, M_NOTROOT, ERRCRIT);
		goto errexit;
	}
#else
	if (ftd_sock_startup() == -1)
		goto errexit;
	if (ftd_dev_lock_create() == -1)
		goto errexit;
#endif

	memset(targets, 0, sizeof(targets));

	if ((cfglist = ftd_config_create_list()) == NULL) {
		goto errexit;
	}

#if defined(FTD_KILL_RMD)
    if (ftd_config_get_secondary(PATH_CONFIG, cfglist) < 0) {
		goto errexit;
	}
#else
    if (ftd_config_get_primary_started(PATH_CONFIG, cfglist) < 0) {
		goto errexit;
	}
#endif

	if (proc_argv(argc, argv) < 0) {
		goto errexit;
	}

	// kill targeted groups 
#if defined(FTD_KILL_PMD)

	ForEachLLElement(cfglist, cfgpp) {
		if (targets[(*cfgpp)->lgnum]) {
			sprintf(pname, "PMD_%03d", (*cfgpp)->lgnum);
#if defined(_WINDOWS)
			strcpy(header.msg.data, pname);
			header.msgtype = FTDCSIGTERM;

			send_message(&header);
#else
			ftd_proc_terminate(pname);
			ftd_proc_kill(pname);
#endif
		}
	}
#elif defined(FTD_KILL_RMD)
	ForEachLLElement(cfglist, cfgpp) {
		if (targets[(*cfgpp)->lgnum]) {
			sprintf(pname, "RMD_%03d", (*cfgpp)->lgnum);
#if defined(_WINDOWS)
			strcpy(header.msg.data, pname);
			header.msgtype = FTDCSIGTERM;
			
			// send the message
			send_message(&header);
#else
			ftd_proc_signal(pname, SIGUSR1);
#endif
		}
	}
#elif defined(FTD_KILL_BACKFRESH)
	{
		int	rc;

		ForEachLLElement(cfglist, cfgpp) {
			if (targets[(*cfgpp)->lgnum]) {
				if (ftd_lg_get_pstore_run_state((*cfgpp)->lgnum, *cfgpp) != FTD_SBACKFRESH) {
					continue;
				}
				sprintf(pname, "PMD_%03d", (*cfgpp)->lgnum);
#if defined(_WINDOWS)
				
				strcpy(header.msg.data, pname);
				header.msgtype = FTDCSIGUSR1;
				
				// send the message
				if ((rc = send_message(&header)) < 0) {
					goto errexit;
				} else if (rc == 1) {
					// group was running and got the kill msg
#else
				if ((rc = ftd_proc_signal(pname, SIGUSR1)) < 0) {
					goto errexit;
				} else if (rc == 1) {
					// group was running and got the kill msg
#endif
				} else {
					// pstore -> ACCUMULATE
					if (ftd_lg_set_pstore_run_state((*cfgpp)->lgnum, *cfgpp, -1) < 0) {
						goto errexit;
					}
					// driver -> PASSTHRU
					if (ftd_lg_set_driver_run_state((*cfgpp)->lgnum,
						FTD_MODE_PASSTHRU) < 0)
					{
						goto errexit;
					}
				}
			}
		}
	}
#elif defined(FTD_KILL_REFRESH)
	{
		int	state, rc;
    
		ForEachLLElement(cfglist, cfgpp) {
			if (targets[(*cfgpp)->lgnum]) {
				if ((state = ftd_lg_get_pstore_run_state((*cfgpp)->lgnum, *cfgpp)) < 0) {
					continue;
				}
				if (state != FTD_SREFRESH
					&& state != FTD_SREFRESHF
					&& state != FTD_SREFRESHC)
				{
					continue;
				}
				sprintf(pname, "PMD_%03d", (*cfgpp)->lgnum);
#if defined(_WINDOWS)
				strcpy(header.msg.data, pname);
				header.msgtype = FTDCSIGUSR1;

				// send the message
				if ((rc = send_message(&header)) < 0) {
					goto errexit;
				} else if (rc == 1) {
					// group was running and got the kill msg
#else
				if ((rc = ftd_proc_signal(pname, SIGUSR1)) < 0) {
					goto errexit;
				} else if (rc == 1) {
					// group was running and got the kill msg
#endif
				} else {
					// group not running - driver -> TRACKING
					if (ftd_lg_set_driver_run_state((*cfgpp)->lgnum,
						FTD_MODE_TRACKING) < 0)
					{
						goto errexit;
					}

				}
			}
		}
	}
#endif

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

errexit:

#if defined(_WINDOWS) && defined(_DEBUG)
	printf("\nPress any key to continue....\n");
	while(!_kbhit())
		Sleep(1);
#endif

	exit(1);
}

