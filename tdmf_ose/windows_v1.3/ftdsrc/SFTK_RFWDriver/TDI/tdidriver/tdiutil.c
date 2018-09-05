//tdiutil.c

//#include "KSUtil.h"
//#include "INetInc.h"
//#include "Tdiioctl.h"

#include "tdiutil.h"


NTSTATUS
SftkDeviceLoadC(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
	UNICODE_STRING UnicodeDeviceName;
	PDEVICE_OBJECT pDeviceObject = NULL;
	PDEVICE_EXTENSION pDeviceExtension = NULL;
	NTSTATUS Status = STATUS_SUCCESS;
	NTSTATUS ErrorCode = STATUS_SUCCESS;

	KdPrint(("SftkDeviceLoadC: Entry...\n") );

	//
	// Initialize Global Data
	//

	//
	// Set up the driver's device entry points.
	//
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = SftkTDIDeviceOpen;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE]  = SftkTDIDeviceClose;
	pDriverObject->MajorFunction[IRP_MJ_READ]   = SftkTDIDeviceRead;
	pDriverObject->MajorFunction[IRP_MJ_WRITE]  = SftkTDIDeviceWrite;
	pDriverObject->MajorFunction[IRP_MJ_CLEANUP]  = SftkTDIDeviceCleanup;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = SftkTDIDeviceIoControl;

	pDriverObject->DriverUnload = SftkTDIDriverUnload;

	//
	// Create The TDI TTCP TCP Client Device
	//
	NdisInitUnicodeString(
		  &UnicodeDeviceName,
		SFTK_TDI_TCP_CLIENT_DEVICE_NAME_W
		);

	Status = IoCreateDevice(
		        pDriverObject,
			    sizeof(DEVICE_EXTENSION),
				&UnicodeDeviceName,
				SFTK_FILE_DEVICE_TCP_CLIENT,
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


   pDeviceObject->Flags |= DO_DIRECT_IO;   // Effects Read/Write Only...

   //
   // Initialize The Device Extension
   //
   pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

   RtlZeroMemory( pDeviceExtension, sizeof(DEVICE_EXTENSION) );

   pDeviceExtension->pDeviceObject = pDeviceObject;


   pDeviceExtension->bSymbolicLinkCreated = FALSE;
   //
   // Create The TDI TTCP TCP Client Device Symbolic Link
   //
   Status = SFTK_TDI_CreateSymbolicLink(
               &UnicodeDeviceName,
               TRUE
               );

   if( NT_SUCCESS (Status ) )
   {
	   pDeviceExtension->bSymbolicLinkCreated = TRUE;
   }


   //
   // Setup The Driver Device Entry Points
   //
   pDeviceExtension->MajorFunction[IRP_MJ_CREATE] = SftkTCPCDeviceOpen;
   pDeviceExtension->MajorFunction[IRP_MJ_CLOSE]  = SftkTCPCDeviceClose;
   pDeviceExtension->MajorFunction[IRP_MJ_READ]   = NULL;
   pDeviceExtension->MajorFunction[IRP_MJ_WRITE]  = NULL;
   pDeviceExtension->MajorFunction[IRP_MJ_CLEANUP]  = SftkTCPCDeviceCleanup;
   pDeviceExtension->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = SftkTCPCDeviceIoControl;

   pDeviceExtension->DeviceUnload = SftkTCPCDeviceUnload;

   pDeviceExtension->SessionManager.bInitialized = 0;


   //
   // Fetch Transport Provider Information
   //
   Status = SFTK_TDI_QueryProviderInfo(
               TCP_DEVICE_NAME_W,
			   &pDeviceExtension->TcpContext.TdiProviderInfo
               );

#ifdef DBG
   if (NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderInfo(
         TCP_DEVICE_NAME_W,
         &pDeviceExtension->TcpContext.TdiProviderInfo
         );
   }
#endif // DBG

	return Status;
}

