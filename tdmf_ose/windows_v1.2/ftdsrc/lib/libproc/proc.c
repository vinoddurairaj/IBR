/*
 * proc.c - proc interface
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
#include <stdio.h>
#include <stdlib.h>
#if !defined(_WINDOWS)
#include <signal.h>
#include <errno.h>
#include <wait.h>
#else
#include "ftd_port.h"
#include "ftd_sock.h"
#endif
#include "procinfo.h"
#include "proc.h"


proc_t *
proc_create(int type)
{
	proc_t *procp;

	if ((procp = (proc_t*)calloc(1, sizeof(proc_t))) == NULL) {
		return NULL;	
	}

#if defined(_WINDOWS)
	procp->pid = INVALID_HANDLE_VALUE;
#endif

	procp->type = type;

	return procp;
}

int
proc_delete(proc_t *procp)
{

#if defined(_WINDOWS)
	if (procp->pid != INVALID_HANDLE_VALUE)
		CloseHandle(procp->pid);

	if (procp->hEvent)
		CloseHandle(procp->hEvent);
#endif

	free(procp);

	return 0;
}

int
proc_init(proc_t *procp, char *commandline, char *procname)
{

	strcpy(procp->procname, procname);

#if !defined(_WINDOWS)
	strcpy(procp->command, commandline);
#endif

	return 0;
}

#ifdef _WINDOWS

int
proc_signal(proc_t *procp, int sig)
{

	if (procp)
	{
		procp->nSignal = sig;

		if (procp->hEvent)
		{
			SetEvent(procp->hEvent);
		} 
		else
		{
			return -1;
		}
	}
	else
		return -1;

	switch(sig)
	{
	case FTDCSIGTERM:
		if ( WaitForSingleObject(procp->pid, 60000) == WAIT_TIMEOUT ) {
			// no more nice guy
			TerminateThread(procp->pid, FTD_EXIT_DIE);
		}
	}

	return 0;
}

int 
proc_exec(proc_t *procp, char **argv, char **envp, int wait)
{
	
	procp->hEvent = CreateEvent (NULL, TRUE, FALSE, NULL); // manual, not signaled
	
	if (!procp->hEvent)
		return -1;

	if (procp->type == PROCTYPE_THREAD) {

		if ( procp->function == NULL )
		{
			return -1;
		}

		procp->pid = CreateThread(	NULL,
									0,
									(LPTHREAD_START_ROUTINE)procp->function,
									procp->command,
									CREATE_SUSPENDED,
									&procp->dwId);

		if ( !procp->pid )
		{
			procp->pid = INVALID_HANDLE_VALUE;
			
			goto errret;
		}

#ifdef _DEBUG
		{
			char buf[256];

			sprintf(buf, "Thread: %s, id: %02x\n", procp->procname, procp->dwId);
			
			OutputDebugString(buf);
			
			printf("\nThread: %s, id: %02x\n", procp->procname, procp->dwId);
		}
#endif
                //
                // Set priority according to GUI not absolute value!
                //
		SetThreadPriority(procp->pid, THREAD_PRIORITY_ABOVE_NORMAL);

		ResumeThread(procp->pid); 

	} else {
	
		int					rc;
		PROCESS_INFORMATION	TProcInfo;
		STARTUPINFO			TStartupInfo;
		SECURITY_DESCRIPTOR sd;
		SECURITY_ATTRIBUTES sa;

		// Initialize a security descriptor and assign it a NULL 
		// discretionary ACL to allow unrestricted access. 
		// Assign the security descriptor to a file. 
		if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
			goto errret;
		if (!SetSecurityDescriptorDacl(&sd, TRUE, (PACL) NULL, FALSE))
			goto errret;
		sa.nLength = sizeof(sa);
		sa.lpSecurityDescriptor = &sd;
		sa.bInheritHandle = FALSE;

		GetStartupInfo(&TStartupInfo);
		memset(&TProcInfo, 0, sizeof(TProcInfo));

		{
			char	*s, *path, *cmd = argv[0];

			while ((s = strstr(cmd, "\\"))
				|| (s = strstr(cmd, "/")))
			{
				cmd = ++s;
			}

			path = strdup(argv[0]);
			path[cmd - argv[0]] = '\0';

//printf("\n$$$ argv[0], cmd, path = %s, %s, %s\n",
//	   argv[0], cmd, path);

			rc = CreateProcess(	NULL,
					cmd,
					&sa,
					NULL,
					FALSE,
					CREATE_NEW_CONSOLE,
					NULL,
					path,
					&TStartupInfo,
					&TProcInfo);

		}
		
		if ( !rc )
		{
			procp->pid = INVALID_HANDLE_VALUE;
			goto errret;
		}

		procp->dwId = TProcInfo.dwProcessId;
		procp->pid = TProcInfo.hProcess;

		if (wait) {
			if ((rc = proc_wait(procp, -1)) != 0) {
				goto errret;
			}
		}

#ifdef _DEBUG
		{
			char buf[256];

			sprintf(buf, "Process: %s, id: %02x\n", argv[0], procp->dwId);
			
			OutputDebugString(buf);
			
			// DPRINTF((ERRFAC,LOG_INFO,("\nProcess: %s, id: %02x\n", argv[0], procp->dwId));
		}
#endif

	}

	return 0;

errret:
	
	if (procp->pid != INVALID_HANDLE_VALUE)
		proc_terminate(procp);

	if (procp->hEvent)
	{
		CloseHandle(procp->hEvent);
		procp->hEvent = NULL;
	}
	
	return -1;
}

int
proc_terminate(proc_t *procp)
{

	if (proc_signal(procp, FTDCSIGTERM) == -1)
		return -1;

	if (procp->pid != INVALID_HANDLE_VALUE) {
		if ( WaitForSingleObject(procp->pid, 60000) == WAIT_TIMEOUT ) {
			return -1;
		}
	}

	return 0;
}

void
proc_kill(proc_t *procp)
{
	if ( !proc_terminate(procp) )
		return;

	if (procp->pid != INVALID_HANDLE_VALUE)
		TerminateThread( procp->pid, FTD_EXIT_DIE );
}

pid_t
proc_get_pid(char *name, int exactflag, int *pcnt)
{

	return 0;
}

int
proc_is_parent(char *procname)
{

	return 0;
}

int
proc_wait(proc_t *procp, int secs)
{

	switch(procp->type) {
	case PROCTYPE_PROC:
		{

		int	rc, tries = 0, maxtries = secs;

		if (secs == -1) {
			if ((rc = WaitForSingleObject(procp->pid, INFINITE)) != WAIT_OBJECT_0) {
				return -1;
			}
		} else {
			while (1) {
				rc = WaitForSingleObject(procp->pid, 10000);
				if ( rc == WAIT_TIMEOUT ) {
					if (tries++ >= maxtries) {
						return -1;
					}
					continue;
				} else if ( rc == WAIT_FAILED ) {
					return -1;
				} else {
					break;
				}
			}
		}

		break;
		
		}
	case PROCTYPE_THREAD:
		break;
	default:
		break;
	}

	return 0;
}

#else

int
proc_signal(proc_t *procp, int sig)
{

	kill(procp->pid, sig);

	return 0;
}

int 
proc_exec(proc_t *procp, char **argv, char **envp, int wait)
{
	int		rc;

	procp->pid = fork();

	switch(procp->pid) {
	case -1:
		return -1;
	case 0:
		/* child */
		execve(procp->command, argv, envp);
		exit(errno);
	default:
        /* parent */
		if (wait) {
			if ((rc = proc_wait(procp)) != 0) {
				return rc;
			}
		}
		break;
	}

	return 0;
}

