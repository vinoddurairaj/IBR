// Ftd.c
#include "ftd_port.h"
#include "ftd_error.h"
#include "ftd_sock.h"

#include <tchar.h>
#include <wchar.h>

extern char** share_envp ;	/* shared environment vector		*/
extern char** share_argv ;	/* shared argument vector			*/


// The name of the service
_TCHAR *SERVICE_NAME = _TEXT(PRODUCTNAME);

// Handle used to communicate status info with
// the SCM. Created by RegisterServiceCtrlHandler
SERVICE_STATUS_HANDLE serviceStatusHandle;

// Flags holding current state of service
BOOL pauseService = FALSE;
BOOL runningService = FALSE;

// Global Event used to kill all threads
HANDLE ghTerminateEvent = NULL;

#define NUMBER_OF_SERVICE_EVENTS	2
#define TERMINATE_EVENT				0
#define MASTER_THREAD				1

// array of objects to wait on when quiting
static HANDLE hServiceEvents[NUMBER_OF_SERVICE_EVENTS];

extern DWORD MasterThread(LPDWORD param);
extern void proc_argv(int argc, char** argv);
extern void proc_envp(char** envp);

#ifdef TEST
static BOOL debuggerStarted = FALSE;
#endif

/*
	function:  StartFTDService(void)

	Initializes the service by creating events/ starting its threads,
	fills in the hServiceEvents array of handles to be waited on.

	events:		ghTerminateEvent
	threads:	MasterThread

	input:		none

	output:		TRUE - it worked.
				FALSE - A error is written to the EventLog and quits
*/
int StartFtdService(void)
{
	HANDLE	hMasterThread;
	DWORD	dwId;
	int		rc;
	char	strFtdStartPath[_MAX_PATH];
	PROCESS_INFORMATION ProcessInfo;
	DWORD	dwExitCode = STILL_ACTIVE;
	
	// Set up the start up info struct.
	STARTUPINFO si;
    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

	//DebugBreak();

	if (ftd_sock_startup() == -1)
		return -1;

	// create the termination event
	ghTerminateEvent = CreateEvent (0, TRUE, FALSE, 0);
	if (!ghTerminateEvent)
		return GetLastError();

	// Start the master thread
	hMasterThread = CreateThread(	NULL,
									0,
									(LPTHREAD_START_ROUTINE)MasterThread,
									0,
									0,
									&dwId);


	if ( !hMasterThread )
		return GetLastError();

    hServiceEvents[TERMINATE_EVENT] = ghTerminateEvent;
	hServiceEvents[MASTER_THREAD] = hMasterThread;

	strcpy(strFtdStartPath, PATH_RUN_FILES);

    SetCurrentDirectory(strFtdStartPath);

	strcpy(strFtdStartPath, QNM);
	strcat(strFtdStartPath, "boot.bat");
	rc = CreateProcess(strFtdStartPath, NULL, NULL, NULL, TRUE,
					CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);
	if (!rc)
		return GetLastError();

/*
	while(dwExitCode == STILL_ACTIVE)
	{
		GetExitCodeProcess(ProcessInfo.hProcess, &dwExitCode);
		Sleep(1);
	}
*/
    return 0;
}

