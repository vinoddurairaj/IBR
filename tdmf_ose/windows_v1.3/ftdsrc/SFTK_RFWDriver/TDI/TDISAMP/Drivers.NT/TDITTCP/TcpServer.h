/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#ifndef __TCPSERVER_H__
#define __TCPSERVER_H__

#include	"NDIS.H"
#include	"TDI.H"
#include	"TDIKRNL.H"

#include "INetInc.h"
#include "TTCPAPI.h"


// Copyright And Configuration Management ----------------------------------
//
//          Header For TDI Test (TTCP) Tcp Server Device - TCPSERVER.h
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
//// GLOBAL DATA

/////////////////////////////////////////////////////////////////////////////
//// STRUCTURE DEFINITIONS

typedef
struct _TCPS_DEVICE_CONTEXT
{
   ULONG    TcpStuff;
   TDI_PROVIDER_INFO TdiProviderInfo;
}
   TCPS_DEVICE_CONTEXT, *PTCPS_DEVICE_CONTEXT;

/////////////////////////////////////////////////////////////////////////////
//// Device Dispatch Functions

NTSTATUS
TCPS_DeviceCleanup(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pFlushIrp
    );


NTSTATUS
TCPS_DeviceOpen(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
TCPS_DeviceClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
TCPS_DeviceRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
TCPS_DeviceWrite(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
TCPS_DeviceIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

VOID
TCPS_DriverUnload(
   IN PDRIVER_OBJECT DriverObject
   );


/////////////////////////////////////////////////////////////////////////////
//// Support Functions

NTSTATUS
TCPS_DeviceLoad(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
TCPS_DeviceUnload(
   IN PDEVICE_OBJECT pDeviceObject
   );

#endif // __TCPSERVER_H__

