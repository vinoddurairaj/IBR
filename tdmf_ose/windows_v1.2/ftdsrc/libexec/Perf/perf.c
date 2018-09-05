/****************************************************************
 * Name:  perf.c                                            *
 *                                                              *
 * Function:  Data collection DLL that interfaces with          *
 *            the performance-monitoring system.                *
 *                                                              *
 ****************************************************************/

//
// All-inclusive header file
//
#include "perf.h"

#include "ftd.h"
#include "ftd_error.h"
#include "ftd_perf.h"
#include "perfutil.h"

//
// Data global to this module
//
//
// App Mem counter data structures
//
static ftd_perf_t	*perfp = NULL;

static	DWORD dwOpenCount = 0;
static	BOOL bInitialized = FALSE;

//
// Initialized object header defined
// in data.c
//
extern FTD_HEADER_DEFINITION FtdHeaderDefinition;

//
// Forward declarations of routines
//
PM_OPEN_PROC		PerfOpen;
PM_COLLECT_PROC		PerfCollect;
PM_CLOSE_PROC		PerfClose;

//++
// Function:
//		PerfOpen
//
// Description:
//		This function sets up the data collection
//		DLL for later calls. 
//
// Arguments:
//		Unicode string describing objects to be opened
//
// Return Value:
//		ERROR_SUCCESS if everything works
//		ERROR if something goes wrong
//--
DWORD APIENTRY 
PerfOpen(
    LPWSTR lpDeviceNames
    )
{
    HKEY			hKeyDriverPerf;
	DWORD			dwFirstCounter;
	DWORD			dwFirstHelp;
    DWORD			dwType;
    DWORD			dwSize;
    DWORD			dwStatus;
    FTD_COUNTERS	counters;

	//
	// DLL must keep track of how many it is opened
	// because the Registry routines may call it in
	// response to multiple performance queries.
	//
	if( dwOpenCount == 0 )
	{
		//
		// Errors in the DLL will go to the Event
		// Log. Open it here.
		PerfOpenEventLog();

		//
		// Open the Performance subkey of the driver's
		// service key in the Registry.
		//
		dwStatus = RegOpenKeyEx(
						HKEY_LOCAL_MACHINE,
						"SYSTEM\\CurrentControlSet"
							"\\Services\\" PRODUCTNAME
								"\\Performance",
						0L,
						KEY_ALL_ACCESS,
						&hKeyDriverPerf );

		if( dwStatus != ERROR_SUCCESS )
		{
			PerfLogErrorWithData(
				LOG_LEVEL_NORMAL,
				FTD_PERF_UNABLE_OPEN_DRIVER_KEY,
				&dwStatus, sizeof( dwStatus ));

			PerfCloseEventLog();

			ftd_perf_delete(perfp);
			return dwStatus;
		}

		//
		// Get base index of first object or counter
		//
		dwSize = sizeof (DWORD);
		dwStatus = RegQueryValueEx(
					hKeyDriverPerf, 
					"First Counter",
					0L,
					&dwType,
					(LPBYTE)&dwFirstCounter,
					&dwSize);

		if( dwStatus != ERROR_SUCCESS )
		{
			PerfLogErrorWithData(
				LOG_LEVEL_NORMAL,
				FTD_PERF_CANT_READ_FIRST_COUNTER,
				&dwStatus, sizeof( dwStatus ));

			RegCloseKey( hKeyDriverPerf );
			PerfCloseEventLog();

			ftd_perf_delete(perfp);
			return dwStatus;
		}

		//
		// Get base index of first help text
		//
		dwSize = sizeof (DWORD);
		dwStatus = RegQueryValueEx(
					hKeyDriverPerf,
					"First Help",
					0L,
					&dwType,
					(LPBYTE)&dwFirstHelp,
					&dwSize );

		if( dwStatus != ERROR_SUCCESS )
		{
			PerfLogErrorWithData(
				LOG_LEVEL_NORMAL,
				FTD_PERF_CANT_READ_FIRST_HELP,
				&dwStatus, sizeof( dwStatus ));

			RegCloseKey( hKeyDriverPerf );
			PerfCloseEventLog();

			ftd_perf_delete(perfp);
			return dwStatus;
		}

		//
		// Don't need Registy handle anymore
		//
		RegCloseKey( hKeyDriverPerf );
 
		//
		// Prepare the  static portions of header
		//
		//
		// Initialize PERF_OBJECT_TYPE struct
		//
		FtdHeaderDefinition.FtdDevice.
				ObjectNameTitleIndex =
						dwFirstCounter + FTDDEVICE;

		FtdHeaderDefinition.FtdDevice.
					ObjectHelpTitleIndex = 
						dwFirstHelp + FTDDEVICE;

		//
		// Initialize PERF_COUNTER_DEFINITION
		//
		FtdHeaderDefinition.actual.
					CounterNameTitleIndex = 
						dwFirstCounter + XFERACTUAL;
		FtdHeaderDefinition.actual.
					CounterHelpTitleIndex = 
						dwFirstHelp + XFERACTUAL;

        FtdHeaderDefinition.actual.
					CounterOffset += 
						(DWORD)((LPBYTE)&counters.actual - (LPBYTE)&counters);
		//
		// Initialize PERF_COUNTER_DEFINITION
		//
		FtdHeaderDefinition.effective.
					CounterNameTitleIndex = 
						dwFirstCounter + XFEREFECTIVE;
		FtdHeaderDefinition.effective.
					CounterHelpTitleIndex = 
						dwFirstHelp + XFEREFECTIVE;

        FtdHeaderDefinition.effective.
					CounterOffset += 
						(DWORD)((LPBYTE)&counters.effective - (LPBYTE)&counters);
		//
		// Initialize PERF_COUNTER_DEFINITION
		//
		FtdHeaderDefinition.entries.
					CounterNameTitleIndex = 
						dwFirstCounter + BABENTRIES;
		FtdHeaderDefinition.entries.
					CounterHelpTitleIndex = 
						dwFirstHelp + BABENTRIES;

        FtdHeaderDefinition.entries.
					CounterOffset += 
						(DWORD)((LPBYTE)&counters.entries - (LPBYTE)&counters);

		//
		// Initialize PERF_COUNTER_DEFINITION
		//
		FtdHeaderDefinition.sectors.
					CounterNameTitleIndex = 
						dwFirstCounter + BABSECTORS;
		FtdHeaderDefinition.sectors.
					CounterHelpTitleIndex = 
						dwFirstHelp + BABSECTORS;

        FtdHeaderDefinition.sectors.
					CounterOffset = 
						(DWORD)((LPBYTE)&counters.sectors - (LPBYTE)&counters);
		//
		// Initialize PERF_COUNTER_DEFINITION
		//
		FtdHeaderDefinition.pctdone.
					CounterNameTitleIndex = 
						dwFirstCounter + PERCENTDONE;
		FtdHeaderDefinition.pctdone.
					CounterHelpTitleIndex = 
						dwFirstHelp + PERCENTDONE;

        FtdHeaderDefinition.pctdone.
					CounterOffset = 
						(DWORD)((LPBYTE)&counters.pctdone - (LPBYTE)&counters);
		//
		// Initialize PERF_COUNTER_DEFINITION
		//
		FtdHeaderDefinition.pctbab.
					CounterNameTitleIndex = 
						dwFirstCounter + PERCENTBABFULL;
		FtdHeaderDefinition.pctbab.
					CounterHelpTitleIndex = 
						dwFirstHelp + PERCENTBABFULL;

        FtdHeaderDefinition.pctbab.
					CounterOffset = 
						(DWORD)((LPBYTE)&counters.pctbab - (LPBYTE)&counters);
		//
		// Initialize PERF_COUNTER_DEFINITION
		//
		FtdHeaderDefinition.bytesread.
					CounterNameTitleIndex = 
						dwFirstCounter + READBYTES;
		FtdHeaderDefinition.bytesread.
					CounterHelpTitleIndex = 
						dwFirstHelp + READBYTES;

        FtdHeaderDefinition.bytesread.
					CounterOffset = 
						(DWORD)((LPBYTE)&counters.bytesread - (LPBYTE)&counters);
		//
		// Initialize PERF_COUNTER_DEFINITION
		//
		FtdHeaderDefinition.byteswritten.
					CounterNameTitleIndex = 
						dwFirstCounter + WRITTENBYTES;
		FtdHeaderDefinition.byteswritten.
					CounterHelpTitleIndex = 
						dwFirstHelp + WRITTENBYTES;

        FtdHeaderDefinition.byteswritten.
					CounterOffset = 
						(DWORD)((LPBYTE)&counters.byteswritten - (LPBYTE)&counters);
		//
		// Mark DLL as successfully initialized
		//
		bInitialized = TRUE;
	}

    if ( (perfp = ftd_perf_create()) == NULL)
	{
		dwStatus = GetLastError();
 		PerfLogErrorWithData(
			LOG_LEVEL_NORMAL,
			FTD_PERF_OPEN_FILE_MAPPING_ERROR,
			&dwStatus, sizeof( dwStatus ) );
		PerfCloseEventLog();
		return dwStatus;
	}
	
	if ( ftd_perf_init(perfp, TRUE) )
	{
		dwStatus = GetLastError();
 		PerfLogErrorWithData(
			LOG_LEVEL_NORMAL,
			FTD_PERF_OPEN_FILE_MAPPING_ERROR,
			&dwStatus, sizeof( dwStatus ) );
		PerfCloseEventLog();
		return dwStatus;
	}

	
	//
	// One way or another, there's one more
	// thread using the DLL.
	//
	dwOpenCount++;

	return ERROR_SUCCESS;
}