/*
	function:  CleanUpFTDService(void)

	cleans up...

	input:		none

	output:		none
*/
void CleanUpFtdService(void)
{
	int i, rc;
	char	strFtdStopPath[_MAX_PATH];
	PROCESS_INFORMATION ProcessInfo;
	DWORD	dwExitCode = STILL_ACTIVE;
	
	// Set up the start up info struct.
	STARTUPINFO si;
    ZeroMemory(&si,sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

	strcpy(strFtdStopPath, PATH_RUN_FILES);

    SetCurrentDirectory(strFtdStopPath);

	strcpy(strFtdStopPath, QNM);
	strcat(strFtdStopPath, "stop.exe -a" /*"shutdown.bat"*/);
	rc = CreateProcess(strFtdStopPath, NULL, NULL, NULL, TRUE,
					CREATE_NO_WINDOW, NULL, NULL, &si, &ProcessInfo);

/*
	while(dwExitCode == STILL_ACTIVE)
	{
		GetExitCodeProcess(ProcessInfo.hProcess, &dwExitCode);
		Sleep(1);
	}
*/

	for (i = 0; i < NUMBER_OF_SERVICE_EVENTS; i++)
		CloseHandle(hServiceEvents[i]);

	ftd_sock_cleanup();
}

/*
	function:  RunFTDService(void)

	waits for everyone in the ServiceEvents array to finish.

	input:		none

	output:		TRUE - if a control panel shutdown.
				FALSE - An error occured.
*/
int RunFtdService(void)
{

	DWORD dwRes = WaitForMultipleObjects(
		NUMBER_OF_SERVICE_EVENTS,			// number of handles in the object handle array 
		(CONST HANDLE *) &hServiceEvents,	// pointer to the object-handle array 
		FALSE,								// wait for everyone flag 
		INFINITE 							// time-out interval in milliseconds 
		);

	SetEvent(ghTerminateEvent);

	WaitForMultipleObjects(
		NUMBER_OF_SERVICE_EVENTS,			// number of handles in the object handle array 
		(CONST HANDLE *) &hServiceEvents,	// pointer to the object-handle array 
		TRUE,								// wait for everyone flag 
		300000 								// time-out interval in milliseconds 
		);

	return dwRes;
}

// Stops the service by allowing ServiceMain to
// complete
VOID StopService() 
{
	runningService=FALSE;

	// Set the event that is holding ServiceMain
	// so that ServiceMain can return
	SetEvent(ghTerminateEvent);
}

// This function consolidates the activities of 
// updating the service status with
// SetServiceStatus
BOOL SendStatusToSCM (DWORD dwCurrentState,
	DWORD dwWin32ExitCode, 
	DWORD dwServiceSpecificExitCode,
	DWORD dwCheckPoint,
	DWORD dwWaitHint)
{
	BOOL success;
	SERVICE_STATUS serviceStatus;

	// Fill in all of the SERVICE_STATUS fields
	serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	serviceStatus.dwCurrentState = dwCurrentState;

	// If in the process of something, then accept
	// no control events, else accept anything
	if (dwCurrentState == SERVICE_START_PENDING)
		serviceStatus.dwControlsAccepted = 0;
	else
		serviceStatus.dwControlsAccepted = 
			SERVICE_ACCEPT_STOP |
			SERVICE_ACCEPT_SHUTDOWN;

	// if a specific exit code is defines, set up
	// the win32 exit code properly
	if (dwServiceSpecificExitCode == 0)
		serviceStatus.dwWin32ExitCode = dwWin32ExitCode;
	else
		serviceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
	
	serviceStatus.dwServiceSpecificExitCode = dwServiceSpecificExitCode;
	serviceStatus.dwCheckPoint = dwCheckPoint;
	serviceStatus.dwWaitHint = dwWaitHint;

	// Pass the status record to the SCM
	success = SetServiceStatus (serviceStatusHandle, &serviceStatus);
	if (!success)
		StopService();

	return success;
}

// Dispatches events received from the service 
// control manager
VOID ServiceCtrlHandler (DWORD controlCode) 
{
	DWORD  currentState = 0;
	BOOL success;

	switch(controlCode)
	{
		// There is no START option because
		// ServiceMain gets called on a start

		// Do nothing in a shutdown. Could do cleanup
		// here but it must be very quick.
		case SERVICE_CONTROL_SHUTDOWN:

        // Stop the service
		case SERVICE_CONTROL_STOP:
			currentState = SERVICE_STOP_PENDING;

			// Tell the SCM what's happening
			success = SendStatusToSCM( SERVICE_STOP_PENDING, NO_ERROR, 0, 1, 600000);
			// Not much to do if not successful

			// Stop the service
			StopService();
			return;

		// Update current status
		case SERVICE_CONTROL_INTERROGATE:
			// it will fall to bottom and send status
			return;
		default:
 			return;
	}
	
	SendStatusToSCM(currentState, NO_ERROR,	0, 0, 0);
}

// Handle an error from ServiceMain by cleaning up
// and telling SCM that the service didn't start.
VOID terminate(int error)
{
	CleanUpFtdService();

	// Send a message to the scm to tell about
	// stopage
	if (serviceStatusHandle)
	{
		if (error == -1)	// service error
			SendStatusToSCM(SERVICE_STOPPED, 0, 1, 0, 0);
		else
			SendStatusToSCM(SERVICE_STOPPED, error, 0, 0, 0);
	}
}

// ServiceMain is called when the SCM wants to
// start the service. When it returns, the service
// has stopped. It therefore waits on an event
// just before the end of the function, and
// that event gets set when it is time to stop. 
// It also returns on any error because the
// service cannot start if there is an eror.
VOID ServiceMain(DWORD argc, _TCHAR* *argv) 
{
	BOOL	success;
	DWORD	error;

	// immediately call Registration function
	serviceStatusHandle = RegisterServiceCtrlHandler( SERVICE_NAME, 
		(LPHANDLER_FUNCTION) ServiceCtrlHandler);
	if (!serviceStatusHandle)
	{
		terminate(GetLastError());
		return;
	}

	SetPriorityClass(GetCurrentProcess(),0x00008000);

	// Notify SCM of progress
	success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 1, 1000);
	if (!success)
	{
		terminate(GetLastError()); 
		return;
	}

	/* process envp */
//	proc_envp(envp);

	/* get cmd line args */
    proc_argv(argc, argv);

	// Start the service itself
	error = StartFtdService();
	if (error)
	{
		terminate(error);
		return;
	}
	else
		runningService = TRUE;

	// The service is now running. 
	// Notify SCM of progress
	success = SendStatusToSCM(SERVICE_RUNNING,NO_ERROR, 0, 0, 0);
	if (!success)
	{
		terminate(GetLastError()); 
		return;
	}

	terminate(RunFtdService());
}


