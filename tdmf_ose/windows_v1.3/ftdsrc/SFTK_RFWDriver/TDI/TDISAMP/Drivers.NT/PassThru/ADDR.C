/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include "ndis.h"
#include "TDI.H"
#include "TDIKRNL.H"
#include "PCATDIH.h"
#include "KSUtil.h"

#include "addr.h"
#include "tcpconn.h"
#include "tcprcv.h"
#include "udprcv.h"

#include "inetinc.h"

// Copyright And Configuration Management ----------------------------------
//
//                 TDI Address Function Filters - Addr.c
//         Transport Data Interface (TDI) Filter For Windows NT
//
//     Copyright (c) 2000-2001 Printing Communications Associates, Inc.
//                               - PCAUSA -
//
//                             Thomas F. Divine
//                           4201 Brunswick Court
//                        Smyrna, Georgia 30080 USA
//                              (770) 432-4580
//                            tdivine@pcausa.com
//
// End ---------------------------------------------------------------------

//
// The Address Object Lists
//
LIST_ENTRY FreeAddrObjList;
LIST_ENTRY OpenAddrObjList;
LIST_ENTRY AddrObjExInfoList;


PAddrObjExInfo
TDIH_GetAddrObjExInfo( PIRP Irp )
{
   PAddrObjExInfo pAddrObjExInfo;

   //
   // Walk The Special Create List
   //
   pAddrObjExInfo = (PAddrObjExInfo )AddrObjExInfoList.Flink;

   while( !IsListEmpty( &AddrObjExInfoList )
      && pAddrObjExInfo != (PAddrObjExInfo )&AddrObjExInfoList
      )
   {
      //
      // Match On Address Handle
      //
		if( pAddrObjExInfo->aox_thread == Irp->Tail.Overlay.Thread )
		{
			return( pAddrObjExInfo );
		}

      //
      // Move To The Next Address Object Record
      //
      pAddrObjExInfo = (PAddrObjExInfo )pAddrObjExInfo->aox_q.Flink;
	}

   return( (PAddrObjExInfo )NULL );
}


AddrObj *
TDIH_GetAddrObjFromFileObject(
   PFILE_OBJECT   FileObject
   )
{
   AddrObj *pAddrObj;

   //
   // Walk The List Of Open Address Objects
   //
   pAddrObj = (AddrObj * )OpenAddrObjList.Flink;

   while( !IsListEmpty( &OpenAddrObjList )
      && pAddrObj != (AddrObj * )&OpenAddrObjList
      )
   {
      //
      // Match On Address Handle
      //
		if( pAddrObj->ao_FileObject == FileObject )
		{
			return( pAddrObj );
		}

      //
      // Move To The Next Address Object Record
      //
      pAddrObj = (AddrObj * )pAddrObj->ao_q.Flink;
	}

   return( (AddrObj * )NULL );
}


