/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include "windows.h"
#include "stdio.h"
#include "WinIoctl.h"
#include "TDIHApi.h"

// Copyright And Configuration Management ----------------------------------
//
//               TDI Hook Driver (PCATDIH) Win32 API Test Utility
//
//                        MAIN Entry Point - TDIHTest.c
//
//     Copyright (c) 2000-2001 Printing Communications Associates, Inc.
//                               - PCAUSA -
//
//                             Thomas F. Divine
//                           4201 Brunswick Court
//                        Smyrna, Georgia 30080 USA
//                              (770) 432-4580
//                            tdivine@pcausa.com
// 
// End ---------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
//// main
//
// Purpose
// Console application MAIN entry point.
//
// Parameters
//
// Return Value
//
// Remarks
//

int main( int argc, char **argv )
{
   HANDLE   hPCATDIH;
   BOOLEAN  bRc;
   DWORD    bytesReturned;

   printf( "PCAUSA TDI Hook Driver (PCATDIH) Test Application\n" );

   hPCATDIH = CreateFile(
               PCATDIH_WIN32_NAME,
               GENERIC_READ | GENERIC_WRITE,
               0,
               NULL,
               OPEN_EXISTING,
               FILE_ATTRIBUTE_NORMAL,
               NULL
               );

   if( hPCATDIH == INVALID_HANDLE_VALUE )
   {
      printf("Can't get a handle to PCATDIH\n");
      return( 0 );
   }

   printf("Opened handle to PCATDIH\n");

   //
   // Start The TDI TTCP Test
   //
   bRc = DeviceIoControl(
            hPCATDIH, 
            IOCTL_W32API_TEST,
            NULL,
            0,
            NULL,
            0,
            &bytesReturned,
            NULL 
            );

	if ( !bRc )
	{
      printf ( "Error in W32API DeviceIoControl Call: %d", GetLastError());
   }
   else
   {
      printf( "\nW32API Test Was Successful\n" );
   }

   //
   // Point proven.  Be a nice program and close up shop.
   //
   CloseHandle(hPCATDIH);

   return 1;
}
