/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include	"NDIS.H"
#include	"TDI.H"
#include	"TDIKRNL.H"
#include "./TDI/Sftk_TdiUtil.h"

// Copyright And Configuration Management ----------------------------------
//
//                  TDI Test TCP (TTCP) Utilities - KSUTIL.c
//
//                  PCAUSA TDI Client Samples For Windows NT
//
//      Copyright (c) 1999-2000 Printing Communications Associates, Inc.
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
//// NOTES
//
// The TDI_xxx utilities are general purpose, and are intended to be used
// and incorporated into your own TDI client driver.
//
//Modified By Veera, Added lot more functionlity and exposed the existing 
//functionality a lot.

NDIS_PHYSICAL_ADDRESS HighestAcceptableMax =
    NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);

static NTSTATUS
_I_TDI_SimpleTdiRequestComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
_I_TDI_RefTransportAddress(
   PTDI_ADDRESS             pTDI_Address
   );

VOID
_I_TDI_DerefTransportAddress(
   PTDI_ADDRESS             pTDI_Address
   );

NTSTATUS
_I_TDI_OpenConnectionContext(
   PWSTR          TransportDeviceNameW,// Zero-terminated String
   PTDI_ENDPOINT   pTDI_Endpoint,
   PVOID          pContext
   );

NTSTATUS
_I_TDI_CloseConnectionContext(
   PTDI_ENDPOINT   pTDI_Endpoint
   );

VOID
_I_TDI_RefConnectionEndpoint(
   PTDI_ENDPOINT   pTDI_Endpoint
   );

VOID
_I_TDI_DerefConnectionEndpoint(
   PTDI_ENDPOINT   pTDI_Endpoint
   );

NTSTATUS
_I_TDI_AssociateAddress(
   PTDI_ADDRESS    pTDI_Address,
   PTDI_ENDPOINT pTDI_Endpoint
   );

NTSTATUS
_I_TDI_DisassociateAddress(
   PTDI_ENDPOINT pTDI_Endpoint
   );


/////////////////////////////////////////////////////////////////////////////
//// STRUCTURES

typedef
struct _TDI_ADDRESS_REQUEST_CONTEXT
{
   PTDI_ADDRESS                      m_pTDI_Address;
   HANDLE                           m_CompletionEvent;
   PTDI_REQUEST_COMPLETION_ROUTINE   m_CompletionRoutine;
   PVOID                            m_CompletionContext;
   PIO_STATUS_BLOCK                 m_pIoStatusBlock;
   IO_STATUS_BLOCK                  m_SafeIoStatusBlock;
   ULONG                            m_Reserved;
}
   TDI_ADDRESS_REQUEST_CONTEXT, *PTDI_ADDRESS_REQUEST_CONTEXT;


typedef
struct _TDI_ENDPOINT_REQUEST_CONTEXT
{
   PTDI_ENDPOINT                     m_pTDI_Endpoint;
   HANDLE                           m_CompletionEvent;
   PTDI_REQUEST_COMPLETION_ROUTINE   m_CompletionRoutine;
   PVOID                            m_CompletionContext;
   PIO_STATUS_BLOCK                 m_pIoStatusBlock;
   IO_STATUS_BLOCK                  m_SafeIoStatusBlock;
   BOOLEAN                          m_bSynchronous;
   ULONG                            m_Reserved;
}
   TDI_ENDPOINT_REQUEST_CONTEXT, *PTDI_ENDPOINT_REQUEST_CONTEXT;


/////////////////////////////////////////////////////////////////////////////
//                    A D D R E S S  F U N C T I O N S                     //
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//// TDI_OpenTransportAddress
//
// Purpose
// Open a TDI address object on the specified transport device for the
// specified transport address.
//
// Parameters
//   TransportDeviceNameW
//      Pointer to a zero-terminated wide character string that specifies
//      the transport device. An example would be: L"\\Device\\Tcp"
//
//   pTransportAddress
//      Pointer to a TRANSPORT_ADDRESS structure (typically a TA_IP_ADDRESS
//      structure) that specifies the local address to open.
//
//   pTDI_Address
//      Pointer to a caller-provided TDI_ADDRESS structure that will be
//      initialized as the transport address object is opened.
//
// Return Value
// status
//
// Remarks
// See the NT DDK documentation topic "5.1 Opening a Transport Address"
// for more information.
//
// The TDI_BuildEaBuffer function is used to build the extended attributes
// buffer.
//
// It is important to note that the call to ZwCreateFile creates a
// client-process-specific file object that represents the transport's
// address object. This means that the m_hAddress handle is only valid
// in the same client process that the ZwCreateFile call was made in.
//
// Fortunately, most TDI operations actually reference the file object
// pointer - not the handle. Operations that do not reference the address
// handle can be performed in arbitrary context unless there are other
// restrictions.
//
// The client process dependency of the address handle does effect
// the design of a TDI Client to some extent. In particular, the
// client process that was used to create an address object must
// continue to exist until the address object is eventually closed.
//
// There are several ways to insure that the client process continues
// to exist while the address object is in use. The most common is to
// create a thread that is attached to the system process. As long as
// the system process continues to exist, the address object handle
// will remain valid.
//
// Callers of TDI_OpenTransportAddress must be running at IRQL PASSIVE_LEVEL.
//

NTSTATUS
TDI_OpenTransportAddress(
   IN PWSTR                TransportDeviceNameW,// Zero-terminated String
   IN PTRANSPORT_ADDRESS   pTransportAddress,   // Local Transport Address
   PTDI_ADDRESS             pTDI_Address
   )
{
   NTSTATUS          status = STATUS_SUCCESS;
   UNICODE_STRING    TransportDeviceName;
   OBJECT_ATTRIBUTES ObjectAttributes;
   IO_STATUS_BLOCK   IoStatusBlock;
   ULONG             TransportAddressLength;
   ULONG             TransportEaBufferLength;
   PFILE_FULL_EA_INFORMATION pTransportAddressEa;
   PDEVICE_OBJECT    pDeviceObject;

   ASSERT( pTDI_Address );

   //
   // Initialize TDI_ADDRESS Structure
   //
   pTDI_Address->m_nStatus = STATUS_UNSUCCESSFUL;
   pTDI_Address->m_pFileObject = NULL;
   pTDI_Address->m_pAtomicIrp = NULL;
   pTDI_Address->m_ReferenceCount = 1;

   InitializeListHead( &pTDI_Address->m_ConnectionList );

   TransportAddressLength = TDI_TransportAddressLength( pTransportAddress );

   //
   // Setup Transport Device Name
   //
   NdisInitUnicodeString( &TransportDeviceName, TransportDeviceNameW );

   //
   // Build an EA buffer for the specified transport address
   //
   status = TDI_BuildEaBuffer(
               TDI_TRANSPORT_ADDRESS_LENGTH,  // EaName Length
               TdiTransportAddress,           // EaName
               TransportAddressLength,        // EaValue Length
               pTransportAddress,             // EaValue
               &pTransportAddressEa,
               &TransportEaBufferLength
               );

   if( !NT_SUCCESS(status) )
   {
      return( pTDI_Address->m_nStatus = status );
   }

   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

   InitializeObjectAttributes(
      &ObjectAttributes,      // OBJECT_ATTRIBUTES instance
      &TransportDeviceName,   // Transport Device Name
      OBJ_CASE_INSENSITIVE,   // Attributes
      NULL,                   // RootDirectory
      NULL                    // SecurityDescriptor
      );

   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

   //
   // Call Transport To Create Transport Address Object
   //
   status = ZwCreateFile(
               &pTDI_Address->m_hAddress,                   // Handle
               GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, // Desired Access
               &ObjectAttributes,                          // Object Attributes
               &IoStatusBlock,                             // Final I/O status block
               0,                                          // Allocation Size
               FILE_ATTRIBUTE_NORMAL,                      // Normal attributes
               FILE_SHARE_READ,
//			   |FILE_SHARE_WRITE|FILE_SHARE_DELETE,                            // Sharing attributes
               FILE_OPEN_IF,                               // Create disposition
               0,                                          // CreateOptions
               pTransportAddressEa,                        // EA Buffer
               TransportEaBufferLength                     // EA length
               );

   //
   // Free up the EA buffer allocated.
   //
   ExFreePool( (PVOID )pTransportAddressEa );

   if (NT_SUCCESS(status))
   {
      //
      // Obtain a referenced pointer to the file object.
      //
      ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

      status = ObReferenceObjectByHandle(
                  pTDI_Address->m_hAddress,   // Object Handle
                  FILE_ANY_ACCESS,           // Desired Access
                  NULL,                      // Object Type
                  KernelMode,                // Processor mode
                  (PVOID *)&pTDI_Address->m_pFileObject,   // File Object pointer
                  NULL                       // Object Handle information
                  );

	  pDeviceObject = IoGetRelatedDeviceObject( pTDI_Address->m_pFileObject );

      //
      // Allocate IRP For General Purpose Use
      //
      pTDI_Address->m_pAtomicIrp = IoAllocateIrp(
                                    pDeviceObject->StackSize,
                                    FALSE
                                    );

      if( !pTDI_Address->m_pAtomicIrp )
      {
         TDI_CloseTransportAddress( pTDI_Address );

         return( pTDI_Address->m_nStatus = STATUS_INSUFFICIENT_RESOURCES );
      }
   }

   if (NT_SUCCESS(status))
   {
   }
   else
   {
   }

   return( pTDI_Address->m_nStatus = status );
}


/////////////////////////////////////////////////////////////////////////////
//// _I_TDI_RefTransportAddress (INTERNAL/PRIVATE)
//
// Purpose
// This routine increments the reference count on the transport address.
//
// Parameters
//   pTDI_Address
//      Pointer to the TDI_ADDRESS structure whose reference count should
//      be incremented.
//
// Return Value
// None.
//
// Remarks
//

VOID
_I_TDI_RefTransportAddress(
   PTDI_ADDRESS pTDI_Address
   )
{
   ASSERT( pTDI_Address );
   ASSERT( pTDI_Address->m_ReferenceCount > 0 );

   if( pTDI_Address )
   {
      InterlockedIncrement( &pTDI_Address->m_ReferenceCount );
   }
}


/////////////////////////////////////////////////////////////////////////////
//// _I_TDI_DestroyTransportAddress (INTERNAL/PRIVATE)
//
// Purpose
// This routine destroys the transport address object.
//
// Parameters
//   pTDI_Address
//      Pointer to the TDI_ADDRESS structure that specifies the transport
//      address object pointer and handle to be destroyed.
//
// Return Value
// status
//
// Remarks
// This function will free resources allocated in TDI_OpenTransportAddress,
// dereference the transport address file object pointer anc actually
// close the transport address file handle.
//
// Callers of _I_TDI_DestroyTransportAddress must be running at IRQL
// PASSIVE_LEVEL.
//

NTSTATUS
_I_TDI_DestroyTransportAddress(
   PTDI_ADDRESS pTDI_Address
   )
{
   NTSTATUS status = STATUS_SUCCESS;

   KdPrint(("TDI_DestroyTransportAddress: Entry...\n"));

   ASSERT( pTDI_Address );
   ASSERT( pTDI_Address->m_ReferenceCount == 0 );
   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
   ASSERT( IsListEmpty( &pTDI_Address->m_ConnectionList ) );

   if( !pTDI_Address )
   {
      return( STATUS_SUCCESS );
   }

   //
   // Free IRP Allocated For Receiveing
   //
   if( pTDI_Address->m_pAtomicIrp )
   {
      IoFreeIrp( pTDI_Address->m_pAtomicIrp );
   }

   pTDI_Address->m_pAtomicIrp = NULL;


   //
   // Free IRP Allocated For Sending
   //
   //
   // Dereference The File Object
   //
   if( pTDI_Address->m_pFileObject != NULL )
   {
      ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
      ObDereferenceObject( pTDI_Address->m_pFileObject );
   }

   pTDI_Address->m_pFileObject = NULL;

   // ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
   if (pTDI_Address->m_hAddress)
   {
		ZwClose( pTDI_Address->m_hAddress );
		pTDI_Address->m_hAddress = NULL;
   }

   return status;
}


/////////////////////////////////////////////////////////////////////////////
//// _I_TDI_DerefTransportAddress (INTERNAL/PRIVATE)
//
// Purpose
// This routine decrements the reference count on the transport address.
//
// Parameters
//   pTDI_Address
//      Pointer to the TDI_ADDRESS structure whose reference count should be
//      decremented.
//
// Return Value
// None.
//
// Remarks
// The TDI_ADDRESS structure will continue to exist until the reference count
// is deceremented to zero. When ther reference count is decremented to zero
// the _I_TDI_DestroyTransportAddress function is finally called to destroy
// the transport address object.
//

