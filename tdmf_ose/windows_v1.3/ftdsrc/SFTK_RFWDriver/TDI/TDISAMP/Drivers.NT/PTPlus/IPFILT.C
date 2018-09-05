/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include "ndis.h"
#include "TDI.H"
#include "TDIKRNL.H"
#include "PCATDIH.h"

// Copyright And Configuration Management ----------------------------------
//
//                TDI Filter Driver IP Device Filter - IPFILT.c
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
//                       I P  D E V I C E  F I L T E R                     //
/////////////////////////////////////////////////////////////////////////////

//
// Notes
// -----
// The IP device is a Microsoft device that provides support for IP
// operations other than TCP and UDP. Since the details of it's operation
// are undocumented, this is an empty skeleton provided strictly for
// experimental purposes.
//
// If you exercise the IP filter you will see calls related to IP functions
// such as ICMP and IGMP. For example, if you exercise the PING.EXE utility,
// you will see that the IP device is called to create a handle and then
// DeviceIoControl calls are made to actually perform the PING.
//
// Since this IP filter is not of much practical use, it is NOT called
// unless the USE_IP_FILTER preprocessor variable is set in PCATDIH.H
//

#ifdef USE_IP_FILTER

/////////////////////////////////////////////////////////////////////////////
//// IPFilter_InitDeviceExtension
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
IPFilter_InitDeviceExtension(
   PTDIH_DeviceExtension pTDIH_DeviceExtension,
   PDEVICE_OBJECT pFilterDeviceObject
   )
{
   NdisZeroMemory( pTDIH_DeviceExtension, sizeof( TDIH_DeviceExtension ) );

   pTDIH_DeviceExtension->NodeIdentifier.NodeType = TDIH_NODE_TYPE_IP_FILTER_DEVICE;
   pTDIH_DeviceExtension->NodeIdentifier.NodeSize = sizeof( TDIH_DeviceExtension );

   pTDIH_DeviceExtension->pFilterDeviceObject = pFilterDeviceObject;

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// IPFilter_Attach
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
IPFilter_Attach(
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

   KdPrint(("PCATDIH: IPFilter_Attach Entry...\n"));

   //
   // Create counted string version of target IP device name.
   //
   ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );
   RtlInitUnicodeString( &uniNtNameString, DD_IP_DEVICE_NAME );

   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
   status = IoGetDeviceObjectPointer(
               &uniNtNameString,
               FILE_READ_ATTRIBUTES,
               &pTargetFileObject,   // Call ObDereferenceObject Eventually...
               &pTargetDeviceObject
               );

   if( !NT_SUCCESS(status) )
   {
      KdPrint(("PCATDIH: Couldn't get the IP Device Object\n"));

      pTargetFileObject = NULL;
      pTargetDeviceObject = NULL;

      return( status );
   }

   //
   // Create counted string version of our IP filter device name.
   //
   ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );
   RtlInitUnicodeString( &uniNtNameString, TDIH_IP_DEVICE_NAME );

   //
   // Create The IP Filter Device Object
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
      KdPrint(("PCATDIH: Couldn't create the IP Filter Device Object\n"));

      ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
      ObDereferenceObject( pTargetFileObject );

      pTargetFileObject = NULL;
      pTargetDeviceObject = NULL;

      return( status );
   }

   //
   // Initialize The Extension For The IP Filter Device Object
   //
   pTDIH_DeviceExtension = (PTDIH_DeviceExtension )( pFilterDeviceObject->DeviceExtension );

   IPFilter_InitDeviceExtension(
      pTDIH_DeviceExtension,
      pFilterDeviceObject
      );

   // Initialize the Executive spin lock for this device extension.
   KeInitializeSpinLock(&(pTDIH_DeviceExtension->IoRequestsSpinLock));

   // Initialize the event object used to denote I/O in progress.
   // When set, the event signals that no I/O is currently in progress.
   KeInitializeEvent(&(pTDIH_DeviceExtension->IoInProgressEvent), NotificationEvent, FALSE);

   //
   // Attach Our Filter To The IP Device Object
   //
   pLowerDeviceObject = IoAttachDeviceToDeviceStack(
                           pFilterDeviceObject, // Source Device (Our Device)
                           pTargetDeviceObject  // Target Device
                           );

   if( !pLowerDeviceObject )
   {
      KdPrint(("PCATDIH: Couldn't attach to IP Device Object\n"));

      //
      // Delete Our IP Filter Device Object
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

#if DBG
   if( pLowerDeviceObject != pTargetDeviceObject )
   {
      KdPrint(("PCATDIH: IP Already Filtered!\n"));
   }
#endif DBG

   //
   // Adopt Target Device I/O Flags
   //
   pFilterDeviceObject->Flags |= pTargetDeviceObject->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO);

   return status;
}


