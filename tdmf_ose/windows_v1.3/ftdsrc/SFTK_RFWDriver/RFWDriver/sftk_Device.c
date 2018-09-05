/**************************************************************************************

Module Name:	sftk_Device.C   
Author Name:	Parag sanghvi
Description:	All Device related API and their initialization/Deinitialization 
				Defined over here.
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/

#include <sftk_main.h>
// Following IOCTLs and Structure needed to defined explictly since it exist in Wxp and W2K Above OS ntdddisk.h file
#define IOCTL_DISK_GET_LENGTH_INFO          CTL_CODE(IOCTL_DISK_BASE, 0x0017, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// The structure GET_LENGTH_INFORMATION is used with the ioctl
// IOCTL_DISK_GET_LENGTH_INFO to obtain the length, in bytes, of the
// disk, partition, or volume.
//

typedef struct _GET_LENGTH_INFORMATION {
    LARGE_INTEGER   Length;
} GET_LENGTH_INFORMATION, *PGET_LENGTH_INFORMATION;

// Allocate and Initialize global variables, create and initialize Control device, 
// and do driver private initialization
NTSTATUS
sftk_init_driver(	IN PDRIVER_OBJECT DriverObject,
					IN PUNICODE_STRING RegistryPath )
{
	NTSTATUS			status = STATUS_SUCCESS;
	
	//
	// Initialize Global Variables.
	//
	OS_ZeroMemory( &GSftk_Config, sizeof(GSftk_Config));

	sftk_init_perf_monitor();	// initialize performance monitor database or table.

	GSftk_Config.NodeId.NodeType	= NODE_TYPE_SFTK_CONFIG;
	GSftk_Config.NodeId.NodeSize	= sizeof(GSftk_Config);
	GSftk_Config.DriverObject		= DriverObject;
	// GSftk_Config.CtrlDeviceObject
	ANCHOR_InitializeListHead( GSftk_Config.Lg_GroupList );
	ANCHOR_InitializeListHead( GSftk_Config.TLg_GroupList );
	ANCHOR_InitializeListHead( GSftk_Config.SftkDev_List);
	ANCHOR_InitializeListHead( GSftk_Config.DevExt_List);

	ANCHOR_InitializeListHead( GSftk_Config.SftkDev_PnpRemovedList);

	OS_INITIALIZE_LOCK( &GSftk_Config.Lock, OS_ERESOURCE_LOCK, NULL);
	
	// Remember registry path
    GDriverRegistryPath.MaximumLength = RegistryPath->Length + sizeof(UNICODE_NULL);
    GDriverRegistryPath.Buffer = OS_AllocMemory( PagedPool,GDriverRegistryPath.MaximumLength);
    if (GDriverRegistryPath.Buffer != NULL)
    {
        RtlCopyUnicodeString(&GDriverRegistryPath, RegistryPath);
    } 
	else 
	{
		// Not critical error
		DebugPrint((DBG_ERROR, "sftk_init_driver: OS_AllocMemory(Pagepool, size %d) Failed: \n", GDriverRegistryPath.MaximumLength));
        GDriverRegistryPath.Length = 0;
        GDriverRegistryPath.MaximumLength = 0;
    }

	// Initializing the Global Source Listen Thread and the Target Listen Threads and Exit Events.
	GSftk_Config.ListenThread = NULL;
	GSftk_Config.TListenThread = NULL;

	KeInitializeEvent( &GSftk_Config.ListenThreadExitEvent, SynchronizationEvent, FALSE);
	KeInitializeEvent( &GSftk_Config.TListenThreadExitEvent, SynchronizationEvent, FALSE);

	//
	// Initialize Parameter setting values from Registry...
	//
	DebugPrint((DBG_DISPATCH, "sftk_init_driver: TODO : Registry variable initalization....\n"));

	//
	// Create Control device here
	//
	status = sftk_CreateCtlDevice(DriverObject);
	if ( !NT_SUCCESS(status) )
	{ // Failed
		// API has already done detailed log event message 
		DebugPrint((DBG_ERROR, "sftk_init_driver: sftk_CreateCtlDevice() Failed with Error 0x%08x \n",status));
		goto done;
	}

	// Initialize other memory initializations
	/*
	ExInitializeNPagedLookasideList(	&ctlp->LGIoLookasideListHead,
                                        NULL,
                                        NULL,
                                        NonPagedPoolMustSucceed,
                                        sizeof(ftd_lg_io_t),
                                        'dtfZ',
                                        (USHORT)300);
    FTDSetFlag(ctlp->flags, FTD_CTL_FLAGS_LGIO_CREATED);
	*/

	OS_SetFlag( GSftk_Config.Flag, SFTK_CONFIG_FLAG_CACHE_CREATED);

	// status = sftk_configured_from_registry();
	if (!NT_SUCCESS(status))
	{ // Failed
		DebugPrint((DBG_ERROR, "sftk_init_driver: sftk_configured_from_registry() Failed with error %08x, ignoring this error \n", status));
		//return status;
	}

#if	MM_TEST_WINDOWS_SLAB
	status = MM_LookAsideList_Test();
	if (!NT_SUCCESS(status))
	{ // Failed
		DebugPrint((DBG_ERROR, "sftk_init_driver: MM_LookAsideList_Test() Failed with error %08x \n", status));
		//return status;
	}
#endif

	status = STATUS_SUCCESS;

done:
	 if (!NT_SUCCESS(status)) 
	 {	// Failed
		 // Free all allocated resources.
		 if (GDriverRegistryPath.Buffer)
			OS_FreeMemory(GDriverRegistryPath.Buffer);

		GDriverRegistryPath.Buffer = NULL;
		OS_DEINITIALIZE_LOCK( &GSftk_Config.Lock, NULL);

		OS_ZeroMemory( &GSftk_Config, sizeof(GSftk_Config));
	 }

	return status;

} // sftk_init_driver()

// DeInitialize and Free global variables, Delete Control device, 
// and do driver private Deinitialization

NTSTATUS
sftk_Deinit_driver( IN PDRIVER_OBJECT DriverObject )
{
	//
	// DeInitialize Parameter setting values from Registry...
	//
	// Free all allocated resources.

	if(OS_IsFlagSet(GSftk_Config.Flag, SFTK_CONFIG_FLAG_SYMLINK_CREATED) )
	{
		UNICODE_STRING      userVisibleName;

		RtlInitUnicodeString(&userVisibleName, FTD_DOS_DRV_DIR_CTL_NAME );	// initialized to null
		IoDeleteSymbolicLink( &userVisibleName);
	}

	if(GSftk_Config.CtrlDeviceObject)
		IoDeleteDevice(GSftk_Config.CtrlDeviceObject);

	if (GSftk_Config.DirectoryObject)
		sftk_del_dir( FTD_DRV_DIR, GSftk_Config.DirectoryObject);

	if (GSftk_Config.DosDirectoryObject)
		sftk_del_dir( FTD_DOS_DRV_DIR, GSftk_Config.DosDirectoryObject);

	if (GDriverRegistryPath.Buffer)
		OS_FreeMemory(GDriverRegistryPath.Buffer);

	GDriverRegistryPath.Buffer = NULL;
	OS_DEINITIALIZE_LOCK( &GSftk_Config.Lock, NULL);

	return STATUS_SUCCESS;
} // sftk_Deinit_driver()

/*************************************************************************
*
* Function: sftk_create_dir()
*
* Description:
*   Create a directory in the object name space. Make the directory
*   a temporary object if requested by the caller.
*
* Expected Interrupt Level (for execution) :
*
*  IRQL_PASSIVE_LEVEL
*
* Return Value: STATUS_SUCCESS/an appropriate error
*
*************************************************************************/
NTSTATUS
sftk_create_dir(PWCHAR DirectoryNameStr, PVOID PtrReturnedObject)
{
    NTSTATUS                        status			= STATUS_SUCCESS;
	BOOLEAN                         bHandleCreated	= FALSE;
    OBJECT_ATTRIBUTES               directoryAttributes;
    UNICODE_STRING                  directoryName;
    HANDLE                          hnd;
    

    try {
        // Create a Unicode string.
        RtlInitUnicodeString(&directoryName, DirectoryNameStr);

        // Create an object attributes structure.
        InitializeObjectAttributes(	&directoryAttributes,
									&directoryName, 
									OBJ_PERMANENT,
                                    NULL, NULL);
    
        // The following call will fail if we do not have appropriate privileges.
		status = ZwCreateDirectoryObject(	&hnd,
											DIRECTORY_ALL_ACCESS,
											&directoryAttributes);

        if (!NT_SUCCESS(status) ) 
        {
			DebugPrint((DBG_ERROR, "sftk_create_dir: ZwCreateDirectoryObject(for %S) Failed, with status = 0x%08x \n", 
						DirectoryNameStr, status));
            try_return(status);
        }

        bHandleCreated = TRUE;

		status = ObReferenceObjectByHandle( hnd,
											OBJECT_TYPE_ALL_ACCESS,
											(POBJECT_TYPE) NULL,
											KernelMode,
											PtrReturnedObject,
											(POBJECT_HANDLE_INFORMATION) NULL);
        if (!NT_SUCCESS(status) ) 
        {
			DebugPrint((DBG_ERROR, "sftk_create_dir: ObReferenceObjectByHandle(for %S) Failed, with status = 0x%08x \n", 
						DirectoryNameStr, status));
            try_return(status);
        }

        try_exit: NOTHING;

    } 
    finally 
    {
        // Make the named directory object a temporary object.
        if (bHandleCreated == TRUE) 
        {
            ZwClose(hnd);            
        }
    }

    return status; 
} // sftk_create_dir()


NTSTATUS
sftk_del_dir(PWCHAR DirectoryNameStr, PVOID PtrObject)
{
    NTSTATUS                        status	= STATUS_SUCCESS;
	HANDLE                          hnd		= NULL;
    UNICODE_STRING                  directoryName;
    OBJECT_ATTRIBUTES               directoryAttributes;
    

    try 
    {
        ObDereferenceObject(PtrObject);

        // Create a Unicode string.
        RtlInitUnicodeString(&directoryName, DirectoryNameStr);

        // Create an object attributes structure.
        InitializeObjectAttributes(&directoryAttributes,
                                    &directoryName, OBJ_OPENIF,
                                    NULL, NULL);
    
        // The following call will fail if we do not have appropriate privileges.
		status = ZwCreateDirectoryObject(	&hnd,
											DIRECTORY_ALL_ACCESS,
											&directoryAttributes);
        if (!NT_SUCCESS(status) ) 
        {
			DebugPrint((DBG_ERROR, "sftk_del_dir: ObReferenceObjectByHandle(for %S) Failed, with status = 0x%08x \n", 
						DirectoryNameStr, status));
            try_return(status);
        }

        ZwMakeTemporaryObject(hnd);
        ZwClose(hnd);            

        try_exit: NOTHING;

    } 
    finally 
    {
    }

    return status; 
} // sftk_del_dir()