//++
// Function:
//		PerfCollect
//
// Description:
//		This function collects one sample 
//		of data for the specified objects.
//
// Arguments:
//		Unicode string describing objects to be opened
//		Ptr to data buffer
//		Size of buffer
//		Number of separate objects in buffer
//	
//
// Return Value:
//		ERROR_MORE_DATA if buffer too small
//		ERROR_SUCCESS all other situations
//--
DWORD APIENTRY 
PerfCollect(
    IN LPWSTR lpValueName,
    IN OUT LPVOID *lppData,
    IN OUT LPDWORD lpcbTotalBytes,
    IN OUT LPDWORD lpNumObjectTypes
	)
{
    PPERF_INSTANCE_DEFINITION   pPerfInstanceDef;
	PFTD_HEADER_DEFINITION		pFtdHeaderDefinition;
    ftd_perf_instance_t			*pThisDeviceInstanceData = NULL;
    PFTD_COUNTERS				pDeviceCounters;
    BOOL                        bFreeMutex = FALSE;
    ULONG                       SpaceNeeded;
	DWORD						dwQueryType;
    DWORD                       dwInstanceIndex;

    //
    // Make sure the FtdPerfOpen function
	// ran successfully.
    //
    if( !bInitialized  || (perfp->pData == NULL))
	{
	    *lpcbTotalBytes = (DWORD) 0;
	    *lpNumObjectTypes = (DWORD) 0;

		//
		// Initialization failed. Quit.
		//
		return ERROR_SUCCESS;
	}


    //
	// Parse argument string to see what kind
	// of data the caller is asking for.
    //
	dwQueryType = PerfGetQueryType( lpValueName );
    
    if( dwQueryType == PERF_QUERY_TYPE_FOREIGN )
	{
		//
		// Can't service foreign requests.
		//
		*lpcbTotalBytes = (DWORD) 0;
		*lpNumObjectTypes = (DWORD) 0;
		return ERROR_SUCCESS;
	}

	if( dwQueryType == PERF_QUERY_TYPE_ITEMS )
	{
		//
		// Caller passed a list of object-type
		// indexes. See if we provide data for any
		// of them.
		//
		if( !PerfIsNumberInList( 
				FtdHeaderDefinition.
					FtdDevice.
						ObjectNameTitleIndex,
				lpValueName ))
		{
			//
			// We don't provide data for any of the
			// objects they've asked for. Quit.
			//
			*lpcbTotalBytes = (DWORD) 0;
			*lpNumObjectTypes = (DWORD) 0;
			return ERROR_SUCCESS;
		}
	}

	//
	// Make sure there's enough room in the caller's
	// buffer for everything we're sending back.
	//
    SpaceNeeded = sizeof(FTD_HEADER_DEFINITION) +
        (FTD_MAX_GROUPS * FTD_MAX_DEVICES * (
            sizeof(PERF_INSTANCE_DEFINITION) +
            QUADWORD_MULTIPLE(MAX_SIZEOF_INSTANCE_NAME) +
            sizeof(FTD_COUNTERS)));

    if ( *lpcbTotalBytes < SpaceNeeded ) {
        // not enough room so return nothing.
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

  
	SetEvent(perfp->hGetEvent);
	if (WaitForSingleObject(perfp->hSetEvent, 1000) != WAIT_OBJECT_0) {
		DWORD err = GetLastError();
		//
		// We don't provide data for any of the
		// objects they've asked for. Quit.
		//
		*lpcbTotalBytes = (DWORD) 0;
		*lpNumObjectTypes = (DWORD) 0;
		return ERROR_SUCCESS;
	}
    


    //
	// Copy the pre-initialized PERF_OBJECT_TYPE and
	// PERF_COUNTER_DEFINITIONs into the caller's data
	// buffer.
	//
	pFtdHeaderDefinition = 
			(PFTD_HEADER_DEFINITION) *lppData;

	memmove(
		pFtdHeaderDefinition,
		&FtdHeaderDefinition,
		sizeof( FTD_HEADER_DEFINITION ));

    
    // process each of the instances in the shared memory buffer
    if (perfp->hMutex != NULL) {
        if (WaitForSingleObject (perfp->hMutex, SHARED_MEMORY_MUTEX_TIMEOUT) == WAIT_FAILED) {
            // unable to obtain a lock
            bFreeMutex = FALSE;
        } else {
            bFreeMutex = TRUE;
        }
    } else {
        bFreeMutex = FALSE;
    }
   


    // prepare to read the instance data
    pPerfInstanceDef = (PERF_INSTANCE_DEFINITION *) &pFtdHeaderDefinition[1];

    // point to the first instance structure in the shared buffer
    pThisDeviceInstanceData = perfp->pData;

    // process each of the instances in the shared memory buffer
    dwInstanceIndex = 0;
    while ( *((int*)pThisDeviceInstanceData) != -1){
        // initialize this instance
        MonBuildInstanceDefinition (
            pPerfInstanceDef,
            &pDeviceCounters, // pointer to first byte after instance def
            0,  // no parent
            0,  //  object to reference
            (DWORD)PERF_NO_UNIQUE_ID,
            pThisDeviceInstanceData->wcszInstanceName);

		pDeviceCounters->CounterBlock.ByteLength = sizeof(FTD_COUNTERS);

        // set pointer to first counter data field
        pDeviceCounters->actual = pThisDeviceInstanceData->actual;
        pDeviceCounters->effective = pThisDeviceInstanceData->effective;
        pDeviceCounters->entries = pThisDeviceInstanceData->entries;
        pDeviceCounters->sectors = pThisDeviceInstanceData->sectors;
        pDeviceCounters->pctdone = pThisDeviceInstanceData->pctdone;
        pDeviceCounters->pctbab = pThisDeviceInstanceData->pctbab;
        pDeviceCounters->bytesread = pThisDeviceInstanceData->bytesread;
        pDeviceCounters->byteswritten = pThisDeviceInstanceData->byteswritten;
        
        // setup for the next instance
        dwInstanceIndex++;
        pThisDeviceInstanceData++;
        pPerfInstanceDef =
            (PERF_INSTANCE_DEFINITION *)((LPBYTE)pDeviceCounters + sizeof(FTD_COUNTERS));
    }

    if (dwInstanceIndex == 0) {
        // zero fill one instance sized block of data if there are no
        // data instances

        memset (pPerfInstanceDef, 0,
            (sizeof(PERF_INSTANCE_DEFINITION) +
            QUADWORD_MULTIPLE(MAX_SIZEOF_INSTANCE_NAME) +
            sizeof(FTD_COUNTERS)));

        // adjust pointer to point to end of zeroed block
        (BYTE *)pPerfInstanceDef += (sizeof(PERF_INSTANCE_DEFINITION) +
            QUADWORD_MULTIPLE(MAX_SIZEOF_INSTANCE_NAME) +
            sizeof(FTD_COUNTERS));
    }

    // done with the shared memory so free the mutex if one was 
    // acquired

    if (bFreeMutex) {
        ReleaseMutex (perfp->hMutex);
    }

    // update arguments for return
    *lppData = (PVOID)pPerfInstanceDef;

    *lpNumObjectTypes = 1;
    pFtdHeaderDefinition->FtdDevice.NumInstances = dwInstanceIndex;

    pFtdHeaderDefinition->FtdDevice.TotalByteLength =
    *lpcbTotalBytes = (DWORD)((PBYTE)pPerfInstanceDef -
                              (PBYTE) pFtdHeaderDefinition);

    return ERROR_SUCCESS;
}

//++
// Function:
//		PerfClose
//
// Description:
//		This function closes down the
//		collection DLL. 
//
// Arguments:
//		(None)
//	
//
// Return Value:
//		ERROR_SUCCESS always
//--
DWORD APIENTRY 
PerfClose( )
{
	ftd_perf_delete(perfp);

    //
	// One less thread is using the DLL
	//
	if( --dwOpenCount <= 0 )
	{
		PerfCloseEventLog();	
	}	

	return ERROR_SUCCESS;
}

