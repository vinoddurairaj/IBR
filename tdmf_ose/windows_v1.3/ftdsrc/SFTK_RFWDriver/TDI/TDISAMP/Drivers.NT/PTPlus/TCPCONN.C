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

#include "ksutil.h"

// Copyright And Configuration Management ----------------------------------
//
//              TCP Connection Function Filters - TCPConn.c
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
// The Connection Context Lists
//
LIST_ENTRY FreeTCPConnList;
LIST_ENTRY OpenTCPConnList;


TCPConn *
TDIH_GetConnFromFileObject(
   PFILE_OBJECT   FileObject
   )
{
   TCPConn *pTCPConn;

   //
   // Walk The List Of Open TCP Connections
   //
   pTCPConn = (TCPConn * )OpenTCPConnList.Flink;

   while( !IsListEmpty( &OpenTCPConnList )
      && pTCPConn != (TCPConn * )&OpenTCPConnList
      )
   {
      if( pTCPConn->tc_FileObject == FileObject )
      {
         return( pTCPConn );  // Found It
      }

      pTCPConn = (TCPConn * )pTCPConn->tc_q.Flink;
   }

   return( (TCPConn * )NULL );
}


TCPConn *
TDIH_GetConnFromConnectionContext(
	PVOID ConnectionContext
   )
{
   TCPConn *pTCPConn;

   //
   // Walk The List Of Open TCP Connections
   //
   pTCPConn = (TCPConn * )OpenTCPConnList.Flink;

   while( !IsListEmpty( &OpenTCPConnList )
      && pTCPConn != (TCPConn * )&OpenTCPConnList
      )
   {
      if( pTCPConn->tc_context == ConnectionContext )
      {
         return( pTCPConn );  // Found It
      }

      pTCPConn = (TCPConn * )pTCPConn->tc_q.Flink;
   }

   return( (TCPConn * )NULL );
}


TCPConn *
TDIH_AllocateTCPConn( VOID )
{
   TCPConn *pTCPConn = (TCPConn * )NULL;

   //
   // Allocate And Initialize A New TCP Connection Record
   // ---------------------------------------------------
   // First attempt to recycle a previously allocated TCPConn structure.
   // If none available, allocate a new one.
   //
   if( !IsListEmpty( &FreeTCPConnList ) )
   {
      pTCPConn = (TCPConn * )RemoveHeadList( &FreeTCPConnList );
   }

	if( !pTCPConn || (PLIST_ENTRY )pTCPConn == &FreeTCPConnList )
	{
		pTCPConn = (TCPConn * )ExAllocatePool(NonPagedPool, sizeof( TCPConn ) );
   }

	if( pTCPConn )
	{
      NdisZeroMemory( pTCPConn, sizeof(TCPConn) );

      InitializeListHead( &pTCPConn->tc_send_q );

      NdisAllocateSpinLock( &pTCPConn->tc_rcv_q_lock );
      InitializeListHead( &pTCPConn->tc_rcv_q );

      InitializeListHead( &pTCPConn->tc_rcv_pkt_q );

      NdisInitializeTimer(
         &pTCPConn->tc_rx_timer,
         TDIH_ReceiveTimerProc,
         pTCPConn
         );
   }

   return( pTCPConn );
}