// Create DirectoryObject/ControlDevice for kernel and Win32 device object
NTSTATUS 
sftk_CreateCtlDevice(IN PDRIVER_OBJECT DriverObject)
{
	NTSTATUS			status	= STATUS_SUCCESS;
	UNICODE_STRING      driverDeviceName, userVisibleName;
	PSFTK_CTL_DEV_EXT	pCtlDevExt;

	GSftk_Config.CtrlDeviceObject	= NULL;
	GSftk_Config.DirectoryObject	= NULL;
	GSftk_Config.DosDirectoryObject= NULL;

	RtlInitUnicodeString(&driverDeviceName, FTD_DRV_DIR_CTL_NAME);	// initialized to null
	RtlInitUnicodeString(&userVisibleName, FTD_DOS_DRV_DIR_CTL_NAME );	// initialized to null
	
	// It would be nice to create directories   in which to create named (temporary) ftd driver objects.
    // Start with a directory in the NT Object Name Space.
	status = sftk_create_dir(	FTD_DRV_DIR,
								&GSftk_Config.DirectoryObject);
    if (!NT_SUCCESS(status) ) 
    {
		DebugPrint((DBG_ERROR, "sftk_CreateCtlDevice: sftk_create_dir(for %S) Failed, with status = 0x%08x \n", 
						FTD_DRV_DIR, status));
		sftk_LogEventString1( DriverObject, MSG_REPL_CREATE_OBJ_DIR_FAILED, status, 0, FTD_DRV_DIR);
        goto done;
    }

    // Create the ctl kernel device
	status = IoCreateDevice(	GSftk_Config.DriverObject,
								sizeof(SFTK_CTL_DEV_EXT),
								&driverDeviceName,
								FILE_DEVICE_UNKNOWN,    // For lack of anything better ?
								0,
								FALSE,  // Not exclusive.
								&GSftk_Config.CtrlDeviceObject );
    if (!NT_SUCCESS(status)) 
    { // failed to create a device object, leave.
        DebugPrint((DBG_ERROR, "sftk_CreateCtlDevice: IoCreateDevice(CtlDevice %S) Failed, returning status 0x%08x \n", 
							driverDeviceName.Buffer, status));
		sftk_LogEventString1( DriverObject, MSG_REPL_CREATE_CTL_DEVICE_FAILED, status, 0, driverDeviceName.Buffer);
        goto done;
    }

    // Initialize the extension for the device object.
    pCtlDevExt = (SFTK_CTL_DEV_EXT *)  GSftk_Config.CtrlDeviceObject->DeviceExtension;
	pCtlDevExt->NodeId.NodeType = NODE_TYPE_SFTK_CTL;
	pCtlDevExt->NodeId.NodeSize = sizeof(SFTK_CTL_DEV_EXT);

	GSftk_Config.CtrlDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
	GSftk_Config.CtrlDeviceObject->Flags |= DO_BUFFERED_IO;

    // create a symbolic link to kernel object.so user mode application can access this device.

    // Now create a directory in the DOS (Win32) visible name space.
	status = sftk_create_dir(	FTD_DOS_DRV_DIR,
								&GSftk_Config.DosDirectoryObject );
    if (!NT_SUCCESS(status)) 
    {
        DebugPrint((DBG_ERROR, "sftk_CreateCtlDevice: sftk_create_dir(for %S ) Failed, with status = 0x%08x \n", 
						FTD_DOS_DRV_DIR, status));
		sftk_LogEventString1( DriverObject, MSG_REPL_CREATE_SYM_LINK_CTL_OBJECT_FAILED, status, 0, FTD_DOS_DRV_DIR);
        goto done;
    }

	// Now Create Symbolic link to kernel ctrl device.
	status = IoCreateSymbolicLink( &userVisibleName, &driverDeviceName);
    if (!NT_SUCCESS(status)) 
    {
        DebugPrint((DBG_ERROR, "sftk_CreateCtlDevice: IoCreateSymbolicLink(for %S to %S) Failed, with status = 0x%08x \n", 
						userVisibleName.Buffer, driverDeviceName.Buffer, status));
		sftk_LogEventString1( DriverObject, MSG_REPL_CREATE_SYM_LINK_CTL_OBJECT_FAILED, status, 0, userVisibleName.Buffer);
        goto done;
    }

	OS_SetFlag(GSftk_Config.Flag, SFTK_CONFIG_FLAG_SYMLINK_CREATED);

    status = STATUS_SUCCESS;

done:

	if ( !NT_SUCCESS(status) )
	{ // Failed

		if(OS_IsFlagSet(GSftk_Config.Flag, SFTK_CONFIG_FLAG_SYMLINK_CREATED) )
			IoDeleteSymbolicLink(&userVisibleName);

		if(GSftk_Config.CtrlDeviceObject)
			IoDeleteDevice(GSftk_Config.CtrlDeviceObject);

		if (GSftk_Config.DirectoryObject)
			sftk_del_dir( FTD_DRV_DIR, GSftk_Config.DirectoryObject);

		if (GSftk_Config.DosDirectoryObject)
			sftk_del_dir( FTD_DOS_DRV_DIR, GSftk_Config.DosDirectoryObject);
	}
	
	return status;
} // sftk_CreateCtlDevice()


PDEVICE_EXTENSION
sftk_find_attached_deviceExt( IN ftd_dev_info_t  *In_dev_info, BOOLEAN	bGrabLock )
{
	PDEVICE_EXTENSION	pAttachedDevExt	= NULL;
	PLIST_ENTRY			plistEntry		= NULL;
	STRING              inDevString, kernelDevString;
    UNICODE_STRING      inDevUnicode, kernelDevUnicode;
	NTSTATUS			status;
	STRING              inUniqueString, kernelUniqueString;

	// Use vdevname and convert it into WCHAR
	RtlInitAnsiString(&inDevString, In_dev_info->vdevname);
	RtlInitUnicodeString(&inDevUnicode, NULL);
	
    status = RtlAnsiStringToUnicodeString(	&inDevUnicode,
											&inDevString,
											TRUE);
	if (!NT_SUCCESS(status))
	{
		DebugPrint((DBG_ERROR, "sftk_find_attached_deviceExt:: RtlAnsiStringToUnicodeString(for %s) Failed returning Error. 0x%08x!", In_dev_info->vdevname, status));
		return NULL;
	}

	if (bGrabLock == TRUE)
		OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_SHARED, TRUE, NULL);

	for( plistEntry = GSftk_Config.DevExt_List.ListEntry.Flink;
		 plistEntry != &GSftk_Config.DevExt_List.ListEntry;
		 plistEntry = plistEntry->Flink )
	{ // for :scan thru each and every Attached Dev Extensions..

		pAttachedDevExt = CONTAINING_RECORD( plistEntry, DEVICE_EXTENSION, DevExt_Link);

		// UpdateDevExtDeviceInfo(pAttachedDevExt)
		if (pAttachedDevExt->DeviceInfo.bValidName == FALSE)
		{ // Update Device Info for current Dev Ext
			UpdateDevExtDeviceInfo(pAttachedDevExt);
		}

		// check OS unique volume Id is valid and if it gets compared.
		if ( (pAttachedDevExt->DeviceInfo.bUniqueVolumeId == TRUE) 
					&& 
			 (In_dev_info->bUniqueVolumeIdValid == TRUE) ) 
		{ // has Retrieved this value from IOCTL_STORAGE_GET_DEVICE_NUMBER 
			OS_ASSERT( pAttachedDevExt->DeviceInfo.UniqueIdInfo != NULL );

			if ((In_dev_info->UniqueIdLength == pAttachedDevExt->DeviceInfo.UniqueIdInfo->UniqueIdLength)
							&& 
				(RtlCompareMemory(	In_dev_info->UniqueId, 
									pAttachedDevExt->DeviceInfo.UniqueIdInfo->UniqueId, 
									In_dev_info->UniqueIdLength) == In_dev_info->UniqueIdLength) )
			{ // Compareed, Found Attached DevExt
				goto done;
			}
		}

		// check Customize Disk Signature based unique volume Id is valid and if it gets compared.
		if ( (pAttachedDevExt->DeviceInfo.bSignatureUniqueVolumeId == TRUE) 
					&& 
			 (In_dev_info->bSignatureUniqueVolumeIdValid == TRUE) ) 
		{ // has Retrieved this value from IOCTL_STORAGE_GET_DEVICE_NUMBER
			OS_ASSERT( pAttachedDevExt->DeviceInfo.SignatureUniqueIdLength > 0 );
			
			if ((In_dev_info->SignatureUniqueIdLength == pAttachedDevExt->DeviceInfo.SignatureUniqueIdLength)
							&& 
				(RtlCompareMemory(	In_dev_info->SignatureUniqueId, 
									pAttachedDevExt->DeviceInfo.SignatureUniqueId, 
									In_dev_info->SignatureUniqueIdLength) == In_dev_info->SignatureUniqueIdLength) )
			{ // Compareed, Found Attached DevExt
				goto done;
			}
		}

		// TODO ::: Do we need this ??? Optional - If needed check DosDevice Drive Letter Symbolik link name 

		// Check \\Device\HardDisk(n)\\Partition(n) values, used mainly in Windows NT
		if (pAttachedDevExt->DeviceInfo.bStorage_device_Number == TRUE)
		{ // has Retrieved this value from IOCTL_STORAGE_GET_DEVICE_NUMBER 
			// Example: DiskPartitionName = L"\\Device\\Harddisk%d\\Partition%d"
			RtlInitUnicodeString(&kernelDevUnicode, pAttachedDevExt->DeviceInfo.DiskPartitionName);
			if ( RtlCompareUnicodeString( &kernelDevUnicode, &inDevUnicode, TRUE) == 0)
			{ // Compareed, Found Attached DevExt
				goto done;
			}
		}
		
		// check \\Device\\HarddiskVolume(n) name, 
		// TODO: This value retrieved from IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, Need Testing
		if (pAttachedDevExt->DeviceInfo.bDiskVolumeName == TRUE)
		{ // has Retrieved this value from IOCTL_MOUNTDEV_QUERY_DEVICE_NAME
			// Example : \Device\HarddiskVolume1 or \DosDevices\D or "\DosDevices\E:\FilesysD\mnt
			RtlInitUnicodeString(&kernelDevUnicode, pAttachedDevExt->DeviceInfo.DiskVolumeName);

			if ( RtlCompareUnicodeString( &kernelDevUnicode, &inDevUnicode, TRUE) == 0)
			{ // Compareed, Found Attached DevExt
				goto done;
			}
		}

		// Now Do special things to make \\Device\HardDiskVolume(nnn) string, we have nnn value stored in VolumeNumber
		if (pAttachedDevExt->DeviceInfo.bVolumeNumber == TRUE)
		{ // has Retrieved this value from IOCTL_VOLUME_QUERY_VOLUME_NUMBER
			CHAR	diskVolumeName[128];

			// DEVICE_HARDDISK_VOLUME_STRING "\\Device\\HarddiskVolume%d"
			sprintf(diskVolumeName, DEVICE_HARDDISK_VOLUME_STRING, pAttachedDevExt->DeviceInfo.VolumeNumber.VolumeNumber);

			RtlInitAnsiString(&kernelDevString, diskVolumeName);

			if ( RtlEqualString( &kernelDevString, &inDevString, TRUE) == TRUE)
			{ // Compareed, Found Attached DevExt
				goto done;
			}
		}

	} // for :scan thru each and every Attached Dev Extensions..

	pAttachedDevExt = NULL; // we did not found..

done:
	if (bGrabLock == TRUE)
		OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);

	// Free Resource
	if( inDevUnicode.Buffer)
		RtlFreeUnicodeString(&inDevUnicode);

	return pAttachedDevExt;
} // sftk_find_attached_deviceExt()

// inserts PDEVICE_EXTENSION entry in the link list
VOID 
AddDevExt_SftkConfigList( IN OUT PDEVICE_EXTENSION DevExt )
{
	PLIST_ENTRY		plistEntry	= NULL;
	PSFTK_DEV		pSftkDev	= NULL;


	// Initialize with default values following fields before we call First time UpdateDevExtDeviceInfo()
	DevExt->DeviceInfo.StorageDeviceNumber.DeviceNumber	= -1;
	DevExt->DeviceInfo.StorageDeviceNumber.PartitionNumber	= -1;

	// Now Query devicename info and update that in DevExt
	UpdateDevExtDeviceInfo( DevExt );

	// Initialize and Insert this dev Ext into Gloabl List.
	InitializeListHead(&DevExt->DevExt_Link);

	OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

	InsertTailList( &GSftk_Config.DevExt_List.ListEntry, &DevExt->DevExt_Link );
	GSftk_Config.DevExt_List.NumOfNodes ++;

	OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
} // AddDevExt_SftkConfigList()

// removes PDEVICE_EXTENSION entry from the link list
VOID 
RemoveDevExt_SftkConfigList( PDEVICE_EXTENSION DevExt )
{
	OS_ASSERT(DevExt->Sftk_dev == NULL);
	// Initilize 
	OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	RemoveEntryList(&DevExt->DevExt_Link);
	GSftk_Config.DevExt_List.NumOfNodes --;

	OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);

} // RemoveDevExt_SftkConfigList() 

// Check previously configured any devices was removed if yes than check
// is this new devext is the one for missing device if yes than update 
// sftk_dev for this devext
VOID 
sftk_reattach_devExt_to_SftkDev()
{
	PLIST_ENTRY			plistEntry, pNextlistEntry;
	ftd_dev_info_t		in_dev_info;
	PSFTK_DEV			pSftkDev;
	PDEVICE_EXTENSION	pAttachedDevExt;

	OS_ZeroMemory( &in_dev_info, sizeof(in_dev_info));

	OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

	for( plistEntry = GSftk_Config.SftkDev_PnpRemovedList.ListEntry.Flink;
		 plistEntry != &GSftk_Config.SftkDev_PnpRemovedList.ListEntry;)
	{ // for :scan thru each and every missing device's sftk_Dev 
		pSftkDev = CONTAINING_RECORD( plistEntry, SFTK_DEV, SftkDev_PnpRemovedLink);

		OS_ASSERT(pSftkDev->DevExtension == NULL);
		OS_ASSERT(OS_IsFlagSet(pSftkDev->Flags, SFTK_DEV_FLAG_SRC_DEVICE_ONLINE) == FALSE);

		strcpy( in_dev_info.vdevname, pSftkDev->Vdevname);	
		strcpy( in_dev_info.devname, pSftkDev->Devname);

		pAttachedDevExt = sftk_find_attached_deviceExt( &in_dev_info, FALSE );

		if (pAttachedDevExt != NULL) 
		{ // we found removed device from sftk_dev so just re-attach it now.
			OS_ASSERT(pAttachedDevExt->Sftk_dev == NULL);

			pNextlistEntry = plistEntry->Flink;
			RemoveEntryList(plistEntry); // Remove Device from the list
			GSftk_Config.SftkDev_PnpRemovedList.NumOfNodes --;
			plistEntry = pNextlistEntry;

			pSftkDev->DevExtension		= pAttachedDevExt;
			OS_SetFlag( pSftkDev->Flags, SFTK_DEV_FLAG_SRC_DEVICE_ONLINE);	// since devext is valid value...
			pAttachedDevExt->Sftk_dev	= pSftkDev;

			// signal event since missing device is found and is online for sftk_Dev to use.
			KeSetEvent( &pSftkDev->PnpEventDiskArrived, 0, FALSE );
		}
		else
			plistEntry = plistEntry->Flink;
	} // for :scan thru each and every missing device's sftk_Dev 
	OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
} // sftk_reattach_devExt_to_SftkDev()

