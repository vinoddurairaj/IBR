/**************************************************************************************

Module Name: sftk_driver.C   
Author Name: Parag sanghvi
Description: Main Windows Driver file for the Replicator Driver component 
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/
// #define NTFOUR	// 

#include <sftk_main.h>

// Define Global Variable here
UNICODE_STRING	GDriverRegistryPath;
SFTK_CONFIG		GSftk_Config;

#if DBG
// ULONG SwrDebug = 0x00000001;
ULONG SwrDebug = DBG_ERROR | DBG_QUEUE | DBG_RTHREAD | DBG_PROTO | DBG_CONNECT | DBG_LISTEN | DBG_SEND | DBG_RECV | DBG_COM | DBG_TDI_QUERY | DBG_TDI_INIT | DBG_TDI_UNINIT;
#endif


//
// Function declarations
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

#ifdef NTFOUR
VOID
SwrNT_Initialize(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID          NextDisk,
    IN ULONG          Count
    );
#endif

VOID
SwrSyncFilterWithTarget(
    IN PDEVICE_OBJECT FilterDevice,
    IN PDEVICE_OBJECT TargetDevice
    );

NTSTATUS
SwrIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#ifndef NTFOUR
NTSTATUS
SwrAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );
NTSTATUS
SwrDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SwrStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SwrRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SwrDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#endif // #ifndef NTFOUR

NTSTATUS
SwrSendToNextDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SwrForwardIrpSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SwrCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SwrClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SwrReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
sftk_io_generic_completion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PSFTK_DEV			Sftk_Dev);

NTSTATUS
SwrDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
SwrFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SwrShutdown(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
SwrUnload(
    IN PDRIVER_OBJECT DriverObject
    );

VOID
SwrLogError(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG UniqueId,
    IN NTSTATUS ErrorCode,
    IN NTSTATUS Status
    );
//
// Define the sections that allow for discarding (i.e. paging) some of
// the code.
//
#if 0
#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, SwrCreate)
#pragma alloc_text (PAGE, SwrClose)
#ifndef NTFOUR
#pragma alloc_text (PAGE, SwrAddDevice)
#pragma alloc_text (PAGE, SwrDispatchPnp)
#pragma alloc_text (PAGE, SwrStartDevice)
#pragma alloc_text (PAGE, SwrRemoveDevice)
#endif
#pragma alloc_text (PAGE, SwrUnload)
#pragma alloc_text (PAGE, SwrSyncFilterWithTarget)
#endif
#endif // #if 0

#ifndef NTFOUR
#ifdef WMI_IMPLEMENTED

// This is WMI guid used to return disk performance information from
// diskperf.sys (see DISK_PERFORMANCE data structure)

//DEFINE_GUID (SwrGuid, 0xBDD865D1,0xD7C1,0x11d0,0xA5,0x01,0x00,0xA0,0xC9,0x06,0x29,0x10);
const GUID SwrGuid={0xBDD865D1,0xD7C1,0x11d0,0xA5,0x01,0x00,0xA0,0xC9,0x06,0x29,0x10};

WMIGUIDREGINFO SwrGuidList[] =
{
    { &SwrGuid,
      1,
      0
    }
};

#define SwrGuidCount (sizeof(SwrGuidList) / sizeof(WMIGUIDREGINFO))
#endif
#endif // #ifndef NTFOUR


// Disable this DriverEntry because there is one more Driver that is being built 
// that will be used for unittesting the TDI Componenet VEERA

#ifndef _TDIDRIVER

/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O manager to set up the disk
    performance driver. The driver object is set up and then the Pnp manager
    calls SwrAddDevice to attach to the boot devices.

Arguments:

    DriverObject - The disk performance driver object.

    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful

--*/
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{

	NTSTATUS			status = STATUS_SUCCESS;
    ULONG               ulIndex;
    PDRIVER_DISPATCH  * dispatch;

	DebugPrint((DBG_ERROR, "Sftk_Repl: DriverEntry:: got entered. !!! \n"));
    // Allocate and Initialize global variables, create and initialize Control device, 
	// and do driver private initialization
	status = sftk_init_driver( DriverObject, RegistryPath);
	if (status != STATUS_SUCCESS)	
	{ // Failed, return error, API has already done system event log message
		DebugPrint((DBG_ERROR, "DriverEntry:: sftk_init_driver() Failed !!! returning error 0x%08x \n", status));
		goto done;
	}
	
    //
    // Create dispatch points
    //
    for (ulIndex = 0, dispatch = DriverObject->MajorFunction;
         ulIndex <= IRP_MJ_MAXIMUM_FUNCTION;
         ulIndex++, dispatch++) 
	{
        *dispatch = SwrSendToNextDriver;
    }

    //
    // Set up the device driver entry points.
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE]          = SwrCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]			= SwrClose;
    DriverObject->MajorFunction[IRP_MJ_READ]            = SwrReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE]           = SwrReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = SwrDeviceControl;

#ifndef NTFOUR
	DriverObject->MajorFunction[IRP_MJ_PNP]             = SwrDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]           = SwrDispatchPower;

    DriverObject->DriverExtension->AddDevice            = SwrAddDevice;
#ifdef WMI_IMPLEMENTED
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]  = SwrWmi;
#endif
#endif // #ifndef NTFOUR

    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]        = SwrShutdown;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]   = SwrFlush;
    DriverObject->DriverUnload                          = SwrUnload;

#ifdef NTFOUR
	SwrNT_Initialize(DriverObject, 0, 0);
#endif

	DebugPrint((DBG_DISPATCH, "DriverEntry:: sftk_init_driver() Started Successfully!!! returning status 0x%08x \n", status));
	sftk_LogEvent(DriverObject, MSG_REPL_DRIVER_START, STATUS_SUCCESS, 0);

done:
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "Sftk_Repl: DriverEntry:: Driver Failed to Load. !!! returning Error status 0x%08x \n", status));
		sftk_LogEvent(DriverObject, MSG_REPL_DRIVER_LOAD_FAILED, status, 0);
	}
	DebugPrint((DBG_ERROR, "Sftk_Repl: DriverEntry:: Returning status 0x%08x . !!! \n", status));
    return status;
} // end DriverEntry()

#endif //_TDIDRIVER

// Disable this DriverEntry because there is one more Driver that is being built 
// that will be used for unittesting the TDI Componenet VEERA


#ifdef NTFOUR
VOID
SwrNT_Initialize(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID          NextDisk,
    IN ULONG          Count
    )
