/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include "ndis.h"
#include "TDI.H"
#include "TDIKRNL.H"
#include "PCATDIH.h"

// Copyright And Configuration Management ----------------------------------
//
//               TDI Filter Driver UDP Device Filter - UDPFILT.c
//
//                Transport Data Interface (TDI) Filter Samples
//                                    For
//                                 Windows NT
//
//     Copyright (c) 2000-2001 Printing Communications Associates, Inc.
//
//                             Thomas F. Divine
//                           4201 Brunswick Court
//                        Smyrna, Georgia 30080 USA
//                              (770) 432-4580
//                            tdivine@pcausa.com
// 
// End ---------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
//                      U D P  D E V I C E  F I L T E R                    //
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//// UDPFilter_InitDeviceExtension
//
// Purpose
//
// Parameters
//
// Return Value
// 
// Remarks
//

NTSTATUS
UDPFilter_InitDeviceExtension(
   PTDIH_DeviceExtension pTDIH_DeviceExtension,
   PDEVICE_OBJECT pFilterDeviceObject
   )
{
   NdisZeroMemory( pTDIH_DeviceExtension, sizeof( TDIH_DeviceExtension ) );

   pTDIH_DeviceExtension->NodeIdentifier.NodeType = TDIH_NODE_TYPE_UDP_FILTER_DEVICE;
   pTDIH_DeviceExtension->NodeIdentifier.NodeSize = sizeof( TDIH_DeviceExtension );

   pTDIH_DeviceExtension->pFilterDeviceObject = pFilterDeviceObject;

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// UDPFilter_Attach
//
// Purpose
//
// Parameters
//
// Return Value
// 
// Remarks
//

NTSTATUS
UDPFilter_Attach(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
   )
{
   NTSTATUS                   status;
   UNICODE_STRING             uniNtNameString;
   PTDIH_DeviceExtension pTDIH_DeviceExtension;
   PDEVICE_OBJECT             pFilterDeviceObject = NULL;
   PDEVICE_OBJECT             pTargetDeviceObject = NULL;
   PFILE_OBJECT               pTargetFileObject = NULL;
   PDEVICE_OBJECT             pLowerDeviceObject = NULL;

   KdPrint(("PCATDIH: UDPFilter_Attach Entry...\n"));

   //
   // Create counted string version of target UDP device name.
   //
   ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );
   RtlInitUnicodeString( &uniNtNameString, DD_UDP_DEVICE_NAME );

   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
   status = IoGetDeviceObjectPointer(
               &uniNtNameString,
               FILE_READ_ATTRIBUTES,
               &pTargetFileObject,   // Call ObDereferenceObject Eventually...
               &pTargetDeviceObject
               );

   if( !NT_SUCCESS(status) )
   {
      KdPrint(("PCATDIH: Couldn't get the UDP Device Object\n"));

      pTargetFileObject = NULL;
      pTargetDeviceObject = NULL;

      return( status );
   }

   //
   // Create counted string version of our UDP filter device name.
   //
   ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );
   RtlInitUnicodeString( &uniNtNameString, TDIH_UDP_DEVICE_NAME );

   //
   // Create The UDP Filter Device Object
   // -----------------------------------
   // IoCreateDevice zeroes the memory occupied by the object.
   //
   // Adopt the DeviceType and Characteristics of the target device.
   //
   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
   status = IoCreateDevice(
               DriverObject,
               sizeof( TDIH_DeviceExtension ),
               &uniNtNameString,
               pTargetDeviceObject->DeviceType,
               pTargetDeviceObject->Characteristics,
               FALSE,                 // This isn't an exclusive device
               &pFilterDeviceObject
               );

   if( !NT_SUCCESS(status) )
   {
      KdPrint(("PCATDIH: Couldn't create the UDP Filter Device Object\n"));

      ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
      ObDereferenceObject( pTargetFileObject );

      pTargetFileObject = NULL;
      pTargetDeviceObject = NULL;

      return( status );
   }

   //
   // Initialize The Extension For The UDP Filter Device Object
   //
   pTDIH_DeviceExtension = (PTDIH_DeviceExtension )( pFilterDeviceObject->DeviceExtension );

   UDPFilter_InitDeviceExtension(
      pTDIH_DeviceExtension,
      pFilterDeviceObject
      );

   // Initialize the Executive spin lock for this device extension.
   KeInitializeSpinLock(&(pTDIH_DeviceExtension->IoRequestsSpinLock));

   // Initialize the event object used to denote I/O in progress.
   // When set, the event signals that no I/O is currently in progress.
   KeInitializeEvent(&(pTDIH_DeviceExtension->IoInProgressEvent), NotificationEvent, FALSE);

   //
   // Attach Our Filter To The UDP Device Object
   //
   pLowerDeviceObject = IoAttachDeviceToDeviceStack(
                           pFilterDeviceObject, // Source Device (Our Device)
                           pTargetDeviceObject  // Target Device
                           );

   if( !pLowerDeviceObject )
   {
      KdPrint(("PCATDIH: Couldn't attach to UDP Device Object\n"));

      //
      // Delete Our UDP Filter Device Object
      //
      IoDeleteDevice( pFilterDeviceObject );

      pFilterDeviceObject = NULL;

      ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
      ObDereferenceObject( pTargetFileObject );

      pTargetFileObject = NULL;
      pTargetDeviceObject = NULL;

      return( status );
   }

   // Initialize the TargetDeviceObject field in the extension.
   pTDIH_DeviceExtension->TargetDeviceObject = pTargetDeviceObject;
   pTDIH_DeviceExtension->TargetFileObject = pTargetFileObject;

   pTDIH_DeviceExtension->LowerDeviceObject = pLowerDeviceObject;

   pTDIH_DeviceExtension->DeviceExtensionFlags |= TDIH_DEV_EXT_ATTACHED;