// Check if Device was existed & configured with SFTK_DEV if yes than 
// remove it from there too.
VOID 
sftk_remove_devExt_from_SftkDev( PDEVICE_EXTENSION DevExt )
{
	if (DevExt->Sftk_dev != NULL)
	{ // SFTK_DEV was attached to this devext
		PSFTK_DEV pSftk_Dev	= DevExt->Sftk_dev;

		OS_ClearFlag( pSftk_Dev->Flags, SFTK_DEV_FLAG_SRC_DEVICE_ONLINE);	// since devext is valid value...
		pSftk_Dev->DevExtension = NULL;
		KeClearEvent( &pSftk_Dev->PnpEventDiskArrived );

		// Add this in Global Removed device list
		OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

		InsertTailList( &GSftk_Config.SftkDev_PnpRemovedList.ListEntry, &pSftk_Dev->SftkDev_PnpRemovedLink );
		GSftk_Config.SftkDev_PnpRemovedList.NumOfNodes ++;

		OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
	}

	DevExt->Sftk_dev = NULL;
} // sftk_remove_devExt_from_SftkDev() 

// look up for PDEVICE_EXTENSION in the link list based on DevExt->VolumeNumber.VolumeNumber
PDEVICE_EXTENSION
LookupVolNum_DevExt_SftkConfigList( IN ULONG   PartVolumeNumber )
{
	PLIST_ENTRY			listEntry;
	PDEVICE_EXTENSION	DevExt;

	OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_SHARED, TRUE, NULL);

	for(listEntry = GSftk_Config.DevExt_List.ListEntry.Flink; 
		listEntry != &GSftk_Config.DevExt_List.ListEntry;
		listEntry = listEntry->Flink)
	{ // for : scan each and every entry 
		DevExt = CONTAINING_RECORD( listEntry, DEVICE_EXTENSION, DevExt_Link);

		// Compare Volumenumber 
		if (DevExt->DeviceInfo.bVolumeNumber == FALSE)
		{ // current DevExt does not have VolNumber initialized, so try to initialize it.
			UpdateDevExtDeviceInfo(DevExt);
		}

		if (DevExt->DeviceInfo.bVolumeNumber == TRUE)
		{
			if (DevExt->DeviceInfo.VolumeNumber.VolumeNumber == PartVolumeNumber)
			{
				OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
				return DevExt; // Found
			}
		}
	} // for : scan each and every entry 

	OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);

	return NULL;
} // LookupVolNum_DevExt_SftkConfigList()

// look up for PDEVICE_EXTENSION in the link list based on Disk/Partition Number
PDEVICE_EXTENSION
LookupDiskPartNum_DevExt_SftkConfigList( IN ULONG  DiskDeviceNumber, IN ULONG  PartitionNumber )
{
	PLIST_ENTRY			listEntry;
	PDEVICE_EXTENSION	DevExt;

	OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_SHARED, TRUE, NULL);

	for(listEntry = GSftk_Config.DevExt_List.ListEntry.Flink; 
		listEntry != &GSftk_Config.DevExt_List.ListEntry;
		listEntry = listEntry->Flink)
	{ // for : scan each and every entry 
		DevExt = CONTAINING_RECORD( listEntry, DEVICE_EXTENSION, DevExt_Link);

		// Compare Volumenumer 
		if (DevExt->DeviceInfo.bStorage_device_Number == FALSE)
		{ // current DevExt does not have Disk/Partition Number initialized, so try to initialize it.
			UpdateDevExtDeviceInfo(DevExt);
		}

		if (DevExt->DeviceInfo.bStorage_device_Number == TRUE)
		{
			if ( (DevExt->DeviceInfo.StorageDeviceNumber.DeviceNumber == DiskDeviceNumber) &&
				 (DevExt->DeviceInfo.StorageDeviceNumber.PartitionNumber == PartitionNumber) )
			{
				OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
				return DevExt; // Found
			}
		}
	} // for : scan each and every entry 

	OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
	return NULL;
} // LookupDiskPartNum_DevExt_SftkConfigList()

