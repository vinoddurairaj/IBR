/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#ifndef __TCPECHO_H__
#define __TCPECHO_H__

#include   "KSUtil.h"

// Copyright And Configuration Management ----------------------------------
//
//          Header For TDI TCP Echo Server Implementation - TCPEcho.H
//
//                     Network Development Framework (NDF)
//                                    For
//                          Windows 95 And Windows NT
//
//      Copyright (c) 1997-2001, Printing Communications Associates, Inc.
//
//                             Thomas F. Divine
//                           4201 Brunswick Court
//                        Smyrna, Georgia 30080 USA
//                              (770) 432-4580
//                            tdivine@pcausa.com
// 
// End ---------------------------------------------------------------------


#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS TCPS_Startup( PDEVICE_OBJECT pDeviceObject );
VOID TCPS_Shutdown( PDEVICE_OBJECT pDeviceObject );

#ifdef __cplusplus
}
#endif


#endif // __TCPECHO_H__

