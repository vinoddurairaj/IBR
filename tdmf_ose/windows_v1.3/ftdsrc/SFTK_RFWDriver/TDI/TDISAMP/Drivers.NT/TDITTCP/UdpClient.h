/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#ifndef __UDPCLIENT_H__
#define __UDPCLIENT_H__

#include	"NDIS.H"
#include	"TDI.H"
#include	"TDIKRNL.H"

#include "INetInc.h"
#include "TTCPAPI.h"


// Copyright And Configuration Management ----------------------------------
//
//          Header For TDI Test (TTCP) Udp Client Device - UDPCLIENT.h
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
struct _UDPC_DEVICE_CONTEXT
{
   ULONG    UdpClientStuff;
   TDI_PROVIDER_INFO TdiProviderInfo;
}
   UDPC_DEVICE_CONTEXT, *PUDPC_DEVICE_CONTEXT;

/////////////////////////////////////////////////////////////////////////////
//// Device Dispatch Functions

NTSTATUS
UDPC_DeviceCleanup(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pFlushIrp
    );


NTSTATUS
UDPC_DeviceOpen(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
UDPC_DeviceClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
UDPC_DeviceRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
UDPC_DeviceWrite(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
UDPC_DeviceIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

VOID
UDPC_DriverUnload(
   IN PDRIVER_OBJECT DriverObject
   );


/////////////////////////////////////////////////////////////////////////////
//// Support Functions

NTSTATUS
UDPC_DeviceLoad(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
UDPC_DeviceUnload(
   IN PDEVICE_OBJECT pDeviceObject
   );

#endif // __UDPCLIENT_H__