VOID
_I_TDI_DerefTransportAddress(
   PTDI_ADDRESS pTDI_Address
   )
{
   LONG  Result;

   ASSERT( pTDI_Address );

   if( pTDI_Address )
   {
      if( pTDI_Address->m_ReferenceCount ) // ATTENTION!!! Is this test OK on SMP???
      {
         Result = InterlockedDecrement( &pTDI_Address->m_ReferenceCount );

         if( !Result )
         {
            //
            // Destroy The Transport Address
            //
            _I_TDI_DestroyTransportAddress( pTDI_Address );
         }
      }
   }
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_CloseTransportAddress
//
// Purpose
// This routine closes the transport address object specified by pTDI_ADDRESS.
//
// Parameters
//   pTDI_Address
//      Pointer to the TDI_ADDRESS structure that specifies the transport
//      address object pointer and handle to be closed.
//
// Return Value
// status
//
// Remarks
// This function calls _I_TDI_DerefTransportAddress to decrement the reference
// count for the TDI_ADDRESS structure. If the reference count is decremented
// to zero by this call, the transport address object specified by pTDI_ADDRESS
// will actually be closed.
//
// Callers of TDI_TransportAddress must be running at IRQL PASSIVE_LEVEL.
//

NTSTATUS
TDI_CloseTransportAddress(
   PTDI_ADDRESS pTDI_Address
   )
{
   KdPrint(("TDI_CloseTransportAddress: Entry...\n"));

   ASSERT( pTDI_Address );

   if( pTDI_Address )
   {
      _I_TDI_DerefTransportAddress( pTDI_Address );
   }

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_SetEventHandlers
//
// Purpose
// Setup event handlers on the address object.
//
// Parameters
//   pTDI_Address
//      Pointer to the TDI_ADDRESS structure that specifies the transport
//      address object pointer and handle that the event handlers will
//      be set on.
//   pEventContext
//      Points to caller-determined context to be passed in to the ClientEventXxx
//      routine as TdiEventContext when it is called by the transport.
//   ConnectEventHandler
//      ConnectEventHandler is an event handler the TDI driver calls in response
//      to an incoming endpoint-to-endpoint connection offer from a remote node.
//   DisconnectEventHandler
//      DisconnectEventHandler is an event handler that the underlying TDI
//      transport calls in response to an incoming disconnection notification
//      from a remote node.
//   ErrorReceiveHandler
//      ErrorReceiveHandler is an event handler that the underlying TDI transport
//      calls in response to an error, either in the transport itself or in a
//      still lower network driver, that makes I/O on a particular local
//      transport address unreliable or impossible.
//   ReceiveEventHandler
//      ReceiveEventHandler is an event handler that the underlying TDI transport
//      calls in response to an incoming receive from a remote node with which
//      the client has an established endpoint-to-endpoint connection. Usually,
//      this is a normal TSDU unless the client has not registered a
//      ReceiveExpeditedEventHandler handler.
//   ReceiveDatagramEventHandler
//      ReceiveDatagramEventHandler is an event handler that the underlying TDI
//      transport calls in response to an incoming receive from a remote node
//      that was directed to a local-node transport address that the client has
//      opened.
//   ReceiveExpeditedEventHandler
//      ReceiveExpeditedEventHandler is an event handler that the underlying
//      TDI transport calls in response to an incoming expedited receive from a
//      remote node with which the client has an established endpoint-to-endpoint
//      connection.
//
// Return Value
// status
//
// Remarks
// Understand that TDI event handlers are setup on a transport address, not a
// connection endpoint.
//
// In general terms, "context" is simply some piece of information that you
// provide in one place and is given back to you when you need it most. For example,
// you pass a pEventContext value to TDI_SetEventHandlers; this value is simply
// given back to you as TdiEventContext when your event handler is called.
//
// Often a context value is a pointer to a data structure that you have
// defined that contains the information that you will need when the event
// handler of callback is called. It doesn't have to be a pointer, however; it
// could be an index number into a table that you maintain, or anything else that
// will help you in your callback.
//
// See the NT DDK documentation topic "5.1 Opening a Transport Address"
// for more information.
//
// Callers of TDI_SetEventHandlers must be running at IRQL PASSIVE_LEVEL.
//

NTSTATUS
TDI_SetEventHandlers(
   PTDI_ADDRESS                pTDI_Address,
   PVOID                      pEventContext,
   PTDI_IND_CONNECT           ConnectEventHandler,
   PTDI_IND_DISCONNECT        DisconnectEventHandler,
   PTDI_IND_ERROR             ErrorEventHandler,
   PTDI_IND_RECEIVE           ReceiveEventHandler,
   PTDI_IND_RECEIVE_DATAGRAM  ReceiveDatagramEventHandler,
   PTDI_IND_RECEIVE_EXPEDITED ReceiveExpeditedEventHandler
   )
{
   NTSTATUS       status;
   PDEVICE_OBJECT pDeviceObject;

   KdPrint(("TDI_SetEventHandlers: Entry...\n") );

   pDeviceObject = IoGetRelatedDeviceObject( pTDI_Address->m_pFileObject );

   //
   // Set The Specified Event Handlers
   //
   do
   {
      //
      // Set The Connect Event Handler
      //
      TdiBuildSetEventHandler(
         pTDI_Address->m_pAtomicIrp,
         pDeviceObject,
         pTDI_Address->m_pFileObject,
         NULL,
         NULL,
         TDI_EVENT_CONNECT,
         ConnectEventHandler,
         pEventContext
         );

      //
      // Submit The Request To The Transport
      //
      status = TDI_MakeSimpleTdiRequest(
                  pDeviceObject,
                  pTDI_Address->m_pAtomicIrp
                  );

      if (!NT_SUCCESS(status))
      {
         break;
      }

      //
      // Set The Disconnect Event Handler
      //
      TdiBuildSetEventHandler(
         pTDI_Address->m_pAtomicIrp,
         pDeviceObject,
         pTDI_Address->m_pFileObject,
         NULL,
         NULL,
         TDI_EVENT_DISCONNECT,
         DisconnectEventHandler,
         pEventContext
         );

      //
      // Submit The Request To The Transport
      //
      status = TDI_MakeSimpleTdiRequest(
                  pDeviceObject,
                  pTDI_Address->m_pAtomicIrp
                  );

      if (!NT_SUCCESS(status))
      {
         break;
      }

      //
      // Set The Error Event Handler
      //
      TdiBuildSetEventHandler(
         pTDI_Address->m_pAtomicIrp,
         pDeviceObject,
         pTDI_Address->m_pFileObject,
         NULL,
         NULL,
         TDI_EVENT_ERROR,
         ErrorEventHandler,
         pEventContext
         );

      //
      // Submit The Request To The Transport
      //
      status = TDI_MakeSimpleTdiRequest(
                  pDeviceObject,
                  pTDI_Address->m_pAtomicIrp
                  );

      if (!NT_SUCCESS(status))
      {
         break;
      }

      //
      // Set The Receive Event Handler
      //
      TdiBuildSetEventHandler(
         pTDI_Address->m_pAtomicIrp,
         pDeviceObject,
         pTDI_Address->m_pFileObject,
         NULL,
         NULL,
         TDI_EVENT_RECEIVE,
         ReceiveEventHandler,
         pEventContext
         );

      //
      // Submit The Request To The Transport
      //
      status = TDI_MakeSimpleTdiRequest(
                  pDeviceObject,
                  pTDI_Address->m_pAtomicIrp
                  );

      if (!NT_SUCCESS(status))
      {
         break;
      }

      //
      // Set The Receive Datagram Event Handler
      //
      TdiBuildSetEventHandler(
         pTDI_Address->m_pAtomicIrp,
         pDeviceObject,
         pTDI_Address->m_pFileObject,
         NULL,
         NULL,
         TDI_EVENT_RECEIVE_DATAGRAM,
         ReceiveDatagramEventHandler,
         pEventContext
         );

      //
      // Submit The Request To The Transport
      //
      status = TDI_MakeSimpleTdiRequest(
                  pDeviceObject,
                  pTDI_Address->m_pAtomicIrp
                  );

      if (!NT_SUCCESS(status))
      {
         break;
      }

      //
      // Set The Receive Expedited Event Handler
      //
      TdiBuildSetEventHandler(
         pTDI_Address->m_pAtomicIrp,
         pDeviceObject,
         pTDI_Address->m_pFileObject,
         NULL,
         NULL,
         TDI_EVENT_RECEIVE_EXPEDITED,
         ReceiveExpeditedEventHandler,
         pEventContext
         );

      //
      // Submit The Request To The Transport
      //
      status = TDI_MakeSimpleTdiRequest(
                  pDeviceObject,
                  pTDI_Address->m_pAtomicIrp
                  );

      if (!NT_SUCCESS(status))
      {
         break;
      }

#ifdef ZNEVER // Not Currently Supported By TDI
      //
      // Set The Send Possible Event Handler
      //
      TdiBuildSetEventHandler(
         pTDI_Address->m_pAtomicIrp,
         pDeviceObject,
         pTDI_Address->m_pFileObject,
         NULL,
         NULL,
         TDI_EVENT_SEND_POSSIBLE,
         SendPossibleEventHandler,
         pEventContext
         );

      //
      // Submit The Request To The Transport
      //
      status = TDI_MakeSimpleTdiRequest(
                  pDeviceObject,
                  pTDI_Address->m_pAtomicIrp
                  );

      if (!NT_SUCCESS(status))
      {
         break;
      }
#endif // ZNEVER

      //
      // All Event Handlers Have Been Set
      //
   }
      while(0);   

   return( status );
}

/////////////////////////////////////////////////////////////////////////////
//// _I_TDI_AddressRequestComplete (INTERNAL/PRIVATE)
//
// Purpose
//
// Parameters
//
// Return Value
//
// Remarks
//

static NTSTATUS
_I_TDI_AddressRequestComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PVOID Context
    )
{
   PTDI_ADDRESS_REQUEST_CONTEXT   pTDI_RequestContext;
   PKEVENT                       pEvent;

   pTDI_RequestContext = (PTDI_ADDRESS_REQUEST_CONTEXT )Context;

   if( pTDI_RequestContext != NULL )
   {
      //
      // Fill IoStatusBlock With Final status And Information
      //
      (pTDI_RequestContext->m_pIoStatusBlock)->Status = pIrp->IoStatus.Status;
      (pTDI_RequestContext->m_pIoStatusBlock)->Information = pIrp->IoStatus.Information;

      //
      // Set The Completion Event, If Specified
      //
      if( pTDI_RequestContext->m_CompletionEvent )
      {
         KeSetEvent( pTDI_RequestContext->m_CompletionEvent, 0, FALSE);
      }

      //
      // Call The Completion Routine, If Specified
      //
      if( pTDI_RequestContext->m_CompletionRoutine )
      {
         (pTDI_RequestContext->m_CompletionRoutine)(
               pTDI_RequestContext->m_CompletionContext,
               pTDI_RequestContext->m_pIoStatusBlock,
               pTDI_RequestContext->m_Reserved
               );
      }

      //
      // Dereference The Address Object
      //
      _I_TDI_DerefTransportAddress( pTDI_RequestContext->m_pTDI_Address );

      //
      // Free Memory Allocated For Private Completion Context
      //
      NdisFreeMemory(
         pTDI_RequestContext,
         sizeof( TDI_ADDRESS_REQUEST_CONTEXT ),
         0
         );
   }

   IoFreeIrp( pIrp );

   return STATUS_MORE_PROCESSING_REQUIRED;
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_SendDatagramOnAddress
//
// Purpose
// Send a chain of MDLs as a datagram on the transport address object.
//
// Parameters
//    pSendMdl
//       Pointer to the first Memory Descriptor List (MDL) of the datagram
//       to be sent.
//
//    pTDI_Address
//       Pointer to the TDI_ADDRESS structure that specifies the transport
//       address object that the datagram will be sent on.
//
//    pIoStatusBlock
//       Pointer to a caller provided IO_STATUS_BLOCK structure that received
//       final completion status and sent byte count.
//
//    UserCompletionRoutine
//       Specifies the entry point for the caller-supplied completion routine
//       to be called when the lower-level TDI driver completes the operation.
//       This routine is declared as follows:
//
//          VOID
//          (*PTDI_REQUEST_COMPLETION_ROUTINE) (
//             PVOID UserCompletionContext,
//             PIO_STATUS_BLOCK IoStatusBlock,
//             ULONG Reserved
//             );//
//
//    UserCompletionContext
//        The value to be passed to UserCompletionRoutine.
//
// Remarks
// This function sends a chain of MDLs that represent the datagram to be sent.
// The number of bytes to be sent is specified is the sum of the MDL byte
// counts for all MDLs in the chain. Having the Next field set to NULL
// identifies the last MDL in the chain. The byte count of each MDL in the
// chain is queried using MmGetMdlByteCount.
//
// Each of the MDLs in the chain must have been probed and locked before
// calling TDI_SendDatagramOnAddress.
//
// Synchronous/Asynchronous Operation
// ----------------------------------
// Synchronous Send
//    If UserCompletionEvent and UserCompletionRoutine are both NULL the send will
//    be performed synchronously.
//
// Asynchronous Send (Event Wait)
//    If a UserCompletionEvent is specified then the send will be performed
//    asynchronously. The framework will set the specified event when the
//    send is completed.
//
//    If UserCompletionEvent is specified, then UserCompletionRoutine and
//    UserCompletionContext are ignored.
//
// Asynchronous Send (Completion Routine)
//    If a UserCompletionEvent is NOT specified (i.e., it is NULL) and a
//    UserCompletionRoutine is specified, then the send will be performed
//    asynchronously. The framework will call the specified completion
//    routine when the send is completed and pass UserCompletionContext
//    as the context.
//
// In this implementation it is an error to specify both a UserCompletionEvent
// and a UserCompletionRoutine. Only one of these two methods can be used.
//
// If the synchronous mode of operation is employed, then the caller of
// TDI_SendDatagramOnAddress must be running at IRQL PASSIVE_LEVEL. If the
// caller intends to wait on the event specified by UserCompletionEvent,
// then the waiting must be performed at IRQL PASSIVE_LEVEL.
//
// For all methods the IO_STATUS_BLOCK pointed to by pIoStatusBlock will
// receive the final completion status and information about the requested
// send operation.
//

NTSTATUS
TDI_SendDatagramOnAddress(
   PTDI_ADDRESS                      pTDI_Address,
   HANDLE                           UserCompletionEvent,    // Optional
   PTDI_REQUEST_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                            UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK                 pIoStatusBlock,         // Required
   PMDL                             pSendMdl,
   PTDI_CONNECTION_INFORMATION      pSendDatagramInfo
   )
{
   NTSTATUS                      status;
   PIRP                          pSendIrp;
   ULONG                         Length;
   KEVENT                        SyncEvent;
   BOOLEAN                       bSynchronous = FALSE;
   PTDI_ADDRESS_REQUEST_CONTEXT   pTDI_RequestContext = NULL;
   PDEVICE_OBJECT                pDeviceObject;
   PMDL                          pNextMdl;

   ASSERT( pTDI_Address );

   //
   // Allocate Memory For Private Completion Context
   //
   status = NdisAllocateMemory(
                  &pTDI_RequestContext,
                  sizeof( TDI_ADDRESS_REQUEST_CONTEXT ),
                  0,       // Allocate non-paged system-space memory
                  HighestAcceptableMax
                  );

   if( !NT_SUCCESS( status ) )
   {
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   RtlZeroMemory( pTDI_RequestContext, sizeof( TDI_ADDRESS_REQUEST_CONTEXT ) );

   if( pIoStatusBlock )
   {
      pTDI_RequestContext->m_pIoStatusBlock = pIoStatusBlock;
   }
   else
   {
      pTDI_RequestContext->m_pIoStatusBlock = &pTDI_RequestContext->m_SafeIoStatusBlock;
   }

   pTDI_RequestContext->m_pTDI_Address = pTDI_Address;

   if( UserCompletionRoutine )
   {
      pTDI_RequestContext->m_CompletionEvent = UserCompletionEvent;
      pTDI_RequestContext->m_CompletionRoutine = UserCompletionRoutine;
      pTDI_RequestContext->m_CompletionContext = UserCompletionContext;

      bSynchronous = FALSE;
   }
   else if( UserCompletionEvent )
   {
      pTDI_RequestContext->m_CompletionEvent = UserCompletionEvent;
      KeResetEvent( pTDI_RequestContext->m_CompletionEvent );

      pTDI_RequestContext->m_CompletionRoutine = NULL;
      pTDI_RequestContext->m_CompletionContext = NULL;

      bSynchronous = FALSE;
   }
   else
   {
      pTDI_RequestContext->m_CompletionEvent = &SyncEvent;

      ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

      KeInitializeEvent(
         pTDI_RequestContext->m_CompletionEvent,
         NotificationEvent,
         FALSE
         );

      pTDI_RequestContext->m_CompletionRoutine = NULL;
      pTDI_RequestContext->m_CompletionContext = NULL;

      bSynchronous = TRUE;
   }

   pDeviceObject = IoGetRelatedDeviceObject( pTDI_Address->m_pFileObject );

   //
   // Allocate IRP For Sending
   //
   pSendIrp = IoAllocateIrp( pDeviceObject->StackSize, FALSE );

   if( !pSendIrp )
   {
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   Length = 0;
   pNextMdl = pSendMdl;

   while( pNextMdl )
   {
      Length += MmGetMdlByteCount( pNextMdl );
      pNextMdl = pNextMdl->Next;
   }

   TdiBuildSendDatagram(
      pSendIrp,
      pDeviceObject,
      pTDI_Address->m_pFileObject,
      _I_TDI_AddressRequestComplete, // Completion routine
      pTDI_RequestContext,           // Completion context
      pSendMdl,                     // the data buffer
      Length,                       // send buffer length
      pSendDatagramInfo
      );

   //
   // Reference The Address Object
   //
   _I_TDI_RefTransportAddress( pTDI_RequestContext->m_pTDI_Address );

   //
   // Submit the request
   //
   (pTDI_RequestContext->m_pIoStatusBlock)->Status = STATUS_UNSUCCESSFUL;
   (pTDI_RequestContext->m_pIoStatusBlock)->Information = 0;

   status = IoCallDriver( pDeviceObject, pSendIrp );

   if( bSynchronous )
   {
      if( (status == STATUS_SUCCESS) || (status == STATUS_PENDING) )
      {
         ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

         status = KeWaitForSingleObject(
                        &SyncEvent,  // Object to wait on.
                        Executive,  // Reason for waiting
                        KernelMode, // Processor mode
                        FALSE,      // Alertable
                        NULL        // Timeout
                        );

         if( NT_SUCCESS( status ) )
         {
            status = pSendIrp->IoStatus.Status;
         }
      }
   }

   if( !NT_SUCCESS( status ) )
   {
      //
      // Handle Case Where Completion Function Will Not Be Called...
      //

      //
      // Dereference The Address Object
      //
      _I_TDI_DerefTransportAddress( pTDI_RequestContext->m_pTDI_Address );

      //
      // Free Memory Allocated For Private Completion Context
      //
      NdisFreeMemory(
         pTDI_RequestContext,
         sizeof( TDI_ADDRESS_REQUEST_CONTEXT ),
         0
         );

      IoFreeIrp( pSendIrp );
   }

   return( status );
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_ReceiveDatagramOnAddress
//
// Purpose
// Requests the underlying TDI transport to indicate a received datagram on
// the specified transport address.
//
// Parameters
//    pReceiveMdl
//       Pointer to the first Memory Descriptor List (MDL), in a possible
//       chain of MDLs, that will be filled with the received datagram.
//
//    pTDI_Address
//       Pointer to the TDI_ADDRESS structure that specifies the transport
//       address object that the datagram will be received on.
//
//    pIoStatusBlock
//       Pointer to a caller provided IO_STATUS_BLOCK structure that received
//       final completion status and received byte count.
//
//    UserCompletionRoutine
//       Specifies the entry point for the caller-supplied completion routine
//       to be called when the lower-level TDI driver completes the operation.
//       This routine is declared as follows:
//
//          VOID
//          (*PTDI_REQUEST_COMPLETION_ROUTINE) (
//             PVOID UserCompletionContext,
//             PIO_STATUS_BLOCK IoStatusBlock,
//             ULONG Reserved
//             );//
//
//    UserCompletionContext
//        The value to be passed to UserCompletionRoutine.
//
// Remarks
// This function passes a chain of MDLs to the underlying TDI transport.
// The TDI transport will fill the memory wrapped by these MDLs with a datagram
// received on the specified transport address. The maximum datagram size that
// can be received is the sum of the MDL byte counts for all MDLs in the chain.
// Having the Next field set to NULL identifies the last MDL in the chain. The
// byte count of each MDL in the chain is queried using MmGetMdlByteCount.
//
// Each of the MDLs in the chain must have been probed and locked before calling
// TDI_ReceiveDatagramOnAddress.
//
// Synchronous/Asynchronous Operation
// ----------------------------------
// Synchronous Receive
//    If UserCompletionEvent and UserCompletionRoutine are both NULL the receive
//    will be performed synchronously.
//
// Asynchronous Receive (Event Wait)
//    If a UserCompletionEvent is specified then the receive will be performed
//    asynchronously. The framework will set the specified event when the
//    datagram is received.
//
//    If UserCompletionEvent is specified, then UserCompletionRoutine and
//    UserCompletionContext are ignored.
//
// Asynchronous Receive (Completion Routine)
//    If a UserCompletionEvent is NOT specified (i.e., it is NULL) and a
//    UserCompletionRoutine is specified, then the receive will be performed
//    asynchronously. The framework will call the specified completion
//    routine when the datagram is received and pass UserCompletionContext
//    as the context.
//
// In this implementation it is an error to specify both a UserCompletionEvent
// and a UserCompletionRoutine. Only one of these two methods can be used.
//
// If the synchronous mode of operation is employed, then the caller of
// TDI_ReceiveDatagramOnAddress must be running at IRQL PASSIVE_LEVEL. If the
// caller intends to wait on the event specified by UserCompletionEvent,
// then the waiting must be performed at IRQL PASSIVE_LEVEL.
//
// For all methods the IO_STATUS_BLOCK pointed to by pIoStatusBlock will
// receive the final completion status and information about the requested
// send operation.
//

NTSTATUS
TDI_ReceiveDatagramOnAddress(
   PTDI_ADDRESS                      pTDI_Address,
   HANDLE                           UserCompletionEvent,    // Optional
   PTDI_REQUEST_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                            UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK                 pIoStatusBlock,         // Required
   PMDL                             pReceiveMdl,
   PTDI_CONNECTION_INFORMATION      pReceiveDatagramInfo,
   PTDI_CONNECTION_INFORMATION      pReturnInfo,
   ULONG                            InFlags
   )
{
   NTSTATUS                      status;
   PIRP                          pReceiveIrp;
   ULONG                         Length;
   KEVENT                        SyncEvent;
   BOOLEAN                       bSynchronous = FALSE;
   PTDI_ADDRESS_REQUEST_CONTEXT   pTDI_RequestContext = NULL;
   PDEVICE_OBJECT                pDeviceObject;
   PMDL                          pNextMdl;

   ASSERT( pTDI_Address );

   //
   // Allocate Memory For Private Completion Context
   //
   status = NdisAllocateMemory(
                  &pTDI_RequestContext,
                  sizeof( TDI_ADDRESS_REQUEST_CONTEXT ),
                  0,       // Allocate non-paged system-space memory
                  HighestAcceptableMax
                  );

   if( !NT_SUCCESS( status ) )
   {
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   RtlZeroMemory( pTDI_RequestContext, sizeof( TDI_ADDRESS_REQUEST_CONTEXT ) );

   if( pIoStatusBlock )
   {
      pTDI_RequestContext->m_pIoStatusBlock = pIoStatusBlock;
   }
   else
   {
      pTDI_RequestContext->m_pIoStatusBlock = &pTDI_RequestContext->m_SafeIoStatusBlock;
   }

   pTDI_RequestContext->m_pTDI_Address = pTDI_Address;

   if( UserCompletionRoutine )
   {
      pTDI_RequestContext->m_CompletionEvent = UserCompletionEvent;
      pTDI_RequestContext->m_CompletionRoutine = UserCompletionRoutine;
      pTDI_RequestContext->m_CompletionContext = UserCompletionContext;

      bSynchronous = FALSE;
   }
   else if( UserCompletionEvent )
   {
      pTDI_RequestContext->m_CompletionEvent = UserCompletionEvent;
      KeResetEvent( pTDI_RequestContext->m_CompletionEvent );

      pTDI_RequestContext->m_CompletionRoutine = NULL;
      pTDI_RequestContext->m_CompletionContext = NULL;

      bSynchronous = FALSE;
   }
   else
   {
      pTDI_RequestContext->m_CompletionEvent = &SyncEvent;

      ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

      KeInitializeEvent(
         pTDI_RequestContext->m_CompletionEvent,
         NotificationEvent,
         FALSE
         );

      pTDI_RequestContext->m_CompletionRoutine = NULL;
      pTDI_RequestContext->m_CompletionContext = NULL;

      bSynchronous = TRUE;
   }

   pDeviceObject = IoGetRelatedDeviceObject( pTDI_Address->m_pFileObject );

   //
   // Allocate IRP For Receiving
   //
   pReceiveIrp = IoAllocateIrp( pDeviceObject->StackSize, FALSE );

   if( !pReceiveIrp )
   {
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   Length = 0;
   pNextMdl = pReceiveMdl;

   while( pNextMdl )
   {
      Length += MmGetMdlByteCount( pNextMdl );
      pNextMdl = pNextMdl->Next;
   }

   TdiBuildReceiveDatagram(
      pReceiveIrp,
      pDeviceObject,
      pTDI_Address->m_pFileObject,
      _I_TDI_AddressRequestComplete,   // Completion routine
      pTDI_RequestContext,   // Completion context
      pReceiveMdl,                  // the data buffer
      Length,                       // send buffer length
      pReceiveDatagramInfo,
      pReturnInfo,
      InFlags
      );

   //
   // Reference The Address Object
   //
   _I_TDI_RefTransportAddress( pTDI_RequestContext->m_pTDI_Address );

   //
   // Submit the request
   //
   (pTDI_RequestContext->m_pIoStatusBlock)->Status = STATUS_UNSUCCESSFUL;
   (pTDI_RequestContext->m_pIoStatusBlock)->Information = 0;

   status = IoCallDriver( pDeviceObject, pReceiveIrp );

   if( bSynchronous )
   {
      if( (status == STATUS_SUCCESS) || (status == STATUS_PENDING) )
      {
         ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

         status = KeWaitForSingleObject(
                        &SyncEvent,  // Object to wait on.
                        Executive,  // Reason for waiting
                        KernelMode, // Processor mode
                        FALSE,      // Alertable
                        NULL        // Timeout
                        );

         if( NT_SUCCESS( status ) )
         {
            status = pReceiveIrp->IoStatus.Status;
         }
      }
   }

   if( !NT_SUCCESS( status ) )
   {
      //
      // Handle Case Where Completion Function Will Not Be Called...
      //

      //
      // Dereference The Address Object
      //
      _I_TDI_DerefTransportAddress( pTDI_RequestContext->m_pTDI_Address );

      //
      // Free Memory Allocated For Private Completion Context
      //
      NdisFreeMemory(
         pTDI_RequestContext,
         sizeof( TDI_ADDRESS_REQUEST_CONTEXT ),
         0
         );

      IoFreeIrp( pReceiveIrp );
   }

   return( status );
}

/////////////////////////////////////////////////////////////////////////////
//// TDI_QueryAddressInfo
//
// Purpose
// TDI_QueryAddressInfo makes a TDI_QUERY_INFORMATION request for
// TDI_QUERY_ADDRESS_INFO for the transport address or connection endpoint
// specified by its file object pointer.
//
// Parameters
//   pFileObject
//      Can be either a TDI transport address file object (from TDI_ADDRESS)
//      or TDI connection endpoint file object (from TDI_ENDPOINT).
//
//    pInfoBuffer
//        Must point to a buffer sufficiently large to hold a TDI_ADDRESS_INFO
//        structure for a TDI_ADDRESS_IP.
//
//    pInfoBufferSize
//       Pointer to a ULONG. On entry, the ULONG pointed to by pInfoBufferSize
//       must be initialized with the size of the buffer at pInfoBuffer. On
//       return, the ULONG pointed to by pInfoBufferSize will be updated to
//       indicate the length of the data written to pInfoBuffer.
//
// Return Value
//
// Remarks
// This function is roughly the TDI equivalent of the Winsock getsockname
// function.
//
// Callers of TDI_QueryAddressInfo must be running at IRQL PASSIVE_LEVEL.
//
// The information returned from TDI_QUERY_ADDRESS_INFO depends on the
// type of the file object:
//
//   Transport Address File Object:
//     in_addr: 0.0.0.0; sin_port: 1042 (0x412)
//
//   Connection Endpoint File Object:
//     in_addr: 172.16.1.130; sin_port: 1042 (0x412)
//
// On reflection, this makes sense. An address object has a unique port
// number, but can be used on multiple host IP addresses on a multihomed
// host. Hence, the IP address is zero.
//
// However, once a connection is established the network interface is
// also established. Hence, the IP address on a connected connection
// endpoint is non-zero.
//

NTSTATUS
TDI_QueryAddressInfo(
   PFILE_OBJECT      pFileObject,
   PVOID             pInfoBuffer,
   PULONG            pInfoBufferSize
   )
{
   NTSTATUS          status = STATUS_UNSUCCESSFUL;
   IO_STATUS_BLOCK   IoStatusBlock;
   PIRP              pIrp = NULL;
   PMDL              pMdl = NULL;
   PDEVICE_OBJECT    pDeviceObject;

   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

   pDeviceObject = IoGetRelatedDeviceObject( pFileObject );

   RtlZeroMemory( pInfoBuffer, *pInfoBufferSize );

   //
   // Allocate IRP For The Query
   //
   pIrp = IoAllocateIrp( pDeviceObject->StackSize, FALSE );

   if( !pIrp )
   {
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   pMdl = TDI_AllocateAndProbeMdl(
            pInfoBuffer,      // Virtual address for MDL construction
            *pInfoBufferSize,  // size of the buffer
            FALSE,
            FALSE,
            NULL
            );

   if( !pMdl )
   {
      IoFreeIrp( pIrp );
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   TdiBuildQueryInformation(
      pIrp,
      pDeviceObject,
      pFileObject,
      _I_TDI_SimpleTdiRequestComplete,  // Completion routine
      NULL,                            // Completion context
      TDI_QUERY_ADDRESS_INFO,
      pMdl
      );

   //
   // Submit The Query To The Transport
   //
   status = TDI_MakeSimpleTdiRequest(
               pDeviceObject,
               pIrp
               );

   //
   // Free Allocated Resources
   //
   TDI_UnlockAndFreeMdl(pMdl);

   *pInfoBufferSize = pIrp->IoStatus.Information;

   IoFreeIrp( pIrp );

   return( status );
}

/////////////////////////////////////////////////////////////////////////////
//                    E N D P O I N T  F U N C T I O N S                   //
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//// _I_TDI_OpenConnectionContext (INTERNAL/PRIVATE)
//
// Purpose
// Open a TDI connection context object on the specified transport
// device.
//
// Parameters
//   TransportDeviceNameW
//      Pointer to a zero-terminated wide character string that specifies
//      the transport device. An example would be: L"\\Device\\Tcp"
//
//
// Return Value
// status
//
// Remarks
// See the NT DDK documentation topic "5.2 Opening a Connection Endpoint"
// for more information.
//
// It is important to note that the call to ZwCreateFile creates a
// client-process-specific file object that represents the connection
// endpoint object. This means that the m_hContext handle is only valid
// in the same client process that the ZwCreateFile call was made in.
//
// Fortunately, most TDI operations actually reference the file object
// pointer - not the handle. Operations that do not reference the connection
// context handle can be performed in arbitrary context unless there are
// other restrictions.
//
// The client process dependency of the connection context handle does effect
// the design of a TDI Client to some extent. In particular, the the client
// process that was used to create a connection contect object must
// continue to exist until the connection context object is eventually closed.
//
// There are several ways to insure that the client process continues
// to exist while the connection context object is in use. The most common
// is to create a thread that is attached to the system process. As long as
// the system process continues to exist, the connection context object
// handle will remain valid.
//
// Callers of _I_TDI_OpenConnectionContext must be running at IRQL PASSIVE_LEVEL.
//

NTSTATUS
_I_TDI_OpenConnectionContext(
   IN PWSTR       TransportDeviceNameW,// Zero-terminated String
   PTDI_ENDPOINT   pTDI_Endpoint,
   PVOID          pContext
  )
{
   NTSTATUS          status = STATUS_SUCCESS;
   UNICODE_STRING    TransportDeviceName;
   OBJECT_ATTRIBUTES ObjectAttributes;
   IO_STATUS_BLOCK   IoStatusBlock;
   ULONG             TransportEaBufferLength;
   PFILE_FULL_EA_INFORMATION pConnectionContextEa;

   //
   // Initialize Context-Related Fields
   //
   pTDI_Endpoint->m_nOpenStatus = STATUS_UNSUCCESSFUL;
   pTDI_Endpoint->m_pFileObject = NULL;

   //
   // Setup Transport Device Name
   //
   NdisInitUnicodeString( &TransportDeviceName, TransportDeviceNameW );

   //
   // Build An EA Buffer For The Connection Context
   // ---------------------------------------------
   // The EaValue will be the ConnectionContext passed to various
   // TDI EventHandler routines.
   //
   status = TDI_BuildEaBuffer(
               TDI_CONNECTION_CONTEXT_LENGTH,   // EaName Length
               TdiConnectionContext,            // EaName
               sizeof( PVOID ),                 // EaValue Length
               &pContext,                       // EaValue
               &pConnectionContextEa,
               &TransportEaBufferLength
               );

   if( !NT_SUCCESS(status) )
   {
      return( pTDI_Endpoint->m_nOpenStatus = status );
   }

   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

   InitializeObjectAttributes(
      &ObjectAttributes,      // OBJECT_ATTRIBUTES instance
      &TransportDeviceName,   // Transport Device Name
      OBJ_CASE_INSENSITIVE,   // Attributes
      NULL,                   // RootDirectory
      NULL                    // SecurityDescriptor
      );

   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

   //
   // Call Transport To Create Connection Endpoint Object
   //
   status = ZwCreateFile(
               &pTDI_Endpoint->m_hContext,                // Handle
               GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, // Desired Access
               &ObjectAttributes,                          // Object Attributes
               &IoStatusBlock,                             // Final I/O status block
               0,                                          // Allocation Size
               FILE_ATTRIBUTE_NORMAL,                      // Normal attributes
               FILE_SHARE_READ,                            // Sharing attributes
               FILE_OPEN_IF,                               // Create disposition
               0,                                          // CreateOptions
               pConnectionContextEa,                       // EA Buffer
               TransportEaBufferLength                     // EA length
               );

   if (NT_SUCCESS(status))
   {
      //
      // Obtain a referenced pointer to the file object.
      //
      ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

      status = ObReferenceObjectByHandle(
                  pTDI_Endpoint->m_hContext,// Object Handle
                  FILE_ANY_ACCESS,           // Desired Access
                  NULL,                      // Object Type
                  KernelMode,                // Processor mode
                  (PVOID *)&pTDI_Endpoint->m_pFileObject, // File Object pointer
                  NULL                       // Object Handle information
                  );

	  if (pTDI_Endpoint->m_hContext)
	  {
//		  ZwClose(pTDI_Endpoint->m_hContext); // we got reference so no need to keep it open anymore. close it here else blue screen comes
//		  pTDI_Endpoint->m_hContext = NULL;
	  }

   }

   //
   // Free up the EA buffer allocated.
   //
   ExFreePool( (PVOID )pConnectionContextEa );

   return( pTDI_Endpoint->m_nOpenStatus = status );
}


/////////////////////////////////////////////////////////////////////////////
//// _I_TDI_CloseConnectionContext (INTERNAL/PRIVATE)
//
// Purpose
// This routine closes the connection context (endpoint) object.
//
// Parameters
//
// Return Value
// status
//
// Remarks
// Callers of _I_TDI_CloseConnectionContext must be running at IRQL PASSIVE_LEVEL.
//

NTSTATUS
_I_TDI_CloseConnectionContext(
   PTDI_ENDPOINT   pTDI_Endpoint
   )
{
   NTSTATUS status = STATUS_SUCCESS;

   KdPrint(("_I_TDI_CloseConnectionContext: Entry...\n"));

   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

   //
   // Dereference The File Object
   //
   if( pTDI_Endpoint->m_pFileObject != NULL )
   {
      ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
      ObDereferenceObject( pTDI_Endpoint->m_pFileObject );

	  if (pTDI_Endpoint->m_hContext != NULL)
	  {
		ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
		ZwClose( pTDI_Endpoint->m_hContext );
		pTDI_Endpoint->m_hContext = NULL;
	  }
   }

   pTDI_Endpoint->m_pFileObject = NULL;

   return status;
}


/////////////////////////////////////////////////////////////////////////////
//// _I_TDI_AssociateAddress (INTERNAL/PRIVATE)
//
// Purpose
//
// Parameters
//   pTDI_Address
//      Pointer to the TDI_ADDRESS structure that specifies the transport
//      address object to associated with a connection endpoint object.
//
// Return Value
// status
//
// Remarks
//

NTSTATUS
_I_TDI_AssociateAddress(
   PTDI_ADDRESS    pTDI_Address,
   PTDI_ENDPOINT   pTDI_Endpoint
   )
{
   PDEVICE_OBJECT    pDeviceObject;

   pTDI_Endpoint->m_pTDI_Address = NULL;
   pTDI_Endpoint->m_nAssociateStatus = STATUS_UNSUCCESSFUL;

   pDeviceObject = IoGetRelatedDeviceObject( pTDI_Address->m_pFileObject );

   //
   // Associate the local endpoint with the address object.
   //
   TdiBuildAssociateAddress(
      pTDI_Address->m_pAtomicIrp,
      pDeviceObject,
      pTDI_Endpoint->m_pFileObject,
      NULL,
      NULL,
      pTDI_Address->m_hAddress
      );

   //
   // Submit The Request To The Transport
   //
   pTDI_Endpoint->m_nAssociateStatus = TDI_MakeSimpleTdiRequest(
                                          pDeviceObject,
                                          pTDI_Address->m_pAtomicIrp
                                          );

   if( NT_SUCCESS( pTDI_Endpoint->m_nAssociateStatus ) )
   {
      pTDI_Endpoint->m_pTDI_Address = pTDI_Address;

      _I_TDI_RefTransportAddress( pTDI_Endpoint->m_pTDI_Address );
   }

  if (pTDI_Address->m_hAddress)
  {
//	  ZwClose(pTDI_Address->m_hAddress); // No Need this anymore.
//	  pTDI_Address->m_hAddress = NULL;
  }

   return( pTDI_Endpoint->m_nAssociateStatus );
}


/////////////////////////////////////////////////////////////////////////////
//// _I_TDI_DisassociateAddress (INTERNAL/PRIVATE)
//
// Purpose
//
// Parameters
//
// Return Value
// status
//
// Remarks
//

NTSTATUS
_I_TDI_DisassociateAddress(
   PTDI_ENDPOINT   pTDI_Endpoint
   )
{
   NTSTATUS       status = STATUS_SUCCESS;
   PDEVICE_OBJECT pDeviceObject;

   pDeviceObject = IoGetRelatedDeviceObject( pTDI_Endpoint->m_pFileObject );
   
   // Disassociate the local endpoint with the address object.
   if( pTDI_Endpoint->m_pTDI_Address )
   {
      TdiBuildDisassociateAddress(
         (pTDI_Endpoint->m_pTDI_Address)->m_pAtomicIrp,
         pDeviceObject,
         pTDI_Endpoint->m_pFileObject,
         NULL,
         NULL
         );

      //
      // Submit The Request To The Transport
      //
      status = TDI_MakeSimpleTdiRequest(
                     pDeviceObject,
                     (pTDI_Endpoint->m_pTDI_Address)->m_pAtomicIrp
                     );
   }

   if( NT_SUCCESS( status ) )
   {
      if( pTDI_Endpoint->m_pTDI_Address )
      {
         _I_TDI_DerefTransportAddress( pTDI_Endpoint->m_pTDI_Address );
      }
   }

   return( status );
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_OpenConnectionEndpoint
//
// Purpose
// Open a new TDI connection endpoint and associate it with a specified
// transport address object.
//
// Parameters
//   TransportDeviceNameW
//      Pointer to a zero-terminated wide character string that specifies
//      the transport device. An example would be: L"\\Device\\Tcp"
//
//   pTDI_Address
//      Pointer to the TDI_ADDRESS structure previously opened using the
//      LS_OpenTransportAddress function. The TDI_ADDRESS pointer specifies
//      the transport address object to be used with the new connection
//      endpoint.
//
//   pTDI_Endpoint
//      Pointer to a caller-provided TDI_ENDPOINT structure that will be
//      initialized as the connection endpoint is opened.
//
//   pContext
//      This is actually an arbitrary value provided by the caller. The value
//      passed as pContext here will simply be returned as TdiEventContex when
//      TDI event handlers are called. Typically pContext is actually a pointer
//      to a structure of interest to the caller.
//
// Return Value
// status
//
// Remarks
// This function combines several of the steps involved in opening a connection
// endpoint into one convenient function.
//
// Before calling this function the caller must have already opened a transport
// address object using  TDI_OpenTransportAddress.
//
// This function performs most of the mechanics of opening a TDI connection endpoint
// object. It builds the extended attributes (EA) buffer, calls ZwCreateFile and
// obtains a pointer to the connection endpoint object file pointer by calling
// ObReferenceObjectByHandle. If these operations are successful, the function calls
// an internal function called _I_TDI_AssociateAddress to associate the new connection
// endpoint with the specified transport address.
//
// The connection endpoint object handle and its file pointer are stored in the
// TDI_ENDPOINT structure. A pointer to a TDI_ENDPOINT structure that has been successfully
// initialized by TDI_OpenConnectionEndpoint is used by subsequent calls that operate
// on a connection endpoint.
//
// It is important to note that TDI_OpenConnectionEndpoint will call ZwCreateFile in the
// process of opening a connection endpoint. The call to ZwCreateFile creates a
// client-process-specific file object that represents the connection endpoint object.
// This means that the m_hContext handle is only valid in the same client process that
// the ZwCreateFile call was made in.
//
// Fortunately, most TDI operations actually reference the file object pointer - not
// the handle. Operations that do not reference the connection handle can be performed
// in arbitrary context unless there are other restrictions.
//
// The client process dependency of the connection endpoint handle does affect the design
// of a TDI Client to some extent. In particular, the client process that was used to 
// create an connection endpoint object must continue to exist until the connection endpoint
// object is eventually closed.
//
// There are several ways to insure that the client process continues to exist while
// the connection endpoint object is in use. The most common is to create a thread that
// is attached to the system process. As long as the system process continues to exist,
// the connection endpoint object handle will remain valid.
//
// See the NT DDK documentation topic "5.2 Opening a Connection Endpoint" for more
// information.
//
// Callers of TDI_OpenConnectionEndpoint must be running at IRQL PASSIVE_LEVEL.
//

NTSTATUS
TDI_OpenConnectionEndpoint(
   IN PWSTR       TransportDeviceNameW,// Zero-terminated String
   PTDI_ADDRESS    pTDI_Address,
   PTDI_ENDPOINT   pTDI_Endpoint,
   PVOID          pContext
   )
{
   NTSTATUS status;

   KdPrint(("TDI_OpenConnectionEndpoint: Entry...\n") );

   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

   //
   // Initialize TDI_CONTEXT Structure
   //
   NdisZeroMemory( pTDI_Endpoint, sizeof( TDI_ENDPOINT ) );

   pTDI_Endpoint->m_ReferenceCount = 1;

   pTDI_Endpoint->m_pTDI_Address = NULL;

   //
   // Open Connection Context
   //
   status = _I_TDI_OpenConnectionContext(
                  TransportDeviceNameW,
                  pTDI_Endpoint,
                  pContext
                  );

   if( !NT_SUCCESS( status ) )
   {
      return( pTDI_Endpoint->m_nOpenStatus );
   }

   KdPrint(("TDI_OpenConnectionEndpoint: Opened Connection Context\n") );

   //
   // Associate Address
   //
   status = _I_TDI_AssociateAddress( pTDI_Address, pTDI_Endpoint );

   if( NT_SUCCESS( status ) )
   {
      KdPrint(("TDI_OpenConnectionEndpoint: Associated Address\n") );

      //
      // Add The Connection To The Address Object's Connection List
      //
      InsertTailList(
         &pTDI_Address->m_ConnectionList,
         &pTDI_Endpoint->m_ListElement
         );
   }

   return( pTDI_Endpoint->m_nAssociateStatus );
}


/////////////////////////////////////////////////////////////////////////////
//// _I_TDI_RefConnectionEndpoint (INTERNAL/PRIVATE)
//
// Purpose
// This routine increments the reference count on the connection endpoint.
//
// Parameters
//
// Return Value
// None.
//
// Remarks
//

VOID
_I_TDI_RefConnectionEndpoint(
   PTDI_ENDPOINT   pTDI_Endpoint
   )
{
   InterlockedIncrement( &pTDI_Endpoint->m_ReferenceCount );
}


/////////////////////////////////////////////////////////////////////////////
//// _I_TDI_DestroyConnection (INTERNAL/PRIVATE)
//
// Purpose
//
// Parameters
//
// Return Value
// status
//
// Remarks
//

NTSTATUS
_I_TDI_DestroyConnection(
   PTDI_ENDPOINT   pTDI_Endpoint
   )
{
   NTSTATUS status;

   //
   // About Disassociate Address
   // --------------------------
   // Logically one would think it appropriate to call _I_TDI_DisassociateAddress
   // at this point. However, the NT DDK TDI documentation states that when
   // closing a connection endpoint it is unnecessary "to disassociate
   // the connection endpoint from from its associated transport address
   // before making a close-connection-endpoint request".
   //
   // Further, on TCP it appears that a call to _I_TDI_DisassociateAddress
   // will often fail with STATUS_CONNECTION_ACTIVE (0xC000023B).
   //
   // So, instead of calling _I_TDI_DisassociateAddress we just remove
   // the reference counts to the transport address and connection context
   // objects that was created by _I_TDI_AssociateAddress.
   //
   _I_TDI_DerefTransportAddress( pTDI_Endpoint->m_pTDI_Address );

   //
   // Remove The Connection To The Address Object's Connection List
   //
   RemoveEntryList( &pTDI_Endpoint->m_ListElement );

   //
   // Close Connection Context
   //
   status = _I_TDI_CloseConnectionContext( pTDI_Endpoint );

#ifdef DBG
   if( NT_SUCCESS( status ) )
   {
      KdPrint(("TDI_DestroyConnection: Closed Connection Context\n") );
   }
#endif // DBG

   return( status );
}


/////////////////////////////////////////////////////////////////////////////
//// _I_TDI_DerefConnectionEndpoint (INTERNAL/PRIVATE)
//
// Purpose
// This routine decrements the reference count on the connection endpoint.
//
// Parameters
//
// Return Value
// None.
//
// Remarks
//

VOID
_I_TDI_DerefConnectionEndpoint(
   PTDI_ENDPOINT   pTDI_Endpoint
   )
{
   LONG  Result;

   Result = InterlockedDecrement( &pTDI_Endpoint->m_ReferenceCount );

   if( !Result )
   {
      //
      // Destroy The Connection
      //
      _I_TDI_DestroyConnection( pTDI_Endpoint );
   }
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_CloseConnectionEndpoint
//
// Purpose
// TDI_CloseConnectionEndpoint closes the connection endpoint specified by
// pTDI_ENDPOINT.
//
// Parameters
// pTDI_Endpoint
//    Pointer to the TDI_ENDPOINT structure that specifies the connection endpoint
//    object pointer and handle to be closed.
//
// Return Value
// status
//
// Remarks
// This function calls the internal function _I_TDI_DerefConnectionEndpoint to
// decrement the reference count for the TDI_ENDPOINT structure. If the reference
// count is decremented to zero by this call, the connection endpoint object
// specified by pTDI_ENDPOINT will actually be closed.
//
// Callers of TDI_CloseConnectionEndpoint must be running at IRQL PASSIVE_LEVEL.
//

NTSTATUS
TDI_CloseConnectionEndpoint(
   PTDI_ENDPOINT   pTDI_Endpoint
   )
{
   KdPrint(("TDI_CloseConnectionEndpoint: Entry...\n"));

   _I_TDI_DerefConnectionEndpoint( pTDI_Endpoint );

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_Connect
//
// Purpose
// This routine establishes a connection between a local connection endpoint
// and a remote transport address.
//
// Parameters
//   pTDI_Endpoint
//      Pointer to the TDI_ENDPOINT structure that specifies the local connection
//      endpoint object pointer to be used.
//   pTransportAddress
//      Pointer to a TRANSPORT_ADDRESS structure that specifies the remote
//      connection address for the connection.
//
// Return Value
//
// Remarks
// For a local-node client to establish an endpoint-to-endpoint connection
// with a remote-node peer process, it must first associate an idle local
// connection endpoint with a local-node address. A client cannot initiate a
// connection attempt to a remote-node peer until it has made a successful
// TDI_ASSOCIATE_ADDRESS request, which it set up with TdiBuildAsociateAddress,
// to its underlying transport.
//
// It terms of the KS library functions, this means that the TDI_ENDPOINT
// structure pointed to by pTDI_Endpoint must have been successfully initialized
// by calling TDI_OpenConntectionEndpoint.
//
// See the NT DDK documentation topic "5.5 Making an Endpoint-to-Endpoint
// Connection" for more information.
//
// As currently written, callers of TDI_Connect must be running at
// IRQL PASSIVE_LEVEL.
//

NTSTATUS
TDI_Connect(
   PTDI_ENDPOINT            pTDI_Endpoint,
   IN PTRANSPORT_ADDRESS   pTransportAddress // Remote Transport Address
   )
{
   NTSTATUS                   status = STATUS_SUCCESS;
	TDI_CONNECTION_INFORMATION RequestConnectionInfo;
//	TDI_CONNECTION_INFORMATION ReturnConnectionInfo;
   LARGE_INTEGER              ConnectionTimeOut = {0,0};
   PDEVICE_OBJECT             pDeviceObject;
   
   KdPrint(("TDI_Connect: Entry...\n"));

   pDeviceObject = IoGetRelatedDeviceObject( pTDI_Endpoint->m_pFileObject );

   //
   // Setup Request Connection Info
   //
	RequestConnectionInfo.UserDataLength = 0;
	RequestConnectionInfo.UserData = NULL;
	RequestConnectionInfo.OptionsLength = 0;
	RequestConnectionInfo.Options = NULL;
	RequestConnectionInfo.RemoteAddressLength = sizeof(TA_IP_ADDRESS);
	RequestConnectionInfo.RemoteAddress = pTransportAddress;

	TdiBuildConnect(
      (pTDI_Endpoint->m_pTDI_Address)->m_pAtomicIrp,
      pDeviceObject,
		pTDI_Endpoint->m_pFileObject,
      _I_TDI_SimpleTdiRequestComplete, // Completion Routine
		NULL,                         // Completion Context
//		&ConnectionTimeOut,           // Timeout Information
		NULL,                         // Timeout Information
		&RequestConnectionInfo,
//		&ReturnConnectionInfo
      NULL
		);

   //
   // Submit The Request To The Transport
   //
   status = TDI_MakeSimpleTdiRequest(
               pDeviceObject,
               (pTDI_Endpoint->m_pTDI_Address)->m_pAtomicIrp
               );

   return status;
}


NTSTATUS
TDI_NewConnect(
   PTDI_ENDPOINT          pTDI_Endpoint,
   IN PTRANSPORT_ADDRESS   pTransportAddress, // Remote Transport Address
   IN PTDI_CONNECT_COMPLETION_ROUTINUE UserCompletionRoutine,	//Optional
   PVOID Context
   )
{
   NTSTATUS                   status = STATUS_SUCCESS;
	TDI_CONNECTION_INFORMATION RequestConnectionInfo;
//	TDI_CONNECTION_INFORMATION ReturnConnectionInfo;
   LARGE_INTEGER              ConnectionTimeOut = {0,0};
   PDEVICE_OBJECT             pDeviceObject;
   
   KdPrint(("TDI_Connect: Entry...\n"));

   pDeviceObject = IoGetRelatedDeviceObject( pTDI_Endpoint->m_pFileObject );

   //
   // Setup Request Connection Info
   //
	RequestConnectionInfo.UserDataLength = 0;
	RequestConnectionInfo.UserData = NULL;
	RequestConnectionInfo.OptionsLength = 0;
	RequestConnectionInfo.Options = NULL;
	RequestConnectionInfo.RemoteAddressLength = sizeof(TA_IP_ADDRESS);
	RequestConnectionInfo.RemoteAddress = pTransportAddress;

	if(UserCompletionRoutine == NULL)
	{

		TdiBuildConnect(
		  (pTDI_Endpoint->m_pTDI_Address)->m_pAtomicIrp,
		pDeviceObject,
			pTDI_Endpoint->m_pFileObject,
		_I_TDI_SimpleTdiRequestComplete, // Completion Routine
			NULL,                         // Completion Context
	//		&ConnectionTimeOut,           // Timeout Information
			NULL,                         // Timeout Information
			&RequestConnectionInfo,
	//		&ReturnConnectionInfo
		  NULL
			);

		//
		// Submit The Request To The Transport
		//
		status = TDI_MakeSimpleTdiRequest(
			       pDeviceObject,
				   (pTDI_Endpoint->m_pTDI_Address)->m_pAtomicIrp
				);

		return status;
	}
	else
	{

		TdiBuildConnect(
		  (pTDI_Endpoint->m_pTDI_Address)->m_pAtomicIrp,
			pDeviceObject,
			pTDI_Endpoint->m_pFileObject,
			UserCompletionRoutine, // Completion Routine
			Context,                         // Completion Context
	//		&ConnectionTimeOut,           // Timeout Information
			NULL,                         // Timeout Information
			&RequestConnectionInfo,
	//		&ReturnConnectionInfo
		  NULL
			);

		ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

		status = IoCallDriver( pDeviceObject, (pTDI_Endpoint->m_pTDI_Address)->m_pAtomicIrp );

		if( !NT_SUCCESS(status) )
		{
			KdPrint( ("IoCallDriver(pDeviceObject = %lx) returned %lx\n",pDeviceObject,status));
		}
		return status;
	}
	return status;
}


/////////////////////////////////////////////////////////////////////////////
//// _I_TDI_EndpointRequestComplete (INTERNAL/PRIVATE)
//
// Purpose
//
// Parameters
//
// Return Value
//
// Remarks
//

static NTSTATUS
_I_TDI_EndpointRequestComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PVOID Context
    )
{
   PTDI_ENDPOINT_REQUEST_CONTEXT  pTDI_RequestContext;
   PKEVENT                       pEvent;
   BOOLEAN                       bSynchronous;

   pTDI_RequestContext = (PTDI_ENDPOINT_REQUEST_CONTEXT )Context;

   if( pTDI_RequestContext != NULL )
   {
      bSynchronous = pTDI_RequestContext->m_bSynchronous;

      //
      // Fill IoStatusBlock With Final status And Information
      //
      (pTDI_RequestContext->m_pIoStatusBlock)->Status = pIrp->IoStatus.Status;
      (pTDI_RequestContext->m_pIoStatusBlock)->Information = pIrp->IoStatus.Information;

      //
      // Set The Completion Event, If Specified
      //
      if( pTDI_RequestContext->m_CompletionEvent )
      {
         KeSetEvent( pTDI_RequestContext->m_CompletionEvent, 0, FALSE);
      }

      //
      // Call The Completion Routine, If Specified
      //
      if( pTDI_RequestContext->m_CompletionRoutine )
      {
         (pTDI_RequestContext->m_CompletionRoutine)(
               pTDI_RequestContext->m_CompletionContext,
               pTDI_RequestContext->m_pIoStatusBlock,
               pTDI_RequestContext->m_Reserved
               );
      }

      //
      // Dereference The Connection
      //
      _I_TDI_DerefConnectionEndpoint( pTDI_RequestContext->m_pTDI_Endpoint );

      //
      // Free Memory Allocated For Private Completion Context
      //
      NdisFreeMemory(
         pTDI_RequestContext,
         sizeof( TDI_ENDPOINT_REQUEST_CONTEXT ),
         0
         );

      if( !bSynchronous )
      {
         IoFreeIrp( pIrp );
      }
   }

   return STATUS_MORE_PROCESSING_REQUIRED;
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_Disconnect
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
TDI_Disconnect(
   PTDI_ENDPOINT                     pTDI_Endpoint,
   HANDLE                           UserCompletionEvent,    // Optional
   PTDI_REQUEST_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                            UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK                 pIoStatusBlock,         // Required
   ULONG                            Flags
   )
{
   NTSTATUS                      status;
   PIRP                          pIrp;
   KEVENT                        SyncEvent;
   BOOLEAN                       bSynchronous = FALSE;
   PTDI_ENDPOINT_REQUEST_CONTEXT  pTDI_RequestContext = NULL;
   PDEVICE_OBJECT                pDeviceObject;

   pDeviceObject = IoGetRelatedDeviceObject( pTDI_Endpoint->m_pFileObject );

   KdPrint(("After IoGetRelatedDeviceObject()\n"));

   //
   // Allocate Memory For Private Completion Context
   //
   status = NdisAllocateMemory(
               &pTDI_RequestContext,
               sizeof( TDI_ENDPOINT_REQUEST_CONTEXT ),
               0,       // Allocate non-paged system-space memory
               HighestAcceptableMax
               );

   if( !NT_SUCCESS( status ) )
   {
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   RtlZeroMemory( pTDI_RequestContext, sizeof( TDI_ENDPOINT_REQUEST_CONTEXT ) );

   if( pIoStatusBlock )
   {
      pTDI_RequestContext->m_pIoStatusBlock = pIoStatusBlock;
   }
   else
   {
      pTDI_RequestContext->m_pIoStatusBlock = &pTDI_RequestContext->m_SafeIoStatusBlock;
   }

   //
   // Initialize Private Completion Context
   //
   pTDI_RequestContext->m_pTDI_Endpoint = pTDI_Endpoint;

   if( UserCompletionRoutine )
   {
      pTDI_RequestContext->m_CompletionEvent = UserCompletionEvent;
      pTDI_RequestContext->m_CompletionRoutine = UserCompletionRoutine;
      pTDI_RequestContext->m_CompletionContext = UserCompletionContext;

      bSynchronous = FALSE;
   }
   else if( UserCompletionEvent )
   {
      pTDI_RequestContext->m_CompletionEvent = UserCompletionEvent;
      KeResetEvent( pTDI_RequestContext->m_CompletionEvent );

      pTDI_RequestContext->m_CompletionRoutine = NULL;
      pTDI_RequestContext->m_CompletionContext = NULL;

      bSynchronous = FALSE;
   }
   else
   {
      pTDI_RequestContext->m_CompletionEvent = &SyncEvent;

      ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

      KeInitializeEvent(
         pTDI_RequestContext->m_CompletionEvent,
         NotificationEvent,
         FALSE
         );

      pTDI_RequestContext->m_CompletionRoutine = NULL;
      pTDI_RequestContext->m_CompletionContext = NULL;

      bSynchronous = TRUE;
   }

   pTDI_RequestContext->m_bSynchronous = bSynchronous;

   //
   // Allocate IRP
   //
   pIrp = IoAllocateIrp( pDeviceObject->StackSize, FALSE );

   if( !pIrp )
   {
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   //
   // Build Disconnect Call
   //
   TdiBuildDisconnect(
      pIrp,
      pDeviceObject,
      pTDI_Endpoint->m_pFileObject,
      _I_TDI_EndpointRequestComplete,// Completion routine
      pTDI_RequestContext,           // Completion context
//		&DisconnectTimeOut,           // Timeout Information
		NULL,                         // Timeout Information
      Flags,                        // Flags
      NULL,                         // Request Connection Info
      NULL                          // Return Connection Info
      );

   //
   // Reference The Connection
   //
   _I_TDI_RefConnectionEndpoint( pTDI_RequestContext->m_pTDI_Endpoint );

   //
   // Submit the request
   //
   (pTDI_RequestContext->m_pIoStatusBlock)->Status = STATUS_UNSUCCESSFUL;
   (pTDI_RequestContext->m_pIoStatusBlock)->Information = 0;

   KdPrint(("Before Calling IoCallDriver()\n"));

   status = IoCallDriver( pDeviceObject, pIrp );

   if( bSynchronous )
   {
      if( (status == STATUS_SUCCESS) || (status == STATUS_PENDING) )
      {
         ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

         status = KeWaitForSingleObject(
                        &SyncEvent,  // Object to wait on.
                        Executive,  // Reason for waiting
                        KernelMode, // Processor mode
                        FALSE,      // Alertable
                        NULL        // Timeout
                        );

         if( NT_SUCCESS( status ) )
         {
            status = pIrp->IoStatus.Status;
         }

         IoFreeIrp( pIrp );
      }
   }

   if( !NT_SUCCESS( status ) )
   {
      KdPrint(( "TDI_Disconnect: Return status 0x%8.8x\n", status ));
   }

   return( status );
}


/////////////////////////////////////////////////////////////////////////////
//// _I_TDI_DoSendOrReceive (INTERNAL/PRIVATE)
//
// Purpose
// Send or receive a chain of MDLs on the connection endpoint.
//
// Parameters
//   pSendMdl - Pointer to the first MDL of the chain to be sent or received on.
//
// Return Value
//
// Remarks
// See calling function descriptions.
//

NTSTATUS
_I_TDI_DoSendOrReceive(
   PTDI_ENDPOINT                     pTDI_Endpoint,
   HANDLE                           UserCompletionEvent,    // Optional
   PTDI_REQUEST_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                            UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK                 pIoStatusBlock,         // Required
   PMDL                             pMdl,
   ULONG                            Flags,
   BOOLEAN                          bSend
   )
{
   NTSTATUS                      status;
   PIRP                          pIrp;
   ULONG                         Length;
   KEVENT                        SyncEvent;
   BOOLEAN                       bSynchronous = FALSE;
   PTDI_ENDPOINT_REQUEST_CONTEXT  pTDI_RequestContext = NULL;
   PDEVICE_OBJECT                pDeviceObject;
   PMDL                          pNextMdl;

   pDeviceObject = IoGetRelatedDeviceObject( pTDI_Endpoint->m_pFileObject );

   //
   // Allocate Memory For Private Completion Context
   //
   status = NdisAllocateMemory(
               &pTDI_RequestContext,
               sizeof( TDI_ENDPOINT_REQUEST_CONTEXT ),
               0,       // Allocate non-paged system-space memory
               HighestAcceptableMax
               );

   if( !NT_SUCCESS( status ) )
   {
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   RtlZeroMemory( pTDI_RequestContext, sizeof( TDI_ENDPOINT_REQUEST_CONTEXT ) );

   if( pIoStatusBlock )
   {
      pTDI_RequestContext->m_pIoStatusBlock = pIoStatusBlock;
   }
   else
   {
      pTDI_RequestContext->m_pIoStatusBlock = &pTDI_RequestContext->m_SafeIoStatusBlock;
   }

   //
   // Initialize Private Completion Context
   //
   pTDI_RequestContext->m_pTDI_Endpoint = pTDI_Endpoint;

   if( UserCompletionRoutine )
   {
      pTDI_RequestContext->m_CompletionEvent = UserCompletionEvent;
      pTDI_RequestContext->m_CompletionRoutine = UserCompletionRoutine;
      pTDI_RequestContext->m_CompletionContext = UserCompletionContext;

      bSynchronous = FALSE;
   }
   else if( UserCompletionEvent )
   {
      pTDI_RequestContext->m_CompletionEvent = UserCompletionEvent;
      KeResetEvent( pTDI_RequestContext->m_CompletionEvent );

      pTDI_RequestContext->m_CompletionRoutine = NULL;
      pTDI_RequestContext->m_CompletionContext = NULL;

      bSynchronous = FALSE;
   }
   else
   {
      pTDI_RequestContext->m_CompletionEvent = &SyncEvent;

      ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

      KeInitializeEvent(
         pTDI_RequestContext->m_CompletionEvent,
         NotificationEvent,
         FALSE
         );

      pTDI_RequestContext->m_CompletionRoutine = NULL;
      pTDI_RequestContext->m_CompletionContext = NULL;

      bSynchronous = TRUE;
   }

   pTDI_RequestContext->m_bSynchronous = bSynchronous;

   //
   // Allocate IRP
   //
   pIrp = IoAllocateIrp( pDeviceObject->StackSize, FALSE );

   if( !pIrp )
   {
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   Length = 0;
   pNextMdl = pMdl;

   while( pNextMdl )
   {
      Length += MmGetMdlByteCount( pNextMdl );
      pNextMdl = pNextMdl->Next;
   }

   if( bSend )
   {
      TdiBuildSend(
         pIrp,
         pDeviceObject,
         pTDI_Endpoint->m_pFileObject,
         _I_TDI_EndpointRequestComplete,   // Completion routine
         pTDI_RequestContext,              // Completion context
         pMdl,                            // the data buffer
         Flags,                           // send flags
         Length                           // total length of send MDLs
         );
   }
   else
   {
      TdiBuildReceive(
         pIrp,
         pDeviceObject,
         pTDI_Endpoint->m_pFileObject,
         _I_TDI_EndpointRequestComplete,   // Completion routine
         pTDI_RequestContext,              // Completion context
         pMdl,                            // the data buffer
         Flags,                           // send flags
         Length                           // total length of receive MDLs
         );
   }

   //
   // Reference The Connection
   //
   _I_TDI_RefConnectionEndpoint( pTDI_RequestContext->m_pTDI_Endpoint );

   //
   // Submit the request
   //
   (pTDI_RequestContext->m_pIoStatusBlock)->Status = STATUS_UNSUCCESSFUL;
   (pTDI_RequestContext->m_pIoStatusBlock)->Information = 0;

   status = IoCallDriver( pDeviceObject, pIrp );

   if( bSynchronous )
   {
      if( (status == STATUS_SUCCESS) || (status == STATUS_PENDING) )
      {
         ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

         status = KeWaitForSingleObject(
                        &SyncEvent,  // Object to wait on.
                        Executive,  // Reason for waiting
                        KernelMode, // Processor mode
                        FALSE,      // Alertable
                        NULL        // Timeout
                        );

         if( NT_SUCCESS( status ) )
         {
            status = pIrp->IoStatus.Status;
         }

         IoFreeIrp( pIrp );
      }
   }

   if( !NT_SUCCESS( status ) )
   {
      KdPrint(( "_I_TDI_DoSendOrReceive: Return status 0x%8.8x\n", status ));
   }

   return( status );
}


NTSTATUS
_I_TDI_DoSendOrReceive1(
   PTDI_ENDPOINT                     pTDI_Endpoint,
   HANDLE                           UserCompletionEvent,    // Optional
   PIO_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                            UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK                 pIoStatusBlock,         // Required
   PMDL                             pMdl,
   ULONG                            Flags,
   BOOLEAN                          bSend,
   PIRP								*pRequestIrp
   )
{
   NTSTATUS                      status;
   PIRP                          pIrp;
   ULONG                         Length;
   PDEVICE_OBJECT                pDeviceObject = NULL;
   PMDL                          pNextMdl;

	pDeviceObject = IoGetRelatedDeviceObject( pTDI_Endpoint->m_pFileObject );

   if(*pRequestIrp == NULL)
   {
		//
		// Allocate IRP
		//
		pIrp = IoAllocateIrp( pDeviceObject->StackSize, FALSE );

		if( !pIrp )
		{
			return( STATUS_INSUFFICIENT_RESOURCES );
		}
		*pRequestIrp = pIrp;
   }
   else
   {
	   pIrp = *pRequestIrp;
   }

   Length = 0;
   pNextMdl = pMdl;

   while( pNextMdl )
   {
      Length += MmGetMdlByteCount( pNextMdl );
      pNextMdl = pNextMdl->Next;
   }

   if( bSend )
   {
      TdiBuildSend(
         pIrp,
         pDeviceObject,
         pTDI_Endpoint->m_pFileObject,
         UserCompletionRoutine,   // Completion routine
         UserCompletionContext,              // Completion context
         pMdl,                            // the data buffer
         Flags,                           // send flags
         Length                           // total length of send MDLs
         );
   }
   else
   {
      TdiBuildReceive(
         pIrp,
         pDeviceObject,
         pTDI_Endpoint->m_pFileObject,
         UserCompletionRoutine,   // Completion routine
         UserCompletionContext,              // Completion context
         pMdl,                            // the data buffer
         Flags,                           // send flags
         Length                           // total length of receive MDLs
         );
   }

   status = IoCallDriver( pDeviceObject, pIrp );

   if( !NT_SUCCESS( status ) )
   {
      KdPrint(( "_I_TDI_DoSendOrReceive: Return status 0x%8.8x\n", status ));
   }

   return( status );
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_SendOnEndpoint
//
// Purpose
// Send a chain of MDLs as connection-oriented data on the connection
// endpoint object.
//
// Parameters
//    pSendMdl
//       Pointer to the first Memory Descriptor List (MDL) of the data
//       to be sent.
//
//    pTDI_Endpoint
//       Pointer to the TDI_ENDPOINT structure that specifies the connection
//       endpoint object that the data will be sent on.
//
//    pIoStatusBlock
//       Pointer to a caller provided IO_STATUS_BLOCK structure that received
//       final completion status and sent byte count.
//
//    UserCompletionRoutine
//       Specifies the entry point for the caller-supplied completion routine
//       to be called when the lower-level TDI driver completes the operation.
//       This routine is declared as follows:
//
//          VOID
//          (*PTDI_REQUEST_COMPLETION_ROUTINE) (
//             PVOID UserCompletionContext,
//             PIO_STATUS_BLOCK IoStatusBlock,
//             ULONG Reserved
//             );
//
//    UserCompletionContext
//        The value to be passed to UserCompletionRoutine.
//
// Remarks
// This function sends a chain of MDLs that represent connection-oriented
// data to be sent. The number of bytes to be sent is specified is the sum
// of the MDL byte counts for all MDLs in the chain. Having the Next field
// set to NULL identifies the last MDL in the chain. The byte count of each
// MDL in the chain is queried
// using MmGetMdlByteCount.
//
// Each of the MDLs in the chain must have been probed and locked before
// calling TDI_SendOnEndpoint.
//
// Synchronous/Asynchronous Operation
// ----------------------------------
// Synchronous Send
//    If UserCompletionEvent and UserCompletionRoutine are both NULL the send will
//    be performed synchronously.
//
// Asynchronous Send (Event Wait)
//    If a UserCompletionEvent is specified then the send will be performed
//    asynchronously. The framework will set the specified event when the
//    send is completed.
//
//    If UserCompletionEvent is specified, then UserCompletionRoutine and
//    UserCompletionContext are ignored.
//
// Asynchronous Send (Completion Routine)
//    If a UserCompletionEvent is NOT specified (i.e., it is NULL) and a
//    UserCompletionRoutine is specified, then the send will be performed
//    asynchronously. The framework will call the specified completion
//    routine when the send is completed and pass UserCompletionContext
//    as the context.
//
// In this implementation it is an error to specify both a UserCompletionEvent
// and a UserCompletionRoutine. Only one of these two methods can be used.
//
// If the synchronous mode of operation is employed, then the caller of
// TDI_SendOnEndpoint must be running at IRQL PASSIVE_LEVEL. If the caller
// intends to wait on the event specified by UserCompletionEvent, then the
// waiting must be performed at IRQL PASSIVE_LEVEL.
//
// For all methods the IO_STATUS_BLOCK pointed to by pIoStatusBlock will
// receive the final completion status and information about the requested
// send operation.
//

NTSTATUS
TDI_SendOnEndpoint(
   PTDI_ENDPOINT                     pTDI_Endpoint,
   HANDLE                           UserCompletionEvent,    // Optional
   PTDI_REQUEST_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                            UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK                 pIoStatusBlock,         // Required
   PMDL                             pSendMdl,
   ULONG                            SendFlags
   )
{
   NTSTATUS status;

   //
   // Call Common Routine To Do The Work
   //
   status = _I_TDI_DoSendOrReceive(
               pTDI_Endpoint,
               UserCompletionEvent,
               UserCompletionRoutine,
               UserCompletionContext,
               pIoStatusBlock,
               pSendMdl,
               SendFlags,
               TRUE           // TRUE -> Send
               );

   return( status );
}

NTSTATUS
TDI_SendOnEndpoint1(
   PTDI_ENDPOINT                     pTDI_Endpoint,
   HANDLE                           UserCompletionEvent,    // Optional
   PIO_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                            UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK                 pIoStatusBlock,         // Required
   PMDL                             pSendMdl,
   ULONG                            SendFlags,
   PIRP								*pRequestIrp
   )
{
   NTSTATUS status;

   //
   // Call Common Routine To Do The Work
   //
   status = _I_TDI_DoSendOrReceive1(
               pTDI_Endpoint,
               UserCompletionEvent,
               UserCompletionRoutine,
               UserCompletionContext,
               pIoStatusBlock,
               pSendMdl,
               SendFlags,
               TRUE,           // TRUE -> Send
			   pRequestIrp
               );

   return( status );
}


NTSTATUS
TDI_ReceiveOnEndpoint(
   PTDI_ENDPOINT                     pTDI_Endpoint,
   HANDLE                           UserCompletionEvent,    // Optional
   PTDI_REQUEST_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                            UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK                 pIoStatusBlock,          // Required
   PMDL                             pReceiveMdl,
   ULONG                            ReceiveFlags
   )
{
   NTSTATUS status;

   //
   // Call Common Routine To Do The Work
   //
   status = _I_TDI_DoSendOrReceive(
               pTDI_Endpoint,
               UserCompletionEvent,
               UserCompletionRoutine,
               UserCompletionContext,
               pIoStatusBlock,
               pReceiveMdl,
               ReceiveFlags,
               FALSE           // FALSE -> Receive
               );

   return( status );
}

NTSTATUS
TDI_ReceiveOnEndpoint1(
   PTDI_ENDPOINT                     pTDI_Endpoint,
   HANDLE                           UserCompletionEvent,    // Optional
   PIO_COMPLETION_ROUTINE   UserCompletionRoutine,  // Optional
   PVOID                            UserCompletionContext,  // Optional
   PIO_STATUS_BLOCK                 pIoStatusBlock,          // Required
   PMDL                             pReceiveMdl,
   ULONG                            ReceiveFlags,
   PIRP								*pRequestIrp
   )
{
   NTSTATUS status;

   //
   // Call Common Routine To Do The Work
   //
   status = _I_TDI_DoSendOrReceive1(
               pTDI_Endpoint,
               UserCompletionEvent,
               UserCompletionRoutine,
               UserCompletionContext,
               pIoStatusBlock,
               pReceiveMdl,
               ReceiveFlags,
               FALSE,           // FALSE -> Receive
			   pRequestIrp
               );

   return( status );
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_QueryConnectionInfo
//
// Purpose
// Fetch the TDI_CONNECTION_INFO for the specified connection endpoint.
//
// Parameters
//    pTDI_Endpoint
//       Pointer to the TDI_ENDPOINT structure that specifies the connection
//       endpoint object for the request.
//
//    pInfoBuffer
//        Must point to a buffer sufficiently large to hold a
//        TDI_CONNECTION_INFORMATION structure.
//
//    pInfoBufferSize
//       Pointer to a ULONG. On entry, the ULONG pointed to by pInfoBufferSize
//       must be initialized with the size of the buffer at pInfoBuffer. On
//       return, the ULONG pointed to by pInfoBufferSize will be updated to
//       indicate the length of the data written to pInfoBuffer.
//
// Return Value
//
// Remarks
// Callers of TDI_QueryConnectionInfo must be running at IRQL PASSIVE_LEVEL.
//

NTSTATUS
TDI_QueryConnectionInfo(
   PTDI_ENDPOINT      pTDI_Endpoint,
   PVOID             pInfoBuffer,
   PULONG            pInfoBufferSize
   )
{
   NTSTATUS          status = STATUS_UNSUCCESSFUL;
   IO_STATUS_BLOCK   IoStatusBlock;
   PIRP              pIrp = NULL;
   PMDL              pMdl;
   PDEVICE_OBJECT    pDeviceObject;

   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

   pDeviceObject = IoGetRelatedDeviceObject( pTDI_Endpoint->m_pFileObject );

   RtlZeroMemory( pInfoBuffer, *pInfoBufferSize );

   //
   // Allocate IRP For The Query
   //
   pIrp = IoAllocateIrp( pDeviceObject->StackSize, FALSE );

   if( !pIrp )
   {
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   //
   // Prepare MDL For The Query Buffer
   //
   pMdl = TDI_AllocateAndProbeMdl(
            pInfoBuffer,         // Virtual address for MDL construction
            *pInfoBufferSize,    // size of the buffer
            FALSE,
            FALSE,
            NULL
            );

   if( !pMdl )
   {
      IoFreeIrp( pIrp );
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   TdiBuildQueryInformation(
      pIrp,
      pDeviceObject,
      pTDI_Endpoint->m_pFileObject,
      _I_TDI_SimpleTdiRequestComplete,    // Completion routine
      NULL,                            // Completion context
      TDI_QUERY_CONNECTION_INFO,
      pMdl);

   //
   // Submit The Query To The Transport
   //
   status = TDI_MakeSimpleTdiRequest(
               pDeviceObject,
               pIrp
               );

   //
   // Free Allocated Resources
   //
   TDI_UnlockAndFreeMdl(pMdl);

   *pInfoBufferSize = pIrp->IoStatus.Information;

   IoFreeIrp( pIrp );

   return( status );
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_SetConnectionInfo
//
// Purpose
// Set TDI_CONNECTION_INFO on the specified connection endpoint.
//
// Parameters
//
// Return Value
//
// Remarks
// Callers of TDI_SetConnectionInfo must be running at IRQL PASSIVE_LEVEL.
//

NTSTATUS
TDI_SetConnectionInfo(
   PTDI_ENDPOINT         pTDI_Endpoint,
   PTDI_CONNECTION_INFO pConnectionInfo
   )
{
   NTSTATUS          status = STATUS_UNSUCCESSFUL;
   IO_STATUS_BLOCK   IoStatusBlock;
   PIRP              pIrp = NULL;
   PMDL              pMdl;
   PDEVICE_OBJECT    pDeviceObject;

   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

   pDeviceObject = IoGetRelatedDeviceObject( pTDI_Endpoint->m_pFileObject );

   RtlZeroMemory( pConnectionInfo, sizeof( TDI_CONNECTION_INFO ) );

   //
   // Allocate IRP For The Request
   //
   pIrp = IoAllocateIrp( pDeviceObject->StackSize, FALSE );

   if( !pIrp )
   {
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   //
   // Prepare MDL For The Set Buffer
   //
   pMdl = TDI_AllocateAndProbeMdl(
            pConnectionInfo,           // Virtual address for MDL construction
            sizeof( TDI_CONNECTION_INFO ),  // size of the buffer
            FALSE,
            FALSE,
            NULL
            );

   if( !pMdl )
   {
      IoFreeIrp( pIrp );
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   TdiBuildSetInformation(
      pIrp,
      pDeviceObject,
      pTDI_Endpoint->m_pFileObject,
      _I_TDI_SimpleTdiRequestComplete,    // Completion routine
      NULL,                            // Completion context
      TDI_QUERY_CONNECTION_INFO,
      pMdl);

   //
   // Submit The Request To The Transport
   //
   status = TDI_MakeSimpleTdiRequest(
               pDeviceObject,
               pIrp
               );

   //
   // Free Allocated Resources
   //
   TDI_UnlockAndFreeMdl(pMdl);

   IoFreeIrp( pIrp );

   return( status );
}


/////////////////////////////////////////////////////////////////////////////
//           C O N T R O L  C H A N N E L  F U N C T I O N S               //
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//// TDI_OpenControlChannel
//
// Purpose
// Open a control channel to the specified transport.
//
// Parameters
//   TransportDeviceNameW
//      Pointer to a zero-terminated wide character string that specifies
//      the transport device. An example would be: L"\\Device\\Tcp"
//
//
// Return Value
//
// Remarks
// Callers of TDI_OpenControlChannel must be running at IRQL PASSIVE_LEVEL.
//

NTSTATUS
TDI_OpenControlChannel(
   IN PWSTR       TransportDeviceNameW,// Zero-terminated String
   PTDI_CHANNEL    pTDI_Channel
   )
{
   NTSTATUS          status = STATUS_SUCCESS;
   UNICODE_STRING    TransportDeviceName;
   HANDLE            hControlChannel;
   PFILE_OBJECT      pControlChannelFileObject = NULL;
   PDEVICE_OBJECT    pDeviceObject = NULL;
   OBJECT_ATTRIBUTES ChannelAttributes;
   IO_STATUS_BLOCK   IoStatusBlock;
   PIRP              pIrp = NULL;
   PMDL              pMdl;

   //
   // Initialize TDI_CHANNEL Structure
   //
   pTDI_Channel->m_pFileObject = NULL;

   //
   // Setup Transport Device Name
   //
   NdisInitUnicodeString( &TransportDeviceName, TransportDeviceNameW );

   //
   // Open A TDI Control Channel To The Specified Transport
   //
   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

   InitializeObjectAttributes(
      &ChannelAttributes,       // Tdi Control Channel attributes
      &TransportDeviceName,      // Transport Device Name
      OBJ_CASE_INSENSITIVE,     // Attributes
      NULL,                     // RootDirectory
      NULL                      // SecurityDescriptor
      );

   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

   status = ZwCreateFile(
               &pTDI_Channel->m_hControlChannel,// Handle
               GENERIC_READ | GENERIC_WRITE, // Desired Access
               &ChannelAttributes,           // Object Attributes
               &IoStatusBlock,               // Final I/O status block
               NULL,                         // Allocation Size
               FILE_ATTRIBUTE_NORMAL,        // Normal attributes
               FILE_SHARE_READ,              // Sharing attributes
               FILE_OPEN_IF,                 // Create disposition
               0,                            // CreateOptions
               NULL,                         // EA Buffer
               0);                           // EA length

   if( !NT_SUCCESS(status) )
   {
      return( status );
   }

   //
   // Obtain A Referenced Pointer To The File Object
   //
   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

   status = ObReferenceObjectByHandle (
               pTDI_Channel->m_hControlChannel,      // Object Handle
               FILE_ANY_ACCESS,                     // Desired Access
               NULL,                                // Object Type
               KernelMode,                          // Processor mode
               (PVOID *)&pTDI_Channel->m_pFileObject,// Object pointer
               NULL);                               // Object Handle information

   if( !NT_SUCCESS(status) )
   {
      ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	  if (pTDI_Channel->m_hControlChannel)
	  {
		  ZwClose( pTDI_Channel->m_hControlChannel );
		  pTDI_Channel->m_hControlChannel = NULL;
	  }

      pTDI_Channel->m_pFileObject = NULL;
   }
   else
   {
	   if (pTDI_Channel->m_hControlChannel)
	  {
//		ZwClose( pTDI_Channel->m_hControlChannel );
//		pTDI_Channel->m_hControlChannel = NULL;
	  }
   }

   return( status );
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_CloseControlChannel
//
// Purpose
// Close a control chanel previously opened using TDI_OpenControlChannel.
//
// Parameters
//
// Return Value
//
// Remarks
// Callers of TDI_CloseControlChannel must be running at IRQL PASSIVE_LEVEL.
//

NTSTATUS
TDI_CloseControlChannel(
   PTDI_CHANNEL    pTDI_Channel
   )
{
   if( pTDI_Channel->m_pFileObject )
   {
      ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
      ObDereferenceObject( pTDI_Channel->m_pFileObject );

      pTDI_Channel->m_pFileObject = NULL;

	  if (pTDI_Channel->m_hControlChannel)
	  {
		ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
		ZwClose( pTDI_Channel->m_hControlChannel );
		pTDI_Channel->m_hControlChannel = NULL;
	  }
   }

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_QueryProviderInfo
//
// Purpose
// Fetch the TDI_PROVIDER_INFO for the specified transport driver.
//
// Parameters
//   TransportDeviceNameW
//      Pointer to a zero-terminated wide character string that specifies
//      the transport device. An example would be: L"\\Device\\Tcp"
//
//
// Return Value
//
// Remarks
// Callers of TDI_QueryProviderInfo must be running at IRQL PASSIVE_LEVEL.
//

NTSTATUS
TDI_QueryProviderInfo(
   PWSTR              TransportDeviceNameW,// Zero-terminated String
   PTDI_PROVIDER_INFO pProviderInfo
   )
{
   NTSTATUS          status = STATUS_SUCCESS;
   TDI_CHANNEL        TDI_Channel;
   PDEVICE_OBJECT    pDeviceObject = NULL;
   PIRP              pIrp = NULL;
   PMDL              pMdl;

   //
   // Open A TDI Control Channel To The Specified Transport
   //
   status = TDI_OpenControlChannel(
               TransportDeviceNameW,
               &TDI_Channel
               );

   if( !NT_SUCCESS(status) )
   {
      DbgPrint( "Unable To Open TDI Control Channel\n" );
      return( status );
   }

   //
   // Obtain The Related Device Object
   //
   pDeviceObject = IoGetRelatedDeviceObject( TDI_Channel.m_pFileObject );

   //
   // Allocate IRP For The Query
   //
   pIrp = IoAllocateIrp( pDeviceObject->StackSize, FALSE );

   if( !pIrp )
   {
      TDI_CloseControlChannel( &TDI_Channel );

      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   //
   // Prepare MDL For The Query Buffer
   //
   pMdl = TDI_AllocateAndProbeMdl(
            pProviderInfo,           // Virtual address for MDL construction
            sizeof( TDI_PROVIDER_INFO ),  // size of the buffer
            FALSE,
            FALSE,
            NULL
            );

   if( !pMdl )
   {
      IoFreeIrp( pIrp );

      TDI_CloseControlChannel( &TDI_Channel );

      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   TdiBuildQueryInformation(
      pIrp,
      pDeviceObject,
      TDI_Channel.m_pFileObject,
      _I_TDI_SimpleTdiRequestComplete,    // Completion routine
      NULL,                            // Completion context
      TDI_QUERY_PROVIDER_INFO,
      pMdl);


   //
   // Submit The Query To The Transport
   //
   status = TDI_MakeSimpleTdiRequest( pDeviceObject, pIrp );

   //
   // Free Allocated Resources
   //
   TDI_UnlockAndFreeMdl(pMdl);

   IoFreeIrp( pIrp );

   TDI_CloseControlChannel( &TDI_Channel );

   return( status );
}

//Added By Veera

/////////////////////////////////////////////////////////////////////////////
//// TDI_QueryNetworkAddress
//
// Purpose
// Fetch the TRANSPORT_ADDRESS for the specified transport Provider.
//
// Parameters
//   TransportDeviceNameW
//      Pointer to a zero-terminated wide character string that specifies
//      the transport device. An example would be: L"\\Device\\Tcp"
//
//
// Return Value
//
// Remarks
// Callers of TDI_QueryNetworkAddress must be running at IRQL PASSIVE_LEVEL.
//
//The Caller has to free up the memory Allocated by this function, if the
//Returned status is STATUS_SUCCESS then the allocated memory is in
//*ppTransportAddress and the length of allocated memory is in *pLength
NTSTATUS
TDI_QueryNetworkAddress(
   IN	PWSTR	TransportDeviceNameW,// Zero-terminated String
   OUT	PVOID	*ppTransportAddress,
   OUT	PULONG	pLength
   )
{
   NTSTATUS          status = STATUS_SUCCESS;
   TDI_CHANNEL        TDI_Channel;
   PDEVICE_OBJECT    pDeviceObject = NULL;
   PIRP              pIrp = NULL;
   PMDL              pMdl;
   int nMaxNetworkAddresses = 20;
   int nBufferSize = 0;

   try
   {
	   ASSERT(ppTransportAddress);
	   ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	   *ppTransportAddress = NULL;

	   NdisZeroMemory(&TDI_Channel,sizeof(TDI_CHANNEL));
		//
		// Open A TDI Control Channel To The Specified Transport
		//
		status = TDI_OpenControlChannel(
					TransportDeviceNameW,
					&TDI_Channel
					);

		if( !NT_SUCCESS(status) )
		{
			DbgPrint( "Unable To Open TDI Control Channel\n" );
			leave;
		}

		//
		// Obtain The Related Device Object
		//
		pDeviceObject = IoGetRelatedDeviceObject( TDI_Channel.m_pFileObject );
		
		*ppTransportAddress = NULL;
		//Get the TCPIP Address Buffers
		nBufferSize = sizeof( TRANSPORT_ADDRESS )+ nMaxNetworkAddresses*sizeof(TDI_ADDRESS_IP);

		status = NdisAllocateMemoryWithTag(ppTransportAddress,nBufferSize,'AREV');

		if(!NT_SUCCESS(status))
		{
			DbgPrint("Unable to allocate memory for the Transport Addresses so leaving\n");
			leave;
		}
		
		//
		// Allocate IRP For The Query
		//
		pIrp = IoAllocateIrp( pDeviceObject->StackSize, FALSE );

		if( !pIrp )
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		//
		// Prepare MDL For The Query Buffer
		//

		//Lets get the Memory for nMaxNetworkAddress first,
		pMdl = TDI_AllocateAndProbeMdl(
					*ppTransportAddress,           // Virtual address for MDL construction
					nBufferSize,  // size of the buffer
					FALSE,
					FALSE,
					NULL
					);

		if( !pMdl )
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		TdiBuildQueryInformation(
			pIrp,
			pDeviceObject,
			TDI_Channel.m_pFileObject,
			_I_TDI_SimpleTdiRequestComplete,    // Completion routine
			NULL,                            // Completion context
			TDI_QUERY_NETWORK_ADDRESS,
			pMdl);


		//
		// Submit The Query To The Transport
		//
		status = TDI_MakeSimpleTdiRequest( pDeviceObject, pIrp );

		*pLength = nBufferSize;
   }
   finally
   {
		//
		// Free Allocated Resources
		//
	   //Free up the MDl
		if(pMdl != NULL)
		{
			TDI_UnlockAndFreeMdl(pMdl);
			pMdl = NULL;
		}

		//Free up the Irp
		if(pIrp != NULL)
		{
			IoFreeIrp( pIrp );
			pIrp = NULL;
		}
		//Free up the Control Channel 
		if(TDI_Channel.m_pFileObject != NULL)
		{
			TDI_CloseControlChannel( &TDI_Channel );
		}

		//If status is not STATUS_SUCCESS then free up the allocated buffer and return
		if(!NT_SUCCESS(status))
		{
			if(*ppTransportAddress != NULL)
			{
				NdisFreeMemory(*ppTransportAddress,nBufferSize,0);
				*ppTransportAddress = NULL;
				*pLength = 0;
			}
		}
   }

   return( status );
}

/////////////////////////////////////////////////////////////////////////////
//// TDI_QueryDataLinkAddress
//
// Purpose
// Fetch the TRANSPORT_ADDRESS for the specified transport Provider.
//
// Parameters
//   TransportDeviceNameW
//      Pointer to a zero-terminated wide character string that specifies
//      the transport device. An example would be: L"\\Device\\Tcp"
//
//
// Return Value
//
// Remarks
// Callers of TDI_QueryDataLinkAddress must be running at IRQL PASSIVE_LEVEL.
//
//The Caller has to free up the memory Allocated by this function, if the
//Returned status is STATUS_SUCCESS then the allocated memory is in
//*ppTransportAddress and the length of allocated memory is in *pLength

NTSTATUS
TDI_QueryDataLinkAddress(
   IN	PWSTR	TransportDeviceNameW,// Zero-terminated String
   OUT	PVOID	*ppTransportAddress,
   OUT	PULONG	pLength
   )
{
   NTSTATUS          status = STATUS_SUCCESS;
   TDI_CHANNEL        TDI_Channel;
   PDEVICE_OBJECT    pDeviceObject = NULL;
   PIRP              pIrp = NULL;
   PMDL              pMdl;
   int nMaxNetworkAddresses = 20;
   int nBufferSize = 0;

   try
   {
	   ASSERT(ppTransportAddress);
	   ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	   *ppTransportAddress = NULL;

	   NdisZeroMemory(&TDI_Channel,sizeof(TDI_CHANNEL));
		//
		// Open A TDI Control Channel To The Specified Transport
		//
		status = TDI_OpenControlChannel(
					TransportDeviceNameW,
					&TDI_Channel
					);

		if( !NT_SUCCESS(status) )
		{
			DbgPrint( "Unable To Open TDI Control Channel\n" );
			leave;
		}

		//
		// Obtain The Related Device Object
		//
		pDeviceObject = IoGetRelatedDeviceObject( TDI_Channel.m_pFileObject );
		
		*ppTransportAddress = NULL;
		//Get the TCPIP Address Buffers
		nBufferSize = sizeof( TRANSPORT_ADDRESS )+ nMaxNetworkAddresses*sizeof(TDI_ADDRESS_IP);

		status = NdisAllocateMemoryWithTag(ppTransportAddress,nBufferSize,'AREV');

		if(!NT_SUCCESS(status))
		{
			DbgPrint("Unable to allocate memory for the Transport Addresses so leaving\n");
			leave;
		}
		
		//
		// Allocate IRP For The Query
		//
		pIrp = IoAllocateIrp( pDeviceObject->StackSize, FALSE );

		if( !pIrp )
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		//
		// Prepare MDL For The Query Buffer
		//

		//Lets get the Memory for nMaxNetworkAddress first,
		pMdl = TDI_AllocateAndProbeMdl(
					*ppTransportAddress,           // Virtual address for MDL construction
					nBufferSize,  // size of the buffer
					FALSE,
					FALSE,
					NULL
					);

		if( !pMdl )
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		TdiBuildQueryInformation(
			pIrp,
			pDeviceObject,
			TDI_Channel.m_pFileObject,
			_I_TDI_SimpleTdiRequestComplete,    // Completion routine
			NULL,                            // Completion context
			TDI_QUERY_DATA_LINK_ADDRESS,
			pMdl);


		//
		// Submit The Query To The Transport
		//
		status = TDI_MakeSimpleTdiRequest( pDeviceObject, pIrp );

		*pLength = nBufferSize;
   }
   finally
   {
		//
		// Free Allocated Resources
		//
	   //Free up the MDl
		if(pMdl != NULL)
		{
			TDI_UnlockAndFreeMdl(pMdl);
			pMdl = NULL;
		}

		//Free up the Irp
		if(pIrp != NULL)
		{
			IoFreeIrp( pIrp );
			pIrp = NULL;
		}
		//Free up the Control Channel 
		if(TDI_Channel.m_pFileObject != NULL)
		{
			TDI_CloseControlChannel( &TDI_Channel );
		}

		//If status is not STATUS_SUCCESS then free up the allocated buffer and return
		if(!NT_SUCCESS(status))
		{
			if(*ppTransportAddress != NULL)
			{
				NdisFreeMemory(*ppTransportAddress,nBufferSize,0);
				*ppTransportAddress = NULL;
				*pLength = 0;
			}
		}
   }

   return( status );
}


//Added By Veera


/////////////////////////////////////////////////////////////////////////////
//// TDI_QueryProviderStatistics
//
// Purpose
// Fetch the TDI_PROVIDER_STATISTICS for the specified transport driver.
//
// Parameters
//   TransportDeviceNameW
//      Pointer to a zero-terminated wide character string that specifies
//      the transport device. An example would be: L"\\Device\\Tcp"
//
// Return Value
//
// Remarks
// Callers of TDI_QueryProviderStatistics must be running at IRQL PASSIVE_LEVEL.
//

NTSTATUS
TDI_QueryProviderStatistics(
   PWSTR                      TransportDeviceNameW,// Zero-terminated String
   PTDI_PROVIDER_STATISTICS   pProviderStatistics
   )
{
   NTSTATUS          status = STATUS_SUCCESS;
   TDI_CHANNEL        TDI_Channel;
   PDEVICE_OBJECT    pDeviceObject = NULL;
   PIRP              pIrp = NULL;
   PMDL              pMdl;

   //
   // Open A TDI Control Channel To The Specified Transport
   //
   status = TDI_OpenControlChannel(
               TransportDeviceNameW,
               &TDI_Channel
               );

   if( !NT_SUCCESS(status) )
   {
      DbgPrint( "Unable To Open TDI Control Channel\n" );
      return( status );
   }

   //
   // Obtain The Related Device Object
   //
   pDeviceObject = IoGetRelatedDeviceObject( TDI_Channel.m_pFileObject );

   //
   // Allocate IRP For The Query
   //
   pIrp = IoAllocateIrp( pDeviceObject->StackSize, FALSE );

   if( !pIrp )
   {
      TDI_CloseControlChannel( &TDI_Channel );

      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   //
   // Prepare MDL For The Query Buffer
   //
   pMdl = TDI_AllocateAndProbeMdl(
            pProviderStatistics,           // Virtual address for MDL construction
            sizeof( TDI_PROVIDER_STATISTICS ),  // size of the buffer
            FALSE,
            FALSE,
            NULL
            );

   if( !pMdl )
   {
      IoFreeIrp( pIrp );

      TDI_CloseControlChannel( &TDI_Channel );

      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   TdiBuildQueryInformation(
      pIrp,
      pDeviceObject,
      TDI_Channel.m_pFileObject,
      _I_TDI_SimpleTdiRequestComplete,    // Completion routine
      NULL,                            // Completion context
      TDI_QUERY_PROVIDER_STATISTICS,
      pMdl);


   //
   // Submit The Query To The Transport
   //
   status = TDI_MakeSimpleTdiRequest( pDeviceObject, pIrp );

   //
   // Free Allocated Resources
   //
   TDI_UnlockAndFreeMdl(pMdl);

   IoFreeIrp( pIrp );

   TDI_CloseControlChannel( &TDI_Channel );

   return( status );
}


/////////////////////////////////////////////////////////////////////////////
//                     U T I L I T Y  F U N C T I O N S                    //
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//// _I_TDI_SimpleTdiRequestComplete (INTERNAL/PRIVATE)
//
// Purpose
// Completion callback for driver calls initiated by TDI_MakeSimpleTdiRequest.
//
// Parameters
//   pDeviceObject - Transport device object.
//   pIrp - IRP setup with the TDI request to submit.
//
// Return Value
//
// Remarks
//

static NTSTATUS
_I_TDI_SimpleTdiRequestComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
   if( Context != NULL )
      KeSetEvent((PKEVENT )Context, 0, FALSE);

   return STATUS_MORE_PROCESSING_REQUIRED;
}

/////////////////////////////////////////////////////////////////////////////
//// TDI_MakeSimpleTdiRequest
//
// Purpose
// This routine submits a request to TDI and waits for it to complete.
//
// Parameters
//   pDeviceObject - Transport device object.
//   pIrp - IRP setup with the TDI request to submit.
//
// Return Value
//
// Remarks
// This is basically a blocking version of IoCallDriver.
//

NTSTATUS
TDI_MakeSimpleTdiRequest(
   IN PDEVICE_OBJECT pDeviceObject,
   IN PIRP pIrp
   )
{
   NTSTATUS status;
   KEVENT Event;

   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

   KeInitializeEvent (&Event, NotificationEvent, FALSE);

   IoSetCompletionRoutine(
      pIrp,                         // The IRP
      _I_TDI_SimpleTdiRequestComplete, // The completion routine
      &Event,                       // The completion context
      TRUE,                         // Invoke On Success
      TRUE,                         // Invoke On Error
      TRUE                          // Invoke On Cancel
      );

   //
   // Submit the request
   //
   status = IoCallDriver( pDeviceObject, pIrp );

   if( !NT_SUCCESS(status) )
   {
      KdPrint( ("IoCallDriver(pDeviceObject = %lx) returned %lx\n",pDeviceObject,status));
   }

   if ((status == STATUS_PENDING) || (status == STATUS_SUCCESS))
   {
      ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

      status = KeWaitForSingleObject(
                  &Event,     // Object to wait on.
                  Executive,  // Reason for waiting
                  KernelMode, // Processor mode
                  FALSE,      // Alertable
                  NULL        // Timeout
                  );

      if (!NT_SUCCESS(status))
      {
         KdPrint(("TDI_MakeSimpleTdiRequest could not wait Wait returned %lx\n",status));
         return status;
      }

      status = pIrp->IoStatus.Status;
   }

   return status;
}

/////////////////////////////////////////////////////////////////////////////
//// TDI_AllocateAndProbeMdl
//
// Purpose
//
// Parameters
//
// Return Value
//
// Remarks
//

PMDL
TDI_AllocateAndProbeMdl(
   PVOID VirtualAddress,
   ULONG Length,
   BOOLEAN SecondaryBuffer,
   BOOLEAN ChargeQuota,
   PIRP Irp OPTIONAL
   )
{
   PMDL pMdl = NULL;

   pMdl = IoAllocateMdl(
            VirtualAddress,
            Length,
            SecondaryBuffer,
            ChargeQuota,
            Irp
            );

   if( !pMdl )
   {
      return( (PMDL )NULL );
   }

   try
   {
      MmProbeAndLockPages( pMdl, KernelMode, IoModifyAccess );
   }
   except( EXCEPTION_EXECUTE_HANDLER )
   {
      IoFreeMdl( pMdl );

      pMdl = NULL;

      return( NULL );
   }

   pMdl->Next = NULL;

   return( pMdl );
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_UnlockAndFreeMdl
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
TDI_UnlockAndFreeMdl(
   PMDL pMdl
   )
{
   MmUnlockPages( pMdl );
   IoFreeMdl( pMdl );
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_BuildEaBuffer
//
// Purpose
// Builds an extended attribute (EA) buffer
//
// Parameters
//   EaNameLength     - Length of the Extended attribute name
//   pEaName          - the extended attriute name
//   EaValueLength    - Length of the Extended attribute value
//   pEaValue         - the extended attribute value
//   pBuffer          - the buffer for constructing the EA
//
// Return Value
// status
//
// Remarks
//

NTSTATUS
TDI_BuildEaBuffer(
    IN  ULONG                     EaNameLength,
    IN  PVOID                     pEaName,
    IN  ULONG                     EaValueLength,
    IN  PVOID                     pEaValue,
    OUT PFILE_FULL_EA_INFORMATION *pEaBufferPointer,
    OUT PULONG                    pEaBufferLength
    )
{
   PFILE_FULL_EA_INFORMATION pEaBuffer;
   ULONG Length;

   //
   // Allocate an EA buffer for passing down the transport address
   //
   *pEaBufferLength = FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +
                      EaNameLength + 1 +
                      EaValueLength;

   pEaBuffer = (PFILE_FULL_EA_INFORMATION)ExAllocatePool(
                  PagedPool,
                  *pEaBufferLength
                  );

   if( pEaBuffer == NULL )
   {
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   *pEaBufferPointer = pEaBuffer;

   pEaBuffer->NextEntryOffset = 0;
   pEaBuffer->Flags           = 0;
   pEaBuffer->EaNameLength    = (UCHAR)EaNameLength;
   pEaBuffer->EaValueLength   = (USHORT)EaValueLength;

   RtlCopyMemory(
      pEaBuffer->EaName,
      pEaName,
      pEaBuffer->EaNameLength + 1
      );

   if( EaValueLength && pEaValue )
   {
      RtlCopyMemory(
         &pEaBuffer->EaName[EaNameLength + 1],
         pEaValue,
         EaValueLength
         );
   }

   return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
//// TDI_TransportAddressLength
//
// Purpose
// Computes the length in bytes of a transport address structure.
//
// Parameters
//   pTransportAddress - Pointer to the TRANSPORT_ADDESSS structure
//                       whose length is to be computed.
//
// Return Value
// The length of the instance in bytes
//
// Remarks
// Since this structure is packed the arithmetic has to be done using
// unaligned pointers.
//

ULONG
TDI_TransportAddressLength(
   PTRANSPORT_ADDRESS pTransportAddress
   )
{
   ULONG Size = 0;

   if( pTransportAddress != NULL )
   {
      LONG Index;

      TA_ADDRESS UNALIGNED *pTaAddress;

      Size  = FIELD_OFFSET(TRANSPORT_ADDRESS,Address) +
              FIELD_OFFSET(TA_ADDRESS,Address) * pTransportAddress->TAAddressCount;

      pTaAddress = (TA_ADDRESS UNALIGNED *)pTransportAddress->Address;

      for (Index = 0;Index <pTransportAddress->TAAddressCount;Index++)
      {
         Size += pTaAddress->AddressLength;
         pTaAddress = (TA_ADDRESS UNALIGNED *)((PCHAR)pTaAddress +
                                               FIELD_OFFSET(TA_ADDRESS,Address) +
                                               pTaAddress->AddressLength);
      }
   }

   return Size;
}

/////////////////////////////////////////////////////////////////////////////
//// TDI_InitIPAddress
//
// Purpose
//
// Parameters
//
// Return Value
//
// Remarks
//

VOID TDI_InitIPAddress(
   PTA_IP_ADDRESS pTransportAddress,
   ULONG          IPAddress,  // As 4-byte ULONG, Network Byte Order
   USHORT         IPPort      // Network Byte Order
   )
{
	pTransportAddress->TAAddressCount = 1;
	pTransportAddress->Address[ 0 ].AddressLength = TDI_ADDRESS_LENGTH_IP;
	pTransportAddress->Address[ 0 ].AddressType = TDI_ADDRESS_TYPE_IP;
	pTransportAddress->Address[ 0 ].Address[ 0 ].sin_port = IPPort;
	pTransportAddress->Address[ 0 ].Address[ 0 ].in_addr = IPAddress;
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_CreateSymbolicLink
//
// Purpose
// Creates or deletes a symbolic link for the specified DeviceName.
//
// Parameters
//
// Return Value
//
// Remarks
//

NTSTATUS
TDI_CreateSymbolicLink(
   IN PUNICODE_STRING   DeviceName,      // "\\Device\\TDITTCP"
   IN BOOLEAN           bCreate
   )
{
    UNICODE_STRING UnicodeDosDeviceName;
    NTSTATUS       status = STATUS_UNSUCCESSFUL;

   if( DeviceName->Length < sizeof(L"\\Device\\") )
   {
      return( STATUS_UNSUCCESSFUL );
   }

   NdisInitUnicodeString(&UnicodeDosDeviceName,NULL);

   UnicodeDosDeviceName.MaximumLength=DeviceName->Length+sizeof(L"\\DosDevices")+sizeof(UNICODE_NULL);

   UnicodeDosDeviceName.Buffer = ExAllocatePool(
                                    NonPagedPool,
                                    UnicodeDosDeviceName.MaximumLength
                                    );

   if (UnicodeDosDeviceName.Buffer != NULL)
   {
      NdisZeroMemory(
         UnicodeDosDeviceName.Buffer,
         UnicodeDosDeviceName.MaximumLength
         );

      RtlAppendUnicodeToString(
         &UnicodeDosDeviceName,
         L"\\DosDevices\\"
         );

      RtlAppendUnicodeToString(
         &UnicodeDosDeviceName,
         (DeviceName->Buffer+(sizeof("\\Device")))
         );

      KdPrint(("TDI_CreateSymbolicLink: %s %ws\n",
         bCreate ? "Creating" : "Deleting",
         UnicodeDosDeviceName.Buffer));

      if( bCreate )
      {
         status = IoCreateSymbolicLink( &UnicodeDosDeviceName, DeviceName );
      }
      else
      {
         status = IoDeleteSymbolicLink( &UnicodeDosDeviceName );
      }

      ExFreePool(UnicodeDosDeviceName.Buffer);
   }

   return status;
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_FreePacketAndBuffers
//
// Purpose
//
// Parameters
//
// Return Value
//
// Remarks
//

VOID TDI_FreePacketAndBuffers(
   PNDIS_PACKET Packet
   )
{
   UINT           nDataSize, nBufferCount;
   PNDIS_BUFFER   pBuffer;

   //
   // Sanity Checks
   //
   if( !Packet )
   {
      return;
   }

   //
   // Query Our Packet
   //
   NdisQueryPacket(
      Packet,
      (PUINT )NULL,
      (PUINT )&nBufferCount,
      &pBuffer,
      &nDataSize
      );

   //
   // Free All Of The Packet's Buffers
   //
   while( nBufferCount-- > 0L )
   {
      NdisUnchainBufferAtFront( Packet, &pBuffer );
      NdisFreeBuffer( pBuffer );
   }

   //
   // Relintialize The Packet
   //
   NdisReinitializePacket( Packet );

   //
   // Return The Packet To The Free List
   //
   NdisFreePacket( Packet );
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_IsBigEndian
//
// Purpose
// Runtime detection of processor byte ordering.
//
// Parameters
//
// Return Value
// Returns TRUE if the host processor uses BIG ENDIAN byte ordering.
// Returns FALSE if the host processor uses LITTLE ENDIAN byte ordering.
//
// Remarks
//

BOOLEAN TDI_IsBigEndian()
{
   static USHORT  _I_ENDIAN_TEST = 0xFF00;

   return( *(PUCHAR )&_I_ENDIAN_TEST );
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_htons
//
// Purpose
// Converts a USHORT from host to network byte order.
//
// Parameters
//
// Return Value
//
// Remarks
//

USHORT TDI_htons( USHORT hostshort )
{
	PUCHAR	pBuffer;
	USHORT	nResult;

   if( TDI_IsBigEndian() )
   {
      return( hostshort );
   }

	nResult = 0;
	pBuffer = (PUCHAR )&hostshort;

	nResult = ( (pBuffer[ 0 ] << 8) & 0xFF00 )
		| ( pBuffer[ 1 ] & 0x00FF );

	return( nResult );
}

/////////////////////////////////////////////////////////////////////////////
//// TDI_htonl
//
// Purpose
// Converts a ULONG from host to network byte order.
//
// Parameters
//
// Return Value
//
// Remarks
//

ULONG TDI_htonl( ULONG hostlong )
{
	PUCHAR pBuffer;
	ULONG	nResult;
	UCHAR	c, *pResult;

   if( TDI_IsBigEndian() )
   {
      return( hostlong );
   }

	pBuffer = (PUCHAR )&hostlong;

	if( !pBuffer )
	{
		return( 0L );
	}

	pResult = (UCHAR * )&nResult;

	c = ((UCHAR * )pBuffer)[ 0 ];
	((UCHAR * )pResult)[ 0 ] = ((UCHAR * )pBuffer)[ 3 ];
	((UCHAR * )pResult)[ 3 ] = c;

	c = ((UCHAR * )pBuffer)[ 1 ];
	((UCHAR * )pResult)[ 1 ] = ((UCHAR * )pBuffer)[ 2 ];
	((UCHAR * )pResult)[ 2 ] = c;

	return( nResult );
}

/////////////////////////////////////////////////////////////////////////////
//// TDI_ntohl
//
// Purpose
// Converts a ULONG from network to host byte order.
//
// Parameters
//
// Return Value
//
// Remarks
//

ULONG TDI_ntohl( ULONG netlong )
{
	return( TDI_htonl( netlong ) );
}

/////////////////////////////////////////////////////////////////////////////
//// TDI_ntohs
//
// Purpose
// Converts a USHORT from network to host byte order.
//
// Parameters
//
// Return Value
//
// Remarks
//

USHORT TDI_ntohs( USHORT netshort )
{
	return( TDI_htons( netshort ) );
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_in_cksum
//
// Purpose
// Standard Internet Protocol checksum routine.
//
// Parameters
//
// Return Value
//
// Remarks
// ICMP, IGMP, UDP and TCP all use the same checksum algorithm
//

USHORT TDI_in_cksum( PUCHAR pStartingByte, int nByteCount )
{
   register ULONG sum = 0;
   register PUSHORT addr = (PUSHORT )pStartingByte;

   //
   // Add 16-bit Words
   //
   while (nByteCount > 1)
   {
      //
      // This Is The Inner Loop
      //
      sum += *(addr++);
      nByteCount -= 2;
   }

   //
   // Add leftover byte, if any
   //
   if (nByteCount > 0)
   {
      if( TDI_IsBigEndian() )
      {
         sum += (*(PUCHAR )addr) << 8;
      }
      else
      {
         sum += *(PUCHAR )addr;
      }
   }

   //
   // Fold 32-bit sum to 16-bit
   //
   while (sum >> 16)
      sum = (sum & 0xffff) + (sum >> 16);

   //
   // Return one's compliment of final sum.
   //
   return (USHORT) ~sum;
}


//
// The name of the System process, in which context we're called in our 
// DriverEntry
//
#define SYSNAME    "System"

//
// This is the offset into a KPEB of the current process name. This is determined
// dynamically by scanning the process block belonging to the GUI for the name
// of the system process, in who's context we execute in DriverEntry
//
static ULONG  ProcessNameOffset = 0;

ULONG
TDI_InitProcessNameOffset( VOID )
{
   PEPROCESS   curproc;
   int         i;

   curproc = PsGetCurrentProcess();

   //
   // Scan for 12KB, hoping the KPEB never grows that big!
   //
   for( i = 0; i < 3*PAGE_SIZE; i++ )
   {
      if( !strncmp( SYSNAME, (PCHAR) curproc + i, strlen(SYSNAME) ))
      {
         ProcessNameOffset = i;
         return( ProcessNameOffset );
      }
    }

    //
    // Name not found - oh, well
    //
    return( 0 );
}


/////////////////////////////////////////////////////////////////////////////
//// TDI_GetCurrentProcessName
//
// Purpose
// Obtain the name of the currently executing process.
//
// Parameters
//
// Return Value
//   Returns TRUE if the current process name was successfully fetched.
//
// Remarks
// Based on code from the "Filemon" samples developed by Mark Russinovich
// and Bryce Cogswell. See the SysInternals web site at:
//
//   <http://www.sysinternals.com>.
//
// Uses undocumented data structure offsets to obtain the name of the
// currently executing process.
//

BOOLEAN
TDI_GetCurrentProcessName( PCHAR ProcessName )
{
   PEPROCESS   curproc;
   char        *nameptr;
   ULONG       i;
   BOOLEAN     bResult = FALSE;

   //
   // We only do this if we determined the process name offset
   //
   if( ProcessNameOffset )
   {
      //
      // Get a pointer to the current process block
      //
      curproc = PsGetCurrentProcess();

      //
      // Dig into it to extract the name 
      //
      nameptr = (PCHAR) curproc + ProcessNameOffset;

      strncpy( ProcessName, nameptr, NT_PROCNAMELEN );

      //
      // Terminate in case process name overflowed
      //
      ProcessName[NT_PROCNAMELEN] = 0;

      bResult = TRUE;
   }
   else
   {
      strcpy( ProcessName, "???" );
   }

   return( bResult );
}

/////////////////////////////////////////////////////////////////////////////
//                        D E B U G  F U N C T I O N S                     //
/////////////////////////////////////////////////////////////////////////////

#ifdef DBG

NTSTATUS
DEBUG_DumpProviderInfo(
   PWSTR pDeviceName,
   PTDI_PROVIDER_INFO pProviderInfo
   )
{
   KdPrint(("%ws ProviderInfo\n", pDeviceName));

   KdPrint(("  Version              : 0x%4.4X\n", pProviderInfo->Version ));
   KdPrint(("  MaxSendSize          : %d\n", pProviderInfo->MaxSendSize ));
   KdPrint(("  MaxConnectionUserData: %d\n", pProviderInfo->MaxConnectionUserData ));
   KdPrint(("  MaxDatagramSize      : %d\n", pProviderInfo->MaxDatagramSize ));
   KdPrint(("  ServiceFlags         : 0x%8.8X\n", pProviderInfo->ServiceFlags ));

   if( pProviderInfo->ServiceFlags & TDI_SERVICE_CONNECTION_MODE )
   {
      KdPrint(("  CONNECTION_MODE\n"));
   }
   if( pProviderInfo->ServiceFlags & TDI_SERVICE_ORDERLY_RELEASE )
   {
      KdPrint(("  ORDERLY_RELEASE\n"));
   }
   if( pProviderInfo->ServiceFlags & TDI_SERVICE_CONNECTIONLESS_MODE )
   {
      KdPrint(("  CONNECTIONLESS_MODE\n"));
   }
   if( pProviderInfo->ServiceFlags & TDI_SERVICE_ERROR_FREE_DELIVERY )
   {
      KdPrint(("  ERROR_FREE_DELIVERY\n"));
   }
   if( pProviderInfo->ServiceFlags & TDI_SERVICE_SECURITY_LEVEL )
   {
      KdPrint(("  SECURITY_LEVEL\n"));
   }
   if( pProviderInfo->ServiceFlags & TDI_SERVICE_BROADCAST_SUPPORTED )
   {
      KdPrint(("  BROADCAST_SUPPORTED\n"));
   }
   if( pProviderInfo->ServiceFlags & TDI_SERVICE_MULTICAST_SUPPORTED )
   {
      KdPrint(("  MULTICAST_SUPPORTED\n"));
   }
   if( pProviderInfo->ServiceFlags & TDI_SERVICE_DELAYED_ACCEPTANCE )
   {
      KdPrint(("  DELAYED_ACCEPTANCE\n"));
   }
   if( pProviderInfo->ServiceFlags & TDI_SERVICE_EXPEDITED_DATA )
   {
      KdPrint(("  EXPEDITED_DATA\n"));
   }
   if( pProviderInfo->ServiceFlags & TDI_SERVICE_INTERNAL_BUFFERING )
   {
      KdPrint(("  INTERNAL_BUFFERING\n"));
   }
   if( pProviderInfo->ServiceFlags & TDI_SERVICE_ROUTE_DIRECTED )
   {
      KdPrint(("  ROUTE_DIRECTED\n"));
   }
   if( pProviderInfo->ServiceFlags & TDI_SERVICE_NO_ZERO_LENGTH )
   {
      KdPrint(("  NO_ZERO_LENGTH\n"));
   }
   if( pProviderInfo->ServiceFlags & TDI_SERVICE_POINT_TO_POINT )
   {
      KdPrint(("  POINT_TO_POINT\n"));
   }
   if( pProviderInfo->ServiceFlags & TDI_SERVICE_MESSAGE_MODE )
   {
      KdPrint(("  MESSAGE_MODE\n"));
   }
   if( pProviderInfo->ServiceFlags & TDI_SERVICE_HALF_DUPLEX )
   {
      KdPrint(("  HALF_DUPLEX\n"));
   }

   KdPrint(("  MinimumLookaheadData : %d\n", pProviderInfo->MinimumLookaheadData ));
   KdPrint(("  MaximumLookaheadData : %d\n", pProviderInfo->MaximumLookaheadData ));
   KdPrint(("  NumberOfResources    : %d\n", pProviderInfo->NumberOfResources ));

   return( STATUS_SUCCESS );
}


NTSTATUS
DEBUG_DumpProviderStatistics(
   PWSTR pDeviceName,
   PTDI_PROVIDER_STATISTICS pProviderStatistics
   )
{
   KdPrint(("%ws ProviderStatistics\n", pDeviceName));

   KdPrint(("  Version               : 0x%4.4X\n", pProviderStatistics->Version ));
   KdPrint(("  OpenConnections       : 0x%4.4X\n", pProviderStatistics->OpenConnections ));

   KdPrint(("  DatagramsSent         : 0x%4.4X\n", pProviderStatistics->DatagramsSent ));
   KdPrint(("  DatagramBytesSent     : 0x%4.4X\n", pProviderStatistics->DatagramBytesSent ));
   
   KdPrint(("  DatagramsReceived     : 0x%4.4X\n", pProviderStatistics->DatagramsReceived ));
   KdPrint(("  DatagramBytesReceived : 0x%4.4X\n", pProviderStatistics->DatagramBytesReceived ));

   KdPrint(("  PacketsSent           : 0x%4.4X\n", pProviderStatistics->PacketsSent ));
   KdPrint(("  PacketsReceived       : 0x%4.4X\n", pProviderStatistics->PacketsReceived ));

   KdPrint(("  DataFramesSent        : 0x%4.4X\n", pProviderStatistics->DataFramesSent ));
   KdPrint(("  DataFrameBytesSent    : 0x%4.4X\n", pProviderStatistics->DataFrameBytesSent ));

   KdPrint(("  DataFramesReceived    : 0x%4.4X\n", pProviderStatistics->DataFramesReceived ));
   KdPrint(("  DataFrameBytesReceived: 0x%4.4X\n", pProviderStatistics->DataFrameBytesReceived ));
   
   return( STATUS_SUCCESS );
}


VOID
DEBUG_DumpTAAddress(
   USHORT nAddressType,			// e.g., TDI_ADDRESS_TYPE_IP
   PUCHAR pTAAddr
   )
{
	if( nAddressType == TDI_ADDRESS_TYPE_IP )
	{
		PTDI_ADDRESS_IP	pIPAddress;
		PUCHAR				pByte;

		pIPAddress = (PTDI_ADDRESS_IP )pTAAddr;

		pByte = (PUCHAR )&pIPAddress->in_addr;

		KdPrint(( "in_addr: %d.%d.%d.%d; ",
			pByte[ 0 ], pByte[ 1 ], pByte[ 2 ], pByte[ 3 ] ));

		KdPrint(( "sin_port: %d (0x%2.2X)\n",
         TDI_ntohs( pIPAddress->sin_port ),
         TDI_ntohs( pIPAddress->sin_port )
         ));
	}
}

VOID
DEBUG_DumpTransportAddress( PTRANSPORT_ADDRESS pTransAddr )
{
	KdPrint(( "TAAddressCount: %d\n", pTransAddr->TAAddressCount ));
	KdPrint(( "AddressLength: %d\n", pTransAddr->Address[ 0 ].AddressLength ));
	KdPrint(( "AddressType: %d\n", pTransAddr->Address[ 0 ].AddressType ));

	DEBUG_DumpTAAddress(
      pTransAddr->Address[ 0 ].AddressType,
      (PUCHAR )&pTransAddr->Address[ 0 ].Address
      );
}

VOID
DEBUG_DumpConnectionInfo( PTDI_CONNECTION_INFO pConnectionInfo )
{
   KdPrint(( "State: 0x%8.8x\n", pConnectionInfo->State ));   // current state of the connection
   KdPrint(( "Last Event: 0x%8.8X\n", pConnectionInfo->Event )); // last event on the connection
   KdPrint(( "TransmittedTsdus: %d\n", pConnectionInfo->TransmittedTsdus ));             // TSDUs sent on this connection
   KdPrint(( "ReceivedTsdus: %d\n", pConnectionInfo->ReceivedTsdus ));                // TSDUs received on this connection
   KdPrint(( "TransmissionErrors: %d\n", pConnectionInfo->TransmissionErrors ));           // TSDUs transmitted in error/this connection
   KdPrint(( "ReceiveErrors: %d\n", pConnectionInfo->ReceiveErrors ));                // TSDUs received in error/this connection.

   KdPrint(( "The Estimated Throughput = %I64X\n",pConnectionInfo->Throughput.QuadPart));
   KdPrint(( "The Estimated Delay = %I64X\n",pConnectionInfo->Delay.QuadPart));
//    LARGE_INTEGER pConnectionInfo->Throughput;           // estimated throughput on this connection.
//    LARGE_INTEGER pConnectionInfo->Delay;                // estimated delay on this connection.
   KdPrint(( "SendBufferSize: %d\n", pConnectionInfo->SendBufferSize ));               // size of buffer for sends - only
                                        // meaningful for internal buffering
                                        // protocols like tcp
   KdPrint(( "ReceiveBufferSize: %d\n", pConnectionInfo->ReceiveBufferSize ));            // size of buffer for receives - only
                                        // meaningful for internal buffering
                                        // protocols like tcp
   KdPrint(( "Unreliable: %s\n", pConnectionInfo->Unreliable ? "TRUE" : "FALSE" ));                 // is this connection "unreliable".
}

VOID
DEBUG_DumpAddressInfo( PTDI_ADDRESS_INFO pAddressInfo )
{
   KdPrint(( "ActivityCount: 0x%8.8x\n", pAddressInfo->ActivityCount ));   // outstanding open file objects/this address
   DEBUG_DumpTransportAddress( &pAddressInfo->Address );// the actual address & its components
}

#endif // DBG