/*++
Routine Description:
    Attach to new disk devices and partitions.
    Set up device objects for counts and times.
    If this is the first time this routine is called,
    then register with the IO system to be called
    after all other disk device drivers have initiated.
Arguments:
    DriverObject - Disk performance driver object.
    NextDisk - Starting disk for this part of the initialization.
    Count - Not used. Number of times this routine has been called.
Return Value:
    NTSTATUS
--*/
{
    NTSTATUS					status;
	PCONFIGURATION_INFORMATION	configurationInformation;
    WCHAR						ntNameBuffer[80];
    UNICODE_STRING				ntUnicodeString;
    PDEVICE_OBJECT				deviceObject;
    PDEVICE_OBJECT				filterDeviceObject, filterDeviceObject1;
    PDEVICE_EXTENSION			deviceExtension, devExt;
    PFILE_OBJECT				fileObject;
    ULONG						diskNumber;
    ULONG						partNumber;
	IO_STATUS_BLOCK				ioStatus;
    KEVENT						event;
    PIRP						irp;
	PDRIVE_LAYOUT_INFORMATION	pDriveLayOut= NULL;
	
	// Allocate memory for Partition information
	DebugPrint((DBG_ERROR, "SwrNT_Initialize() Input : DiskCount %d Number Of time API has been Called %d \n",NextDisk,Count)); 

	pDriveLayOut = OS_AllocMemory(NonPagedPool, 8192);
	if(pDriveLayOut == NULL)
	{
		DebugPrint((DBG_ERROR, "SwrNT_Initialize() failed to allocate memory for PDRIVE_LAYOUT_INFORMATION size %d \n",8192)); 
		return;
	}
	OS_ZeroMemory( pDriveLayOut, 8192);

	// Get the configuration information.
    configurationInformation = IoGetConfigurationInformation();

    // Find disk devices.
    for (diskNumber = (ULONG)NextDisk;
         diskNumber < configurationInformation->DiskCount;
         diskNumber++) 
	{ // for : Scan thru each and every disks

        // Create device name for the physical disk.
		swprintf(	ntNameBuffer,	
					DEVICE_HARDDISK_PARTITION,	// L"\\Device\\Harddisk%d\\Partition%d"
					diskNumber, 0);
        
		RtlInitUnicodeString( &ntUnicodeString, ntNameBuffer);

        // Create device object for partition 0.
		status = IoCreateDevice( DriverObject,
                                 DEVICE_EXTENSION_SIZE,
                                 NULL,
                                 FILE_DEVICE_DISK,
                                 0,
                                 FALSE,
                                 &filterDeviceObject);

		if (!NT_SUCCESS(status)) 
		{
			DebugPrint((DBG_ERROR, "SwrNT_Initialize:: IoCreateDevice(%S) Failed with error 0x%08x \n", 
												ntNameBuffer, status));
			// TODO : Log Event Message
			continue; // ignore this error and continue for next disk
		}

        filterDeviceObject->Flags |= DO_DIRECT_IO;

		deviceExtension = (PDEVICE_EXTENSION) filterDeviceObject->DeviceExtension;

		OS_ZeroMemory(deviceExtension, DEVICE_EXTENSION_SIZE);

		// Initialize deviceExtension
		deviceExtension->NodeId.NodeType	= NODE_TYPE_FILTER_DEV;
		deviceExtension->NodeId.NodeSize	= sizeof(DEVICE_EXTENSION);

		deviceExtension->DeviceObject = filterDeviceObject;

		// Attaches the device object to the highest device object in the chain and
		// return the previously highest device object, which is passed to
		// IoCallDriver when pass IRPs down the device stack
        status = IoAttachDevice( filterDeviceObject,
                                 &ntUnicodeString,
                                 &deviceExtension->TargetDeviceObject);
		if (!NT_SUCCESS(status)) 
		{
			DebugPrint((DBG_ERROR, "SwrNT_Initialize:: IoAttachDevice(%S) Failed with error 0x%08x \n", 
												ntNameBuffer, status));
			// TODO : Log Event Message
			IoDeleteDevice(filterDeviceObject);
			continue;
		}

		deviceExtension->PhysicalDeviceObject = deviceExtension->TargetDeviceObject;
		
		deviceExtension->DeviceInfo.PhysicalDeviceName.Buffer
				= deviceExtension->DeviceInfo.PhysicalDeviceNameBuffer;

        // Propogate driver's alignment requirements.
        filterDeviceObject->AlignmentRequirement =
        		deviceExtension->TargetDeviceObject->AlignmentRequirement;

		// Call Interanl Routines to initialize this device.
		SwrSyncFilterWithTarget(filterDeviceObject, deviceExtension->TargetDeviceObject);
		AddDevExt_SftkConfigList(deviceExtension);
		sftk_reattach_devExt_to_SftkDev();

		// Get Parition Information which returns Partition Counts.
		// deviceExtension->DeviceInfo.pRawDiskDevice = deviceExtension->TargetDeviceObject;;	// store this for later use if needed.

		// - Retrieve Signature number from RAW Disk Device
		// - Retrieve Partition starting Offset and Size in bytes.
		OS_ZeroMemory( pDriveLayOut, 8192);

		OS_InitializeEvent(&event, NotificationEvent, FALSE);
		irp = IoBuildDeviceIoControlRequest(	IOCTL_DISK_GET_DRIVE_LAYOUT,
												deviceExtension->TargetDeviceObject, 
												NULL, 0,
												pDriveLayOut, 8192, 
												FALSE, &event, &ioStatus);
		if (!irp) 
		{
			DebugPrint((DBG_ERROR, "SwrNT_Initialize() failed to allocate IRP for IOCTL_DISK_GET_DRIVE_LAYOUT\n")); 
			// TODO : Log Event here
			continue;
		}

		status = IoCallDriver( deviceExtension->TargetDeviceObject, irp );
		if (status == STATUS_PENDING) 
		{
		   OS_KeWaitForSingleObject(&event,Executive,KernelMode,FALSE,NULL);
		   status = ioStatus.Status;
		}
		if (!NT_SUCCESS(status))
		{ // failed
			DebugPrint((DBG_ERROR, "SwrNT_Initialize() IoCallDriver( %S Device 0x%08x ) for IOCTL_DISK_GET_DRIVE_LAYOUT failed. error 0x%08x\n", 
								ntNameBuffer, deviceExtension->TargetDeviceObject, status)); 
			// TODO : Log Event here
			continue;
		}

		// Now Scan thru each and every partition and create device for it
		// pDriveLayOut->PartitionCount;	// store signature
		partNumber = 0;
		while(TRUE)
		{
            partNumber++;

			// Create device name for the physical disk.
			swprintf(	ntNameBuffer,	
						DEVICE_HARDDISK_PARTITION,	// L"\\Device\\Harddisk%d\\Partition%d"
						diskNumber, partNumber);
        
			RtlInitUnicodeString( &ntUnicodeString, ntNameBuffer);

            // Get target device object.
            status = IoGetDeviceObjectPointer( &ntUnicodeString,
                                               FILE_READ_ATTRIBUTES,
                                               &fileObject,
                                               &deviceObject);

            if (!NT_SUCCESS(status)) 
			{ // Failed
				DebugPrint((DBG_ERROR, "SwrNT_Initialize() IoGetDeviceObjectPointer(%S) failed. error 0x%08x\n", ntNameBuffer, status)); 

				if(partNumber <= pDriveLayOut->PartitionCount)
					continue;
				else
					break;
            }
			ObDereferenceObject(fileObject);

            // Check if this device is already mounted.
            if (!deviceObject->Vpb ||
                (deviceObject->Vpb->Flags & VPB_MOUNTED) ) 
			{ // Device is already mounted ....
                // Can't attach to a device that is already mounted.
                continue;
            }

			 // Create device object for partition 0.
			status = IoCreateDevice( DriverObject,
									 DEVICE_EXTENSION_SIZE,
									 NULL,
									 FILE_DEVICE_DISK,
									 0,
									 FALSE,
									 &filterDeviceObject1);

			if (!NT_SUCCESS(status)) 
			{
				DebugPrint((DBG_ERROR, "SwrNT_Initialize:: Part: IoCreateDevice(%S) Failed with error 0x%08x \n", 
													ntNameBuffer, status));
				// TODO : Log Event Message
				continue; // ignore this error and continue for next disk
			}

			filterDeviceObject1->Flags |= DO_DIRECT_IO;

			devExt = (PDEVICE_EXTENSION) filterDeviceObject1->DeviceExtension;

			OS_ZeroMemory(devExt, DEVICE_EXTENSION_SIZE);

			// Initialize devExt
			devExt->NodeId.NodeType	= NODE_TYPE_FILTER_DEV;
			devExt->NodeId.NodeSize	= sizeof(DEVICE_EXTENSION);

			devExt->DeviceObject = filterDeviceObject1;

			// Attaches the device object to the highest device object in the chain and
			// return the previously highest device object, which is passed to
			// IoCallDriver when pass IRPs down the device stack
			status = IoAttachDevice( deviceObject,
									 &ntUnicodeString,
									 &devExt->TargetDeviceObject);
			if (!NT_SUCCESS(status)) 
			{
				DebugPrint((DBG_ERROR, "SwrNT_Initialize:: Part: IoAttachDevice(%S) Failed with error 0x%08x \n", 
													ntNameBuffer, status));
				// TODO : Log Event Message
				IoDeleteDevice(filterDeviceObject1);
				continue;
			}

			devExt->PhysicalDeviceObject = devExt->TargetDeviceObject;
			
			devExt->DeviceInfo.PhysicalDeviceName.Buffer
					= devExt->DeviceInfo.PhysicalDeviceNameBuffer;

			// Propogate driver's alignment requirements.
			filterDeviceObject1->AlignmentRequirement =
        			devExt->TargetDeviceObject->AlignmentRequirement;

			// Call Interanl Routines to initialize this device.
			SwrSyncFilterWithTarget(filterDeviceObject1, devExt->TargetDeviceObject);
			AddDevExt_SftkConfigList(devExt);
			sftk_reattach_devExt_to_SftkDev();
		} // while(TRUE)
	} // // for : Scan thru each and every disks

	if (pDriveLayOut)
		OS_FreeMemory(pDriveLayOut);

	// Check if this is the first time this routine has been called.
    if (!NextDisk) 
	{
        // Register with IO system to be called a second time after all
        // other device drivers have initialized.
        IoRegisterDriverReinitialization(DriverObject,
                                         SwrNT_Initialize,
                                         (PVOID)configurationInformation->DiskCount);
    }

	return;
} // SwrNT_Initialize()

#endif // #ifdef NTFOUR

VOID
SwrSyncFilterWithTarget(
    IN PDEVICE_OBJECT FilterDevice,
    IN PDEVICE_OBJECT TargetDevice
    )
{
    ULONG                   propFlags;

    PAGED_CODE();

    // Propogate all useful flags from target to Swr. MountMgr will look
    // at the Swr object capabilities to figure out if the disk is
    // a removable and perhaps other things.

    propFlags = TargetDevice->Flags & FILTER_DEVICE_PROPOGATE_FLAGS;
    FilterDevice->Flags |= propFlags;

    propFlags = TargetDevice->Characteristics & FILTER_DEVICE_PROPOGATE_CHARACTERISTICS;
    FilterDevice->Characteristics |= propFlags;

} // SwrSyncFilterWithTarget

NTSTATUS
SwrIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Forwarded IRP completion routine. Set an event and return
    STATUS_MORE_PROCESSING_REQUIRED. Irp forwarder will wait on this
    event and then re-complete the irp after cleaning up.

Arguments:

    DeviceObject is the device object of the WMI driver
    Irp is the WMI irp that was just completed
    Context is a PKEVENT that forwarder will wait on

Return Value:

    STATUS_MORE_PORCESSING_REQUIRED

--*/

{
    PKEVENT Event = (PKEVENT) Context;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);

    return(STATUS_MORE_PROCESSING_REQUIRED);

} // end SwrIrpCompletion()


#ifndef NTFOUR
NTSTATUS
SwrAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++
Routine Description:

    Creates and initializes a new filter device object FiDO for the
    corresponding PDO.  Then it attaches the device object to the device
    stack of the drivers for the device.

Arguments:

    DriverObject - Disk performance driver object.
    PhysicalDeviceObject - Physical Device Object from the underlying layered driver

Return Value:

    NTSTATUS
--*/

