/*
 * ftd_proc.c - FTD proc interface
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

#include "ftd_proc.h"
#include "ftd_error.h"
#include "ftd_config.h"
#include "ftd_sock.h"
#include "ftd_lg_states.h"
#include "volmntpt.h"

/*
 * ftd_proc_create_list -- create a linked list of ftd_proc_t objects 
 */
LList *
ftd_proc_create_list(void)
{

	LList *proclist = CreateLList(sizeof(ftd_proc_t**));
	
	return proclist;
}

/*
 * ftd_proc_delete_list -- delete the list of ftd_proc_t objects 
 */
int
ftd_proc_delete_list(LList *proclist)
{
	ftd_proc_t	**procpp;

	ForEachLLElement(proclist, procpp) {
		ftd_proc_delete((*procpp));
	}
	
	FreeLList(proclist);
	
	return 0;
}

/*
 * ftd_proc_add_to_list --
 * add ftd_proc_t object to linked list of ftd_proc_t objects 
 */
int
ftd_proc_add_to_list(LList *proclist, ftd_proc_t **fprocp)
{

	AddToTailLL(proclist, fprocp);

	return 0;
}

/*
 * ftd_proc_remove_from_list --
 * remove ftd_proc_t object from linked list of ftd_proc_t objects 
 */
int
ftd_proc_remove_from_list(LList *proclist, ftd_proc_t **fprocp)
{

	RemCurrOfLL(proclist, fprocp);

	return 0;
}

/*
 * ftd_proc_create -- create a ftd_proc_t object
 */
ftd_proc_t *
ftd_proc_create(int type)
{
	ftd_proc_t *fprocp;

	if ((fprocp = (ftd_proc_t*)calloc(1, sizeof(ftd_proc_t))) == NULL) {
		goto errret;
	}
	if ((fprocp->procp = proc_create(type)) == NULL) {
		goto errret;
	}
	if ((fprocp->fsockp = ftd_sock_create(FTD_SOCK_GENERIC)) == NULL) {
		goto errret;
	}
	
	if ((fprocp->csockplist = CreateLList(sizeof(ftd_sock_t**))) == NULL) {
		goto errret;
	}

	fprocp->magicvalue = FTDPROCMAGIC;

	return fprocp;

errret:

	if (fprocp && fprocp->procp) {
		free(fprocp->procp);
	}
	if (fprocp) {
		free(fprocp);
	}

	return NULL;
}

/*
 * ftd_proc_init -- initialize the ftd_proc_t object
 */
int
ftd_proc_init(ftd_proc_t *fprocp, char *command, char *argv0)
{

	if (proc_init(fprocp->procp, command, argv0) < 0) {
		return -1;
	}

	return 0;
}

/*
 * ftd_proc_delete -- delete a ftd_proc_t object
 */
int
ftd_proc_delete(ftd_proc_t *fprocp)
{
	ftd_sock_t		**csockpp;

	if (fprocp && fprocp->magicvalue != FTDPROCMAGIC) {
		// not a valid proc object
		return -1;
	}
	
	if (fprocp->procp) {
		proc_delete(fprocp->procp);
	}
	
	if (fprocp->fsockp) {
		ftd_sock_delete(&fprocp->fsockp);
	}

	ForEachLLElement(fprocp->csockplist, csockpp) {
		ftd_sock_delete(csockpp);
	}
	
	FreeLList(fprocp->csockplist);

	free(fprocp);

	return 0;
}

/*
 * ftd_proc_signal -- send a signal to the target process
 */
int
ftd_proc_signal(char *procname, int sig)
{
	proc_t	*procp;
	int		pcnt;

#if defined(_WINDOWS)	
	if ((procp = proc_create(PROCTYPE_THREAD)) == NULL) {
		return -1;
	}
#else
	if ((procp = proc_create(PROCTYPE_PROC)) == NULL) {
		return -1;
	}
#endif

	if (proc_init(procp, "", procname) < 0) {
		proc_delete(procp);
		return -1;
	}
	procp->pid = proc_get_pid(procname, 1, &pcnt);	

	if (!pcnt) {
		// not running
		proc_delete(procp);
		return 0;
	}

	if (proc_signal(procp, sig) < 0) {
		proc_delete(procp);
		return -1;
	}

	proc_delete(procp);

	return 1;
}

/*
 * ftd_proc_wait_for_child_connect --
 * accept connection from child. return error if child dies 
 */
int
ftd_proc_wait_for_child_connect(LList *proclist, ftd_sock_t *listener,
	ftd_proc_t *fprocp, int sec, int usec)
{
	int			rc, found;
	ftd_proc_t	**fpp;

	while (1) {
		rc = sock_accept_nonb(listener->sockp, fprocp->fsockp->sockp,
			0, 200000);
		if (rc < 0) {
			return rc;
		} else if (rc == 0) {
			// has proc gone away ?
			found = 0;
			ForEachLLElement(proclist, fpp) {
				if ((*fpp)->procp->pid == fprocp->procp->pid) {
					found = 1;
					break;
				}
			}
			if (!found) {
				return -1;
			}
		} else {
			break;
		}
	}

	return 0;
}

/*
 * ftd_proc_exec_pmd -- execute the target pmd process
 */
