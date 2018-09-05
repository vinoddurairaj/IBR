/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include "ndis.h"

#include "INetInc.h"
#include "TDITTCP.h"

// Copyright And Configuration Management ----------------------------------
//
//             TDI Test (TTCP) Tcp Server Device - TCPSERVER.c
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
//// TDI TTCP TCP Server GLOBAL DATA
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
static BOOLEAN       g_bSymbolicLinkCreated = FALSE;
static LIST_ENTRY    g_TCPServerList;
static KEVENT        g_TCPS_KillEvent;

/////////////////////////////////////////////////////////////////////////////
//// LOCAL STRUCTURE DEFINITIONS

typedef
struct _TCPS_SERVER
{
   LIST_ENTRY           m_ListElement;

   HANDLE               m_hTestThread;

   TDITTCP_TEST_PARAMS  m_TestParams;

   TA_IP_ADDRESS        m_LocalAddress;   // TDI Address
   KS_ADDRESS           m_KS_Address;

   LIST_ENTRY			   m_ActiveSessionList;
   LIST_ENTRY			   m_FreeSessionList;
}
   TCPS_SERVER, *PTCPS_SERVER;


typedef
struct _TCPS_SESSION
{
   LIST_ENTRY           m_ListElement;

   PTCPS_SERVER         m_pServer;        // Parent Server

   //
   // Local Connection Endpoint
   // -------------------------
   // The KS_ENDPOINT structure contains information that defines:
   //   TDI Address Object
   //   TDI Connection Context
   //
   // Together these two objects represent the local connection endpoint.
   //
   KS_ENDPOINT                m_KS_Endpoint;

	TA_IP_ADDRESS		         m_RemoteAddress;
	TDI_CONNECTION_INFORMATION m_RemoteConnectionInfo;

   ULONG             m_ReceiveBufferSize;
   PUCHAR            m_pReceiveBuffer;
   PMDL              m_pReceiveMdl;
   IO_STATUS_BLOCK   m_pReceiveIoStatus;

   PIRP              m_pListenIrp;

   ULONG             m_nBytesReceived;
}
   TCPS_SESSION, *PTCPS_SESSION;


/////////////////////////////////////////////////////////////////////////////
//// LOCAL PROCEDURE PROTOTYPES

NTSTATUS
TCPS_AllocateSession(
   PTCPS_SERVER   pServer,
   PTCPS_SESSION  *hSession
   );

VOID
TCPS_FreeSession(
   PTCPS_SESSION  pSession
   );

VOID
TCPS_ReceiveCompletion(
    PVOID UserCompletionContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG Reserved
    );

