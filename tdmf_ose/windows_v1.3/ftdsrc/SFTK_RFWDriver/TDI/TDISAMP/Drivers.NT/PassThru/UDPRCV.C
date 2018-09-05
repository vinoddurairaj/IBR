/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include "ndis.h"
#include "TDI.H"
#include "TDIKRNL.H"
#include "PCATDIH.h"

#include "inetinc.h"
#include "KSUtil.h"

#include "addr.h"
#include "udprcv.h"

// Copyright And Configuration Management ----------------------------------
//
//                 UDP Receive Function Filters - UDPRcv.c
//         Transport Data Interface (TDI) Filter For Windows NT
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


NTSTATUS
TDIH_TdiReceiveDGOnEventComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              Irp,
   void              *Context
   );

/////////////////////////////////////////////////////////////////////////////
//// TDIH_TdiReceiveDGEventHandler
//
// Purpose
// This is the hook for TDI_EVENT_RECIEVE_DATAGRAM event.
//
// Parameters
// See NTDDK documentation for TDI_EVENT_RECIEVE_DATAGRAM.
//
// Return Value
// See NTDDK documentation for TDI_EVENT_RECIEVE_DATAGRAM.
// 
// Remarks
// This hook is called by the UDP device to indicate a packet to a
// TDI client who has set a TDI_EVENT_RECEIVE_DATAGRAM event handler
// on the Address Object.
//