VOID
TDIH_FreeTCPConn( TCPConn *pTCPConn )
{
   // ATTENTION!!! Anything else to do here???

   //
   // Free Orphan TDIH_PACKET(s)
   // --------------------------
   // A final packet, usually of zero length and a non-success status,
   // will be seen occasionally. This is possible during a disconnect
   // the private receive mechanism may be called with an aborted packet,
   // which is queued in the filter but not necessarily passed up to
   // the higher-level client.
   //
   // Seeing multiple orphan packets or packets with non-zero length
   // would be suspicious.
   //
   while( !IsListEmpty( &pTCPConn->tc_rcv_pkt_q ) )
   {
      PTDIH_PACKET   pTDIH_Packet;

      pTDIH_Packet = (PTDIH_PACKET )RemoveHeadList( &pTCPConn->tc_rcv_pkt_q );

      KdPrint(( "TDIH_FreeTCPConn: Orphan TDIH_PACKET; Status: 0x%8.8X; Bytes %d\n",
         pTDIH_Packet->IoStatus, pTDIH_Packet->ContigSize ));

      TDIH_FreePacket( pTDIH_Packet );
   }

   //
   // Cancel Orphan TdiReceive Requests
   //
   if( !IsListEmpty( &pTCPConn->tc_rcv_q ) )
   {
      KdPrint(( "TDIH_FreeTCPConn: Orphan TdiReceive Requests To Cancel!!!\n" ));
   }

   //
   // Add To The Free TCP Connection List
   // -----------------------------------
   // Don't free TCPConn structures. Instead, queue them in
   // the Free TCP Connection List for re-use. This should
   // reduce memory fragmentation since TCPConn are likely
   // to be needed again.
   //
   InsertTailList(
      &FreeTCPConnList,
      &pTCPConn->tc_q
      );
}


