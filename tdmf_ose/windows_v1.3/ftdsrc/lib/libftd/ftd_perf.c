/*
 * ftd_perf.c - FTD performance monitor interface
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
#include "ftd_error.h"
#include "ftd_lg.h"

#include <winperf.h>
#include "ftd_perf.h"

static HANDLE gMapHandle = NULL;

int
ftd_perf_init(ftd_perf_t *perfp, BOOL bReadOnlyAccess)
{
    LONG				Status, FtdStatus;
    TCHAR				szMappedObject[] = SHARED_MEMORY_OBJECT_NAME;
    BOOL				bFreeMutex;
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;

    SetLastError (ERROR_SUCCESS);   // just to clear it out

	// Initialize a security descriptor and assign it a NULL 
	// discretionary ACL to allow unrestricted access. 
	// Assign the security descriptor to a file. 
	if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
		return FALSE;

	if (!SetSecurityDescriptorDacl(&sd, TRUE, (PACL) NULL, FALSE))
		return FALSE;

	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = &sd;
	sa.bInheritHandle = FALSE;
    
	// now create the file mapping
	perfp->hSharedMemory = CreateFileMapping(
                    (HANDLE)0xFFFFFFFF,		// to create a temp file
                    &sa,					// default security
                    PAGE_READWRITE,         // to allow read & write access
                    0,
				    SHARED_MEMORY_OBJECT_SIZE,  // file size
				    szMappedObject);			// object name

    FtdStatus = GetLastError();    // to see if this is the first opening

    // return error if unsuccessful
    if (perfp->hSharedMemory == NULL) 
    {
        // this is fatal, if we can't get data then there's no
        // point in continuing.
		return -1;
    } 
    else 
    {
        // the application memory file was created/access successfully
        // so we need to get the sync. events and mutex before we use it
        perfp->hMutex = CreateMutex (&sa, FALSE, SHARED_MEMORY_MUTEX_NAME);
		if (perfp->hMutex == NULL)
			return -1;

		perfp->hGetEvent = CreateEvent(&sa, FALSE, FALSE, FTD_PERF_GET_EVENT_NAME);
        if (perfp->hGetEvent == NULL) 
        {
			CloseHandle(perfp->hMutex);
			return -1;
		}

		perfp->hSetEvent = CreateEvent(&sa, FALSE, FALSE, FTD_PERF_SET_EVENT_NAME);
        if (perfp->hSetEvent == NULL) 
        {
			CloseHandle(perfp->hGetEvent);
			CloseHandle(perfp->hMutex);
			return -1;
		}

		// if successful then wait for ownership, otherwise,
        // we'll just take our chances.
        if (perfp->hMutex != NULL) 
        {
            if (WaitForSingleObject (perfp->hMutex,
                SHARED_MEMORY_MUTEX_TIMEOUT) == WAIT_FAILED) 
            {
                // unable to obtain a lock
                bFreeMutex = FALSE;
            } 
            else 
            {
                bFreeMutex = TRUE;
            }
        } 
        else 
        {
            bFreeMutex = FALSE;
        }

        if (perfp->pData!=NULL)
        {
            UnmapViewOfFile(gMapHandle);
            CloseHandle(perfp->hSharedMemory);
        }

        if (FtdStatus != ERROR_ALREADY_EXISTS) 
        {
            // this is the first access to the file so initialize the
            // instance count
            gMapHandle = perfp->pData = (ftd_perf_instance_t *)MapViewOfFile(
                            perfp->hSharedMemory,  // shared mem handle
				            FILE_MAP_WRITE,         // access desired
				            0,                      // starting offset
				            0,
				            0);                     // map the entire object
            if (perfp->pData == NULL) 
            {
                Status = -1;
                // this is fatal, if we can't get data then there's no
                // point in continuing.
            } 
            else 
            {
				Status = ERROR_SUCCESS;
	        }
        } 
        else 
        {
            // the status is ERROR_ALREADY_EXISTS which is successful
            Status = -1;//ERROR_SUCCESS;
        }

        // ****
        // STEVE 2004
        // Why do we re-allocate this here? And why do the previous one 
        // if we will do this one after it? Let's only do it if we were 
        // unable to do it before!
        // ****

        // see if Read Only access is required
        if (Status != ERROR_SUCCESS) 
        {
            // ****
            // As a minimum, lets get rid of the previous one!
            // ****
            if (perfp->pData!=NULL)
            {
                UnmapViewOfFile(gMapHandle);
                CloseHandle(perfp->hSharedMemory);
            }

            // by now the shared memory has already been initialized so
            // we if we don't need write access any more or if it has
            // already been opened, then open with the desired access
            gMapHandle = perfp->pData = (ftd_perf_instance_t *)MapViewOfFile(
                            perfp->hSharedMemory,  // shared mem handle
                    (bReadOnlyAccess ? FILE_MAP_READ : FILE_MAP_WRITE), // access desired
		            0,                      // starting offset
		            0,
		            0);                     // map the entire object

            if (perfp->pData == NULL) 
            {
                Status = -1;
                // this is fatal, if we can't get data then there's no
                // point in continuing.
            } 
            else 
            {
				Status = ERROR_SUCCESS;
	        }
        }

        // done with the shared memory so free the mutex if one was
        // acquired

        if (bFreeMutex) 
        {
            ReleaseMutex (perfp->hMutex);
        }
    }

    return Status;
}

/*
 * ftd_perf_create -- create a ftd_perf_t object
 */
ftd_perf_t *
ftd_perf_create(void)
{
	ftd_perf_t	*perfp;

    if ((perfp = (ftd_perf_t*)calloc(1, sizeof(ftd_perf_t))) == NULL) 
    {
		return NULL;
	}

	perfp->magicvalue = FTDPERFMAGIC;

	return perfp;
}

/*
 * ftd_perfp_delete -- delete a ftd_perfp_t object
 */
BOOL
ftd_perf_delete(ftd_perf_t *perfp)
{


    if (perfp && perfp->magicvalue != FTDPERFMAGIC) 
    {
		// not a valid perf object
		return FALSE;
	}

    //
    // Get rid of our view of the memory mapped file we took
    // Hopefully nobody is looking at this anymore!!
    //
    if (gMapHandle)
        UnmapViewOfFile(gMapHandle);

	if (perfp->hSharedMemory)
		CloseHandle (perfp->hSharedMemory);

	if (perfp->hGetEvent)
		CloseHandle(perfp->hGetEvent);

	if (perfp->hSetEvent)
		CloseHandle(perfp->hSetEvent);

	if (perfp->hMutex)
		CloseHandle (perfp->hMutex);

	free(perfp);

	return TRUE;
}