{
    NTSTATUS                status;
    IO_STATUS_BLOCK         ioStatus;
    PDEVICE_OBJECT          filterDeviceObject;
    PDEVICE_EXTENSION       deviceExtension;
    PIRP                    irp;
    STORAGE_DEVICE_NUMBER   number;
    PWMILIB_CONTEXT         wmilibContext;
    PCHAR                   buffer;
    ULONG                   buffersize;

    PAGED_CODE();

    // Create a filter device object for this device (partition).
    DebugPrint( ( DBG_DISPATCH, "SwrAddDevice: Driver %X Device %X\n",DriverObject, PhysicalDeviceObject));

    status = IoCreateDevice(DriverObject,
                            DEVICE_EXTENSION_SIZE,
                            NULL,
                            FILE_DEVICE_DISK,
                            0, // FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &filterDeviceObject);

    if (!NT_SUCCESS(status)) {
       DebugPrint((DBG_ERROR, "SwrAddDevice: IoCreateDevice() Failed 0x%08x, Cannot create filterDeviceObject\n",status));
       return status;
    }

    filterDeviceObject->Flags |= DO_DIRECT_IO;

    deviceExtension = (PDEVICE_EXTENSION) filterDeviceObject->DeviceExtension;

    OS_ZeroMemory(deviceExtension, DEVICE_EXTENSION_SIZE);

	// Initialize deviceExtension
	deviceExtension->NodeId.NodeType	= NODE_TYPE_FILTER_DEV;
	deviceExtension->NodeId.NodeSize	= sizeof(DEVICE_EXTENSION);

    // Attaches the device object to the highest device object in the chain and
    // return the previously highest device object, which is passed to
    // IoCallDriver when pass IRPs down the device stack
    deviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;

    deviceExtension->TargetDeviceObject =
        IoAttachDeviceToDeviceStack(filterDeviceObject, PhysicalDeviceObject);

    if (deviceExtension->TargetDeviceObject == NULL) {
        IoDeleteDevice(filterDeviceObject);
        DebugPrint( ( DBG_ERROR, "SwrAddDevice: Unable to attach %X to target %X\n",
            filterDeviceObject, PhysicalDeviceObject));
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Save the filter device object in the device extension
    //
    deviceExtension->DeviceObject = filterDeviceObject;

    deviceExtension->DeviceInfo.PhysicalDeviceName.Buffer
            = deviceExtension->DeviceInfo.PhysicalDeviceNameBuffer;

    OS_InitializeEvent(&deviceExtension->PagingPathCountEvent,
                      NotificationEvent, TRUE);


#ifdef WMI_IMPLEMENTED	
    //
    // Initialize WMI library context
    //
    wmilibContext = &deviceExtension->WmilibContext;
    OS_ZeroMemory(wmilibContext, sizeof(WMILIB_CONTEXT));
    wmilibContext->GuidCount = SwrGuidCount;
    wmilibContext->GuidList = SwrGuidList;
    wmilibContext->QueryWmiRegInfo = SwrQueryWmiRegInfo;
    wmilibContext->QueryWmiDataBlock = SwrQueryWmiDataBlock;
    wmilibContext->WmiFunctionControl = SwrWmiFunctionControl;
#endif
    //
    // default to DO_POWER_PAGABLE
    //

    filterDeviceObject->Flags |=  DO_POWER_PAGABLE;

    //
    // Clear the DO_DEVICE_INITIALIZING flag
    //

    filterDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;

} // end SwrAddDevice()


NTSTATUS
SwrDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Dispatch for PNP

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status;
    PDEVICE_EXTENSION	deviceExtension	= DeviceObject->DeviceExtension;

	PAGED_CODE();

	DebugPrint( ( DBG_DISPATCH, "SwrDispatchPnp: Device %X Irp %X\n",DeviceObject, Irp));

	switch(deviceExtension->NodeId.NodeType)
	{
		case NODE_TYPE_SFTK_CTL:	// FTD_NODE_TYPE_EXT_DEVICE
		case NODE_TYPE_SFTK_LG: // FTD_NODE_TYPE_LG_DEVICE 

			status			= STATUS_INVALID_DEVICE_REQUEST;
			DebugPrint( ( DBG_ERROR, "SwrDispatchPnp: %s : Device %X Irp %X, Failing with status 0x%08x \n",
							(deviceExtension->NodeId.NodeType == NODE_TYPE_SFTK_CTL)?"NODE_TYPE_SFTK_CTL":"NODE_TYPE_SFTK_LG",
							DeviceObject, Irp, status));
			
			Irp->IoStatus.Status		= status;
            Irp->IoStatus.Information	= 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
			return(status);
			break;
	}

    switch(irpSp->MinorFunction) {

        case IRP_MN_START_DEVICE:
            
			// Call the Start Routine handler to schedule a completion routine
            DebugPrint((DBG_DISPATCH,"SwrDispatchPnp: Schedule completion for START_DEVICE"));
            status = SwrStartDevice(DeviceObject, Irp);
            break;

        case IRP_MN_REMOVE_DEVICE:
        {
            // Call the Remove Routine handler to schedule a completion routine
            DebugPrint((DBG_DISPATCH,"SwrDispatchPnp: Schedule completion for REMOVE_DEVICE"));
            status = SwrRemoveDevice(DeviceObject, Irp);
            break;
        }
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
        {
            PIO_STACK_LOCATION irpStack;
            ULONG count;
            BOOLEAN setPagable;

            DebugPrint((DBG_DISPATCH, "SwrDispatchPnp: Processing DEVICE_USAGE_NOTIFICATION"));

            irpStack = IoGetCurrentIrpStackLocation(Irp);

            if (irpStack->Parameters.UsageNotification.Type != DeviceUsageTypePaging) {
                status = SwrSendToNextDriver(DeviceObject, Irp);
                break; // out of case statement
            }

            deviceExtension = DeviceObject->DeviceExtension;

            // wait on the paging path event
            status = OS_KeWaitForSingleObject(&deviceExtension->PagingPathCountEvent,
                                           Executive, KernelMode,
                                           FALSE, NULL);

            // if removing last paging device, need to set DO_POWER_PAGABLE
            // bit here, and possible re-set it below on failure.
            setPagable = FALSE;
            if (!irpStack->Parameters.UsageNotification.InPath &&
                deviceExtension->PagingPathCount == 1 ) {

                // removing the last paging file
                // must have DO_POWER_PAGABLE bits set
                if (DeviceObject->Flags & DO_POWER_INRUSH) {
                    DebugPrint((DBG_DISPATCH, "SwrDispatchPnp: last paging file "
										"removed but DO_POWER_INRUSH set, so not "
										"setting PAGABLE bit "
										"for DO %p\n", DeviceObject));
                } else {
                    DebugPrint( ( DBG_DISPATCH, "SwrDispatchPnp: Setting  PAGABLE "
                                "bit for DO %p\n", DeviceObject));
                    DeviceObject->Flags |= DO_POWER_PAGABLE;
                    setPagable = TRUE;
                }

            }

            // send the irp synchronously
            status = SwrForwardIrpSynchronous(DeviceObject, Irp);

            // now deal with the failure and success cases.
            // note that we are not allowed to fail the irp
            // once it is sent to the lower drivers.
            if (NT_SUCCESS(status)) {

                IoAdjustPagingPathCount(
                    &deviceExtension->PagingPathCount,
                    irpStack->Parameters.UsageNotification.InPath);

                if (irpStack->Parameters.UsageNotification.InPath) {
                    if (deviceExtension->PagingPathCount == 1) {

                        // first paging file addition
                        DebugPrint((DBG_DISPATCH, "SwrDispatchPnp: Clearing PAGABLE bit "
                                    "for DO %p\n", DeviceObject));
                        DeviceObject->Flags &= ~DO_POWER_PAGABLE;
                    }
                }

            } else {

                // cleanup the changes done above
                if (setPagable == TRUE) {
                    DeviceObject->Flags &= ~DO_POWER_PAGABLE;
                    setPagable = FALSE;
                }

            }

            // set the event so the next one can occur.
            KeSetEvent(&deviceExtension->PagingPathCountEvent,
                       IO_NO_INCREMENT, FALSE);

            // and complete the irp
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;
            break;

        }

        default:
            DebugPrint((DBG_DISPATCH,"SwrDispatchPnp: default: Forwarding irp"));

            // Simply forward all other Irps
            return SwrSendToNextDriver(DeviceObject, Irp);

    }

    return status;

} // end SwrDispatchPnp()


NTSTATUS
SwrStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called when a Pnp Start Irp is received.
    It will schedule a completion routine to initialize and register with WMI.

Arguments:

    DeviceObject - a pointer to the device object

    Irp - a pointer to the irp


Return Value:

    Status of processing the Start Irp

--*/
{
    PDEVICE_EXTENSION   deviceExtension;
    KEVENT              event;
    NTSTATUS            status;

    PAGED_CODE();

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    status = SwrForwardIrpSynchronous(DeviceObject, Irp);

    SwrSyncFilterWithTarget(DeviceObject, deviceExtension->TargetDeviceObject);

    // Complete WMI registration
    AddDevExt_SftkConfigList(deviceExtension);

	sftk_reattach_devExt_to_SftkDev();

#ifdef WMI_IMPLEMENTED
	{
		ULONG registrationFlag = 0;
		status = IoWMIRegistrationControl(	DeviceObject,
											WMIREG_ACTION_REGISTER | registrationFlag );
		if (! NT_SUCCESS(status)) 
		{
			WCHAR	wStr[64];
			swprintf(wStr, L"0x%08x",DeviceObject);

			DebugPrint( ( DBG_ERROR, "SwrStartDevice: IoWMIRegistrationControl() Failed with Status 0x%08x DeviceObject %X Irp %X\n",
							status, DeviceObject, Irp));
			sftk_LogEventString1(GSftk_Config.DriverObject, MSG_REPL_WMI_REGISTER_FAILED, status, 0, wStr);
			// SwrLogError(	DeviceObject,261,STATUS_SUCCESS,IO_ERR_INTERNAL_ERROR);
		}
	}
#endif

    // Complete the Irp
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
} // SwrStartDevice()


NTSTATUS
SwrRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called when the device is to be removed.
    It will de-register itself from WMI first, detach itself from the
    stack before deleting itself.

Arguments:

    DeviceObject - a pointer to the device object

    Irp - a pointer to the irp


Return Value:

    Status of removing the device

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;
    PWMILIB_CONTEXT     wmilibContext;

    PAGED_CODE();

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

#ifdef WMI_IMPLEMENTED	
    // Remove registration with WMI first
    IoWMIRegistrationControl(DeviceObject, WMIREG_ACTION_DEREGISTER);

    // quickly zero out the count first to invalid the structure
    wmilibContext = &deviceExtension->WmilibContext;
    InterlockedExchange(
        (PLONG) &(wmilibContext->GuidCount),
        (LONG) 0);

    OS_ZeroMemory(wmilibContext, sizeof(WMILIB_CONTEXT));
#endif // #ifdef WMI_IMPLEMENTED	

    status = SwrForwardIrpSynchronous(DeviceObject, Irp);
	if (!NT_SUCCESS(status))
	{
		DebugPrint( ( DBG_ERROR, "SwrRemoveDevice: SwrForwardIrpSynchronous() Failed with Status 0x%08x DeviceObject %X Irp %X\n",
							status, DeviceObject, Irp));
	}

	// Check if Device was existed & configured with SFTK_DEV if yes than 
	// remove it from there too.
	sftk_remove_devExt_from_SftkDev(deviceExtension);

	RemoveDevExt_SftkConfigList(deviceExtension);
	
    IoDetachDevice(deviceExtension->TargetDeviceObject);
    IoDeleteDevice(DeviceObject);

    // Complete the Irp
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
} // SwrRemoveDevice()

NTSTATUS
SwrDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
	PDEVICE_EXTENSION	deviceExtension	= DeviceObject->DeviceExtension;
    NTSTATUS			status;

	switch(deviceExtension->NodeId.NodeType)
	{
		case NODE_TYPE_SFTK_CTL:	// FTD_NODE_TYPE_EXT_DEVICE
		case NODE_TYPE_SFTK_LG: // FTD_NODE_TYPE_LG_DEVICE 

			status = STATUS_INVALID_DEVICE_REQUEST;
			DebugPrint( ( DBG_ERROR, "SwrDispatchPower: %s : Device %X Irp %X, Failing with status 0x%08x \n",
							(deviceExtension->NodeId.NodeType == NODE_TYPE_SFTK_CTL)?"NODE_TYPE_SFTK_CTL":"NODE_TYPE_SFTK_LG",
							DeviceObject, Irp, status));

			Irp->IoStatus.Status		= status;
            Irp->IoStatus.Information	= 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
			return(status);
			break;
	}

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);

    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    return PoCallDriver(deviceExtension->TargetDeviceObject, Irp);

} // end SwrDispatchPower

#endif //  #ifndef NTFOUR


NTSTATUS
SwrSendToNextDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
Routine Description:
    This routine sends the Irp to the next driver in line
    when the Irp is not processed by this driver.

Arguments:
    DeviceObject
    Irp

Return Value:
    NTSTATUS
--*/
{
    PDEVICE_EXTENSION	deviceExtension	= DeviceObject->DeviceExtension;
	NTSTATUS			status;

	switch(deviceExtension->NodeId.NodeType)
	{
		case NODE_TYPE_SFTK_CTL:	// FTD_NODE_TYPE_EXT_DEVICE
		case NODE_TYPE_SFTK_LG: // FTD_NODE_TYPE_LG_DEVICE 

			status			= STATUS_INVALID_DEVICE_REQUEST;
			DebugPrint( ( DBG_ERROR, "SwrSendToNextDriver: %s : Device %X Irp %X, Failing with status 0x%08x \n",
							(deviceExtension->NodeId.NodeType == NODE_TYPE_SFTK_CTL)?"NODE_TYPE_SFTK_CTL":"NODE_TYPE_SFTK_LG",
							DeviceObject, Irp, status));

			Irp->IoStatus.Status		= status;
            Irp->IoStatus.Information	= 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
			return(status);

			break;
	}

	// Disk Devices
    IoSkipCurrentIrpStackLocation(Irp);
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);

} // end SwrSendToNextDriver()


