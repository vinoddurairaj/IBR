#include "DBServer.h"
#include <windows.h>
#include <tchar.h>
//#include <wchar.h>
#include <conio.h>
#include <stdio.h>
#include <errors.h>

// The name of the service
_TCHAR *SERVICE_NAME = _TEXT("DtcCOLLECTOR");// _TEXT();

// Handle used to communicate status info with
// the SCM. Created by RegisterServiceCtrlHandler
SERVICE_STATUS_HANDLE	serviceStatusHandle;
SERVICE_STATUS			serviceStatus;

// Flags holding current state of service
BOOL pauseService = FALSE;
BOOL runningService = FALSE;

// Global Event used to kill all threads
HANDLE ghTerminateEvent = NULL;

#define NUMBER_OF_SERVICE_EVENTS    2
#define TERMINATE_EVENT             0
#define COLLECTOR_THREAD            1

// array of objects to wait on when quiting
static HANDLE hServiceEvents[NUMBER_OF_SERVICE_EVENTS];

//DBServer.cpp
extern DWORD WINAPI CollectorMain(void* notused);

#ifdef TEST
static BOOL debuggerStarted = FALSE;
#endif

extern void WINAPI DispatchCommand(int nCh);


/*
    function:  StartFTDService(void)

    Initializes the service by creating events/ starting its threads,
    fills in the hServiceEvents array of handles to be waited on.

    events:     ghTerminateEvent
    threads:    MasterThread

    input:      none

    output:     TRUE - it worked.
                FALSE - A error is written to the EventLog and quits
*/
int StartFtdService(void)
{
    HANDLE  hCollectorThread;
    DWORD   dwId;

    //DebugBreak();

	// Load rebranding dll
	char szFileName[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	GetModuleFileName(NULL, szFileName, _MAX_PATH);
	_splitpath(szFileName, drive, dir, fname, ext);
	_makepath(szFileName, drive, dir, "RBRes", "dll");
						
	g_ResourceManager.SetResourceDllName(szFileName);


    error_syslog_DirectAccess((LPSTR)g_ResourceManager.GetFullProductName(),"COLLECTOR STARTED");

    //
    // SVG
    //
    if (SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS))
    {
#if DBG
        OutputDebugString("Set collector base priority to ABOVE_NORMAL_PRIORITY_CLASS suceeded\n");
#endif
    }
    else
    {
#if DBG
        OutputDebugString("Set collector base priority to ABOVE_NORMAL_PRIORITY_CLASS failed!\n");
#endif
    }

    // create the termination event
    ghTerminateEvent = CreateEvent (0, TRUE, FALSE, 0);
    if (!ghTerminateEvent)
        return GetLastError();

    // Start the master thread
    hCollectorThread = CreateThread(    NULL,
                                        0,
                                        (LPTHREAD_START_ROUTINE)CollectorMain,
                                        0,
                                        0,
                                        &dwId);


    if ( !hCollectorThread )
        return GetLastError();

    hServiceEvents[TERMINATE_EVENT] = ghTerminateEvent;
    hServiceEvents[COLLECTOR_THREAD] = hCollectorThread;

    return 0;
}

/*
    function:  CleanUpFTDService(void)

    cleans up...

    input:      none

    output:     none
*/
void CleanUpFtdService(void)
{
    int i;

    for (i = 0; i < NUMBER_OF_SERVICE_EVENTS; i++)
        CloseHandle(hServiceEvents[i]);
}

