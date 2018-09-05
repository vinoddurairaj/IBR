/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include "ndis.h"
#include "TDI.H"
#include "TDIKRNL.H"
#include "PCATDIH.h"

#include "inetinc.h"
#include "KSUtil.h"

#include "addr.h"
#include "tcpconn.h"
#include "tcpsend.h"
#include "tcprcv.h"
#include "udpsend.h"
#include "udprcv.h"

// Copyright And Configuration Management ----------------------------------
//
//                 TDI Filter Driver Entry Module - PCATDIH.c
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

NDIS_HANDLE g_hTDIH_BufferPool = NULL;


/////////////////////////////////////////////////////////////////////////////
//                        Forward Procedure Prototypes                     //
/////////////////////////////////////////////////////////////////////////////

FILE_FULL_EA_INFORMATION UNALIGNED *
FindEA(
    PFILE_FULL_EA_INFORMATION  StartEA,
    CHAR                      *TargetName,
    USHORT                     TargetNameLength
    );

#if DBG

VOID
TDIH_Unload(
   PDRIVER_OBJECT DriverObject
   );

#endif // DBG

/////////////////////////////////////////////////////////////////////////////
//               D E F A U L T  F U N C T I O N  H A N D L I N G           //
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//// TDIH_InternalDeviceControlCompletion
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
TDIH_InternalDeviceControlCompletion(
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
      KdPrint(( "TDIH_InternalDeviceControlCompletion: Invalid Device Object Pointer\n" ));
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
//// TDIH_DispatchInternalDeviceControl
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
TDIH_DispatchInternalDeviceControl(
   PDEVICE_OBJECT pDeviceObject,
   PIRP           Irp
   )
{
   NTSTATUS                RC = STATUS_SUCCESS;
   PIO_STACK_LOCATION      IrpSp = NULL;
   PTDIH_DeviceExtension   pTDIH_DeviceExtension;
   PDEVICE_OBJECT          pLowerDeviceObject = NULL;
   BOOLEAN                 CompleteIrp = FALSE;
   ULONG                   ReturnedInformation = 0;

   // Get a pointer to the device extension that must exist for
   // all of the device objects created by the filter driver.
   pTDIH_DeviceExtension = (PTDIH_DeviceExtension )(pDeviceObject->DeviceExtension);

   // Get the current I/O stack location.
   IrpSp = IoGetCurrentIrpStackLocation(Irp);
   ASSERT(IrpSp);

   //
   // Possibly Call Win32 API Device Dispatcher
   //
   if( pTDIH_DeviceExtension->NodeIdentifier.NodeType == TDIH_NODE_TYPE_W32API_DEVICE )
   {
      return( W32API_Dispatch( pDeviceObject, Irp ) );
   }

#ifdef USE_IP_FILTER
   //
   // Possibly Call IP Device Filter Dispatcher
   //
   if( pTDIH_DeviceExtension->NodeIdentifier.NodeType == TDIH_NODE_TYPE_IP_FILTER_DEVICE )
   {
      return( IPFilter_Dispatch( pDeviceObject, Irp ) );
   }
#endif // USE_IP_FILTER

#ifdef USE_AFD_FILTER
   //
   // Possibly Call AFD Device Filter Dispatcher
   //
   if( pTDIH_DeviceExtension->NodeIdentifier.NodeType == TDIH_NODE_TYPE_AFD_FILTER_DEVICE )
   {
      return( AfdFilter_Dispatch( pDeviceObject, Irp ) );
   }
#endif // USE_AFD_FILTER

   if (((int)IrpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE)
   {
      switch(IrpSp->MinorFunction)
      {
         case TDI_SEND:
            return( TDIH_TdiSend( pTDIH_DeviceExtension, Irp, IrpSp ) );

         case TDI_RECEIVE:
            return( TDIH_TdiReceive( pTDIH_DeviceExtension, Irp, IrpSp ) );

         case TDI_ASSOCIATE_ADDRESS:
            return( TDIH_TdiAssociateAddress( pTDIH_DeviceExtension, Irp, IrpSp ) );

         case TDI_DISASSOCIATE_ADDRESS:
            return( TDIH_TdiDisAssociateAddress( pTDIH_DeviceExtension, Irp, IrpSp ) );

         case TDI_CONNECT:
            return( TDIH_TdiConnect( pTDIH_DeviceExtension, Irp, IrpSp ) );

         case TDI_DISCONNECT:
            return( TDIH_TdiDisconnect( pTDIH_DeviceExtension, Irp, IrpSp ) );

         case TDI_LISTEN:
            return( TDIH_TdiListen( pTDIH_DeviceExtension, Irp, IrpSp ) );
            break;

         case TDI_ACCEPT:
            KdPrint(("TdiAccept: Entry\n" ));
            break;

         default:
            KdPrint(("TdiAction: ConnectionFile MinorFunction: 0x%8.8X\n",
               IrpSp->MinorFunction
               ));
            break;
      }
   }
   else if (((int)IrpSp->FileObject->FsContext2) == TDI_TRANSPORT_ADDRESS_FILE)
   {
      switch(IrpSp->MinorFunction)
      {
         case TDI_SEND_DATAGRAM:
            return( TDIH_TdiSendDatagram( pTDIH_DeviceExtension, Irp, IrpSp ) );

         case TDI_RECEIVE_DATAGRAM:
            return( TDIH_TdiReceiveDatagram( pTDIH_DeviceExtension, Irp, IrpSp ) );

         case TDI_SET_EVENT_HANDLER:
            return( TDIH_TdiSetEvent( pTDIH_DeviceExtension, Irp, IrpSp ) );

         default:
            KdPrint(("TdiAction: AddressFile MinorFunction: 0x%8.8X\n",
               IrpSp->MinorFunction
               ));
            break;
      }
   }

   //
   // Handle Functions Common To All TDI Objects
   //
   switch(IrpSp->MinorFunction)
   {
      case TDI_QUERY_INFORMATION:
            {
               PTDI_REQUEST_KERNEL_QUERY_INFORMATION p;
               p = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION)&IrpSp->Parameters;

               switch( p->QueryType )
               {
                  case TDI_QUERY_BROADCAST_ADDRESS:
                     KdPrint(("TdiQueryInformation: BROADCAST_ADDRESS\n" ));
                     break;

                  case TDI_QUERY_PROVIDER_INFORMATION:
                     KdPrint(("TdiQueryInformation: PROVIDER_INFORMATION\n" ));
                     break;

                  case TDI_QUERY_ADDRESS_INFO:
                     KdPrint(("TdiQueryInformation: ADDRESS_INFO\n" ));
                     break;

                  case TDI_QUERY_CONNECTION_INFO:
                     KdPrint(("TdiQueryInformation: CONNECTION_INFO\n" ));
                     break;

                  case TDI_QUERY_PROVIDER_STATISTICS:
                     KdPrint(("TdiQueryInformation: PROVIDER_STATISTICS\n" ));
                     break;

                  case TDI_QUERY_DATAGRAM_INFO:
                     KdPrint(("TdiQueryInformation: DATAGRAM_INFO\n" ));
                     break;

                  case TDI_QUERY_DATA_LINK_ADDRESS:
                     KdPrint(("TdiQueryInformation: DATA_LINK_ADDRESS\n" ));
                     break;

                  case TDI_QUERY_NETWORK_ADDRESS:
                     KdPrint(("TdiQueryInformation: NETWORK_ADDRESS\n" ));
                     break;

                  case TDI_QUERY_MAX_DATAGRAM_INFO:
                     KdPrint(("TdiQueryInformation: MAX_DATAGRAM_INFO\n" ));
                     break;

                  default:
                     KdPrint(("TdiQueryInformation: QueryType: %d\n",
                        p->QueryType ));
                     break;
               }
            }
            break;

      case TDI_SET_INFORMATION:
            KdPrint(("TdiSetInformation: Entry\n" ));
            break;

      case TDI_ACTION:
            KdPrint(("TdiAction: Entry\n" ));
            break;

      default:
         KdPrint(("TdiAction: MinorFunction: 0x%8.8X\n",
            IrpSp->MinorFunction
            ));
         break;
   }

   try
   {
      pLowerDeviceObject = pTDIH_DeviceExtension->LowerDeviceObject;

      // Be careful about not screwing up badly. This is actually not recommended by the I/O Manager.
      if (Irp->CurrentLocation == 1)
      {
         ULONG ReturnedInformation = 0;

         // Bad!! Fudge the error code. Break if we can ...

         KdPrint(("TDIH_DispatchInternalDeviceControl encountered bogus current location\n"));

         KdPrint(("   TDIH_DispatchInternalDeviceControl: Node Type: %d, Context2: %d; Major FCN: 0x%2.2X, Minor FCN: 0x%2.2X\n",
                     pTDIH_DeviceExtension->NodeIdentifier.NodeType,
                     (int)IrpSp->FileObject->FsContext2,
                     IrpSp->MajorFunction,
                     IrpSp->MinorFunction
                     ));

   //      UTIL_BreakPoint();
         RC = STATUS_INVALID_DEVICE_REQUEST;
         Irp->IoStatus.Status = RC;
         Irp->IoStatus.Information = ReturnedInformation;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);

         return( RC );
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
         TDIH_InternalDeviceControlCompletion,
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


/////////////////////////////////////////////////////////////////////////////
//               D E F A U L T  F U N C T I O N  H A N D L I N G           //
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//// TDIH_DefaultCompletion
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
TDIH_DefaultCompletion(
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
      KdPrint(( "TDIH_DefaultCompletion: Invalid Device Object Pointer\n" ));
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
//// TDIH_Create
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
TDIH_Create(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   )
{
   NTSTATUS                            RC;
   FILE_FULL_EA_INFORMATION            *ea;
   FILE_FULL_EA_INFORMATION UNALIGNED  *targetEA;

   ea = (PFILE_FULL_EA_INFORMATION) Irp->AssociatedIrp.SystemBuffer;

   //
   // Handle Control Channel Open
   //
   if (!ea)
   {
      KdPrint(( "TDIH_TdiOpenControlChannel: Entry...\n" ));
   }

   //
   // Handle Address Object Open
   //
   targetEA = FindEA(ea, TdiTransportAddress, TDI_TRANSPORT_ADDRESS_LENGTH);

   if (targetEA != NULL)
   {
      return( TDIH_TdiOpenAddress( pTDIH_DeviceExtension, Irp, IrpSp ) );
   }

   //
   // Handle Connection Object Open
   //
   targetEA = FindEA(ea, TdiConnectionContext, TDI_CONNECTION_CONTEXT_LENGTH);

   if (targetEA != NULL)
   {
      return( TDIH_TdiOpenConnection(
               pTDIH_DeviceExtension,
               Irp,
               IrpSp,
               *((CONNECTION_CONTEXT UNALIGNED *)
                  &(targetEA->EaName[targetEA->EaNameLength + 1]))
               )
            );
   }

//   KdPrint(( "TDIH_Create: Creating UNKNOWN TDI Object\n" ));

   try
   {
      PDEVICE_OBJECT       pLowerDeviceObject = NULL;

      pLowerDeviceObject = pTDIH_DeviceExtension->LowerDeviceObject;

      IoCopyCurrentIrpStackLocationToNext( Irp );

      IoSetCompletionRoutine(
         Irp,
         TDIH_DefaultCompletion,
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
//// TDIH_DispatchDeviceControl
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
TDIH_DispatchDeviceControl(
   PDEVICE_OBJECT pDeviceObject,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   )
{
   NTSTATUS               RC;
   PTDIH_DeviceExtension   pTDIH_DeviceExtension;

   // Get a pointer to the device extension that must exist for
   // all of the device objects created by the filter driver.
   pTDIH_DeviceExtension = (PTDIH_DeviceExtension )(pDeviceObject->DeviceExtension);

   switch(IrpSp->Parameters.DeviceIoControl.IoControlCode)
   {
      case IOCTL_TCP_QUERY_INFORMATION_EX:
         KdPrint(( "TCPQueryInformationEx: Entry...\n" ));
//      return(TCPQueryInformationEx(Irp, IrpSp));
         break;

      case IOCTL_TCP_SET_INFORMATION_EX:
         KdPrint(( "TCPSetInformationEx: Entry...\n" ));
//      return(TCPSetInformationEx(Irp, IrpSp));
         break;

      default:
         KdPrint(( "TDIH_DispatchDeviceControl: Code: %d\n",
            IrpSp->Parameters.DeviceIoControl.IoControlCode ));
         break;
   }

   try
   {
      PDEVICE_OBJECT       pLowerDeviceObject = NULL;

      pLowerDeviceObject = pTDIH_DeviceExtension->LowerDeviceObject;

      // Be careful about not screwing up badly. This is actually not recommended by the I/O Manager.
      if (Irp->CurrentLocation == 1)
      {
         ULONG ReturnedInformation = 0;

         // Bad!! Fudge the error code. Break if we can ...

         KdPrint(("TDIH_DispatchDeviceControl encountered bogus current location\n"));

         KdPrint(("   TDIH_DispatchD: Node Type: %d, Context2: %d; Major FCN: 0x%2.2X, Minor FCN: 0x%2.2X\n",
                     pTDIH_DeviceExtension->NodeIdentifier.NodeType,
                     (int)IrpSp->FileObject->FsContext2,
                     IrpSp->MajorFunction,
                     IrpSp->MinorFunction
                     ));

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
         TDIH_DefaultCompletion,
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
//// TDIH_Cleanup
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
TDIH_Cleanup(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   )
{
   NTSTATUS                            RC;

   switch ((int) IrpSp->FileObject->FsContext2)
   {
      case TDI_TRANSPORT_ADDRESS_FILE:
         return( TDIH_TdiCloseAddress( pTDIH_DeviceExtension, Irp, IrpSp ) );

      case TDI_CONNECTION_FILE:
         return( TDIH_TdiCloseConnection( pTDIH_DeviceExtension, Irp, IrpSp ) );

      case TDI_CONTROL_CHANNEL_FILE:
      default:
         //
         // This Should Never Happen
         //
         break;
   }

   try
   {
      PDEVICE_OBJECT       pLowerDeviceObject = NULL;

      pLowerDeviceObject = pTDIH_DeviceExtension->LowerDeviceObject;

      IoCopyCurrentIrpStackLocationToNext( Irp );

      IoSetCompletionRoutine(
         Irp,
         TDIH_DefaultCompletion,
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
//// TDIH_Close
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
TDIH_Close(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   )
{
   NTSTATUS                            RC;

   switch ((int) IrpSp->FileObject->FsContext2)
   {
      case TDI_TRANSPORT_ADDRESS_FILE:
         KdPrint(( "TDIH_Close: Address File\n" ));
         {
            AddrObj  *pAddrObj;

            pAddrObj = TDIH_GetAddrObjFromFileObject( IrpSp->FileObject );

            if( pAddrObj )
            {
               KdPrint(("   AddrObj Still Open!!!\n"));
            }
         }
         break;

      case TDI_CONNECTION_FILE:
         KdPrint(( "TDIH_Close: Connection File\n" ));
         break;

      case TDI_CONTROL_CHANNEL_FILE:
         KdPrint(( "TDIH_Close: Control Channel File\n" ));
         break;

      default:
         //
         // This Should Never Happen
         //
         break;
   }

   try
   {
      PDEVICE_OBJECT       pLowerDeviceObject = NULL;

      pLowerDeviceObject = pTDIH_DeviceExtension->LowerDeviceObject;

      IoCopyCurrentIrpStackLocationToNext( Irp );

      IoSetCompletionRoutine(
         Irp,
         TDIH_DefaultCompletion,
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
//// TDIH_DefaultDispatch
//
// Purpose
//
// Parameters
//
// Return Value
// 
// Remarks
// This is the generic dispatch routine for IP/TCP/UDP/RawIP and W32API
// devices.
//
// The PCATDIH driver supports five(5) different devices, each with their
// own interpretaion of how the driver MajorFunction table. To accomodate
// this, all functions in the one-and-only driver MajorFunction table are
// initialized to point to a single function: TDIH_DefaultDispatch.
//
// TDIH_DefaultDispatch uses various means to determine which device is
// associated with each call. The primary means is to examine the
// NodeIdentifier.NodeType field which we have defined in ALL device
// extensions that are created by PCATDIH. An alternate means would have
// been to have saved pointers to each device object that we created
// as global variables and then to compare the DeviceObject pointer passed
// to this routine with each of the device object pointers that we created.
//
// Two of the devices are handled specially:
//   \\Device\PCATDIH - The "Win32 API Device"
//   \\Device\Ip      - The IP Filter Device
//
// The other three are "ordinary" TDI filter devices and are dispatched to
// functions like TDIH_Create, TDIH_DispatchInternalDeviceControl, etc.
//

NTSTATUS
TDIH_DefaultDispatch(
   PDEVICE_OBJECT DeviceObject,
   PIRP Irp
   )
{
   NTSTATUS                RC = STATUS_SUCCESS;
   PIO_STACK_LOCATION      IrpSp = NULL;
   PTDIH_DeviceExtension   pTDIH_DeviceExtension;
   PDEVICE_OBJECT          pLowerDeviceObject = NULL;

   // Get a pointer to the device extension that must exist for
   // all of the device objects created by the filter driver.
   pTDIH_DeviceExtension = (PTDIH_DeviceExtension )(DeviceObject->DeviceExtension);

   // Get the current I/O stack location.
   IrpSp = IoGetCurrentIrpStackLocation(Irp);
   ASSERT(IrpSp);

   ASSERT(IrpSp->MajorFunction != IRP_MJ_INTERNAL_DEVICE_CONTROL);

   //
   // Possibly Call Win32 API Device Dispatcher
   //
   if( pTDIH_DeviceExtension->NodeIdentifier.NodeType == TDIH_NODE_TYPE_W32API_DEVICE )
   {
      return( W32API_Dispatch( DeviceObject, Irp ) );
   }

#ifdef USE_IP_FILTER
   //
   // Possibly Call IP Device Filter Dispatcher
   //
   if( pTDIH_DeviceExtension->NodeIdentifier.NodeType == TDIH_NODE_TYPE_IP_FILTER_DEVICE )
   {
      return( IPFilter_Dispatch( DeviceObject, Irp ) );
   }
#endif // USE_IP_FILTER

#ifdef USE_AFD_FILTER
   //
   // Possibly Call AFD Device Filter Dispatcher
   //
   if( pTDIH_DeviceExtension->NodeIdentifier.NodeType == TDIH_NODE_TYPE_AFD_FILTER_DEVICE )
   {
      return( AfdFilter_Dispatch( DeviceObject, Irp ) );
   }
#endif // USE_AFD_FILTER

   switch(IrpSp->MajorFunction)
   {
      case IRP_MJ_CREATE:
         return( TDIH_Create(pTDIH_DeviceExtension, Irp, IrpSp) );

      case IRP_MJ_CLEANUP:
         return( TDIH_Cleanup(pTDIH_DeviceExtension, Irp, IrpSp) );

      case IRP_MJ_DEVICE_CONTROL:
         RC = TdiMapUserRequest(DeviceObject, Irp, IrpSp);

         if( RC == STATUS_SUCCESS )
         {
            KdPrint(("TDIH_DefaultDispatch: Mapped DeviceControl To InternalDeviceControl\n" ));
            return( TDIH_DispatchInternalDeviceControl(DeviceObject, Irp));
         }

         return( TDIH_DispatchDeviceControl( DeviceObject, Irp, IrpSp ) );

      case IRP_MJ_CLOSE:
         return( TDIH_Close(pTDIH_DeviceExtension, Irp, IrpSp) );

      case IRP_MJ_QUERY_SECURITY:
         KdPrint(("TDIH_DefaultDispatch: IRP_MJ_QUERY_SECURITY\n" ));
         break;

      case IRP_MJ_WRITE:
         KdPrint(("TDIH_DefaultDispatch: IRP_MJ_WRITE\n" ));
         break;

      case IRP_MJ_READ:
         KdPrint(("TDIH_DefaultDispatch: IRP_MJ_READ\n" ));
         break;

      default:
         break;
   }

   try
   {
      pLowerDeviceObject = pTDIH_DeviceExtension->LowerDeviceObject;

      // Be careful about not screwing up badly. This is actually not recommended by the I/O Manager.
      if (Irp->CurrentLocation == 1)
      {
         ULONG ReturnedInformation = 0;

         // Bad!! Fudge the error code. Break if we can ...

         KdPrint(("PCATDIH: TDIH_DefaultDispatch encountered bogus current location\n"));

   //      UTIL_BreakPoint();
         RC = STATUS_INVALID_DEVICE_REQUEST;
         Irp->IoStatus.Status = RC;
         Irp->IoStatus.Information = ReturnedInformation;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);

         return( RC );
      }

      switch( pTDIH_DeviceExtension->NodeIdentifier.NodeType )
      {
         case TDIH_NODE_TYPE_TCP_FILTER_DEVICE:
            KdPrint(("TDIH_DefaultDispatch: TCP, Major FCN: 0x%2.2X, Minor FCN: 0x%2.2X\n",
                        IrpSp->MajorFunction,
                        IrpSp->MinorFunction
                        ));
            break;

         case TDIH_NODE_TYPE_UDP_FILTER_DEVICE:
            KdPrint(("TDIH_DefaultDispatch: UDP, Major FCN: 0x%2.2X, Minor FCN: 0x%2.2X\n",
                        IrpSp->MajorFunction,
                        IrpSp->MinorFunction
                        ));
            break;

         case TDIH_NODE_TYPE_RAW_IP_FILTER_DEVICE:
            KdPrint(("TDIH_DefaultDispatch: RAW IP, Major FCN: 0x%2.2X, Minor FCN: 0x%2.2X\n",
                        IrpSp->MajorFunction,
                        IrpSp->MinorFunction
                        ));
            break;

         case TDIH_NODE_TYPE_IP_FILTER_DEVICE:
         default:
            KdPrint(("TDIH_DefaultDispatch: Target Object: 0x%x, Major FCN: 0x%2.2X, Minor FCN: 0x%2.2X\n",
                        pLowerDeviceObject,
                        IrpSp->MajorFunction,
                        IrpSp->MinorFunction
                        ));
            break;
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
         TDIH_DefaultCompletion,
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

      try_exit:   NOTHING;
   }
   finally
   {
   }

   return(RC);
}


/////////////////////////////////////////////////////////////////////////////
//                          D R I V E R  E N T R Y                         //
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//// DriverEntry
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
DriverEntry(
   PDRIVER_OBJECT DriverObject,
   PUNICODE_STRING RegistryPath
   )
{
   NTSTATUS status;
   ULONG i;

   KdPrint(("PCATDIH: Driver Entry...\n"));

   //
   // Initialize Global Variables
   //
   InitializeListHead( &FreeAddrObjList );
   InitializeListHead( &OpenAddrObjList );
   InitializeListHead( &AddrObjExInfoList );

   InitializeListHead( &FreeTCPConnList );
   InitializeListHead( &OpenTCPConnList );

   //
   // Allocate Our NDIS_BUFFER Pool
   //
   NdisAllocateBufferPool(
      &status,
      &g_hTDIH_BufferPool,
      TDIH_BUFFER_POOL_SIZE
      );

   ASSERT( (status == NDIS_STATUS_SUCCESS) );

   if( status != NDIS_STATUS_SUCCESS )
   {
      return( STATUS_UNSUCCESSFUL );
   }

   //
   // Initialize Process Name Finding Machanism
   //
   KS_InitProcessNameOffset();

   //
   // Initialize Our Driver Object
   //
#if DBG
   DriverObject->DriverUnload = TDIH_Unload;
#else
   DriverObject->DriverUnload = NULL;
#endif

   //
   // Setup MajorFunction Table
   // -------------------------
   // See additional comments for TDIH_DefaultDispatch.
   //
   for (i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
   {
      DriverObject->MajorFunction[i] = TDIH_DefaultDispatch;
   }

   //
   // Special Case Internal Device Controls
   //
   DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
      TDIH_DispatchInternalDeviceControl;

#ifdef USE_IP_FILTER
   //
   // Attach Our Filter Over IP Device
   //
   status = IPFilter_Attach( DriverObject, RegistryPath );
#endif // USE_IP_FILTER

   //
   // Attach Our Filter Over TCP Device
   //
   status = TCPFilter_Attach( DriverObject, RegistryPath );

   //
   // Attach Our Filter Over UDP Device
   //
   status = UDPFilter_Attach( DriverObject, RegistryPath );

#ifdef USE_RAW_IP_FILTER
   //
   // Attach Our Filter Over RAW IP Device
   //
   status = RawIPFilter_Attach( DriverObject, RegistryPath );
#endif // USE_RAW_IP_FILTER

#ifdef USE_AFD_FILTER
   //
   // Attach Our Filter Over AFD Device
   //
   status = AfdFilter_Attach( DriverObject, RegistryPath );
#endif // USE_AFD_FILTER

   //
   // Initialize Our Win32 API Device
   //
   status = W32API_Initialize( DriverObject, RegistryPath );

   return status;
}


/////////////////////////////////////////////////////////////////////////////
//// FindEA
//
// Purpose
// Parses an extended attribute list for a given target attribute.
//
// Parameters
//
// Return Value
// 
// Remarks
//

FILE_FULL_EA_INFORMATION UNALIGNED * // Returns: requested attribute or NULL.
FindEA(
    PFILE_FULL_EA_INFORMATION StartEA,  // First extended attribute in list.
    CHAR *TargetName,                   // Name of target attribute.
    USHORT TargetNameLength)            // Length of above.
{
   USHORT i;
   BOOLEAN found;
   FILE_FULL_EA_INFORMATION UNALIGNED *CurrentEA;

   PAGED_CODE();

   if( !StartEA )
   {
      return( NULL );
   }

   do
   {
      found = TRUE;

      CurrentEA = StartEA;
      StartEA += CurrentEA->NextEntryOffset;

      if (CurrentEA->EaNameLength != TargetNameLength)
      {
         continue;
      }

      for (i=0; i < CurrentEA->EaNameLength; i++)
      {
         if (CurrentEA->EaName[i] == TargetName[i])
         {
            continue;
         }
         found = FALSE;
         break;
      }

      if (found)
      {
         return(CurrentEA);
      }
   }
      while(CurrentEA->NextEntryOffset != 0);

    return(NULL);
}

#if DBG

/////////////////////////////////////////////////////////////////////////////
//// TDIH_Unload
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
TDIH_Unload(
   PDRIVER_OBJECT DriverObject
   )
{
   PDEVICE_OBJECT             pDeviceObject;
   PDEVICE_OBJECT             pNextDeviceObject;
   PTDIH_DeviceExtension pTDIH_DeviceExtension;

   KdPrint(("TDIH_Unload: Entry...\n"));

   pDeviceObject = DriverObject->DeviceObject;

   while( pDeviceObject != NULL )
   {
      pNextDeviceObject = pDeviceObject->NextDevice;

      pTDIH_DeviceExtension = (PTDIH_DeviceExtension )pDeviceObject->DeviceExtension;

      //
      // Detach Based On Filter Type
      //
      if( pTDIH_DeviceExtension->NodeIdentifier.NodeType == TDIH_NODE_TYPE_TCP_FILTER_DEVICE )
      {
         TCPFilter_Detach( pDeviceObject );   // Calls IoDeleteDevice
      }
      else if( pTDIH_DeviceExtension->NodeIdentifier.NodeType == TDIH_NODE_TYPE_UDP_FILTER_DEVICE )
      {
         UDPFilter_Detach( pDeviceObject );   // Calls IoDeleteDevice
      }
#ifdef USE_IP_FILTER
      else if( pTDIH_DeviceExtension->NodeIdentifier.NodeType == TDIH_NODE_TYPE_IP_FILTER_DEVICE )
      {
         IPFilter_Detach( pDeviceObject );   // Calls IoDeleteDevice
      }
#endif // USE_IP_FILTER
#ifdef USE_RAW_IP_FILTER
      else if( pTDIH_DeviceExtension->NodeIdentifier.NodeType == TDIH_NODE_TYPE_RAW_IP_FILTER_DEVICE )
      {
         RawIPFilter_Detach( pDeviceObject );   // Calls IoDeleteDevice
      }
#endif // USE_RAW_IP_FILTER
#ifdef USE_AFD_FILTER
      else if( pTDIH_DeviceExtension->NodeIdentifier.NodeType == TDIH_NODE_TYPE_AFD_FILTER_DEVICE )
      {
         AfdFilter_Detach( pDeviceObject );   // Calls IoDeleteDevice
      }
#endif // USE_AFD_FILTER
      else if( pTDIH_DeviceExtension->NodeIdentifier.NodeType == TDIH_NODE_TYPE_W32API_DEVICE )
      {
         W32API_Unload( pDeviceObject );   // Calls IoDeleteDevice
      }
      else
      {
         KdPrint(("PCATDIH: Unknown Node Type!!!\n"));
      }

      pDeviceObject = pNextDeviceObject;
   }

   if( !IsListEmpty( &FreeAddrObjList ) )
   {
      KdPrint(( "   FreeAddrObjList Not Empty!!!\n" ));
   }

   if( !IsListEmpty( &OpenAddrObjList ) )
   {
      KdPrint(( "   OpenAddrObjList Not Empty!!!\n" ));
   }

   if( !IsListEmpty( &FreeTCPConnList ) )
   {
      // Need to NdisFreeSpinLock( &pTCPConn->tc_rcv_q_lock ); then free memory...
      KdPrint(( "   FreeTCPConnList Not Empty!!!\n" ));
   }

   if( !IsListEmpty( &OpenTCPConnList ) )
   {
      KdPrint(( "   OpenTCPConnList Not Empty!!!\n" ));
   }


   //
   // Free Our NDIS_BUFFER Pool
   //
   NdisFreeBufferPool( g_hTDIH_BufferPool );

   KdPrint(("TDIH_Unload: Exit!!!\n"));
}

#endif // DBG