NTSTATUS
SwrForwardIrpSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
Routine Description:
    This routine sends the Irp to the next driver in line
    when the Irp needs to be processed by the lower drivers
    prior to being processed by this one.

Arguments:
    DeviceObject
    Irp

Return Value:
    NTSTATUS
--*/
{
    PDEVICE_EXTENSION   deviceExtension;
    KEVENT event;
    NTSTATUS status;

    OS_InitializeEvent(&event, NotificationEvent, FALSE);
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // copy the irpstack for the next device
    IoCopyCurrentIrpStackLocationToNext(Irp);

    // set a completion routine
    IoSetCompletionRoutine(Irp, SwrIrpCompletion,
                            &event, TRUE, TRUE, TRUE);

    // call the next lower device
    status = IoCallDriver(deviceExtension->TargetDeviceObject, Irp);

    // wait for the actual completion
    if (status == STATUS_PENDING) 
	{
        OS_KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = Irp->IoStatus.Status;
    }

    return status;
} // end SwrForwardIrpSynchronous()


NTSTATUS
SwrCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
Routine Description:
    This routine services open commands. It establishes
    the driver's existance by returning status success.
Arguments:
    DeviceObject - Context for the activity.
    Irp          - The device control argument block.
Return Value:
    NT Status
--*/
{
	NTSTATUS			status			= STATUS_SUCCESS;
    BOOLEAN             CompleteIrp		= TRUE;
	PDEVICE_EXTENSION	deviceExtension	= DeviceObject->DeviceExtension;
    try 
    {
        try 
        {
			switch(deviceExtension->NodeId.NodeType)
			{
				case NODE_TYPE_SFTK_CTL:	// FTD_NODE_TYPE_EXT_DEVICE
					break;	// nothing to do, just complete IRP

				case NODE_TYPE_SFTK_LG: // FTD_NODE_TYPE_LG_DEVICE 
					
					try_return (status = sftk_lg_open( DeviceObject->DeviceExtension) );
					break;

				default: 
					
					OS_ASSERT(deviceExtension->TargetDeviceObject != NULL);


					// Set current stack back one.
					DebugPrint( ( DBG_DISPATCH, "SwrCreate: default: sending down: DeviceObject %X Irp %X, default:Device NodeType 0x%08x \n",
									DeviceObject, Irp, deviceExtension->NodeId.NodeType));
					break;

					// Set current stack back one. and Pass request down to next driver layer.
					// IoSkipCurrentIrpStackLocation(Irp);
					// return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);
            } // switch(deviceExtension->NodeId.NodeType)
        } 
        except (sftk_ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
        {
			// Log event message here
			sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
									0, __FILE__, __LINE__, GetExceptionCode());

            // We encountered an exception somewhere, eat it up.
            DebugPrint( ( DBG_ERROR,  "SwrCreate() : EXCEPTION_EXECUTE_HANDLER, status = %d.", GetExceptionCode()));
        }
        try_exit:   NOTHING;
    } 
    finally 
    {
        if (CompleteIrp == TRUE) 
        {
            Irp->IoStatus.Status		= status;
            Irp->IoStatus.Information	= 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
    }

    return(status);
} // end SwrCreate()

NTSTATUS
SwrClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
Routine Description:
    This routine services Close commands. It Closes and derefence 
    opened driver's object and return status success.
Arguments:
    DeviceObject - Context for the activity.
    Irp          - The device control argument block.
Return Value:
    NT Status
--*/
{
	NTSTATUS			status			= STATUS_SUCCESS;
    BOOLEAN             CompleteIrp		= TRUE;
	PDEVICE_EXTENSION	deviceExtension	= DeviceObject->DeviceExtension;

    try 
    {
        try 
        {
			switch(deviceExtension->NodeId.NodeType)
			{
				case NODE_TYPE_SFTK_CTL:	// FTD_NODE_TYPE_EXT_DEVICE
					break;	// nothing to do, just complete IRP

				case NODE_TYPE_SFTK_LG: // FTD_NODE_TYPE_LG_DEVICE 
					
					try_return (status = sftk_lg_close( DeviceObject->DeviceExtension) );
					break;

				default: 
					// NODE_TYPE_FILTER_DEV

					OS_ASSERT(deviceExtension->TargetDeviceObject != NULL);
					DebugPrint( ( DBG_DISPATCH, "SwrClose: Passing Down, DeviceObject %X Irp %X, default:Device NodeType 0x%08x \n",
									DeviceObject, Irp, deviceExtension->NodeId.NodeType));

					break;
					// Set current stack back one. and Pass request down to next driver layer.
					// IoSkipCurrentIrpStackLocation(Irp);
					// return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);					
            } // switch(deviceExtension->NodeId.NodeType)
        } 
        except (sftk_ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
        {
			// Log event message here
			sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
									0, __FILE__, __LINE__, GetExceptionCode());

            // We encountered an exception somewhere, eat it up.
            DebugPrint( ( DBG_ERROR,  "SwrClose() : EXCEPTION_EXECUTE_HANDLER, status = %d.", GetExceptionCode()));
        }
        try_exit:   NOTHING;

    } 
    finally 
    {
        if (CompleteIrp == TRUE) 
        {
            Irp->IoStatus.Status		= status;
            Irp->IoStatus.Information	= 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
    }

    return(status);
} // end SwrClose()

NTSTATUS
SwrReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
Routine Description:
    This is the driver entry point for read and write requests
    to disks to which the Swr driver has attached.
    This driver collects statistics and then sets a completion
    routine so that it can collect additional information when
    the request completes. Then it calls the next driver below
    it.
Arguments:
    DeviceObject
    Irp
Return Value:
    NTSTATUS
--*/
{
    PDEVICE_EXTENSION	deviceExtension	= DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION	currentIrpStack	= IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION	nextIrpStack	= IoGetNextIrpStackLocation(Irp);
    PIRP_CONTEXT		pIrpContext		= NULL;
	PSFTK_DEV			pSftk_Dev		= NULL;
	LONG				currentState;
	NTSTATUS			status;	
    

	if (deviceExtension->NodeId.NodeType != NODE_TYPE_FILTER_DEV )
	{ // invalid deviue IO Request
		status = STATUS_INVALID_DEVICE_REQUEST;
		
		// OS_ASSERT(FALSE);
		DebugPrint( ( DBG_ERROR, "SwrReadWrite: IO to Non-Filter Disk Device !!! DeviceObject %X Irp %X, Ops: %s, Returning error 0x%08x \n",
			DeviceObject, Irp, (currentIrpStack->MajorFunction == IRP_MJ_READ)?"IRP_MJ_READ":"IRP_MJ_WRITE",status ));

		Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return status;
	}  // invalid deviue IO Request

	// Disk Filter Device IO, Process it
	switch(currentIrpStack->MajorFunction)
	{
		case IRP_MJ_READ: 

					DebugPrint( ( DBG_RDWR, "SwrReadWrite: READ: DeviceObject %X Irp %X,  Offset %I64d, Size %d \n",
									DeviceObject, Irp, 
									currentIrpStack->Parameters.Read.ByteOffset.QuadPart,
									currentIrpStack->Parameters.Read.Length));
#if 0
					// in Back refresh do we need to fail read, its safe to do this causes we support
					// offline src disk back refresh 
					if (	(deviceExtension->Sftk_dev) && 
							(deviceExtension->Sftk_dev->Sftk_Lg)  &&
							(sftk_lg_get_state(deviceExtension->Sftk_dev->Sftk_Lg) ==  SFTK_MODE_BACKFRESH) )
					{
						status = STATUS_DEVICE_OFF_LINE;

						DebugPrint( ( DBG_ERROR, "SwrReadWrite: READ: %s : SFTK_MODE_BACKFRESH: DeviceObject %X Irp %X,  Offset %I64d, Size %d, returning status 0x%08x (STATUS_DEVICE_OFF_LINE) \n",
								deviceExtension->Sftk_dev->Vdevname, DeviceObject, Irp, 
								currentIrpStack->Parameters.Read.ByteOffset.QuadPart,
								currentIrpStack->Parameters.Read.Length, status));
						// I believe for moment, we do Back Refresh in Src Disk Offline mode, means No Writes allowed
						// During Back Refresh to src disk, so return error.
						Irp->IoStatus.Status		= status;
						Irp->IoStatus.Information	= 0;
						IoCompleteRequest(Irp, IO_NO_INCREMENT);
						return status;
					}
#endif
					// update stats 
					if (deviceExtension->Sftk_dev != NULL) 
					{
						pSftk_Dev = deviceExtension->Sftk_dev;

						pSftk_Dev->Statistics.RdCount ++;
						pSftk_Dev->Statistics.BlksRd += Get_Values_InSectorsFromBytes(currentIrpStack->Parameters.Read.Length);

						if (pSftk_Dev->SftkLg != NULL)
						{
							pSftk_Dev->SftkLg->Statistics.RdCount ++;
							pSftk_Dev->SftkLg->Statistics.BlksRd += Get_Values_InSectorsFromBytes(currentIrpStack->Parameters.Read.Length);
						}
					}

					break;

		case IRP_MJ_WRITE: 
					
					DebugPrint( ( DBG_RDWR, "SwrReadWrite: WRITE: DeviceObject %X Irp %X,  Offset %I64d, Size %d \n",
									DeviceObject, Irp, 
									currentIrpStack->Parameters.Write.ByteOffset.QuadPart,
									currentIrpStack->Parameters.Write.Length));

					if (deviceExtension->Sftk_dev == NULL) 
						break; // just be pass thru
					
					pSftk_Dev = deviceExtension->Sftk_dev;

					OS_ASSERT(pSftk_Dev->SftkLg != NULL);

					if (pSftk_Dev->SftkLg == NULL)
					{
						DebugPrint( ( DBG_ERROR, "SwrReadWrite: FIXME FIXME (pSftk_Dev->SftkLg == NULL) WRITE: %s : DeviceObject %X Irp %X,  Offset %I64d, Size %d, passing to next driver \n",
								pSftk_Dev->Vdevname, DeviceObject, Irp, 
								currentIrpStack->Parameters.Write.ByteOffset.QuadPart,
								currentIrpStack->Parameters.Write.Length));

						OS_ASSERT(FALSE);
						break;	// ooops!!! should not happen...
					}

					// update stats 
					if (deviceExtension->Sftk_dev != NULL) 
					{
						pSftk_Dev = deviceExtension->Sftk_dev;

						pSftk_Dev->Statistics.WrCount ++;
						pSftk_Dev->Statistics.BlksWr += Get_Values_InSectorsFromBytes(currentIrpStack->Parameters.Write.Length);

						if (pSftk_Dev->SftkLg != NULL)
						{
							pSftk_Dev->SftkLg->Statistics.WrCount ++;
							pSftk_Dev->SftkLg->Statistics.BlksWr += Get_Values_InSectorsFromBytes(currentIrpStack->Parameters.Write.Length);
						}
					}

					currentState = sftk_lg_get_state(pSftk_Dev->SftkLg);
					if (currentState == SFTK_MODE_PASSTHRU)
					{
						DebugPrint( ( DBG_ERROR, "SwrReadWrite: WRITE: %s : SFTK_MODE_PASSTHRU: DeviceObject %X Irp %X,  Offset %I64d, Size %d, passing to next driver \n",
								pSftk_Dev->Vdevname, DeviceObject, Irp, 
								currentIrpStack->Parameters.Write.ByteOffset.QuadPart,
								currentIrpStack->Parameters.Write.Length));
						break;	// nothing to do.
					}

					if (currentState == SFTK_MODE_BACKFRESH)
					{
						status = STATUS_DEVICE_OFF_LINE;
						DebugPrint( ( DBG_ERROR, "SwrReadWrite: WRITE: %s : SFTK_MODE_BACKFRESH: DeviceObject %X Irp %X,  Offset %I64d, Size %d, returning status 0x%08x (STATUS_DEVICE_OFF_LINE) \n",
								pSftk_Dev->Vdevname, DeviceObject, Irp, 
								currentIrpStack->Parameters.Write.ByteOffset.QuadPart,
								currentIrpStack->Parameters.Write.Length, status));

						sftk_LogEventCharStr1Num1( GSftk_Config.DriverObject, MSG_REPL_SRC_DEVICE_WRITE_FAILED, status, 0, pSftk_Dev->Vdevname, pSftk_Dev->SftkLg->LGroupNumber);
						// I believe for moment, we do Back Refresh in Src Disk Offline mode, means No Writes allowed
						// During Back Refresh to src disk, so return error.
						Irp->IoStatus.Status		= status;
						Irp->IoStatus.Information	= 0;
						IoCompleteRequest(Irp, IO_NO_INCREMENT);
						return status;
					}

					// Mark IRP pending and pass this IRP to Device Queue.

					// IRP completion routine 

					// We need to do temporary registartion of Completion Routine. 
					// why ? causes we are passing IRP as status_Pending to IO Manager, This IRP 
					// will be processed by our thread later, Now By this time, if IO Manager decides
					// to cancel this IRP (he can do that...), We need to have completion routine
					// who can get cancellation and undo the queue registry work, so we do not get crash of accessing
					// invalid (or already cancelled).IRP in Thread.

					// Now Inside Thread, we also register another IRP completion routine, this we do only if 
					// BAB successfully allocated for this IRP. That completion routine will have valid opaque
					// pointer to cache manager, so in that completion routine, we can move BAB memory from
					// Pending queue to commit Queue.

					IoCopyCurrentIrpStackLocationToNext(Irp);

					//
					// you can call IoSetCompletionRoutine more than one time for same stack location, if and only
					// if IRP is still currently not processed and resides on the same stack location.
					//
					IoSetCompletionRoutine(Irp, sftk_io_generic_completion, pSftk_Dev, TRUE, TRUE, TRUE);
					IoMarkIrpPending( Irp );	// Mark IRP as pending

					// Queue IRP to our queue list. we use IRP Context fields to add it into link list
					pIrpContext = (PIRP_CONTEXT) &Irp->Tail.Overlay.DriverContext[0];
					InitializeListHead( &pIrpContext->ListEntry );
					pIrpContext->Irp = Irp;
					
					OS_ACQUIRE_LOCK( &pSftk_Dev->MasterQueueLock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
					InsertTailList( &pSftk_Dev->MasterQueueList.ListEntry, &pIrpContext->ListEntry);
					pSftk_Dev->MasterQueueList.NumOfNodes ++;
					OS_RELEASE_LOCK( &pSftk_Dev->MasterQueueLock, NULL);

					// KeReleaseSemaphore(&pSftk_Dev->MasterQueueSemaphore, 0, 1, FALSE);
					KeSetEvent( &pSftk_Dev->MasterQueueEvent, 0, FALSE);

					return STATUS_PENDING;
					
	} // switch(currentIrpStack->MajorFunction)

	OS_ASSERT(deviceExtension->TargetDeviceObject != NULL) ;
	// DebugPrint( ( DBG_RDWR, "SwrReadWrite: sending down: DeviceObject %X Irp %X \n",DeviceObject, Irp));

	// Set current stack back one. and Pass request down to next driver layer.
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);
} // end SwrReadWrite()

NTSTATUS
sftk_io_generic_completion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PSFTK_DEV			Sftk_Dev)
{
	PIO_STACK_LOCATION	currentIrpStack	= IoGetCurrentIrpStackLocation(Irp);
	PIRP_CONTEXT		pIrpContext		= (PIRP_CONTEXT) &Irp->Tail.Overlay.DriverContext[0];

	DebugPrint( ( DBG_RDWR, "sftk_generic_completion: WRITE: DeviceObject %X Irp %X,  Offset %I64d, Size %d, IRP complteted with status 0x%08x Information %d \n",
										DeviceObject, Irp, 
										currentIrpStack->Parameters.Write.ByteOffset.QuadPart,
										currentIrpStack->Parameters.Write.Length,
										Irp->IoStatus.Status,
										Irp->IoStatus.Information));

	// Remove Context IRP from the Queue and 
	// We may not need lock for queue, anyhow for safe side. we grab lock before we remove entry
	OS_ACQUIRE_LOCK( &Sftk_Dev->MasterQueueLock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	pIrpContext->Irp = NULL;

	// Explicitly do Remove entries, since RemoveEntryList() macro does not check for null values in Flink or Blink...
	// RemoveEntryList( &pIrpContext->ListEntry );
	if (pIrpContext->ListEntry.Flink)
		pIrpContext->ListEntry.Flink->Blink = pIrpContext->ListEntry.Blink;

	if (pIrpContext->ListEntry.Blink)
		pIrpContext->ListEntry.Blink->Flink = pIrpContext->ListEntry.Flink;

	pIrpContext->ListEntry.Flink = pIrpContext->ListEntry.Blink = NULL;
	OS_RELEASE_LOCK( &Sftk_Dev->MasterQueueLock, NULL);

	// Propogate IRP Pending states
	if (Irp->PendingReturned) 
	{
        IoMarkIrpPending(Irp);
    }

    return STATUS_SUCCESS;	// since we are done with this IRP, we must return status success
							// if we need to call explicitly IoCompleteRequest for this IRP than only we must return
							// STATUS_MORE_PROCESSING_REQUIRED
} // sftk_io_generic_completion()

NTSTATUS
SwrDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++
Routine Description:
    This device control dispatcher handles only the disk performance
    device control. All others are passed down to the disk drivers.
    The disk performane device control returns a current snapshot of
    the performance data.
Arguments:
    DeviceObject - Context for the activity.
    Irp          - The device control argument block.
Return Value:
    Status is returned.
--*/
{
	NTSTATUS			status					= STATUS_SUCCESS;
    PIO_STACK_LOCATION  pCurrentStackLocation	= NULL;
	PDEVICE_EXTENSION	deviceExtension			= DeviceObject->DeviceExtension;
    BOOLEAN             CompleteIrp				= FALSE;
    LONG                flag = 0, cmd, err = 0;
    LONG                arg;
    ULONG               dev = 0;

    try 
    {
        try 
        {
            // Get the current I/O stack location.
            pCurrentStackLocation = IoGetCurrentIrpStackLocation(Irp);
            OS_ASSERT(pCurrentStackLocation);

            cmd = pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode;
            arg = (int)Irp->AssociatedIrp.SystemBuffer;

			switch(deviceExtension->NodeId.NodeType)
			{
				case NODE_TYPE_SFTK_CTL:	// FTD_NODE_TYPE_EXT_DEVICE
						DebugPrint( ( DBG_DISPATCH, "SwrDeviceControl: DeviceObject %X Irp %X, NODE_TYPE_SFTK_CTL: Ioctl 0x%08x \n",
										DeviceObject, Irp, cmd));

						CompleteIrp = TRUE;

						status = sftk_ctl_ioctl(dev, cmd, arg, flag, Irp); 
#if DBG
						if (status != STATUS_SUCCESS)
						{
							DebugPrint( ( DBG_ERROR , "SwrDeviceControl: DeviceObject %X Irp %X, NODE_TYPE_SFTK_CTL: Ioctl 0x%08x Failed with 0x%08x\n",
										DeviceObject, Irp, cmd, status));
						}
#endif

						try_return(status);
						break;
						
				case NODE_TYPE_SFTK_LG: // FTD_NODE_TYPE_LG_DEVICE 
					{
						// Get a pointer to the lg extension
						SFTK_LG *lgp = (SFTK_LG *)(DeviceObject->DeviceExtension);
						
						DebugPrint( ( DBG_DISPATCH, "SwrDeviceControl: DeviceObject %X Irp %X, NODE_TYPE_SFTK_LG: LG %d Ioctl 0x%08x \n",
										DeviceObject, Irp, lgp->LGroupNumber, cmd, status));

						CompleteIrp = TRUE;
                
#if TARGET_SIDE
						flag = lgp->Role.CreationRole;
#endif
						status = sftk_lg_ioctl(lgp->LGroupNumber, cmd, arg, flag);
#if DBG
						if (status != STATUS_SUCCESS)
						{
							DebugPrint( ( DBG_ERROR , "SwrDeviceControl: DeviceObject %X Irp %X, NODE_TYPE_SFTK_LG: LG %d Ioctl 0x%08x Failed with 0x%08x\n",
										DeviceObject, Irp, lgp->LGroupNumber, cmd, status));
						}
#endif

						try_return(status);
						break;
					} 
				default:
					{
						OS_ASSERT(deviceExtension->TargetDeviceObject != NULL) ;
						DebugPrint( ( DBG_DISPATCH, "SwrDeviceControl: default: sending down: DeviceObject %X Irp %X, default:Attached Device Ioctl 0x%08x \n",
										DeviceObject, Irp, cmd));
#if DBG
						// DbgDisplayIoctlMessage(pCurrentStackLocation);
#endif

						switch (pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode) 
						{
							case IOCTL_DISK_SET_DRIVE_LAYOUT:
								{
								PDRIVE_LAYOUT_INFORMATION	pDriveLayout= (PDRIVE_LAYOUT_INFORMATION)
																			Irp->AssociatedIrp.SystemBuffer;
								// Pass this IRP synchronously !!!
								status = SwrForwardIrpSynchronous(DeviceObject, Irp);
								
								if (NT_SUCCESS(status))
								{ // success
									if (deviceExtension->DeviceInfo.Signature != pDriveLayout->Signature)
									{
										PDEVICE_EXTENSION	pAttachedDevExt	= NULL;
										PLIST_ENTRY			plistEntry		= NULL;
										BOOLEAN				bInformedService= FALSE;
								
										DebugPrint( ( DBG_ERROR , "SwrDeviceControl: IOCTL_DISK_SET_DRIVE_LAYOUT  DeviceObject %X, DevExt %X successed, We are chaning old signature !!!\n",
														DeviceObject, deviceExtension));
										DebugPrint( ( DBG_ERROR , "SwrDeviceControl: old signature 0x%08x New Signature 0x%08x !!!\n",
														deviceExtension->DeviceInfo.Signature, pDriveLayout->Signature));

										DebugPrint( ( DBG_ERROR , "TODO TODO : SwrDeviceControl: Chaned signature should signal event to wake up service and update Config file if it requires.....FIXME FIXME !!!\n"));

										// signature is changed so make new Signature GUID, so Make bSignatureUniqueVolumeId  to FALSE
										deviceExtension->DeviceInfo.bSignatureUniqueVolumeId = FALSE;
										UpdateDevExtDeviceInfo(deviceExtension);

										if (deviceExtension->Sftk_dev != NULL)
										{
											bInformedService = TRUE;	// informed to service about change of Signature GUID
										}

										// - scan thru each and every other devext who has current devext as parent device....
										//   and remake their signature GUID !!!
										OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_SHARED, TRUE, NULL);

										for( plistEntry = GSftk_Config.DevExt_List.ListEntry.Flink;
											 plistEntry != &GSftk_Config.DevExt_List.ListEntry;
											 plistEntry = plistEntry->Flink )
										{ // for :scan thru each and every Attached Dev Extensions..
											 pAttachedDevExt = CONTAINING_RECORD( plistEntry, DEVICE_EXTENSION, DevExt_Link);
											 if (pAttachedDevExt->DeviceInfo.pRawDiskDevice == deviceExtension->DeviceObject)
											 {
												pAttachedDevExt->DeviceInfo.bSignatureUniqueVolumeId = FALSE;
												UpdateDevExtDeviceInfo(pAttachedDevExt);
											 }

											 if (pAttachedDevExt->Sftk_dev != NULL)
											 {
												 bInformedService = TRUE; // informed to service about change of Signature GUID
											 }
										} // for :scan thru each and every Attached Dev Extensions..

										OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
										if (bInformedService == TRUE)
										{ // informed to service about change of Signature GUID
										DebugPrint( ( DBG_ERROR , "TODO TODO ***** : SwrDeviceControl: Changed signature should signal event to wake up service and service must update Config file.....FIXME FIXME **** !!!\n",
														deviceExtension->DeviceInfo.Signature, pDriveLayout->Signature));

										}
									} // if (deviceExtension->DeviceInfo.Signature != pDriveLayout->Signature)
								} // success
								else
								{ // failed
									DebugPrint( ( DBG_ERROR , "SwrDeviceControl: IOCTL_DISK_SET_DRIVE_LAYOUT  DeviceObject %X, DevExt %X failed with status 0x%08x !!!\n",
													DeviceObject, deviceExtension, status));
								}

								IoCompleteRequest(Irp, IO_NO_INCREMENT);
								return status;
								}  // case IOCTL_DISK_SET_DRIVE_LAYOUT:
							default:
									// Set current stack back one. and Pass request down to next driver layer.
									IoSkipCurrentIrpStackLocation(Irp);
									return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);
						} // switch (pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode) 
					}
			} // switch(deviceExtension->NodeId.NodeType)
        } 
        except (sftk_ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
        {
			// Log event message here
			sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
									0, __FILE__, __LINE__, GetExceptionCode());

            // We encountered an exception somewhere, eat it up.
            DebugPrint( ( DBG_ERROR,  "SwrDeviceControl() : EXCEPTION_EXECUTE_HANDLER, status = %d.", GetExceptionCode()));
        }
        try_exit:   NOTHING;
    } 
    finally 
    {
        if (CompleteIrp == TRUE) 
        {
            if (status == STATUS_SUCCESS) 
                Irp->IoStatus.Information = pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
            else 
                Irp->IoStatus.Information = 0;

            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
    }

    return status;
} // end SwrDeviceControl()

NTSTATUS
SwrFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
Routine Description:
    This routine is called for a flush IRPs.  These are sent by the
    system when the file system does a flush.
Arguments:
    DriverObject - Pointer to device object to being shutdown by system.
    Irp          - IRP involved.
Return Value:
    NT Status
--*/
{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
	NTSTATUS			status;

	DebugPrint( ( DBG_DISPATCH, "SwrFlush: DeviceObject %X Irp %X\n",
                    DeviceObject, Irp));

	switch(deviceExtension->NodeId.NodeType)
	{
		case NODE_TYPE_SFTK_CTL:	// FTD_NODE_TYPE_EXT_DEVICE
		case NODE_TYPE_SFTK_LG: // FTD_NODE_TYPE_LG_DEVICE 

			status			= STATUS_INVALID_DEVICE_REQUEST;
			DebugPrint( ( DBG_ERROR, "SwrFlush: Non-Filter Disk Device '%s' !!! DeviceObject %X Irp %X, Returning error 0x%08x \n",
								(deviceExtension->NodeId.NodeType == NODE_TYPE_SFTK_CTL)?"NODE_TYPE_SFTK_CTL":"NODE_TYPE_SFTK_LG",
								DeviceObject, Irp, status ));

			Irp->IoStatus.Status		= status;
            Irp->IoStatus.Information	= 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
			return(status);

			break;
	}

    // Set current stack back one.
    // Set current stack back one. and Pass request down to next driver layer.
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);
} // end SwrFlush()

