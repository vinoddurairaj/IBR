#ifndef __TDIHAPI_H__
#define __TDIHAPI_H__

/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

// Copyright And Configuration Management ----------------------------------
//
//               TDI Hook Driver (PCATDIH) IOCTL Codes - TDIHAPI.H
//
//      Copyright (c) 2000-2001 Printing Communications Associates, Inc.
//                                - PCAUSA -
//                          http://www.pcausa.com
//
//                             Thomas F. Divine
//                           4201 Brunswick Court
//                        Smyrna, Georgia 30080 USA
//                              (770) 432-4580
//                         mailto:tdivine@pcausa.com
// 
// End ---------------------------------------------------------------------

//
// API Version Information
// -----------------------
// Make sure that this is coordinated with information in the VERSION
// resource.
//
#define PCATDIH_API_VERSION     0x02000311

//
// PCATDIH Driver Device Name Strings
//
#define PCATDIH_BASE_NAME_A        "PCATDIH"
#define PCATDIH_BASE_NAME_W        L"PCATDIH"

#define PCATDIH_WIN32_NAME_A        "\\\\.\\PCATDIH"
#define PCATDIH_WIN32_NAME_W        L"\\\\.\\PCATDIH"

#ifdef UNICODE

#define PCATDIH_BASE_NAME       PCATDIH_BASE_NAME_W
#define PCATDIH_WIN32_NAME      PCATDIH_WIN32_NAME_W

#else

#define PCATDIH_BASE_NAME       PCATDIH_BASE_NAME_A
#define PCATDIH_WIN32_NAME      PCATDIH_WIN32_NAME_A

#endif



/////////////////////////////////////////////////////////////////////////////
//// Function Codes For TDI TTCP TCP Server IOCTL
//

//
// PCATDIH Device Types (For IOCTL)
//
#define FILE_DEVICE_PCATDIH_BASE 0x00008000  // First User Device Type

#define IOCTL_W32API_TEST\
   CTL_CODE(FILE_DEVICE_PCATDIH_BASE, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS)



#endif // __TDIHAPI_H__

