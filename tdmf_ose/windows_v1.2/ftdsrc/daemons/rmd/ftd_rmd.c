/*
 * ftd_rmd.c - secondary mirror daemon
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
#include "ftd_journal.h"
#include "ftd_lg.h"
#include "ftd_stat.h"
#include "ftd_error.h"
#include "ftd_sock.h"
#include "ftd_fsm.h"
#include "ftd_fsm_tab.h"
#include "ftd_lic.h"

extern char** share_envp;   /* shared environment vector		*/
extern char** share_argv;   /* shared argument vector			*/

static int ftd_rmd(ftd_lg_t *lgp);
static int ftd_rmd_apply(ftd_lg_t *lgp, int cpon);

#if !defined(_WINDOWS)

static void sig_handler(int s);
static void install_sigaction(void);
static void proc_envp(char **envp, int *state, int *apply, SOCKET *sock,
	int *cpon); 

static ftd_lg_t	*lgp;

/*
 * sig_handler -- RMD signal handler 
 */
static void
sig_handler(int s) 
{
	int	msgtype = 0;
 
	switch(s) {
	case SIGTERM:
		msgtype = FTDCSIGTERM;
		break;
	case SIGPIPE:
		msgtype = FTDCSIGPIPE;
		break;
	case SIGUSR1:
		msgtype = FTDCKILL;
		break;
	case SIGHUP:
		break;
	default:
		break;
	}

	write(lgp->sigpipe[STDOUT_FILENO], &msgtype, sizeof(msgtype));

	return;
}

/*
 * install_sigaction -- install signal actions for RMD 
 */
