/*
 * ftd_pmd.c - ftd primary mirror daemon
 *
 * Copyright (c) 2000 Legato Systems, Inc.  All Rights Reserved.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in
 * subparagraph (c)(1)(ii) of the Rights in Technical Data and
 * Computer Software clause at DFARS 52.227-7013 and in similar
 * clauses in the FAR and NASA FAR Supplement.
 *
 */
#include "ftd_port.h"
#include "ftd_proc.h"
#include "ftd_sock.h"
#include "ftd_error.h"
#include "ftd_lg.h"
#include "ftd_fsm.h"
#include "ftd_ioctl.h"
#include "ftd_lic.h"
#include "ftd_stat.h"

extern char** share_envp;   /* shared environment vector		*/
extern char** share_argv;   /* shared argument vector			*/

static int init_state(int cstate, int pstate, int dstate);

#if !defined(_WINDOWS)

static void sig_handler(int s);
static void install_sigaction(void);
static void proc_envp(char **envp, ftd_lg_t *lgp);

static ftd_lg_t	*lgp;
static int		sigpipe[2];

/*
 * sig_handler -- signal handler
 */
static void
sig_handler(int s)
{
	int	msgtype = 0;

	switch(s) {
	case SIGUSR1:
		msgtype = FTDCSIGUSR1;
		break;
	case SIGTERM:
		msgtype = FTDCSIGTERM;
		break;
	case SIGPIPE:
		msgtype = FTDCSIGPIPE;
		break;
	case SIGHUP:
		break;
	default:
		// error - do nothing
		return;
	}

	write(sigpipe[STDOUT_FILENO], &msgtype, sizeof(msgtype));

	return;
}

/*
 * install_sigaction -- install signal actions for PMD 
 */
static void
install_sigaction(void)
{
	int			i, j, numsig;
	struct sigaction	caction;
	sigset_t		block_mask;
	static int		csignal[] = { SIGUSR1, SIGTERM, SIGPIPE , SIGHUP};

	pipe(sigpipe);

	sigemptyset(&block_mask);
	numsig = sizeof(csignal)/sizeof(*csignal);

	for (i = 0; i < numsig; i++) {
		caction.sa_handler = sig_handler;
		caction.sa_mask = block_mask;
		caction.sa_flags = SA_RESTART;
		sigaction(csignal[i], &caction, NULL);
	} 

	return;
}

/*
 * proc_envp -- process environment variables
 */
