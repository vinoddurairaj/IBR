/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include "ndis.h"
#include	"TDI.H"
#include	"TDIKRNL.H"
#include "KSUtil.h"

#include "INetInc.h"
#include "TDITTCP.h"

// Copyright And Configuration Management ----------------------------------
//
//             TDI Test (TTCP) Tcp Server Device - TCPLCIENT.c
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
//// TDI TTCP TCP Client GLOBAL DATA
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


/////////////////////////////////////////////////////////////////////////////
//// LOCAL PROCEDURE PROTOTYPES


/////////////////////////////////////////////////////////////////////////////
//// LOCAL STRUCTURE DEFINITIONS

typedef
struct _TCPC_SESSION
{
   TDITTCP_TEST_PARAMS  m_TestParams;

   TA_IP_ADDRESS        m_LocalAddress;   // TDI Address
   KS_ADDRESS           m_KS_Address;

   //
   // Local Connection Endpoint
   // -------------------------
   // The KS_ENDPOINT structure contains information that defines:
   //   TDI Address Object
   //   TDI Connection Context
   //
   // Together these two objects represent the local connection endpoint.
   //
   KS_ENDPOINT       m_KS_Endpoint;

   TA_IP_ADDRESS     m_RemoteAddress;

   UCHAR             m_InfoBuffer[ 256 ];

   ULONG             m_PatternBufferSize;
   PUCHAR            m_pPatternBuffer;
   PMDL              m_pPatternMdl;
   IO_STATUS_BLOCK   m_PatternIoStatus;
   KEVENT            m_FinalSendEvent;

   ULONG             m_nNumBuffersToSend;
}
   TCPC_SESSION, *PTCPC_SESSION;


