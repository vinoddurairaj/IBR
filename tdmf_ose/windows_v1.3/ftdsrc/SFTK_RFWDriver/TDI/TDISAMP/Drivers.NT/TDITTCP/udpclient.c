/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include "ndis.h"

#include "INetInc.h"
#include "TDITTCP.h"

// Copyright And Configuration Management ----------------------------------
//
//                Test (TTCP) Udp Server Device - UDPCLIENT.c
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
//// TDI TTCP UDP Client GLOBAL DATA
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
static BOOLEAN        g_bSymbolicLinkCreated = FALSE;

#define  PATTERN_HEADER_SIZE 32


/////////////////////////////////////////////////////////////////////////////
//// LOCAL PROCEDURE PROTOTYPES


/////////////////////////////////////////////////////////////////////////////
//// LOCAL STRUCTURE DEFINITIONS

typedef
struct _UDPC_SESSION
{
   TDITTCP_TEST_PARAMS        m_TestParams;

   TA_IP_ADDRESS              m_LocalAddress;   // TDI Address

   TA_IP_ADDRESS              m_RemoteAddress;

   //
   // Local Address Object
   //
   KS_ADDRESS                 m_KS_Address;

   TDI_CONNECTION_INFORMATION m_RemoteConnectionInfo;

   ULONG                      m_PatternBufferSize;

   UCHAR                      m_pPatternHeaderBuffer[ PATTERN_HEADER_SIZE ];
   PMDL                       m_pPatternHeaderMdl;

   ULONG                      m_PatternDataBufferSize;
   PUCHAR                     m_pPatternDataBuffer;
   PMDL                       m_pPatternDataMdl;

   IO_STATUS_BLOCK            m_pPatternIoStatus;
   KEVENT                     m_FinalSendEvent;

   ULONG                      m_nNumBuffersToSend;

   PMDL                       m_pUdpGuardMdl;
   UCHAR                      m_UdpGuardBuffer[ UDP_GUARD_BUFFER_LENGTH ];

}
   UDPC_SESSION, *PUDPC_SESSION;