int
ftd_proc_exec_pmd(LList *proclist, ftd_proc_t *fprocp, ftd_sock_t *listener,
	int lgnum, int state, int consleep, char **envp)
{
    char		**argv = NULL, **envv = NULL;
	int			i, rc;

	if ((argv = (char**)calloc(2, sizeof(char*))) == NULL) {
		goto errexit;
	}
	argv[0] = (char*)malloc(256 * sizeof(char));
	sprintf(argv[0], "PMD_%03d", lgnum);
	argv[1] = (char*)NULL;

#if !defined(_WINDOWS)
    i = 0;
	while (envp[i]) {
		if ((envv = (char**)realloc(envv, (i+1)*sizeof(char*))) == NULL) {
			goto errexit;
		}
		envv[i] = strdup(envp[i]);
		i++;
	}

	if (consleep > 0) {
		if ((envv = (char**)realloc(envv, (i+2)*sizeof(char*))) == NULL) {
			goto errexit;
		}
		envv[i] = (char*)calloc(32, sizeof(char));
		sprintf(envv[i], "_FTD_CONSLEEP=%d\n", consleep);
		i++;
	}

	if ((envv = (char**)realloc(envv, (i+2)*sizeof(char*))) == NULL) {
		goto errexit;
	}
	
	envv[i] = (char*)calloc(32, sizeof(char));
	sprintf(envv[i], "_FTD_STATE=%d\n", state);
	
	i++;
	envv[i] = (char*)NULL;

#else
    fprocp->args.procp = fprocp->procp;
    fprocp->args.lgnum = lgnum;
    fprocp->args.state = state;
	fprocp->args.consleep = consleep;
#endif

	ftd_proc_init(fprocp, FTD_PMD_PATH, argv[0]);

	fprocp->lgnum = lgnum;
	fprocp->proctype = FTD_PROC_PMD;
	fprocp->state = state;

#if defined(_WINDOWS)
	fprocp->procp->command = (LPDWORD)&fprocp->args;
#endif

	if ((proc_exec(fprocp->procp, argv, envv, 0)) != 0) {
		// exec error
        reporterr(ERRFAC, M_EXEC, ERRWARN, argv[0], ftd_strerror());
		goto errexit;
	}
	
	rc = 0;

	goto fini;

errexit:

	rc = -1;

fini:

	if (argv) {
		i = 0;
		while (argv[i]) {
			free(argv[i]);
			i++;
		}
		free(argv);
	}
	if (envv) {
		i = 0;
		while (envp[i]) {
			free(envv[i]);
			i++;
		}
		free(envv);
	}

	return rc;
}

/*
 * ftd_proc_exec_rmd -- execute the target rmd process
 */
int
ftd_proc_exec_rmd(LList *proclist, ftd_proc_t *fprocp, ftd_sock_t *fsockp, 
	ftd_sock_t *listener, int lgnum, char **envp)
{
    SOCKET		sock;
    char		**argv = NULL, **envv = NULL;
	int			i, rc;
	ftd_proc_t	**fprocpp;

	if ((argv = (char**)calloc(5, sizeof(char*))) == NULL) {
		goto errexit;
	}
	argv[0] = (char*)malloc(256 * sizeof(char));
	sprintf(argv[0], "RMD_%03d", lgnum);
	argv[1] = (char*)NULL;

	// if it's already running then skip it
	ForEachLLElement(proclist, fprocpp) {
		if (!strcmp((*fprocpp)->procp->procname, argv[0])) {
			goto errexit;
		}
	}

#if !defined(_WINDOWS)
	i = 0;
	while (envp[i]) {
		if ((envv = (char**)realloc(envv, (i+1)*sizeof(char*))) == NULL) {
			goto errexit;
		}
		envv[i] = strdup(envp[i]);
		i++;
	}

	if ((envv = (char**)realloc(envv, (i+3)*sizeof(char*))) == NULL) {
		goto errexit;
	}
	envv[i] = (char*)malloc(32 * sizeof(char));
	sprintf(envv[i], "_FTD_STATE=%d", fprocp->state);
	i++;
	envv[i] = (char*)malloc(32 * sizeof(char));
	sprintf(envv[i], "_FTD_SOCK=%d", fsockp->sockp->sock);
	i++;
	envv[i] = (char*)NULL;

#else
    fprocp->args.procp = fprocp->procp;
    fprocp->args.lgnum = lgnum;
    fprocp->args.state = fprocp->state;
    fprocp->args.apply = 0;

    DuplicateHandle(GetCurrentProcess(), (HANDLE)fsockp->sockp->sock, 
        GetCurrentProcess(), (HANDLE*)&sock, 0, TRUE, DUPLICATE_SAME_ACCESS);
	
	// TODO:  close the original?
	// send the dup and save the dup to be closed by parent
    fprocp->args.dsock = sock;
#endif

	ftd_proc_init(fprocp, FTD_RMD_PATH, argv[0]);

	fprocp->lgnum = lgnum;
	fprocp->proctype = FTD_PROC_RMD;

    if ((proc_exec(fprocp->procp, argv, envv, 0)) != 0) {
		// exec error
        reporterr(ERRFAC, M_EXEC, ERRWARN, argv[0], ftd_strerror());
		goto errexit;
	}

	rc = 0;

	goto fini;

errexit:

	rc = -1;

fini:

	if (argv) {
		i = 0;
		while (argv[i]) {
			free(argv[i]);
			i++;
		}
		free(argv);
	}
	if (envv) {
		i = 0;
		while (envp[i]) {
			free(envv[i]);
			i++;
		}
		free(envv);
	}

	return rc;
}

