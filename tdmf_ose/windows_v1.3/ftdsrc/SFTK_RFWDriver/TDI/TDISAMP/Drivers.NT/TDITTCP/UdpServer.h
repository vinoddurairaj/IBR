/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#ifndef __UDPSERVER_H__
#define __UDPSERVER_H__

#include	"NDIS.H"
#include	"TDI.H"
#include	"TDIKRNL.H"

#include "INetInc.h"
#include "TTCPAPI.h"


// Copyright And Configuration Management ----------------------------------
//
//          Header For TDI Test (TTCP) Udp Server Device - UDPSERVER.h
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
struct _UDPS_DEVICE_CONTEXT
{
   ULONG    UdpServerStuff;
   TDI_PROVIDER_INFO TdiProviderInfo;
}
   UDPS_DEVICE_CONTEXT, *PUDPS_DEVICE_CONTEXT;

/////////////////////////////////////////////////////////////////////////////
//// Device Dispatch Functions

NTSTATUS
UDPS_DeviceCleanup(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pFlushIrp
    );


NTSTATUS
UDPS_DeviceOpen(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
UDPS_DeviceClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
UDPS_DeviceRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
UDPS_DeviceWrite(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

NTSTATUS
UDPS_DeviceIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    );

VOID
UDPS_DriverUnload(
   IN PDRIVER_OBJECT DriverObject
   );


/////////////////////////////////////////////////////////////////////////////
//// Support Functions

NTSTATUS
UDPS_DeviceLoad(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
UDPS_DeviceUnload(
   IN PDEVICE_OBJECT pDeviceObject
   );

#endif // __UDPSERVER_H__