int
proc_terminate(proc_t *procp)
{

	kill(procp->pid, SIGTERM);

	return -1; // cause we want to kill in UNIX
}

void
proc_kill(proc_t *procp)
{

	kill(procp->pid, SIGKILL);

	return;
}

pid_t
proc_get_pid(char *name, int exactflag, int *pcnt)
{
	proc_info_t	***p = NULL;
	pid_t		pid;

	pid = 0;
	if (!p && (p = (proc_info_t***)malloc(sizeof(proc_info_t***))) == NULL) {
		return 0;
	}
	*p = NULL;
	if ((*pcnt = get_proc_names(name, exactflag, p)) > 0) {
		pid = (*p)[0]->pid;
	}
	del_proc_names(p, *pcnt);

	return pid;
}

int
proc_is_parent(char *procname)
{
	pid_t	pid, ppid;
	int		pcnt;

	/* process may have been forked but not exec'd yet */
	pid = proc_get_pid(procname, 0, &pcnt);

	ppid = getppid();
	if (pid > 0 && ppid == pid) {
		return 1;
	}

	return 0;
}

int
proc_wait(proc_t *procp)
{
	pid_t	pid;
	int		rc, status, tries = 0, maxtries = 5;

	while (1) {
		pid = waitpid(procp->pid, &status, 0);
		if (pid == procp->pid) {
			break;
		}
		if (pid == -1) {
			if (errno == ECHILD) {
				break;
			} else {
				continue;
			}
		} else if (pid == 0) {
			if (tries++ >= maxtries) {
				break;
			}
			usleep(100000);
			continue;
		}
	}
	rc = pid;

	if (WIFSIGNALED(status)) {
		rc = WTERMSIG(status);
#ifdef TDMF_TRACE
		fprintf(stderr,"\n*** %s exitsignal = %d\n",
			procp->procname, rc);
#endif
	}
	if (WIFEXITED(status)) {
		rc = WEXITSTATUS(status);
#ifdef TDMF_TRACE
		fprintf(stderr,"\n*** %s exitstatus = %d\n",
			procp->procname, rc);
#endif
	}

	return rc;
}

#endif