/*
 * ftd_proc_exec_apply -- execute the target apply process
 */
int
ftd_proc_exec_apply(ftd_proc_t *fprocp, ftd_sock_t *listener, int lgnum,
	int cpon, char **envp)
{
    char	**argv = NULL, **envv = NULL;
	int		i, rc;

	if ((argv = (char**)calloc(2, sizeof(char*))) == NULL) {
		goto errexit;
	}
	argv[0] = (char*)malloc(256 * sizeof(char));
	sprintf(argv[0], "RMDA_%03d", lgnum);
	argv[1] = (char*)NULL;

#if !defined(_WINDOWS)
	i = 0;
	while (envp[i]) {
		if ((envv = (char**)realloc(envv, (i+1)*sizeof(char*))) == NULL) {
			goto errexit;
		}
		envv[i] = strdup(envp[i]);
		i++;
	}

	if ((envv = (char**)realloc(envv, (i+3)*sizeof(char*))) == NULL) {
		goto errexit;
	}
	envv[i] = (char*)malloc(32 * sizeof(char));
	sprintf(envv[i], "_FTD_APPLY=1");
	i++;

	if (cpon) {
		envv[i] = (char*)malloc(32 * sizeof(char));
		sprintf(envv[i], "_FTD_CPON=1");
		i++;
	}
	envv[i] = (char*)NULL;

#else
    fprocp->args.procp = fprocp->procp;
    fprocp->args.lgnum = lgnum;
    fprocp->args.apply = 1;
	fprocp->args.cpon = cpon;
    fprocp->args.dsock = (SOCKET)-1;
#endif

    ftd_proc_init(fprocp, FTD_RMD_PATH, argv[0]);
	fprocp->lgnum = lgnum;
	fprocp->proctype = FTD_PROC_APPLY;

	if ((proc_exec(fprocp->procp, argv, envv, 0)) != 0) {
		// exec error
		reporterr(ERRFAC, M_EXEC, ERRWARN, argv[0], ftd_strerror());
		goto errexit;
	}

	rc = 0;

	goto fini;

errexit:

	rc = -1;

fini:

	if (argv) {
		i = 0;
		while (argv[i]) {
			free(argv[i]);
			i++;
		}
		free(argv);
	}
	if (envv) {
		i = 0;
		while (envp[i]) {
			free(envv[i]);
			i++;
		}
		free(envv);
	}

	return rc;
}

/*
 * ftd_proc_exec_throt -- execute the target throttle-eval process
 */
int
ftd_proc_exec_throt(ftd_proc_t *fprocp, int wait, char **envp)
{
    char	**argv = NULL, **envv = NULL;
	int		i, rc;

#if !defined(_WINDOWS)
	/* only allow one of these guys */
	ftd_proc_kill(FTD_THROT);
#endif
	
	if ((argv = (char**)calloc(2, sizeof(char*))) == NULL) {
		goto errexit;
	}
	argv[0] = (char*)malloc(256 * sizeof(char));
	sprintf(argv[0], "%s", FTD_THROT);
	argv[1] = (char*)NULL;

#if !defined(_WINDOWS)
	i = 0;
	while (envp[i]) {
		if ((envv = (char**)realloc(envv, (i+1)*sizeof(char*))) == NULL) {
			goto errexit;
		}
		envv[i] = strdup(envp[i]);
		i++;
	}
	if ((envv = (char**)realloc(envv, (i+1)*sizeof(char*))) == NULL) {
		goto errexit;
	}
	envv[i] = (char*)NULL;

#else
    fprocp->args.procp = fprocp->procp;
#endif

	ftd_proc_init(fprocp, FTD_THROT_PATH, argv[0]);
	fprocp->proctype = FTD_PROC_THROT;

	if ((proc_exec(fprocp->procp, argv, envv, 0)) != 0) {
		// exec error
		reporterr(ERRFAC, M_EXEC, ERRWARN,
			argv[0], ftd_strerror());
		goto errexit;
	}

	rc = 0;

	goto fini;

errexit:

	rc = -1;

fini:

	if (argv) {
		i = 0;
		while (argv[i]) {
			free(argv[i]);
			i++;
		}
		free(argv);
	}
	if (envv) {
		i = 0;
		while (envp[i]) {
			free(envv[i]);
			i++;
		}
		free(envv);
	}

	return rc;
}

/*
 * ftd_proc_exec -- execute the target command
 */
int
ftd_proc_exec(char *command, int wait)
{
	proc_t		*procp;
    char		**argv = NULL, **envv = NULL;
	int			i, rc;

	if ((procp = proc_create(PROCTYPE_PROC)) == NULL) {
		return -1;
	}
	if (proc_init(procp, command, "") < 0) {
		goto errexit;
	}
	if ((argv = (char**)calloc(2, sizeof(char*))) == NULL) {
		goto errexit;
	}
	if ((envv = (char**)calloc(1, sizeof(char*))) == NULL) {
		goto errexit;
	}
	argv[0] = (char*)malloc(256 * sizeof(char));
	sprintf(argv[0], "%s", command);
	argv[1] = (char*)NULL;

	envv[0] = (char*)NULL;

	if ((proc_exec(procp, argv, envv, wait)) != 0) {
		// exec error
		reporterr(ERRFAC, M_EXEC, ERRWARN, command, ftd_strerror());
		goto errexit;
	}

	rc = 0;

	goto fini;

errexit:

	rc = -1;

fini:

	proc_delete(procp);

	if (argv) {
		i = 0;
		while (argv[i]) {
			free(argv[i]);
			i++;
		}
		free(argv);
	}
	if (envv) {
		i = 0;
		while (envv[i]) {
			free(envv[i]);
			i++;
		}
		free(envv);
	}

	return rc;
}