NTSTATUS
SwrShutdown(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
Routine Description:
    This routine is called for a shutdown.  These are sent by the
    system before it actually shuts down. 
Arguments:
    DriverObject - Pointer to device object to being shutdown by system.
    Irp          - IRP involved.
Return Value:
    NT Status
--*/
{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
	NTSTATUS			status;

    // Set current stack back one.
    DebugPrint( ( DBG_DISPATCH, "SwrShutdown: DeviceObject %X Irp %X\n",
                    DeviceObject, Irp));

	// First flush all LG and their devices to Pstore file with marking Successful Shutdown ON
	sftk_flush_all_pstore(TRUE, TRUE, TRUE);

	switch(deviceExtension->NodeId.NodeType)
	{
		case NODE_TYPE_SFTK_CTL:	// FTD_NODE_TYPE_EXT_DEVICE
		case NODE_TYPE_SFTK_LG: // FTD_NODE_TYPE_LG_DEVICE 
			status			= STATUS_SUCCESS;

			DebugPrint( ( DBG_ERROR, "SwrShutDown: Non-Filter Disk Device '%s' !!! DeviceObject %X Irp %X, Returning error 0x%08x \n",
								(deviceExtension->NodeId.NodeType == NODE_TYPE_SFTK_CTL)?"NODE_TYPE_SFTK_CTL":"NODE_TYPE_SFTK_LG",
								DeviceObject, Irp, status ));
			Irp->IoStatus.Status		= status;
            Irp->IoStatus.Information	= 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
			return(status);

			break;
	}

    // Set current stack back one. and Pass request down to next driver layer.
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);
} // end SwrShutdown()