#ifdef DBG
   if( pLowerDeviceObject != pTargetDeviceObject )
   {
      KdPrint(("PCATDIH: UDP Already Filtered!\n"));
   }
#endif DBG

   //
   // Adopt Target Device I/O Flags
   //
   pFilterDeviceObject->Flags |= pTargetDeviceObject->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO);

   return status;
}


#ifdef DBG

/////////////////////////////////////////////////////////////////////////////
//// UDPFilter_Detach
//
// Purpose
//
// Parameters
//
// Return Value
// 
// Remarks
//

VOID
UDPFilter_Detach(
   PDEVICE_OBJECT pDeviceObject
   )
{
   PTDIH_DeviceExtension pTDIH_DeviceExtension;
   BOOLEAN		NoRequestsOutstanding = FALSE;

   pTDIH_DeviceExtension = (PTDIH_DeviceExtension )pDeviceObject->DeviceExtension;

   ASSERT( pTDIH_DeviceExtension );

   try
   {
      try
      {
         // We will wait until all IRP-based I/O requests have been completed.

         while (TRUE)
         {
				// Check if there are requests outstanding
            UTIL_IsLargeIntegerZero(
               NoRequestsOutstanding,
               pTDIH_DeviceExtension->OutstandingIoRequests,
               &(pTDIH_DeviceExtension->IoRequestsSpinLock)
               );

				if( !NoRequestsOutstanding )
            {
					// Drop the resource and go to sleep.

					// Worst case, we will allow a few new I/O requests to slip in ...
					KeWaitForSingleObject(
                  (void *)(&(pTDIH_DeviceExtension->IoInProgressEvent)),
                  Executive, KernelMode, FALSE, NULL
                  );

				}
            else
            {
					break;
				}
			}

			// Detach if attached.
			if( pTDIH_DeviceExtension->DeviceExtensionFlags & TDIH_DEV_EXT_ATTACHED)
         {
            IoDetachDevice( pTDIH_DeviceExtension->TargetDeviceObject );

            pTDIH_DeviceExtension->DeviceExtensionFlags &= ~(TDIH_DEV_EXT_ATTACHED);
			}

			// Delete our device object. But first, take care of the device extension.
	      pTDIH_DeviceExtension->NodeIdentifier.NodeType = 0;
	      pTDIH_DeviceExtension->NodeIdentifier.NodeSize = 0;

         if( pTDIH_DeviceExtension->TargetFileObject )
         {
            ObDereferenceObject( pTDIH_DeviceExtension->TargetFileObject );
         }

         pTDIH_DeviceExtension->TargetFileObject = NULL;

			// Note that on 4.0 and later systems, this will result in a recursive fast detach.
			IoDeleteDevice( pDeviceObject );

         KdPrint(("PCATDIH: UDPFilter_Detach Finished\n"));
		}
      except (EXCEPTION_EXECUTE_HANDLER)
      {
			// Eat it up.
			;
		}

		try_exit:  NOTHING;
	}
   finally
   {
      ;
	}

	return;
}

#endif // DBG