// updates the dev ext with the Device/Partition OR Volume Number
NTSTATUS 
UpdateDevExtDeviceInfo( PDEVICE_EXTENSION DevExt )
{ 
	NTSTATUS                status;
    IO_STATUS_BLOCK         ioStatus;
    KEVENT                  event;
    PIRP                    irp;

	// IOCTL_STORAGE_GET_DEVICE_NUMBER to retrieve this info, Example : \\Device\HardDisk(n)\\Partition(n)
	if (DevExt->DeviceInfo.bStorage_device_Number == FALSE)	
	{
		OS_InitializeEvent(&event, NotificationEvent, FALSE);
    
		// Request for the device number
		irp = IoBuildDeviceIoControlRequest(
						IOCTL_STORAGE_GET_DEVICE_NUMBER,
						DevExt->TargetDeviceObject,
						NULL,
						0,
						&DevExt->DeviceInfo.StorageDeviceNumber,
						sizeof(DevExt->DeviceInfo.StorageDeviceNumber),
						FALSE,
						&event,
						&ioStatus);
		if (!irp) 
		{
			DebugPrint( ( DBG_ERROR, "UpdateDevExtDeviceInfo(): IoBuildDeviceIoControlRequest() failed to allocate IRP for IOCTL_STORAGE_GET_DEVICE_NUMBER\n"));
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		status = IoCallDriver(DevExt->TargetDeviceObject, irp);

		if (status == STATUS_PENDING) 
		{
			OS_KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
			status = ioStatus.Status;
		}

		if (NT_SUCCESS(status)) 
		{	// succeeded
			// Create device name for each partition
			swprintf(	DevExt->DeviceInfo.DiskPartitionName,	
						DEVICE_HARDDISK_PARTITION,	// L"\\Device\\Harddisk%d\\Partition%d"
						DevExt->DeviceInfo.StorageDeviceNumber.DeviceNumber, 
						DevExt->DeviceInfo.StorageDeviceNumber.PartitionNumber);
			// Set default name for physical disk
			OS_RtlCopyMemory(	&(DevExt->DeviceInfo.StorageManagerName[0]),
								L"PhysDisk",
								8 * sizeof(WCHAR));
			DevExt->DeviceInfo.bStorage_device_Number = TRUE;
		} // succeeded
	} // if (DevExt->DeviceInfo.bStorage_device_Number == FALSE)

	// IOCTL_VOLUME_QUERY_VOLUME_NUMBER to retrieve HarddiskVolume number and its volumename like logdisk, etc..
	// if disk is LogiDisk type then we get following info successfully.
	if (DevExt->DeviceInfo.bVolumeNumber == FALSE)	
	{ // Now, get the VOLUME_NUMBER information

        OS_ZeroMemory(&DevExt->DeviceInfo.VolumeNumber, sizeof(VOLUME_NUMBER));

        OS_InitializeEvent(&event, NotificationEvent, FALSE);

        irp = IoBuildDeviceIoControlRequest( IOCTL_VOLUME_QUERY_VOLUME_NUMBER,
											 DevExt->TargetDeviceObject, NULL, 0,
											 &DevExt->DeviceInfo.VolumeNumber,
											 sizeof(VOLUME_NUMBER),
											 FALSE, &event, &ioStatus);
        if (!irp) 
		{
			DebugPrint((DBG_ERROR, "UpdateDevExtDeviceInfo() failed to allocate IRP for IOCTL_VOLUME_QUERY_VOLUME_NUMBER\n")); 
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = IoCallDriver(DevExt->TargetDeviceObject, irp);
        if (status == STATUS_PENDING) 
		{
            OS_KeWaitForSingleObject(&event, Executive,
                                  KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (!NT_SUCCESS(status) || (DevExt->DeviceInfo.VolumeNumber.VolumeManagerName[0] == (WCHAR) UNICODE_NULL) ) 
		{
            OS_RtlCopyMemory(	&DevExt->DeviceInfo.StorageManagerName[0],
								L"LogiDisk",
								8 * sizeof(WCHAR));
        }
        else 
		{
            OS_RtlCopyMemory(	&DevExt->DeviceInfo.StorageManagerName[0],
								&DevExt->DeviceInfo.VolumeNumber.VolumeManagerName[0],
								8 * sizeof(WCHAR));
        }

		if (NT_SUCCESS(status))
			DevExt->DeviceInfo.bVolumeNumber = TRUE;

    } // if (DevExt->DeviceInfo.bVolumeNumber == FALSE)

	// IOCTL_MOUNTDEV_QUERY_DEVICE_NAME used to retrieve \Device\HarddiskVolume1 into DiskVolumeName...
	if (DevExt->DeviceInfo.bDiskVolumeName == FALSE)
	{ // Get DiskVolume name information 
        PMOUNTDEV_NAME  output;
		ULONG			outputSize = sizeof(MOUNTDEV_NAME);

		// First Retrieve Totla size of DiskVolume name
        output = OS_AllocMemory(PagedPool, outputSize);
        if (!output) 
		{
			DebugPrint((DBG_ERROR, "UpdateDevExtDeviceInfo() failed to allocate memory for PMOUNTDEV_NAME output size %d \n",outputSize)); 
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        OS_InitializeEvent(&event, NotificationEvent, FALSE);
        irp = IoBuildDeviceIoControlRequest(	IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
												DevExt->TargetDeviceObject, NULL, 0,
												output, outputSize, FALSE, &event, &ioStatus);
        if (!irp) 
		{
            OS_FreeMemory(output);
			DebugPrint((DBG_ERROR, "UpdateDevExtDeviceInfo() failed to allocate IRP for IOCTL_MOUNTDEV_QUERY_DEVICE_NAME\n")); 
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = IoCallDriver(DevExt->TargetDeviceObject, irp);
        if (status == STATUS_PENDING) 
		{
            OS_KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (status == STATUS_BUFFER_OVERFLOW) 
		{ // now retrieve actual DiskVolume Name with passing proper asking size.
            outputSize = sizeof(MOUNTDEV_NAME) + output->NameLength;
            OS_FreeMemory(output);

            output = OS_AllocMemory(PagedPool, outputSize);
            if (!output) 
			{
				DebugPrint((DBG_ERROR, "UpdateDevExtDeviceInfo() failed to allocate memory for output size %d\n",outputSize));
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            OS_InitializeEvent(&event, NotificationEvent, FALSE);
            irp = IoBuildDeviceIoControlRequest(	IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
													DevExt->TargetDeviceObject, NULL, 0,
													output, outputSize, FALSE, &event, &ioStatus);

            if (!irp) 
			{
				DebugPrint((DBG_ERROR, "UpdateDevExtDeviceInfo() failed to allocate IRP for IOCTL_MOUNTDEV_QUERY_DEVICE_NAME\n")); 
                OS_FreeMemory(output);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            status = IoCallDriver(DevExt->TargetDeviceObject, irp);
            if (status == STATUS_PENDING) 
			{
                OS_KeWaitForSingleObject(&event,
										 Executive,
										 KernelMode,
										 FALSE,
										 NULL);
                status = ioStatus.Status;
            }
        } // if (status == STATUS_BUFFER_OVERFLOW) 

        if (!NT_SUCCESS(status)) 
		{ // failed
            OS_FreeMemory(output);
        } 
		else 
		{ //succeeded

			// DiskVolumeName will have a value like \Device\HardDiskVolume(nnn) where nnn stands for volume number...
			OS_ZeroMemory(	DevExt->DeviceInfo.DiskVolumeName, sizeof(DevExt->DeviceInfo.DiskVolumeName) );

			OS_RtlCopyMemory(	DevExt->DeviceInfo.DiskVolumeName,
								output->Name,
								output->NameLength);
			OS_FreeMemory(output);

			DevExt->DeviceInfo.bDiskVolumeName = TRUE;
		}
	} // if (DevExt->bDiskVolumeName == FALSE)

	// following fields are supported only >= Win2k OS 

	// IOCTL_MOUNTDEV_QUERY_UNIQUE_ID used to retrieve Volume/Disk Unique Persistence ID, 
	// It Contains the unique volume ID. The format for unique volume names is "\??\Volume{GUID}\", 
	// where GUID is a globally unique identifier that identifies the volume.
	if (DevExt->DeviceInfo.bUniqueVolumeId == FALSE)	
	{
		PMOUNTDEV_UNIQUE_ID  uniqueIdInfo;
		ULONG				 outputSize = sizeof(MOUNTDEV_UNIQUE_ID);

		DevExt->DeviceInfo.UniqueIdInfo = NULL;

		// First Retrieve Total size of Unique Id name 
        uniqueIdInfo = OS_AllocMemory(NonPagedPool, outputSize);
        if (!uniqueIdInfo) 
		{
			DebugPrint((DBG_ERROR, "UpdateDevExtDeviceInfo() failed to allocate memory PMOUNTDEV_UNIQUE_ID for uniqueIdInfo size %d \n",outputSize)); 
            return STATUS_INSUFFICIENT_RESOURCES;
        }

		OS_ZeroMemory(	uniqueIdInfo, outputSize );
        OS_InitializeEvent(&event, NotificationEvent, FALSE);
        irp = IoBuildDeviceIoControlRequest(	IOCTL_MOUNTDEV_QUERY_UNIQUE_ID,
												DevExt->TargetDeviceObject, uniqueIdInfo, outputSize,
												uniqueIdInfo, outputSize, FALSE, &event, &ioStatus);
        if (!irp) 
		{
            OS_FreeMemory(uniqueIdInfo);
			DebugPrint((DBG_ERROR, "UpdateDevExtDeviceInfo() failed to allocate IRP for IOCTL_MOUNTDEV_QUERY_UNIQUE_ID\n")); 
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = IoCallDriver(DevExt->TargetDeviceObject, irp);
        if (status == STATUS_PENDING) 
		{
            OS_KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (status == STATUS_BUFFER_OVERFLOW) 
		{ // now retrieve actual DiskVolume Name with passing proper asking size.
            outputSize = sizeof(MOUNTDEV_UNIQUE_ID) + uniqueIdInfo->UniqueIdLength;
            OS_FreeMemory(uniqueIdInfo);

            uniqueIdInfo = OS_AllocMemory(NonPagedPool, outputSize);
            if (!uniqueIdInfo) 
			{
				DebugPrint((DBG_ERROR, "UpdateDevExtDeviceInfo() failed to allocate Final MOUNTDEV_UNIQUE_ID memory for uniqueIdInfo size %d\n",outputSize));
                return STATUS_INSUFFICIENT_RESOURCES;
            }

			OS_ZeroMemory(	uniqueIdInfo, outputSize );
            OS_InitializeEvent(&event, NotificationEvent, FALSE);
            irp = IoBuildDeviceIoControlRequest(	IOCTL_MOUNTDEV_QUERY_UNIQUE_ID,
													DevExt->TargetDeviceObject, uniqueIdInfo, outputSize,
													uniqueIdInfo, outputSize, FALSE, &event, &ioStatus);

            if (!irp) 
			{
				DebugPrint((DBG_ERROR, "UpdateDevExtDeviceInfo() failed to allocate IRP for IOCTL_MOUNTDEV_QUERY_UNIQUE_ID\n")); 
                OS_FreeMemory(uniqueIdInfo);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            status = IoCallDriver(DevExt->TargetDeviceObject, irp);
            if (status == STATUS_PENDING) 
			{
                OS_KeWaitForSingleObject(&event,
										 Executive,
										 KernelMode,
										 FALSE,
										 NULL);
                status = ioStatus.Status;
            }
        } // if (status == STATUS_BUFFER_OVERFLOW) 

        if (!NT_SUCCESS(status)) 
		{ // failed
            OS_FreeMemory(uniqueIdInfo);
        } 
		else 
		{ //succeeded
			DevExt->DeviceInfo.UniqueIdInfo = uniqueIdInfo;
			DevExt->DeviceInfo.bUniqueVolumeId = TRUE;
		}
	} // if (DevExt->DeviceInfo.bUniqueVolumeId == FALSE)	

	// following fields are supported only >= Win2k OS 
	// IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME is Optional returns Drive letter (if Drive Letter is persistent across boot) or
	// suggest Drive Letter Dos Sym,bolic Name.
	if (DevExt->DeviceInfo.bSuggestedDriveLetter == FALSE)	
	{
		PMOUNTDEV_SUGGESTED_LINK_NAME	suggestedDriveLinkName;
		ULONG							outputSize = sizeof(MOUNTDEV_SUGGESTED_LINK_NAME);

		DevExt->DeviceInfo.SuggestedDriveLinkName = NULL;

		// First Retrieve Total size of Unique Id name
        suggestedDriveLinkName = OS_AllocMemory(NonPagedPool, outputSize);
        if (!suggestedDriveLinkName) 
		{
			DebugPrint((DBG_ERROR, "UpdateDevExtDeviceInfo() failed to allocate memory PMOUNTDEV_SUGGESTED_LINK_NAME for suggestedDriveLinkName size %d \n",outputSize)); 
            return STATUS_INSUFFICIENT_RESOURCES;
        }

		OS_ZeroMemory(	suggestedDriveLinkName, outputSize );
        OS_InitializeEvent(&event, NotificationEvent, FALSE);
        irp = IoBuildDeviceIoControlRequest(	IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME,
												DevExt->TargetDeviceObject, suggestedDriveLinkName, outputSize,
												suggestedDriveLinkName, outputSize, FALSE, &event, &ioStatus);
        if (!irp) 
		{
            OS_FreeMemory(suggestedDriveLinkName);
			DebugPrint((DBG_ERROR, "UpdateDevExtDeviceInfo() failed to allocate IRP for IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME\n")); 
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = IoCallDriver(DevExt->TargetDeviceObject, irp);
        if (status == STATUS_PENDING) 
		{
            OS_KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (status == STATUS_BUFFER_OVERFLOW) 
		{ // now retrieve actual DiskVolume Name with passing proper asking size.
            outputSize = sizeof(MOUNTDEV_SUGGESTED_LINK_NAME) + suggestedDriveLinkName->NameLength;
            OS_FreeMemory(suggestedDriveLinkName);

            suggestedDriveLinkName = OS_AllocMemory(NonPagedPool, outputSize);
            if (!suggestedDriveLinkName) 
			{
				DebugPrint((DBG_ERROR, "UpdateDevExtDeviceInfo() failed to allocate Final PMOUNTDEV_SUGGESTED_LINK_NAME memory for suggestedDriveLinkName size %d\n",outputSize));
                return STATUS_INSUFFICIENT_RESOURCES;
            }

			OS_ZeroMemory(	suggestedDriveLinkName, outputSize );
            OS_InitializeEvent(&event, NotificationEvent, FALSE);
            irp = IoBuildDeviceIoControlRequest(	IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME,
													DevExt->TargetDeviceObject, suggestedDriveLinkName, outputSize,
													suggestedDriveLinkName, outputSize, FALSE, &event, &ioStatus);

            if (!irp) 
			{
				DebugPrint((DBG_ERROR, "UpdateDevExtDeviceInfo() failed to allocate IRP for IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME\n")); 
                OS_FreeMemory(suggestedDriveLinkName);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            status = IoCallDriver(DevExt->TargetDeviceObject, irp);
            if (status == STATUS_PENDING) 
			{
                OS_KeWaitForSingleObject(&event,
										 Executive,
										 KernelMode,
										 FALSE,
										 NULL);
                status = ioStatus.Status;
            }
        } // if (status == STATUS_BUFFER_OVERFLOW) 

        if (!NT_SUCCESS(status)) 
		{ // failed
            OS_FreeMemory(suggestedDriveLinkName);
        } 
		else 
		{ //succeeded
			DevExt->DeviceInfo.SuggestedDriveLinkName = suggestedDriveLinkName;
			DevExt->DeviceInfo.bSuggestedDriveLetter = TRUE;
		}
	} // if (DevExt->DeviceInfo.bSuggestedDriveLetter == FALSE)	

	// Generate Customize Unique Signature Volume id , this specially is used for Windows NT 4.0 systems
	sftk_GenerateSignatureGuid(DevExt);

	if (	(DevExt->DeviceInfo.bStorage_device_Number == TRUE) && 
			(DevExt->DeviceInfo.bVolumeNumber == TRUE) &&
			(DevExt->DeviceInfo.bDiskVolumeName == TRUE) && 
			(DevExt->DeviceInfo.bUniqueVolumeId == TRUE) &&
			(DevExt->DeviceInfo.bSignatureUniqueVolumeId == TRUE) )
			// && (DevExt->DeviceInfo.bSuggestedDriveLetter == TRUE) )
	{
		DevExt->DeviceInfo.bValidName = TRUE;
	}

    return status;
} // UpdateDevExtDeviceInfo()


// This API will create Signature based unique ID. This API supports Disk/Disk Partition/ Disk Dynamic Spanned Partition
NTSTATUS	
sftk_GenerateSignatureGuid( PDEVICE_EXTENSION DevExt )
{
	NTSTATUS					status;
	UNICODE_STRING				ntUnicodeString;
    PFILE_OBJECT				fileObject	= NULL;
	PDEVICE_OBJECT				pDiskDevice	= NULL;
	PDRIVE_LAYOUT_INFORMATION	pDriveLayOut= NULL;
	IO_STATUS_BLOCK				ioStatus;
    KEVENT						event;
    PIRP						irp;
	BOOLEAN						bRawDisk	= FALSE;
	BOOLEAN						bIsVolume	= FALSE;

	if (DevExt->DeviceInfo.bSignatureUniqueVolumeId == TRUE)
	{
		OS_ASSERT(DevExt->DeviceInfo.SignatureUniqueIdLength > 0);
		OS_ASSERT(DevExt->DeviceInfo.pRawDiskDevice != NULL);

		return STATUS_SUCCESS;	// we already have done this
	}

	DevExt->DeviceInfo.pRawDiskDevice = NULL;

	// Retrieve Signature + Partition Starting Offset + Size of Partition.
	// If its Raw disk than Starting Offset may be  0 and size of partition may be complete size of disk.
	if ( DevExt->DeviceInfo.bStorage_device_Number == TRUE ) 
	{ // we have valid disk and Partition number 
		if ( (DevExt->DeviceInfo.StorageDeviceNumber.DeviceNumber	!= -1) &&
			 (DevExt->DeviceInfo.StorageDeviceNumber.PartitionNumber == 0) )
		{	// its RAW Disk device 
			// Current Device is RAW Device.so do not need to retrieve Pointer for it
			DevExt->DeviceInfo.StartingOffset.QuadPart  = 0; // its RAW Disk 
			DevExt->DeviceInfo.PartitionLength.QuadPart = 0; // its RAW Disk 
			bRawDisk = TRUE;

			// pDiskDevice = DevExt->TargetDeviceObject;	
			// Don't store TargetDeviceObject, use DeviceObject since we need to compare later about this in IOCTL_DISK_SET_DRIVE_LAYOUT
			pDiskDevice = DevExt->DeviceObject;	
		} 
		else
		{	// current device is either Partition\Dynamic partition\ or FTDisk object 
			// so retrieve its Disk Device pointer
			OS_ASSERT(DevExt->DeviceInfo.StorageDeviceNumber.DeviceNumber != -1);
			OS_ASSERT(DevExt->DeviceInfo.StorageDeviceNumber.PartitionNumber != -1);
			OS_ASSERT(DevExt->DeviceInfo.StorageDeviceNumber.PartitionNumber > 0);
			// since we have Valid Disk number and Partition number, Just retrieve directly RAW Disk device from it 
		}
	} // we have valid disk and Partition number 
	else
	{ // else get valid disk number 
		// OS_ASSERT(DevExt->DeviceInfo.bStorage_device_Number == FALSE );
		// - Get Volume Or partition Raw Disk number, if its Dynamic or FT Partition than we use 
		// first Disk of partition for signature.
		status = sftk_GetVolumeDiskExtents( DevExt );

		if(!NT_SUCCESS(status))
		{ // Failed 
			DebugPrint((DBG_ERROR, "sftk_GenerateSignatureGuid: sftk_GetVolumeDiskExtents() Failed DevExt 0x%08x with 0x%08x !!\n", DevExt, status)); 
			// TODO can we do anything else don't think so... so just return error
			status = STATUS_UNSUCCESSFUL;
			goto done;
		}
		bIsVolume = TRUE;
	} // else get valid disk number 

	// - Retrieve RAW Disk Device (or \Device\HardDisk(n)\Partition0) Object pointer
	if (pDiskDevice == NULL)
	{ // Get RAW Disk device object.
		swprintf(	DevExt->DeviceInfo.DiskPartitionName,	
					DEVICE_HARDDISK_PARTITION,	// L"\\Device\\Harddisk%d\\Partition%d"
					DevExt->DeviceInfo.StorageDeviceNumber.DeviceNumber, 
					0);
        
		RtlInitUnicodeString( &ntUnicodeString, DevExt->DeviceInfo.DiskPartitionName);
        status = IoGetDeviceObjectPointer(&ntUnicodeString,
                                          FILE_READ_ATTRIBUTES,
                                          &fileObject,
                                          &pDiskDevice);
		if(!NT_SUCCESS(status))
		{ // Failed 
			DebugPrint((DBG_ERROR, "sftk_GenerateSignatureGuid: IoGetDeviceObjectPointer(%S) Failed for DevExt 0x%08x with error 0x%08x !!\n",
												DevExt->DeviceInfo.DiskPartitionName, DevExt, status)); 
			goto done;
		}

		if(fileObject)
			ObDereferenceObject(fileObject);
		// No Need to Zero out DiskPartitionName since we always check bStorage_device_Number == TRUE before using this values...
		// OS_ZeroMemory( DevExt->DeviceInfo.DiskPartitionName, sizeof(DevExt->DeviceInfo.DiskPartitionName)) ;
	} // Get RAW Disk device object

	OS_ASSERT(pDiskDevice != NULL);
	DevExt->DeviceInfo.pRawDiskDevice = pDiskDevice;	// store this for later use if needed.
		
	// - Retrieve Signature number from RAW Disk Device
	// - Retrieve Partition starting Offset and Size in bytes.
	pDriveLayOut = OS_AllocMemory(NonPagedPool, 8192);

	if(pDriveLayOut == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "sftk_GenerateSignatureGuid() failed to allocate memory for PDRIVE_LAYOUT_INFORMATION size %d Error 0x%08x \n",8192, status)); 
		goto done;
	}

	OS_ZeroMemory( pDriveLayOut, 8192);

	OS_InitializeEvent(&event, NotificationEvent, FALSE);
	irp = IoBuildDeviceIoControlRequest(	IOCTL_DISK_GET_DRIVE_LAYOUT,
											pDiskDevice, NULL, 0,
											pDriveLayOut, 8192, FALSE, &event, &ioStatus);
	if (!irp) 
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "sftk_GenerateSignatureGuid() failed to allocate IRP for IOCTL_DISK_GET_DRIVE_LAYOUT , error 0x%08x\n", status)); 
		goto done;
	}

	status = IoCallDriver( pDiskDevice, irp );
	if (status == STATUS_PENDING) 
	{
       OS_KeWaitForSingleObject(&event,Executive,KernelMode,FALSE,NULL);
       status = ioStatus.Status;
	}
	if (!NT_SUCCESS(status))
	{ // failed
		DebugPrint((DBG_ERROR, "sftk_GenerateSignatureGuid() IoCallDriver(Device 0x%08x ) for IOCTL_DISK_GET_DRIVE_LAYOUT failed. error 0x%08x\n", pDiskDevice, status)); 
		goto done;
	}

	// store signature
	DevExt->DeviceInfo.Signature = pDriveLayOut->Signature;	// store signature

	// - Retrieve Partition starting Offset and Size in bytes.
	if ( (bRawDisk == FALSE) && (bIsVolume == FALSE) )
	{ // Its basic Partition, Retrieve Partition starting Offset and Size in bytes.
		PPARTITION_INFORMATION pPartitionInfo = (PPARTITION_INFORMATION) pDriveLayOut;

		OS_ZeroMemory( pPartitionInfo, 8192);

		OS_InitializeEvent(&event, NotificationEvent, FALSE);
		irp = IoBuildDeviceIoControlRequest(	IOCTL_DISK_GET_PARTITION_INFO,
												DevExt->TargetDeviceObject, NULL, 0,
												pPartitionInfo, 8192, FALSE, &event, &ioStatus);
		if (!irp) 
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			DebugPrint((DBG_ERROR, "sftk_GenerateSignatureGuid() failed to allocate IRP for IOCTL_DISK_GET_PARTITION_INFO , error 0x%08x\n", status)); 
			goto done;
		}

		status = IoCallDriver( DevExt->TargetDeviceObject, irp );
		if (status == STATUS_PENDING) 
		{
		   OS_KeWaitForSingleObject(&event,Executive,KernelMode,FALSE,NULL);
		   status = ioStatus.Status;
		}
		if (!NT_SUCCESS(status))
		{ // failed
			DebugPrint((DBG_ERROR, "sftk_GenerateSignatureGuid() IoCallDriver(Device 0x%08x ) for IOCTL_DISK_GET_PARTITION_INFO failed. error 0x%08x\n", DevExt->TargetDeviceObject, status)); 
			goto done;
		}

		DevExt->DeviceInfo.StartingOffset	= pPartitionInfo->StartingOffset;
		DevExt->DeviceInfo.PartitionLength	= pPartitionInfo->PartitionLength;
	} // Its basic Partition, Retrieve Partition starting Offset and Size in bytes.

	if (DevExt->DeviceInfo.PartitionLength.QuadPart <= 0)
	{
		GET_LENGTH_INFORMATION	getLength;

		OS_ZeroMemory( &getLength, sizeof(getLength));

		OS_InitializeEvent(&event, NotificationEvent, FALSE);
		irp = IoBuildDeviceIoControlRequest(	IOCTL_DISK_GET_LENGTH_INFO, // IOCTL_DISK_GET_PARTITION_INFO,
												DevExt->TargetDeviceObject, &getLength, sizeof(getLength), 
												&getLength, sizeof(getLength), FALSE, &event, &ioStatus);
		if (!irp) 
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			DebugPrint((DBG_ERROR, "sftk_GenerateSignatureGuid() failed to allocate IRP for IOCTL_DISK_GET_PARTITION_INFO, error 0x%08x\n", status)); 
			// goto done;
		}
		else
		{ // successed
			status = IoCallDriver( DevExt->TargetDeviceObject, irp );
			if (status == STATUS_PENDING) 
			{
			   OS_KeWaitForSingleObject(&event,Executive,KernelMode,FALSE,NULL);
			   status = ioStatus.Status;
			}

			if (!NT_SUCCESS(status))
			{ // failed
				DebugPrint((DBG_ERROR, "sftk_GenerateSignatureGuid() IoCallDriver(Device 0x%08x ) for IOCTL_DISK_GET_PARTITION_INFO failed. error 0x%08x\n", DevExt->TargetDeviceObject, status)); 
				// goto done;
			}
			else
			{ // success
				DevExt->DeviceInfo.PartitionLength.QuadPart = getLength.Length.QuadPart;
			}
		}
	}

	// - Genarate & store Unique Signature based Id - 
	sprintf(	DevExt->DeviceInfo.SignatureUniqueId,	
				SIGNATURE_UNIQUE_ID,	// "volume(%08x-%016x-%016x)"	// total 50 bytes
				DevExt->DeviceInfo.Signature, 
				DevExt->DeviceInfo.StartingOffset.QuadPart,
				DevExt->DeviceInfo.PartitionLength.QuadPart);

	DevExt->DeviceInfo.SignatureUniqueIdLength = (USHORT) strlen( DevExt->DeviceInfo.SignatureUniqueId );
	DevExt->DeviceInfo.bSignatureUniqueVolumeId = TRUE;	// we genareted signature unique id successfuly
	status = STATUS_SUCCESS;

done:
	if (pDriveLayOut)
		OS_FreeMemory(pDriveLayOut);

	return status;
} // sftk_GenerateSignatureGuid()

//
//
NTSTATUS 
sftk_GetVolumeDiskExtents( PDEVICE_EXTENSION DevExt)
{
	NTSTATUS				status				= STATUS_SUCCESS;
	PVOLUME_DISK_EXTENTS	pVolumeDiskExtents	= NULL;
	ULONG					outputSize			= sizeof(VOLUME_DISK_EXTENTS);
	IO_STATUS_BLOCK         ioStatus;
    KEVENT                  event;
    PIRP                    irp;

	pVolumeDiskExtents= OS_AllocMemory(NonPagedPool, outputSize);

	if(pVolumeDiskExtents==NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "sftk_GetVolumeDiskExtents() failed to allocate memory for PVOLUME_DISK_EXTENTS size %d Error 0x%08x \n",outputSize, status)); 
		goto done;
	}

	OS_ZeroMemory( pVolumeDiskExtents, outputSize);

	OS_InitializeEvent(&event, NotificationEvent, FALSE);
	irp = IoBuildDeviceIoControlRequest(	IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
											DevExt->TargetDeviceObject, NULL, 0,
											pVolumeDiskExtents, outputSize, FALSE, &event, &ioStatus);
	if (!irp) 
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		DebugPrint((DBG_ERROR, "sftk_GetVolumeDiskExtents() failed to allocate IRP for IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, error 0x%08x\n", status)); 
		goto done;
	}

	status = IoCallDriver( DevExt->TargetDeviceObject, irp );
	if (status == STATUS_PENDING) 
	{
       OS_KeWaitForSingleObject(&event,Executive,KernelMode,FALSE,NULL);
       status = ioStatus.Status;
	}

	if(status==STATUS_BUFFER_OVERFLOW)
	{
		outputSize= sizeof(VOLUME_DISK_EXTENTS) + pVolumeDiskExtents->NumberOfDiskExtents*sizeof(DISK_EXTENT);

		OS_FreeMemory(pVolumeDiskExtents);

		pVolumeDiskExtents= OS_AllocMemory(NonPagedPool, outputSize);

		if(pVolumeDiskExtents==NULL)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			DebugPrint((DBG_ERROR, "sftk_GetVolumeDiskExtents() failed to allocate memory for PVOLUME_DISK_EXTENTS size %d Error 0x%08x \n",outputSize, status)); 
			goto done;
		}

		OS_ZeroMemory( pVolumeDiskExtents, outputSize);

		OS_InitializeEvent(&event, NotificationEvent, FALSE);
		irp = IoBuildDeviceIoControlRequest(	IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
												DevExt->TargetDeviceObject, NULL, 0,
												pVolumeDiskExtents, outputSize, FALSE, &event, &ioStatus);
		if (!irp) 
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			DebugPrint((DBG_ERROR, "sftk_GetVolumeDiskExtents() failed to allocate IRP for IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, error 0x%08x\n", status)); 
			goto done;
		}

		status = IoCallDriver( DevExt->TargetDeviceObject, irp );
		if (status == STATUS_PENDING) 
		{
		   OS_KeWaitForSingleObject(&event,Executive,KernelMode,FALSE,NULL);
		   status = ioStatus.Status;
		}
	} // if(status==STATUS_BUFFER_OVERFLOW) 

	if(NT_SUCCESS(status))
	{
		if( (pVolumeDiskExtents->NumberOfDiskExtents > 0) &&
			(wcsncmp(DevExt->DeviceInfo.StorageManagerName, L"FTDISK", 6) ==0) )
		{ // current device is FTDISK !!!
			DebugPrint((DBG_ERROR, "sftk_GetVolumeDiskExtents() DevExt 0x%08x is FTDISK Device !!\n", DevExt)); 
			DevExt->DeviceInfo.IsVolumeFtVolume = TRUE;
		}

		DevExt->DeviceInfo.StorageDeviceNumber.DeviceNumber = pVolumeDiskExtents->Extents[0].DiskNumber;
		DevExt->DeviceInfo.StartingOffset	= pVolumeDiskExtents->Extents[0].StartingOffset;
		DevExt->DeviceInfo.PartitionLength		= pVolumeDiskExtents->Extents[0].ExtentLength;
		// Enable following code later
		{
		ULONG i;
		DevExt->DeviceInfo.PartitionLength.QuadPart = 0; 
		for ( i=0; i < pVolumeDiskExtents->NumberOfDiskExtents; i++)
			DevExt->DeviceInfo.PartitionLength.QuadPart	+= pVolumeDiskExtents->Extents[i].ExtentLength.QuadPart;
		}

	}
	
done: 
	if(pVolumeDiskExtents)
		OS_FreeMemory(pVolumeDiskExtents);

	return status;
} // sftk_GetVolumeDiskExtents()

NTSTATUS
sftk_StartSourceListenThread(PSFTK_CONFIG Sftk_Config)
{
	NTSTATUS status			= STATUS_SUCCESS;
	HANDLE threadHandle		= NULL;

	if(Sftk_Config->ListenThread == NULL)
	{ // No Source Listen Thread is Present so lets create one

		try
		{
			OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
			Sftk_Config->Lg_GroupList.ParentPtr = Sftk_Config;

			//Creating the Source Listen Thread
			status = PsCreateSystemThread(
				&threadHandle,		// thread handle
				0L,					// desired access
				NULL,				// object attributes
				NULL,				// process handle
				NULL,				// client id
				COM_ListenThread,	// Listen Thread
				(PVOID )&Sftk_Config->Lg_GroupList	// The Source Group List ANCHOR_LINKLIST
				);

			if(!NT_SUCCESS(status))
			{
				DebugPrint((DBG_ERROR, "sftk_ctl_config_end(): Unable to Create Source Listen Thread \n"));
				leave;
			}

			status = ObReferenceObjectByHandle (
							threadHandle,		// Object Handle
							THREAD_ALL_ACCESS,  // Desired Access
							NULL,               // Object Type
							KernelMode,         // Processor mode
							(PVOID *)&Sftk_Config->ListenThread,	// Object pointer
							NULL);									// Object Handle information

			if( !NT_SUCCESS(status) )
			{
				DebugPrint((DBG_ERROR, "sftk_ctl_config_end(): Unable to take ObReferenceObjectByHandle for Source Listen Thread \n"));
				Sftk_Config->ListenThread = NULL;
				leave;
			}

			ZwClose(threadHandle );
			threadHandle = NULL;
		} // try
		finally
		{
			if(threadHandle != NULL)
			{ // Check for the Thread Handle if it is still open then just close it.
				ZwClose(threadHandle );
				threadHandle = NULL;
			}
		}
	}// if(Sftk_Config->ListenThread == NULL)
	return status;
}// sftk_StartSourceListenThread

NTSTATUS
sftk_StartTargetListenThread(PSFTK_CONFIG Sftk_Config)
{
	NTSTATUS status			= STATUS_SUCCESS;
	HANDLE threadHandle		= NULL;

	if(Sftk_Config->TListenThread == NULL)
	{ // No Target Listen Thread is Present so lets create one

		try
		{
			OS_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
			Sftk_Config->TLg_GroupList.ParentPtr = Sftk_Config;

			//Creating the Source Listen Thread
			status = PsCreateSystemThread(
				&threadHandle,		// thread handle
				0L,					// desired access
				NULL,				// object attributes
				NULL,				// process handle
				NULL,				// client id
				COM_ListenThread,	// The Listen Thread
				(PVOID )&Sftk_Config->TLg_GroupList	// The Source Group List ANCHOR_LINKLIST
				);

			if(!NT_SUCCESS(status))
			{
				DebugPrint((DBG_ERROR, "sftk_ctl_config_end(): Unable to Create Target Listen Thread \n"));
				leave;
			}

			status = ObReferenceObjectByHandle (
							threadHandle,      // Object Handle
							THREAD_ALL_ACCESS,                     // Desired Access
							NULL,                                // Object Type
							KernelMode,                          // Processor mode
							(PVOID *)&Sftk_Config->TListenThread,// Object pointer
							NULL);                               // Object Handle information

			if( !NT_SUCCESS(status) )
			{
				DebugPrint((DBG_ERROR, "sftk_ctl_config_end(): Unable to take ObReferenceObjectByHandle for Target Listen Thread \n"));
				Sftk_Config->TListenThread = NULL;
				leave;
			}

			ZwClose(threadHandle );
			threadHandle = NULL;
		} // try
		finally
		{
			if(threadHandle != NULL)
			{ // Check for the Thread Handle if it is still open then just close it.
				ZwClose(threadHandle );
				threadHandle = NULL;
			}
		}
	}// if(Sftk_Config->TListenThread == NULL)
return status;
}// sftk_StartTargetListenThread

VOID
sftk_StopSourceListenThread(PSFTK_CONFIG Sftk_Config)
{
	NTSTATUS status = STATUS_SUCCESS;
	
	if(Sftk_Config->ListenThread == NULL)
	{ // Check to see if the Thread is valid 
		return;
	}
	KeSetEvent(&Sftk_Config->ListenThreadExitEvent, 0, FALSE);
	status = OS_KeWaitForSingleObject(Sftk_Config->ListenThread, Executive, KernelMode, FALSE, NULL);
	if(status == STATUS_SUCCESS)
	{
		DebugPrint((DBG_CONFIG, "sftk_ctl_config_end: Exiting the Source Listen Thread because there are no Logical Groups Left\n"));
		ObDereferenceObject(Sftk_Config->ListenThread);
		Sftk_Config->ListenThread = NULL;
	}
	else
	{
		DebugPrint((DBG_ERROR, "sftk_ctl_config_end: Error in Exiting the Source Listen Thread\n"));
		OS_ASSERT(FALSE);
	}
}// sftk_StopSourceListenThread

VOID
sftk_StopTargetListenThread(PSFTK_CONFIG Sftk_Config)
{
	NTSTATUS status = STATUS_SUCCESS;

	if(Sftk_Config->TListenThread == NULL)
	{ // Check to see if the Thread is valid or not.
		return;
	}
	KeSetEvent(&Sftk_Config->TListenThreadExitEvent, 0, FALSE);
	OS_KeWaitForSingleObject(Sftk_Config->TListenThread, Executive, KernelMode, FALSE, NULL);
	if(status == STATUS_SUCCESS)
	{
		DebugPrint((DBG_CONFIG, "sftk_ctl_config_end: Exiting the Target Listen Thread because there are no Logical Groups Left\n"));
		ObDereferenceObject(Sftk_Config->TListenThread);
		Sftk_Config->TListenThread = NULL;
	}
	else
	{
		DebugPrint((DBG_ERROR, "sftk_ctl_config_end: Error in Exiting the Target Listen Thread\n"));
		OS_ASSERT(FALSE);
	}

	DebugPrint((DBG_CONFIG, "sftk_ctl_config_end: Exiting the Target Listen Thread because there are no Logical Groups Left\n"));
}// sftk_StopTargetListenThread


NTSTATUS
sftk_ctl_config_begin( PIRP Irp )
{
	NTSTATUS			status		= STATUS_SUCCESS;
	PSFTK_LG			pSftkLg		= NULL;
	PSFTK_DEV			pSftkDev	= NULL;
	PIO_STACK_LOCATION  pCurStackLocation= IoGetCurrentIrpStackLocation(Irp);
	PCONFIG_BEGIN_INFO	pConfigBegin	 = Irp->AssociatedIrp.SystemBuffer;
	ULONG				sizeOfBuffer	 = pCurStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	PLIST_ENTRY			listEntry, listEntry1;
#if TARGET_SIDE
	BOOLEAN				bPrimary = TRUE;
	PANCHOR_LINKLIST	pLGList  = NULL;
	UCHAR				counter = 1;
#endif

	if (sizeOfBuffer < sizeof(CONFIG_BEGIN_INFO))
	{
		status = STATUS_INVALID_PARAMETER;
		DebugPrint((DBG_ERROR, "sftk_ctl_config_begin: sizeOfBuffer %d < sizeof(CONFIG_BEGIN_INFO) %d, Failed with status 0x%08x !!! \n",
										sizeOfBuffer, sizeof(CONFIG_BEGIN_INFO), status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	if (pConfigBegin->HostId != 0)
	{
		GSftk_Config.HostId = pConfigBegin->HostId;
	}
	else
	{
		DebugPrint((DBG_ERROR, "sftk_ctl_config_begin: FIXME FIXME pConfigBegin->HostId %d == 0 %d, Invalid Parameter !! Igonring !!! \n",
										pConfigBegin->HostId));
		OS_ASSERT(FALSE);
	}

	if (strlen(pConfigBegin->Version) > 0)
	{
		RtlCopyMemory(	GSftk_Config.Version, 
						pConfigBegin->Version, 
						sizeof(GSftk_Config.Version));
	}
	else
	{
		DebugPrint((DBG_ERROR, "sftk_ctl_config_begin: FIXME FIXME strlen(pConfigBegin->Version) %d <= 0,  Invalid Parameter !! Igonring !!! \n",
										strlen(pConfigBegin->Version)));
		OS_ASSERT(FALSE);
	}

	if (strlen(pConfigBegin->SystemName) > 0)
	{
		RtlCopyMemory(	GSftk_Config.SystemName, 
						pConfigBegin->SystemName, 
						sizeof(GSftk_Config.SystemName));
	}
	else
	{
		DebugPrint((DBG_ERROR, "sftk_ctl_config_begin: FIXME FIXME strlen(pConfigBegin->SystemName) %d <= 0,  Invalid Parameter !! Igonring !!! \n",
										strlen(pConfigBegin->SystemName)));
		OS_ASSERT(FALSE);
	}

	// Walk thru all existing LG and their devices to mark Config Start time 
	// ConfigValid = FALSE 
	// Later following IOCTLS will make this flag to TRUE if service has LG and dev entries
	OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_SHARED, TRUE, NULL);

#if TARGET_SIDE
	do
	{
		if (bPrimary == TRUE)
			pLGList = &GSftk_Config.Lg_GroupList;
		else
			pLGList = &GSftk_Config.TLg_GroupList;

		for(listEntry = pLGList->ListEntry.Flink; 
			listEntry != &pLGList->ListEntry;
			listEntry = listEntry->Flink)
#else
		for(listEntry = GSftk_Config.Lg_GroupList.ListEntry.Flink; 
			listEntry != &GSftk_Config.Lg_GroupList.ListEntry;
			listEntry = listEntry->Flink)
#endif
		{ // for : scan each and every LG existing entries
			pSftkLg = CONTAINING_RECORD( listEntry, SFTK_LG, Lg_GroupLink);

			OS_ASSERT(pSftkLg != NULL);
			OS_ASSERT(pSftkLg->NodeId.NodeType == NODE_TYPE_SFTK_LG);
#if TARGET_SIDE
			if (bPrimary == TRUE)
			{
				OS_ASSERT(pSftkLg->Role.CreationRole == PRIMARY);
			}
			else
			{
				OS_ASSERT(pSftkLg->Role.CreationRole == SECONDARY);
			}
#endif
			pSftkLg->ConfigStart = FALSE;
			pSftkLg->ResetConnections = FALSE;

			OS_ACQUIRE_LOCK( &pSftkLg->Lock, OS_ACQUIRE_SHARED, TRUE, NULL);
			for(listEntry1 = pSftkLg->LgDev_List.ListEntry.Flink; 
				listEntry1 != &pSftkLg->LgDev_List.ListEntry;
				listEntry1 = listEntry1->Flink)
			{ // for : scan each and every Devices under current LG existing entries
				pSftkDev = CONTAINING_RECORD( listEntry1, SFTK_DEV, LgDev_Link);

				OS_ASSERT(pSftkDev != NULL);
				OS_ASSERT(pSftkDev->NodeId.NodeType == NODE_TYPE_SFTK_DEV);
				OS_ASSERT(pSftkDev->SftkLg == pSftkLg);
				
				pSftkDev->ConfigStart = FALSE;
			} // for : scan each and every Devices under current LG existing entries
			OS_RELEASE_LOCK( &pSftkLg->Lock, NULL);
		} // for : scan each and every LG existing entries

#if TARGET_SIDE
	if (counter == 1)
		bPrimary = FALSE;

	counter++;
	} while (counter <= 2);
#endif

	OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
	return status;
} // sftk_ctl_config_begin()

NTSTATUS
sftk_ctl_config_end( PIRP Irp )
{
	NTSTATUS	status				= STATUS_SUCCESS;
	PSFTK_LG	pSftkLg				= NULL;
	PSFTK_DEV	pSftkDev			= NULL;
	BOOLEAN		bFormattedPstoreFile= FALSE;
	PLIST_ENTRY	listEntry, listEntry1;
	BOOLEAN		bFlushAllLg = FALSE;
	WCHAR		wchar1[60], wchar2[60];
#if TARGET_SIDE
	BOOLEAN				bPrimary = TRUE;
	PANCHOR_LINKLIST	pLGList  = NULL;
	UCHAR				counter = 1;
	HANDLE				threadHandle = NULL;
#endif
	
	
	// Walk thru all existing LG and their devices to mark Config Start time 
	// ConfigValid = FALSE 
	// Later following IOCTLS will make this flag to TRUE if service has LG and dev entries
	OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_SHARED, TRUE, NULL);

#if TARGET_SIDE
	do
	{
		if (bPrimary == TRUE)
			pLGList = &GSftk_Config.Lg_GroupList;
		else
			pLGList = &GSftk_Config.TLg_GroupList;

		for(listEntry = pLGList->ListEntry.Flink; 
			listEntry != &pLGList->ListEntry;)
#else
		for(listEntry = GSftk_Config.Lg_GroupList.ListEntry.Flink; 
			listEntry != &GSftk_Config.Lg_GroupList.ListEntry;)
#endif
		{ // for : scan each and every LG existing entries
			pSftkLg = CONTAINING_RECORD( listEntry, SFTK_LG, Lg_GroupLink);

			OS_ASSERT(pSftkLg != NULL);
			OS_ASSERT(pSftkLg->NodeId.NodeType == NODE_TYPE_SFTK_LG);

#if TARGET_SIDE
			if (bPrimary == TRUE)
			{
				OS_ASSERT(pSftkLg->Role.CreationRole == PRIMARY);
			}
			else
			{
				OS_ASSERT(pSftkLg->Role.CreationRole == SECONDARY);
			}
#endif
			listEntry = listEntry->Flink;

			if (pSftkLg->ConfigStart == FALSE)
			{ // Delete current LG and its all devices ..
				DebugPrint((DBG_ERROR, "sftk_ctl_config_end() LG 0x%08x and its all Devices requires to delete !! \n", pSftkLg->LGroupNumber)); 

				swprintf( wchar1, L"%d", pSftkLg->LGroupNumber );
				sftk_LogEventString1(GSftk_Config.DriverObject, MSG_REPL_DELETEING_LG, STATUS_SUCCESS, 
									0, wchar1);

				// TODO : Log event message here
				status = sftk_delete_lg(pSftkLg, FALSE);
				if (!NT_SUCCESS(status)) 
				{ // failed
					DebugPrint((DBG_ERROR, "sftk_ctl_config_end: LG 0x%08x sftk_delete_lg() Failed with status 0x%08x !!! \n",
											pSftkLg->LGroupNumber, status));
					// Log event message here
				}
				continue;
			}

			OS_ACQUIRE_LOCK( &pSftkLg->Lock, OS_ACQUIRE_SHARED, TRUE, NULL);

			for(listEntry1 = pSftkLg->LgDev_List.ListEntry.Flink; 
				listEntry1 != &pSftkLg->LgDev_List.ListEntry;)
			{ // for : scan each and every Devices under current LG existing entries
				pSftkDev = CONTAINING_RECORD( listEntry1, SFTK_DEV, LgDev_Link);

				OS_ASSERT(pSftkDev != NULL);
				OS_ASSERT(pSftkDev->NodeId.NodeType == NODE_TYPE_SFTK_DEV);
				
				listEntry1 = listEntry1->Flink;

				if (pSftkDev->ConfigStart == FALSE )
				{ // Delete current device under LG 
					DebugPrint((DBG_ERROR, "sftk_ctl_config_end: LG 0x%08x, Device %s requires to Delete !!! \n",
											pSftkLg->LGroupNumber, pSftkDev->Vdevname));
					// Log event message here
					swprintf( wchar1, L"%d", pSftkDev->cdev);
					swprintf( wchar2, L"%d", pSftkLg->LGroupNumber );
					sftk_LogEventString2(GSftk_Config.DriverObject, MSG_REPL_DELETEING_DEV, STATUS_SUCCESS, 
									0, wchar1, wchar2);

					status = sftk_delete_SftekDev(pSftkDev, FALSE, FALSE);
					if (!NT_SUCCESS(status)) 
					{ // failed
						DebugPrint((DBG_ERROR, "sftk_ctl_config_end: LG 0x%08x sftk_delte_SftekDev(Device - %s) Failed with status 0x%08x !!! \n",
											pSftkLg->LGroupNumber, pSftkDev->Vdevname, status));
						// Log event message here
					}

					if ( (pSftkLg->ResetConnections == FALSE) && 
						 (sftk_lg_is_socket_alive( pSftkLg ) == TRUE) )
					{
						pSftkLg->ResetConnections = TRUE;
					}
					continue;
				}
			} // for : scan each and every Devices under current LG existing entries

			// open Pstore file if its not already opened
			if (OS_IsFlagSet( pSftkLg->flags, SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE) == TRUE) 
			{ // open pstore now, and merge Bitmap from Pstore Bitmap 
				// We done with configuration, Clear this Flag now
				if (pSftkLg->LastShutDownUpdated == TRUE)
				{ // We have some Writes during boot time, update registry 
					sftk_lg_update_lastshutdownKey( pSftkLg, TRUE);
					pSftkLg->LastShutDownUpdated = FALSE;
				}
				// clear flag so we may used pstore file now
				OS_ClearFlag( pSftkLg->flags, SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE);
				OS_ClearFlag( pSftkLg->flags, SFTK_LG_FLAG_NEW_PSTORE_FILE_FORMATTED);

				bFlushAllLg = TRUE;
			} // open pstore now, and merge Bitmap from Pstore Bitmap 

			if ( (pSftkLg->ResetConnections == TRUE) && 
				 (sftk_lg_is_socket_alive( pSftkLg ) == TRUE) )
			{ // since we added or removed device under LG, Inform RMD by reseting connections !!!
				DebugPrint((DBG_ERROR, "sftk_ctl_config_end : calling Resete connection COM_ResetAllConnections(LgNum %d) \n", 
									pSftkLg->LGroupNumber)); 

				status = COM_ResetAllConnections( &pSftkLg->SessionMgr );

				if (!NT_SUCCESS(status))
				{ // reset socket connection failed....bumer...called mike to fix this batch process error handling....
					DebugPrint((DBG_ERROR, "sftk_ctl_config_end : COM_ResetAllConnections(LgNum %d) Failed with status 0x%08x \n", 
									pSftkLg->LGroupNumber, status)); 
					// Log event message here
					swprintf( wchar1, L"%d", pSftkLg->LGroupNumber);
					swprintf( wchar2, L"0x%08X", status );
					sftk_LogEventString2(GSftk_Config.DriverObject, MSG_LG_RECONFIG_RESET_SOCKET_CONNECTION_ERROR, status, 0, wchar1, wchar2);

					status = STATUS_SUCCESS;	// ignore this error.....FIX ME, Can't do anything here.....
				} // reset socket connection failed....bumer...called mike to fix this batch process error handling....
			} // since we added or removed device under LG, Inform RMD by reseting connections !!!
	#if 0
	/*
			else
			{
				// LG is created or already exist not deleted.
				// For current LG, start sockect connections if its not already started.
				if (sftk_lg_is_socket_alive( pSftkLg ) == FALSE)
				{ // start session manager to establishe socket connections...
					SM_INIT_PARAMS	sm_Params;

					RtlZeroMemory( &sm_Params, sizeof(sm_Params));

					// set all values as default....TODO: Service must have passed this default values during LG create !!
					sm_Params.lgnum						= pSftkLg->LGroupNumber;
					sm_Params.nSendWindowSize			= CONFIGURABLE_MAX_SEND_BUFFER_SIZE(pSftkLg->MaxTransferUnit);
					sm_Params.nMaxNumberOfSendBuffers	= DEFAULT_MAX_SEND_BUFFERS;	// 5 defined in ftdio.h
					sm_Params.nReceiveWindowSize		= CONFIGURABLE_MAX_RECIEVE_BUFFER_SIZE(pSftkLg->MaxTransferUnit);	
					sm_Params.nMaxNumberOfReceiveBuffers= DEFAULT_MAX_RECEIVE_BUFFERS;	// 5 defined in ftdio.h
					sm_Params.nChunkSize				= 0;	 
					sm_Params.nChunkDelay				= 0;	 

					DebugPrint((DBG_ERROR, "sftk_ctl_config_end : calling COM_StartPMD(LgNum %d) \n", 
										sm_Params.lgnum, status)); 
						
					status = COM_StartPMD( &pSftkLg->SessionMgr, &sm_Params);
					if (!NT_SUCCESS(status))
					{ // socket connection failed....bumer...called mike to fix this batch process error handling....
						DebugPrint((DBG_ERROR, "sftk_ctl_config_end : COM_StartPMD(LgNum %d) Failed with status 0x%08x \n", 
										pSftkLg->LGroupNumber, status)); 
						// Log event message here
						swprintf( wchar1, L"%d", pSftkLg->LGroupNumber);
						swprintf( wchar2, L"0x%08X", status );
						sftk_LogEventString2(GSftk_Config.DriverObject, MSG_LG_START_SOCKET_CONNECTION_ERROR, status, 0, wchar1, wchar2);

						status = STATUS_SUCCESS;	// ignore this error.....FIX ME, Can't do anything here.....
					} // socket connection failed....bumer...called mike to fix this batch process error handling....
				} // start session manager to establishe socket connections...
			}
	*/
	#endif
			pSftkLg->ResetConnections = FALSE;
			OS_RELEASE_LOCK( &pSftkLg->Lock, NULL);
		} // for : scan each and every LG existing entries

#if TARGET_SIDE
	if (counter == 1)
		bPrimary = FALSE;

	counter++;
	} while (counter <= 2);
#endif

	// Check to see if there are any Groups left on the Source Side
	// If there are no Groups left then just exit the Listen Thread that is listening on the Group Ports
	if(IsListEmpty(&GSftk_Config.Lg_GroupList.ListEntry))
	{	// Source Logical Group List is Empty
		sftk_StopSourceListenThread(&GSftk_Config);
	} // if(IsListEmpty(&GSftk_Config.Lg_GroupList))
	else
	{	// Source Logical Group List is not Empty, just check if we have the Source Listen Thread 
		// already existing, if not just create a new thread.
		status = sftk_StartSourceListenThread(&GSftk_Config);
	}

	// Check to see if there are any Groups left on the Target Side
	// If there are no Groups left then just exit the Listen Thread that is listening on the Group Ports
	if(IsListEmpty(&GSftk_Config.TLg_GroupList.ListEntry))
	{	// Target Logical Groups List is Empty
		sftk_StopTargetListenThread(&GSftk_Config);	
	}
	else
	{	// Target Logical Group List is not Empty, just check if we have the Target Listen Thread 
		// already existing, if not just create a new thread.
		status = sftk_StartTargetListenThread(&GSftk_Config);
	}

	// now we must flush this Sftk_LG to pstore file since we just updating lastshutdown reg key
	if (bFlushAllLg == TRUE)
	{
		// we not changing any state mode yet....
		status = sftk_flush_all_pstore(FALSE, TRUE, FALSE );
		if (!NT_SUCCESS(status)) 
		{ // failed
			DebugPrint((DBG_ERROR, "sftk_ctl_config_end: sftk_flush_all_pstore(FALSE) Failed with status 0x%08x !!! \n",
										status));
		}
	}

	status = STATUS_SUCCESS;
	OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
	return status;
} // sftk_ctl_config_end()

BOOLEAN 
sftk_ExceptionFilter(	PEXCEPTION_POINTERS ExceptionInformation,
						ULONG ExceptionCode)
{
    DbgPrint("*************************************************************\n");
    DbgPrint("** sftk_block: Exception Caught in driver!                 **\n");
    DbgPrint("** sftk_block: Execute the following windbg commands:      **\n");
    DbgPrint("**     !exr %x ; !cxr %x ; !kb                 **\n",
            ExceptionInformation->ExceptionRecord, ExceptionInformation->ContextRecord);
    DbgPrint("*************************************************************\n");
    __try 
    {
		SFTK_BreakPoint();
    } 
    __except (EXCEPTION_EXECUTE_HANDLER) 
    {// nothing
    }

    DbgPrint("*************************************************************\n");
    DbgPrint("** sftk_block: ExceptionFilter:Continuing past break point.**\n");
    DbgPrint("*************************************************************\n");

    return EXCEPTION_EXECUTE_HANDLER;
} // sftk_ExceptionFilter()

BOOLEAN 
sftk_ExceptionFilterDontStop(	PEXCEPTION_POINTERS ExceptionInformation,
								ULONG ExceptionCode)
{
	DbgPrint("*************************************************************\n");
    DbgPrint("** sftk_block: Exception Caught in DtcBlock driver!        **\n");
    DbgPrint("** sftk_block: Execute the following windbg commands:      **\n");
    DbgPrint("**     !exr %x ; !cxr %x ; !kb                 **\n",
            ExceptionInformation->ExceptionRecord, ExceptionInformation->ContextRecord);
    DbgPrint("*************************************************************\n");

    return EXCEPTION_EXECUTE_HANDLER;
} // sftk_ExceptionFilterDontStop()

//
// --------- Performance Monitoring and updating to Global Variable API ------------------
//
// it assumes global variable is initialize with zeros before calling this API sftk_init_perf_monitor()
VOID
sftk_init_perf_monitor()
{
	LONG	i;

	OS_ZeroMemory(GSftk_Config.PerfTable, sizeof(GSftk_Config.PerfTable));

	for(i=0; i < MAX_PERF_TYPE; i++ )
	{
		GSftk_Config.PerfTable[i].MinDiffTime.QuadPart = 0xEFFFFFFFFFFFFFFF;	// max positive value
		GSftk_Config.PerfTable[i].MaxDiffTime.QuadPart = 0;	// max positive value
	}
	return;
} // sftk_init_perf_monitor()

VOID
sftk_perf_monitor( PERF_TYPE	Type, PLARGE_INTEGER	StartTime, ULONG	WorkLoad)
{
	LARGE_INTEGER	endTime, diffTime;

	if (Type >= MAX_PERF_TYPE)
		return;	// Didn't set to watch this performance type

	OS_PerfGetDiffTime( StartTime, &endTime, &diffTime );

	GSftk_Config.PerfTable[Type].TotalCount ++;	
	GSftk_Config.PerfTable[Type].TotalLoad += WorkLoad;
	GSftk_Config.PerfTable[Type].TotalDiffTime.QuadPart += diffTime.QuadPart;	

	if (diffTime.QuadPart < GSftk_Config.PerfTable[Type].MinDiffTime.QuadPart)
	{
		GSftk_Config.PerfTable[Type].MinLoadCount = WorkLoad;	
		GSftk_Config.PerfTable[Type].MinDiffTime.QuadPart = diffTime.QuadPart;	
	}

	if (diffTime.QuadPart > GSftk_Config.PerfTable[Type].MaxDiffTime.QuadPart)
	{
		GSftk_Config.PerfTable[Type].MaxLoadCount = WorkLoad;	
		GSftk_Config.PerfTable[Type].MaxDiffTime.QuadPart = diffTime.QuadPart;	
	}
	return;
} // sftk_perf_monitor()

//
// IOCTL caller should use typedef enum PERF_TYPE to format return buffer and display perf info
//
NTSTATUS
sftk_ctl_get_perf_info( PIRP Irp )
{
	PIO_STACK_LOCATION  pCurStackLocation	= IoGetCurrentIrpStackLocation(Irp);
	PVOID				outBuffer			= Irp->AssociatedIrp.SystemBuffer;
	ULONG				sizeOfBuffer		= pCurStackLocation->Parameters.DeviceIoControl.InputBufferLength;

	if (sizeOfBuffer <	sizeof(GSftk_Config.PerfTable))
	{
		DebugPrint((DBG_ERROR, "sftk_ctl_get_perf_info: sizeOfBuffer %d < sizeof(GSftk_Config.PerfTable) %d Failed with status 0x%08x !!! \n",
										sizeOfBuffer, sizeof(GSftk_Config.PerfTable), STATUS_BUFFER_OVERFLOW));
		return STATUS_BUFFER_OVERFLOW;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	OS_RtlCopyMemory( outBuffer, &GSftk_Config.PerfTable[0], sizeOfBuffer);
	
	return STATUS_SUCCESS;
} // sftk_ctl_get_perf_info()

NTSTATUS
sftk_ctl_get_all_attached_diskInfo( PIRP Irp )
{
	NTSTATUS					status				= STATUS_SUCCESS;
	PIO_STACK_LOCATION			pCurStackLocation	= IoGetCurrentIrpStackLocation(Irp);
	PATTACHED_DISK_INFO_LIST	outBuffer			= Irp->AssociatedIrp.SystemBuffer;
	ULONG						sizeOfBuffer		= pCurStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	ULONG						sizeExpected;
	PLIST_ENTRY					plistEntry, pListEntry;
	PDEVICE_EXTENSION			pDevExt;
	PATTACHED_DISK_INFO_LIST	pAttachDiskInfoList;
	PATTACHED_DISK_INFO			pAttachDiskInfo;

	sizeExpected = MAX_SIZE_ATTACH_DISK_INFOLIST(outBuffer->NumOfDisks);
	if (sizeOfBuffer < sizeExpected)
	{
		status = STATUS_BUFFER_OVERFLOW;
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_Get_All_StatsInfo: Actial sizeOfBuffer %d < %d MAX_SIZE_ATTACH_DISK_INFOLIST(InBuffer), outBuffer->NumOfDisks %d Failed with status 0x%08x !!! \n",
										sizeOfBuffer, sizeExpected, outBuffer->NumOfDisks, status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	if (outBuffer->NumOfDisks == 0)
	{ // return total number of disk counter informations, so caller can allocate proper memory for next call
		OS_ZeroMemory(outBuffer, sizeOfBuffer);
		outBuffer->NumOfDisks = GSftk_Config.DevExt_List.NumOfNodes;
		return STATUS_SUCCESS;
	}

	sizeExpected = MAX_SIZE_ATTACH_DISK_INFOLIST( GSftk_Config.DevExt_List.NumOfNodes);
	if (sizeOfBuffer < sizeExpected)
	{
		status = STATUS_BUFFER_OVERFLOW;
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_Get_All_StatsInfo: Actial sizeOfBuffer %d < %d MAX_SIZE_ATTACH_DISK_INFOLIST(DevExt_List), NumOfDevEntries %d Failed with status 0x%08x !!! \n",
										sizeOfBuffer, sizeExpected, GSftk_Config.DevExt_List.NumOfNodes, status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	OS_ZeroMemory(outBuffer, sizeOfBuffer);

	pAttachDiskInfoList				= outBuffer;
	pAttachDiskInfoList->NumOfDisks	= 0;
	pAttachDiskInfo					= &outBuffer->DiskInfo[0];

	for( plistEntry = GSftk_Config.DevExt_List.ListEntry.Flink;
		 plistEntry != &GSftk_Config.DevExt_List.ListEntry;
		 plistEntry = plistEntry->Flink )
	{ // for :scan thru each and every LG
		pDevExt = CONTAINING_RECORD( plistEntry, DEVICE_EXTENSION, DevExt_Link);

		// UpdateDevExtDeviceInfo(pAttachedDevExt)
		if (pDevExt->DeviceInfo.bValidName == FALSE)
		{ // Update Device Info for current Dev Ext, refresh this devext.
			UpdateDevExtDeviceInfo(pDevExt);
		}

		OS_ZeroMemory( pAttachDiskInfo, sizeof(ATTACHED_DISK_INFO) );

		pAttachDiskInfo->DiskNumber	= pDevExt->DeviceInfo.DiskNumber;
		wcscpy( pAttachDiskInfo->PhysicalDeviceNameBuffer,	pDevExt->DeviceInfo.PhysicalDeviceNameBuffer);
		pAttachDiskInfo->bValidName	= pDevExt->DeviceInfo.bValidName;
		RtlCopyMemory(	pAttachDiskInfo->StorageManagerName,	
						pDevExt->DeviceInfo.StorageManagerName, 
						sizeof(pAttachDiskInfo->StorageManagerName));

		pAttachDiskInfo->bStorage_device_Number	= pDevExt->DeviceInfo.bStorage_device_Number;
		pAttachDiskInfo->DeviceType			= pDevExt->DeviceInfo.StorageDeviceNumber.DeviceType;
		pAttachDiskInfo->DeviceNumber		= pDevExt->DeviceInfo.StorageDeviceNumber.DeviceNumber;
		pAttachDiskInfo->PartitionNumber	= pDevExt->DeviceInfo.StorageDeviceNumber.PartitionNumber;
		wcscpy( pAttachDiskInfo->DiskPartitionName,	pDevExt->DeviceInfo.DiskPartitionName);

		pAttachDiskInfo->bVolumeNumber	= pDevExt->DeviceInfo.bVolumeNumber;
		pAttachDiskInfo->VolumeNumber	= pDevExt->DeviceInfo.VolumeNumber.VolumeNumber;
		RtlCopyMemory( pAttachDiskInfo->VolumeManagerName,	pDevExt->DeviceInfo.VolumeNumber.VolumeManagerName, sizeof(pAttachDiskInfo->VolumeManagerName));

		pAttachDiskInfo->bDiskVolumeName	= pDevExt->DeviceInfo.bDiskVolumeName;
		wcscpy( pAttachDiskInfo->DiskVolumeName,	pDevExt->DeviceInfo.DiskVolumeName);

		pAttachDiskInfo->bUniqueVolumeId	= pDevExt->DeviceInfo.bUniqueVolumeId;
		if (pDevExt->DeviceInfo.UniqueIdInfo)
		{
			pAttachDiskInfo->UniqueIdLength	= pDevExt->DeviceInfo.UniqueIdInfo->UniqueIdLength;
			RtlCopyMemory(	pAttachDiskInfo->UniqueId, 
							pDevExt->DeviceInfo.UniqueIdInfo->UniqueId, 
							pDevExt->DeviceInfo.UniqueIdInfo->UniqueIdLength );
		}

		pAttachDiskInfo->bSuggestedDriveLetter	= pDevExt->DeviceInfo.bSuggestedDriveLetter;
		if(pDevExt->DeviceInfo.SuggestedDriveLinkName)
		{
			pAttachDiskInfo->UseOnlyIfThereAreNoOtherLinks	= pDevExt->DeviceInfo.SuggestedDriveLinkName->UseOnlyIfThereAreNoOtherLinks;
			pAttachDiskInfo->NameLength	= pDevExt->DeviceInfo.SuggestedDriveLinkName->NameLength;
			RtlCopyMemory(	pAttachDiskInfo->SuggestedDriveName, 
							pDevExt->DeviceInfo.SuggestedDriveLinkName->Name, 
							pDevExt->DeviceInfo.SuggestedDriveLinkName->NameLength );
		}
		
		pAttachDiskInfo->bSignatureUniqueVolumeId	= pDevExt->DeviceInfo.bSignatureUniqueVolumeId;
		pAttachDiskInfo->SignatureUniqueIdLength	= pDevExt->DeviceInfo.SignatureUniqueIdLength;
		RtlCopyMemory(	pAttachDiskInfo->SignatureUniqueId, 
						pDevExt->DeviceInfo.SignatureUniqueId, 
						pDevExt->DeviceInfo.SignatureUniqueIdLength );

		pAttachDiskInfo->IsVolumeFtVolume	= pDevExt->DeviceInfo.IsVolumeFtVolume;
		pAttachDiskInfo->pRawDiskDevice		= pDevExt->DeviceInfo.pRawDiskDevice;

		pAttachDiskInfo->Signature					= pDevExt->DeviceInfo.Signature;
		pAttachDiskInfo->StartingOffset.QuadPart	= pDevExt->DeviceInfo.StartingOffset.QuadPart;
		pAttachDiskInfo->PartitionLength.QuadPart	= pDevExt->DeviceInfo.PartitionLength.QuadPart;

		pAttachDiskInfo->SftkDev	= pDevExt->Sftk_dev;

		if (pDevExt->Sftk_dev)
		{
			pAttachDiskInfo->cdev	= pDevExt->Sftk_dev->cdev;
			if (pDevExt->Sftk_dev->SftkLg)
				pAttachDiskInfo->LGNum	= pDevExt->Sftk_dev->SftkLg->LGroupNumber;
		}
		
		// increment array counter and buffer pointer
		pAttachDiskInfo = (PATTACHED_DISK_INFO) ((ULONG) pAttachDiskInfo + sizeof(ATTACHED_DISK_INFO));
		pAttachDiskInfoList->NumOfDisks ++;
	} // for :scan thru each and every LG

	return STATUS_SUCCESS;
} // sftk_ctl_get_all_attached_diskInfo()

#if	MM_TEST_WINDOWS_SLAB
//
// IOCTL caller should use typedef enum PERF_TYPE to format return buffer and display perf info
//
NTSTATUS
sftk_ctl_get_MM_Alloc_Info( PIRP Irp )
{
	PIO_STACK_LOCATION  pCurStackLocation	= IoGetCurrentIrpStackLocation(Irp);
	MM_ANCHOR_INFO		*outBuffer			= Irp->AssociatedIrp.SystemBuffer;
	ULONG				sizeOfBuffer		= pCurStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	ULONG				i;

	if (sizeOfBuffer <	(sizeof(MM_ANCHOR_INFO) * MM_TYPE_MAX) )
	{
		return STATUS_BUFFER_OVERFLOW;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	for (i=0; i < MM_TYPE_MAX; i++)
	{
		outBuffer[i].NumOfFreeList	= GSftk_Config.MmSlab[i].FreeList.NumOfNodes;
		// outBuffer[i].NumOfUsedList	= GSftk_Config.MmSlab[i].UsedList.NumOfNodes;
		outBuffer[i].Type			= GSftk_Config.MmSlab[i].Type;
		outBuffer[i].Flag			= GSftk_Config.MmSlab[i].Flag;
		outBuffer[i].FixedSize		= GSftk_Config.MmSlab[i].NodeSize;
		outBuffer[i].TotalSize		= GSftk_Config.MmSlab[i].TotalMemSize;

		outBuffer[i].MaximumSize	= GSftk_Config.MmSlab[i].MaximumSize;
		outBuffer[i].MinimumSize	= GSftk_Config.MmSlab[i].MinimumSize;
		outBuffer[i].NumOfNodestoKeep	= GSftk_Config.MmSlab[i].NumOfNodestoKeep;
		outBuffer[i].TotalNumAllocated	= GSftk_Config.MmSlab[i].TotalNumberOfNodes;

		if (i == MM_TYPE_4K_PAGED_MEM)
		{
			outBuffer[i].Lookaside.Depth			= GSftk_Config.MmSlab[i].parm.PagedLookaside.L.Depth;
			outBuffer[i].Lookaside.MaximumDepth		= GSftk_Config.MmSlab[i].parm.PagedLookaside.L.MaximumDepth;
			outBuffer[i].Lookaside.TotalAllocates	= GSftk_Config.MmSlab[i].parm.PagedLookaside.L.TotalAllocates;
			outBuffer[i].Lookaside.AllocateMisses	= GSftk_Config.MmSlab[i].parm.PagedLookaside.L.AllocateMisses;
			outBuffer[i].Lookaside.TotalFrees		= GSftk_Config.MmSlab[i].parm.PagedLookaside.L.TotalFrees;
			outBuffer[i].Lookaside.FreeMisses		= GSftk_Config.MmSlab[i].parm.PagedLookaside.L.FreeMisses;
			outBuffer[i].Lookaside.PoolType			= GSftk_Config.MmSlab[i].parm.PagedLookaside.L.Type;
			outBuffer[i].Lookaside.Tag				= GSftk_Config.MmSlab[i].parm.PagedLookaside.L.Tag;
			outBuffer[i].Lookaside.Size				= GSftk_Config.MmSlab[i].parm.PagedLookaside.L.Size;
			outBuffer[i].Lookaside.Allocate			= GSftk_Config.MmSlab[i].parm.PagedLookaside.L.Allocate;
			outBuffer[i].Lookaside.Free				= GSftk_Config.MmSlab[i].parm.PagedLookaside.L.Free;
			outBuffer[i].Lookaside.LastTotalAllocates	= GSftk_Config.MmSlab[i].parm.PagedLookaside.L.LastTotalAllocates;
			outBuffer[i].Lookaside.LastAllocateMisses	= GSftk_Config.MmSlab[i].parm.PagedLookaside.L.LastAllocateMisses;
		}
		else
		{
			outBuffer[i].Lookaside.Depth			= GSftk_Config.MmSlab[i].parm.NPagedLookaside.L.Depth;
			outBuffer[i].Lookaside.MaximumDepth		= GSftk_Config.MmSlab[i].parm.NPagedLookaside.L.MaximumDepth;
			outBuffer[i].Lookaside.TotalAllocates	= GSftk_Config.MmSlab[i].parm.NPagedLookaside.L.TotalAllocates;
			outBuffer[i].Lookaside.AllocateMisses	= GSftk_Config.MmSlab[i].parm.NPagedLookaside.L.AllocateMisses;
			outBuffer[i].Lookaside.TotalFrees		= GSftk_Config.MmSlab[i].parm.NPagedLookaside.L.TotalFrees;
			outBuffer[i].Lookaside.FreeMisses		= GSftk_Config.MmSlab[i].parm.NPagedLookaside.L.FreeMisses;
			outBuffer[i].Lookaside.PoolType			= GSftk_Config.MmSlab[i].parm.NPagedLookaside.L.Type;
			outBuffer[i].Lookaside.Tag				= GSftk_Config.MmSlab[i].parm.NPagedLookaside.L.Tag;
			outBuffer[i].Lookaside.Size				= GSftk_Config.MmSlab[i].parm.NPagedLookaside.L.Size;
			outBuffer[i].Lookaside.Allocate			= GSftk_Config.MmSlab[i].parm.NPagedLookaside.L.Allocate;
			outBuffer[i].Lookaside.Free				= GSftk_Config.MmSlab[i].parm.NPagedLookaside.L.Free;
			outBuffer[i].Lookaside.LastTotalAllocates	= GSftk_Config.MmSlab[i].parm.NPagedLookaside.L.LastTotalAllocates;
			outBuffer[i].Lookaside.LastAllocateMisses	= GSftk_Config.MmSlab[i].parm.NPagedLookaside.L.LastAllocateMisses;
		}
		
		outBuffer[i].Allocate		= GSftk_Config.MmSlab[i].Allocate;
		outBuffer[i].Free			= GSftk_Config.MmSlab[i].Free;
	}
	
	return STATUS_SUCCESS;
} // sftk_ctl_get_MM_Alloc_Info()

#endif // #if	MM_TEST_WINDOWS_SLAB