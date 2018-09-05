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
      InitializeListHead( &pTCPConn->tc_rcv_q );
   }

   return( pTCPConn );
}


VOID
TDIH_FreeTCPConn( TCPConn *pTCPConn )
{
   // ATTENTION!!! Anything else to do here???

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
   TCPConn                 *pTCPConn = (TCPConn * )NULL;
   PIO_STACK_LOCATION      IrpSp = NULL;

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

   // Get the current I/O stack location.
   IrpSp = IoGetCurrentIrpStackLocation(Irp);
   ASSERT(IrpSp);

   //
   // Locate TCP Connection Object
   //
   pTCPConn = TDIH_GetConnFromFileObject( IrpSp->FileObject );
   ASSERT( pTCPConn );

   if( pTCPConn )
   {
      KdPrint(("  Found TCPConn For Close\n" ));

      //
      // Remove From The TCP Connection List
      //
      RemoveEntryList( &pTCPConn->tc_q );

      TDIH_FreeTCPConn( pTCPConn );
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
   NTSTATUS RC;

   KdPrint(( "TDIH_TdiCloseConnection: Entry...\n" ));

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
	NTSTATUS    Status;
	AddrObj     *pAddrObj;
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

	KdPrint(( "TDIH_ConnectEventHandler: Status: 0x%8.8X\n", Status ));

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
   PTDIH_DeviceExtension   pTDIH_DeviceExtension;
   BOOLEAN                 CanDetachProceed = FALSE;
   PDEVICE_OBJECT          pAssociatedDeviceObject = NULL;
	NTSTATUS                Status = Irp->IoStatus.Status;

   KdPrint(( "TDIH_TdiConnectComplete: Status: 0x%8.8X\n", Status ));

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
      KdPrint(( "TDIH_ConnectComplete: Invalid Device Object Pointer\n" ));
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

   KdPrint(( "TDIH_TdiConnect: Entry...\n" ));

   p = (PTDI_REQUEST_KERNEL )&IrpSp->Parameters;

   //
   // Display The Remote Address
   //
   DEBUG_DumpTransportAddress(
      (p->RequestConnectionInformation)->RemoteAddress
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



