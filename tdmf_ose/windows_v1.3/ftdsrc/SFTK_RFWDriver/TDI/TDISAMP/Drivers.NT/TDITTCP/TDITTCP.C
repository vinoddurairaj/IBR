/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include "ndis.h"

#include "INetInc.h"
#include "TDITTCP.h"

// Copyright And Configuration Management ----------------------------------
//
//             DriverEntry For TDI Test TCP (TTCP) Driver - TDITTCP.c
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
//// TDITTCP GLOBAL DATA
//
// Some developers have suggested that the gollowing g_ data should
// be placed in the DeviceExtension instead of simply being "ordinary"
// global data.
//
// DeviceExtension memory allocated from the non-paged pool. Device driver
// global memory is also allocated from the non-paged pool - UNLESS you
// use #pragma directives to cause it to be allocated from the paged pool.
//
// In this driver, use of global memory is safe enough...
//
PDRIVER_OBJECT g_pTheDriverObject = NULL;


/////////////////////////////////////////////////////////////////////////////
//// DriverEntry
//
// Purpose
// This routine initializes the TDITTCP driver.
//
// Parameters
//   pDriverObject - Pointer to driver object created by system.
//   RegistryPath - Pointer to the Unicode name of the registry path
//                  for this driver.
//
// Return Value
// The function return value is the final status from the initialization
// operation.
//
// Remarks
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
   PDEVICE_EXTENSION pDeviceExtension = NULL;
   NTSTATUS          LoadStatus, Status = STATUS_SUCCESS;
   ULONG             DevicesCreated = 0;

   //
   // Initialize Global Data
   //
   KdPrint(("TDITTCP: DriverEntry\n") );
   KdPrint(("  RegistryPath: %ws\n", RegistryPath->Buffer) );

   g_pTheDriverObject = pDriverObject;

   LoadStatus = TCPS_DeviceLoad(
                  pDriverObject,
                  RegistryPath
                  );

   if( NT_SUCCESS( LoadStatus ) )
   {
      ++DevicesCreated;
   }

   LoadStatus = TCPC_DeviceLoad(
                  pDriverObject,
                  RegistryPath
                  );

   if( NT_SUCCESS( LoadStatus ) )
   {
      ++DevicesCreated;
   }

   LoadStatus = UDPS_DeviceLoad(
                  pDriverObject,
                  RegistryPath
                  );

   if( NT_SUCCESS( LoadStatus ) )
   {
      ++DevicesCreated;
   }

   LoadStatus = UDPC_DeviceLoad(
                  pDriverObject,
                  RegistryPath
                  );

   if( NT_SUCCESS( LoadStatus ) )
   {
      ++DevicesCreated;
   }

   if( !DevicesCreated )
   {
      return STATUS_UNSUCCESSFUL;
   }

   return STATUS_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
//// TDITTCPDeviceOpen (IRP_MJ_CREATE Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDITTCP device create/open requests.
//
// Parameters
//    pDeviceObject - Pointer to the device object.
//    pIrp - Pointer to the request packet.
//
// Return Value
// Status is returned.
//
// Remarks
//

