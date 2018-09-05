#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <winioctl.h>

#include "error.h"

//--------------------------------------------------------------------
//
// PrintNtError
//
// Translates an NTDLL error code into its text equivalent. This
// only deals with ones commonly returned by defragmenting FS Control
// commands.
//--------------------------------------------------------------------
void PrintNtError( NTSTATUS Status, char *error )
{
	switch( Status ) {
	case STATUS_SUCCESS:
		sprintf(error, "STATUS_SUCCESS");
		break;
	case STATUS_INVALID_PARAMETER:
		sprintf(error, "STATUS_INVALID_PARAMETER");
		break;
	case STATUS_BUFFER_TOO_SMALL:
		sprintf(error, "STATUS_BUFFER_TOO_SMALL");
		break;
	case STATUS_ALREADY_COMMITTED:
		sprintf(error, "STATUS_ALREADY_COMMITTED");
		break;
	case STATUS_INVALID_DEVICE_REQUEST:
		sprintf(error, "STATUS_INVALID_DEVICE_REQUEST");
		break;
	default:
		sprintf(error, "0x%08x", Status );
		break;
	}		  
}


//--------------------------------------------------------------------
//
// PrintWin32Error
// 
// Translates a Win32 error into a text equivalent
//
//--------------------------------------------------------------------
void PrintWin32Error( DWORD ErrorCode, char *error )
{
	LPVOID lpMsgBuf;
 
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					NULL, ErrorCode, 
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR) &lpMsgBuf, 0, NULL );
	sprintf(error, "%s", (LPTSTR) lpMsgBuf );
	LocalFree( lpMsgBuf );
}