NTSTATUS
SftkDeviceLoadS(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
   UNICODE_STRING UnicodeDeviceName;
   PDEVICE_OBJECT pDeviceObject = NULL;
   PDEVICE_EXTENSION pDeviceExtension = NULL;
   NTSTATUS Status = STATUS_SUCCESS;
   NTSTATUS ErrorCode = STATUS_SUCCESS;

   KdPrint(("SftkDeviceLoadS: Entry...\n") );

   //
   // Initialize Global Data
   //

   //
   // Set up the driver's device entry points.
   //
   pDriverObject->MajorFunction[IRP_MJ_CREATE] = SftkTDIDeviceOpen;
   pDriverObject->MajorFunction[IRP_MJ_CLOSE]  = SftkTDIDeviceClose;
   pDriverObject->MajorFunction[IRP_MJ_READ]   = SftkTDIDeviceRead;
   pDriverObject->MajorFunction[IRP_MJ_WRITE]  = SftkTDIDeviceWrite;
   pDriverObject->MajorFunction[IRP_MJ_CLEANUP]  = SftkTDIDeviceCleanup;
   pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = SftkTDIDeviceIoControl;

   pDriverObject->DriverUnload = SftkTDIDriverUnload;

   //
   // Create The TDI TTCP TCP Server Device
   //
   NdisInitUnicodeString(
      &UnicodeDeviceName,
      SFTK_TDI_TCP_SERVER_DEVICE_NAME_W
      );

   Status = IoCreateDevice(
               pDriverObject,
               sizeof(DEVICE_EXTENSION),
               &UnicodeDeviceName,
               SFTK_FILE_DEVICE_TCP_SERVER,
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


   pDeviceObject->Flags |= DO_DIRECT_IO;   // Effects Read/Write Only...

   //
   // Initialize The Device Extension
   //
   pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

   RtlZeroMemory( pDeviceExtension, sizeof(DEVICE_EXTENSION) );

   pDeviceExtension->pDeviceObject = pDeviceObject;

   pDeviceExtension->bSymbolicLinkCreated = FALSE;
   //
   // Create The TDI TTCP TCP Server Device Symbolic Link
   //
   Status = SFTK_TDI_CreateSymbolicLink(
               &UnicodeDeviceName,
               TRUE
               );

   if( NT_SUCCESS (Status ) )
   {
      pDeviceExtension->bSymbolicLinkCreated = TRUE;
   }


   //
   // Setup The Driver Device Entry Points
   //
   pDeviceExtension->MajorFunction[IRP_MJ_CREATE] = SftkTCPSDeviceOpen;
   pDeviceExtension->MajorFunction[IRP_MJ_CLOSE]  = SftkTCPSDeviceClose;
   pDeviceExtension->MajorFunction[IRP_MJ_READ]   = NULL;
   pDeviceExtension->MajorFunction[IRP_MJ_WRITE]  = NULL;
   pDeviceExtension->MajorFunction[IRP_MJ_CLEANUP]  = SftkTCPSDeviceCleanup;
   pDeviceExtension->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = SftkTCPSDeviceIoControl;

   pDeviceExtension->DeviceUnload = SftkTCPSDeviceUnload;

   pDeviceExtension->SessionManager.bInitialized = 0;

   //
   // Fetch Transport Provider Information
   //
   Status = SFTK_TDI_QueryProviderInfo(
               TCP_DEVICE_NAME_W,
               &pDeviceExtension->TcpContext.TdiProviderInfo
               );

#ifdef DBG
   if (NT_SUCCESS(Status))
   {
      DEBUG_DumpProviderInfo(
         TCP_DEVICE_NAME_W,
         &pDeviceExtension->TcpContext.TdiProviderInfo
         );
   }
#endif // DBG

//Has to do Something about this
//But will wait until everything gets compiled
//Until Then everything is commented out
//Veera
//   InitializeListHead( &g_TCPServerList );
//Veera


   return STATUS_SUCCESS;
}

////////////////////////////////////////
/////Below are the Global functions that will be called from the DriverEntry()
///////////////////////////////////////

NTSTATUS
SftkTDIDeviceOpen(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   pDeviceExtension = pDeviceObject->DeviceExtension;

   //
   // Possibly Dispatch To Secondary Function Handler
   // -----------------------------------------------
   // The TDITTCP driver creates four devices:
   //   TDI SFTK TCP Server
   //   TDI SFTK UDP Server
   //
   // A different DEVICE_EXTENSION structure is created for each device.
   //
   // The device extension includes a secondary MajorFunction table that
   // points to different functions for each device. This means that each
   // of the four types of devices listed above can have their own
   // DeviceOpen routine.
   //
   if( pDeviceExtension && pDeviceExtension->MajorFunction[ IRP_MJ_CREATE ] )
   {
      return( (*pDeviceExtension->MajorFunction[ IRP_MJ_CREATE ])
               ( pDeviceObject, pIrp )
            );
   }

   KdPrint(("TDITTCPDeviceOpen: Default Handling...\n") );

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

   pIrp->IoStatus.Information = 0;

   TdiCompleteRequest( pIrp, STATUS_SUCCESS );

   return STATUS_SUCCESS;
}


NTSTATUS
SftkTDIDeviceClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   pDeviceExtension = pDeviceObject->DeviceExtension;

   //
   // Possibly Dispatch To Secondary Function Handler
   // -----------------------------------------------
   // The TDITTCP driver creates four devices:
   //   TDI TTCP TCP Server
   //   TDI TTCP UDP Server
   //   TDI TTCP TCP Client
   //   TDI TTCP UDP Client
   //
   // A different DEVICE_EXTENSION structure is created for each device.
   //
   // The device extension includes a secondary MajorFunction table that
   // points to different functions for each device. This means that each
   // of the four types of devices listed above can have their own
   // DeviceOpen routine.
   //
   if( pDeviceExtension && pDeviceExtension->MajorFunction[ IRP_MJ_CLOSE ] )
   {
      return( (*pDeviceExtension->MajorFunction[ IRP_MJ_CLOSE ])
               ( pDeviceObject, pIrp )
            );
   }

   KdPrint(("TDITTCPDeviceClose: Default Handling...\n") );

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

   KdPrint( ("TDITTCPDeviceClose: Closed!!\n") );

   pIrp->IoStatus.Information = 0;

   TdiCompleteRequest( pIrp, STATUS_SUCCESS );

   return STATUS_SUCCESS;
}