NTSTATUS
TDIH_TdiReceiveDGEventHandler(
   PVOID TdiEventContext,        // Context From SetEventHandler
   LONG SourceAddressLength,     // length of the originator of the datagram
   PVOID SourceAddress,          // string describing the originator of the datagram
   LONG OptionsLength,           // options for the receive
   PVOID Options,                //
   ULONG ReceiveDatagramFlags,   //
   ULONG BytesIndicated,         // number of bytes in this indication
   ULONG BytesAvailable,         // number of bytes in complete Tsdu
   ULONG *BytesTaken,            // number of bytes used by indication routine
   PVOID Tsdu,                   // pointer describing this TSDU, typically a lump of bytes
   PIRP *IoRequestPacket         // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
   )
{
	NTSTATUS                Status;
	AddrObj                 *pAddrObj;
   PTDIH_DeviceExtension   pTDIH_DeviceExtension;

//   KdPrint(("TDIH_TdiReceiveDGEventHandler: Entry...\n") );

//   KdPrint(("  Bytes Indicated: %d; BytesAvailable: %d; Flags: 0x%8.8x\n",
//      BytesIndicated, BytesAvailable, ReceiveDatagramFlags));

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
   // Pass To Receive Datagram Event Handler Set On The Address Object
   //
   ASSERT( pAddrObj->ao_rcvdg );
	Status = pAddrObj->ao_rcvdg(
               pAddrObj->ao_rcvdgcontext,
               SourceAddressLength,    // length of the originator of the datagram
               SourceAddress,          // string describing the originator of the datagram
               OptionsLength,          // options for the receive
               Options,                //
               ReceiveDatagramFlags,   //
               BytesIndicated,         // number of bytes in this indication
               BytesAvailable,         // number of bytes in complete Tsdu
               BytesTaken,             // number of bytes used by indication routine
               Tsdu,                   // pointer describing this TSDU, typically a lump of bytes
               IoRequestPacket         // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
		         );

//	KdPrint(( "TDIH_RcvDGEventHandler: Status: 0x%8.8X; Taken: %d\n",
//      Status, *BytesTaken ));

   if( Status == STATUS_MORE_PROCESSING_REQUIRED
      && IoRequestPacket && *IoRequestPacket
      )
   {
      PIO_STACK_LOCATION   IrpSp = NULL;

      KdPrint(("  Returned STATUS_MORE_PROCESSING_REQUIRED\n"));

      //
      // Handle TDI_RECEIVE_DATAGRAM Request Passdown
      // --------------------------------------------
      // The caller has provided a TDI_RECEIVE_DATAGRAN request at
      // IoRequestPacket to obtain the remaining Tdsu data.
      //
      // In the PassThru sample driver we want to filter processing of
      // the request using our own completion handler. The key steps are:
      //
      //   1.) Copy the current IRP stack location to next.
      //   2.) Set our completion routine.
      //   3.) Emulate the effect of IoCallDriver in advancing the
      //       IRP stack location.
      //

      // Get the current I/O stack location.
      IrpSp = IoGetCurrentIrpStackLocation( *IoRequestPacket );
      ASSERT(IrpSp);

      // Be careful about not screwing up badly. This is actually not recommended by the I/O Manager.
      if ((*IoRequestPacket)->CurrentLocation == 1)
      {
         KdPrint(("TDIH_RcvDGEventHandler encountered bogus current location\n"));
      }

      IoCopyCurrentIrpStackLocationToNext( (*IoRequestPacket) );

      //
      // Set Completion Routine
      //
      IoSetCompletionRoutine(
         (*IoRequestPacket),
         TDIH_TdiReceiveDGOnEventComplete,
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
      (*IoRequestPacket)->CurrentLocation--;
      (*IoRequestPacket)->Tail.Overlay.CurrentStackLocation--;
   }

   return( Status );
}


/////////////////////////////////////////////////////////////////////////////
//// TDIH_TdiChainedReceiveDGEventHandler
//
// Purpose
// This is the hook for TDI_EVENT_CHAINED_RECIEVE_DATAGRAM event.
//
// Parameters
// See NTDDK documentation for TDI_EVENT_CHAINED_RECIEVE_DATAGRAM.
//
// Return Value
// See NTDDK documentation for TDI_EVENT_CHAINED_RECIEVE_DATAGRAM.
// 
// Remarks
// This hook is called by the UDP device to indicate a packet to a
// TDI client who has set a TDI_EVENT_CHAINED_RECEIVE_DATAGRAM event
// handler on the Address Object.
//
// It is defined in the NT DDK. However, its first actual use seems to
// be in Windows 2000.
//

NTSTATUS
TDIH_TdiChainedReceiveDGEventHandler(
   PVOID TdiEventContext,        // Context From SetEventHandler
   LONG SourceAddressLength,     // length of the originator of the datagram
   PVOID SourceAddress,          // string describing the originator of the datagram
   LONG OptionsLength,           // options for the receive
   PVOID Options,                //
   ULONG ReceiveDatagramFlags,   //
   ULONG ReceiveDatagramLength,  // length of client data in TSDU
   ULONG StartingOffset,         // offset of start of client data in TSDU
   PMDL  Tsdu,                   // TSDU data chain
   PVOID TsduDescriptor          // for call to TdiReturnChainedReceives
   )
{
	NTSTATUS                Status;
	AddrObj                 *pAddrObj;
   PTDIH_DeviceExtension   pTDIH_DeviceExtension;

//   KdPrint(("TDIH_TdiChainedReceiveDGEventHandler: Entry...\n") );

//   KdPrint(("  ReceiveDatagramLength: %d; StartingOffset: %d; Flags: 0x%8.8x\n",
//      ReceiveDatagramLength, StartingOffset, ReceiveDatagramFlags));

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
   // Pass To Receive Datagram Event Handler Set On The Address Object
   //
   ASSERT( pAddrObj->ao_chainedrcvdg );
	Status = pAddrObj->ao_chainedrcvdg(
               pAddrObj->ao_chainedrcvdgcontext,
               SourceAddressLength,    // length of the originator of the datagram
               SourceAddress,          // string describing the originator of the datagram
               OptionsLength,          // options for the receive
               Options,                //
               ReceiveDatagramFlags,   //
               ReceiveDatagramLength,  // length of client data in TSDU
               StartingOffset,         // offset of start of client data in TSDU
               Tsdu,                   // TSDU data chain
               TsduDescriptor          // for call to TdiReturnChainedReceives
		         );

//	KdPrint(( "TDIH_ChainedRcvDGEventHandler: Status: 0x%8.8X\n", Status ));

   return( Status );
}


/////////////////////////////////////////////////////////////////////////////
//// _I_TdiReceiveDGComplete
//
// Purpose
// Common handler for handling completion of receive datagrams initiated
// by calling TdiReceiveDatagram and receive datagrams initiated from a
// receive datagram event handler.
//
// Parameters
//   bReceiveDGOnEventComplete - TRUE if completing a receive datagram
//     initiated from a receive datagram event handler. FALSE if completing
//     a receive datagram initiated by calling TdiReceiveDatagram.
//
// Return Value
// In the PassThru sample there is no difference in the way the receive
// datagrams are completed.
// 
// Remarks
//

NTSTATUS
_I_TdiReceiveDGComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              Irp,
   void              *Context,
   BOOLEAN           bReceiveDGOnEventComplete
   )
{
   PTDIH_DeviceExtension   pTDIH_DeviceExtension;
   BOOLEAN                 CanDetachProceed = FALSE;
   PDEVICE_OBJECT          pAssociatedDeviceObject = NULL;
	NTSTATUS                Status = Irp->IoStatus.Status;
   ULONG                   nByteCount = Irp->IoStatus.Information;

#if DBG
//   KdPrint(( "_I_TdiReceiveDGComplete: Final Status: 0x%8.8X; Bytes Transfered: %d\n",
//      Status, nByteCount ));
#endif

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
      KdPrint(( "_I_TdiReceiveDGComplete: Invalid Device Object Pointer\n" ));
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
//// TDIH_TdiReceiveDGComplete
//
// Purpose
// Completion of a DG receive initiated by a call to TdiReceiveDatagram.
//
// Parameters
//
// Return Value
// 
// Remarks
// Calls common _I_TdiReceiveDGComplete function to do the work.
//

NTSTATUS
TDIH_TdiReceiveDGComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              Irp,
   void              *Context
   )
{
   return( _I_TdiReceiveDGComplete(
               pDeviceObject,
               Irp,
               Context,
               FALSE       // NOT In Event
               )
            );
}