NTSTATUS
TDITTCPDeviceOpen(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   pDeviceExtension = pDeviceObject->DeviceExtension;

   //
   // Possibly Dispatch To Secondary Function Handler
   // -----------------------------------------------
   // The TDITTCP driver creates four devices:
   //   TDI TTCP TCP Server
   //   TDI TTCP UDP Server
   //   TDI TTCP TCP Client
   //   TDI TTCP UDP Client
   //
   // A different DEVICE_EXTENSION structure is created for each device.
   //
   // The device extension includes a secondary MajorFunction table that
   // points to different functions for each device. This means that each
   // of the four types of devices listed above can have their own
   // DeviceOpen routine.
   //
   if( pDeviceExtension && pDeviceExtension->MajorFunction[ IRP_MJ_CREATE ] )
   {
      return( (*pDeviceExtension->MajorFunction[ IRP_MJ_CREATE ])
               ( pDeviceObject, pIrp )
            );
   }

   KdPrint(("TDITTCPDeviceOpen: Default Handling...\n") );

   //
   // No need to do anything.
   //

   //
   // Fill these in before calling IoCompleteRequest.
   //
   // DON'T get cute and try to use the status field of
   // the irp in the return status.  That IRP IS GONE as
   // soon as you call IoCompleteRequest.
   //

   pIrp->IoStatus.Information = 0;

   TdiCompleteRequest( pIrp, STATUS_SUCCESS );

   return STATUS_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
//// TDITTCPDeviceClose (IRP_MJ_CLOSE Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDITTCP device close requests.
//
// Parameters
//    pDeviceObject - Pointer to the device object.
//    pIrp - Pointer to the request packet.
//
// Return Value
// Status is returned.
//
// Remarks
//

NTSTATUS
TDITTCPDeviceClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   pDeviceExtension = pDeviceObject->DeviceExtension;

   //
   // Possibly Dispatch To Secondary Function Handler
   // -----------------------------------------------
   // The TDITTCP driver creates four devices:
   //   TDI TTCP TCP Server
   //   TDI TTCP UDP Server
   //   TDI TTCP TCP Client
   //   TDI TTCP UDP Client
   //
   // A different DEVICE_EXTENSION structure is created for each device.
   //
   // The device extension includes a secondary MajorFunction table that
   // points to different functions for each device. This means that each
   // of the four types of devices listed above can have their own
   // DeviceOpen routine.
   //
   if( pDeviceExtension && pDeviceExtension->MajorFunction[ IRP_MJ_CLOSE ] )
   {
      return( (*pDeviceExtension->MajorFunction[ IRP_MJ_CLOSE ])
               ( pDeviceObject, pIrp )
            );
   }

   KdPrint(("TDITTCPDeviceClose: Default Handling...\n") );

   //
   // No need to do anything.
   //

   //
   // Fill these in before calling IoCompleteRequest.
   //
   // DON'T get cute and try to use the status field of
   // the irp in the return status.  That IRP IS GONE as
   // soon as you call IoCompleteRequest.
   //

   KdPrint( ("TDITTCPDeviceClose: Closed!!\n") );

   pIrp->IoStatus.Information = 0;

   TdiCompleteRequest( pIrp, STATUS_SUCCESS );

   return STATUS_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
//// TDITTCPDeviceRead (IRP_MJ_READ Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDITTCP device read requests.
//
// Parameters
//    pDeviceObject - Pointer to the device object.
//    pIrp - Pointer to the request packet.
//
// Return Value
// Status is returned.
//
// Remarks
// The TDITTCP sample DOES NOT support device Read/Write requests on the
// device. The DeviceIoControl interface is used instead.
//

NTSTATUS
TDITTCPDeviceRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   pIrp->IoStatus.Information = 0;      // Nothing Returned Yet

   pDeviceExtension = pDeviceObject->DeviceExtension;

   //
   // Possibly Dispatch To Secondary Function Handler
   // -----------------------------------------------
   // The TDITTCP driver creates four devices:
   //   TDI TTCP TCP Server
   //   TDI TTCP UDP Server
   //   TDI TTCP TCP Client
   //   TDI TTCP UDP Client
   //
   // A different DEVICE_EXTENSION structure is created for each device.
   //
   // The device extension includes a secondary MajorFunction table that
   // points to different functions for each device. This means that each
   // of the four types of devices listed above can have their own
   // DeviceOpen routine.
   //
   if( pDeviceExtension && pDeviceExtension->MajorFunction[ IRP_MJ_READ ] )
   {
      return( (*pDeviceExtension->MajorFunction[ IRP_MJ_READ ])
               ( pDeviceObject, pIrp )
            );
   }

   KdPrint(("TDITTCPDeviceRead: Default Handling...\n") );

   //
   // Return Failure
   //
   TdiCompleteRequest( pIrp, STATUS_NOT_IMPLEMENTED );

   return STATUS_NOT_IMPLEMENTED;
}


/////////////////////////////////////////////////////////////////////////////
//// TDITTCPDeviceWrite (IRP_MJ_WRITE Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDITTCP device write requests.
//
// Parameters
//    pDeviceObject - Pointer to the device object.
//    pIrp - Pointer to the request packet.
//
// Return Value
// Status is returned.
//
// Remarks
// The TDITTCP sample DOES NOT support device Read/Write requests on the
// protocol device. The DeviceIoControl interface is used instead.
//

NTSTATUS
TDITTCPDeviceWrite(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   pIrp->IoStatus.Information = 0;      // Nothing Returned Yet

   pDeviceExtension = pDeviceObject->DeviceExtension;

   //
   // Possibly Dispatch To Secondary Function Handler
   // -----------------------------------------------
   // The TDITTCP driver creates four devices:
   //   TDI TTCP TCP Server
   //   TDI TTCP UDP Server
   //   TDI TTCP TCP Client
   //   TDI TTCP UDP Client
   //
   // A different DEVICE_EXTENSION structure is created for each device.
   //
   // The device extension includes a secondary MajorFunction table that
   // points to different functions for each device. This means that each
   // of the four types of devices listed above can have their own
   // DeviceOpen routine.
   //
   if( pDeviceExtension && pDeviceExtension->MajorFunction[ IRP_MJ_WRITE ] )
   {
      return( (*pDeviceExtension->MajorFunction[ IRP_MJ_WRITE ])
               ( pDeviceObject, pIrp )
            );
   }

   KdPrint(("TDITTCPDeviceWrite: Default Handling...\n") );

   //
   // Return Failure
   //
   TdiCompleteRequest( pIrp, STATUS_NOT_IMPLEMENTED );

   return STATUS_NOT_IMPLEMENTED;
}


/////////////////////////////////////////////////////////////////////////////
//// TDITTCPDeviceIoControl (IRP_MJ_DEVICE_CONTROL Dispatch Routine)
//
// Purpose
// This is the dispatch routine for PROTOCOL device IOCTL requests.
//
// Parameters
//    pDeviceObject - Pointer to the device object.
//    pIrp - Pointer to the request packet.
//
// Return Value
// Status is returned.
//
// Remarks
//