int WINAPI _tWinMain(HINSTANCE ghInstance, HINSTANCE hPrevInstance, 
				   LPSTR lpCmdLine, int nCmdShow)
{
	SERVICE_TABLE_ENTRY serviceTable[] = 
	{ 
		{ SERVICE_NAME,	(LPSERVICE_MAIN_FUNCTION) ServiceMain},
		{ NULL, NULL }
	};

////------------------------------------------

//
// SVG TST 03-06-03 - remove this for processor independence
//
#if 1
	HANDLE  hProcess = GetCurrentProcess();
	DWORD   ProcessAffinityMask, // process affinity mask
			SystemAffinityMask;  // system affinity mask

	BOOL b = GetProcessAffinityMask(  hProcess,                  // handle to process
								      &ProcessAffinityMask, // process affinity mask
								      &SystemAffinityMask   // system affinity mask
								    );
	if ( b )
	{
		DWORD bit = 1;
		//find first bit set
		while( (bit & ProcessAffinityMask) == 0 )	
			bit <<= 1;
		//clear all other bits
		ProcessAffinityMask &= bit;

		b = SetProcessAffinityMask( hProcess, 
								    ProcessAffinityMask );

	}
#endif

    // use 0x00008000 because above normal only defined in DDK 
	SetPriorityClass(GetCurrentProcess(),0x00008000);

//------------------------------------------

	if ( strstr(lpCmdLine,"-console") )
	{
		/* get cmd line args */
		StartFtdService();
		RunFtdService();
		CleanUpFtdService();
	}
	else
	{
		// Register with the SCM
		if (!StartServiceCtrlDispatcher(serviceTable))
		{
		}
	}

	return 0;
}