/////////////////////////////////////////////////////////////////////////////
//// TDIH_TdiReceiveDGOnEventComplete
//
// Purpose
// Completion of a DG receive initiated from receive DG event handler.
//
// Parameters
//
// Return Value
// 
// Remarks
// Calls common _I_TdiReceiveDGComplete function to do the work.
//

NTSTATUS
TDIH_TdiReceiveDGOnEventComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              Irp,
   void              *Context
   )
{
   return( _I_TdiReceiveDGComplete(
               pDeviceObject,
               Irp,
               Context,
               TRUE      // Completion Of Receive DG From Event Handler
               )
            );
}

/////////////////////////////////////////////////////////////////////////////
//// TDIH_TdiReceiveDatagram
//
// Purpose
// This is the hook for TdiReceiveDatagram
//
// Parameters
//
// Return Value
// 
// Remarks
//

NTSTATUS
TDIH_TdiReceiveDatagram(
   PTDIH_DeviceExtension   pTDIH_DeviceExtension,
   PIRP                    Irp,
   PIO_STACK_LOCATION      IrpSp
   )
{
   NTSTATUS RC;

   KdPrint(( "TDIH_TdiReceiveDatagram: Entry...\n" ));

   try
   {
      PDEVICE_OBJECT       pLowerDeviceObject = NULL;

      pLowerDeviceObject = pTDIH_DeviceExtension->LowerDeviceObject;

      // Be careful about not screwing up badly. This is actually not recommended by the I/O Manager.
      if (Irp->CurrentLocation == 1)
      {
         ULONG ReturnedInformation = 0;

         // Bad!! Fudge the error code. Break if we can ...

         KdPrint(("TDIH_TdiReceiveDatagram encountered bogus current location\n"));

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
         TDIH_TdiReceiveDGComplete,
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



