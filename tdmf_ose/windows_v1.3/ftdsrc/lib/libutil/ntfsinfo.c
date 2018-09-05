//====================================================================
//
// Ntsinfo.c
//
// Shows NTFS volume information.
//
//====================================================================

//// Mike Pollett need for windows .h files
//#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x0400
//#endif

#include <windows.h>
#include <stdio.h>
#include <winioctl.h>
#include <conio.h>

#include "list.h"
#include "misc.h"
#include "error.h"
#include "ntfsinfo.h"


//--------------------------------------------------------------------
//                      F U N C T I O N S
//--------------------------------------------------------------------

//--------------------------------------------------------------------
//
// GetNTFSInfo
//
// Open the volume and query its data.
//
//--------------------------------------------------------------------
BOOLEAN GetInfo( int DriveId, PNTFS_VOLUME_DATA_BUFFER VolumeInfo, char *szError ) 
{
	static char			volumeName[] = "\\\\.\\A:";
	HANDLE				volumeHandle;
	IO_STATUS_BLOCK		ioStatus;
	NTSTATUS			status;
	char				error[256];			

	//
	// open the volume
	//
	volumeName[4] = DriveId + 'A'; 
	volumeHandle = CreateFile( volumeName, GENERIC_READ, 
					FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 
					0, 0 );
	if( volumeHandle == INVALID_HANDLE_VALUE )	{

		PrintWin32Error( GetLastError(), error );
		sprintf(szError, "Error opening volume: %s", error);
		return FALSE;
	}

	//
	// Query the volume information
	//
	status = NtFsControlFile( volumeHandle, NULL, NULL, 0, &ioStatus,
						FSCTL_GET_VOLUME_INFORMATION,
						NULL, 0,
						VolumeInfo, sizeof( NTFS_VOLUME_DATA_BUFFER ) );

	if( status != STATUS_SUCCESS ) {
		
		PrintNtError( status, error );
		sprintf(szError, "Error obtaining NTFS information: %s", error);
		CloseHandle( volumeHandle );
		return FALSE;
	}

	//
	// Close the volume
	//
	CloseHandle( volumeHandle );

	return TRUE;
}

//--------------------------------------------------------------------
//
// LocateNDLLCalls
//
// Loads function entry points in NTDLL
//
//--------------------------------------------------------------------
static BOOLEAN LocateNTDLLCalls()
{

 	if( !(NtFsControlFile = (void *) GetProcAddress( GetModuleHandle("ntdll.dll"),
			"NtFsControlFile" )) ) {

		return FALSE;
	}
    
    return TRUE;
}


BOOLEAN GetNTFSInfo( char *cDriveLetter, PNTFS_VOLUME_DATA_BUFFER VolumeInfo, char *szError ) 
{
	int drive;

	if( !LocateNTDLLCalls() ) {

		sprintf(szError, "Not running on supported version of Windows NT.");
		return FALSE;
	}

	if( cDriveLetter[0] >= 'a' && cDriveLetter[0] <= 'z' ) {
		drive = cDriveLetter[0] - 'a';
	} else if( cDriveLetter[0] >= 'A' && cDriveLetter[0] <= 'Z' ) {
		drive = cDriveLetter[0] - 'A';
	} else {
		sprintf(szError, "illegal drive: %c", cDriveLetter[0] );
		return FALSE;
	}

	//
	// Get ntfs volume data
	//
	if( !GetInfo( drive, VolumeInfo, szError ) )
		return FALSE;

	return TRUE;
}

