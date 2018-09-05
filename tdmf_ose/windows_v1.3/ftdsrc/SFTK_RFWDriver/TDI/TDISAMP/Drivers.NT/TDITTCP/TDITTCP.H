/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#ifndef __TDITTCP_H__
#define __TDITTCP_H__

#include	"NDIS.H"
#include	"TDI.H"
#include	"TDIKRNL.H"

#include "TTCPAPI.h"

#include "KSUtil.h"

#include "TcpServer.h"
#include "TcpClient.h"
#include "UdpServer.h"
#include "UdpClient.h"


// Copyright And Configuration Management ----------------------------------
//
//              Header For TDI Test TCP (TTCP) Driver - TDITTCP.h
//
//                  PCAUSA TDI Client Samples For Windows NT
//
//      Copyright (c) 1999-2001 Printing Communications Associates, Inc.
//                                - PCAUSA -
//
//                             Thomas F. Divine
//                           4201 Brunswick Court
//                        Smyrna, Georgia 30080 USA
//                              (770) 432-4580
//                            tdivine@pcausa.com
//
// End ---------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
//// STRUCTURE DEFINITIONS

typedef
VOID
(*PDEVICE_UNLOAD) (
    IN struct _DEVICE_OBJECT *DeviceObject
    );

typedef
struct _DEVICE_EXTENSION
{
   PDEVICE_OBJECT       pDeviceObject;
   PDRIVER_DISPATCH     MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
   PDEVICE_UNLOAD       DeviceUnload;

   union
   {
      TCPS_DEVICE_CONTEXT  TcpServerContext;
      TCPC_DEVICE_CONTEXT  TcpClientContext;
      UDPS_DEVICE_CONTEXT  UdpServerContext;
      UDPC_DEVICE_CONTEXT  UdpClientContext;
   };
}
   DEVICE_EXTENSION, *PDEVICE_EXTENSION;



/////////////////////////////////////////////////////////////////////////////
//// GLOBAL DATA

extern PDRIVER_OBJECT   g_pTheDriverObject;

#define  UDP_GUARD_BUFFER_LENGTH 4


/////////////////////////////////////////////////////////////////////////////
//// Device Dispatch Functions

NTSTATUS
TDITTCPDeviceCleanup(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pFlushIrp
    );


NTSTATUS
TDITTCPDeviceOpen(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
TDITTCPDeviceClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
TDITTCPDeviceRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
TDITTCPDeviceWrite(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
TDITTCPDeviceIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

VOID
TDITTCPDriverUnload(
   IN PDRIVER_OBJECT DriverObject
   );


/////////////////////////////////////////////////////////////////////////////
//// DEBUG/UTILITY Functions

void TDITTCP_FillPatternBuffer( PUCHAR cp, int cnt );


#ifdef DBG

NTSTATUS
TDITTCP_DumpTestParams(
   PTDITTCP_TEST_PARAMS pTestParams
   );

#endif

#endif // __TDITTCP_H__