/*
    function:  RunFTDService(bool bService)

    waits for everyone in the ServiceEvents array to finish.

    input:      none

    output:     TRUE - if a control panel shutdown.
                FALSE - An error occured.
*/
int RunFtdService(bool bService)
{
    DWORD   dwRes;
#ifdef _COLLECT_STATITSTICS_
    DWORD   dwCollectorDataTimeout = 10;
#endif

    if (bService)
    {
        dwRes = WaitForMultipleObjects(
            NUMBER_OF_SERVICE_EVENTS,           // number of handles in the object handle array 
            (CONST HANDLE *) &hServiceEvents,   // pointer to the object-handle array 
            FALSE,                              // wait for everyone flag 
            INFINITE                            // time-out interval in milliseconds 
            );
    }
    else
    {
        do
        {
            //
            // Wait for 1 second for end of service event
            //
            dwRes = WaitForMultipleObjects(
                NUMBER_OF_SERVICE_EVENTS,           // number of handles in the object handle array 
                (CONST HANDLE *) &hServiceEvents,   // pointer to the object-handle array 
                FALSE,                              // wait for everyone flag 
                1000                                // time-out interval in milliseconds 
                );

            //
            // Check for user interaction
            //
            if (_kbhit())
            {
                DispatchCommand(_getch());
            }

#ifdef _COLLECT_STATITSTICS_
            //
            // Accumulate all collector statistics Data
            //
            AccumulateCollectorStats();

            dwCollectorDataTimeout--;
            if (!dwCollectorDataTimeout)
            {
                dwCollectorDataTimeout = 10;
                //
                // Every 10 seconds send collector data to GUI 
                //
                SendCollectorStatisticsData();
            }
#endif

        //
        // Only get out if this is not a timeout event
        //
        } while (dwRes == WAIT_TIMEOUT);
    }

    SetEvent(ghTerminateEvent);

    WaitForMultipleObjects(
        NUMBER_OF_SERVICE_EVENTS,           // number of handles in the object handle array 
        (CONST HANDLE *) &hServiceEvents,   // pointer to the object-handle array 
        TRUE,                               // wait for everyone flag 
        300000                              // time-out interval in milliseconds 
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
	{ 
        StopService();
	} 

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
        default:
			currentState = SERVICE_RUNNING; 
            break;
    }

     // Send current status. 
     SendStatusToSCM(currentState, NO_ERROR, 0, 0, 0);
}

// Handle an error from ServiceMain by cleaning up
// and telling SCM that the service didn't start.
VOID terminate(int error)
{
    CleanUpFtdService();

#ifdef _SEND_COLLECTOR_STATISTICS_TO_APPLICATION_
    DeleteCommonApplicationMemory();
#endif

    // Send a message to the scm to tell about
    // stopage
    if (serviceStatusHandle)
    {
        if (error == -1)    // service error
            SendStatusToSCM(SERVICE_STOPPED, 0, 1, 0, 0);
        else
            SendStatusToSCM(SERVICE_STOPPED, error, 0, 0, 0);
    }

    error_syslog_DirectAccess((LPSTR)g_ResourceManager.GetFullProductName(),"COLLECTOR STOPPED");

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
    BOOL    success;
    DWORD   error;

    // immediately call Registration function
    serviceStatusHandle = RegisterServiceCtrlHandler( SERVICE_NAME, 
        (LPHANDLER_FUNCTION) ServiceCtrlHandler);
    if (!serviceStatusHandle)
    {
        terminate(GetLastError());
        return;
    }

    // Notify SCM of progress
    success = SendStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 1, 1000);
    if (!success)
    {
        terminate(GetLastError()); 
        return;
    }

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

    terminate(RunFtdService(true));
}

int WINAPI _tWinMain(HINSTANCE ghInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow)
{
    SERVICE_TABLE_ENTRY serviceTable[] = 
    { 
        { SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION) ServiceMain},
        { NULL, NULL }
    };
    
    if ( strstr(lpCmdLine,"-console") )
    {   //start Collector as a console program (for debuging purposes)
        /* get cmd line args */
        
#ifdef _SEND_COLLECTOR_STATISTICS_TO_APPLICATION_
        InitializeCommonApplicationMemory();
#endif
		
        StartFtdService();

        RunFtdService(false);
        CleanUpFtdService();
    }
    else
    {   //start Collector as a Windows Service
        // Register with the SCM
        if (!StartServiceCtrlDispatcher(serviceTable))
        {
        }
    }

    return 0;
}