/*
 * ftd_proc_terminate -- terminate, kindly, the target process
 */
int
ftd_proc_terminate(char *procname)
{
	proc_t	*procp;
	int		pcnt, tries;

#if defined(_WINDOWS)
	if ((procp = proc_create(PROCTYPE_THREAD)) == NULL) {
		return -1;
	}
#else
	if ((procp = proc_create(PROCTYPE_PROC)) == NULL) {
		return -1;
	}
#endif

	if (proc_init(procp, "", procname) < 0) {
		proc_delete(procp);
		return -1;
	}
	tries = 0;

	while ((procp->pid = proc_get_pid(procname, 1, &pcnt)) > 0) {
		proc_terminate(procp);
		if (++tries == 5) {
			break;
		}
		sleep(1);	
	}

	proc_delete(procp);

	return 0;
}

/*
 * ftd_proc_kill -- terminate, rudely, the target process
 */
int
ftd_proc_kill(char *procname)
{
	proc_t	*procp;
	int		pcnt;

#if defined(_WINDOWS)
	if ((procp = proc_create(PROCTYPE_THREAD)) == NULL) {
		return -1;
	}
#else
	if ((procp = proc_create(PROCTYPE_PROC)) == NULL) {
		return -1;
	}
#endif

	if (proc_init(procp, "", procname) < 0) {
		proc_delete(procp);
		return -1;
	}
	while (procp->pid = proc_get_pid(procname, 1, &pcnt)) {
		proc_kill(procp);
	}

	proc_delete(procp);

	return 0;
}

/*
 * ftd_proc_kill_all_pmd -- kill all pmd procs in proc list
 */
int
ftd_proc_kill_all_pmd(LList *proclist)
{
	ftd_proc_t **fprocpp;

	ForEachLLElement(proclist, fprocpp) {
		if ((*fprocpp)->proctype == FTD_PROC_PMD) {
			proc_terminate((*fprocpp)->procp);
		}
	}

	return 0;
}

/*
 * ftd_proc_kill_all_rmd -- kill all rmd procs in proc list
 */
int
ftd_proc_kill_all_rmd(LList *proclist)
{
	ftd_proc_t **fprocpp;

	ForEachLLElement(proclist, fprocpp) {
		if ((*fprocpp)->proctype == FTD_PROC_RMD) {
			proc_terminate((*fprocpp)->procp);
		}
	}

	return 0;
}

/*
 * ftd_proc_kill_all_apply -- kill all journal-apply procs in proc list
 */
int
ftd_proc_kill_all_apply(LList *proclist)
{
	ftd_proc_t **fprocpp;

	ForEachLLElement(proclist, fprocpp) {
		if ((*fprocpp)->proctype == FTD_PROC_APPLY) {
			proc_kill((*fprocpp)->procp);
		}
	}

	return 0;
}

/*
 * ftd_proc_kill_all -- kill all procs in proc list
 */
int
ftd_proc_kill_all(LList *proclist)
{
	ftd_proc_t **fprocpp;

	ForEachLLElement(proclist, fprocpp) {
		proc_kill((*fprocpp)->procp);
	}

	return 0;
}

/*
 * ftd_lgnum_to_proc --
 * return target ftd_proc_t object for the lgnum/role 
 */
ftd_proc_t *
ftd_proc_lgnum_to_proc(LList *proclist, int lgnum, int role)
{
	ftd_proc_t **fprocpp;

	ForEachLLElement(proclist, fprocpp) {
		if ((*fprocpp)->lgnum == lgnum) {
			if ((*fprocpp)->proctype == FTD_PROC_PMD 
			&& role == ROLEPRIMARY) { 
				return *fprocpp;
			} else if ((*fprocpp)->proctype == FTD_PROC_RMD 
			&& role == ROLESECONDARY) { 
				return *fprocpp;
			} 
		}
	}

	return NULL;
}

/*
 * ftd_pid_to_proc -- return target ftd_proc_t object for the pid
 */
ftd_proc_t *
ftd_proc_pid_to_proc(LList *proclist, pid_t pid)
{
	ftd_proc_t **fprocpp;

	ForEachLLElement(proclist, fprocpp) {
		if ((*fprocpp)->procp->pid == pid) {
			return *fprocpp;
		}
	}

	return NULL;
}

/*
 * ftd_pid_to_proc_addr -- return target ftd_proc_t object address for the pid
 */
ftd_proc_t **
ftd_proc_pid_to_proc_addr(LList *proclist, pid_t pid)
{
	ftd_proc_t **fprocpp;

	ForEachLLElement(proclist, fprocpp) {
		if ((*fprocpp)->procp->pid == pid) {
			return fprocpp;
		}
	}

	return NULL;
}

