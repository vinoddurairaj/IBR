/****************************************************************
 * Name:  eventlog.c                                            *
 *                                                              *
 * Function:  Event Log management functions for                *
 *            the data collection DLL.                          *
 *                                                              *
 ****************************************************************/

//
// All-inclusive header file
//
#include "perf.h"

//
// Data global to this module
//
static HANDLE hEventLog		= NULL;
static DWORD  dwUserCount	= 0;
static DWORD  dwLogLevel	= DEFAULT_LOG_LEVEL;

//
// Forward declarations of local routines
//
// (None)


//++
// Function:
//		PerfOpenEventLog
//
// Description:
//		This function opens a connection with 
//		the Event Log. If it's already open,
//		just increment a counter. 
//
// Arguments:
//		(None)
//
// Return Value:
//		(None)
//--
VOID
PerfOpenEventLog(
	VOID
	)
{
    HKEY hLogLevelKey;
	
	TCHAR LogLevelKeyName[] = 
			"SYSTEM\\CurrentControlSet\\Services\\"
			PRODUCTNAME 
			"\\Parameters";

	TCHAR LogLevelValueName[] = "EventLogLevel";

    LONG status;

    DWORD dwLevelFromRegistry;
    DWORD dwValueType;
    DWORD dwValueSize;
   
	if( hEventLog == NULL )
	{
		//
		// Find the logging-level value in the
		// Registry. If it's not there, or we
		// have problems with the Registry, just
		// use the default value.
		//
		status = RegOpenKeyEx(
					HKEY_LOCAL_MACHINE,
					LogLevelKeyName,
					0,
					KEY_READ,
					&hLogLevelKey );

	   if( status == ERROR_SUCCESS)
	   { 
			dwValueSize = 
				sizeof( dwLevelFromRegistry );

			status = RegQueryValueEx(
						hLogLevelKey,
						LogLevelValueName,
						NULL,
						&dwValueType,
						(LPBYTE)&dwLevelFromRegistry,
						&dwValueSize );

		   if( status == ERROR_SUCCESS )
		   {
			   dwLogLevel = dwLevelFromRegistry;
		   }
		   else
		   {
			   dwLogLevel = DEFAULT_LOG_LEVEL;
		   }
		   
		   RegCloseKey( hLogLevelKey );
	   }
	   else
	   {
		   dwLogLevel = DEFAULT_LOG_LEVEL;
	   }

	   //
	   // Open a handle to the Event Log
	   //
	   hEventLog = RegisterEventSource(
						(LPTSTR)NULL,
						PRODUCTNAME );
	}

    //
	// Increment count of Event Log users
	//
    if( hEventLog != NULL ) dwUserCount++;
}


//++
// Function:
//		PerfCloseEventLog
//
// Description:
//		This function breaks our connection with
//		the Event Log once everyone is finished
//		using it.
//
// Arguments:
//		(None)
//
// Return Value:
//		(None)
//--
VOID
PerfCloseEventLog(
	VOID
	)

{
	if( hEventLog != NULL )
	{
		dwUserCount--;
        if( dwUserCount <= 0 )
		{ 
			DeregisterEventSource( hEventLog );
			hEventLog = NULL;
		}
	}
}

//++
// Function:
//		PerfLogInformation
//
// Description:
//		This function logs an informational
//		message without any dump data.
//
// Arguments:
//		LOG_LEVEL of the message
//		Message code number
//
// Return Value:
//		(None)
//--
VOID
PerfLogInformation(
	IN DWORD dwMessageLevel,
	IN DWORD dwMessage
	)
{
    LPTSTR  lpszStrings[1];

	lpszStrings[0] = PRODUCTNAME;

	if( ( hEventLog != NULL ) &&
		( dwLogLevel > LOG_LEVEL_NONE ) &&
		( dwMessageLevel <= dwLogLevel ))
	{
		ReportEvent(
			hEventLog,
			EVENTLOG_INFORMATION_TYPE,
			0,
			dwMessage,
			(PSID)NULL,
			1,
			0,
			lpszStrings,
			NULL );
	}
}


//++
// Function:
//		PerfLogError
//
// Description:
//		This function logs an error
//		message without any dump data.
//
// Arguments:
//		LOG_LEVEL of the message
//		Message code number
//
// Return Value:
//		(None)
//--
VOID
PerfLogError(
	IN DWORD dwMessageLevel,
	IN DWORD dwMessage
	)
{
    LPTSTR  lpszStrings[1];

	lpszStrings[0] = PRODUCTNAME;

	if( ( hEventLog != NULL ) &&
		( dwLogLevel > LOG_LEVEL_NONE ) &&
		( dwMessageLevel <= dwLogLevel ))
	{
		ReportEvent(
			hEventLog,			// Event Log handle
			EVENTLOG_ERROR_TYPE,// Message type
			0,					// Category
			dwMessage,			// Message code
			(PSID)NULL,			// Security ID
			1,					// Number of insertion strings
			0,					// Byte-count of dump data
			lpszStrings,		// Array of insertion strings
			NULL );				// Ptr to dump-data or NULL
	}
}

//++
// Function:
//		PerfLogErrorWithData
//
// Description:
//		This function logs an error
//		message that has dump data.
//
// Arguments:
//		LOG_LEVEL of the message
//		Message code number
//		Pointer to an array of DWORD dump data
//		Count of bytes in dump data
//
// Return Value:
//		(None)
//--
VOID
PerfLogErrorWithData(
	IN DWORD	dwMessageLevel,
	IN DWORD	dwMessage,
	IN LPDWORD	lpdwDumpData,
	IN DWORD	cbDumpDataSize
	)
{
    LPTSTR  lpszStrings[1];

	lpszStrings[0] = PRODUCTNAME;

	if( ( hEventLog != NULL ) &&
		( dwLogLevel > LOG_LEVEL_NONE ) &&
		( dwMessageLevel <= dwLogLevel ))
	{
		ReportEvent(
			hEventLog,
			EVENTLOG_ERROR_TYPE,
			0,
			dwMessage,
			(PSID)NULL,
			1,
			cbDumpDataSize,
			lpszStrings,
			(PVOID)lpdwDumpData );
	}
}