NTSTATUS
SftkTDIDeviceRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   pIrp->IoStatus.Information = 0;      // Nothing Returned Yet

   pDeviceExtension = pDeviceObject->DeviceExtension;

   //
   // Possibly Dispatch To Secondary Function Handler
   // -----------------------------------------------
   // The TDITTCP driver creates four devices:
   //   TDI TTCP TCP Server
   //   TDI TTCP UDP Server
   //   TDI TTCP TCP Client
   //   TDI TTCP UDP Client
   //
   // A different DEVICE_EXTENSION structure is created for each device.
   //
   // The device extension includes a secondary MajorFunction table that
   // points to different functions for each device. This means that each
   // of the four types of devices listed above can have their own
   // DeviceOpen routine.
   //
   if( pDeviceExtension && pDeviceExtension->MajorFunction[ IRP_MJ_READ ] )
   {
      return( (*pDeviceExtension->MajorFunction[ IRP_MJ_READ ])
               ( pDeviceObject, pIrp )
            );
   }

   KdPrint(("TDITTCPDeviceRead: Default Handling...\n") );

   //
   // Return Failure
   //
   TdiCompleteRequest( pIrp, STATUS_NOT_IMPLEMENTED );

   return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SftkTDIDeviceWrite(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   pIrp->IoStatus.Information = 0;      // Nothing Returned Yet

   pDeviceExtension = pDeviceObject->DeviceExtension;

   if( pDeviceExtension && pDeviceExtension->MajorFunction[ IRP_MJ_WRITE ] )
   {
      return( (*pDeviceExtension->MajorFunction[ IRP_MJ_WRITE ])
               ( pDeviceObject, pIrp )
            );
   }

   KdPrint(("TDITTCPDeviceWrite: Default Handling...\n") );

   //
   // Return Failure
   //
   TdiCompleteRequest( pIrp, STATUS_NOT_IMPLEMENTED );

   return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
SftkTDIDeviceIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;
   NTSTATUS             Status;
   PIO_STACK_LOCATION   pIrpSp;
   ULONG                nFunctionCode;

   pIrp->IoStatus.Information = 0;      // Nothing Returned Yet

   pDeviceExtension = pDeviceObject->DeviceExtension;

   if( pDeviceExtension && pDeviceExtension->MajorFunction[ IRP_MJ_DEVICE_CONTROL ] )
   {
      return( (*pDeviceExtension->MajorFunction[ IRP_MJ_DEVICE_CONTROL ])
               ( pDeviceObject, pIrp )
            );
   }

   KdPrint(("TDITTCPDeviceIoControl: Default Handling...\n") );

   pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

   nFunctionCode=pIrpSp->Parameters.DeviceIoControl.IoControlCode;

   switch( nFunctionCode )
   {
      default:
         KdPrint((  "FunctionCode: 0x%8.8X\n", nFunctionCode ));
         Status = STATUS_NOT_IMPLEMENTED;
         break;
   }

   TdiCompleteRequest( pIrp, Status );

   return( Status );
}