/////////////////////////////////////////////////////////////////////////////
//// TDIH_TdiOpenAddressComplete
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
TDIH_TdiOpenAddressComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              Irp,
   void              *Context
   )
{
   PTDIH_DeviceExtension   pTDIH_DeviceExtension;
   BOOLEAN                 CanDetachProceed = FALSE;
   PDEVICE_OBJECT          pAssociatedDeviceObject = NULL;
	NTSTATUS                Status = Irp->IoStatus.Status;

   KdPrint(( "TDIH_TdiOpenAddressComplete: Status: 0x%8.8X\n", Status ));

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
      KdPrint(( "TDIH_TdiOpenAddressComplete: Invalid Device Object Pointer\n" ));
      return(STATUS_SUCCESS);
   }

   // Note that you could do all sorts of processing at this point
   // depending upon the results of the operation. Be careful though
   // about what you chose to do, about the fact that this completion
   // routine is being invoked in an arbitrary thread context and probably
   // at high IRQL.

   //
   // Create A New Address Object Record, If Successful
   //
   if( NT_SUCCESS( Status ) )
   {
      AddrObj              *pAddrObj = NULL;
      PIO_STACK_LOCATION      IrpSp = NULL;

      // Get the current I/O stack location.
      IrpSp = IoGetCurrentIrpStackLocation(Irp);
      ASSERT(IrpSp);

      pAddrObj = TDIH_GetAddrObjFromFileObject( IrpSp->FileObject );

      if( pAddrObj )
      {
         KdPrint(("   AddrObj ALREADY EXISTS!!!\n"));
      }

      //
      // Allocate A New Address Object Record
      // ------------------------------------
      // First attempt to recycle a previously allocated AddrObj structure.
      // If none available, allocate a new one.
      //
      if( !IsListEmpty( &FreeAddrObjList ) )
      {
         pAddrObj = (AddrObj * )RemoveHeadList( &FreeAddrObjList );
      }

	   if( !pAddrObj || (PLIST_ENTRY )pAddrObj == &FreeAddrObjList )
		{
		   pAddrObj = (AddrObj * )ExAllocatePool(NonPagedPool, sizeof(AddrObj) );
      }

      //
      // Initialize The New Address Object Record
      //
		if( pAddrObj )
		{
         PAddrObjExInfo    pAddrObjExInfo;

         //
         // Initialize The New Address Object
         //
         NdisZeroMemory( pAddrObj, sizeof(AddrObj) );

         KS_GetCurrentProcessName( pAddrObj->ao_ProcessName );

         pAddrObjExInfo = TDIH_GetAddrObjExInfo( Irp );

         if( pAddrObjExInfo )
         {
            KdPrint(("   Marking Internally Initiated Address Object\n" ));

            //
            // Special Handling For Address Objects Created In The TDI Filter
            // --------------------------------------------------------------
            // See extensive comments for the W32API_TestOpenAddress function
            // in W32Api.c.
            //
            pAddrObj->aox_valid = TRUE;

            pAddrObj->aox_data1 = pAddrObjExInfo->aox_data1;
            pAddrObj->aox_data2 = pAddrObjExInfo->aox_data2;

            //
            // Return The AddrObj Pointer
            // --------------------------
            // Hopefully you will find a use for it...
            //
            pAddrObjExInfo->aox_ao = pAddrObj;
         }

         InitializeListHead( &pAddrObj->ao_tc_q );

         InitializeListHead( &pAddrObj->ao_senddg_q );
         InitializeListHead( &pAddrObj->ao_rcvdg_q );

			pAddrObj->ao_DeviceExtension = pTDIH_DeviceExtension;

         if( pTDIH_DeviceExtension->NodeIdentifier.NodeType == TDIH_NODE_TYPE_TCP_FILTER_DEVICE )
         {
            pAddrObj->ao_prot = IPPROTO_TCP;
         }
         else if( pTDIH_DeviceExtension->NodeIdentifier.NodeType == TDIH_NODE_TYPE_UDP_FILTER_DEVICE )
         {
            pAddrObj->ao_prot = IPPROTO_UDP;
         }
         else if( pTDIH_DeviceExtension->NodeIdentifier.NodeType == TDIH_NODE_TYPE_RAW_IP_FILTER_DEVICE )
         {
            // ATTENTION!!! Must Extract Protocol From EA
            pAddrObj->ao_prot = 0;
         }

			pAddrObj->ao_FileObject = IrpSp->FileObject;

         //
         // Add To The Open Address Object List
         //
         InsertTailList(
            &OpenAddrObjList,
            &pAddrObj->ao_q
            );
      }
   }

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
//// TDIH_TdiOpenAddress
//
// Purpose
// This is the hook for TdiOpenAddress
//
// Parameters
//
// Return Value
// 
// Remarks
//