/////////////////////////////////////////////////////////////////////////////
//// TCPS_DeviceLoad
//
// Purpose
// This routine initializes the TDI TTCP TCP Server device.
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
TCPS_DeviceLoad(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
   UNICODE_STRING UnicodeDeviceName;
   PDEVICE_OBJECT pDeviceObject = NULL;
   PDEVICE_EXTENSION pDeviceExtension = NULL;
   NTSTATUS Status = STATUS_SUCCESS;
   NTSTATUS ErrorCode = STATUS_SUCCESS;

   KdPrint(("TCPS_DeviceLoad: Entry...\n") );

   //
   // Initialize Global Data
   //

   //
   // Set up the driver's device entry points.
   //
   pDriverObject->MajorFunction[IRP_MJ_CREATE] = TDITTCPDeviceOpen;
   pDriverObject->MajorFunction[IRP_MJ_CLOSE]  = TDITTCPDeviceClose;
   pDriverObject->MajorFunction[IRP_MJ_READ]   = TDITTCPDeviceRead;
   pDriverObject->MajorFunction[IRP_MJ_WRITE]  = TDITTCPDeviceWrite;
   pDriverObject->MajorFunction[IRP_MJ_CLEANUP]  = TDITTCPDeviceCleanup;
   pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = TDITTCPDeviceIoControl;

   pDriverObject->DriverUnload = TDITTCPDriverUnload;

   //
   // Create The TDI TTCP TCP Server Device
   //
   NdisInitUnicodeString(
      &UnicodeDeviceName,
      TDI_TCP_SERVER_DEVICE_NAME_W
      );

   Status = IoCreateDevice(
               pDriverObject,
               sizeof(DEVICE_EXTENSION),
               &UnicodeDeviceName,
               FILE_DEVICE_TCP_SERVER,
               0,            // Standard device characteristics
               FALSE,      // This isn't an exclusive device
               &pDeviceObject
               );

   if( !NT_SUCCESS( Status ) )
   {
      KdPrint(("TDITTCP: IoCreateDevice() failed:\n") );

      Status = STATUS_UNSUCCESSFUL;

      return(Status);
   }

   //
   // Create The TDI TTCP TCP Server Device Symbolic Link
   //
   Status = KS_CreateSymbolicLink(
               &UnicodeDeviceName,
               TRUE
               );

   if( NT_SUCCESS (Status ) )
   {
      g_bSymbolicLinkCreated = TRUE;
   }

   pDeviceObject->Flags |= DO_DIRECT_IO;   // Effects Read/Write Only...

   //
   // Initialize The Device Extension
   //
   pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

   RtlZeroMemory( pDeviceExtension, sizeof(DEVICE_EXTENSION) );

   pDeviceExtension->pDeviceObject = pDeviceObject;

   //
   // Setup The Driver Device Entry Points
   //
   pDeviceExtension->MajorFunction[IRP_MJ_CREATE] = TCPS_DeviceOpen;
   pDeviceExtension->MajorFunction[IRP_MJ_CLOSE]  = TCPS_DeviceClose;
   pDeviceExtension->MajorFunction[IRP_MJ_READ]   = NULL;
   pDeviceExtension->MajorFunction[IRP_MJ_WRITE]  = NULL;
   pDeviceExtension->MajorFunction[IRP_MJ_CLEANUP]  = TCPS_DeviceCleanup;
   pDeviceExtension->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = TCPS_DeviceIoControl;

   pDeviceExtension->DeviceUnload = TCPS_DeviceUnload;

   //
   // Fetch Transport Provider Information
   //
   Status = KS_QueryProviderInfo(
               TCP_DEVICE_NAME_W,
               &pDeviceExtension->TcpServerContext.TdiProviderInfo
               );

#ifdef DBG
   if (NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderInfo(
         TCP_DEVICE_NAME_W,
         &pDeviceExtension->TcpServerContext.TdiProviderInfo
         );
   }
#endif // DBG

   InitializeListHead( &g_TCPServerList );

   //
   // Initialize The Sever Kill Event
   //
   KeInitializeEvent(
      &g_TCPS_KillEvent,
      NotificationEvent,
      FALSE
      );

   return STATUS_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
//// TCPS_DeviceOpen (IRP_MJ_CREATE Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDI TTCP TCP Server device create/open
// requests.
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
TCPS_DeviceOpen(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   KdPrint(("TCPS_DeviceOpen: Entry...\n") );

   pDeviceExtension = pDeviceObject->DeviceExtension;

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

   KdPrint( ("TCPS_DeviceOpen: Opened!!\n") );

   pIrp->IoStatus.Information = 0;

   TdiCompleteRequest( pIrp, STATUS_SUCCESS );

   return STATUS_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
//// TCPS_DeviceClose (IRP_MJ_CLOSE Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDI TTCP TCP Server device close requests.
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
TCPS_DeviceClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   KdPrint(("TCPS_DeviceClose: Entry...\n") );

   pDeviceExtension = pDeviceObject->DeviceExtension;

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

   KdPrint( ("TCPS_DeviceClose: Closed!!\n") );

   pIrp->IoStatus.Information = 0;

   TdiCompleteRequest( pIrp, STATUS_SUCCESS );

   return STATUS_SUCCESS;
}


NTSTATUS
TCPS_ConnectedCallback(
   IN PDEVICE_OBJECT pDeviceObject,
   IN PIRP pIrp,
   IN PVOID Context
   )
{
   PTCPS_SESSION  pSession = (PTCPS_SESSION)Context;
   PTCPS_SERVER   pServer = pSession->m_pServer;
	NTSTATUS Status = pIrp->IoStatus.Status;

	KdPrint(( "TCPS_ConnectedCallback: FinalStatus: 0x%8.8x\n", Status ));

   RemoveEntryList( &pSession->m_ListElement );

   if( NT_SUCCESS( Status ) )
   {
#if DBG
	   DEBUG_DumpTransportAddress(
         (PTRANSPORT_ADDRESS )&pSession->m_RemoteAddress
         );
#endif

      //
	   // Start The First Receive
      //
      pSession->m_nBytesReceived = 0;

      //
	   // Start The First Receive On The Session
      //
      (pSession->m_pReceiveMdl)->Next = NULL;   // IMPORTANT!!!

      Status = KS_ReceiveOnEndpoint(
                  &pSession->m_KS_Endpoint,
                  NULL,       // User Completion Event
                  TCPS_ReceiveCompletion,// User Completion Routine
                  pSession,   // User Completion Context
                  &pSession->m_pReceiveIoStatus,
                  pSession->m_pReceiveMdl,
                  0           // Flags
                  );
   }

	return( STATUS_MORE_PROCESSING_REQUIRED );
}

VOID
TCPS_DisconnectCallback(
    PVOID UserCompletionContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG Reserved
   )
{
   PTCPS_SESSION  pSession = (PTCPS_SESSION)UserCompletionContext;
   PTCPS_SERVER   pServer = pSession->m_pServer;
	NTSTATUS       Status = IoStatusBlock->Status;
   PDEVICE_OBJECT pDeviceObject;

	KdPrint(( "TCPS_DisconnectCallback: FinalStatus: 0x%8.8x\n", Status ));

   //
   // Start Another Listen On The Session
   //
   // ATTENTION!!! Check For Shutdown Status!!!

   pDeviceObject = IoGetRelatedDeviceObject(
                     pSession->m_KS_Endpoint.m_pFileObject
                     );

   TdiBuildListen(
      pSession->m_pListenIrp,
      pDeviceObject,
      pSession->m_KS_Endpoint.m_pFileObject,
      TCPS_ConnectedCallback, // Completion Routine
      pSession,               // Completion Context
      0,                      // Flags
//      TDI_QUERY_ACCEPT,       // Flags
      NULL,                   // Request Connection Info
		&pSession->m_RemoteConnectionInfo
      );

   Status = IoCallDriver( pDeviceObject, pSession->m_pListenIrp );

   //
   // Add The Session To The Free Session List
   //
	InsertTailList(
		&pServer->m_FreeSessionList,
		&pSession->m_ListElement
		);
}

VOID
TCPS_ReceiveCompletion(
    PVOID UserCompletionContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG Reserved
    )
{
   PTCPS_SESSION  pSession = (PTCPS_SESSION )UserCompletionContext;
   PTCPS_SERVER   pServer = (PTCPS_SERVER )pSession->m_pServer;
   NTSTATUS       Status = IoStatusBlock->Status;
	ULONG          nByteCount = IoStatusBlock->Information;

   if( NT_SUCCESS( Status ) )
   {
      KdPrint(( "TCPS_ReceiveCompletion: Received %d Bytes\n", nByteCount ));
   }
   else
   {
      KdPrint(( "TCPS_ReceiveCompletion: Status 0x%8.8X\n", Status ));
   }

   //
	// Check On State
	//

   //
	// Handle Disconnect Or Transfer Failure
   //
//	if( !NT_SUCCESS( Status ) || !nByteCount )
	if( !nByteCount )
	{
      //
      // Build Disconnect Call
      //
      Status = KS_Disconnect(
                  &pSession->m_KS_Endpoint,
                  NULL,       // UserCompletionEvent
                  TCPS_DisconnectCallback,    // UserCompletionRoutine
                  pSession,   // UserCompletionContext
                  NULL,       // pIoStatusBlock
                  0           // Disconnect Flags
                  );

		return;
   }

   //
   // Update Statistics
   //
   pSession->m_nBytesReceived += nByteCount;

   //
	// Start Another Receive On Same Buffer
   //
   (pSession->m_pReceiveMdl)->Next = NULL;   // IMPORTANT!!!

   Status = KS_ReceiveOnEndpoint(
               &pSession->m_KS_Endpoint,
               NULL,       // User Completion Event
               TCPS_ReceiveCompletion,// User Completion Routine
               pSession,   // User Completion Context
               &pSession->m_pReceiveIoStatus,
               pSession->m_pReceiveMdl,
               0           // Flags
               );
}


/////////////////////////////////////////////////////////////////////////////
//                    T D I  E V E N T  H A N D L E R S                    //
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//// TCPS_DisconnectEventHandler (Session Event Handler)
//
// Purpose
//
// Parameters
//   TdiEventContext - Pointer to TCPS_SESSION structure for the session.
//
// Return Value
//
// Remarks
// Disconnection indication prototype. This is invoked when a connection is
// being disconnected for a reason other than the user requesting it. Note
// that this is a change from TDI V1, which indicated only when the remote
// caused a disconnection. Any non-directed disconnection will cause this
// indication.
//

NTSTATUS TCPS_DisconnectEventHandler(
   IN PVOID TdiEventContext,     // Context From SetEventHandler
   IN CONNECTION_CONTEXT ConnectionContext,
   IN LONG DisconnectDataLength,
   IN PVOID DisconnectData,
   IN LONG DisconnectInformationLength,
   IN PVOID DisconnectInformation,
   IN ULONG DisconnectFlags
   )
{
   KdPrint(("TCPS_DisconnectEventHandler: Entry; EventContext: 0x%8.8X; ConnectionContext: 0x%8.8X; Flags: 0x%8.8X\n",
      (ULONG )TdiEventContext, (ULONG)ConnectionContext, DisconnectFlags)
      );

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// TCPS_ServerErrorEventHandler (Server Event Handler)
//
// Purpose
//
// Parameters
//   TdiEventContext - Pointer to TCPS_SERVER structure for the server.
//
// Return Value
//
// Remarks
// A protocol error has occurred when this indication happens. This indication
// occurs only for errors of the worst type; the address this indication is
// delivered to is no longer usable for protocol-related operations, and
// should not be used for operations henceforth. All connections associated
// it are invalid.
// For NetBIOS-type providers, this indication is also delivered when a name
// in conflict or duplicate name occurs.
//

NTSTATUS TCPS_ServerErrorEventHandler(
   IN PVOID TdiEventContext,  // The endpoint's file object.
   IN NTSTATUS Status         // Status code indicating error type.
   )
{
   KdPrint(("TCPS_ServerErrorEventHandler: Status: 0x%8.8X\n", Status) );

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// TCPS_SessionErrorEventHandler (Server Event Handler)
//
// Purpose
//
// Parameters
//   TdiEventContext - Pointer to TCPS_SERVER structure for the server.
//
// Return Value
//
// Remarks
// A protocol error has occurred when this indication happens. This indication
// occurs only for errors of the worst type; the address this indication is
// delivered to is no longer usable for protocol-related operations, and
// should not be used for operations henceforth. All connections associated
// it are invalid.
// For NetBIOS-type providers, this indication is also delivered when a name
// in conflict or duplicate name occurs.
//

NTSTATUS TCPS_SessionErrorEventHandler(
   IN PVOID TdiEventContext,  // The endpoint's file object.
   IN NTSTATUS Status         // Status code indicating error type.
   )
{
   KdPrint(("TCPS_SessionErrorEventHandler: Status: 0x%8.8X\n", Status) );

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// TCPS_ReceiveExpeditedEventHandler (Session Event Handler)
//
// Purpose
//
// Parameters
//   TdiEventContext - Pointer to TCPS_SESSION structure for the session.
//
// Return Value
//
// Remarks
// This indication is delivered if expedited data is received on the connection.
// This will only occur in providers that support expedited data.
//

NTSTATUS TCPS_ReceiveExpeditedEventHandler(
   IN PVOID TdiEventContext,     // Context From SetEventHandler
   IN CONNECTION_CONTEXT ConnectionContext,
   IN ULONG ReceiveFlags,          //
   IN ULONG BytesIndicated,        // number of bytes in this indication
   IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
   OUT ULONG *BytesTaken,          // number of bytes used by indication routine
   IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
   OUT PIRP *IoRequestPacket        // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
   )
{
   KdPrint(("TCPS_ReceiveExpeditedEventHandler: Entry...\n") );

   KdPrint(("  Bytes Indicated: %d; BytesAvailable: %d; Flags: 0x%8.8x\n",
      BytesIndicated, BytesAvailable, ReceiveFlags));

   return( STATUS_SUCCESS );
}

/////////////////////////////////////////////////////////////////////////////
//// TCPS_FreeSession
//
// Purpose
//
// Parameters
//
// Return Value
//
// Remarks
// Callers of TCPS_FreeSession must be running at IRQL PASSIVE_LEVEL.
//

VOID
TCPS_FreeSession(
   PTCPS_SESSION  pSession
   )
{
   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

   if( pSession )
   {
      KS_CloseConnectionEndpoint( &pSession->m_KS_Endpoint );

      if( pSession->m_pListenIrp )
      {
         IoFreeIrp( pSession->m_pListenIrp );
      }

      if( pSession->m_pReceiveMdl )
      {
         KS_UnlockAndFreeMdl( pSession->m_pReceiveMdl );
      }

      pSession->m_pReceiveMdl = NULL;

      if( pSession->m_pReceiveBuffer )
      {
         NdisFreeMemory(
            pSession->m_pReceiveBuffer,
            pSession->m_ReceiveBufferSize,
            0
            );
      }

      pSession->m_pReceiveBuffer = NULL;

      NdisFreeMemory( pSession, sizeof( TCPS_SESSION ), 0 );
   }
}

/////////////////////////////////////////////////////////////////////////////
//// TCPS_AllocateSession
//
// Purpose
//
// Parameters
//
// Return Value
//
// Remarks
// Callers of TCPS_AllocateSession must be running at IRQL PASSIVE_LEVEL.
//

NTSTATUS
TCPS_AllocateSession(
   PTCPS_SERVER   pServer,
   PTCPS_SESSION  *hSession
   )
{
   NTSTATUS       Status;
   PTCPS_SESSION  pSession;
   PDEVICE_OBJECT pDeviceObject;

   ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

   //
   // Allocate Session Memory
   //
   Status = NdisAllocateMemory(
               &pSession,
               sizeof( TCPS_SESSION ),
               0,       // Allocate non-paged system-space memory
               HighestAcceptableMax
               );

   if( !NT_SUCCESS( Status ) )
   {
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   NdisZeroMemory( pSession, sizeof( TCPS_SESSION ) );

   pSession->m_pServer = pServer;

   //
   // Allocate The Receive Buffer
   //
   pSession->m_ReceiveBufferSize = (pSession->m_pServer)->m_TestParams.m_nBufferSize;

   Status = NdisAllocateMemory(
               &pSession->m_pReceiveBuffer,
               pSession->m_ReceiveBufferSize,
               0,       // Allocate non-paged system-space memory
               HighestAcceptableMax
               );

   if( !NT_SUCCESS( Status ) )
   {
      TCPS_FreeSession( pSession );

      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   NdisZeroMemory(
      pSession->m_pReceiveBuffer,
      pSession->m_ReceiveBufferSize
      );

   pSession->m_pReceiveMdl = KS_AllocateAndProbeMdl(
                              pSession->m_pReceiveBuffer,   // Virtual Address
                              pSession->m_ReceiveBufferSize,
                              FALSE,
                              FALSE,
                              NULL
                              );

   if( !pSession->m_pReceiveMdl )
   {
      TCPS_FreeSession( pSession );

      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   //
   // Create The Connection Endpoint
   //
   Status = KS_OpenConnectionEndpoint(
                  TCP_DEVICE_NAME_W,
                  &pServer->m_KS_Address,
                  &pSession->m_KS_Endpoint,
                  &pSession->m_KS_Endpoint    // Context
                  );

   // BUGBUG!!! Resources Aren't Being Freed Systematically!!!
   // Use TCPS_FreeSession....

   if( !NT_SUCCESS( Status ) )
   {
      //
      // Connection Endpoint Could Not Be Created
      //
      KS_CloseConnectionEndpoint( &pSession->m_KS_Endpoint );

      NdisFreeMemory( pSession, sizeof( TCPS_SESSION ), 0 );

      return( Status );
   }

   KdPrint(("TCPS_AllocateSession: Created Local TDI Connection Endpoint\n") );

   pDeviceObject = IoGetRelatedDeviceObject(
                     pSession->m_KS_Endpoint.m_pFileObject
                     );
   //
   // Allocate Irp For Use In Listening For A Connection
   //
   pSession->m_pListenIrp = IoAllocateIrp(
                              pDeviceObject->StackSize,
                              FALSE
                              );

   if( !pSession->m_pListenIrp )
   {
      KS_CloseConnectionEndpoint( &pSession->m_KS_Endpoint );
      return( STATUS_INSUFFICIENT_RESOURCES );
   }

   //
	// Build The Listen Request
   //
	pSession->m_RemoteConnectionInfo.UserDataLength = 0;
	pSession->m_RemoteConnectionInfo.UserData = NULL;

	pSession->m_RemoteConnectionInfo.OptionsLength = 0;
	pSession->m_RemoteConnectionInfo.Options = NULL;

	pSession->m_RemoteConnectionInfo.RemoteAddressLength = sizeof( TA_IP_ADDRESS );
	pSession->m_RemoteConnectionInfo.RemoteAddress = &pSession->m_RemoteAddress;

   //
   // Start Listening On The Session
   //
   TdiBuildListen(
      pSession->m_pListenIrp,
      pDeviceObject,
      pSession->m_KS_Endpoint.m_pFileObject,
      TCPS_ConnectedCallback, // Completion Routine
      pSession,               // Completion Context
      0,                      // Flags
      NULL,                   // Request Connection Info
		&pSession->m_RemoteConnectionInfo
      );

   Status = IoCallDriver( pDeviceObject, pSession->m_pListenIrp );

   *hSession = pSession;

   KdPrint(( "Created Session: 0x%8.8X\n", (ULONG )pSession ));

   return( Status );
}


/////////////////////////////////////////////////////////////////////////////
//// TCPS_FreeSessionPool
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
TCPS_FreeSessionPool(
   PTCPS_SERVER   pServer
   )
{
   PTCPS_SESSION  pSession;

   while( !IsListEmpty( &pServer->m_FreeSessionList ) )
   {
      pSession = (PTCPS_SESSION )RemoveHeadList( &pServer->m_FreeSessionList );

      TCPS_FreeSession( pSession );
   }
}


/////////////////////////////////////////////////////////////////////////////
//// TCPS_AllocateSessionPool
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
TCPS_AllocateSessionPool(
   PTCPS_SERVER   pServer,
   INT            nNumberOfSessions    // Number Of Sessions To Allocate
   )
{
   PTCPS_SESSION  pSession;
   NTSTATUS       Status;
   INT            i, NumAllocated = 0;

   for( i = 0; i < nNumberOfSessions; ++i )
   {
      Status = TCPS_AllocateSession(
                  pServer,
                  &pSession
                  );

      if( !NT_SUCCESS( Status ) )
      {
         break;
      }

      //
      // Add The Session To The Free Session List
      //
		InsertTailList(
			&pServer->m_FreeSessionList,
			&pSession->m_ListElement
			);

      ++NumAllocated;
   }

   return( NumAllocated > 0 ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL );
}


/////////////////////////////////////////////////////////////////////////////
//// TCPS_TestThread
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
TCPS_TestThread(
   IN PVOID pContext
   )
{
   NTSTATUS             Status = STATUS_SUCCESS;
   NDIS_STATUS          nNdisStatus;
   PTTCP_TEST_START_CMD pStartCmd = NULL;
   PTCPS_SERVER         pServer = (PTCPS_SERVER )pContext;
   LARGE_INTEGER        DelayTime;

   KdPrint(("TCPS_TestThread: Starting...\n") );

   //
   // Initialize Default Settings
   //
   Status = STATUS_SUCCESS;      // Always Indicate I/O Success

   //
   // Locate Test Session Parameter Buffer
   //
   pServer = (PTCPS_SERVER )pContext;

   //
   // Perform Additional Initialization Of TCPS_SERVER Structure
   //
   InitializeListHead( &pServer->m_ActiveSessionList );
   InitializeListHead( &pServer->m_FreeSessionList );

   //
   // Setup Local TDI Address
   //
   KS_InitIPAddress(
      &pServer->m_LocalAddress,
      INADDR_ANY,                   // Any Local Address
      pServer->m_TestParams.m_Port  // Specific Port
      );

   //
   // Open Transport Address
   //
   Status = KS_OpenTransportAddress(
                  TCP_DEVICE_NAME_W,
                  (PTRANSPORT_ADDRESS )&pServer->m_LocalAddress,
                  &pServer->m_KS_Address
                  );

   if( !NT_SUCCESS( Status ) )
   {
      //
      // Address Object Could Not Be Created
      //
      goto ExitTheThread;
   }

   //
   // Setup Event Handlers On The Server Address Object
   //
   Status = KS_SetEventHandlers(
                  &pServer->m_KS_Address,
                  pServer,       // Event Context
                  NULL,          // ConnectEventHandler
                  TCPS_DisconnectEventHandler,
                  TCPS_SessionErrorEventHandler,
                  NULL,          // ReceiveEventHandler,
                  NULL,          // ReceiveDatagramEventHandler
                  TCPS_ReceiveExpeditedEventHandler
                  );

   if( !NT_SUCCESS( Status ) )
   {
      //
      // Event Handlers Could Not Be Set
      //
      goto ExitTheThread;
   }

   KdPrint(("TCPS_TestThread: Set Event Handlers On The Server Address Object\n") );

   //
   // Allocate Free Sessions, Used To Accept Connections
   //
//   Status = TCPS_AllocateSessionPool( pServer, 4 );
   Status = TCPS_AllocateSessionPool( pServer, 1 );

   if( !NT_SUCCESS( Status ) )
   {
      goto ExitTheThread;
   }

   KdPrint(("TCPS_TestThread: Session Pool Allocated\n") );

   //
   // Add The Server To The Active Server List
   //
	InsertTailList(
		&g_TCPServerList,
		&pServer->m_ListElement
		);

   Status = KeWaitForSingleObject(
               &g_TCPS_KillEvent,  // Object to wait on.
               Executive,  // Reason for waiting
               KernelMode, // Processor mode
               FALSE,      // Alertable
               NULL        // Timeout
               );

   //
   // ATTENTION!!! Test Exit...
   //

   KdPrint(( "Waiting After Disconnect...\n" ));

   DelayTime.QuadPart = 10*1000*1000*5;   // 5 Seconds

   KeDelayExecutionThread( KernelMode, FALSE, &DelayTime );

   //
   // Exit The Thread
   //
ExitTheThread:

   KdPrint(("TCPS_TestThread: Exiting...\n") );

   RemoveEntryList( &pServer->m_ListElement );

   //
   // Close Active Sessions
   //

   //
   // Free The Free Session Pool
   //
   ASSERT( IsListEmpty( &pServer->m_ActiveSessionList ) );

   TCPS_FreeSessionPool( pServer );

   ASSERT( IsListEmpty( &pServer->m_FreeSessionList ) );

   //
   // Close The Transport Address
   //
   KS_CloseTransportAddress( &pServer->m_KS_Address );

   NdisFreeMemory( pServer, sizeof( TCPS_SERVER ), 0 );

   (void)PsTerminateSystemThread( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// TCPS_StartTest
//
// Purpose
// Start A TTCP TCP Server Test
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
TCPS_StartTest(
   IN PDEVICE_OBJECT pDeviceObject,
   IN PIRP pIrp
   )
{
   NDIS_STATUS          nNdisStatus;
   PTTCP_TEST_START_CMD pStartCmd = NULL;
   PTCPS_SERVER         pServer = NULL;

   //
   // Initialize Default Settings
   //
   pIrp->IoStatus.Information = sizeof( ULONG );  // For m_Status

   //
   // Locate Test Start Command Buffer
   //
   pStartCmd = (PTTCP_TEST_START_CMD )pIrp->AssociatedIrp.SystemBuffer;

   pStartCmd->m_Status = STATUS_UNSUCCESSFUL;

   TDITTCP_DumpTestParams( &pStartCmd->m_TestParams );

   //
   // Allocate Memory For The Test Session
   //
   nNdisStatus = NdisAllocateMemory(
                  &pServer,
                  sizeof( TCPS_SERVER ),
                  0,       // Allocate non-paged system-space memory
                  HighestAcceptableMax
                  );

   if( !NT_SUCCESS( nNdisStatus ) )
   {
      return( nNdisStatus );
   }

   NdisZeroMemory( pServer, sizeof( TCPS_SERVER ) );

   NdisMoveMemory(
      &pServer->m_TestParams,
      &pStartCmd->m_TestParams,
      sizeof( TDITTCP_TEST_PARAMS )
      );

   //
   // Start The Thread That Will Execute The Test
   //
   PsCreateSystemThread(
      &pServer->m_hTestThread,   // thread handle
      0L,                        // desired access
      NULL,                      // object attributes
      NULL,                      // process handle
      NULL,                      // client id
      TCPS_TestThread,           // start routine
      (PVOID )pServer            // start context
      );

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// TCPS_DeviceIoControl (IRP_MJ_DEVICE_CONTROL Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDI TTCP TCP Server device IOCTL requests.
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
TCPS_DeviceIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;
   NTSTATUS             Status;
   PIO_STACK_LOCATION   pIrpSp;
   ULONG                nFunctionCode;

   KdPrint(("TCPS_DeviceIoControl: Entry...\n") );

   pDeviceExtension = pDeviceObject->DeviceExtension;

   pIrp->IoStatus.Information = 0;      // Nothing Returned Yet

   pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

   nFunctionCode=pIrpSp->Parameters.DeviceIoControl.IoControlCode;

   switch( nFunctionCode )
   {
      case IOCTL_TCP_SERVER_START_TEST:
         Status = TCPS_StartTest( pDeviceObject, pIrp );
         break;

      default:
         KdPrint((  "FunctionCode: 0x%8.8X\n", nFunctionCode ));
         Status = STATUS_UNSUCCESSFUL;
         break;
   }

   TdiCompleteRequest( pIrp, Status );

   return( Status );
}


/////////////////////////////////////////////////////////////////////////////
//// TCPS_DeviceCleanup (IRP_MJ_CLEANUP Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDI TTCP TCP Server device cleanup
// requests.
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
TCPS_DeviceCleanup(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   KdPrint(("TCPS_DeviceCleanup: Entry...\n"));

   pDeviceExtension = pDeviceObject->DeviceExtension;

   pIrp->IoStatus.Status = STATUS_SUCCESS;
   pIrp->IoStatus.Information = 0;

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// TCPS_DeviceUnload
//
// Purpose
//
// Parameters
//   pDeviceObject - Pointer to device object created by system.
//
// Return Value
//
// Remarks
//

VOID
TCPS_DeviceUnload(
   IN PDEVICE_OBJECT pDeviceObject
   )
{
   PDEVICE_EXTENSION pDeviceExtension;
   PTCPS_SERVER      pServer = NULL;
   KEVENT            TCPS_UnloadEvent;
   LARGE_INTEGER     UnloadWait;
   NTSTATUS          Status;

   KdPrint(("TCPS_DeviceUnload: Entry...\n") );

   pDeviceExtension = pDeviceObject->DeviceExtension;

   KeSetEvent( &g_TCPS_KillEvent, 0, FALSE);

   //
   // Initialize The Sever Unload Event
   //
   KeInitializeEvent(
      &TCPS_UnloadEvent,
      NotificationEvent,
      FALSE
      );

   while( !IsListEmpty( &g_TCPServerList ) )
   {
//      UnloadWait.QuadPart = -(10 * 1000 * 3000);
      UnloadWait.QuadPart = -(10 * 1000 * 3000);

      Status = KeWaitForSingleObject(
                  &TCPS_UnloadEvent,  // Object to wait on.
                  Executive,  // Reason for waiting
                  KernelMode, // Processor mode
                  FALSE,      // Alertable
                  &UnloadWait // Timeout
                  );
   }

   //
   // Destroy The Symbolic Link
   //
   if( g_bSymbolicLinkCreated )
   {
      UNICODE_STRING UnicodeDeviceName;

      NdisInitUnicodeString(
         &UnicodeDeviceName,
         TDI_TCP_SERVER_DEVICE_NAME_W
         );

      KS_CreateSymbolicLink(
         &UnicodeDeviceName,
         FALSE
         );
  }
}