VOID
SwrUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++
Routine Description:
    Free all the allocated resources, etc.
Arguments:
    DriverObject - pointer to a driver object.
Return Value:
    VOID.
--*/
{
    PAGED_CODE();

	// TODO : If shutdown call does not work than do here. FIXME FIXME: Enable this code. if required
	// flush all LG and their devices to Pstore file with marking Successful Shutdown ON
	// sftk_flush_all_pstore(TRUE);

	// First flush all LG and their devices to Pstore file with marking Successful Shutdown ON
	sftk_flush_all_pstore(TRUE, TRUE, TRUE);

	sftk_Deinit_driver( DriverObject );

    return;
} // SwrUnload()

VOID
SwrLogError(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG UniqueId,
    IN NTSTATUS ErrorCode,
    IN NTSTATUS Status
    )
/*++
Routine Description:
    Routine to log an error with the Error Logger
Arguments:
    DeviceObject - the device object responsible for the error
    UniqueId     - an id for the error
    Status       - the status of the error
Return Value:
    None
--*/
{
    PIO_ERROR_LOG_PACKET errorLogEntry;

	return; // TODO  : Write this with proper logic....

    errorLogEntry = (PIO_ERROR_LOG_PACKET)
                    IoAllocateErrorLogEntry(
                        DeviceObject,
                        (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) + sizeof(DEVICE_OBJECT))
                        );

    if (errorLogEntry != NULL) {
        errorLogEntry->ErrorCode = ErrorCode;
        errorLogEntry->UniqueErrorValue = UniqueId;
        errorLogEntry->FinalStatus = Status;
        //
        // The following is necessary because DumpData is of type ULONG
        // and DeviceObject can be more than that
        //
        OS_RtlCopyMemory(
            &errorLogEntry->DumpData[0],
            &DeviceObject,
            sizeof(DEVICE_OBJECT));
        errorLogEntry->DumpDataSize = sizeof(DEVICE_OBJECT);
        IoWriteErrorLogEntry(errorLogEntry);
    }
} // SwrLogError()


#if DBG
#if DBG_MESSAGE_DUMP_TO_FILE
#define DEBUG_BUFFER_LENGTH 1024
UCHAR DbgBufferString[DEBUG_BUFFER_LENGTH];
#endif

VOID
SwrDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )
/*++
Routine Description:
    Debug print for all Swr
Arguments:
    Debug print level between 0 and 3, with 3 being the most verbose.
Return Value:
    None
--*/
{
    // if ((DebugPrintLevel <= (SwrDebug & 0x0000ffff)) || ((1 << (DebugPrintLevel + 15)) & SwrDebug)) 
	if (SwrDebug & DebugPrintLevel)
	{
#if DBG_MESSAGE_DUMP_TO_FILE
		va_list ap;
		va_start(ap, DebugMessage);
		_vsnprintf(DbgBufferString, DEBUG_BUFFER_LENGTH, DebugMessage, ap);
		va_end(ap);
		// vswprintf
        OS_DbgPrint((DbgBufferString));
#else
		KdPrint((DebugMessage));
#endif
    }
    
} // SwrDebugPrint()
#endif