NTSTATUS
TDIH_TdiOpenAddress(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   )
{
   NTSTATUS    RC;
   char        ProcessName[ NT_PROCNAMELEN ];

   if( KS_GetCurrentProcessName( ProcessName ) )
   {
      KdPrint(( "TDIH_TdiOpenAddress: Process: \042%s\042\n", ProcessName ));
   }
   else
   {
      KdPrint(( "TDIH_TdiOpenAddress: Process: UNKNOWN\n" ));
   }

   //
   // Pass The Request To A TCP/IP Device
   //
   try
   {
      PDEVICE_OBJECT       pLowerDeviceObject = NULL;

      pLowerDeviceObject = pTDIH_DeviceExtension->LowerDeviceObject;

      // Be careful about not screwing up badly. This is actually not recommended by the I/O Manager.
      if (Irp->CurrentLocation == 1)
      {
         ULONG ReturnedInformation = 0;

         // Bad!! Fudge the error code. Break if we can ...

         KdPrint(("TDIH_TdiOpenAddress encountered bogus current location\n"));

   //      UTIL_BreakPoint();
         RC = STATUS_INVALID_DEVICE_REQUEST;
         Irp->IoStatus.Status = RC;
         Irp->IoStatus.Information = ReturnedInformation;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);

         return( RC );
      }

      IoCopyCurrentIrpStackLocationToNext( Irp );

      IoSetCompletionRoutine(
         Irp,
         TDIH_TdiOpenAddressComplete,
         pTDIH_DeviceExtension,
         TRUE,
         TRUE,
         TRUE
         );

      UTIL_IncrementLargeInteger(
         pTDIH_DeviceExtension->OutstandingIoRequests,
         (unsigned long)1,
         &(pTDIH_DeviceExtension->IoRequestsSpinLock)
         );

      // Clear the fast-IO notification event protected by the resource
      // we have acquired.
      KeClearEvent(&(pTDIH_DeviceExtension->IoInProgressEvent));

      ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );
      RC = IoCallDriver(pLowerDeviceObject, Irp);

      try_return(RC);

      try_exit:   NOTHING;
   }
   finally
   {
   }

   return(RC);
}