NTSTATUS
SftkTDIDeviceCleanup(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension;

   pIrp->IoStatus.Information = 0;      // Nothing Returned Yet

   pDeviceExtension = pDeviceObject->DeviceExtension;

   if( pDeviceExtension && pDeviceExtension->MajorFunction[ IRP_MJ_CLEANUP ] )
   {
      return( (*pDeviceExtension->MajorFunction[ IRP_MJ_CLEANUP ])
               ( pDeviceObject, pIrp )
            );
   }

   KdPrint(("TDITTCPDeviceCleanup: Default Handling...\n"));

   pIrp->IoStatus.Status = STATUS_SUCCESS;

   return( STATUS_SUCCESS );
}


VOID
SftkTDIDriverUnload(
    IN PDRIVER_OBJECT pDriverObject
    )
{
   PDEVICE_OBJECT     pDeviceObject;
   PDEVICE_OBJECT     pOldDeviceObject;
   PDEVICE_EXTENSION  pDeviceExtension;

   KdPrint(("TDITTCPDriverUnload: Entry...\n") );

   //
   // Delete The Driver's Devices
   //
   pDeviceObject = pDriverObject->DeviceObject;

   while( pDeviceObject != NULL )
   {
      pDeviceExtension = pDeviceObject->DeviceExtension;

      if( pDeviceExtension && pDeviceExtension->DeviceUnload )
      {
         (*pDeviceExtension->DeviceUnload)( pDeviceObject );
      }

      //
      // Now Delete This Device Object
      //
      pOldDeviceObject = pDeviceObject;
      pDeviceObject = pDeviceObject->NextDevice;

      IoDeleteDevice( pOldDeviceObject );
   }
}

////////////////////////////////////////////////////
////////////Client Side Functions that will take the functionlity from
////////////the DriverEntry()
////////////////////////////////////////////////////