/////////////////////////////////////////////////////////////////////////////
//// TDIH_TdiOpenConnectionComplete
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
TDIH_TdiOpenConnectionComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              Irp,
   void              *Context
   )
{
   PTDIH_DeviceExtension   pTDIH_DeviceExtension = NULL;
   BOOLEAN                 CanDetachProceed = FALSE;
	NTSTATUS                Status = Irp->IoStatus.Status;
   TCPConn                 *pTCPConn = NULL;

   KdPrint(( "TDIH_TdiOpenConnectionComplete: Status: 0x%8.8X\n", Status ));

   ASSERT( Context );
   if( Context )
   {
      pTCPConn = (TCPConn * )Context;

      pTDIH_DeviceExtension = pTCPConn->tc_DeviceExtension;
      ASSERT( pTDIH_DeviceExtension );
   }

   //
   // Propogate The IRP Pending Flag
   //
   if (Irp->PendingReturned)
   {
      IoMarkIrpPending(Irp);
   }

   // Ensure that this is a valid device object pointer, else return
   // immediately.
   if( pTDIH_DeviceExtension )
   {
      PDEVICE_OBJECT          pAssociatedDeviceObject = NULL;

      pAssociatedDeviceObject = pTDIH_DeviceExtension->pFilterDeviceObject;

      if (pAssociatedDeviceObject != pDeviceObject)
      {
         KdPrint(( "TDIH_OpenConnectionCompletion: Invalid Device Object Pointer\n" ));
         return(STATUS_SUCCESS);
      }
   }

   // Note that you could do all sorts of processing at this point
   // depending upon the results of the operation. Be careful though
   // about what you chose to do, about the fact that this completion
   // routine is being invoked in an arbitrary thread context and probably
   // at high IRQL.

   if( pTCPConn )
   {
      if( NT_SUCCESS( Status ) )
      {
         //
         // Add To The Open TCP Connection List
         //
         InsertTailList(
            &OpenTCPConnList,
            &pTCPConn->tc_q
            );
      }
      else
      {
         //
         // Add To The Free TCP Connection List
         //
         InsertTailList(
            &FreeTCPConnList,
            &pTCPConn->tc_q
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
//// TDIH_TdiOpenConnection
//
// Purpose
// This is the hook for TdiOpenConnection
//
// Parameters
//
// Return Value
// 
// Remarks
//

NTSTATUS
TDIH_TdiOpenConnection(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp,
   PVOID                   ConnectionContext
   )
{
   NTSTATUS RC;
   TCPConn *pTCPConn = (TCPConn * )NULL;

   KdPrint(( "TDIH_TdiOpenConnection: Entry...\n" ));

   //
   // Allocate And Initialize A New TCP Connection Record
   //
   pTCPConn = TDIH_AllocateTCPConn();

	if( pTCPConn )
	{
      pTCPConn->tc_DeviceExtension = pTDIH_DeviceExtension;

      pTCPConn->tc_FileObject = IrpSp->FileObject;

      pTCPConn->tc_ao = NULL;          // Not Yet Associated

      // ATTENTION Fetch Context from EA Buffer...
      pTCPConn->tc_context = ConnectionContext; // Caller's Context
   }
   else
   {
      KdPrint(( "TDIH_TdiOpenConnection: Cound not allocate TCPConn Entry\n" ));
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

         KdPrint(("TDIH_TdiOpenConnection encountered bogus current location\n"));

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
         TDIH_TdiOpenConnectionComplete,
//         pTDIH_DeviceExtension,
         pTCPConn,
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
//// TDIH_TdiCloseConnectionComplete
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
TDIH_TdiCloseConnectionComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              Irp,
   void              *Context
   )
{
   PTDIH_DeviceExtension   pTDIH_DeviceExtension = NULL;
   BOOLEAN                 CanDetachProceed = FALSE;
   PDEVICE_OBJECT          pAssociatedDeviceObject = NULL;
	NTSTATUS                Status = Irp->IoStatus.Status;

   KdPrint(( "TDIH_TdiCloseConnectionComplete: Status: 0x%8.8X\n", Status ));

   ASSERT( Context );
   if( Context )
   {
      pTDIH_DeviceExtension = (PTDIH_DeviceExtension )(Context);
   }

   //
   // Propogate The IRP Pending Flag
   //
   if (Irp->PendingReturned)
   {
      IoMarkIrpPending(Irp);
   }

   // Ensure that this is a valid device object pointer, else return
   // immediately.
   if( pTDIH_DeviceExtension )
   {
      pAssociatedDeviceObject = pTDIH_DeviceExtension->pFilterDeviceObject;

      if (pAssociatedDeviceObject != pDeviceObject)
      {
         KdPrint(( "TDIH_CloseConnectionComplete: Invalid Device Object Pointer\n" ));
         return(STATUS_SUCCESS);
      }
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
//// TDIH_TdiCloseConnection
//
// Purpose
// This is the hook for TdiCloseConnection
//
// Parameters
//
// Return Value
// 
// Remarks
//

NTSTATUS
TDIH_TdiCloseConnection(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   )
{
   NTSTATUS             RC;
   TCPConn              *pTCPConn = (TCPConn * )NULL;

   KdPrint(( "TDIH_TdiCloseConnection: Entry...\n" ));

   //
   // Locate TCP Connection Object
   //
   pTCPConn = TDIH_GetConnFromFileObject( IrpSp->FileObject );
   ASSERT( pTCPConn );

   if( pTCPConn )
   {
      BOOLEAN  bTimerCancelled;

      KdPrint(("  Found TCPConn For Close\n" ));

      //
      // Remove From The TCP Connection List
      //
      RemoveEntryList( &pTCPConn->tc_q );

      NdisCancelTimer( &pTCPConn->tc_rx_timer, &bTimerCancelled );

      TDIH_FreeTCPConn( pTCPConn );
   }
   else
   {
      KdPrint(("  TCPConn For Close NOT FOUND!!!\n" ));
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

         KdPrint(("TDIH_TdiCloseConnection encountered bogus current location\n"));

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
         TDIH_TdiCloseConnectionComplete,
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


TDIH_TdiAcceptComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              Irp,
   void              *Context
   )
{
   PTDIH_DeviceExtension   pTDIH_DeviceExtension = (PTDIH_DeviceExtension )NULL;
   BOOLEAN                 CanDetachProceed = FALSE;
   PDEVICE_OBJECT          pAssociatedDeviceObject = NULL;
	NTSTATUS                Status = Irp->IoStatus.Status;
   TCPConn                 *pTCPConn = (TCPConn * )NULL;
   PIO_STACK_LOCATION      IrpSp = NULL;

   KdPrint(( "TDIH_TdiAcceptComplete: Status: 0x%8.8X\n", Status ));

   pTDIH_DeviceExtension = (PTDIH_DeviceExtension )Context;

   // Get the current I/O stack location.
   IrpSp = IoGetCurrentIrpStackLocation( Irp );
   ASSERT(IrpSp);

   pTCPConn = TDIH_GetConnFromFileObject( IrpSp->FileObject );
   ASSERT( pTCPConn );

   if( pTCPConn && (Status == STATUS_SUCCESS) )
   {
      PTDI_REQUEST_KERNEL_ACCEPT p;

      p = (PTDI_REQUEST_KERNEL_ACCEPT )&(IrpSp->Parameters);

      //
      // Save The Remote Address
      //
      if( p && p->ReturnConnectionInformation )
      {
         //
         // Save Return Remote Address
         //
         NdisMoveMemory(
            &pTCPConn->tc_RemoteAddress,
            (p->ReturnConnectionInformation)->RemoteAddress,
            sizeof( TA_IP_ADDRESS )
            );
      }

      //
      // Display The Remote Address
      //
      DEBUG_DumpTransportAddress(
         (PTRANSPORT_ADDRESS )&pTCPConn->tc_RemoteAddress
         );

      //
      // Start First Private Receive On Connection
      //
      TDIH_TdiStartPrivateReceive( pTCPConn );
   }

   //
   // Propogate The IRP Pending Flag
   //
   if (Irp->PendingReturned)
   {
      IoMarkIrpPending(Irp);
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


NTSTATUS
TDIH_TdiConnectEventHandler(
   IN PVOID TdiEventContext,     // Context From SetEventHandler
   IN LONG RemoteAddressLength,
   IN PVOID RemoteAddress,
   IN LONG UserDataLength,       // Unused for MSTCP
   IN PVOID UserData,            // Unused for MSTCP
   IN LONG OptionsLength,
   IN PVOID Options,
   OUT CONNECTION_CONTEXT *ConnectionContext,
   OUT PIRP *hAcceptIrp
   )
{
	NTSTATUS                Status;
	AddrObj                 *pAddrObj;
   PTDIH_DeviceExtension   pTDIH_DeviceExtension;

   KdPrint(("TDIH_TdiConnectEventHandler: Entry...\n") );

   //
   // EventContext Points To Our Address Object Record
   //
   ASSERT( TdiEventContext );
	pAddrObj = (AddrObj * )TdiEventContext;

   if( !pAddrObj )
   {
      return( STATUS_DATA_NOT_ACCEPTED );
   }

   pTDIH_DeviceExtension = pAddrObj->ao_DeviceExtension;

   //
   // Pass To Connect Event Handler Set On The Address Object
   //
   ASSERT( pAddrObj->ao_connect );
	Status = pAddrObj->ao_connect(
               pAddrObj->ao_conncontext,
               RemoteAddressLength,
               RemoteAddress,
               UserDataLength,      // Unused for MSTCP
               UserData,            // Unused for MSTCP
               OptionsLength,
               Options,
               ConnectionContext,
               hAcceptIrp
		         );

   if( Status == STATUS_MORE_PROCESSING_REQUIRED && hAcceptIrp && *hAcceptIrp )
   {
      PTDI_REQUEST_KERNEL_ACCEPT p;
      PIRP                       Irp;
      PIO_STACK_LOCATION         IrpSp = NULL;
      TCPConn                    *pTCPConn = (TCPConn * )NULL;

   	KdPrint(( "TDIH_ConnectEventHandler: Client Accepting Connection\n" ));

      Irp = *hAcceptIrp;

      // Get the current I/O stack location.
      IrpSp = IoGetCurrentIrpStackLocation( Irp );
      ASSERT(IrpSp);

      p = (PTDI_REQUEST_KERNEL_ACCEPT )&(IrpSp->Parameters);

      pTCPConn = TDIH_GetConnFromFileObject( IrpSp->FileObject );
      ASSERT( pTCPConn );

      if( pTCPConn )
      {
         if( p && p->RequestConnectionInformation )
         {
            //
            // Save Request Remote Address
            //
            NdisMoveMemory(
               &pTCPConn->tc_RemoteAddress,
               (p->RequestConnectionInformation)->RemoteAddress,
               sizeof( TA_IP_ADDRESS )
               );
         }
         else
         {
            //
            // Save Default Remote Address
            //
            NdisMoveMemory(
               &pTCPConn->tc_RemoteAddress,
               RemoteAddress,
               sizeof( TA_IP_ADDRESS )
               );
         }

         //
         // Display The Remote Address
         //
         DEBUG_DumpTransportAddress(
            (PTRANSPORT_ADDRESS )&pTCPConn->tc_RemoteAddress
            );
      }

      // Be careful about not screwing up badly. This is actually not recommended by the I/O Manager.
      if (Irp->CurrentLocation == 1)
      {
         KdPrint(("TDIH_TdiConnectEventHandler encountered bogus current location\n"));
      }

      IoCopyCurrentIrpStackLocationToNext( Irp );

      //
      // Set Completion Routine
      //
      IoSetCompletionRoutine(
         Irp,
         TDIH_TdiAcceptComplete,
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

      // IoSetNextIrpStackLocation
      Irp->CurrentLocation--;
      Irp->Tail.Overlay.CurrentStackLocation--;
   }
   else
   {
   	KdPrint(( "TDIH_ConnectEventHandler: Status: 0x%8.8X\n", Status ));
   }

   return( Status );
}


/////////////////////////////////////////////////////////////////////////////
//// TDIH_TdiConnectComplete
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
TDIH_TdiConnectComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              Irp,
   void              *Context
   )
{
   PTDIH_DeviceExtension   pTDIH_DeviceExtension = (PTDIH_DeviceExtension )NULL;
   BOOLEAN                 CanDetachProceed = FALSE;
   PDEVICE_OBJECT          pAssociatedDeviceObject = NULL;
	NTSTATUS                Status = Irp->IoStatus.Status;
   AddrObj                 *pAddrObj = NULL;
   TCPConn                 *pTCPConn = NULL;

   KdPrint(( "TDIH_TdiConnectComplete: Status: 0x%8.8X\n", Status ));

   ASSERT( Context );

   if( Context )
   {
      pTCPConn = (TCPConn * )Context;

      pAddrObj = pTCPConn->tc_ao;
      ASSERT( pAddrObj );

      pTDIH_DeviceExtension = pAddrObj->ao_DeviceExtension;
      ASSERT( pTDIH_DeviceExtension );
   }

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
      KdPrint(( "TDIH_ConnectComplete: Invalid Device Object Pointer\n" ));
      return(STATUS_SUCCESS);
   }

   // Note that you could do all sorts of processing at this point
   // depending upon the results of the operation. Be careful though
   // about what you chose to do, about the fact that this completion
   // routine is being invoked in an arbitrary thread context and probably
   // at high IRQL.

   ASSERT( pTCPConn );
   if( pTCPConn )
   {
      //
      // Start First Private Receive On Connection
      //
      TDIH_TdiStartPrivateReceive( pTCPConn );
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
//// TDIH_TdiConnect
//
// Purpose
// This is the hook for TdiConnect
//
// Parameters
//
// Return Value
// 
// Remarks
//

NTSTATUS
TDIH_TdiConnect(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   )
{
   NTSTATUS             RC;
   PTDI_REQUEST_KERNEL  p;
   TCPConn              *pTCPConn = NULL;

   KdPrint(( "TDIH_TdiConnect: Entry...\n" ));

   p = (PTDI_REQUEST_KERNEL )&IrpSp->Parameters;

   pTCPConn = TDIH_GetConnFromFileObject( IrpSp->FileObject );
   ASSERT( pTCPConn );

   //
   // Save Remote Address
   //
   NdisMoveMemory(
      &pTCPConn->tc_RemoteAddress,
      (p->RequestConnectionInformation)->RemoteAddress,
      sizeof( TA_IP_ADDRESS )
      );

   //
   // Display The Remote Address
   //
   DEBUG_DumpTransportAddress(
      (PTRANSPORT_ADDRESS )&pTCPConn->tc_RemoteAddress
      );

   //
   // Pass The Request To TCP Device
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

         KdPrint(("TDIH_TdiConnect encountered bogus current location\n"));

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
         TDIH_TdiConnectComplete,
//         pTDIH_DeviceExtension,
         pTCPConn,
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


NTSTATUS
TDIH_TdiListenComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              Irp,
   void              *Context
   )
{
   PTDIH_DeviceExtension   pTDIH_DeviceExtension;
   BOOLEAN                 CanDetachProceed = FALSE;
   PDEVICE_OBJECT          pAssociatedDeviceObject = NULL;
	NTSTATUS                Status = Irp->IoStatus.Status;
   TCPConn                 *pTCPConn;
   PIO_STACK_LOCATION      IrpSp = NULL;

   KdPrint(( "TDIH_TdiListenComplete: Status: 0x%8.8X\n", Status ));

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
      KdPrint(( "TDIH_TdiListenComplete: Invalid Device Object Pointer\n" ));
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
   // Locate TCP Connection Object
   //
   pTCPConn = TDIH_GetConnFromFileObject( IrpSp->FileObject );

   if( pTCPConn )
	{
      KdPrint(("  Found TCPConn For Listen Completion\n" ));

      if( Status == STATUS_SUCCESS )
      {
         PTDI_REQUEST_KERNEL p;

         p = (PTDI_REQUEST_KERNEL)&IrpSp->Parameters;

         if( p->RequestFlags == 0 )
         {
            //
            // Underlying Transport Has Accepted Connection
            //
            if( p && p->ReturnConnectionInformation )
            {
               //
               // Save Return Remote Address
               //
               NdisMoveMemory(
                  &pTCPConn->tc_RemoteAddress,
                  (p->ReturnConnectionInformation)->RemoteAddress,
                  sizeof( TA_IP_ADDRESS )
                  );
            }

            //
            // Display The Remote Address
            //
            DEBUG_DumpTransportAddress(
               (PTRANSPORT_ADDRESS )&pTCPConn->tc_RemoteAddress
               );

            //
            // Start First Private Receive On Connection
            //
            TDIH_TdiStartPrivateReceive( pTCPConn );
         }
         else if( p->RequestFlags == TDI_QUERY_ACCEPT )
         {
            //
            // Higher-Level Client Must Accept Or Reject Offer
            // -----------------------------------------------
            // Expect a separate TdiAccept or TdiDisconnect from the
            // client to indicate how to deal with the connection
            // attempt.
            //
         }
      }
   }
   else
   {
      KdPrint(( "TDIH_TdiListenComplete: TCPConn Record Not Found\n" ));
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


NTSTATUS
TDIH_TdiListen(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   )
{
   NTSTATUS RC;

   KdPrint(( "TDIH_TdiListen: Entry...\n" ));

   try
   {
      PDEVICE_OBJECT       pLowerDeviceObject = NULL;

      pLowerDeviceObject = pTDIH_DeviceExtension->LowerDeviceObject;

      // Be careful about not screwing up badly. This is actually not recommended by the I/O Manager.
      if (Irp->CurrentLocation == 1)
      {
         ULONG ReturnedInformation = 0;

         // Bad!! Fudge the error code. Break if we can ...

         KdPrint(("TDIH_TdiListen encountered bogus current location\n"));

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
         TDIH_TdiListenComplete,
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
//// TDIH_TdiDisconnectComplete
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
TDIH_TdiDisconnectComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              Irp,
   void              *Context
   )
{
   PTDIH_DeviceExtension   pTDIH_DeviceExtension;
   BOOLEAN                 CanDetachProceed = FALSE;
   PDEVICE_OBJECT          pAssociatedDeviceObject = NULL;
	NTSTATUS                Status = Irp->IoStatus.Status;

   KdPrint(( "TDIH_TdiDisconnectComplete: Status: 0x%8.8X\n", Status ));

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
      KdPrint(( "TDIH_DisconnectComplete: Invalid Device Object Pointer\n" ));
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
//// TDIH_TdiDisconnect
//
// Purpose
// This is the hook for TdiDisconnect
//
// Parameters
//
// Return Value
// 
// Remarks
//

NTSTATUS
TDIH_TdiDisconnect(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   )
{
   NTSTATUS RC;

   KdPrint(( "TDIH_TdiDisconnect: Entry...\n" ));

   //
   // Pass The Request To TCP Device
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

         KdPrint(("TDIH_TdiDisconnect encountered bogus current location\n"));

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
         TDIH_TdiDisconnectComplete,
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
//// TDIH_TdiAssociateAddressComplete
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
TDIH_TdiAssociateAddressComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              Irp,
   void              *Context
   )
{
   PTDIH_DeviceExtension   pTDIH_DeviceExtension = NULL;
   BOOLEAN                 CanDetachProceed = FALSE;
   PDEVICE_OBJECT          pAssociatedDeviceObject = NULL;
	NTSTATUS                Status = Irp->IoStatus.Status;
   TCPConn                 *pTCPConn = (TCPConn * )NULL;
   PIO_STACK_LOCATION      IrpSp = NULL;
   PTDI_REQUEST_KERNEL_ASSOCIATE p;
   PFILE_OBJECT            pFileObject = NULL;

   KdPrint(( "TDIH_TdiAssociateAddressComplete: Status: 0x%8.8X\n", Status ));

   ASSERT( Context );

   if( Context )
   {
      pTDIH_DeviceExtension = (PTDIH_DeviceExtension )(Context);
   }

   //
   // Propogate The IRP Pending Flag
   //
   if (Irp->PendingReturned)
   {
      IoMarkIrpPending(Irp);
   }

   // Ensure that this is a valid device object pointer, else return
   // immediately.
   if( pTDIH_DeviceExtension )
   {
      pAssociatedDeviceObject = pTDIH_DeviceExtension->pFilterDeviceObject;

      if (pAssociatedDeviceObject != pDeviceObject)
      {
         KdPrint(( "TDIH_AssociateAddressComplete: Invalid Device Object Pointer\n" ));
         return(STATUS_SUCCESS);
      }
   }

   // Note that you could do all sorts of processing at this point
   // depending upon the results of the operation. Be careful though
   // about what you chose to do, about the fact that this completion
   // routine is being invoked in an arbitrary thread context and probably
   // at high IRQL.

   // Get the current I/O stack location.
   IrpSp = IoGetCurrentIrpStackLocation(Irp);
   ASSERT(IrpSp);

   if( NT_SUCCESS( Status ) )
   {
      //
      // Locate TCP Connection Object
      //
      pTCPConn = TDIH_GetConnFromFileObject( IrpSp->FileObject );

      if( pTCPConn )
      {
         KdPrint(("  Found TCPConn To Associate\n" ));
         ASSERT( !pTCPConn->tc_ao );

         //
         // Locate The Transport Address Object
         //
         p = (PTDI_REQUEST_KERNEL_ASSOCIATE)&IrpSp->Parameters;

         //
         // Obtain a referenced pointer to the file object.
         //
         ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
         Status = ObReferenceObjectByHandle(
                     p->AddressHandle,          // Object Handle
                     FILE_ANY_ACCESS,           // Desired Access
                     NULL,                      // Object Type
                     KernelMode,                // Processor mode
                     (PVOID *)&pFileObject,     // File Object pointer
                     NULL                       // Object Handle information
                     );

         if( NT_SUCCESS( Status ) )
         {
            AddrObj *pAddrObj;

            pAddrObj = TDIH_GetAddrObjFromFileObject( pFileObject );

            ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
            ObDereferenceObject( pFileObject );

            if( pAddrObj )
            {
               KdPrint(("  Found AddrObj To Associate\n" ));

               pTCPConn->tc_ao = pAddrObj;

               //
               // Save Receive Event Handlers
               //
               pTCPConn->ao_rcv = pAddrObj->ao_rcv;
               pTCPConn->ao_rcvcontext = pAddrObj->ao_rcvcontext;
               pTCPConn->ao_exprcv = pAddrObj->ao_exprcv;
               pTCPConn->ao_exprcvcontext = pAddrObj->ao_exprcvcontext;
            }   
         }
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
//// TDIH_TdiAssociateAddress
//
// Purpose
// This is the hook for TdiAssociateAddress
//
// Parameters
//
// Return Value
// 
// Remarks
//

NTSTATUS
TDIH_TdiAssociateAddress(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   )
{
   NTSTATUS RC;
   KdPrint(( "TDIH_TdiAssociateAddress: Entry...\n" ));

   try
   {
      PDEVICE_OBJECT       pLowerDeviceObject = NULL;

      pLowerDeviceObject = pTDIH_DeviceExtension->LowerDeviceObject;

      // Be careful about not screwing up badly. This is actually not recommended by the I/O Manager.
      if (Irp->CurrentLocation == 1)
      {
         ULONG ReturnedInformation = 0;

         // Bad!! Fudge the error code. Break if we can ...

         KdPrint(("TDIH_TdiAssociateAddress encountered bogus current location\n"));

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
         TDIH_TdiAssociateAddressComplete,
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
//// TDIH_TdiDisAssociateAddressComplete
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
TDIH_TdiDisAssociateAddressComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              Irp,
   void              *Context
   )
{
   PTDIH_DeviceExtension   pTDIH_DeviceExtension;
   BOOLEAN                 CanDetachProceed = FALSE;
   PDEVICE_OBJECT          pAssociatedDeviceObject = NULL;
	NTSTATUS                Status = Irp->IoStatus.Status;
   TCPConn                 *pTCPConn;
   PIO_STACK_LOCATION      IrpSp = NULL;

   KdPrint(( "TDIH_TdiDisAssociateAddressComplete: Status: 0x%8.8X\n", Status ));

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
      KdPrint(( "TDIH_DisAssociateAddressComplete: Invalid Device Object Pointer\n" ));
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
   // Locate TCP Connection Object
   //
   pTCPConn = TDIH_GetConnFromFileObject( IrpSp->FileObject );

   if( pTCPConn )
	{
      KdPrint(("  Found TCPConn To DisAssociate\n" ));
      pTCPConn->tc_ao = NULL;
   }
   else
   {
      KdPrint(( "TDIH_TdiDisAssociateAddress: TCPConn Record Not Found\n" ));
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
//// TDIH_TdiDisAssociateAddress
//
// Purpose
// This is the hook for TdiDisAssociateAddress
//
// Parameters
//
// Return Value
// 
// Remarks
//

NTSTATUS
TDIH_TdiDisAssociateAddress(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   )
{
   NTSTATUS RC;

   KdPrint(( "TDIH_TdiDisAssociateAddress: Entry...\n" ));

   try
   {
      PDEVICE_OBJECT       pLowerDeviceObject = NULL;

      pLowerDeviceObject = pTDIH_DeviceExtension->LowerDeviceObject;

      // Be careful about not screwing up badly. This is actually not recommended by the I/O Manager.
      if (Irp->CurrentLocation == 1)
      {
         ULONG ReturnedInformation = 0;

         // Bad!! Fudge the error code. Break if we can ...

         KdPrint(("TDIH_TdiDisAssociateAddress encountered bogus current location\n"));

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
         TDIH_TdiDisAssociateAddressComplete,
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