/////////////////////////////////////////////////////////////////////////////
//// TDIH_TdiCloseAddressComplete
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
TDIH_TdiCloseAddressComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              Irp,
   void              *Context
   )
{
   PTDIH_DeviceExtension   pTDIH_DeviceExtension = NULL;
   BOOLEAN                 CanDetachProceed = FALSE;
   PDEVICE_OBJECT          pAssociatedDeviceObject = NULL;
	NTSTATUS                Status = Irp->IoStatus.Status;
   AddrObj                 *pAddrObj;
   PIO_STACK_LOCATION      IrpSp = NULL;

   KdPrint(( "TDIH_TdiCloseAddressComplete: Status: 0x%8.8X\n", Status ));

   ASSERT( Context );
   pTDIH_DeviceExtension = (PTDIH_DeviceExtension )(Context);

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

   ASSERT( pAssociatedDeviceObject == pDeviceObject );
   if( pAssociatedDeviceObject != pDeviceObject )
   {
      KdPrint(( "TDIH_TdiCloseAddressComplete: Invalid Device Object Pointer\n" ));
      return(STATUS_SUCCESS);
   }

   // Note that you could do all sorts of processing at this point
   // depending upon the results of the operation. Be careful though
   // about what you chose to do, about the fact that this completion
   // routine is being invoked in an arbitrary thread context and probably
   // at high IRQL.

   // Get the current I/O stack location.
   IrpSp = IoGetCurrentIrpStackLocation(Irp);
   ASSERT(IrpSp);

   //
   // Locate The Address Object Record
   //
   pAddrObj = TDIH_GetAddrObjFromFileObject( IrpSp->FileObject );

   if( pAddrObj )
   {
      KdPrint(("  Found AddrObject For Close\n" ));

      ASSERT( IsListEmpty(&pAddrObj->ao_senddg_q) );
      ASSERT( IsListEmpty(&pAddrObj->ao_rcvdg_q) );

      //
      // Remove From The Open Address List
      //
      RemoveEntryList( &pAddrObj->ao_q );

      //
      // Add To The Free Address Object List
      // -----------------------------------
      // Don't free AddrObj structures. Instead, queue them in
      // the Free Address Object List for re-use. This should
      // reduce memory fragmentation since AddrObj are likely
      // to be needed again.
      //
      InsertTailList(
         &FreeAddrObjList,
         &pAddrObj->ao_q
         );

      // ATTENTION!!! Close Orphan TCPConn's ???
   }
   else
   {
      KdPrint(("  AddrObject For Close NOT FOUND!!!\n" ));
   }

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
//// TDIH_TdiCloseAddress
//
// Purpose
// This is the hook for TdiCloseAddress
//
// Parameters
//
// Return Value
// 
// Remarks
//

NTSTATUS
TDIH_TdiCloseAddress(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   )
{
   NTSTATUS             RC;

   KdPrint(( "TDIH_TdiCloseAddress: Entry...\n" ));

   //
   // Pass The Request To A TCP/IP Device
   //
   try
   {
      PDEVICE_OBJECT       pLowerDeviceObject = NULL;

      pLowerDeviceObject = pTDIH_DeviceExtension->LowerDeviceObject;

      // Be careful about not screwing up badly. This is actually not recommended by the I/O Manager.
      if (Irp->CurrentLocation == 1)
      {
         ULONG ReturnedInformation = 0;

         // Bad!! Fudge the error code. Break if we can ...

         KdPrint(("TDIH_TdiCloseAddress encountered bogus current location\n"));

   //      UTIL_BreakPoint();
         RC = STATUS_INVALID_DEVICE_REQUEST;
         Irp->IoStatus.Status = RC;
         Irp->IoStatus.Information = ReturnedInformation;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);

         return( RC );
      }

      IoCopyCurrentIrpStackLocationToNext( Irp );

      IoSetCompletionRoutine(
         Irp,
         TDIH_TdiCloseAddressComplete,
         pTDIH_DeviceExtension,
         TRUE,
         TRUE,
         TRUE
         );

      UTIL_IncrementLargeInteger(
         pTDIH_DeviceExtension->OutstandingIoRequests,
         (unsigned long)1,
         &(pTDIH_DeviceExtension->IoRequestsSpinLock)
         );

      // Clear the fast-IO notification event protected by the resource
      // we have acquired.
      KeClearEvent(&(pTDIH_DeviceExtension->IoInProgressEvent));

      ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );
      RC = IoCallDriver(pLowerDeviceObject, Irp);

      try_return(RC);

      try_exit:   NOTHING;
   }
   finally
   {
   }

   return(RC);
}