NTSTATUS
SftkTCPCDeviceOpen(
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

NTSTATUS
SftkTCPCDeviceClose(
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

NTSTATUS SftkTCPCDisconnectEventHandler(
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

NTSTATUS SftkTCPCErrorEventHandler(
   IN PVOID TdiEventContext,  // The endpoint's file object.
   IN NTSTATUS Status         // Status code indicating error type.
   )
{
   KdPrint(("TCPC_ErrorEventHandler: Status: 0x%8.8X\n", Status) );

   return( STATUS_SUCCESS );
}

NTSTATUS SftkTCPCReceiveExpeditedEventHandler(
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
   KdPrint(("SftkTCPCReceiveExpeditedEventHandler: Entry...\n") );

   KdPrint(("  Bytes Indicated: %d; BytesAvailable: %d; Flags: 0x%8.8x\n",
      BytesIndicated, BytesAvailable, ReceiveFlags));

   return( STATUS_SUCCESS );
}



VOID
SftkDoQueryAddressInfoTest(
   PTCP_SESSION  pSession
   )
{
//	PTCPC_SESSION pCliSession = pSession->clientSession;
	NTSTATUS    Status = STATUS_SUCCESS;
	PUCHAR	pInfoBuffer= NULL;
	ULONG nInfoBufferSize = 256;
	
	NDIS_STATUS NdisStatus = NdisAllocateMemoryWithTag((PVOID)pInfoBuffer,nInfoBufferSize,MEM_TAG);

	if(NdisStatus != NDIS_STATUS_SUCCESS)
	{
		KdPrint(("Failed To Allocate Memory\n"));
		return;
	}


   //
   // Query TDI Address Info On Transport Address
   //
   Status = SFTK_TDI_QueryAddressInfo(
				pSession->pServer->KSAddress.m_pFileObject,  // Address Object
               pInfoBuffer,
               &nInfoBufferSize
               );

   KdPrint(( "Query Address Info Status: 0x%8.8x\n", Status ));

   if( NT_SUCCESS( Status ) )
   {
      DEBUG_DumpAddressInfo( (PTDI_ADDRESS_INFO )pInfoBuffer );
   }

   //
   // Query TDI Address Info On Connection Endpoint
   //
   Status = SFTK_TDI_QueryAddressInfo(
               pSession->KSEndpoint.m_pFileObject,  // Connection Object
               pInfoBuffer,
               &nInfoBufferSize
               );

   KdPrint(( "Query Address Info Status: 0x%8.8x\n", Status ));

   if( NT_SUCCESS( Status ) )
   {
      DEBUG_DumpAddressInfo( (PTDI_ADDRESS_INFO )pInfoBuffer );
   }

   NdisFreeMemory((PVOID)pInfoBuffer,nInfoBufferSize,0);
}

VOID
SftkDoQueryConnectionInfoTest(
   PTCP_SESSION  pSession
   )
{
//	PTCPC_SESSION pCliSession = pSession->clientSession;
	NTSTATUS    Status = STATUS_SUCCESS;
//   ULONG       nInfoBufferSize = sizeof( pSession->m_InfoBuffer );

	PUCHAR	pInfoBuffer= NULL;
	ULONG nInfoBufferSize = 256;
	
	NDIS_STATUS NdisStatus = NdisAllocateMemoryWithTag((PVOID)pInfoBuffer,nInfoBufferSize,MEM_TAG);

	if(NdisStatus != NDIS_STATUS_SUCCESS)
	{
		KdPrint(("Failed To Allocate Memory\n"));
		return;
	}

   //
   // Query TDI Connection Info
   //
   Status = SFTK_TDI_QueryConnectionInfo(
				&pSession->KSEndpoint,  // Connection Endpoint
				pInfoBuffer,
				&nInfoBufferSize
               );

   KdPrint(( "Query Connection Info Status: 0x%8.8x\n", Status ));

   if( NT_SUCCESS( Status ) )
   {
      DEBUG_DumpConnectionInfo( (PTDI_CONNECTION_INFO )pInfoBuffer );
   }
   NdisFreeMemory((PVOID)pInfoBuffer,nInfoBufferSize,0);
}

void DUMP_ConnectionInfo(PCONNECTION_DETAILS lpConnectionDetails)
{
	PCONNECTION_INFO pConnectionInfo = NULL;
	int i =0;

	KdPrint(("The SendWindow = %d , ReceiveWindow = %d , Number of Connections = %d\n",
		lpConnectionDetails->nSendWindowSize, lpConnectionDetails->nReceiveWindowSize,
		lpConnectionDetails->nConnections));

	pConnectionInfo = lpConnectionDetails->ConnectionDetails;
	for(i=0;i<lpConnectionDetails->nConnections;i++)
	{
		KdPrint(("The Connection Local Address = %lx , Local Port = %lx , \n RemoteAddres = %lx , Remote Port = %lx , \n NumberOfSession = %lx\n",
			   pConnectionInfo->ipLocalAddress.in_addr, pConnectionInfo->ipLocalAddress.sin_port,
			   pConnectionInfo->ipRemoteAddress.in_addr, pConnectionInfo->ipRemoteAddress.sin_port,
			   pConnectionInfo->nNumberOfSessions));
		pConnectionInfo += 1;
	}
}

NTSTATUS
SftkStartConnectionThread(
   IN PDEVICE_OBJECT pDeviceObject,
   IN PIRP pIrp
   )
{
   LONG lErrorCode=0;
   PCONNECTION_DETAILS pConnectionDetails = NULL;
   HANDLE	hConnectThread = NULL;
   PIO_STACK_LOCATION   pIrpSp;
   NDIS_STATUS nNdisStatus = STATUS_SUCCESS;


   try
   {
		//
		// Initialize Default Settings
		//
		pIrp->IoStatus.Information = sizeof( ULONG );  // For m_Status

		pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
		//
		// Locate Test Start Command Buffer
		//
//		pConnectionDetails = (PCONNECTION_DETAILS )pIrp->AssociatedIrp.SystemBuffer;

		DUMP_ConnectionInfo((PCONNECTION_DETAILS)pIrp->AssociatedIrp.SystemBuffer);


		nNdisStatus = NdisAllocateMemoryWithTag(
			&((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->SessionManager.lpConnectionDetails,
						pIrpSp->Parameters.DeviceIoControl.InputBufferLength,
						MEM_TAG
						);

		if(!NT_SUCCESS(nNdisStatus))
		{
			KdPrint(("Unable to Accocate Memory for SendBuffer Status = %lx\n",nNdisStatus));
			leave;
		}

		//Initializing the Memory
		NdisZeroMemory(((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->SessionManager.lpConnectionDetails,pIrpSp->Parameters.DeviceIoControl.InputBufferLength);

		NdisMoveMemory(((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->SessionManager.lpConnectionDetails,pIrp->AssociatedIrp.SystemBuffer,pIrpSp->Parameters.DeviceIoControl.InputBufferLength);

		DUMP_ConnectionInfo(((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->SessionManager.lpConnectionDetails);


//		((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->lpConnectionDetails = pConnectionDetails;

//		lErrorCode = InitializeConnectThread(&((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->SessionManager);

		//
		// Start The Thread That Will Execute The Test
		//
		PsCreateSystemThread(
			&hConnectThread,   // thread handle
			0L,                        // desired access
			NULL,                      // object attributes
			NULL,                      // process handle
			NULL,                      // client id
			CreateConnectThread,           // start routine
			(PVOID )&((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->SessionManager            // start context
			);
   }
   finally
   {

   }

   return( STATUS_SUCCESS );
}


NTSTATUS
SftkTCPCDeviceIoControl(
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
      case SFTK_IOCTL_TCP_CLIENT_START_CACHE:
		   Status = STATUS_SUCCESS;
//This Has to be removed for the time being
//Will Add Custom Functionality Later
//Veera
//         Status = TCPC_StartTest( pDeviceObject, pIrp );
//Veera
         break;
	  case SFTK_IOCTL_TCP_CLIENT_START_CONNECTIONS:
		  Status = SftkStartConnectionThread(pDeviceObject,pIrp);
		  break;

      default:
         KdPrint((  "FunctionCode: 0x%8.8X\n", nFunctionCode ));
         Status = STATUS_UNSUCCESSFUL;
         break;
   }

   TdiCompleteRequest( pIrp, Status );

   return( Status );
}

NTSTATUS
SftkTCPCDeviceCleanup(
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

VOID
SftkTCPCDeviceUnload(
   IN PDEVICE_OBJECT pDeviceObject
   )
{
   PDEVICE_EXTENSION  pDeviceExtension;

   KdPrint(("TCPC_DeviceUnload: Entry...\n") );

   pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

   UninitializeConnectListenThread(&pDeviceExtension->SessionManager);

   //
   // Destroy The Symbolic Link
   //
   if( pDeviceExtension->bSymbolicLinkCreated)
   {
      UNICODE_STRING UnicodeDeviceName;

      NdisInitUnicodeString(
         &UnicodeDeviceName,
         SFTK_TDI_TCP_CLIENT_DEVICE_NAME_W
         );

      SFTK_TDI_CreateSymbolicLink(
         &UnicodeDeviceName,
         FALSE
         );
   }
}


//////////////////////////////////
/////////Server Specific Functions
//////////////////////////////////

NTSTATUS
SftkTCPSDeviceOpen(
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

NTSTATUS
SftkTCPSDeviceClose(
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
SftkStartListenThread(
   IN PDEVICE_OBJECT pDeviceObject,
   IN PIRP pIrp
   )
{
   LONG lErrorCode=0;
   HANDLE	hConnectThread = NULL;
   PIO_STACK_LOCATION   pIrpSp;
   NDIS_STATUS nNdisStatus = STATUS_SUCCESS;


   try
   {
   //
   // Initialize Default Settings
   //
   pIrp->IoStatus.Information = sizeof( ULONG );  // For m_Status

   pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

   //
   // Locate Test Start Command Buffer
   //
//   pConnectionDetails = (PCONNECTION_DETAILS )pIrp->AssociatedIrp.SystemBuffer;

	DUMP_ConnectionInfo((PCONNECTION_DETAILS)pIrp->AssociatedIrp.SystemBuffer);


	nNdisStatus = NdisAllocateMemoryWithTag(
		&((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->SessionManager.lpConnectionDetails,
					pIrpSp->Parameters.DeviceIoControl.InputBufferLength,
					MEM_TAG
					);

	if(!NT_SUCCESS(nNdisStatus))
	{
		KdPrint(("Unable to Accocate Memory for SendBuffer Status = %lx\n",nNdisStatus));
		leave;
	}

	//Initializing the Memory
	NdisZeroMemory(((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->SessionManager.lpConnectionDetails,pIrpSp->Parameters.DeviceIoControl.InputBufferLength);

	NdisMoveMemory(((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->SessionManager.lpConnectionDetails,pIrp->AssociatedIrp.SystemBuffer,pIrpSp->Parameters.DeviceIoControl.InputBufferLength);

	DUMP_ConnectionInfo(((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->SessionManager.lpConnectionDetails);

	//Changed for System Context
//	lErrorCode = InitializeListenThread(&((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->SessionManager);

   //
   // Start The Thread That Will Execute The Test
   //
   PsCreateSystemThread(
      &hConnectThread,   // thread handle
      0L,                        // desired access
      NULL,                      // object attributes
      NULL,                      // process handle
      NULL,                      // client id
      CreateListenThread,           // start routine
      (PVOID )&((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->SessionManager            // start context
      );
   }
   finally
   {

   }

   return( STATUS_SUCCESS );
}

NTSTATUS
SftkTCPSDeviceIoControl(
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
      case SFTK_IOCTL_TCP_SERVER_START:

		  Status = SftkStartListenThread(pDeviceObject,pIrp);
//////////////////////
//This has to be removed for the time being will have to replace with an equivalent
//Functionality
//Veera
//         Status = TCPS_StartTest( pDeviceObject, pIrp );
//Veera
		 Status = STATUS_SUCCESS;
         break;

      default:
         KdPrint((  "FunctionCode: 0x%8.8X\n", nFunctionCode ));
         Status = STATUS_UNSUCCESSFUL;
         break;
   }

   TdiCompleteRequest( pIrp, Status );

   return( Status );
}

NTSTATUS
SftkTCPSDeviceCleanup(
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

VOID
SftkTCPSDeviceUnload(
   IN PDEVICE_OBJECT pDeviceObject
   )
{
   PDEVICE_EXTENSION pDeviceExtension;
//   PTCPS_SERVER      pServer = NULL;
   KEVENT            TCPSUnloadEvent;
   LARGE_INTEGER     UnloadWait;
   NTSTATUS          Status;

   KdPrint(("TCPS_DeviceUnload: Entry...\n") );

   pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

   UninitializeConnectListenThread(&pDeviceExtension->SessionManager);

   //
   // Destroy The Symbolic Link
   //
   if( pDeviceExtension->bSymbolicLinkCreated)
   {
      UNICODE_STRING UnicodeDeviceName;

      NdisInitUnicodeString(
         &UnicodeDeviceName,
         SFTK_TDI_TCP_SERVER_DEVICE_NAME_W
         );

      SFTK_TDI_CreateSymbolicLink(
         &UnicodeDeviceName,
         FALSE
         );
  }
}


VOID
SftkTCPSDisconnectCallback(
    PVOID UserCompletionContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG Reserved
   )
{
	return;
}



////Server Event Handler Routines


NTSTATUS SftkTCPSServerErrorEventHandler(
   IN PVOID TdiEventContext,  // The endpoint's file object.
   IN NTSTATUS Status         // Status code indicating error type.
   )
{
   KdPrint(("TCPS_ServerErrorEventHandler: Status: 0x%8.8X\n", Status) );

   return( STATUS_SUCCESS );
}

NTSTATUS SftkTCPSSessionErrorEventHandler(
   IN PVOID TdiEventContext,  // The endpoint's file object.
   IN NTSTATUS Status         // Status code indicating error type.
   )
{
   KdPrint(("TCPS_SessionErrorEventHandler: Status: 0x%8.8X\n", Status) );

   return( STATUS_SUCCESS );
}

NTSTATUS SftkTCPSReceiveExpeditedEventHandler(
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