#if DBG

/////////////////////////////////////////////////////////////////////////////
//// IPFilter_Detach
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
IPFilter_Detach(
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

         KdPrint(("PCATDIH: IPFilter_Detach Finished\n"));
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


/////////////////////////////////////////////////////////////////////////////
//// IPFilter_DefaultCompletion
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
IPFilter_DefaultCompletion(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              Irp,
   void              *Context
   )
{
   PTDIH_DeviceExtension pTDIH_DeviceExtension;
   BOOLEAN           CanDetachProceed = FALSE;
   PDEVICE_OBJECT    pAssociatedDeviceObject = NULL;

   pTDIH_DeviceExtension = (PTDIH_DeviceExtension )(Context);

   ASSERT( pTDIH_DeviceExtension );

   //
   // Propogate The IRP Pending Flag
   //
   if (Irp->PendingReturned)
   {
      IoMarkIrpPending(Irp);
   }

   // Ensure that this is a valid device object pointer, else return
   // immediately.
   pAssociatedDeviceObject = pTDIH_DeviceExtension->pFilterDeviceObject;

   if (pAssociatedDeviceObject != pDeviceObject)
   {
      KdPrint(( "IPFilter_DefaultCompletion: Invalid Device Object Pointer\n" ));
      return(STATUS_SUCCESS);
   }

   // Note that you could do all sorts of processing at this point
   // depending upon the results of the operation. Be careful though
   // about what you chose to do, about the fact that this completion
   // routine is being invoked in an arbitrary thread context and probably
   // at high IRQL.

   UTIL_DecrementLargeInteger(
      pTDIH_DeviceExtension->OutstandingIoRequests,
      (unsigned long)1,
      &(pTDIH_DeviceExtension->IoRequestsSpinLock)
      );

   // If the outstanding count is 0, signal the appropriate event which will
   // allow any pending detach to proceed.
   UTIL_IsLargeIntegerZero(
      CanDetachProceed,
      pTDIH_DeviceExtension->OutstandingIoRequests,
      &(pTDIH_DeviceExtension->IoRequestsSpinLock)
      );

   if (CanDetachProceed)
   {
      // signal the event object. Note that this is simply an
      // advisory check we do here (to wake up a sleeping thread).
      // It is the responsibility of the thread performing the detach to
      // ensure that no operations are truly in progress.
      KeSetEvent(&(pTDIH_DeviceExtension->IoInProgressEvent), IO_NO_INCREMENT, FALSE);
   }

   // Although the success return value is hard-coded here, you can
   // return an appropriate value (either success or more-processing-reqd)
   // based upon what it is that you wish to do in your completion routine.
   return(STATUS_SUCCESS);
}


/////////////////////////////////////////////////////////////////////////////
//// IPFilter_Detach
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
IPFilter_Dispatch(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )
{
   NTSTATUS                RC = STATUS_SUCCESS;
   PIO_STACK_LOCATION      pCurrentStackLocation = NULL;
   PTDIH_DeviceExtension   pTDIH_DeviceExtension;
   BOOLEAN                 CompleteIrp = FALSE;
   PDEVICE_OBJECT          pLowerDeviceObject = NULL;
   ULONG                   ReturnedInformation = 0;

   // Get a pointer to the device extension that must exist for
   // all of the device objects created by the filter driver.
   pTDIH_DeviceExtension = (PTDIH_DeviceExtension )(DeviceObject->DeviceExtension);

   try
   {
      pLowerDeviceObject = pTDIH_DeviceExtension->LowerDeviceObject;

      // Get the current I/O stack location.
      pCurrentStackLocation = IoGetCurrentIrpStackLocation(Irp);
      ASSERT(pCurrentStackLocation);

      KdPrint(("IPFilter_Dispatch: IP Device, Major FCN: 0x%2.2X, Minor FCN: 0x%2.2X\n",
                  pCurrentStackLocation->MajorFunction,
                  pCurrentStackLocation->MinorFunction
                  ));

      // Be careful about not screwing up badly. This is actually not recommended by the I/O Manager.
      if (Irp->CurrentLocation == 1) {
         // Bad!! Fudge the error code. Break if we can ...

         KdPrint(("PCATDIH: IPFilter_Dispatch encountered bogus current location\n"));

//         UTIL_BreakPoint();
         CompleteIrp = TRUE;
         try_return(RC = STATUS_INVALID_DEVICE_REQUEST);
      }

      //
      // Copy Contents Of Current Stack Location To Next Stack Location
      //
      IoCopyCurrentIrpStackLocationToNext( Irp );

      //
      // Setup Our Completion Routine
      // ============================
      // We will specify a default completion routine. This provides us
      // with the opportunity to do whatever we like once the function
      // processing has been completed.
      //
      // Specify that our completion routine be invoked regardless of how
      // the IRP is completed/cancelled.
      //
      IoSetCompletionRoutine(
         Irp,
         IPFilter_DefaultCompletion,
         pTDIH_DeviceExtension,
         TRUE,
         TRUE,
         TRUE
         );

      // Increment the count of outstanding I/O requests. The count will
      // be decremented in the completion routine.
      // Acquire a special end-resource spin-lock to synchronize access.
      UTIL_IncrementLargeInteger(
         pTDIH_DeviceExtension->OutstandingIoRequests,
         (unsigned long)1,
         &(pTDIH_DeviceExtension->IoRequestsSpinLock)
         );

      // Clear the fast-IO notification event protected by the resource
      // we have acquired.
      KeClearEvent(&(pTDIH_DeviceExtension->IoInProgressEvent));

      // Forward the request. Note that if the target does not
      // wish to service the function, the request will get redirected
      // to IopInvalidDeviceRequest() (a routine that completes the
      // IRP with STATUS_INVALID_DEVICE_REQUEST).
      // However, we must release our resources before forwarding the
      // request. That will avoid the sort of problems discussed in
      // Chapter 12 of the text.

      RC = IoCallDriver(pLowerDeviceObject, Irp);

      // Note that at this time, the filter driver completion routine
      // does not return STATUS_MORE_PROCESSING_REQUIRED. However, if you
      // do modify this code and use it in your own filter driver and if your
      // completion routine *could* return the STATUS_MORE_PROCESSING_REQUIRED
      // return code, you must not blindly return the return-code obtained from
      // the call to IoCallDriver() above. See Chapter 12 for a discussion of
      // this issue.
      try_return(RC);

      // The filter driver does not particularly care to respond to these requests.
      CompleteIrp = TRUE;
      try_return(RC = STATUS_INVALID_DEVICE_REQUEST);

      try_exit:   NOTHING;

   }
   finally
   {
      // Complete the IRP only if we must.
      if (CompleteIrp)
      {
         Irp->IoStatus.Status = RC;
         Irp->IoStatus.Information = ReturnedInformation;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
      }
   }

   return(RC);
}

#endif // USE_IP_FILTER