/////////////////////////////////////////////////////////////////////////////
//// UDPC_DeviceLoad
//
// Purpose
// This routine initializes the TDI TTCP UDP Client device.
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
UDPC_DeviceLoad(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
   UNICODE_STRING UnicodeDeviceName;
   PDEVICE_OBJECT pDeviceObject = NULL;
   PDEVICE_EXTENSION pDeviceExtension = NULL;
   NTSTATUS Status = STATUS_SUCCESS;
   NTSTATUS ErrorCode = STATUS_SUCCESS;

   KdPrint(("UDPC_DeviceLoad: Entry...\n") );

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
   // Create The TDI TTCP UDP Client Device
   //
   NdisInitUnicodeString(
      &UnicodeDeviceName,
      TDI_UDP_CLIENT_DEVICE_NAME_W
      );

   Status = IoCreateDevice(
               pDriverObject,
               sizeof(DEVICE_EXTENSION),
               &UnicodeDeviceName,
               FILE_DEVICE_UDP_CLIENT,
               0,            // Standard device characteristics
               FALSE,      // This isn't an exclusive device
               &pDeviceObject
               );

   if( !NT_SUCCESS( Status ) )
   {
      KdPrint(("UDPC_DeviceLoad: IoCreateDevice() failed:\n") );

      Status = STATUS_UNSUCCESSFUL;

      return(Status);
   }

   //
   // Create The TDI TTCP UDP Client Device Symbolic Link
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
   pDeviceExtension->MajorFunction[IRP_MJ_CREATE] = UDPC_DeviceOpen;
   pDeviceExtension->MajorFunction[IRP_MJ_CLOSE]  = UDPC_DeviceClose;
   pDeviceExtension->MajorFunction[IRP_MJ_READ]   = NULL;
   pDeviceExtension->MajorFunction[IRP_MJ_WRITE]  = NULL;
   pDeviceExtension->MajorFunction[IRP_MJ_CLEANUP]  = UDPC_DeviceCleanup;
   pDeviceExtension->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = UDPC_DeviceIoControl;

   pDeviceExtension->DeviceUnload = UDPC_DeviceUnload;

   //
   // Fetch Transport Provider Information
   //
   Status = KS_QueryProviderInfo(
               UDP_DEVICE_NAME_W,
               &pDeviceExtension->UdpClientContext.TdiProviderInfo
               );

#ifdef DBG
   if (NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderInfo(
         UDP_DEVICE_NAME_W,
         &pDeviceExtension->UdpClientContext.TdiProviderInfo );
   }
#endif // DBG

   return STATUS_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
//// UDPC_DeviceOpen (IRP_MJ_CREATE Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDI TTCP UDP Client device create/open
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
UDPC_DeviceOpen(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   KdPrint(("UDPC_DeviceOpen: Entry...\n") );

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

   KdPrint( ("UDPC_DeviceOpen: Opened!!\n") );

   pIrp->IoStatus.Information = 0;

   TdiCompleteRequest( pIrp, STATUS_SUCCESS );

   return STATUS_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
//// UDPC_DeviceClose (IRP_MJ_CLOSE Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDI TTCP UDP Client device close requests.
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
UDPC_DeviceClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   KdPrint(("UDPC_DeviceClose: Entry...\n") );

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

   KdPrint( ("UDPC_DeviceClose: Closed!!\n") );

   pIrp->IoStatus.Information = 0;

   TdiCompleteRequest( pIrp, STATUS_SUCCESS );

   return STATUS_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
//// UDPC_ErrorEventHandler
//
// Purpose
//
// Parameters
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

NTSTATUS UDPC_ErrorEventHandler(
   IN PVOID TdiEventContext,  // The endpoint's file object.
   IN NTSTATUS Status         // Status code indicating error type.
   )
{
   KdPrint(("UDPC_ErrorEventHandler: Status: 0x%8.8X\n", Status) );

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// UDPC_ReceiveDatagramEventHandler
//
// Purpose
//
// Parameters
//
// Return Value
//
// Remarks
// TDI_IND_RECEIVE_DATAGRAM indication handler definition.  This client
// routine is called by the transport provider when a connectionless TSDU
// is received that should be presented to the client.
//

NTSTATUS
UDPC_ReceiveDatagramEventHandler(
   IN PVOID TdiEventContext,     // Context From SetEventHandler
   IN LONG SourceAddressLength,    // length of the originator of the datagram
   IN PVOID SourceAddress,         // string describing the originator of the datagram
   IN LONG OptionsLength,          // options for the receive
   IN PVOID Options,               //
   IN ULONG ReceiveDatagramFlags,  //
   IN ULONG BytesIndicated,
   IN ULONG BytesAvailable,
   OUT ULONG *BytesTaken,
   IN PVOID Tsdu,				// pointer describing this TSDU, typically a lump of bytes
   OUT PIRP *IoRequestPacket	// TdiReceive IRP if MORE_PROCESSING_REQUIRED.
   )
{
   KdPrint(("UDPC_ReceiveDatagramEventHandler: Entry...\n") );

   KdPrint(("  Bytes Indicated: %d; BytesAvailable: %d; Flags: 0x%8.8x\n",
      BytesIndicated, BytesAvailable, ReceiveDatagramFlags));

   return( STATUS_SUCCESS );
}

NTSTATUS
UDPC_SendGuardBuffer(
   PUDPC_SESSION  pSession
   )
{
   NTSTATUS Status;

   (pSession->m_pUdpGuardMdl)->Next = NULL;   // IMPORTANT!!!

   Status = KS_SendDatagramOnAddress(
                  &pSession->m_KS_Address,
                  NULL,       // User Completion Event
                  NULL,       // User Completion Routine
                  NULL,       // User Completion Context
                  &pSession->m_pPatternIoStatus,
                  pSession->m_pUdpGuardMdl,
                  &pSession->m_RemoteConnectionInfo
                  );

   if( NT_SUCCESS(Status) )
   {
      KdPrint(( "UDPC_SendGuardBuffer: Status 0x%8.8X\n", pSession->m_pPatternIoStatus.Status ));
      KdPrint(( "UDPC_SendGuardBuffer: Sent %d Bytes\n", pSession->m_pPatternIoStatus.Information ));
   }
   else
   {
      KdPrint(( "UDPC_SendGuardBuffer: Status 0x%8.8X\n", Status ));
   }

   return( Status );
}

/////////////////////////////////////////////////////////////////////////////
//// UDPC_TestThread
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
UDPC_TestThread(
   IN PVOID pContext
   )
{
   NTSTATUS             Status = STATUS_SUCCESS;
   NDIS_STATUS          nNdisStatus;
   PTTCP_TEST_START_CMD pStartCmd = NULL;
   PUDPC_SESSION        pSession = (PUDPC_SESSION )pContext;
   LARGE_INTEGER        DelayTime;

   KdPrint(("UDPC_TestThread: Starting...\n") );

   //
   // Initialize Default Settings
   //
   Status = STATUS_SUCCESS;      // Always Indicate I/O Success

   //
   // Locate Test Session Parameter Buffer
   //
   pSession = (PUDPC_SESSION )pContext;

   pSession->m_PatternBufferSize = pSession->m_TestParams.m_nBufferSize;
   pSession->m_nNumBuffersToSend = pSession->m_TestParams.m_nNumBuffersToSend;

   //
   // Force Size To Allow Send Buffer Chaining Demonstration
   //
   if( pSession->m_PatternBufferSize < 64 )
   {
      pSession->m_PatternBufferSize = 64;
   }

   pSession->m_PatternDataBufferSize = pSession->m_PatternBufferSize - PATTERN_HEADER_SIZE;

   //
   // Allocate The Pattern Header MDL
   //
   RtlFillMemory( 
      pSession->m_pPatternHeaderBuffer,
      PATTERN_HEADER_SIZE,
      'X'
      );

   pSession->m_pPatternHeaderMdl = KS_AllocateAndProbeMdl(
                              pSession->m_pPatternHeaderBuffer,   // Virtual Address
                              PATTERN_HEADER_SIZE,
                              FALSE,
                              FALSE,
                              NULL
                              );

   if( !pSession->m_pPatternHeaderMdl )
   {
      goto ExitTheThread;
   }

   //
   // Allocate The Pattern Data Buffer
   //
   nNdisStatus = NdisAllocateMemory(
                  &pSession->m_pPatternDataBuffer,
                  pSession->m_PatternDataBufferSize,
                  0,       // Allocate non-paged system-space memory
                  HighestAcceptableMax
                  );

   if( !NT_SUCCESS( nNdisStatus ) )
   {
      pSession->m_pPatternDataBuffer = NULL;

      goto ExitTheThread;
   }

   TDITTCP_FillPatternBuffer( 
      pSession->m_pPatternDataBuffer,
      pSession->m_PatternDataBufferSize
      );

   pSession->m_pPatternDataMdl = KS_AllocateAndProbeMdl(
                              pSession->m_pPatternDataBuffer,   // Virtual Address
                              pSession->m_PatternDataBufferSize,
                              FALSE,
                              FALSE,
                              NULL
                              );

   if( !pSession->m_pPatternDataMdl )
   {
      goto ExitTheThread;
   }

   //
   // Setup UDP Guard Memory And MDL
   //
   RtlCopyMemory( pSession->m_UdpGuardBuffer, "PCAx", UDP_GUARD_BUFFER_LENGTH );

   pSession->m_pUdpGuardMdl = KS_AllocateAndProbeMdl(
                                 pSession->m_UdpGuardBuffer,   // Virtual Address
                                 UDP_GUARD_BUFFER_LENGTH,
                                 FALSE,
                                 FALSE,
                                 NULL
                                 );

   if( !pSession->m_pUdpGuardMdl )
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
   // Create The Local Address Object
   //
   Status = KS_OpenTransportAddress(
                  UDP_DEVICE_NAME_W,
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

   KdPrint(("UDPC_TestThread: Created Local Address Object\n") );

   //
   // Setup Event Handlers On The Address Object
   //
   Status = KS_SetEventHandlers(
                  &pSession->m_KS_Address,
                  pSession,      // Event Context
                  NULL,          // ConnectEventHandler
                  NULL,          // DisconnectEventHandler,
                  UDPC_ErrorEventHandler,
                  NULL,          // ReceiveEventHandler,
                  UDPC_ReceiveDatagramEventHandler,
                  NULL           // ReceiveExpeditedEventHandler
                  );

   if( !NT_SUCCESS( Status ) )
   {
      //
      // Event Handlers Could Not Be Set
      //
      KS_CloseTransportAddress( &pSession->m_KS_Address );

      goto ExitTheThread;
   }

   //
   // Setup Remote TDI Address
   //
   KS_InitIPAddress(
      &pSession->m_RemoteAddress,
      pSession->m_TestParams.m_RemoteAddress.s_addr,
      pSession->m_TestParams.m_Port
      );

   //
   // Setup Remote Connection Info
   //
   NdisZeroMemory(
      &pSession->m_RemoteConnectionInfo,
      sizeof( TDI_CONNECTION_INFORMATION )
      );

   pSession->m_RemoteConnectionInfo.RemoteAddress = &pSession->m_RemoteAddress;
   pSession->m_RemoteConnectionInfo.RemoteAddressLength = sizeof( TA_IP_ADDRESS );

   if( NT_SUCCESS( nNdisStatus ) )
   {
      int   i;

      switch( pSession->m_TestParams.m_SendMode )
      {
         case TTCP_SEND_NEXT_FROM_COMPLETION:
            KdPrint(( "  Send Mode: Send Next From Completion.\n"));

         case TTCP_SEND_SYNCHRONOUS:
         default:
            KdPrint(( "  Send Mode: Synchronous Send.\n"));

            //
            // Send Guard Buffer Preamble
            //
            Status = UDPC_SendGuardBuffer( pSession );

            if( !NT_SUCCESS(Status) )
            {
               KdPrint(( "UDPC_SendBuffer: Status 0x%8.8X\n", Status ));
               break;
            }

            while( pSession->m_nNumBuffersToSend-- )
            {
               //
               // Synchronous Send On The Connection
               //
               (pSession->m_pPatternHeaderMdl)->Next = pSession->m_pPatternDataMdl;
               (pSession->m_pPatternDataMdl)->Next = NULL;   // IMPORTANT!!!

               Status = KS_SendDatagramOnAddress(
                              &pSession->m_KS_Address,
                              NULL,       // User Completion Event
                              NULL,       // User Completion Routine
                              NULL,       // User Completion Context
                              &pSession->m_pPatternIoStatus,
                              pSession->m_pPatternHeaderMdl,   // First Of Chain
                              &pSession->m_RemoteConnectionInfo
                              );

               if( NT_SUCCESS(Status) )
               {
                  KdPrint(( "UDPC_SendBuffer: Status 0x%8.8X\n", pSession->m_pPatternIoStatus.Status ));
                  KdPrint(( "UDPC_SendBuffer: Sent %d Bytes\n", pSession->m_pPatternIoStatus.Information ));
               }
               else
               {
                  KdPrint(( "UDPC_SendBuffer: Status 0x%8.8X\n", Status ));
                  break;
               }
            }

            for( i = 0; i < 4; ++i )
            {
               //
               // Send Guard Buffer Postamble
               //
               (pSession->m_pUdpGuardMdl)->Next = NULL;   // IMPORTANT!!!

               Status = KS_SendDatagramOnAddress(
                              &pSession->m_KS_Address,
                              NULL,       // User Completion Event
                              NULL,       // User Completion Routine
                              NULL,       // User Completion Context
                              &pSession->m_pPatternIoStatus,
                              pSession->m_pUdpGuardMdl,
                              &pSession->m_RemoteConnectionInfo
                              );

               if( NT_SUCCESS(Status) )
               {
                  KdPrint(( "UDPC_SendBuffer: Sent %d Bytes\n", pSession->m_pPatternIoStatus.Information ));
               }
               else
               {
                  KdPrint(( "UDPC_SendBuffer: Status 0x%8.8X\n", Status ));
                  break;
               }
            }
            break;
      }
   }

   //
   // ATTENTION!!! Test Exit...
   //

   KdPrint(( "Waiting After Disconnect...\n" ));

   DelayTime.QuadPart = 10*1000*1000*5;   // 5 Seconds

   KeDelayExecutionThread( KernelMode, FALSE, &DelayTime );

   KS_CloseTransportAddress( &pSession->m_KS_Address );

   //
   // Exit The Thread
   //
ExitTheThread:

   KdPrint(("UDPC_TestThread: Exiting...\n") );

   if( pSession )
   {
      //
      // Free UDP Guard MDL And Memory
      //
      if( pSession->m_pUdpGuardMdl )
      {
         KS_UnlockAndFreeMdl( pSession->m_pUdpGuardMdl );
      }

      pSession->m_pUdpGuardMdl = NULL;


      //
      // Free Pattern Data MDL And Memory
      //
      if( pSession->m_pPatternDataMdl )
      {
         KS_UnlockAndFreeMdl( pSession->m_pPatternDataMdl );
      }

      pSession->m_pPatternDataMdl = NULL;


      if( pSession->m_pPatternDataBuffer )
      {
         NdisFreeMemory(
            pSession->m_pPatternDataBuffer,
            pSession->m_PatternDataBufferSize,
            0
            );
      }

      pSession->m_pPatternDataBuffer = NULL;


      //
      // Free Pattern Header MDL
      //
      if( pSession->m_pPatternDataMdl )
      {
         KS_UnlockAndFreeMdl( pSession->m_pPatternDataMdl );
      }

      pSession->m_pPatternDataMdl = NULL;

      NdisFreeMemory( pSession, sizeof( UDPC_SESSION ), 0 );
   }

   pSession = NULL;

   (void)PsTerminateSystemThread( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// UDPC_StartTest
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
UDPC_StartTest(
   IN PDEVICE_OBJECT pDeviceObject,
   IN PIRP pIrp
   )
{
   HANDLE               hTestThread;
   NDIS_STATUS          nNdisStatus;
   PTTCP_TEST_START_CMD pStartCmd = NULL;
   PUDPC_SESSION        pSession = NULL;

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
                  sizeof( UDPC_SESSION ),
                  0,       // Allocate non-paged system-space memory
                  HighestAcceptableMax
                  );

   if( !NT_SUCCESS( nNdisStatus ) )
   {
      return( nNdisStatus );
   }

   NdisZeroMemory( pSession, sizeof( UDPC_SESSION ) );

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
      UDPC_TestThread,  // start routine
      (PVOID )pSession  // start context
      );

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// UDPC_DeviceIoControl (IRP_MJ_DEVICE_CONTROL Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDI TTCP UDP Client device IOCTL requests.
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
UDPC_DeviceIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;
   NTSTATUS             Status;
   PIO_STACK_LOCATION   pIrpSp;
   ULONG                nFunctionCode;

   KdPrint(("UDPC_DeviceIoControl: Entry...\n") );

   pDeviceExtension = pDeviceObject->DeviceExtension;

   pIrp->IoStatus.Information = 0;      // Nothing Returned Yet

   pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

   nFunctionCode=pIrpSp->Parameters.DeviceIoControl.IoControlCode;

   switch( nFunctionCode )
   {
      case IOCTL_UDP_CLIENT_START_TEST:
         Status = UDPC_StartTest( pDeviceObject, pIrp );
         break;

      default:
         KdPrint((  "FunctionCode: 0x%8.8X\n", nFunctionCode ));
         Status = STATUS_NOT_IMPLEMENTED;
         break;
   }

   TdiCompleteRequest( pIrp, Status );

   return( Status );
}


/////////////////////////////////////////////////////////////////////////////
//// UDPC_DeviceCleanup (IRP_MJ_CLEANUP Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDI TTCP UDP Client device cleanup
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
UDPC_DeviceCleanup(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   KdPrint(("UDPC_DeviceCleanup: Entry...\n"));

   pDeviceExtension = pDeviceObject->DeviceExtension;

   pIrp->IoStatus.Status = STATUS_SUCCESS;
   pIrp->IoStatus.Information = 0;

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// UDPC_DeviceUnload
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
UDPC_DeviceUnload(
   IN PDEVICE_OBJECT pDeviceObject
   )
{
   PDEVICE_EXTENSION  pDeviceExtension;

   KdPrint(("UDPC_DeviceUnload: Entry...\n") );

   pDeviceExtension = pDeviceObject->DeviceExtension;

   //
   // Destroy The Symbolic Link
   //
   if( g_bSymbolicLinkCreated )
   {
      UNICODE_STRING UnicodeDeviceName;

      NdisInitUnicodeString(
         &UnicodeDeviceName,
         TDI_UDP_CLIENT_DEVICE_NAME_W
         );

      KS_CreateSymbolicLink(
         &UnicodeDeviceName,
         FALSE
         );
   }
}
