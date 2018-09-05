/////////////////////////////////////////////////////////////////////////////
//// INCLUDE FILES

#include "ndis.h"

#include "INetInc.h"
#include "TDITTCP.h"

// Copyright And Configuration Management ----------------------------------
//
//                Test (TTCP) Udp Server Device - UDPSERVER.c
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
//// TDI TTCP UDP Server GLOBAL DATA
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
static LIST_ENTRY    g_UDPServerList;
static KEVENT        g_UDPS_KillEvent;


/////////////////////////////////////////////////////////////////////////////
//// LOCAL PROCEDURE PROTOTYPES


/////////////////////////////////////////////////////////////////////////////
//// LOCAL STRUCTURE DEFINITIONS

typedef
struct _UDPS_SERVER
{
   LIST_ENTRY           m_ListElement;

   HANDLE               m_hTestThread;

   TDITTCP_TEST_PARAMS  m_TestParams;

   TA_IP_ADDRESS        m_LocalAddress;   // TDI Address
   KS_ADDRESS           m_KS_Address;

	TA_IP_ADDRESS		         m_RemoteAddress;
	TDI_CONNECTION_INFORMATION m_RemoteConnectionInfo;
}
   UDPS_SERVER, *PUDPS_SERVER;


/////////////////////////////////////////////////////////////////////////////
//// UDPS_DeviceLoad
//
// Purpose
// This routine initializes the TDI TTCP UDP Server device.
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
UDPS_DeviceLoad(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
   UNICODE_STRING UnicodeDeviceName;
   PDEVICE_OBJECT pDeviceObject = NULL;
   PDEVICE_EXTENSION pDeviceExtension = NULL;
   NTSTATUS Status = STATUS_SUCCESS;
   NTSTATUS ErrorCode = STATUS_SUCCESS;

   KdPrint(("UDPS_DeviceLoad: Entry...\n") );

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
   // Create The TDI TTCP UDP Server Device
   //
   NdisInitUnicodeString(
      &UnicodeDeviceName,
      TDI_UDP_SERVER_DEVICE_NAME_W
      );

   Status = IoCreateDevice(
               pDriverObject,
               sizeof(DEVICE_EXTENSION),
               &UnicodeDeviceName,
               FILE_DEVICE_UDP_SERVER,
               0,            // Standard device characteristics
               FALSE,      // This isn't an exclusive device
               &pDeviceObject
               );

   if( !NT_SUCCESS( Status ) )
   {
      KdPrint(("UDPS_DeviceLoad: IoCreateDevice() failed:\n") );

      Status = STATUS_UNSUCCESSFUL;

      return(Status);
   }

   //
   // Create The TDI TTCP UDP Server Device Symbolic Link
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
   pDeviceExtension->MajorFunction[IRP_MJ_CREATE] = UDPS_DeviceOpen;
   pDeviceExtension->MajorFunction[IRP_MJ_CLOSE]  = UDPS_DeviceClose;
   pDeviceExtension->MajorFunction[IRP_MJ_READ]   = NULL;
   pDeviceExtension->MajorFunction[IRP_MJ_WRITE]  = NULL;
   pDeviceExtension->MajorFunction[IRP_MJ_CLEANUP]  = UDPS_DeviceCleanup;
   pDeviceExtension->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = UDPS_DeviceIoControl;

   pDeviceExtension->DeviceUnload = UDPS_DeviceUnload;

   //
   // Fetch Transport Provider Information
   //
   Status = KS_QueryProviderInfo(
               UDP_DEVICE_NAME_W,
               &pDeviceExtension->UdpServerContext.TdiProviderInfo
               );

#ifdef DBG
   if (NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderInfo(
         UDP_DEVICE_NAME_W,
         &pDeviceExtension->UdpServerContext.TdiProviderInfo
         );
   }
#endif // DBG

   InitializeListHead( &g_UDPServerList );

   //
   // Initialize The Sever Kill Event
   //
   KeInitializeEvent(
      &g_UDPS_KillEvent,
      NotificationEvent,
      FALSE
      );

   return STATUS_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
//// UDPS_DeviceOpen (IRP_MJ_CREATE Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDI TTCP UDP Server device create/open
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
UDPS_DeviceOpen(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   KdPrint(("UDPS_DeviceOpen: Entry...\n") );

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

   KdPrint( ("UDPS_DeviceOpen: Opened!!\n") );

   pIrp->IoStatus.Information = 0;

   TdiCompleteRequest( pIrp, STATUS_SUCCESS );

   return STATUS_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
//// UDPS_DeviceClose (IRP_MJ_CLOSE Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDI TTCP UDP Server device close requests.
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
UDPS_DeviceClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   KdPrint(("UDPS_DeviceClose: Entry...\n") );

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

   KdPrint( ("UDPS_DeviceClose: Closed!!\n") );

   pIrp->IoStatus.Information = 0;

   TdiCompleteRequest( pIrp, STATUS_SUCCESS );

   return STATUS_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
//// UDPS_ErrorEventHandler
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

NTSTATUS UDPS_ErrorEventHandler(
   IN PVOID TdiEventContext,  // The endpoint's file object.
   IN NTSTATUS Status         // Status code indicating error type.
   )
{
   KdPrint(("UDPS_ErrorEventHandler: Status: 0x%8.8X\n", Status) );

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// UDPS_ReceiveDatagramEventHandler
//
// Purpose
//
// Parameters
//
// Return Value
//
// Remarks
//

NTSTATUS UDPS_ReceiveDatagramEventHandler(
   IN PVOID TdiEventContext,       // Context From SetEventHandler
   IN LONG SourceAddressLength,    // length of the originator of the datagram
   IN PVOID SourceAddress,         // string describing the originator of the datagram
   IN LONG OptionsLength,          // options for the receive
   IN PVOID Options,               //
   IN ULONG ReceiveDatagramFlags,  //
   IN ULONG BytesIndicated,        // number of bytes in this indication
   IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
   OUT ULONG *BytesTaken,          // number of bytes used by indication routine
   IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
   OUT PIRP *IoRequestPacket       // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
   )
{
   PTRANSPORT_ADDRESS   pTransAddr = (PTRANSPORT_ADDRESS )SourceAddress;
   BOOLEAN              bIsCompleteTsdu = FALSE;

   KdPrint(("UDPS_ReceiveDatagramEventHandler: Entry...\n") );

   KdPrint(("  Bytes Indicated: %d; BytesAvailable: %d; Flags: 0x%8.8x\n",
      BytesIndicated, BytesAvailable, ReceiveDatagramFlags));

   DEBUG_DumpTransportAddress( pTransAddr );

   //
   // Determine Whether Tsdu Contains A Full TSDU
   // -------------------------------------------
   // We could check (ReceiveDatagramFlags & TDI_RECEIVE_ENTIRE_MESSAGE).
   // However, checking (BytesIndicated == BytesAvailable) seems more
   // reliable.
   //
   if( BytesIndicated == BytesAvailable )
   {
      bIsCompleteTsdu = TRUE;
   }

   if( bIsCompleteTsdu )
   {
      //
      // Process Complete TSDU
      // ---------------------
      // Handler should copy all of the indicated data to internal buffer
      // and return control as quickly as possible.
      //

      *BytesTaken = BytesAvailable;

      return( STATUS_SUCCESS );
   }

   //
   // Process Partial TSDU
   // --------------------
   // One could check (ReceiveDatagramFlags & TDI_RECEIVE_COPY_LOOKAHEAD) to
   // determine whether copying the BytesIndicated lookahead data is
   // required or not. However, since the case where lookahead data must
   // be copied must be dealt with anyway, it seems simpler to just go ahead
   // and always copy the lookahead data here in the handler.
   // 

   *BytesTaken = BytesIndicated;

   return( STATUS_SUCCESS );
}

/////////////////////////////////////////////////////////////////////////////
//// UDPS_TestThread
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
UDPS_TestThread(
   IN PVOID pContext
   )
{
   NTSTATUS             Status = STATUS_SUCCESS;
   NDIS_STATUS          nNdisStatus;
   PTTCP_TEST_START_CMD pStartCmd = NULL;
   PUDPS_SERVER         pServer = (PUDPS_SERVER )pContext;
   LARGE_INTEGER        DelayTime;

   KdPrint(("UDPS_TestThread: Starting...\n") );

   //
   // Initialize Default Settings
   //
   Status = STATUS_SUCCESS;      // Always Indicate I/O Success

   //
   // Locate Test Session Parameter Buffer
   //
   pServer = (PUDPS_SERVER )pContext;

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
                  UDP_DEVICE_NAME_W,
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
                  NULL,          // DisconnectEventHandler,
                  UDPS_ErrorEventHandler,
                  NULL,          // ReceiveEventHandler,
                  UDPS_ReceiveDatagramEventHandler,
                  NULL           // ReceiveExpeditedEventHandler
                  );

   if( !NT_SUCCESS( Status ) )
   {
      //
      // Event Handlers Could Not Be Set
      //
      goto ExitTheThread;
   }

   KdPrint(("UDPS_TestThread: Set Event Handlers On The Server Address Object\n") );

   //
   //
   // Add The Server To The Active Server List
   //
	InsertTailList(
		&g_UDPServerList,
		&pServer->m_ListElement
		);

   Status = KeWaitForSingleObject(
               &g_UDPS_KillEvent,  // Object to wait on.
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

   KdPrint(("UDPS_TestThread: Exiting...\n") );

   if( pServer )
   {
      //
      // Close The Transport Address
      //
      KS_CloseTransportAddress( &pServer->m_KS_Address );

      NdisFreeMemory( pServer, sizeof( UDPS_SERVER ), 0 );
   }

   pServer = NULL;

   (void)PsTerminateSystemThread( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// UDPS_StartTest
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
UDPS_StartTest(
   IN PDEVICE_OBJECT pDeviceObject,
   IN PIRP pIrp
   )
{
   NDIS_STATUS          nNdisStatus;
   PTTCP_TEST_START_CMD pStartCmd = NULL;
   PUDPS_SERVER         pServer = NULL;

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
                  sizeof( UDPS_SERVER ),
                  0,       // Allocate non-paged system-space memory
                  HighestAcceptableMax
                  );

   if( !NT_SUCCESS( nNdisStatus ) )
   {
      return( nNdisStatus );
   }

   NdisZeroMemory( pServer, sizeof( UDPS_SERVER ) );

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
      0L,               // desired access
      NULL,             // object attributes
      NULL,             // process handle
      NULL,             // client id
      UDPS_TestThread,  // start routine
      (PVOID )pServer  // start context
      );

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// UDPS_DeviceIoControl (IRP_MJ_DEVICE_CONTROL Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDI TTCP UDP Server device IOCTL requests.
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
UDPS_DeviceIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;
   NTSTATUS             Status;
   PIO_STACK_LOCATION   pIrpSp;
   ULONG                nFunctionCode;

   KdPrint(("UDPS_DeviceIoControl: Entry...\n") );

   pDeviceExtension = pDeviceObject->DeviceExtension;

   pIrp->IoStatus.Information = 0;      // Nothing Returned Yet

   pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

   nFunctionCode=pIrpSp->Parameters.DeviceIoControl.IoControlCode;

   switch( nFunctionCode )
   {
      case IOCTL_UDP_SERVER_START_TEST:
         Status = UDPS_StartTest( pDeviceObject, pIrp );
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
//// UDPS_DeviceCleanup (IRP_MJ_CLEANUP Dispatch Routine)
//
// Purpose
// This is the dispatch routine for TDI TTCP UDP Server device cleanup
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
UDPS_DeviceCleanup(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   KdPrint(("UDPS_DeviceCleanup: Entry...\n"));

   pDeviceExtension = pDeviceObject->DeviceExtension;

   pIrp->IoStatus.Status = STATUS_SUCCESS;
   pIrp->IoStatus.Information = 0;

   return( STATUS_SUCCESS );
}


/////////////////////////////////////////////////////////////////////////////
//// UDPS_DeviceUnload
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
UDPS_DeviceUnload(
   IN PDEVICE_OBJECT pDeviceObject
   )
{
   PDEVICE_EXTENSION pDeviceExtension;
   KEVENT            UDPS_UnloadEvent;

   KdPrint(("UDPS_DeviceUnload: Entry...\n") );

   pDeviceExtension = pDeviceObject->DeviceExtension;

   KeSetEvent( &g_UDPS_KillEvent, 0, FALSE);

   //
   //
   // Destroy The Symbolic Link
   //
   if( g_bSymbolicLinkCreated )
   {
      UNICODE_STRING UnicodeDeviceName;

      NdisInitUnicodeString(
         &UnicodeDeviceName,
         TDI_UDP_SERVER_DEVICE_NAME_W
         );

      KS_CreateSymbolicLink(
         &UnicodeDeviceName,
         FALSE
         );
   }
}
