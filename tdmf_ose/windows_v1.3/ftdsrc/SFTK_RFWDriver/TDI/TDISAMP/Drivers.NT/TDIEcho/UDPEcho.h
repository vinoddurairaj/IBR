/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#ifndef __UDPECHO_H__
#define __UDPECHO_H__

#include   "KSUtil.h"

// Copyright And Configuration Management ----------------------------------
//
//          Header For TDI UDP Echo Server Implementation - UDPEcho.H
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

NTSTATUS UDPS_Startup( PDEVICE_OBJECT pDeviceObject );
VOID UDPS_Shutdown( PDEVICE_OBJECT pDeviceObject );

#ifdef __cplusplus
}
#endif


#endif // __UDPECHO_H__