//
//	VOID TruncateUnicodeString(	PUNICODE_STRING UniSource, 
//								PUNICODE_STRING UniDestination, 
//								USHORT			MaxSizeOfDestLength,
//								PCHAR			PreCharStringtoTruncate )
//
//		Truncate source Unicode string and copy it into Destination string
//			Destination unicode string must have assigned valid Buffer pointer and 
//			MaximumLength before calling this function. 
//	
//	Calling/Exit State:
//		Unused arguments are represented by null pointers.
//
//	Description:
//		Truncate source Unicode string and copy it into Destination string
//	1) if PreCharStringtoTruncate specified and source unicode strind does have this prefix
//	   string then it truncate this string and copy rest of source unicode string to destination
//2) if MaxSizeOfDestLength is greater than final Destination string then it truncates post string 
//   of destination string then it again truncates destination post string and final length of 
//	  destination string gets set to MaxSizeOfDestLength
//	
//	Notes:
//		Must passed proper value in argument MaxSizeOfDestLength, and PreCharStringtoTruncate.
//	Must have initialized with proper buffer to UniDestination->Buffer
//	Must have initialized with proper Maxlength to UniDestination->MaximumLength with actual size of
//	Buffer.
VOID
TruncateUnicodeString(	PUNICODE_STRING UniSource, 
						PUNICODE_STRING UniDestination, 
						USHORT			MaxSizeOfDestLength,
						OPTIONAL PCHAR	PreCharStringtoTruncate )
{
	long lenofstring, i,j,k;

	UniDestination->Length = 0;	// make it sure it set to zero in case any error happens we are safe
	if (UniSource->Length > UniDestination->MaximumLength)	
	{
		KdPrint(("HA:TruncateUnicodeString:dst buffer overflow, dst buffer maxlength %d is less than source length %d\n",
				UniDestination->MaximumLength, UniSource->Length));
		return;
	}

	UniDestination->Length = 0;

	
	lenofstring = strlen(PreCharStringtoTruncate);

	// Copy Source unicode to Destination unicode string with pre truncation 
	for(i=0, j=0, k=0;(i < (UniSource->Length/2)); i++, j++)	
	{
		if ((PreCharStringtoTruncate) && (j < lenofstring) )
		{ // truncate pre string
			if (UniSource->Buffer[i] == (WCHAR) PreCharStringtoTruncate[j])
			{
				continue;
			}
		}
		UniDestination->Buffer[k] = UniSource->Buffer[i];
		UniDestination->Length += 2;
		k++;
	}
	
	// truncate Length of Destination string to asked MaxSizeOfDestLength
	if (UniDestination->Length > MaxSizeOfDestLength )
	{
		UniDestination->Length = MaxSizeOfDestLength;
	}

	// terminate destination string with Unicode NULL 
	UniDestination->Buffer[UniDestination->Length/2] = UNICODE_NULL;

} // TruncateUnicodeString()


//***************************************************************************
//  Event Logging Functions
//***************************************************************************

//***************************************************************************
//  DrvLogEvent()
//      This function allows logging events to the Windows NT System event
//      log, allowing the logging of multiple insertion strings as well
//      as binary data.  Note that there is a predefined small limit
//      on the number of bytes that the error log packet may be (around 150
//      bytes).
//***************************************************************************

NTSTATUS    DrvLogEvent( IN  PVOID    DriverOrDeviceObject,
                         IN  ULONG    LogMessageCode,
                         IN  USHORT   EventCategory,
                         IN  UCHAR    MajorFunction,
                         IN  ULONG    IoControlCode,
                         IN  UCHAR    RetryCount,
                         IN  NTSTATUS FinalStatus,
                         IN  PVOID    DumpData,
                         IN  USHORT   DumpDataSize,
                         IN  LPWSTR   WszInsertionString, // Strings (if any) go here...
                         ... )
{
    PIO_ERROR_LOG_PACKET    pIoErrorLogPacket           = NULL;
    UCHAR                   ucIoErrorLogPacketSize      = 0;
    va_list                 pArgs                       = NULL;
    USHORT                  usStringIndex               = 0;
    PWCHAR                  wszCurrentInsertionString   = NULL;
    ULONG                   ulTotalStringLength         = 0;
    PUCHAR                  pucCurrentStringOffset      = NULL;
    USHORT                  usNumberOfStrings           = 0;
	ULONG                   ulLengthLeft				= 0;
	ULONG                   ulLengthDone				= 0;


    ulTotalStringLength = 0;
    usNumberOfStrings   = 0;

    if ((DumpDataSize % sizeof(ULONG)) != 0)  
		return STATUS_INVALID_PARAMETER;

    // Count the number of strings, and find out how much space they all need
    va_start( pArgs, WszInsertionString );
    wszCurrentInsertionString = WszInsertionString;
    while (wszCurrentInsertionString != NULL)   // While we still have more strings...
    {
        usNumberOfStrings++;
        ulTotalStringLength += (wcslen(wszCurrentInsertionString) * sizeof(WCHAR))
                            +  sizeof(WCHAR); // (last one is for the terminating NULL
        wszCurrentInsertionString = va_arg( pArgs, PWCHAR );
    }
    va_end( pArgs );

    ucIoErrorLogPacketSize  = (UCHAR) (sizeof(IO_ERROR_LOG_PACKET) + DumpDataSize + ulTotalStringLength);

    if ((ucIoErrorLogPacketSize % 4) != 0)
        ucIoErrorLogPacketSize += 4 - (ucIoErrorLogPacketSize % 4);

	// Callers of IoAllocateErrorLogEntry must be running at IRQL <= DISPATCH_LEVEL
    pIoErrorLogPacket   = IoAllocateErrorLogEntry(  DriverOrDeviceObject,
                                                    ucIoErrorLogPacketSize );
    if (pIoErrorLogPacket == NULL)
    {
        KdPrint(( "*** Error - Allocating %d bytes for error log, max is %d - ID: %u\n",
                  ucIoErrorLogPacketSize, ERROR_LOG_MAXIMUM_SIZE, LogMessageCode ));

		// Truncate Last String for display and continue for allocation 
		ucIoErrorLogPacketSize = ERROR_LOG_MAXIMUM_SIZE;
		ulTotalStringLength = ucIoErrorLogPacketSize - ((UCHAR) (sizeof(IO_ERROR_LOG_PACKET) + DumpDataSize));
		// Max ulTotalStringLength = 152 WCHAR
		pIoErrorLogPacket   = IoAllocateErrorLogEntry(  DriverOrDeviceObject,
														ucIoErrorLogPacketSize );
		if (pIoErrorLogPacket == NULL)
		{
			KdPrint(( "*** Error - 2nd time failed.... Allocating %d bytes for error log, max is %d - ID: %u\n",
                  ucIoErrorLogPacketSize, ERROR_LOG_MAXIMUM_SIZE, LogMessageCode ));

			return STATUS_NO_MEMORY;
		}
    }

    // We got here, so we were successful allocating memory...
    RtlZeroMemory( pIoErrorLogPacket, ucIoErrorLogPacketSize );

    pIoErrorLogPacket->ErrorCode            = LogMessageCode;
    pIoErrorLogPacket->EventCategory        = EventCategory;
    pIoErrorLogPacket->MajorFunctionCode    = MajorFunction;
    pIoErrorLogPacket->IoControlCode        = IoControlCode;
    pIoErrorLogPacket->RetryCount           = RetryCount;
    pIoErrorLogPacket->FinalStatus          = FinalStatus;
    pIoErrorLogPacket->DumpDataSize         = DumpDataSize;
    pIoErrorLogPacket->NumberOfStrings      = usNumberOfStrings;
    pIoErrorLogPacket->StringOffset         = sizeof(IO_ERROR_LOG_PACKET) + DumpDataSize;

    // Unused values...
    pIoErrorLogPacket->UniqueErrorValue      = 0;
    pIoErrorLogPacket->SequenceNumber        = 0;
    pIoErrorLogPacket->DeviceOffset.LowPart  = 0;
    pIoErrorLogPacket->DeviceOffset.HighPart = 0;

    // Now see if we should copy any dump data into our packet...
    if (DumpDataSize > 0)
        RtlCopyMemory( &(pIoErrorLogPacket->DumpData), DumpData, DumpDataSize );

    pucCurrentStringOffset = (PUCHAR) ((ULONG) pIoErrorLogPacket
                                + sizeof(IO_ERROR_LOG_PACKET) + DumpDataSize );

	ulLengthLeft = 0;
	ulLengthDone = 0;
    va_start( pArgs, DumpDataSize );
    for ( usStringIndex = 0; usStringIndex < usNumberOfStrings; usStringIndex++ )
    {
        wszCurrentInsertionString = va_arg( pArgs, PWCHAR );

		ulLengthLeft  = (wcslen( wszCurrentInsertionString ) * sizeof(WCHAR) + sizeof(WCHAR));
		if ( (ulLengthDone + ulLengthLeft) > ulTotalStringLength)
		{
			ulLengthLeft = ulTotalStringLength - ulLengthDone;
		}

		if (ulLengthLeft <= sizeof(WCHAR))
			break; // can't do anthing, skip last string

        RtlMoveMemory( pucCurrentStringOffset, wszCurrentInsertionString, (ulLengthLeft - sizeof(WCHAR)) ); // Null wchar

        pucCurrentStringOffset += ulLengthLeft;
		ulLengthDone += ulLengthLeft;

		if (ulLengthDone >= ulTotalStringLength)
			break; // we are done
    }
    va_end( pArgs );

	// Callers of this routine must be running at IRQL <= DISPATCH_LEVEL
    IoWriteErrorLogEntry( pIoErrorLogPacket );
    return STATUS_SUCCESS;

} // DrvLogEvent()