NTSTATUS
TDITTCPDeviceIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;
   NTSTATUS             Status;
   PIO_STACK_LOCATION   pIrpSp;
   ULONG                nFunctionCode;

   pIrp->IoStatus.Information = 0;      // Nothing Returned Yet

   pDeviceExtension = pDeviceObject->DeviceExtension;

   //
   // Possibly Dispatch To Secondary Function Handler
   // -----------------------------------------------
   // The TDITTCP driver creates four devices:
   //   TDI TTCP TCP Server
   //   TDI TTCP UDP Server
   //   TDI TTCP TCP Client
   //   TDI TTCP UDP Client
   //
   // A different DEVICE_EXTENSION structure is created for each device.
   //
   // The device extension includes a secondary MajorFunction table that
   // points to different functions for each device. This means that each
   // of the four types of devices listed above can have their own
   // DeviceOpen routine.
   //
   if( pDeviceExtension && pDeviceExtension->MajorFunction[ IRP_MJ_DEVICE_CONTROL ] )
   {
      return( (*pDeviceExtension->MajorFunction[ IRP_MJ_DEVICE_CONTROL ])
               ( pDeviceObject, pIrp )
            );
   }

   KdPrint(("TDITTCPDeviceIoControl: Default Handling...\n") );

   pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

   nFunctionCode=pIrpSp->Parameters.DeviceIoControl.IoControlCode;

   switch( nFunctionCode )
   {
      default:
         KdPrint((  "FunctionCode: 0x%8.8X\n", nFunctionCode ));
         Status = STATUS_NOT_IMPLEMENTED;
         break;
   }

   TdiCompleteRequest( pIrp, Status );

   return( Status );
}


/////////////////////////////////////////////////////////////////////////////
//// TDITTCPDeviceCleanup (IRP_MJ_CLEANUP Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDITTCP device cleanup requests.
//
// Parameters
//    pDeviceObject - Pointer to the device object.
//    pFlushIrp - Pointer to the flush request packet.
//
// Return Value
// Status is returned.
//
// Remarks
//

NTSTATUS
TDITTCPDeviceCleanup(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   pIrp->IoStatus.Information = 0;      // Nothing Returned Yet

   pDeviceExtension = pDeviceObject->DeviceExtension;

   //
   // Possibly Dispatch To Secondary Function Handler
   // -----------------------------------------------
   // The TDITTCP driver creates four devices:
   //   TDI TTCP TCP Server
   //   TDI TTCP UDP Server
   //   TDI TTCP TCP Client
   //   TDI TTCP UDP Client
   //
   // A different DEVICE_EXTENSION structure is created for each device.
   //
   // The device extension includes a secondary MajorFunction table that
   // points to different functions for each device. This means that each
   // of the four types of devices listed above can have their own
   // DeviceOpen routine.
   //
   if( pDeviceExtension && pDeviceExtension->MajorFunction[ IRP_MJ_CLEANUP ] )
   {
      return( (*pDeviceExtension->MajorFunction[ IRP_MJ_CLEANUP ])
               ( pDeviceObject, pIrp )
            );
   }

   KdPrint(("TDITTCPDeviceCleanup: Default Handling...\n"));

   pIrp->IoStatus.Status = STATUS_SUCCESS;

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// TDITTCPDriverUnload
//
// Purpose
//
// Parameters
//   pDriverObject - Pointer to driver object created by system.
//
// Return Value
//
// Remarks
//

VOID
TDITTCPDriverUnload(
    IN PDRIVER_OBJECT pDriverObject
    )
{
   PDEVICE_OBJECT     pDeviceObject;
   PDEVICE_OBJECT     pOldDeviceObject;
   PDEVICE_EXTENSION  pDeviceExtension;

   KdPrint(("TDITTCPDriverUnload: Entry...\n") );

   //
   // Delete The Driver's Devices
   //
   pDeviceObject = pDriverObject->DeviceObject;

   while( pDeviceObject != NULL )
   {
      pDeviceExtension = pDeviceObject->DeviceExtension;

      //
      // Possibly Dispatch To Secondary Function Handler
      // -----------------------------------------------
      // The TDITTCP driver creates four devices:
      //   TDI TTCP TCP Server
      //   TDI TTCP UDP Server
      //   TDI TTCP TCP Client
      //   TDI TTCP UDP Client
      //
      // A different DEVICE_EXTENSION structure is created for each device.
      //
      // The device extension includes a secondary MajorFunction table that
      // points to different functions for each device. This means that each
      // of the four types of devices listed above can have their own
      // DeviceOpen routine.
      //
      if( pDeviceExtension && pDeviceExtension->DeviceUnload )
      {
         (*pDeviceExtension->DeviceUnload)( pDeviceObject );
      }

      //
      // Now Delete This Device Object
      //
      pOldDeviceObject = pDeviceObject;
      pDeviceObject = pDeviceObject->NextDevice;

      IoDeleteDevice( pOldDeviceObject );
   }
}
