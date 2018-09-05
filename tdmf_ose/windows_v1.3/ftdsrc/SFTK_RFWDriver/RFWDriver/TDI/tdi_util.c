//tdiutil.c

#include "sftk_main.h"
#include "tdi_util.h"


NTSTATUS
SftkDeviceLoadC(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
	UNICODE_STRING UnicodeDeviceName;
	PDEVICE_OBJECT pDeviceObject = NULL;
	PDEVICE_EXTENSION pDeviceExtension = NULL;
	NTSTATUS status = STATUS_SUCCESS;
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

	status = IoCreateDevice(
		        pDriverObject,
			    sizeof(DEVICE_EXTENSION),
				&UnicodeDeviceName,
				SFTK_FILE_DEVICE_TCP_CLIENT,
				0,            // Standard device characteristics
				FALSE,      // This isn't an exclusive device
				&pDeviceObject
               );

   if( !NT_SUCCESS( status ) )
   {
      KdPrint(("TDITTCP: IoCreateDevice() failed:\n") );

      status = STATUS_UNSUCCESSFUL;

      return(status);
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
   status = TDI_CreateSymbolicLink(
               &UnicodeDeviceName,
               TRUE
               );

   if( NT_SUCCESS (status ) )
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

	return status;
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
   NTSTATUS status = STATUS_SUCCESS;
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

   status = IoCreateDevice(
               pDriverObject,
               sizeof(DEVICE_EXTENSION),
               &UnicodeDeviceName,
               SFTK_FILE_DEVICE_TCP_SERVER,
               0,            // Standard device characteristics
               FALSE,      // This isn't an exclusive device
               &pDeviceObject
               );

   if( !NT_SUCCESS( status ) )
   {
      KdPrint(("TDITTCP: IoCreateDevice() failed:\n") );

      status = STATUS_UNSUCCESSFUL;

      return(status);
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
   status = TDI_CreateSymbolicLink(
               &UnicodeDeviceName,
               TRUE
               );

   if( NT_SUCCESS (status ) )
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
   NTSTATUS             status;
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
         status = STATUS_NOT_IMPLEMENTED;
         break;
   }

   TdiCompleteRequest( pIrp, status );

   return( status );
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

NTSTATUS
SftkTCPCDeviceIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension	= NULL;
   NTSTATUS             status				= STATUS_SUCCESS;
   PIO_STACK_LOCATION   pIrpSp				= NULL;
   ULONG                nFunctionCode;
   PVOID				pSystemBuffer		= NULL;
   ULONG				nSizeOfBuffer		= 0;
   PCONNECTION_DETAILS	pConnection_detail	= NULL;
   PSM_INIT_PARAMS		pSMInitParams		= NULL;
   
   KdPrint(("SftkTCPCDeviceIoControl: Entry...\n") );

   pDeviceExtension = pDeviceObject->DeviceExtension;

   pIrp->IoStatus.Information = 0;      // Nothing Returned Yet

   pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

   nFunctionCode = pIrpSp->Parameters.DeviceIoControl.IoControlCode;
   pSystemBuffer = pIrp->AssociatedIrp.SystemBuffer;
   nSizeOfBuffer = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;


   switch( nFunctionCode )
   {
   case SFTK_IOCTL_INIT_PMD:
	   ASSERT(pSystemBuffer != NULL);
	   pSMInitParams = (PSM_INIT_PARAMS)pSystemBuffer;

	   DebugPrint((DBG_COM, "SftkTCPCDeviceIoControl : SFTK_IOCTL_INIT_PMD: \n")); 

	   OS_RtlCopyMemory(&pDeviceExtension->SMInitParams , pSMInitParams , sizeof(SM_INIT_PARAMS));
	   status = COM_InitializeSessionManager(&pDeviceExtension->SessionManager , &pDeviceExtension->SMInitParams);
		if (!NT_SUCCESS(status))
		{ // LG Already exist
			DebugPrint((DBG_ERROR, "SftkTCPCDeviceIoControl : SFTK_IOCTL_INIT_PMD: COM_InitializeSessionManager Failed!! returning status 0x%08x \n", 
							status)); 
			// TODO : Call Cache API to initialize Glbal Cache manager.
			break;
		}
	   break;
   case SFTK_IOCTL_UNINIT_PMD:

	   DebugPrint((DBG_COM, "SftkTCPCDeviceIoControl : SFTK_IOCTL_UNINIT_PMD: \n")); 
	   COM_UninitializeSessionManager(&pDeviceExtension->SessionManager);
	   break;
   case SFTK_IOCTL_TCP_ADD_CONNECTIONS:

	   ASSERT(pSystemBuffer != NULL);
	   DebugPrint((DBG_COM, "SftkTCPCDeviceIoControl : SFTK_IOCTL_TCP_ADD_CONNECTIONS: \n")); 
		pConnection_detail = (PCONNECTION_DETAILS) pSystemBuffer;

		// Call Veera's API like:
		status = COM_AddConnections( &pDeviceExtension->SessionManager, pConnection_detail );
		if (!NT_SUCCESS(status))
		{ // LG Already exist
			DebugPrint((DBG_ERROR, "SftkTCPCDeviceIoControl : SFTK_IOCTL_TCP_ADD_CONNECTIONS: COM_AddConnections(LgNum %d) Not Found !! returning status 0x%08x \n", 
							pConnection_detail->lgnum, status)); 
			// TODO : Call Cache API to initialize Glbal Cache manager.
			break;
		}
	   break;
   case SFTK_IOCTL_TCP_REMOVE_CONNECTIONS:

	   ASSERT(pSystemBuffer != NULL);
	   DebugPrint((DBG_COM, "SftkTCPCDeviceIoControl : SFTK_IOCTL_TCP_REMOVE_CONNECTIONS: \n")); 
		pConnection_detail = (PCONNECTION_DETAILS) pSystemBuffer;

		// Call Veera's API like:
		status = COM_RemoveConnections( &pDeviceExtension->SessionManager, pConnection_detail );
		if (!NT_SUCCESS(status))
		{ // LG Already exist
			DebugPrint((DBG_ERROR, "SftkTCPCDeviceIoControl : SFTK_IOCTL_TCP_REMOVE_CONNECTIONS: COM_RemoveConnections(LgNum %d) Not Found !! returning status 0x%08x \n", 
							pConnection_detail->lgnum, status)); 
			// TODO : Call Cache API to initialize Glbal Cache manager.
			break;
		}
	   break;
   case SFTK_IOCTL_TCP_ENABLE_CONNECTIONS:

	   ASSERT(pSystemBuffer != NULL);
	   DebugPrint((DBG_COM, "SftkTCPCDeviceIoControl : SFTK_IOCTL_TCP_ENABLE_CONNECTIONS: \n")); 
		pConnection_detail = (PCONNECTION_DETAILS) pSystemBuffer;

		// Call Veera's API like:
		status = COM_EnableConnections( &pDeviceExtension->SessionManager, pConnection_detail ,TRUE);
		if (!NT_SUCCESS(status))
		{ // LG Already exist
			DebugPrint((DBG_ERROR, "SftkTCPCDeviceIoControl : SFTK_IOCTL_TCP_ENABLE_CONNECTIONS: COM_EnableConnections(LgNum %d) Not Found !! returning status 0x%08x \n", 
							pConnection_detail->lgnum, status)); 
			// TODO : Call Cache API to initialize Glbal Cache manager.
			break;
		}
	   break;
   case SFTK_IOCTL_TCP_DISABLE_CONNECTIONS:

	   ASSERT(pSystemBuffer != NULL);
	   DebugPrint((DBG_COM, "SftkTCPCDeviceIoControl : SFTK_IOCTL_TCP_DISABLE_CONNECTIONS: \n")); 
		pConnection_detail = (PCONNECTION_DETAILS) pSystemBuffer;

		// Call Veera's API like:
		status = COM_EnableConnections( &pDeviceExtension->SessionManager, pConnection_detail ,FALSE);
		if (!NT_SUCCESS(status))
		{ // LG Already exist
			DebugPrint((DBG_ERROR, "SftkTCPCDeviceIoControl : SFTK_IOCTL_TCP_DISABLE_CONNECTIONS: COM_EnableConnections(LgNum %d) Not Found !! returning status 0x%08x \n", 
							pConnection_detail->lgnum, status)); 
			// TODO : Call Cache API to initialize Glbal Cache manager.
			break;
		}
	   break;
   case SFTK_IOCTL_TCP_QUERY_SM_PERFORMANCE:
	   break;
   case SFTK_IOCTL_TCP_SET_CONNECTIONS_TUNABLES:
	   break;
   case SFTK_IOCTL_START_PMD:

	   ASSERT(pSystemBuffer != NULL);
	   pSMInitParams = (PSM_INIT_PARAMS)pSystemBuffer;

	   DebugPrint((DBG_COM, "SftkTCPCDeviceIoControl : SFTK_IOCTL_START_PMD: \n")); 

		status = COM_StartPMD( &pDeviceExtension->SessionManager, pSMInitParams);
		if (!NT_SUCCESS(status))
		{ // LG Already exist
			DebugPrint((DBG_ERROR, "sftk_ctl_ioctl : SFTK_IOCTL_START_PMD: COM_StartPMD(LgNum %d) Not Found !! returning status 0x%08x \n", 
							pSMInitParams->lgnum, status)); 
			// TODO : Call Cache API to initialize Glbal Cache manager.
			break;
		}
	   break;
   case SFTK_IOCTL_STOP_PMD:
	   DebugPrint((DBG_COM, "SftkTCPCDeviceIoControl : SFTK_IOCTL_STOP_PMD: \n")); 
	   COM_StopPMD( &pDeviceExtension->SessionManager);
	   break;
      default:
         KdPrint((  "FunctionCode: 0x%8.8X\n", nFunctionCode ));
         status = STATUS_UNSUCCESSFUL;
         break;
   }

   TdiCompleteRequest( pIrp, status );

   return( status );
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

   COM_UninitializeSessionManager(&pDeviceExtension->SessionManager);

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

      TDI_CreateSymbolicLink(
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
SftkTCPSDeviceIoControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
   PDEVICE_EXTENSION    pDeviceExtension	= NULL;
   NTSTATUS             status				= STATUS_SUCCESS;
   PIO_STACK_LOCATION   pIrpSp				= NULL;
   ULONG                nFunctionCode;
   PVOID				pSystemBuffer		= NULL;
   ULONG				nSizeOfBuffer		= 0;
   PCONNECTION_DETAILS	pConnection_detail	= NULL;
   PSM_INIT_PARAMS		pSMInitParams		= NULL;

   KdPrint(("SftkTCPSDeviceIoControl: Entry...\n") );

   pDeviceExtension = pDeviceObject->DeviceExtension;

   pIrp->IoStatus.Information = 0;      // Nothing Returned Yet

   pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

   nFunctionCode=pIrpSp->Parameters.DeviceIoControl.IoControlCode;
   pSystemBuffer = pIrp->AssociatedIrp.SystemBuffer;
   nSizeOfBuffer = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;

   switch( nFunctionCode )
   {
   case SFTK_IOCTL_INIT_RMD:
	   ASSERT(pSystemBuffer != NULL);
	   pSMInitParams = (PSM_INIT_PARAMS)pSystemBuffer;

	   DebugPrint((DBG_COM, "SftkTCPCDeviceIoControl : SFTK_IOCTL_INIT_RMD: \n")); 

	   OS_RtlCopyMemory(&pDeviceExtension->SMInitParams , pSMInitParams , sizeof(SM_INIT_PARAMS));
	   status = COM_InitializeSessionManager(&pDeviceExtension->SessionManager , &pDeviceExtension->SMInitParams);
		if (!NT_SUCCESS(status))
		{ // LG Already exist
			DebugPrint((DBG_ERROR, "SftkTCPCDeviceIoControl : SFTK_IOCTL_INIT_RMD: COM_InitializeSessionManager Failed!! returning status 0x%08x \n", 
							status)); 
			// TODO : Call Cache API to initialize Glbal Cache manager.
			break;
		}
	   break;
   case SFTK_IOCTL_UNINIT_RMD:

       DebugPrint((DBG_COM, "SftkTCPCDeviceIoControl : SFTK_IOCTL_UNINIT_RMD: \n")); 
	   COM_UninitializeSessionManager(&pDeviceExtension->SessionManager);
	   break;
   case SFTK_IOCTL_TCP_ADD_CONNECTIONS:

	   ASSERT(pSystemBuffer != NULL);
	   DebugPrint((DBG_COM, "SftkTCPCDeviceIoControl : SFTK_IOCTL_TCP_ADD_CONNECTIONS: \n")); 
		pConnection_detail = (PCONNECTION_DETAILS) pSystemBuffer;

		// Call Veera's API like:
		status = COM_AddConnections( &pDeviceExtension->SessionManager, pConnection_detail );
		if (!NT_SUCCESS(status))
		{ // LG Already exist
			DebugPrint((DBG_ERROR, "SftkTCPCDeviceIoControl : SFTK_IOCTL_TCP_ADD_CONNECTIONS: COM_AddConnections(LgNum %d) Not Found !! returning status 0x%08x \n", 
							pConnection_detail->lgnum, status)); 
			// TODO : Call Cache API to initialize Glbal Cache manager.
			break;
		}
	   break;
   case SFTK_IOCTL_TCP_REMOVE_CONNECTIONS:

	   ASSERT(pSystemBuffer != NULL);
	   DebugPrint((DBG_COM, "SftkTCPCDeviceIoControl : SFTK_IOCTL_TCP_REMOVE_CONNECTIONS: \n")); 
		pConnection_detail = (PCONNECTION_DETAILS) pSystemBuffer;

		// Call Veera's API like:
		status = COM_RemoveConnections( &pDeviceExtension->SessionManager, pConnection_detail );
		if (!NT_SUCCESS(status))
		{ // LG Already exist
			DebugPrint((DBG_ERROR, "SftkTCPCDeviceIoControl : SFTK_IOCTL_TCP_REMOVE_CONNECTIONS: COM_RemoveConnections(LgNum %d) Not Found !! returning status 0x%08x \n", 
							pConnection_detail->lgnum, status)); 
			// TODO : Call Cache API to initialize Glbal Cache manager.
			break;
		}
	   break;
   case SFTK_IOCTL_TCP_ENABLE_CONNECTIONS:

	   ASSERT(pSystemBuffer != NULL);
	   DebugPrint((DBG_COM, "SftkTCPCDeviceIoControl : SFTK_IOCTL_TCP_ENABLE_CONNECTIONS: \n")); 
		pConnection_detail = (PCONNECTION_DETAILS) pSystemBuffer;

		// Call Veera's API like:
		status = COM_EnableConnections( &pDeviceExtension->SessionManager, pConnection_detail ,TRUE);
		if (!NT_SUCCESS(status))
		{ // LG Already exist
			DebugPrint((DBG_ERROR, "SftkTCPCDeviceIoControl : SFTK_IOCTL_TCP_ENABLE_CONNECTIONS: COM_EnableConnections(LgNum %d) Not Found !! returning status 0x%08x \n", 
							pConnection_detail->lgnum, status)); 
			// TODO : Call Cache API to initialize Glbal Cache manager.
			break;
		}
	   break;
   case SFTK_IOCTL_TCP_DISABLE_CONNECTIONS:

	   ASSERT(pSystemBuffer != NULL);
	   DebugPrint((DBG_COM, "SftkTCPCDeviceIoControl : SFTK_IOCTL_TCP_DISABLE_CONNECTIONS: \n")); 
		pConnection_detail = (PCONNECTION_DETAILS) pSystemBuffer;

		// Call Veera's API like:
		status = COM_EnableConnections( &pDeviceExtension->SessionManager, pConnection_detail ,FALSE);
		if (!NT_SUCCESS(status))
		{ // LG Already exist
			DebugPrint((DBG_ERROR, "SftkTCPCDeviceIoControl : SFTK_IOCTL_TCP_DISABLE_CONNECTIONS: COM_EnableConnections(LgNum %d) Not Found !! returning status 0x%08x \n", 
							pConnection_detail->lgnum, status)); 
			// TODO : Call Cache API to initialize Glbal Cache manager.
			break;
		}
	   break;
   case SFTK_IOCTL_TCP_QUERY_SM_PERFORMANCE:
	   break;
   case SFTK_IOCTL_TCP_SET_CONNECTIONS_TUNABLES:
	   break;
   case SFTK_IOCTL_START_RMD:

	   ASSERT(pSystemBuffer != NULL);
	   pSMInitParams = (PSM_INIT_PARAMS)pSystemBuffer;

	   DebugPrint((DBG_COM, "SftkTCPCDeviceIoControl : SFTK_IOCTL_START_RMD: \n")); 

		status = COM_StartRMD( &pDeviceExtension->SessionManager, pSMInitParams);
		if (!NT_SUCCESS(status))
		{ // LG Already exist
			DebugPrint((DBG_ERROR, "sftk_ctl_ioctl : SFTK_IOCTL_START_RMD: COM_StartRMD(LgNum %d) Not Found !! returning status 0x%08x \n", 
							pSMInitParams->lgnum, status)); 
			// TODO : Call Cache API to initialize Glbal Cache manager.
			break;
		}
	   break;
   case SFTK_IOCTL_STOP_RMD:
	   DebugPrint((DBG_COM, "SftkTCPCDeviceIoControl : SFTK_IOCTL_STOP_RMD: \n")); 
	   COM_StopRMD( &pDeviceExtension->SessionManager);
	   break;
      default:
         KdPrint((  "FunctionCode: 0x%8.8X\n", nFunctionCode ));
         status = STATUS_UNSUCCESSFUL;
         break;
   }

   TdiCompleteRequest( pIrp, status );

   return( status );
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
   NTSTATUS          status;

   KdPrint(("TCPS_DeviceUnload: Entry...\n") );

   pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

   COM_UninitializeSessionManager(&pDeviceExtension->SessionManager);

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

      TDI_CreateSymbolicLink(
         &UnicodeDeviceName,
         FALSE
         );
  }
}