/////////////////////////////////////////////////////////////////////////////
//// TDIH_TdiSetEventComplete
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
TDIH_TdiSetEventComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              Irp,
   void              *Context
   )
{
   PTDIH_DeviceExtension   pTDIH_DeviceExtension;
   BOOLEAN                 CanDetachProceed = FALSE;
   PDEVICE_OBJECT          pAssociatedDeviceObject = NULL;
	NTSTATUS                Status = Irp->IoStatus.Status;

   KdPrint(( "TDIH_TdiSetEventComplete: Status: 0x%8.8X\n", Status ));

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
      KdPrint(( "TDIH_TdiSetEventComplete: Invalid Device Object Pointer\n" ));
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
//// TDIH_TdiSetEvent
//
// Purpose
// This is the hook for TdiSetEvent
//
// Parameters
//
// Return Value
// 
// Remarks
//

NTSTATUS
TDIH_TdiSetEvent(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   )
{
   NTSTATUS                      RC;
   AddrObj                       *pAddrObj;
   PTDI_REQUEST_KERNEL_SET_EVENT pSrc, pDest;
   PDEVICE_OBJECT                pLowerDeviceObject = NULL;
   PIO_STACK_LOCATION            irpNextSp = NULL;

   pLowerDeviceObject = pTDIH_DeviceExtension->LowerDeviceObject;

   // Be careful about not screwing up badly. This is actually not recommended by the I/O Manager.
   if (Irp->CurrentLocation == 1)
   {
      ULONG ReturnedInformation = 0;

      // Bad!! Fudge the error code. Break if we can ...

      KdPrint(("TDIH_TdiSetEvent encountered bogus current location\n"));

//      UTIL_BreakPoint();
      RC = STATUS_INVALID_DEVICE_REQUEST;
      Irp->IoStatus.Status = RC;
      Irp->IoStatus.Information = ReturnedInformation;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return( RC );
   }

   IoCopyCurrentIrpStackLocationToNext( Irp );

   pSrc = (PTDI_REQUEST_KERNEL_SET_EVENT)&IrpSp->Parameters;

   irpNextSp = IoGetNextIrpStackLocation(Irp);
   pDest = (PTDI_REQUEST_KERNEL_SET_EVENT)&irpNextSp->Parameters;

   IoSetCompletionRoutine(
      Irp,
      TDIH_TdiSetEventComplete,
      pTDIH_DeviceExtension,
      TRUE,
      TRUE,
      TRUE
      );

   //
   // Locate Our Address Object
   //
   pAddrObj = TDIH_GetAddrObjFromFileObject( IrpSp->FileObject );

   if( pAddrObj )
   {
      switch( pSrc->EventType )
      {
         case TDI_EVENT_CONNECT:
            KdPrint(( "TDIH_TdiSetEvent: Hooking connect event\n" ));

            if( pSrc->EventHandler )
            {
               //
               // Hook The Caller's Event Handler
               // -------------------------------
               // Save the caller's handler and context and replace it
               // with our own.
               //
               pAddrObj->ao_connect = pSrc->EventHandler;
               pAddrObj->ao_conncontext = pSrc->EventContext;

               pDest->EventHandler = (PVOID )TDIH_TdiConnectEventHandler;
               pDest->EventContext = (PVOID )pAddrObj;	// context is our Address Object Record
            }
            else
            {
               //
               // Caller Has No Event Handler
               // ---------------------------
               // This could be used to "unset" and event handler. In most
               // cases we don't want to be called either...
               //
               pAddrObj->ao_connect = NULL;
               pAddrObj->ao_conncontext = NULL;

               pDest->EventHandler = NULL;
               pDest->EventContext = NULL;
            }
            break;

         case TDI_EVENT_RECEIVE:
            KdPrint(( "TDIH_TdiSetEvent: Hooking receive event\n" ));

            if( pSrc->EventHandler )
            {
               //
               // Hook The Caller's Event Handler
               // -------------------------------
               // Save the caller's handler and context and replace it
               // with our own.
               //
               pAddrObj->ao_rcv = pSrc->EventHandler;
               pAddrObj->ao_rcvcontext = pSrc->EventContext;

               pDest->EventHandler = (PVOID )TDIH_TdiReceiveEventHandler;
               pDest->EventContext = (PVOID )pAddrObj;	// context is our Address Object Record
            }
            else
            {
               //
               // Caller Has No Event Handler
               // ---------------------------
               // This could be used to "unset" and event handler. In most
               // cases we don't want to be called either...
               //
               pAddrObj->ao_rcv = NULL;
               pAddrObj->ao_rcvcontext = NULL;

               pDest->EventHandler = NULL;
               pDest->EventContext = NULL;
            }
            break;

         case TDI_EVENT_CHAINED_RECEIVE:
            KdPrint(( "TDIH_TdiSetEvent: Hooking chained receive event\n" ));

            if( pSrc->EventHandler )
            {
               //
               // Hook The Caller's Event Handler
               // -------------------------------
               // Save the caller's handler and context and replace it
               // with our own.
               //
               pAddrObj->ao_chainedrcv = pSrc->EventHandler;
               pAddrObj->ao_chainedrcvcontext = pSrc->EventContext;

               pDest->EventHandler = (PVOID )TDIH_TdiChainedReceiveEventHandler;
               pDest->EventContext = (PVOID )pAddrObj;	// context is our Address Object Record
            }
            else
            {
               //
               // Caller Has No Event Handler
               // ---------------------------
               // This could be used to "unset" and event handler. In most
               // cases we don't want to be called either...
               //
               pAddrObj->ao_chainedrcv = NULL;
               pAddrObj->ao_chainedrcvcontext = NULL;

               pDest->EventHandler = NULL;
               pDest->EventContext = NULL;
            }
            break;

         case TDI_EVENT_RECEIVE_DATAGRAM:
            KdPrint(( "TDIH_TdiSetEvent: Hooking receive datagram event\n" ));

            if( pSrc->EventHandler )
            {
               //
               // Hook The Caller's Event Handler
               // -------------------------------
               // Save the caller's handler and context and replace it
               // with our own.
               //
               pAddrObj->ao_rcvdg = pSrc->EventHandler;
               pAddrObj->ao_rcvdgcontext = pSrc->EventContext;

               pDest->EventHandler = (PVOID )TDIH_TdiReceiveDGEventHandler;
               pDest->EventContext = (PVOID )pAddrObj;	// context is our Address Object Record
            }
            else
            {
               //
               // Caller Has No Event Handler
               // ---------------------------
               // This could be used to "unset" and event handler. In most
               // cases we don't want to be called either...
               //
               pAddrObj->ao_rcvdg = NULL;
               pAddrObj->ao_rcvdgcontext = NULL;

               pDest->EventHandler = NULL;
               pDest->EventContext = NULL;
            }
            break;

         case TDI_EVENT_CHAINED_RECEIVE_DATAGRAM:
            KdPrint(( "TDIH_TdiSetEvent: Hooking chained receive datagram event\n" ));

            if( pSrc->EventHandler )
            {
               //
               // Hook The Caller's Event Handler
               // -------------------------------
               // Save the caller's handler and context and replace it
               // with our own.
               //
               pAddrObj->ao_chainedrcvdg = pSrc->EventHandler;
               pAddrObj->ao_chainedrcvdgcontext = pSrc->EventContext;

               pDest->EventHandler = (PVOID )TDIH_TdiChainedReceiveDGEventHandler;
               pDest->EventContext = (PVOID )pAddrObj;	// context is our Address Object Record
            }
            else
            {
               //
               // Caller Has No Event Handler
               // ---------------------------
               // This could be used to "unset" and event handler. In most
               // cases we don't want to be called either...
               //
               pAddrObj->ao_chainedrcvdg = NULL;
               pAddrObj->ao_chainedrcvdgcontext = NULL;

               pDest->EventHandler = NULL;
               pDest->EventContext = NULL;
            }
            break;

         case TDI_EVENT_DISCONNECT:
         case TDI_EVENT_ERROR:
         case TDI_EVENT_RECEIVE_EXPEDITED:
         case TDI_EVENT_SEND_POSSIBLE:
         default:
            KdPrint(( "TDIH_TdiSetEvent: Hooking %d event\n", pSrc->EventType ));
            break;
      }
   }
   else
   {
      KdPrint(("  AddrObject For Event NOT FOUND!!!\n" ));
   }

   //
   // Pass The Request Down To The Lower Device
   //
   UTIL_IncrementLargeInteger(
      pTDIH_DeviceExtension->OutstandingIoRequests,
      (unsigned long)1,
      &(pTDIH_DeviceExtension->IoRequestsSpinLock)
      );

   // Clear the fast-IO notification event protected by the resource
   // we have acquired.
   KeClearEvent(&(pTDIH_DeviceExtension->IoInProgressEvent));

   ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );
   RC = IoCallDriver(pLowerDeviceObject, Irp);

   return(RC);
}
