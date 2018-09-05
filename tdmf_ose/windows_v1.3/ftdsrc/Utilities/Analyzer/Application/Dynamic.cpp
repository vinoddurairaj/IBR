/*
    Dynamic.c - based on an idea described in: 
				1998 James M. Finnegan - Microsoft Systems Journal

    Dynamically load and unload an NT kernel mode driver
*/

#include "stdafx.h"

#include <windows.h>
#include <stdio.h>

char Dbg[1024];


BOOL InstallAndStartDriver(SC_HANDLE hSCM, LPCTSTR DriverName, LPCTSTR ServiceExe)
{
    SC_HANDLE  hSrv;
    BOOL       bRet;

    // use SCManager to register driver
    hSrv = CreateService(hSCM, DriverName, DriverName, SERVICE_ALL_ACCESS, 
                         SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, 
                         SERVICE_ERROR_NORMAL, ServiceExe, NULL, NULL,
                         NULL, NULL, NULL );


    unsigned int LError = GetLastError();

    sprintf(Dbg,"CreateService Return->0x%08x Error 0x%08x\n",hSrv,LError);
    OutputDebugString(Dbg);

    if (LError != 0x00000431)
    {
        if(!hSrv)
            return FALSE;
    }
    else
    {
        hSrv = OpenService(hSCM, DriverName, SERVICE_ALL_ACCESS);
        sprintf(Dbg,"CreateService Return->0x%08x Error 0x%08x\n",hSrv,LError);
        OutputDebugString(Dbg);
    }


    // try to start the driver...
    bRet = StartService(hSrv, 0, NULL);

    sprintf(Dbg,"Start Service Return->0x%08x Error 0x%08x\n",bRet,GetLastError());
    OutputDebugString(Dbg);

    bRet = TRUE;

    CloseServiceHandle(hSrv);
    return bRet;
}


/*
	UnloadDriver - stop and unregister kernel driver
*/
BOOL UnloadDriver(LPCTSTR DriverName)
{
    SC_HANDLE       hSCManager;
    SC_HANDLE       hService;
    SERVICE_STATUS  serviceStatus;
    BOOL            bReturn;


   // Open Service Control Manager on the local machine...
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    // Open the Service Control Manager for our driver service...
    hService = OpenService(hSCManager, DriverName, SERVICE_ALL_ACCESS);

    // Stop the driver.  Will return TRUE on success... 
    bReturn = ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus);

    // Delete the driver from the registry...
    if(bReturn == TRUE)
    {
        bReturn = DeleteService(hService);
    }

    // Close the SC Manager
    CloseServiceHandle(hService);

    // Close Service Control Manager
    CloseServiceHandle(hSCManager);

    return bReturn;
}


/*
	LoadDriver - load, register and start driver
*/
HANDLE LoadDriver(BOOL *fNTDynaLoaded, LPCTSTR DosName, LPCTSTR DriverName, LPCTSTR ServiceExe)
{
    HANDLE hDev;

    
    // Default to FALSE: there was no need to dynamically load the driver, as 
	// it is already installed in this computers SCM database (registry)
    *fNTDynaLoaded = FALSE;

	// try open the driver, in case it is already installed
    hDev = CreateFile(DosName, GENERIC_READ | GENERIC_WRITE, 
                      FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
                      OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
       
    // if we can't get a handle, the driver has to be installed first...
    if(hDev == INVALID_HANDLE_VALUE)
    {
	    SC_HANDLE   hSCM;
	    char        szCurDir[256];

	    // Connect to the local SCM
	    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	    // Assume driver is in the same directory as the DLL.
	    GetCurrentDirectory(sizeof(szCurDir)-lstrlen(ServiceExe)-2, szCurDir);
		lstrcat(szCurDir, "\\");
	    lstrcat(szCurDir, ServiceExe);
	    
	    // Install driver in the registry and start it...
	    InstallAndStartDriver(hSCM, DriverName, szCurDir);
	
	    // Disconnect from the SCM
	    CloseServiceHandle(hSCM);

        OutputDebugString("HELLO\n");

        // get a driver handle...
        hDev = CreateFile(DosName, GENERIC_READ | GENERIC_WRITE, 
                          FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
                          OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
            
	    // Tell the caller we dynamically loaded the driver
        *fNTDynaLoaded = TRUE;
    }

    sprintf(Dbg,"Return->0x%08x\n",hDev);
    OutputDebugString(Dbg);

    return hDev;
}
