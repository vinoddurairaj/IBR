/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#ifndef __TCPCLIENT_H__
#define __TCPCLIENT_H__

#include	"NDIS.H"
#include	"TDI.H"
#include	"TDIKRNL.H"

#include "INetInc.h"
#include "TTCPAPI.h"


// Copyright And Configuration Management ----------------------------------
//
//          Header For TDI Test (TTCP) Tcp Client Device - TCPCLIENT.h
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
struct _TCPC_DEVICE_CONTEXT
{
   ULONG    TcpStuff;
   TDI_PROVIDER_INFO TdiProviderInfo;
}
   TCPC_DEVICE_CONTEXT, *PTCPC_DEVICE_CONTEXT;

/////////////////////////////////////////////////////////////////////////////
//// Device Dispatch Functions

NTSTATUS
TCPC_DeviceCleanup(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pFlushIrp
    );


NTSTATUS
TCPC_DeviceOpen(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
TCPC_DeviceClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
TCPC_DeviceRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
TCPC_DeviceWrite(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
TCPC_DeviceIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

VOID
TCPC_DriverUnload(
   IN PDRIVER_OBJECT DriverObject
   );


/////////////////////////////////////////////////////////////////////////////
//// Support Functions

NTSTATUS
TCPC_DeviceLoad(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
TCPC_DeviceUnload(
   IN PDEVICE_OBJECT pDeviceObject
   );

#endif // __TCPCLIENT_H__

