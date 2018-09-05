/*****************************************************************************
 *                                                                           *
 *  This software  is the licensed software of Fujitsu Software              *
 *  Technology Corporation                                                   *
 *                                                                           *
 *  Copyright (c) 2004 by Softek Storage Technology Corporation              *
 *                                                                           *
 *  THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF            *
 *  FUJITSU SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED             *
 *  UNDER LICENSE FROM FUJITSU SOFTWARE TECHNOLOGY CORPORATION               *
 *                                                                           *
 *****************************************************************************

 *****************************************************************************
 *                                                                           *
 *  Revision History:                                                        *
 *                                                                           *
 *  Date        By              Change                                       *
 *  ----------- --------------  -------------------------------------------  *
 *  04-27-2004  Parag Sanghvi   Initial version.                             *
 *                                                                           *
 *                                                                           *
 *****************************************************************************/

#include "CLI.h"

#define SCSI_PORT_DEVICE_NAME			"\\\\.\\scsi%d:"
#define PHYSICAL_DRIVE_DEVICE_NAME		"\\\\.\\PhysicalDrive%d"

// Return -
//		TRUE	- Success
//		FALSE	- Error
BOOL OpenDevice(HANDLE *PtrHandle, CHAR * DeviceName)
{
	DWORD dwDesiredAccess_Rd_Wr = GENERIC_READ | GENERIC_WRITE;
	DWORD dwDesiredAccess_Rd	= GENERIC_READ ;
	DWORD dwDesiredAccess_Query = 0;
	DWORD dwDesiredAccess;
	DWORD i = 0;

	dwDesiredAccess = dwDesiredAccess_Rd_Wr;
	dwDesiredAccess = dwDesiredAccess_Rd;
	dwDesiredAccess = dwDesiredAccess_Query;

	do {
		switch(i)	{
			case 0 : dwDesiredAccess = dwDesiredAccess_Query; break;
			case 1 : dwDesiredAccess = dwDesiredAccess_Rd; break;
			case 2 : dwDesiredAccess = dwDesiredAccess_Rd_Wr; break;
		}

	*PtrHandle = CreateFile(	DeviceName,
							dwDesiredAccess,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL, OPEN_EXISTING, 0, NULL);
     
	if (*PtrHandle == INVALID_HANDLE_VALUE) {
		// printf("Error 0x%08x dwDesiredAccess %08X opening the Device %s.\n",GetLastError(),dwDesiredAccess, DeviceName );
        return FALSE;
    }
	else
		break;	// success

	i++;
	} while (i < 3);

	return TRUE;
} // OpenDevice()

VOID CloseDevice(HANDLE Handle)
{
	if (Handle != INVALID_HANDLE_VALUE) {
		CloseHandle(Handle);
	}
} // CloseDevice()

BOOL SendIoctl( HANDLE Handle,			DWORD dwIoControlCode, 
				LPVOID lpInBuffer,		DWORD nInBufferSize,
				LPVOID lpOutBuffer,		DWORD nOutBufferSize,
				LPDWORD lpBytesReturned)
{
	BOOL	bret = FALSE;
	bret = DeviceIoControl(	Handle,
							dwIoControlCode,
							lpInBuffer,
							nInBufferSize,
							lpOutBuffer,
							nOutBufferSize,
							lpBytesReturned,
							NULL);
	if (bret == FALSE)
	{ // If the function fails, the return value is zero.
		//printf("Error getting drive %d (DiskNumIndex %d) information. GetLastError() %d (0x%08x)\n", i, DiskNumIndex, GetLastError(), GetLastError());
		GetErrorText();	// retrieve Error text message from GetLastError if we get and display to screen
	}

	return bret;
}