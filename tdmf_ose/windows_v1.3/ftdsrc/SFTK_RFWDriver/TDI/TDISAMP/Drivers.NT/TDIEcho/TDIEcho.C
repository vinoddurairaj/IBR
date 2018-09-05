/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include "ndis.h"
#include   "TDI.H"
#include   "TDIKRNL.H"
#include "KSUtil.h"

#include "TCPEcho.h"
#include "UDPEcho.h"
#include "TDIEcho.h"

// Copyright And Configuration Management ----------------------------------
//
//                 TDI Echo Server Driver Entry Points - TDIEcho.c
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

//
// The following is the debug print macro- when we are building checked
// drivers "DBG" will be defined (by the \ddk\setenv.cmd script), and we
// will see debug messages appearing on the KD screen on the host debug
// machine. When we build free drivers "DBG" is not defined, and calls
// to TDIEchoKdPrint are removed.
//

#if DBG
#define TDIEchoKdPrint(arg) DbgPrint arg
#else
#define TDIEchoKdPrint(arg)
#endif


NTSTATUS
TDIEchoDispatch(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

VOID
TDIEchoUnload(
   IN PDRIVER_OBJECT DriverObject
   );

NTSTATUS
DriverEntry(
   IN PDRIVER_OBJECT  DriverObject,
   IN PUNICODE_STRING RegistryPath
   )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path
                   to driver-specific key in the registry

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
   PDEVICE_OBJECT       pDeviceObject        = NULL;
   NTSTATUS             ntStatus;
   WCHAR                deviceNameBuffer[]  = L"\\Device\\TDIEcho";
   UNICODE_STRING       deviceNameUnicodeString;
   PDEVICE_EXTENSION    pDeviceExtension;
   WCHAR                deviceLinkBuffer[]  = L"\\DosDevices\\TDIECHO";
   UNICODE_STRING       deviceLinkUnicodeString;


   TDIEchoKdPrint (("TDIECHO.SYS: entering DriverEntry\n"));


   //
   // A real driver would:
   //
   //     1. Report it's resources (IoReportResourceUsage)
   //
   //     2. Attempt to locate the device(s) it supports


   //
   // OK, we've claimed our resources & found our h/w, so create
   // a device and initialize stuff...
   //
   RtlInitUnicodeString(
      &deviceNameUnicodeString,
      deviceNameBuffer
      );


   //
   // Create an EXCLUSIVE device, i.e. only 1 thread at a time can send
   // i/o requests.
   //
   ntStatus = IoCreateDevice(
                  DriverObject,
                  sizeof( DEVICE_EXTENSION ),
                  &deviceNameUnicodeString,
                  FILE_DEVICE_TDIECHO,
                  0,
                  TRUE,
                  &pDeviceObject
                  );

   if (NT_SUCCESS(ntStatus))
   {
      pDeviceExtension = (PDEVICE_EXTENSION ) pDeviceObject->DeviceExtension;

      TCPS_Startup( pDeviceObject );
      UDPS_Startup( pDeviceObject );

      //
      // Set up synchronization objects, state info,, etc.
      //

      //
      // Create a symbolic link that Win32 apps can specify to gain access
      // to this driver/device
      //
      RtlInitUnicodeString (&deviceLinkUnicodeString, deviceLinkBuffer );

      ntStatus = IoCreateSymbolicLink(
                     &deviceLinkUnicodeString,
                     &deviceNameUnicodeString
                     );

      if (!NT_SUCCESS(ntStatus))
      {
         TDIEchoKdPrint (("TDIECHO.SYS: IoCreateSymbolicLink failed\n"));
      }

      //
      // Create dispatch points for device control, create, close.
      //
      DriverObject->MajorFunction[IRP_MJ_CREATE]         = 
      DriverObject->MajorFunction[IRP_MJ_CLOSE]          = 
      DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = TDIEchoDispatch;
      DriverObject->DriverUnload                         = TDIEchoUnload;
  }

done_DriverEntry:

  if (!NT_SUCCESS(ntStatus))
  {
    //
    // Something went wrong, so clean up (free resources, etc.)
    //
    if (pDeviceObject)
      IoDeleteDevice (pDeviceObject);
  }

  return ntStatus;
}



NTSTATUS
TDIEchoDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++

Routine Description:

    Process the IRPs sent to this device.

Arguments:

    DeviceObject - pointer to a device object

    Irp          - pointer to an I/O Request Packet

Return Value:


--*/
{
   PIO_STACK_LOCATION  irpStack;
   PDEVICE_EXTENSION   pDeviceExtension;
   PVOID               ioBuffer;
   ULONG               inputBufferLength;
   ULONG               outputBufferLength;
   ULONG               ioControlCode;
   NTSTATUS            ntStatus;

   Irp->IoStatus.Status      = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0;

   //
   // Get a pointer to the current location in the Irp. This is where
   //     the function codes and parameters are located.
   //
   irpStack = IoGetCurrentIrpStackLocation (Irp);

   //
   // Get a pointer to the device extension
   //
   pDeviceExtension = DeviceObject->DeviceExtension;

   //
   // Get the pointer to the input/output buffer and it's length
   //
   ioBuffer           = Irp->AssociatedIrp.SystemBuffer;
   inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
   outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;


   switch (irpStack->MajorFunction)
   {
      case IRP_MJ_CREATE:

         TDIEchoKdPrint (("TDIECHO.SYS: IRP_MJ_CREATE\n"));

         break;

      case IRP_MJ_CLOSE:

         TDIEchoKdPrint (("TDIECHO.SYS: IRP_MJ_CLOSE\n"));

         break;

      case IRP_MJ_DEVICE_CONTROL:

         TDIEchoKdPrint (("TDIECHO.SYS: IRP_MJ_DEVICE_CONTROL\n"));

         ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

         switch (ioControlCode)
         {
            case IOCTL_TDITCPS_HELLO:
               //
               // Some app is saying hello
               //

               break;

            default:

               Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

               TDIEchoKdPrint (("TDIECHO.SYS: unknown IRP_MJ_DEVICE_CONTROL\n"));

               break;
         }

         break;
   }

   //
   // DON'T get cute and try to use the status field of
   // the irp in the return status.  That IRP IS GONE as
   // soon as you call IoCompleteRequest.
   //
   ntStatus = Irp->IoStatus.Status;

   IoCompleteRequest( Irp, IO_NO_INCREMENT );

   //
   // We never have pending operation so always return the status code.
   //
   return ntStatus;
}



VOID
TDIEchoUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object

Return Value:


--*/
{
   WCHAR                  deviceLinkBuffer[]  = L"\\DosDevices\\TDIECHO";
   UNICODE_STRING         deviceLinkUnicodeString;
   PDEVICE_OBJECT         pDeviceObject, pNextDeviceObject;

   //
   // Free any resources
   //
   pDeviceObject = DriverObject->DeviceObject;

   UDPS_Shutdown( pDeviceObject );
   TCPS_Shutdown( pDeviceObject );

   //
   // Delete the symbolic link
   //
   RtlInitUnicodeString(
      &deviceLinkUnicodeString,
      deviceLinkBuffer
      );

   IoDeleteSymbolicLink( &deviceLinkUnicodeString );

   //
   // Delete the device object
   //
   IoDeleteDevice (DriverObject->DeviceObject);

   TDIEchoKdPrint (("TDIECHO.SYS: unloading\n"));
}