/////////////////////////////////////////////////////////////////////////////
//// TCPC_DeviceLoad
//
// Purpose
// This routine initializes the TDI TTCP TCP Client device.
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
TCPC_DeviceLoad(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
   UNICODE_STRING UnicodeDeviceName;
   PDEVICE_OBJECT pDeviceObject = NULL;
   PDEVICE_EXTENSION pDeviceExtension = NULL;
   NTSTATUS Status = STATUS_SUCCESS;
   NTSTATUS ErrorCode = STATUS_SUCCESS;

   KdPrint(("TCPC_DeviceLoad: Entry...\n") );

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
   // Create The TDI TTCP TCP Client Device
   //
   NdisInitUnicodeString(
      &UnicodeDeviceName,
      TDI_TCP_CLIENT_DEVICE_NAME_W
      );

   Status = IoCreateDevice(
               pDriverObject,
               sizeof(DEVICE_EXTENSION),
               &UnicodeDeviceName,
               FILE_DEVICE_TCP_CLIENT,
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
   // Create The TDI TTCP TCP Client Device Symbolic Link
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
   pDeviceExtension->MajorFunction[IRP_MJ_CREATE] = TCPC_DeviceOpen;
   pDeviceExtension->MajorFunction[IRP_MJ_CLOSE]  = TCPC_DeviceClose;
   pDeviceExtension->MajorFunction[IRP_MJ_READ]   = NULL;
   pDeviceExtension->MajorFunction[IRP_MJ_WRITE]  = NULL;
   pDeviceExtension->MajorFunction[IRP_MJ_CLEANUP]  = TCPC_DeviceCleanup;
   pDeviceExtension->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = TCPC_DeviceIoControl;

   pDeviceExtension->DeviceUnload = TCPC_DeviceUnload;

   //
   // Fetch Transport Provider Information
   //
   Status = KS_QueryProviderInfo(
               TCP_DEVICE_NAME_W,
               &pDeviceExtension->TcpClientContext.TdiProviderInfo
               );

#ifdef DBG
   if (NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderInfo(
         TCP_DEVICE_NAME_W,
         &pDeviceExtension->TcpClientContext.TdiProviderInfo
         );
   }
#endif // DBG

   return STATUS_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
//// TCPC_DeviceOpen (IRP_MJ_CREATE Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDI TTCP TCP Client device create/open
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
TCPC_DeviceOpen(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   KdPrint(("TCPC_DeviceOpen: Entry...\n") );

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

   KdPrint( ("TCPC_DeviceOpen: Opened!!\n") );

   pIrp->IoStatus.Information = 0;

   TdiCompleteRequest( pIrp, STATUS_SUCCESS );

   return STATUS_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
//// TCPC_DeviceClose (IRP_MJ_CLOSE Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDI TTCP TCP Client device close requests.
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
TCPC_DeviceClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   KdPrint(("TCPC_DeviceClose: Entry...\n") );

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

   KdPrint( ("TCPC_DeviceClose: Closed!!\n") );

   pIrp->IoStatus.Information = 0;

   TdiCompleteRequest( pIrp, STATUS_SUCCESS );

   return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
//                    T D I  E V E N T  H A N D L E R S                    //
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//// TCPC_DisconnectEventHandler
//
// Purpose
//
// Parameters
//   TdiEventContext - Pointer to TCPC_SESSION structure for the session.
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

NTSTATUS TCPC_DisconnectEventHandler(
   IN PVOID TdiEventContext,						// Context From SetEventHandler
   IN CONNECTION_CONTEXT ConnectionContext,	// As passed to TdiOpenConnection
   IN LONG DisconnectDataLength,
   IN PVOID DisconnectData,
   IN LONG DisconnectInformationLength,
   IN PVOID DisconnectInformation,
   IN ULONG DisconnectFlags
   )
{
   KdPrint(("TCPC_DisconnectEventHandler: Entry; EventContext: 0x%8.8X; ConnectionContext: 0x%8.8X; Flags: 0x%8.8X\n",
      (ULONG )TdiEventContext, (ULONG)ConnectionContext, DisconnectFlags)
      );

   return( STATUS_SUCCESS );
}

/////////////////////////////////////////////////////////////////////////////
//// TCPC_ErrorEventHandler
//
// Purpose
//
// Parameters
//   TdiEventContext - Pointer to TCPC_SESSION structure for the session.
//
// Return Value
//
// Remarks
// A protocol error has occurred when this indication happens. This indication
// occurs only for errors of the worst type; the address this indication is
// delivered to is no longer usable for protocol-related operations, and
// should not be used for operations henceforth. All connections associated
// it are invalid.
//

NTSTATUS TCPC_ErrorEventHandler(
   IN PVOID TdiEventContext,  // The endpoint's file object.
   IN NTSTATUS Status         // Status code indicating error type.
   )
{
   KdPrint(("TCPC_ErrorEventHandler: Status: 0x%8.8X\n", Status) );

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// TCPC_ReceiveEventHandler
//
// Purpose
//
// Parameters
//   TdiEventContext - Pointer to TCPC_SESSION structure for the session.
//
// Return Value
//
// Remarks
// TDI_IND_RECEIVE indication handler definition.  This client routine is
// called by the transport provider when a connection-oriented TSDU is received
// that should be presented to the client.
//

NTSTATUS TCPC_ReceiveEventHandler(
   IN PVOID TdiEventContext,     // Context From SetEventHandler
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,				// pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *IoRequestPacket	// TdiReceive IRP if MORE_PROCESSING_REQUIRED.
	)
{
//   KdPrint(("TCPC_ReceiveEventHandler: Entry...\n") );

   KdPrint(("  Bytes Indicated: %d; BytesAvailable: %d; Flags: 0x%8.8x\n",
      BytesIndicated, BytesAvailable, ReceiveFlags));

   return( STATUS_SUCCESS );
}

/////////////////////////////////////////////////////////////////////////////
//// TCPC_ReceiveExpeditedEventHandler
//
// Purpose
//
// Parameters
//   TdiEventContext - Pointer to TCPC_SESSION structure for the session.
//
// Return Value
//
// Remarks
// This indication is delivered if expedited data is received on the connection.
// This will only occur in providers that support expedited data.
//

NTSTATUS TCPC_ReceiveExpeditedEventHandler(
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
   KdPrint(("TCPC_ReceiveExpeditedEventHandler: Entry...\n") );

   KdPrint(("  Bytes Indicated: %d; BytesAvailable: %d; Flags: 0x%8.8x\n",
      BytesIndicated, BytesAvailable, ReceiveFlags));

   return( STATUS_SUCCESS );
}

VOID
TCPC_SendCompletion(
    PVOID UserCompletionContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG Reserved
    )
{
   PTCPC_SESSION  pSession = (PTCPC_SESSION )UserCompletionContext;
   NTSTATUS       Status = STATUS_SUCCESS;

   if( NT_SUCCESS( IoStatusBlock->Status ) )
   {
//      KdPrint(( "TCPC_SendBuffer: Sent %d Bytes\n", IoStatusBlock->Information ));
   }
   else
   {
      KdPrint(( "TCPC_SendBuffer: Status 0x%8.8X\n", IoStatusBlock->Status ));

      KeSetEvent( &pSession->m_FinalSendEvent, 0, FALSE );

      return;
   }

   --pSession->m_nNumBuffersToSend;    // Sent One

   if( pSession->m_nNumBuffersToSend )
   {
      // ATTENTION!!!
      // ------------
      // Do not call KS_SendOnEndpoint (which eventually calls
      // IoCallDriver) directly from this completion routine. Use a
      // worker thread, running at IRQL = PASSIVE_LEVEL, to make the
      // call.
      //

      //
      // Start Another Asynchronous Send On The Connection
      //
      (pSession->m_pPatternMdl)->Next = NULL;   // IMPORTANT!!!

      Status = KS_SendOnEndpoint(
                     &pSession->m_KS_Endpoint,
                     NULL,       // User Completion Event
                     TCPC_SendCompletion,       // User Completion Routine
                     pSession,   // User Completion Context
                     &pSession->m_PatternIoStatus,
                     pSession->m_pPatternMdl,
                     0           // Send Flags
                     );

      if( !NT_SUCCESS( Status ) )
      {
         if( Status != STATUS_PENDING )
         {
            KdPrint(( "TCPC_SendBuffer: Status 0x%8.8X\n", IoStatusBlock->Status ));
   
            KeSetEvent( &pSession->m_FinalSendEvent, 0, FALSE );

            return;
         }
      }
   }
   else
   {
      KeSetEvent( &pSession->m_FinalSendEvent, 0, FALSE );
   }
}


/////////////////////////////////////////////////////////////////////////////
//// DoQueryAddressInfoTest
//
// Purpose
// Demonstrate TD_ADDRESS_INFO TDI Query
//
// Parameters
//
// Return Value
//
// Remarks
//

VOID
DoQueryAddressInfoTest(
   PTCPC_SESSION  pSession
   )
{
   NTSTATUS    Status = STATUS_SUCCESS;
   ULONG       nInfoBufferSize = sizeof( pSession->m_InfoBuffer );

   //
   // Query TDI Address Info On Transport Address
   //
   Status = KS_QueryAddressInfo(
               pSession->m_KS_Address.m_pFileObject,  // Address Object
               pSession->m_InfoBuffer,
               &nInfoBufferSize
               );

   KdPrint(( "Query Address Info Status: 0x%8.8x\n", Status ));

   if( NT_SUCCESS( Status ) )
   {
      DEBUG_DumpAddressInfo( (PTDI_ADDRESS_INFO )pSession->m_InfoBuffer );
   }

   //
   // Query TDI Address Info On Connection Endpoint
   //
   Status = KS_QueryAddressInfo(
               pSession->m_KS_Endpoint.m_pFileObject,  // Connection Object
               pSession->m_InfoBuffer,
               &nInfoBufferSize
               );

   KdPrint(( "Query Address Info Status: 0x%8.8x\n", Status ));

   if( NT_SUCCESS( Status ) )
   {
      DEBUG_DumpAddressInfo( (PTDI_ADDRESS_INFO )pSession->m_InfoBuffer );
   }
}


/////////////////////////////////////////////////////////////////////////////
//// DoQueryConnectionInfoTest
//
// Purpose
// Demonstrate TD_CONNECTION_INFO TDI Query
//
// Parameters
//
// Return Value
//
// Remarks
//

VOID
DoQueryConnectionInfoTest(
   PTCPC_SESSION  pSession
   )
{
   NTSTATUS    Status = STATUS_SUCCESS;
   ULONG       nInfoBufferSize = sizeof( pSession->m_InfoBuffer );

   //
   // Query TDI Connection Info
   //
   Status = KS_QueryConnectionInfo(
               &pSession->m_KS_Endpoint,  // Connection Endpoint
               pSession->m_InfoBuffer,
               &nInfoBufferSize
               );

   KdPrint(( "Query Connection Info Status: 0x%8.8x\n", Status ));

   if( NT_SUCCESS( Status ) )
   {
      DEBUG_DumpConnectionInfo( (PTDI_CONNECTION_INFO )pSession->m_InfoBuffer );
   }
}

/////////////////////////////////////////////////////////////////////////////
//// TCPC_TestThread
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
TCPC_TestThread(
   IN PVOID pContext
   )
{
   NTSTATUS             Status = STATUS_SUCCESS;
   NDIS_STATUS          nNdisStatus;
   PTTCP_TEST_START_CMD pStartCmd = NULL;
   PTCPC_SESSION        pSession = (PTCPC_SESSION )pContext;
   LARGE_INTEGER        DelayTime;
   IO_STATUS_BLOCK      IoStatusBlock;

   KdPrint(("TCPC_TestThread: Starting...\n") );

   //
   // Initialize Default Settings
   //
   Status = STATUS_SUCCESS;      // Always Indicate I/O Success

   //
   // Locate Test Session Parameter Buffer
   //
   pSession = (PTCPC_SESSION )pContext;

   //
   // Allocate The Pattern Buffer
   //
   pSession->m_PatternBufferSize = pSession->m_TestParams.m_nBufferSize;
   pSession->m_nNumBuffersToSend = pSession->m_TestParams.m_nNumBuffersToSend;

   nNdisStatus = NdisAllocateMemory(
                  &pSession->m_pPatternBuffer,
                  pSession->m_PatternBufferSize,
                  0,       // Allocate non-paged system-space memory
                  HighestAcceptableMax
                  );

   if( !NT_SUCCESS( nNdisStatus ) )
   {
      pSession->m_pPatternBuffer = NULL;

      goto ExitTheThread;
   }

   TDITTCP_FillPatternBuffer( 
      pSession->m_pPatternBuffer,
      pSession->m_PatternBufferSize
      );

   pSession->m_pPatternMdl = KS_AllocateAndProbeMdl(
                              pSession->m_pPatternBuffer,   // Virtual Address
                              pSession->m_PatternBufferSize,
                              FALSE,
                              FALSE,
                              NULL
                              );

   if( !pSession->m_pPatternMdl )
   {
      goto ExitTheThread;
   }

   //
   // Setup Local TDI Address
   //
   KS_InitIPAddress(
      &pSession->m_LocalAddress,
      INADDR_ANY,    // Any Local Address
      0              // Any Local Port
      );

   //
   // Open Transport Address
   //
   Status = KS_OpenTransportAddress(
                  TCP_DEVICE_NAME_W,
                  (PTRANSPORT_ADDRESS )&pSession->m_LocalAddress,
                  &pSession->m_KS_Address
                  );

   if( !NT_SUCCESS( Status ) )
   {
      //
      // Address Object Could Not Be Created
      //
      goto ExitTheThread;
   }

   //
   // Create The Connection Endpoint
   //
   Status = KS_OpenConnectionEndpoint(
                  TCP_DEVICE_NAME_W,
                  &pSession->m_KS_Address,
                  &pSession->m_KS_Endpoint,
                  &pSession->m_KS_Endpoint    // Context
                  );

   if( !NT_SUCCESS( Status ) )
   {
      //
      // Connection Endpoint Could Not Be Created
      //
      KS_CloseConnectionEndpoint( &pSession->m_KS_Endpoint );
      KS_CloseTransportAddress( &pSession->m_KS_Address );

      goto ExitTheThread;
   }

   KdPrint(("TCPC_TestThread: Created Local TDI Connection Endpoint\n") );

   KdPrint(("  pSession: 0x%8.8X; pAddress: 0x%8.8X; pConnection: 0x%8.8X\n",
      (ULONG )pSession,
      (ULONG )&pSession->m_KS_Address,
      (ULONG )&pSession->m_KS_Endpoint
      ));

   //
   // Setup Event Handlers On The Address Object
   //
   Status = KS_SetEventHandlers(
                  &pSession->m_KS_Address,
                  pSession,      // Event Context
                  NULL,          // ConnectEventHandler
                  TCPC_DisconnectEventHandler,
                  TCPC_ErrorEventHandler,
                  TCPC_ReceiveEventHandler,
                  NULL,          // ReceiveDatagramEventHandler
                  TCPC_ReceiveExpeditedEventHandler
                  );

   if( !NT_SUCCESS( Status ) )
   {
      //
      // Event Handlers Could Not Be Set
      //
      KS_CloseConnectionEndpoint( &pSession->m_KS_Endpoint );
      KS_CloseTransportAddress( &pSession->m_KS_Address );

      goto ExitTheThread;
   }

   KdPrint(("TCPC_TestThread: Set Event Handlers On The Address Object\n") );

   //
   // Setup Remote TDI Address
   //
   KS_InitIPAddress(
      &pSession->m_RemoteAddress,
      pSession->m_TestParams.m_RemoteAddress.s_addr,
      pSession->m_TestParams.m_Port
      );

   //
   // Request Connection To Remote Node
   //
   Status = KS_Connect(
               &pSession->m_KS_Endpoint,
               (PTRANSPORT_ADDRESS )&pSession->m_RemoteAddress
               );

   KdPrint(( "Connect Status: 0x%8.8x\n", Status ));

   if( NT_SUCCESS( nNdisStatus ) )
   {
      //
      // Specify NODELAY Send Option On The Connection Endpoint
      // ------------------------------------------------------
      // Returns successfully, BUT does not appear to effect TCP stream.
      //
      if( pSession->m_TestParams.m_nNoDelay )
      {
         pSession->m_TestParams.m_nNoDelay = TRUE;

         Status = KS_TCPSetInformation(
                     pSession->m_KS_Endpoint.m_pFileObject,// Connection Endpoint File Object
                     CO_TL_ENTITY,              // Entity
                     INFO_CLASS_PROTOCOL,       // Class
                     INFO_TYPE_CONNECTION,      // Type
                     TCP_SOCKET_NODELAY,        // Id
                     &pSession->m_TestParams.m_nNoDelay,          // Value
                     sizeof( pSession->m_TestParams.m_nNoDelay )  // ValueLength
                     );

         KdPrint(( "NoDelay Status: 0x%8.8x\n", Status ));
      }

      //
      // Make TDI_ADDRESS_INFO Query Test
      //
      DoQueryAddressInfoTest( pSession );

      //
      // Make TDI_CONNECTION_INFO Query Test
      //
      DoQueryConnectionInfoTest( pSession );

      //
      // Set TDI Connection Info
      // ===================
      // Is apparently not implemented in TCP/IP driver. Returns error
      // 0xC0000002 -> STATUS_NOT_IMPLEMENTED.
      //

      switch( pSession->m_TestParams.m_SendMode )
      {
         case TTCP_SEND_NEXT_FROM_COMPLETION:
            KdPrint(( "  Send Mode: Send Next From Completion.\n"));

            //
            // Initialize The Final Send Event
            //
            KeInitializeEvent(
               &pSession->m_FinalSendEvent,
               NotificationEvent,
               FALSE
               );

            //
            // Start The First Send On The Connection
            //
            (pSession->m_pPatternMdl)->Next = NULL;   // IMPORTANT!!!

            Status = KS_SendOnEndpoint(
                           &pSession->m_KS_Endpoint,
                           NULL,       // User Completion Event
                           TCPC_SendCompletion,// User Completion Routine
                           pSession,   // User Completion Context
                           &pSession->m_PatternIoStatus,
                           pSession->m_pPatternMdl,
                           0           // Send Flags
                           );

            if( NT_SUCCESS(Status) )
            {
               //
               // Wait For Final Send To Complete
               // -------------------------------
               // The TCPC_SendCompletion callback will set the event
               // when the final buffer has been send and acknowledged.
               //
               Status = KeWaitForSingleObject(
                              &pSession->m_FinalSendEvent,  // Object to wait on.
                              Executive,  // Reason for waiting
                              KernelMode, // Processor mode
                              FALSE,      // Alertable
                              NULL        // Timeout
                              );
            }
            else
            {
               KdPrint(( "TCPC_SendBuffer: Status 0x%8.8X\n", Status ));
            }
            break;

         case TTCP_SEND_SYNCHRONOUS:
         default:
            KdPrint(( "  Send Mode: Synchronous Send.\n"));

            while( pSession->m_nNumBuffersToSend-- )
            {
               //
               // Synchronous Send On The Connection
               //
               (pSession->m_pPatternMdl)->Next = NULL;   // IMPORTANT!!!

               Status = KS_SendOnEndpoint(
                              &pSession->m_KS_Endpoint,
                              NULL,       // User Completion Event
                              NULL,       // User Completion Routine
                              NULL,       // User Completion Context
                              &pSession->m_PatternIoStatus,
                              pSession->m_pPatternMdl,
                              0           // Send Flags
                              );

               if( NT_SUCCESS(Status) )
               {
//                  KdPrint(( "TCPC_SendBuffer: Sent %d Bytes\n", pSession->m_PatternIoStatus.Information ));
               }
               else
               {
                  KdPrint(( "TCPC_SendBuffer: Status 0x%8.8X\n", Status ));
                  break;
               }
            }
            break;
      }
   }

   //
   // Perfrom Synchronous Disconnect
   //
   Status = KS_Disconnect(
               &pSession->m_KS_Endpoint,
               NULL,    // UserCompletionEvent
               NULL,    // UserCompletionRoutine
               NULL,    // UserCompletionContext
               NULL,    // pIoStatusBlock
               0        // Disconnect Flags
               );

   KdPrint(( "Disconnect Status: 0x%8.8x\n", Status ));

   KdPrint(( "Waiting After Disconnect...\n" ));

   DelayTime.QuadPart = 10*1000*1000*5;   // 5 Seconds

   KeDelayExecutionThread( KernelMode, FALSE, &DelayTime );


   //
   // Close The Connection Endpoint
   // -----------------------------
   // This will close the connection file object and the address file
   // object.
   //
   KS_CloseConnectionEndpoint( &pSession->m_KS_Endpoint );
   KS_CloseTransportAddress( &pSession->m_KS_Address );

   //
   // Exit The Thread
   //
ExitTheThread:

   KdPrint(("TCPC_TestThread: Exiting...\n") );

   if( pSession )
   {
      if( pSession->m_pPatternMdl )
      {
         KS_UnlockAndFreeMdl( pSession->m_pPatternMdl );
      }

      pSession->m_pPatternMdl = NULL;


      if( pSession->m_pPatternBuffer )
      {
         NdisFreeMemory(
            pSession->m_pPatternBuffer,
            pSession->m_PatternBufferSize,
            0
            );
      }

      pSession->m_pPatternBuffer = NULL;

      NdisFreeMemory( pSession, sizeof( TCPC_SESSION ), 0 );
   }

   pSession = NULL;

   (void)PsTerminateSystemThread( STATUS_SUCCESS );
}

/////////////////////////////////////////////////////////////////////////////
//// TCPC_StartTest
//
// Purpose
// Start A TTCP TCP Client Test
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
TCPC_StartTest(
   IN PDEVICE_OBJECT pDeviceObject,
   IN PIRP pIrp
   )
{
   HANDLE               hTestThread;
   NDIS_STATUS          nNdisStatus;
   PTTCP_TEST_START_CMD pStartCmd = NULL;
   PTCPC_SESSION        pSession = NULL;

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
                  &pSession,
                  sizeof( TCPC_SESSION ),
                  0,       // Allocate non-paged system-space memory
                  HighestAcceptableMax
                  );

   if( !NT_SUCCESS( nNdisStatus ) )
   {
      return( nNdisStatus );
   }

   NdisZeroMemory( pSession, sizeof( TCPC_SESSION ) );

   NdisMoveMemory(
      &pSession->m_TestParams,
      &pStartCmd->m_TestParams,
      sizeof( TDITTCP_TEST_PARAMS )
      );

   //
   // Start The Thread That Will Execute The Test
   //
   PsCreateSystemThread(
      &hTestThread,     // thread handle
      0L,               // desired access
      NULL,             // object attributes
      NULL,             // process handle
      NULL,             // client id
      TCPC_TestThread,  // start routine
      (PVOID )pSession  // start context
      );

   return( STATUS_SUCCESS );
}

/////////////////////////////////////////////////////////////////////////////
//// TCPC_DeviceIoControl (IRP_MJ_DEVICE_CONTROL Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDI TTCP TCP Client device IOCTL requests.
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
TCPC_DeviceIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;
   NTSTATUS             Status;
   PIO_STACK_LOCATION   pIrpSp;
   ULONG                nFunctionCode;

   KdPrint(("TCPC_DeviceIoControl: Entry...\n") );

   pDeviceExtension = pDeviceObject->DeviceExtension;

   pIrp->IoStatus.Information = 0;      // Nothing Returned Yet

   pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

   nFunctionCode=pIrpSp->Parameters.DeviceIoControl.IoControlCode;

   switch( nFunctionCode )
   {
      case IOCTL_TCP_CLIENT_START_TEST:
         Status = TCPC_StartTest( pDeviceObject, pIrp );
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
//// TCPC_DeviceCleanup (IRP_MJ_CLEANUP Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDI TTCP TCP Client device cleanup
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
TCPC_DeviceCleanup(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   KdPrint(("TCPC_DeviceCleanup: Entry...\n"));

   pDeviceExtension = pDeviceObject->DeviceExtension;

   pIrp->IoStatus.Status = STATUS_SUCCESS;
   pIrp->IoStatus.Information = 0;

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// TCPC_DeviceUnload
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
TCPC_DeviceUnload(
   IN PDEVICE_OBJECT pDeviceObject
   )
{
   PDEVICE_EXTENSION  pDeviceExtension;

   KdPrint(("TCPC_DeviceUnload: Entry...\n") );

   pDeviceExtension = pDeviceObject->DeviceExtension;

   //
   // Destroy The Symbolic Link
   //
   if( g_bSymbolicLinkCreated )
   {
      UNICODE_STRING UnicodeDeviceName;

      NdisInitUnicodeString(
         &UnicodeDeviceName,
         TDI_TCP_CLIENT_DEVICE_NAME_W
         );

      KS_CreateSymbolicLink(
         &UnicodeDeviceName,
         FALSE
         );
   }
}