/*
 * ftd_name_to_proc -- return target ftd_proc_t object for the procname
 */
ftd_proc_t *
ftd_proc_name_to_proc(LList *proclist, char *procname)
{
	ftd_proc_t **fprocpp;

	ForEachLLElement(proclist, fprocpp) {
		if (!strcmp((*fprocpp)->procp->procname, procname)) {
			return *fprocpp;
		}
	}

	return NULL;
}

/*
 * ftd_pid_to_proc -- return target ftd_proc_t object for the pid
 */
pid_t
ftd_proc_get_pid(char *procname, int exactmatch, int *pcnt)
{

	return proc_get_pid(procname, exactmatch, pcnt);
}

/*
 * ftd_proc_hup_pmds --
 * send a hup msg to all pmd children for purpose of re-initializing them
 * start daemons for any started groups that are not currently up
 */
int
ftd_proc_hup_pmds(LList *proclist, ftd_sock_t *fsockp, int consleep,
	char **envp)
{
	ftd_proc_t		**fprocpp, *fprocp;
	ftd_lg_cfg_t	**cfgpp, *cfgp;
	int				found;
	LList			*cfglist;
	ftd_header_t	header;

	if ((cfglist = CreateLList(sizeof(ftd_lg_cfg_t))) == NULL) {
		return -1;
	}

	ftd_config_get_primary_started(PATH_CONFIG, cfglist);

	ForEachLLElement(cfglist, cfgpp) {
		cfgp = *cfgpp;
		
		found = 0;
		// is is running ?

		ForEachLLElement(proclist, fprocpp) {
			if (cfgp->lgnum == (*fprocpp)->lgnum) {
				found = 1;
				break;
			}
		}
		
		if (found) {
			memset(&header, 0, sizeof(header));
			header.magicvalue = MAGICHDR;
			header.msgtype = FTDCHUP;
			header.ackwanted = 1;
			FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"ftd_proc_hup_pmds",fsockp, &header);
		} else {
#if defined(_WINDOWS)
			if ((fprocp = ftd_proc_create(PROCTYPE_THREAD)) == NULL) {
				return -1;
			}
#else
			if ((fprocp = ftd_proc_create(PROCTYPE_PROC)) == NULL) {
				return -1;
			}
#endif
			if (ftd_proc_exec_pmd(proclist, fprocp, fsockp,
				cfgp->lgnum, fprocp->state, 0, envp) < 0) {
				continue;
			}
			
			ftd_proc_add_to_list(proclist, &fprocp);
		}
	}

	FreeLList(cfglist);

	return 0;
}

/*
 * ftd_proc_do_command --  get the ftd command from the client 
 */
int
ftd_proc_do_command(ftd_sock_t *fsockp, char *msg)
{
#if !defined(_WINDOWS)
	char	tbuf[256];
#endif
	char	*errbuf, sizestring[256];
	int		numerrmsgs, errbufsize, erroffset, rc, i;
 
	i = 3;
	while (1) {
		i++;
		if ((rc = ftd_sock_recv(fsockp, &msg[i], sizeof(char))) < 0) {
			return rc;
		}
		if (msg[i] == '\n' || msg[i] == '\0' || i == 255) {
			msg[i] = '\0';
			break;
		}
	}

	if (!strncmp(msg, " get error messages", 22)) {
		i = 23;
		while (isspace(msg[i])) {
			i++;
		}
		sscanf(msg, "%d", &erroffset);
		
		get_log_msgs(ERRFAC, &errbuf, &errbufsize, &numerrmsgs, &erroffset);
			
		if (errbufsize > 0) {  
			sprintf(sizestring, "%d %d\n", numerrmsgs, erroffset);
			ftd_sock_send ( FALSE,fsockp, sizestring, strlen(sizestring));
			ftd_sock_send ( FALSE,fsockp, "{", 1);
			ftd_sock_send ( FALSE,fsockp, errbuf, strlen(errbuf));
			ftd_sock_send ( FALSE,fsockp, "}\n", 2);
			free(errbuf);
		} else {
			sprintf(sizestring, "0 %d\n{}\n", erroffset);
			ftd_sock_send ( FALSE,fsockp, sizestring, strlen(sizestring));
		}
		return 1;
	} else {
#if defined(_WINDOWS)
		(void)ftd_proc_process_dev_info_request(fsockp, msg);
#else
		if (ftd_proc_process_proc_request(fsockp, msg) == -999) {
			/*
			 * see if this is a process info request, otherwise, a device info
			 * request (it will return a "0\n" message on error)
			 */
			(void)ftd_proc_process_dev_info_request(fsockp, msg);
		}
#endif		
		/* ftd command serviced, return */
		return 1;
	}

}

/*
 * ftd_proc_process_dev_info_request -- start processing a device list request
 */