static void
install_sigaction(void)
{
	int			i, j, numsig;
	struct sigaction	caction;
	sigset_t		block_mask;
	static int		csignal[] = { SIGUSR1, SIGTERM, SIGPIPE , SIGHUP};

	sigemptyset(&block_mask);
	numsig = sizeof(csignal)/sizeof(*csignal);

	for (i = 0; i < numsig; i++) {
		for (j = 0; j < numsig; j++) {
			sigaddset(&block_mask, csignal[j]);
		}
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
proc_envp(char **envp, int *state, int *apply, SOCKET *sock, int *cpon) 
{
	int	envcnt;
	
	envcnt = 0;
	while (envp[envcnt]) {
		if (0 == strncmp(envp[envcnt], "_FTD_", 5)) {
			if (0 == strncmp(envp[envcnt], "_FTD_STATE=", 11)) { 
				sscanf(&(envp[envcnt])[11], "%d", state);
			} else if (0 == strncmp(envp[envcnt], "_FTD_APPLY=", 11)) { 
				sscanf(&(envp[envcnt])[11], "%d", apply);
			} else if (0 == strncmp(envp[envcnt], "_FTD_CPON=", 10)) { 
				sscanf(&(envp[envcnt])[10], "%d", cpon);
			} else if (0 == strncmp(envp[envcnt], "_FTD_SOCK=", 10)) { 
				sscanf(&(envp[envcnt])[10], "%d", sock);
			}
		}
        envcnt++;
	}

	return;
}

#endif

/*
 * ftd_rmd -- secondary mirror process
 */
static int
ftd_rmd(ftd_lg_t *lgp)
{
	int		rc, n, i, ret;
#if defined(_WINDOWS)
	ftd_perf_t	*perfp = NULL;
#endif

#if !defined(_WINDOWS)    
	// save signal mask 
	sigprocmask(0, NULL, &lgp->fprocp->sigs);
#endif

	n = 1; 
	if (ftd_sock_set_opt(lgp->dsockp, SOL_SOCKET, SO_KEEPALIVE,
		(char*)&n, sizeof(int)))
	{
		return -1;
	}

#if !defined(_WINDOWS)
	(void)ftd_stat_init_file(lgp);
#else
/*
	if ( (perfp = ftd_perf_create()) == NULL) {
		return -1;
	}

	if ( ftd_perf_init(perfp, FALSE) ) {
		return -1;
	}

	(void)ftd_stat_init_memory(lgp, perfp);
*/
#endif

	// get journal state 
	if (ftd_journal_get_cur_state(lgp->jrnp, 1) < 0) {
		reporterr(ERRFAC, M_JRNINIT, ERRCRIT, LG_PROCNAME(lgp));
		return FTD_EXIT_DIE;
    }

	// set group checkpoint state according to journal state 
	if (GET_JRN_CP_ON(lgp->jrnp->flags)) {
		SET_LG_CPON(lgp->flags);
	}
	
	if (GET_JRN_CP_PEND(lgp->jrnp->flags)) {
		SET_LG_CPPEND(lgp->flags);
	}

	// we know the first three msg types - we need to
	// process them first before continueing
	//
	// messages:
	//
	// 1. version exchange
	// 2. handshake
	// 3. config file exchange
	//

	// process: version, handshake, config info
	i = 0;
	while (TRUE) 
		{
		if ((rc = ftd_sock_recv_lg_msg(lgp->dsockp, lgp, -1)) != 0) 
			{
			if (rc < 0) 
				{
				return FTD_EXIT_DIE;
				} 
			if (rc == FTD_LG_NET_NOT_READABLE) 
				{
				continue;
				} 
			else 
				{
				error_tracef( TRACEINF, "ftd_rmd:rc == %d",rc );
				return rc;			// <<<<<<<<<<<
				}
			}
		if (i++ == 2) 
			{
			break;
			}
		}

	// set net connection to non-blocking
	ftd_sock_set_nonb(lgp->dsockp);

	if (GET_LG_STATE(lgp->flags) == FTD_SREFRESHF) {
		// got state from handshake - hammer journal files
		ftd_journal_delete_all_files(lgp->jrnp);
		SET_JRN_MODE(lgp->jrnp->flags, FTDMIRONLY);
	}

	// figure out if we are journaling or mirroring 
	if (GET_JRN_MODE(lgp->jrnp->flags) == FTDMIRONLY) {
		if (ftd_lg_stop_journaling(lgp, FTDJRNCO, FTDMIRONLY) < 0) {
			return FTD_EXIT_DIE;
		}
	} else {
		if (ftd_lg_start_journaling(lgp, GET_JRN_STATE(lgp->jrnp->flags)) < 0) {
			return FTD_EXIT_DIE;
		}
	}

	// if coherent journals to apply then do it 
	if (GET_LG_CPPEND(lgp->flags)) {
		if (ftd_sock_send_start_apply(lgp->lgnum, lgp->isockp, 1) < 0) {
			return FTD_EXIT_DIE;
		}
	} else if (GET_JRN_STATE(lgp->jrnp->flags) == FTDJRNCO
		&& GET_JRN_MODE(lgp->jrnp->flags) != FTDMIRONLY
		&& !GET_LG_CPON(lgp->flags) )
	{
		if (ftd_sock_send_start_apply(lgp->lgnum, lgp->isockp, 0) < 0) {
			return FTD_EXIT_DIE;
		}
	}

	SET_LG_STATE(lgp->flags, FTD_SNORMAL);

	// init the state machine 
	ftd_fsm_init(ftd_ttfsm, ftd_ttstab, FTD_NSTATES);

	// start the state machine 
	error_tracef( TRACEINF4, "ftd_rmd():before ftd_fsm()" );
	ret = ftd_fsm(lgp, ftd_ttfsm, ftd_ttstab, FTD_SNORMAL, 0);
	error_tracef( TRACEINF4, "ftd_rmd():after ftd_fsm()" );

#if defined(_WINDOWS)
/*
	if (perfp) {
		ftd_stat_remove_memory(lgp, perfp);
	
		ftd_perf_delete(perfp);
	}
*/
#endif
	
	return ret;
}

/*
 * ftd_rmd_apply -- secondary journal apply process
 */
static int
ftd_rmd_apply(ftd_lg_t *lgp, int cpon)
{
	ftd_header_t	header, ack;
	int				rc, ret;

	if (ftd_journal_get_all_files(lgp->jrnp) < 0) {
		return -1;
	}
	
	if (ftd_journal_co(lgp->jrnp) == 0) {
		// no coherent journal files
		return 0;
	}
	
	if (ftd_lg_open_devs(lgp, O_WRONLY, 0, 5) < 0) {
		return -1;
	}
     
	if (cpon) {
		SET_LG_CPPEND(lgp->flags);
	}

	// init the state machine 
	ftd_fsm_init(ftd_ttfsm, ftd_ttstab, FTD_NSTATES);

	// start the state machine 
	ret = ftd_fsm(lgp, ftd_ttfsm, ftd_ttstab, FTD_SAPPLY, 0);

	ftd_lg_close_devs(lgp);

	if (GET_LG_CPPEND(lgp->flags)) {
		// set cp state in group thread via master 
		memset(&header, 0, sizeof(header));

		header.msgtype = FTDCAPPLYDONECPON;
		header.msg.lg.lgnum = lgp->lgnum;


		rc = FTD_SOCK_SEND_HEADER( TRUE,LG_PROCNAME(lgp),"ftd_rmd_apply",lgp->isockp, &header);
		rc = FTD_SOCK_RECV_HEADER(LG_PROCNAME(lgp) ,"ftd_rmd_apply",lgp->isockp, &ack);
		
 		UNSET_LG_CPPEND(lgp->flags);
	}
	
	ftd_lg_close_devs(lgp);

	return ret;
}

#if !defined(_WINDOWS)	
/*
 * main -- secondary mirror daemon
 */
int
main (int argc, char **argv, char **envp)
{
#else

/*
 * RemoteThread - secondary mirror thread
 */
DWORD 
RemoteThread(LPDWORD param)
{
	ftd_lg_t	*lgp = NULL;
#endif
	ftd_sock_t	*tsockp = NULL;
	int			state, apply, cpon, lgnum, portnum, rc;
	SOCKET		sock = (SOCKET)-1;
	char		prefix[32];
	ftd_header_t	header;
	char		procname[80];
#if defined(_WINDOWS)
	ftd_proc_args_t *args = (ftd_proc_args_t *) param;
#endif

	cpon = FALSE;
	apply = FALSE;

#if !defined(_WINDOWS)
	(void)proc_envp(envp, &state, &apply, &sock, &cpon);
#else
	args = (ftd_proc_args_t *) param;
	sock = args->dsock;
	apply = args->apply;
	cpon = args->cpon;
	state = args->state;
#endif

#if defined(_WINDOWS)
	if (apply)
		sprintf(procname, "RMDA_%03d", args->lgnum);
	else
		sprintf(procname, "RMD_%03d", args->lgnum);

	if (ftd_init_errfac(PRODUCTNAME, procname, NULL, NULL, 1, 1) == NULL) {
		goto errexit;
	}
	
	lgnum = args->lgnum;

   error_tracef( TRACEINF, "RemoteThread %s : Id %d" ,procname, GetCurrentThreadId() );

#else
	strcpy(procname, argv[0]);

	if (ftd_init_errfac(CAPQ, procname, NULL, NULL, 1, 1) == NULL) {
		error_tracef( TRACEERR, "RemoteThread main():%s Calling ftd_init_errfac()", procname );
		goto errexit;
	}
	if (!proc_is_parent(FTD_MASTER)) {
		reporterr(ERRFAC, M_PARENTNOTMASTER, ERRCRIT, FTD_MASTER);
		goto errexit;
	}
	// install signal handler 
	install_sigaction();

	if (!strncmp(argv[0], "RMDA_", 5)) {
		sscanf(argv[0], "RMDA_%03d", &lgnum);
	} else {
		sscanf(argv[0], "RMD_%03d", &lgnum);
	}
#endif

	if (apply) {
		reporterr(ERRFAC, M_RMDASTART, ERRINFO);
	} else {
		reporterr(ERRFAC, M_RMDSTART, ERRINFO);
		
		// create a temp sock object for error communication 
		// back to peer if we fail before group gets set up
		if ((tsockp = ftd_sock_create(FTD_SOCK_INET)) == NULL) {
			goto errexit;
		}
		
		// sock - passed to us by the master
		FTD_SOCK_FD(tsockp) = (SOCKET)sock;

		ftd_sock_set_connect(tsockp);

		// validate it
		tsockp->magicvalue = FTDSOCKMAGIC;
	}

	if (ftd_lic_verify(CAPQ) < 0) {
		error_tracef( TRACEERR, "RemoteThread main():%s Calling ftd_lic_verify()", procname );
		goto errexit;
	}
	if ((lgp = ftd_lg_create()) == NULL) {
		error_tracef( TRACEERR, "RemoteThread main():%s Calling ftd_lg_create()", procname );
		goto errexit;
	}
	if ((rc = ftd_lg_init(lgp, lgnum, ROLESECONDARY, 0)) < 0) {
		error_tracef( TRACEERR, "RemoteThread main():%s Calling ftd_lg_init()", procname );
		goto errexit;
	}

#if defined(_WINDOWS)
	lgp->procp = args->procp;
#else
	strcpy(LG_PROCNAME(lgp), argv[0]);
#endif

	SET_LG_STATE(lgp->flags, state);

	if (ftd_proc_init(lgp->fprocp, FTD_RMD_PATH, procname) < 0) {
		error_tracef( TRACEERR, "RemoteThread main():%s Calling ftd_proc_init()", procname );
		goto errexit;
	}

	// this should go into _init
	lgp->fprocp->proctype = FTD_PROC_RMD;

	if (ftd_sock_init(lgp->isockp, "localhost", "localhost",
		LOCALHOSTIP, LOCALHOSTIP, SOCK_STREAM, AF_INET, 1, 1) < 0)
	{
		error_tracef( TRACEERR, "RemoteThread main():%s Calling ftd_sock_init(isockp)", procname );
		goto errexit;
	}

	if (!apply) {
		// now do real initialization based on config arguments 
		if (ftd_sock_init(lgp->dsockp, lgp->cfgp->shostname,
			lgp->cfgp->phostname, 0, 0, SOCK_STREAM, AF_INET, 0, 0) < 0)
		{
			error_tracef( TRACEERR, "RemoteThread main():%s Calling ftd_sock_init(dsockp)", procname );
    		goto errexit;
		}

		// copy sock handle from temp sock object and
		// clobber the temp sock object 
		FTD_SOCK_FD(lgp->dsockp) = FTD_SOCK_FD(tsockp);
		FTD_SOCK_FD(tsockp) = (SOCKET)-1;

		ftd_sock_set_disconnect(tsockp);
		ftd_sock_set_connect(lgp->dsockp);
		ftd_sock_delete(&tsockp);
	}
	
	if (!(portnum = ftd_sock_get_port(FTD_MASTER))) {
		portnum = FTD_SERVER_PORT;
	}	

	FTD_SOCK_PORT(lgp->dsockp) = portnum;
	FTD_SOCK_PORT(lgp->isockp) = portnum;

	if (ftd_sock_connect(lgp->isockp, portnum) < 0) {
		error_tracef( TRACEERR, "RemoteThread main():%s Calling ftd_sock_connect(isockp) error", procname );
		goto errexit;
	}

#if defined(_WINDOWS)
	FTD_SOCK_HEVENT(lgp->dsockp) = CreateEvent(NULL, FALSE, FALSE, NULL);
			
	if (FTD_SOCK_HEVENT(lgp->dsockp) == WSA_INVALID_EVENT)
	{
		error_tracef( TRACEERR, "RemoteThread main():%s Calling dsockp == WSA_INVALID_EVENT error", procname );
		goto errexit;
	}
	FTD_SOCK_HEVENT(lgp->isockp) = CreateEvent(NULL, FALSE, FALSE, NULL);
			
	if (FTD_SOCK_HEVENT(lgp->isockp) == WSA_INVALID_EVENT)
	{
		error_tracef( TRACEERR, "RemoteThread main():%s Calling isockp == WSA_INVALID_EVENT error", procname );
		goto errexit;
	}
#endif

	memset(&header, 0, sizeof(header));
#if !defined(_WINDOWS)
	// tells master this is a child connect 
	header.cli = getpid();
#else
	header.cli = args->procp->pid;
#endif

	if (FTD_SOCK_SEND_HEADER( FALSE, LG_PROCNAME(lgp),"RemoteThread",lgp->isockp, &header) < 0) {
		error_tracef( TRACEERR, "RemoteThread main():%s Calling FTD_SOCK_SEND_HEADER", procname );
		goto errexit;
	}

	sprintf(prefix, "j%03d", lgp->lgnum);

	if ((lgp->jrnp = ftd_journal_create(lgp->cfgp->jrnpath,
		prefix)) == NULL) {
		error_tracef( TRACEERR, "RemoteThread main():%s : ftd_journal_create()", procname );
		goto errexit;
	}

	if (apply) //RMDA  - remote mirror apply 
		{
		if (ftd_rmd_apply(lgp, cpon) == 0) 
			{
			reporterr(ERRFAC, M_RMDAEND, ERRINFO);
			} 
		else 
			{
			reporterr(ERRFAC, M_RMDAEXIT, ERRINFO);
			}
		} 
	else // ONLY RMD
		{
		#if !defined(_WINDOWS)
			install_sigaction();
		#endif
		// remote mirror 
		
		rc = ftd_rmd(lgp);

		if (rc == FTD_EXIT_DIE) 
			{
			{
			err_msg_t	*lerrmsg;


			if (FTD_SOCK_CONNECT(lgp->dsockp)) 
				{
				lerrmsg = ftd_get_last_error(ERRFAC);
				
				if (strstr(lerrmsg->msg, "[FATAL /")) 
					{
					// report the FATAL error to peer
					ftd_sock_send_err(lgp->dsockp, lerrmsg);
					} 
				else 
					{
					// just tell peer not to try to reconnect
					memset(&header, 0, sizeof(header));
					header.msgtype = FTDACKKILL;
					FTD_SOCK_SEND_HEADER( FALSE,LG_PROCNAME(lgp) ,"RemoteThread",lgp->dsockp, &header);
					}
				// wait for the connection to be closed by the peer 
				rc = ftd_sock_wait_for_peer_close(lgp->dsockp);
				}
			}
			}
		reporterr(ERRFAC, M_RMDEXIT, ERRINFO);
		} // only RMD

	#if defined(_WINDOWS)
		closesocket(sock);
		CloseHandle(FTD_SOCK_HEVENT(lgp->dsockp));
		CloseHandle(FTD_SOCK_HEVENT(lgp->isockp));
		error_tracef( TRACEWRN, "RemoteThread main():%s:EXIT, rc == %d, Thread %d", procname, rc, GetCurrentThreadId() );
	#endif

	ftd_sock_delete(&tsockp);
	ftd_lg_delete(lgp);
	ftd_delete_errfac();

	Return(FTD_EXIT_DIE);

errexit:

		if (FTD_SOCK_VALID(tsockp)) 
			{
			ftd_sock_send_err(tsockp, ftd_get_last_error(ERRFAC));
			
			rc = ftd_sock_wait_for_peer_close(tsockp);

			if (FTD_SOCK_FD(tsockp) != (SOCKET)-1) 
				{
				#if defined(_WINDOWS)
							closesocket(FTD_SOCK_FD(tsockp));
				#else
							close(FTD_SOCK_FD(tsockp));
				#endif
				}
			
			ftd_sock_delete(&tsockp);
			}

		if (lgp)
			{
			#if defined(_WINDOWS)
			closesocket(sock);

			if (FTD_SOCK_HEVENT(lgp->dsockp)) 
				{
				CloseHandle(FTD_SOCK_HEVENT(lgp->dsockp));
				}
			if (FTD_SOCK_HEVENT(lgp->isockp)) 
				{
				CloseHandle(FTD_SOCK_HEVENT(lgp->isockp));
				}
			#endif
			ftd_lg_close(lgp);
			ftd_lg_delete(lgp);
			}	

		if (apply) 
			{
			reporterr(ERRFAC, M_RMDAEXIT, ERRINFO);
			} 
		else 
			{
			reporterr(ERRFAC, M_RMDEXIT, ERRINFO);
			}

		ftd_delete_errfac();

		Return(FTD_EXIT_DIE);

//errexit:
}