static void 
proc_envp(char **envp, ftd_lg_t *lgp) 
{
	int	envcnt, state;
	
	envcnt = 0;
	while (envp[envcnt]) {
		if (0 == strncmp(envp[envcnt], "_FTD_", 5)) {
			if (0 == strncmp(envp[envcnt], "_FTD_STATE=", 11)) { 
				sscanf(&(envp[envcnt])[11], "%d", &state);
				
				if ((state && 0x00ff) == FTD_SBACKFRESH) {
					// look for force flag
					if ((state && 0x0100) {
						SET_LG_BACK_FORCE(lgp->flags);
						state = FTD_SBACKFRESH;
					}
				}

				SET_LG_STATE(lgp->flags, state);

			} else if (0 == strncmp(envp[envcnt], "_FTD_CONSLEEP=", 14)) { 
				sscanf(&(envp[envcnt])[14], "%d", &lgp->consleep);
			}
		}
        envcnt++;
	}

	return;
}
#endif

/*
 * init_state -- determine initial input to lg state machine
 */
static int
init_state(int cstate, int pstate, int dstate)
{
	int		state;

	switch(cstate) {
	case FTD_SNORMAL:
		// target command line state = NORMAL - use driver state
		state = ftd_lg_driver_state_to_lg_state(dstate);

		if (state == FTD_SREFRESH || state == FTD_STRACKING) {

			// must make sure we restart the correct
			// type of refresh - look @ pstore state 

			if (pstate == FTD_SREFRESHF) {
				state = FTD_SREFRESHF;
			}
		}
		break;
	default:
		// target command line state != NORMAL - use it
		// override driver/pstore
		state = cstate;
	}

	return ftd_fsm_state_to_input(state);
}

#if !defined(_WINDOWS)	
/*
 * main -- primary mirror daemon
 */
int
main (int argc, char **argv, char **envp)
{
#else
/*
 * PrimaryThread - FTD master thread
 */
DWORD 
PrimaryThread(LPDWORD param)
{
	ftd_lg_t	*lgp = NULL;
#endif
	int		lgnum, rc = 0;
	int		cstate, dstate, pstate;
	u_char		inchar;
	int		portnum, n;
	ftd_header_t	header;
	char		procname[80];
    int lgnum_trace;

#if defined(_WINDOWS)
 	ftd_proc_args_t *args = (ftd_proc_args_t *) param;

	sprintf(procname, "PMD_%03d", args->lgnum);
	lgnum_trace = args->lgnum;

	if (ftd_init_errfac(PRODUCTNAME, procname, NULL, NULL, 1, 1) == NULL) {
		Return(1);
	}
#else
	// install signal handler
	install_sigaction();
	
	strcpy(procname, argv[0]);

	if (ftd_init_errfac(CAPQ, procname, NULL, NULL, 1, 1) == NULL) {
		Return(1);
	}

#endif
    error_tracef(TRACEINF,"PrimaryThread %s : Id %d" ,procname, GetCurrentThreadId());

	if (ftd_lic_verify(CAPQ) < 0) {
		error_tracef(TRACEERR,"PMD_%03d : ftd_lic_verify()",lgnum_trace);
		goto errexit;
	}

#if !defined(_WINDOWS)
	if (!proc_is_parent(FTD_MASTER)) {
		reporterr(ERRFAC, M_PARENTNOTMASTER, ERRCRIT, FTD_MASTER);
		goto errexit;
	}
	sscanf(argv[0], "PMD_%03d", &lgnum);
#else
	lgnum = args->lgnum;
#endif

	if ((lgp = ftd_lg_create()) == NULL) {
		error_tracef(TRACEERR,"PMD_%03d : ftd_lg_create()",lgnum_trace);
		goto errexit;
	}
	if (ftd_lg_init(lgp, lgnum, ROLEPRIMARY, 1) < 0) {
		error_tracef( TRACEERR,"PMD_%03d : ftd_lg_init()",lgnum_trace);
		goto errexit;
	}

#if !defined(_WINDOWS)
	// close lg signal pipe and use local one
	close(lgp->sigpipe[STDIN_FILENO]);
	close(lgp->sigpipe[STDOUT_FILENO]);

	lgp->sigpipe[STDIN_FILENO] = sigpipe[STDIN_FILENO];
	lgp->sigpipe[STDOUT_FILENO] = sigpipe[STDOUT_FILENO];
#endif
	
	if (ftd_lg_open(lgp) < 0) {
		error_tracef( TRACEERR,"PMD_%03d : ftd_lg_open()",lgnum_trace);
		goto errexit;
	}
	if (ftd_lg_get_driver_state(lgp, 0) < 0) {
		error_tracef( TRACEERR,"PMD_%03d : ftd_lg_get_driver_state()",lgnum_trace);
		goto errexit;
	}
	if (ftd_proc_init(lgp->fprocp, FTD_PMD_PATH, procname) < 0) {
		error_tracef( TRACEERR,"PMD_%03d : ftd_proc_init()",lgnum_trace);
		goto errexit;
	}

	// this should go into _init
	lgp->fprocp->proctype = FTD_PROC_PMD;

#if !defined(_WINDOWS)
	(void)proc_envp(envp, lgp);
#else

	if ((args->state & 0x000000ff) == FTD_SBACKFRESH) {
		
		// look for force flag
		if ((args->state & FTD_LG_BACK_FORCE)) {
			SET_LG_BACK_FORCE(lgp->flags);
			args->state = FTD_SBACKFRESH;
		}
	}
	
	SET_LG_STATE(lgp->flags, args->state);

	lgp->procp = args->procp;
	lgp->consleep = args->consleep;

#endif

	ftd_stat_init_driver(lgp);

	// create an ipc connection between us and the local master
	if (ftd_sock_init(lgp->isockp, "localhost",	"localhost",
		LOCALHOSTIP, LOCALHOSTIP, SOCK_STREAM, AF_INET, 1, 1) < 0)
	{
		error_tracef( TRACEERR,"PMD_%03d : ftd_sock_init()",lgnum_trace);
		goto errexit;
	}

	if (!(portnum = ftd_sock_get_port(FTD_MASTER))) {
		portnum = FTD_SERVER_PORT;
	}

	if (ftd_sock_connect(lgp->isockp, portnum) < 0) {
		error_tracef( TRACEERR,"PMD_%03d : ftd_sock_connect()",lgnum_trace);
		goto errexit;
	}

	memset(&header, 0, sizeof(header));
#if !defined(_WINDOWS)
	header.cli = getpid(); 	// tells master this is a child connect
#else
	header.cli = args->procp->pid;
#endif
	

	if (FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"****PrimaryThread is there!",lgp->isockp, &header) < 0) {
		error_tracef( TRACEERR,"PMD_%03d : FTD_SOCK_SEND_HEADER",lgnum_trace);
		goto errexit;
	}

	if (ftd_sock_init(lgp->dsockp, lgp->cfgp->phostname,
		lgp->cfgp->shostname, 0, 0, SOCK_STREAM, AF_INET, 1, 0) < 0)
	{
		error_tracef( TRACEERR,"PMD_%03d : ftd_sock_init()",lgnum_trace);
		goto errexit;
	}

#if defined(_WINDOWS)
	FTD_SOCK_HEVENT(lgp->dsockp) = CreateEvent(NULL, FALSE, FALSE, NULL);
			
	if (FTD_SOCK_HEVENT(lgp->dsockp) == WSA_INVALID_EVENT)
	{
		error_tracef( TRACEERR,"PMD_%03d : dsockp WSA_INVALID_EVENT",lgnum_trace);
		goto errexit;
	}
	FTD_SOCK_HEVENT(lgp->isockp) = CreateEvent(NULL, FALSE, FALSE, NULL);
			
	if (FTD_SOCK_HEVENT(lgp->isockp) == WSA_INVALID_EVENT)
	{
		error_tracef( TRACEERR,"PMD_%03d : isockp WSA_INVALID_EVENT",lgnum_trace);
		goto errexit;
	}
#endif

	// set ipc connection to non-blocking
	ftd_sock_set_nonb(lgp->isockp);

	// sleep for a short time if we are reconnecting
	if (lgp->consleep > 0) {
#if defined(_WINDOWS)		
		// check signals on NT since there are no REAL interupts
		// and we want to be interuptable
		if (ftd_lg_check_signals(lgp, lgp->consleep) == FTD_EXIT_DIE) {
			// force kill - signaled
			rc = FTD_EXIT_DIE;
			error_tracef( TRACEERR,"PMD_%03d : ftd_lg_check_signals()",lgnum_trace);
			goto errexit;
		}
#else
		sleep(lgp->consleep);
#endif	
	}

	n = 1;
	if (ftd_sock_set_opt(lgp->dsockp, SOL_SOCKET, SO_KEEPALIVE,
		(char*)&n, sizeof(int))) {
		error_tracef( TRACEERR,"PMD_%03d : ftd_sock_set_opt()",lgnum_trace);
		goto errexit;
	}

	if (ftd_sock_connect_forever(lgp, portnum) < 0) {
		error_tracef( TRACEERR,"PMD_%03d : ftd_sock_connect_forever()",lgnum_trace);
		goto errexit;
	}

	// set socket to blocking
	ftd_sock_set_b(lgp->dsockp);

	// initial msg - so master knows we want an rmd
	if ((rc = ftd_sock_send_noop(lgp->dsockp, lgp->lgnum, 0)) < 0) {
		error_tracef( TRACEERR,"PMD_%03d : ftd_sock_send_noop()",lgnum_trace);
		goto errexit;
	}
	
	if ((rc = ftd_sock_send_version(lgp->dsockp, lgp)) < 0) {
		error_tracef( TRACEERR,"PMD_%03d : ftd_sock_send_version()",lgnum_trace);
		goto errexit;
	}
	if ((rc = ftd_sock_send_handshake(lgp)) < 0) {
		error_tracef( TRACEERR,"PMD_%03d : ftd_sock_send_handshake()",lgnum_trace);
		goto errexit;
	}
	if ((rc = ftd_sock_send_chkconfig(lgp)) < 0) {
		error_tracef( TRACEERR,"PMD_%03d : ftd_sock_send_chkconfig()",lgnum_trace);
		goto errexit;
	}

	// set socket to non-blocking
	ftd_sock_set_nonb(lgp->dsockp);
    
	// set connection status to CONNECTED !!!
	ftd_stat_set_connect(lgp, 1);

#if !defined(_WINDOWS)    
	// save signal mask
	sigprocmask(0, NULL, &lgp->fprocp->sigs);
#endif

	// init the state machine
	ftd_fsm_init(ftd_ttfsm, ftd_ttstab, FTD_NSTATES);

	// command line state
	cstate = GET_LG_STATE(lgp->flags);

	// pstore state
	pstate = ftd_lg_get_pstore_run_state(lgp->lgnum, lgp->cfgp);

	// driver state
	dstate = ftd_lg_get_driver_run_state(lgp->lgnum);

	// determine initial inputs for state machine
	inchar = init_state(cstate, pstate, dstate);

	// start the state machine
	rc = ftd_fsm(lgp, ftd_ttfsm, ftd_ttstab, FTD_SNORMAL, inchar);
    error_tracef( TRACEINF,"%s : Id %d, ftd_fsm rc = 0x%x" ,procname, GetCurrentThreadId(), rc);

errexit:

	if (lgp) {
	// set connection status to ACCUMULATE !!!
		ftd_stat_set_connect(lgp, -1);
	// set RUN STATE to TRACKING !!!
		ftd_lg_set_pstore_run_state(lgp->lgnum, lgp->cfgp, FTD_STRACKING);
	// set RUN STATE to TRACKING !!!
	    ftd_ioctl_set_group_state(lgp->ctlfd, lgp->lgnum, FTD_MODE_TRACKING);

		if (rc == FTD_LG_NET_BROKEN) {
			reporterr(ERRFAC, M_NETBROKE, ERRWARN,
				lgp->dsockp->sockp->rhostname);
		}
		
#if defined(_WINDOWS)
		if (FTD_SOCK_HEVENT(lgp->dsockp)) {
			CloseHandle(FTD_SOCK_HEVENT(lgp->dsockp));
		}
		if (FTD_SOCK_HEVENT(lgp->isockp)) {
			CloseHandle(FTD_SOCK_HEVENT(lgp->isockp));
		}
		// check signals on NT since there are no REAL interupts
		if (ftd_lg_check_signals(lgp, 0) == FTD_EXIT_DIE) {
			// force kill - signaled
			rc = FTD_EXIT_DIE;
		}
#endif

		ftd_lg_close(lgp);
		ftd_lg_delete(lgp);
	}

	reporterr(ERRFAC, M_PMDEXIT, ERRINFO);

	ftd_delete_errfac();
	if (rc == FTD_LG_NET_BROKEN || rc == FTD_EXIT_NETWORK) {
		Return(FTD_EXIT_NETWORK);
	} else {
		Return(FTD_EXIT_DIE);
	}

}