int
ftd_proc_process_dev_info_request(ftd_sock_t *fsockp, char *msg)
{
#if !defined(_WINDOWS)
	if (!strncmp(msg, "ftd get all devices", 19)) {
		ftd_sock_send ( FALSE,fsockp, "1 ", strlen("1 "));
		
		// get a string of all devs on the system
		capture_all_devs(fsockp->sockp->sock);
		
		ftd_sock_send ( FALSE,fsockp, "\n", strlen("\n"));
	}
#else
	if (!strncmp(msg, "ftd get all devices", 19)) {
		char	szDriveString[_MAX_PATH];
		char	szDrive[4];
		char	szDeviceInfo[100];
		char	szDeviceList[3 * 1024];
		int		i = 0, iDrive;
		ULONGLONG		iTotal;
		ULARGE_INTEGER FreeBytesAvailableToCaller, TotalNumberOfBytes, TotalNumberOfFreeBytes;
		char	szDiskInfo[256];
		char	strWinDir[_MAX_PATH];
		int		iGroupID = -1;
		char	*szGroup;

		szGroup = strrchr(msg, ' ');
		iGroupID = atoi(szGroup);

		GetWindowsDirectory(strWinDir, _MAX_PATH);
		
		memset(szDeviceList, 0, sizeof(szDeviceList));

		memset(szDriveString, 0, sizeof(szDriveString));
		GetLogicalDriveStrings(sizeof(szDriveString), szDriveString);

		while(szDriveString[i] != 0 || szDriveString[i+1] != 0)
		{
			sprintf(szDrive, "%c%c%c", szDriveString[i], szDriveString[i+1], szDriveString[i+2]);
			i = i + 4;

			iDrive = GetDriveType(szDrive);
			
			if(iDrive != DRIVE_REMOVABLE && iDrive != DRIVE_CDROM && iDrive != DRIVE_RAMDISK &&	iDrive != DRIVE_REMOTE)
			{
				if (szDrive[0] != strWinDir[0])
				{
					memset(szDiskInfo, 0, sizeof(szDiskInfo));
					getDiskSigAndInfo(szDrive, szDiskInfo, iGroupID);
					
					//if we get no disk info back set the string to 1 blank for parse
					if(strlen(szDiskInfo) == 0)
					{
						szDiskInfo[0] = ' ';
					}
					
					if (GetDiskFreeSpaceEx(szDrive, &FreeBytesAvailableToCaller, &TotalNumberOfBytes, &TotalNumberOfFreeBytes)) {
						iTotal = TotalNumberOfBytes.QuadPart/1024/1024;
						sprintf(szDeviceInfo, "[-%c-]  %I64dMB {%s\\",szDrive[0], iTotal, szDiskInfo);
					} else {
						sprintf(szDeviceInfo, "[-%c-]  Unknown {%s\\",szDrive[0], szDiskInfo);
					}
					
					strcat(szDeviceList, szDeviceInfo);
					memset(szDeviceInfo, 0, sizeof(szDeviceInfo));
				}

#if !defined(NTFOUR)
				{
					char   szMountPt[_MAX_PATH];
					HANDLE hDrive;

					hDrive = getFirstVolMntPtHandle(&szDrive[0], szMountPt, _MAX_PATH);

					if (hDrive != INVALID_HANDLE_VALUE)
					{ 
						do
						{
							char szMountPtFullName[_MAX_PATH];
							_snprintf(szMountPtFullName, _MAX_PATH, "%c:\\%s", szDrive[0], szMountPt);

							memset(szDiskInfo, 0, sizeof(szDiskInfo));
							getMntPtSigAndInfo(szMountPtFullName, szDiskInfo, iGroupID);

							//if we get no disk info back set the string to 1 blank for parse
							if(strlen(szDiskInfo) == 0)
							{
								szDiskInfo[0] = ' ';
							}

							if (GetDiskFreeSpaceEx(szMountPtFullName, &FreeBytesAvailableToCaller, &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
							{
								iTotal = TotalNumberOfBytes.QuadPart/1024/1024;
								sprintf(szDeviceInfo, "[-%s-]  %I64dMB {%s\\", szMountPtFullName, iTotal, szDiskInfo);
							}
							else
							{
								sprintf(szDeviceInfo, "[-%s-]  Unknown {%s\\", szMountPtFullName, szDiskInfo);
							}

							strcat(szDeviceList, szDeviceInfo);
							memset(szDeviceInfo, 0, sizeof(szDeviceInfo));

						} while (getNextVolMntPt(hDrive, szMountPt, _MAX_PATH) > 0);

						closeVolMntPtHandle(hDrive);
					}
				}
#endif

			}
		}
		{
			int	rc;

			if ((rc = ftd_sock_send ( FALSE,fsockp, szDeviceList, sizeof(szDeviceList))) < 0) {
				return rc;
			}
		}
	}

#endif

	return 0;
}

#if !defined(_WINDOWS)
/*
 * ftd_proc_process_proc_request --
 * write a character buffer containing process name and pid - either
 * for the 1st exact match found or for all processes with a certain
 * prefix string.
 */
int
ftd_proc_process_proc_request(ftd_sock_t *fsockp, char *msg)
{
	int		retval, ebufsize, numemsgs, exactflag, i, len, geterrorsflag;
	int		offset, j, oldoffset;
	char	*buf, *ebuf, prefix[256], offstring[256], outstring[256];
  
	i = j = offset = retval = geterrorsflag = 0;
	offstring[0] = offstring[1] = '\0';

	if (!strncmp(msg, "ftd get process info", 20)) {
		exactflag = 1;
		len = 20;
	} else if (!strncmp(msg, "ftd get all process info", 24)) {
		exactflag = 0;
		geterrorsflag = 0;
		len = 24;
	} else if (!strncmp(msg, "ftd get all process and error info", 34)) {
		exactflag = 0;
		len = 34;
		geterrorsflag = 1;
	} else {
		return -999;
	}

    msg += len;
    while (isspace(*msg)) {
		msg++;
	}
    
	while (isprint(*msg) && !(isspace(*msg))) {
		prefix[i++] = *msg++;
	}

	prefix[i] = '\0';
	if (geterrorsflag) {
		while (isspace(*msg)) {
			msg++;
		}
        while (isprint(*msg) && !(isspace(*msg))) {
			offstring[j++] = *msg++;
		}

		offstring[j] = '\0';
		sscanf(offstring, "%d", &offset);
	}

	*msg = '\0';
	if (geterrorsflag) {
		oldoffset = offset;
		get_log_msgs(ERRFAC, &ebuf, &ebufsize, &numemsgs, &offset);
	}

	if (0 >= (retval = capture_proc_names(prefix, exactflag, &buf))) {
		ftd_sock_send ( FALSE,fsockp, " 0 {} \n", strlen(" 0 {} \n"));
	} else {
		if (retval == 0) {
			free(buf);
			buf = (char *)NULL;
		} else {
			if (geterrorsflag == 0) buf[strlen(buf)] = '\n';
			ftd_sock_send ( FALSE,fsockp, buf, strlen(buf));
			free(buf);
			buf = (char *)NULL;
		}
	} 

	if (geterrorsflag != 0) {
		if (oldoffset == offset) {
			sprintf(outstring, " { ***NOERRORS*** %d } {} \n", offset);
			ftd_sock_send ( FALSE,fsockp, outstring, strlen(outstring));
		} else {
			sprintf(outstring, " { ***ERRORS*** %d } { ", offset);
			ftd_sock_send ( FALSE,fsockp, outstring, strlen(outstring));
			ftd_sock_send ( FALSE,fsockp, ebuf, ebufsize);
			ftd_sock_send ( FALSE,fsockp, " } \n", strlen(" } \n"));
		}
		if (ebufsize > 0) {
			free(ebuf);
		}
	}
    
	return retval;
}    

/*
 * ftd_proc_daemon_init -- initialize process as a daemon 
 */
int
ftd_proc_daemon_init(void) 
{
	pid_t pid;

	if ((pid = fork()) < 0) {
		return -1;
	} else if (pid != 0) {
		exit(0);
	}
	setsid();

	chdir("/");

	umask(0);

    return 0;
}

/*
 * ftd_proc_reaper -- handle exiting children 
 */
int
ftd_proc_reaper(LList *proclist, ftd_sock_t *netsockp, char **envp)
{
	ftd_proc_t	**fprocpp, *fprocp;
	int			status, exitstatus, exitsignal, exitduetostatus, exitduetosignal;
	int			tries, maxtries, j, pmd_exited, lgnum;
	pid_t		pid;

	tries = 0;
	maxtries = 3;

	while(1) {
		pid = waitpid(0, &status, WNOHANG);

		if (pid == -1) {
			switch(errno) {
			case EINTR:
				continue;
			case ECHILD:
				// no more children 
				return 0;
			default:
				if ((fprocpp = ftd_proc_pid_to_proc_addr(proclist, pid)) == NULL) {
					continue;
				}
				reporterr(ERRFAC, M_ABEXIT, ERRCRIT, fprocp->procp->procname);

				ftd_proc_remove_from_list(proclist, fprocpp);

				ftd_proc_delete(*fprocpp);

				continue;
			}
		} else if (pid == 0) {
			if (tries++ >= maxtries) {
				break;
			}
			usleep(100000);
			continue;
		} else {
			if ((fprocpp = ftd_proc_pid_to_proc_addr(proclist, pid)) == NULL) {
				// nothing more we can do
				continue;
			}
		}

		// have exited fprocpp

		// handle throtd relaunch, if necessary 
		if (!strcmp((*fprocpp)->procp->procname, FTD_THROT)) {
			reporterr (ERRFAC, M_RETHROTD, ERRWARN);
			
			ftd_proc_remove_from_list(proclist, fprocpp);
			
			if ((fprocp = ftd_proc_create(PROCTYPE_PROC)) == NULL) {
				continue;
			}
			(void)ftd_proc_exec_throt(fprocp, 0, envp);  

			ftd_proc_add_to_list(proclist, &fprocp);

			continue;
		}

		/* reap child daemons */
		pmd_exited = 0;

		ForEachLLElement(proclist, fprocpp) 
			{
			fprocp = *fprocpp;
			if (fprocp->procp->pid == pid) 
				{
				if (fprocp->proctype == FTD_PROC_PMD) 
					{
					pmd_exited = 1;
					error_tracef( TRACEINF, "ftd_proc_reaper():PMD: %d", fprocp->lgnum );
					} 
				else if (fprocp->proctype == FTD_PROC_RMD) 
					{
					error_tracef( TRACEINF, "ftd_proc_reaper():RMD: %d", fprocp->lgnum );
					}
			   	break;
				}
    	    }

		// save lgnum
		lgnum = (*fprocpp)->lgnum;

		ftd_proc_remove_from_list(proclist, fprocpp);
		ftd_proc_delete(*fprocpp);

		if (!pmd_exited) {
			continue;
		}
        
		// relaunch PMD if it exited with reasonable status 
		exitstatus = 0;
		exitsignal = 0;
		exitduetostatus = 0;
		exitduetosignal = 0;
		exitduetostatus = WIFEXITED(status);
		exitduetosignal = WIFSIGNALED(status);
		
		if (exitduetostatus) {
			exitstatus = WEXITSTATUS(status);
		}
		if (exitduetosignal) {
			exitsignal = WTERMSIG(status);
		}

		error_tracef( TRACEINF, "ftd_proc_reaper():child exited, exit status=%d signal=%d", exitstatus, exitsignal );

		if (exitsignal == 0) {
			if (exitstatus == FTD_EXIT_RESTART) {
				// start a new pmd for the group here
				if ((fprocp = ftd_proc_create(PROCTYPE_PROC)) == NULL) {
					continue;
				}

				fprocp->lgnum = lgnum;

				if (ftd_proc_exec_pmd(proclist, fprocp, netsockp,
					fprocp->lgnum, FTD_SNORMAL, 5, envp) < 0)
				{
					ftd_proc_delete(fprocp);
					continue;
				}

				ftd_proc_add_to_list(proclist, &fprocp);
			} 
		} else if (exitsignal != SIGTERM) {
			reporterr (ERRFAC, M_SIGNAL, ERRCRIT,
				fprocp->procp->procname, exitsignal);

			ftd_proc_remove_from_list(proclist, fprocpp);

			ftd_proc_delete((*fprocpp));
		} // exitsignal == 0 
	} // while 

	return 0;
}

#else // #if !defined(_WINDOWS)


/*
 * ftd_proc_reaper -- handle exiting children 
 */
int
ftd_proc_reaper(LList *proclist, ftd_proc_t	**fprocpp, ftd_sock_t *netsockp, char **envp)
{
	ftd_proc_t		*fprocp;
	ftd_header_t	header;
	DWORD			exitstatus;
	void (__cdecl *function)(void *);
	int				lgnum, restart = FALSE;
	LPDWORD			command;

	GetExitCodeThread((*fprocpp)->procp->pid, &exitstatus);

	// reap child
	if ((*fprocpp)->proctype == FTD_PROC_PMD) 
		{
		restart = FTD_PROC_PMD;

		error_tracef( TRACEINF, "ftd_proc_reaper():PMD:%d", (*fprocpp)->lgnum );
		} 
	else if ((*fprocpp)->proctype == FTD_PROC_RMD) 
		{
		error_tracef( TRACEINF, "ftd_proc_reaper():RMD:%d", (*fprocpp)->lgnum );
		} 
	else if ((*fprocpp)->proctype == FTD_PROC_APPLY) 
		{
		error_tracef( TRACEINF, "ftd_proc_reaper():RMDA:%d", (*fprocpp)->lgnum );
		
		// tell rmd - just send it a NOOP in case it's waiting
		if ((fprocp = ftd_proc_lgnum_to_proc(proclist, (*fprocpp)->lgnum, ROLESECONDARY))) 
			{
			memset(&header, 0, sizeof(header));
			header.msgtype = FTDCNOOP;
			

			if (FTD_SOCK_CONNECT(fprocp->fsockp)) 
				{
				FTD_SOCK_SEND_HEADER( FALSE,__FILE__ ,"ftd_proc_reaper",fprocp->fsockp, &header);
				}
			}
	   	} 
	else if ((*fprocpp)->proctype == FTD_PROC_THROT) 
		{
		restart = FTD_PROC_THROT;
	    error_tracef( TRACEINF, "ftd_proc_reaper():RMD:STATD" );
		}  

	// save lgnum
	lgnum = (*fprocpp)->lgnum;

	ftd_proc_remove_from_list(proclist, fprocpp);

#if defined(_WINDOWS)
	// save function too
	function = (*fprocpp)->procp->function; 
	command = (*fprocpp)->procp->command; 
#endif

	// now clobber it
	ftd_proc_delete((*fprocpp));

	if (!restart) {
		return 0;
	}

	switch(restart) {
	case FTD_PROC_PMD:

		// relaunch PMD if it exited with reasonable status 
		error_tracef( TRACEINF, "ftd_proc_reaper(): Relaunch PMD_%d, exit status was %d", lgnum, exitstatus );

		if (exitstatus == FTD_EXIT_RESTART) {
			reporterr (ERRFAC, M_REPMD, ERRWARN, lgnum);
			
			// start a new pmd for the group here
			if ((fprocp = ftd_proc_create(PROCTYPE_THREAD)) == NULL) {
				return -1;
			}
			
			// restore state
			fprocp->procp->function = function; 
			fprocp->lgnum = lgnum;

			if (ftd_proc_exec_pmd(proclist, fprocp, netsockp,
				fprocp->lgnum, FTD_SNORMAL, 5, envp) < 0)
			{
				ftd_proc_delete(fprocp);
				return -1;
			}

			ftd_proc_add_to_list(proclist, &fprocp);
		} 
		break;
	case FTD_PROC_THROT:

		reporterr (ERRFAC, M_RETHROTD, ERRWARN);

		if ((fprocp = ftd_proc_create(PROCTYPE_THREAD)) == NULL) {
			return -1;
		}
					
		fprocp->procp->function = function;
		fprocp->procp->command = command;
					
		(void)ftd_proc_exec_throt(fprocp, 0, envp);  

		ftd_proc_add_to_list(proclist, &fprocp);
		
		break;
	default:
		break;
	}

	return 0;
}

#endif