#if DBG
VOID
DbgDisplayIoctlMessage(PIO_STACK_LOCATION pCurrentStackLocation)
{
	if (!(SwrDebug & DBG_IOCTL_DISPLAY))
		return;

	switch (pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode) 
	{
		// IOCTL_DISK_BASE : FILE_DEVICE_DISK
		case IOCTL_DISK_GET_DRIVE_GEOMETRY:
			KdPrint(("IOCTL_DISK_GET_DRIVE_GEOMETRY: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;   
		case IOCTL_DISK_GET_PARTITION_INFO  :
			KdPrint(("IOCTL_DISK_GET_PARTITION_INFO: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break; 
		case IOCTL_DISK_SET_PARTITION_INFO   :
			KdPrint(("IOCTL_DISK_SET_PARTITION_INFO: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;
		case IOCTL_DISK_GET_DRIVE_LAYOUT     :
			KdPrint(("IOCTL_DISK_GET_DRIVE_LAYOUT: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;
		case IOCTL_DISK_SET_DRIVE_LAYOUT     :
			KdPrint(("IOCTL_DISK_SET_DRIVE_LAYOUT: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;
		case IOCTL_DISK_VERIFY:
			KdPrint(("IOCTL_DISK_VERIFY: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;
		case IOCTL_DISK_FORMAT_TRACKS:
			KdPrint(("IOCTL_DISK_FORMAT_TRACKS: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;
		case IOCTL_DISK_REASSIGN_BLOCKS:
			KdPrint(("IOCTL_DISK_REASSIGN_BLOCKS: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;
		case IOCTL_DISK_PERFORMANCE:
			KdPrint(("IOCTL_DISK_PERFORMANCE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;
		case IOCTL_DISK_IS_WRITABLE:
			KdPrint(("IOCTL_DISK_IS_WRITABLE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;
		case IOCTL_DISK_LOGGING:
			KdPrint(("IOCTL_DISK_LOGGING: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;
		case IOCTL_DISK_FORMAT_TRACKS_EX:
			KdPrint(("IOCTL_DISK_FORMAT_TRACKS_EX: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;
		case IOCTL_DISK_HISTOGRAM_STRUCTURE:
			KdPrint(("IOCTL_DISK_HISTOGRAM_STRUCTURE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;
		case IOCTL_DISK_HISTOGRAM_DATA:
			KdPrint(("IOCTL_DISK_HISTOGRAM_DATA: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;
		case IOCTL_DISK_HISTOGRAM_RESET:
			KdPrint(("IOCTL_DISK_HISTOGRAM_RESET: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;
		case IOCTL_DISK_REQUEST_STRUCTURE:
			KdPrint(("IOCTL_DISK_REQUEST_STRUCTURE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;
		case IOCTL_DISK_REQUEST_DATA:
			KdPrint(("IOCTL_DISK_REQUEST_DATA: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;
		case IOCTL_DISK_CONTROLLER_NUMBER:
			KdPrint(("IOCTL_DISK_CONTROLLER_NUMBER: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;             
		case SMART_GET_VERSION:
			KdPrint(("SMART_GET_VERSION: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;                    
		case SMART_SEND_DRIVE_COMMAND:
			KdPrint(("SMART_SEND_DRIVE_COMMAND: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;             
		case SMART_RCV_DRIVE_DATA:
			KdPrint(("SMART_RCV_DRIVE_DATA: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;             
		case IOCTL_DISK_UPDATE_DRIVE_SIZE:
			KdPrint(("IOCTL_DISK_UPDATE_DRIVE_SIZE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;             
		case IOCTL_DISK_GROW_PARTITION:
			KdPrint(("IOCTL_DISK_GROW_PARTITION: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;             
		case IOCTL_DISK_GET_CACHE_INFORMATION:
			KdPrint(("IOCTL_DISK_GET_CACHE_INFORMATION: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;             
		case IOCTL_DISK_SET_CACHE_INFORMATION:
			KdPrint(("IOCTL_DISK_SET_CACHE_INFORMATION: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;             
		case IOCTL_DISK_DELETE_DRIVE_LAYOUT:
			KdPrint(("IOCTL_DISK_DELETE_DRIVE_LAYOUT: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;             
		case IOCTL_DISK_FORMAT_DRIVE:
			KdPrint(("IOCTL_DISK_FORMAT_DRIVE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;             
		case IOCTL_DISK_SENSE_DEVICE:
			KdPrint(("IOCTL_DISK_SENSE_DEVICE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;             
		case IOCTL_DISK_INTERNAL_SET_VERIFY:
			KdPrint(("IOCTL_DISK_INTERNAL_SET_VERIFY: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;             
		case IOCTL_DISK_INTERNAL_CLEAR_VERIFY:
			KdPrint(("IOCTL_DISK_INTERNAL_CLEAR_VERIFY: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;             
		case IOCTL_DISK_INTERNAL_SET_NOTIFY:
			KdPrint(("IOCTL_DISK_INTERNAL_SET_NOTIFY: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;             
		case IOCTL_DISK_CHECK_VERIFY:
			KdPrint(("IOCTL_DISK_CHECK_VERIFY: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;             
		case IOCTL_DISK_MEDIA_REMOVAL:
			KdPrint(("IOCTL_DISK_MEDIA_REMOVAL: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;             
		case IOCTL_DISK_EJECT_MEDIA:
			KdPrint(("IOCTL_DISK_EJECT_MEDIA: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;                  
		case IOCTL_DISK_LOAD_MEDIA:
			KdPrint(("IOCTL_DISK_LOAD_MEDIA: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;                   
		case IOCTL_DISK_RESERVE:
			KdPrint(("IOCTL_DISK_RESERVE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;                      
		case IOCTL_DISK_RELEASE:
			KdPrint(("IOCTL_DISK_RELEASE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;                      
		case IOCTL_DISK_FIND_NEW_DEVICES:
			KdPrint(("IOCTL_DISK_FIND_NEW_DEVICES: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;             
		case IOCTL_DISK_GET_MEDIA_TYPES:
			KdPrint(("IOCTL_DISK_GET_MEDIA_TYPES: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;              
		case IOCTL_DISK_SIMBAD:
			KdPrint(("IOCTL_DISK_SIMBAD: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;             

		// MOUNTDEVCONTROLTYPE:
		case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
			KdPrint(("IOCTL_MOUNTDEV_QUERY_UNIQUE_ID: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;

		case IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY:
			KdPrint(("IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;  
		case IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME:
			KdPrint(("IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;
		case IOCTL_MOUNTDEV_LINK_CREATED:
			KdPrint(("IOCTL_MOUNTDEV_LINK_CREATED: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;
		case IOCTL_MOUNTDEV_LINK_DELETED:
			KdPrint(("IOCTL_MOUNTDEV_LINK_DELETED: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;          
		case IOCTL_MOUNTMGR_CREATE_POINT:
			KdPrint(("IOCTL_MOUNTMGR_CREATE_POINT: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;            
		case IOCTL_MOUNTMGR_DELETE_POINTS:
			KdPrint(("IOCTL_MOUNTMGR_DELETE_POINTS: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;            
		case IOCTL_MOUNTMGR_QUERY_POINTS:
			KdPrint(("IOCTL_MOUNTMGR_QUERY_POINTS: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;            
		case IOCTL_MOUNTMGR_DELETE_POINTS_DBONLY:
			KdPrint(("IOCTL_MOUNTMGR_DELETE_POINTS_DBONLY: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;                   
		case IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER:
			KdPrint(("IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;                      
		case IOCTL_MOUNTMGR_AUTO_DL_ASSIGNMENTS:
			KdPrint(("IOCTL_MOUNTMGR_AUTO_DL_ASSIGNMENTS: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;            
		case IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_CREATED:
			KdPrint(("IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_CREATED: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;            
		case IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_DELETED:
			KdPrint(("IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_DELETED: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;            
		case IOCTL_MOUNTMGR_CHANGE_NOTIFY:
			KdPrint(("IOCTL_MOUNTMGR_CHANGE_NOTIFY: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;            
		case IOCTL_MOUNTMGR_KEEP_LINKS_WHEN_OFFLINE:
			KdPrint(("IOCTL_MOUNTMGR_KEEP_LINKS_WHEN_OFFLINE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;            
		case IOCTL_MOUNTMGR_CHECK_UNPROCESSED_VOLUMES:
			KdPrint(("IOCTL_MOUNTMGR_CHECK_UNPROCESSED_VOLUMES: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;            
		case IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION:
			KdPrint(("IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;            
		case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
			KdPrint(("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
			
		// IOCTL_STORAGE_BASE : FILE_DEVICE_MASS_STORAGE
		case IOCTL_STORAGE_CHECK_VERIFY:
			KdPrint(("IOCTL_STORAGE_CHECK_VERIFY: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_STORAGE_CHECK_VERIFY2:
			KdPrint(("IOCTL_STORAGE_CHECK_VERIFY2: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_STORAGE_MEDIA_REMOVAL:
			KdPrint(("IOCTL_STORAGE_MEDIA_REMOVAL: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_STORAGE_EJECT_MEDIA:
			KdPrint(("IOCTL_STORAGE_EJECT_MEDIA: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_STORAGE_LOAD_MEDIA:
			KdPrint(("IOCTL_STORAGE_LOAD_MEDIA: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_STORAGE_LOAD_MEDIA2:
			KdPrint(("IOCTL_STORAGE_LOAD_MEDIA2: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_STORAGE_RESERVE:
			KdPrint(("IOCTL_STORAGE_RESERVE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_STORAGE_RELEASE:
			KdPrint(("IOCTL_STORAGE_RELEASE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_STORAGE_FIND_NEW_DEVICES:
			KdPrint(("IOCTL_STORAGE_FIND_NEW_DEVICES: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_STORAGE_EJECTION_CONTROL:
			KdPrint(("IOCTL_STORAGE_EJECTION_CONTROL: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_STORAGE_MCN_CONTROL:
			KdPrint(("IOCTL_STORAGE_MCN_CONTROL: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_STORAGE_GET_MEDIA_TYPES:
			KdPrint(("IOCTL_STORAGE_GET_MEDIA_TYPES: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_STORAGE_GET_MEDIA_TYPES_EX:
			KdPrint(("IOCTL_STORAGE_GET_MEDIA_TYPES_EX: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_STORAGE_RESET_BUS:
			KdPrint(("IOCTL_STORAGE_RESET_BUS: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_STORAGE_RESET_DEVICE:
			KdPrint(("IOCTL_STORAGE_RESET_DEVICE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_STORAGE_GET_DEVICE_NUMBER:
			KdPrint(("IOCTL_STORAGE_GET_DEVICE_NUMBER: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_STORAGE_PREDICT_FAILURE:
			KdPrint(("IOCTL_STORAGE_PREDICT_FAILURE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_STORAGE_QUERY_PROPERTY:
			KdPrint(("IOCTL_STORAGE_QUERY_PROPERTY: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case OBSOLETE_IOCTL_STORAGE_RESET_BUS:
			KdPrint(("OBSOLETE_IOCTL_STORAGE_RESET_BUS: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case OBSOLETE_IOCTL_STORAGE_RESET_DEVICE:
			KdPrint(("OBSOLETE_IOCTL_STORAGE_RESET_DEVICE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           

		// IOCTL_VOLUME_BASE : FILE_DEVICE_MASS_STORAGE
		case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS:
			KdPrint(("IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_VOLUME_SUPPORTS_ONLINE_OFFLINE:
			KdPrint(("IOCTL_VOLUME_SUPPORTS_ONLINE_OFFLINE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_VOLUME_ONLINE:
			KdPrint(("IOCTL_VOLUME_ONLINE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_VOLUME_OFFLINE:
			KdPrint(("IOCTL_VOLUME_OFFLINE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_VOLUME_IS_OFFLINE:
			KdPrint(("IOCTL_VOLUME_IS_OFFLINE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_VOLUME_IS_IO_CAPABLE:
			KdPrint(("IOCTL_VOLUME_IS_IO_CAPABLE: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_VOLUME_QUERY_FAILOVER_SET:
			KdPrint(("IOCTL_VOLUME_QUERY_FAILOVER_SET: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_VOLUME_QUERY_VOLUME_NUMBER:
			KdPrint(("IOCTL_VOLUME_QUERY_VOLUME_NUMBER: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_VOLUME_LOGICAL_TO_PHYSICAL:
			KdPrint(("IOCTL_VOLUME_LOGICAL_TO_PHYSICAL: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
		case IOCTL_VOLUME_PHYSICAL_TO_LOGICAL:
			KdPrint(("IOCTL_VOLUME_PHYSICAL_TO_LOGICAL: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           

		default:
			KdPrint(("Default: 0x%08x, InputBufferLength %d OutputBufferLength: %d\n",
										pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode,
										pCurrentStackLocation->Parameters.DeviceIoControl.InputBufferLength,
										pCurrentStackLocation->Parameters.DeviceIoControl.OutputBufferLength));
			break;           
	} // switch (pCurrentStackLocation->Parameters.DeviceIoControl.IoControlCode) 
} // DbgDisplayIoctlMessage()
#endif

