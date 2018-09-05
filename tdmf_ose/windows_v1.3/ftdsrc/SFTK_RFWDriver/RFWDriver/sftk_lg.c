/**************************************************************************************

Module Name: sftk_lg.C   
Author Name: Parag sanghvi 
Description: All APIS related with SFTK_LG	struct
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/
#include <sftk_main.h>

NTSTATUS
sftk_ctl_new_lg( PIRP Irp )
{
	NTSTATUS			status				= STATUS_SUCCESS;
	PIO_STACK_LOCATION  pCurStackLocation	= IoGetCurrentIrpStackLocation(Irp);
	ftd_lg_info_t		*in_lg_info			= Irp->AssociatedIrp.SystemBuffer;
	ULONG				sizeOfBuffer		= pCurStackLocation->Parameters.DeviceIoControl.InputBufferLength;
    PSFTK_LG			pSftk_Lg			= NULL;
	
	if (sizeOfBuffer < sizeof(ftd_lg_info_t))
	{
		status = STATUS_BUFFER_OVERFLOW;
		DebugPrint((DBG_ERROR, "sftk_ctl_new_lg: LG %d, sizeOfBuffer %d < sizeof(ftd_lg_info_t) %d, Failed with status 0x%08x !!! \n",
										in_lg_info->lgdev, sizeOfBuffer, sizeof(ftd_lg_info_t), status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

    // We need to check if it exists already, if so say INUSE to user
#if TARGET_SIDE
	pSftk_Lg = sftk_lookup_lg_by_lgnum( in_lg_info->lgdev, in_lg_info->lgCreationRole );
#else
    pSftk_Lg = sftk_lookup_lg_by_lgnum( in_lg_info->lgdev );
#endif
    if (pSftk_Lg != NULL) 
    { // LG Already exist
		if (pSftk_Lg->ConfigStart == FALSE)
		{ // Config start time Phase, Do Validation of existing LG 
			ANSI_STRING		pstore_String;
			UNICODE_STRING	pStoreUnicodeName;

			// Do Validation of Vdevname and other paramteres
			// Do according action if validation fails !!! 
			// -- First check Pstor file name in_lg_info->vdevname
			RtlInitAnsiString( &pstore_String, in_lg_info->vdevname);
			status = OS_AnsiStringToUnicodeString( &pStoreUnicodeName, &pstore_String, TRUE);
			if (!NT_SUCCESS(status)) 
			{ // failed
				DebugPrint((DBG_ERROR, "sftk_ctl_new_lg: OS_AnsiStringToUnicodeString(PstoreFile Name %s) to allocate Unicode String Failed with status 0x%08x \n", 
							in_lg_info->vdevname, status));
			}
			else
			{
				if ( RtlCompareUnicodeString(&pStoreUnicodeName, &pSftk_Lg->PStoreFileName, TRUE) != 0)
				{
					DebugPrint((DBG_ERROR, "sftk_ctl_new_lg: FIXME FIXME RtlCompareUnicodeString(In PstoreFile Name %S, LG Pstore name %S ) Failed !! FIXME FIXME \n", 
								pStoreUnicodeName.Buffer, pSftk_Lg->PStoreFileName.Buffer));

					// copy Service supplied latest Pstore file into LG and later update registry for this
					if (OS_IsFlagSet( pSftk_Lg->flags, SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE) == TRUE) 
					{ // open pstore now, and merge Bitmap from Pstore Bitmap 
						if(pSftk_Lg->PStoreFileName.Buffer)
							RtlFreeUnicodeString( &pSftk_Lg->PStoreFileName );

						// instead of allocating again Pstore Unicode string just copy local to LG pstore unicode !
						pSftk_Lg->PStoreFileName.Buffer			= pStoreUnicodeName.Buffer;
						pSftk_Lg->PStoreFileName.Length			= pStoreUnicodeName.Length;
						pSftk_Lg->PStoreFileName.MaximumLength	= pStoreUnicodeName.MaximumLength;

						sftk_lg_update_PstoreFileNameKey( pSftk_Lg, pSftk_Lg->PStoreFileName.Buffer);
						// Log event message here
						sftk_LogEventNum1Wchar1(GSftk_Config.DriverObject, MSG_REPL_LG_PSTORE_NAME_CHANGED, STATUS_SUCCESS, 
												0, pSftk_Lg->LGroupNumber, pSftk_Lg->PStoreFileName.Buffer);
					}
					else
					{
						RtlFreeUnicodeString( &pStoreUnicodeName );
					}
				}
				else
					RtlFreeUnicodeString( &pStoreUnicodeName );
			}
			// -- check LG stateSize 
			if ( pSftk_Lg->statsize != in_lg_info->statsize )
			{
				DebugPrint((DBG_ERROR, "sftk_ctl_new_lg: FIXME FIXME ( pSftk_Lg->statsize %d != in_lg_info->statsize %d ) Failed !! FIXME FIXME \n", 
								pSftk_Lg->statsize, in_lg_info->statsize));
			}

			// open Pstore file if its not already opened
			if (OS_IsFlagSet( pSftk_Lg->flags, SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE) == TRUE) 
			{ // open pstore now, and merge Bitmap from Pstore Bitmap 
				// open pstore file and merger bitmap here.....
				status = sftk_Update_LG_from_pstore( pSftk_Lg );
				if ( !NT_SUCCESS(status) ) 
				{ // failed to open or create pstore file...
					DebugPrint((DBG_ERROR, "sftk_ctl_new_lg:: sftk_Update_LG_from_pstore( LGNum %d, PstoreFile Name %S) Failed with status 0x%08x !\n", 
											pSftk_Lg->LGroupNumber, pSftk_Lg->PStoreFileName.Buffer, status ));
					// Log Event, and error handling, we should terminate here ??? or just continue ... ???
					sftk_LogEventNum1Wchar1(GSftk_Config.DriverObject, MSG_REPL_DELETEING_LG_DUE_TO_PSTORE, status, 
												0, pSftk_Lg->LGroupNumber, pSftk_Lg->PStoreFileName.Buffer);

					DebugPrint((DBG_ERROR, "**** FIXME FIXME ***** sftk_ctl_new_lg: Mark to Delete this LG %d due to PstoreFile Name %S can't create or opened !!\n", 
								pSftk_Lg->LGroupNumber, pSftk_Lg->PStoreFileName.Buffer));

					return status;
				} // failed, seriouse error !!!
			} // open pstore now, and merge Bitmap from Pstore Bitmap 
				
			pSftk_Lg->ConfigStart = TRUE;	// Already exist

			return STATUS_SUCCESS;
		} // Config start time Phase, Do Validation of existing LG 

		// other wise This call came not during Config start time 
		DebugPrint((DBG_ERROR, "sftk_ctl_new_lg: specified Lgdev number = %d already exist !!! returning error 0x%08x \n",
							in_lg_info->lgdev, STATUS_OBJECTID_EXISTS));
		return STATUS_OBJECTID_EXISTS;	
    } // // LG Already exist

    // create a LG device and its relative threads 
	status = sftk_Create_InitializeLGDevice( GSftk_Config.DriverObject, 
											 in_lg_info,
											 &pSftk_Lg,
											 TRUE,
											 TRUE);

    if ( !NT_SUCCESS(status) )
    {
		DebugPrint((DBG_ERROR, "sftk_ctl_new_lg: sftk_Create_InitializeLGDevice (Lgdev number = %d) Failed to create!!! returning error 0x%08x \n",
							in_lg_info->lgdev, status));
        return status;
    }
	else
	{ // success
		if(pSftk_Lg)
			pSftk_Lg->ConfigStart = TRUE;	// Created new one
	}

    return status;
} // sftk_ctl_new_lg()


NTSTATUS
sftk_ctl_del_lg( dev_t dev, int cmd, int arg, int flag)
{
    
    PSFTK_LG		pSftk_Lg	= NULL;
	NTSTATUS		status		= STATUS_SUCCESS;
#if TARGET_SIDE
	dev_t			lgNum;
	ftd_state_t		*pFtd_State_state_s = (ftd_state_t *) arg;
	lgNum = pFtd_State_state_s->lg_num;
#else
	dev_t			lgNum		= *(dev_t *)arg;
#endif
    // Retrieve LG Device
#if TARGET_SIDE
	pSftk_Lg = sftk_lookup_lg_by_lgnum( pFtd_State_state_s->lg_num, pFtd_State_state_s->lgCreationRole);
#else
    pSftk_Lg = sftk_lookup_lg_by_lgnum( lgNum );
#endif
    if (pSftk_Lg == NULL) 
    {
#if TARGET_SIDE
		DebugPrint((DBG_ERROR, "sftk_ctl_del_lg: specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
					pFtd_State_state_s->lg_num, STATUS_DEVICE_DOES_NOT_EXIST));
#else
		DebugPrint((DBG_ERROR, "sftk_ctl_del_lg: specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
							lgNum, STATUS_DEVICE_DOES_NOT_EXIST));
#endif

        return STATUS_DEVICE_DOES_NOT_EXIST;	
    }

    // Nope so create a NT one
	status = sftk_delete_lg( pSftk_Lg, TRUE );

    if ( !NT_SUCCESS(status) )
    {
		DebugPrint((DBG_ERROR, "sftk_ctl_del_lg: sftk_delete_lg(Lgdev number = %d) Failed to Delete!!! returning error 0x%08x \n",
							lgNum, status));
        return status;
    }

    return status;
} // sftk_ctl_new_lg()

NTSTATUS
sftk_delete_lg( IN OUT	PSFTK_LG Sftk_Lg, BOOLEAN bGrabGlobalLock )
{
	NTSTATUS	status		= STATUS_SUCCESS;
	PLIST_ENTRY	plistEntry	= NULL;
	PSFTK_DEV	pSftkDev	= NULL;
	NTSTATUS	tmpStatus;	// for debugging purpose only

	OS_ASSERT(Sftk_Lg != NULL);

	// Go thru each and every devices under this LG and delete them all.
	OS_ACQUIRE_LOCK( &Sftk_Lg->Lock, OS_ACQUIRE_SHARED, TRUE, NULL);

	// changed lg state to Pass thru first
	sftk_lg_change_State(Sftk_Lg, Sftk_Lg->state, SFTK_MODE_PASSTHRU, FALSE);

	// first uninitialize and stop session socket manager for current LG
	COM_UninitializeSessionManager( &Sftk_Lg->SessionMgr );

	// while ( !IsListEmpty( &Sftk_Lg->LgDev_List.ListEntry) ) 
	for(plistEntry  = Sftk_Lg->LgDev_List.ListEntry.Flink; 
		plistEntry != &Sftk_Lg->LgDev_List.ListEntry;)
	{ // for : scan each and every Devices under current LG existing entries
		pSftkDev = CONTAINING_RECORD( plistEntry, SFTK_DEV, LgDev_Link);

		OS_ASSERT(pSftkDev != NULL);
		OS_ASSERT(pSftkDev->NodeId.NodeType == NODE_TYPE_SFTK_DEV);
		OS_ASSERT(pSftkDev->SftkLg == Sftk_Lg);
		OS_ASSERT(pSftkDev->LGroupNumber == Sftk_Lg->LGroupNumber);
	
		plistEntry = plistEntry->Flink;

		status = sftk_delete_SftekDev( pSftkDev, FALSE, bGrabGlobalLock);

		if (!NT_SUCCESS(status)) 
		{ // failed
			DebugPrint((DBG_ERROR, "sftk_delete_lg: sftk_delte_SftekDev(Device - %s) Failed with status 0x%08x !!! \n",
								pSftkDev->Vdevname, status));
			OS_ASSERT(FALSE);
		}
	} // for : scan each and every Devices under current LG existing entries
	OS_RELEASE_LOCK( &Sftk_Lg->Lock, NULL);
	
	// Now free all resources allocated for LG
	if ( OS_IsFlagSet( Sftk_Lg->flags, SFTK_LG_FLAG_ADDED_TO_LGDEVICE_ANCHOR) ) 
	{ // Remove from LG Device List Anchor
		if (bGrabGlobalLock == TRUE)
			OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
		RemoveEntryList( &Sftk_Lg->Lg_GroupLink );
		GSftk_Config.Lg_GroupList.NumOfNodes --;
		if (bGrabGlobalLock == TRUE)
			OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
	}

	if ( OS_IsFlagSet( Sftk_Lg->flags, SFTK_LG_FLAG_STARTED_LG_THREADS) ) 
	{ // Terminate Thread for LG
		sftk_Terminate_LGThread( Sftk_Lg );
	}

	sftk_lg_close_named_event(Sftk_Lg, FALSE);

	// Delet Pstore file related information
	if (Sftk_Lg->PsHeader)
		OS_FreeMemory(Sftk_Lg->PsHeader);

	if ( OS_IsFlagSet( Sftk_Lg->flags, SFTK_LG_FLAG_PSTORE_FILE_CREATED) ) 
	{ // Delete Pstore File 
		tmpStatus = sftk_delete_pstorefile( Sftk_Lg);
		if (!NT_SUCCESS(tmpStatus)) 
		{
			DebugPrint((DBG_ERROR, "sftk_delete_lg: sftk_delete_pstorefile( LG %d PstoreFile Name %S) Failed with status 0x%08x \n", 
						Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer, tmpStatus));
		}
	}

	if ( OS_IsFlagSet( Sftk_Lg->flags, SFTK_LG_FLAG_REG_CREATED) ) 
	{ // Delete Reg Key
		tmpStatus = sftk_lg_Delete_RegKey( Sftk_Lg);
		if (!NT_SUCCESS(tmpStatus)) 
		{
			DebugPrint((DBG_ERROR, "sftk_delete_lg: sftk_lg_Delete_RegKey( LG %d PstoreFile Name %S) Failed with status 0x%08x \n", 
						Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer, tmpStatus));
		}
	}

	if(Sftk_Lg->statbuf)
		OS_FreeMemory( Sftk_Lg->statbuf );

	if ( OS_IsFlagSet( Sftk_Lg->flags, SFTK_LG_FLAG_SYMLINK_INITIALIZED) ) 
        IoDeleteSymbolicLink(&Sftk_Lg->UserVisibleName);

    if (Sftk_Lg->UserVisibleName.Buffer != NULL) 
		OS_FreeMemory( Sftk_Lg->UserVisibleName.Buffer );

	if(Sftk_Lg->PStoreFileName.Buffer)
		RtlFreeUnicodeString( &Sftk_Lg->PStoreFileName );

	if ( OS_IsFlagSet( Sftk_Lg->flags, SFTK_LG_FLAG_CACHE_INITIALIZED) ) 
	{
		// Memory Manager cleanup if needed....
        DebugPrint((DBG_ERROR, "FIXME FIXME : sftk_delete_lg: Call Reverse of RplCCLgInit() to DeInit Logical Group From Cache Manaher.... \n"));
	}

	if ( OS_IsFlagSet( Sftk_Lg->flags, SFTK_LG_FLAG_SOCKET_INITIALIZED) ) 
	{
		// TODO - Cache Manager, Jerome ....
        DebugPrint((DBG_ERROR, "FIXME FIXME : sftk_delete_lg: Call DeInit Socket per Group based, if itrs needed...while deltion of Logical Group ... \n"));
	}

	QM_DeInit(Sftk_Lg);

    // Now, delete any device objects, etc. we may have created
    if (Sftk_Lg->DeviceObject) 
        IoDeleteDevice(Sftk_Lg->DeviceObject);

	return status;
} // sftk_delete_lg()

// Create DirectoryObject/LogicalGroup(n) device for kernel and Win32 device object and inistialze it
NTSTATUS 
sftk_Create_InitializeLGDevice(	IN		PDRIVER_OBJECT	DriverObject, 
								IN		ftd_lg_info_t	*LG_Info, 
								IN OUT	PSFTK_LG		*Sftk_Lg, 
								IN		BOOLEAN			CreatePstoreFile,
								IN		BOOLEAN			CreateRegistry)
{
	PSFTK_LG			pSftk_LG		= NULL;
	NTSTATUS            status			= STATUS_SUCCESS;
	PDEVICE_OBJECT      deviceObject	= NULL;
	STRING              ntString;
    UNICODE_STRING      driverDeviceName;
    UNICODE_STRING      numberString;
	SM_INIT_PARAMS		sm_Params;

	*Sftk_Lg = NULL;

	// We need to check if it exists already, if so say return error
#if TARGET_SIDE
	pSftk_LG = sftk_lookup_lg_by_lgnum( LG_Info->lgdev, LG_Info->lgCreationRole );
#else
    pSftk_LG = sftk_lookup_lg_by_lgnum( LG_Info->lgdev );
#endif
    if (pSftk_LG != NULL) 
    { // LG Already exist
		DebugPrint((DBG_ERROR, "sftk_Create_InitializeLGDevice: LG %d Already Exist, Failed with status 0x%08x !!! \n",
										LG_Info->lgdev, STATUS_GROUP_EXISTS));
		return STATUS_GROUP_EXISTS;	// LG exist return error
	}
	pSftk_LG = NULL;

    try 
    {
        try 
        {
            // initialize the temp unicode strings
            RtlInitUnicodeString(&numberString, NULL);
            RtlInitUnicodeString(&driverDeviceName, NULL);

            // populate the unicode string for the device number
            numberString.MaximumLength = sizeof(WCHAR)*10;
            numberString.Buffer = OS_AllocMemory( PagedPool, (numberString.MaximumLength + sizeof(UNICODE_NULL)) );
            if (!numberString.Buffer) 
            {
				DebugPrint((DBG_ERROR, "sftk_Create_InitializeLGDevice: OS_AllocMemory(size %d) for number string Failed, returning status = 0x%08x \n", 
										(numberString.MaximumLength + sizeof(UNICODE_NULL)), STATUS_INSUFFICIENT_RESOURCES));
                try_return(status = STATUS_INSUFFICIENT_RESOURCES); 
            }

            RtlIntegerToUnicodeString(LG_Info->lgdev, 10, &numberString);

            // create a device object representing the driver itself
            //  so that requests can be targeted to the driver ...
            driverDeviceName.MaximumLength = sizeof(FTD_DRV_DIR_LG_NAME) + numberString.Length;
            driverDeviceName.Buffer = OS_AllocMemory( PagedPool,(driverDeviceName.MaximumLength + sizeof(UNICODE_NULL)) );

            if (!driverDeviceName.Buffer) 
            {
                DebugPrint((DBG_ERROR, "sftk_Create_InitializeLGDevice: OS_AllocMemory(size %d) for LG Kernel Device Name, Failed, returning status = 0x%08x \n", 
										(driverDeviceName.MaximumLength + sizeof(UNICODE_NULL)), STATUS_INSUFFICIENT_RESOURCES));
                try_return(status = STATUS_INSUFFICIENT_RESOURCES); 
            }

            OS_ZeroMemory(driverDeviceName.Buffer, (driverDeviceName.MaximumLength + sizeof(UNICODE_NULL)) );
            RtlAppendUnicodeToString(&driverDeviceName, FTD_DRV_DIR_LG_NAME);	// "\\Devices\\DTC\\lg"
            RtlAppendUnicodeStringToString(&driverDeviceName, &numberString);	// logical group Number

            if (!NT_SUCCESS(status = IoCreateDevice(	DriverObject,
														sizeof(SFTK_LG),
														&driverDeviceName,
														FILE_DEVICE_UNKNOWN,    // For lack of anything better ?
														0,
														FALSE,					// Not exclusive.
														&deviceObject))) 
            { // failed to create a device object, 
                DebugPrint((DBG_ERROR, "sftk_Create_InitializeLGDevice: IoCreateDevice(LgDevice %S) Failed, returning status 0x%08x \n", 
							driverDeviceName.Buffer, status));

                try_return(status);
            }

            // Initialize the pSftk_LG for the device object.
            pSftk_LG = ( PSFTK_LG ) deviceObject->DeviceExtension;
			OS_ZeroMemory( pSftk_LG, sizeof(SFTK_LG) );

			RtlInitUnicodeString(&pSftk_LG->PStoreFileName, NULL);
			RtlInitUnicodeString(&pSftk_LG->UserVisibleName, NULL);

			pSftk_LG->DeviceObject = deviceObject;
			pSftk_LG->DriverObject = DriverObject;
			
			
			pSftk_LG->NodeId.NodeType	= NODE_TYPE_SFTK_LG;
			pSftk_LG->NodeId.NodeSize	= sizeof(SFTK_LG);
			pSftk_LG->state				= 0;
			pSftk_LG->flags				= 0;
			pSftk_LG->LGroupNumber		= LG_Info->lgdev;	// Logical group number supplied from Service

			// TODO Get this value from input LG_Info->MaxTransferUnit
			pSftk_LG->MaxTransferUnit		= DEFAULT_MAX_TRANSFER_UNIT_SIZE;
			OS_ASSERT((pSftk_LG->MaxTransferUnit % SECTOR_SIZE) == 0);

			// TODO Get this value from input LG_Info->NumOfAsyncRefreshIO
			pSftk_LG->NumOfAsyncRefreshIO	= DEFAULT_NUM_OF_ASYNC_REFRESH_IO;
			ANCHOR_InitializeListHead( pSftk_LG->RefreshIOPkts );
			pSftk_LG->WaitingRefreshNextBuffer	= FALSE;
			pSftk_LG->RefreshNextBuffer			= NULL;
			KeInitializeEvent( &pSftk_LG->RefreshPktsWaitEvent, NotificationEvent, FALSE);
			KeInitializeEvent( &pSftk_LG->ReleaseFreeAllPktsWaitEvent, NotificationEvent, FALSE);

			KeInitializeEvent( &pSftk_LG->RefreshEmptyAckQueueEvent, SynchronizationEvent, FALSE);
			KeInitializeEvent( &pSftk_LG->ReleasePoolDoneEvent, SynchronizationEvent, FALSE);
			
			KeInitializeEvent( &pSftk_LG->EventPacketsAvailableForRetrival, NotificationEvent, FALSE);
			
			OS_INITIALIZE_LOCK( &pSftk_LG->Lock, OS_ERESOURCE_LOCK, NULL);
			OS_INITIALIZE_LOCK( &pSftk_LG->AckLock, OS_ERESOURCE_LOCK, NULL);

			ANCHOR_InitializeListHead( pSftk_LG->LgDev_List );
			InitializeListHead( &pSftk_LG->Lg_GroupLink );

			pSftk_LG->statsize	= LG_Info->statsize;
			pSftk_LG->statbuf	= OS_AllocMemory( NonPagedPool, LG_Info->statsize );
			if (pSftk_LG->statbuf == NULL) 
			{
				DebugPrint((DBG_ERROR, "sftk_Create_InitializeLGDevice: OS_AllocMemory(size %d) for SFTK_LG.StateBuf Failed, returning status = 0x%08x \n", 
										LG_Info->statsize, STATUS_INSUFFICIENT_RESOURCES));
				try_return(status = STATUS_INSUFFICIENT_RESOURCES); 
			}

            // In order to allow user-space helper applications to access our
            // lg object for the ftd driver, create a symbolic link to
            // the object.
            pSftk_LG->UserVisibleName.MaximumLength = sizeof(FTD_DOS_DRV_DIR_LG_NAME) + numberString.Length;
            pSftk_LG->UserVisibleName.Buffer = OS_AllocMemory(NonPagedPool, (pSftk_LG->UserVisibleName.MaximumLength + sizeof(UNICODE_NULL)) );
            if (!pSftk_LG->UserVisibleName.Buffer) 
            {
                DebugPrint((DBG_ERROR, "sftk_Create_InitializeLGDevice: OS_AllocMemory(size %d) for User Visibile string Failed, returning status = 0x%08x \n", 
										(pSftk_LG->UserVisibleName.MaximumLength + sizeof(UNICODE_NULL)), STATUS_INSUFFICIENT_RESOURCES));

                try_return(status = STATUS_INSUFFICIENT_RESOURCES); 
            }

            OS_ZeroMemory(pSftk_LG->UserVisibleName.Buffer, (pSftk_LG->UserVisibleName.MaximumLength + sizeof(UNICODE_NULL)) );

            RtlAppendUnicodeToString(&pSftk_LG->UserVisibleName, FTD_DOS_DRV_DIR_LG_NAME);
            RtlAppendUnicodeStringToString(&pSftk_LG->UserVisibleName, &numberString);

            if (!NT_SUCCESS(status = IoCreateSymbolicLink(&pSftk_LG->UserVisibleName, &driverDeviceName))) 
			{	// failed to create a Symbolic Link for LG device object, 
                
				DebugPrint((DBG_ERROR, "sftk_Create_InitializeLGDevice: IoCreateSymbolicLink(DosDevice %S, KernelDevice %S) Failed with status 0x%08x \n", 
							pSftk_LG->UserVisibleName.Buffer, driverDeviceName.Buffer, status));

                try_return(status);
            }

			OS_SetFlag( pSftk_LG->flags, SFTK_LG_FLAG_SYMLINK_INITIALIZED);

			// Open PSTORE Device....

			//
			// Create and store unicode PSTORE File Name in SFTK_LG.
			//
			RtlCopyMemory( pSftk_LG->PStoreName, LG_Info->vdevname, sizeof(LG_Info->vdevname));
			RtlInitAnsiString( &ntString, LG_Info->vdevname);

			status = OS_AnsiStringToUnicodeString( &pSftk_LG->PStoreFileName, &ntString, TRUE);

			if (!NT_SUCCESS(status)) 
			{
				DebugPrint((DBG_ERROR, "sftk_Create_InitializeLGDevice: OS_AnsiStringToUnicodeString(PstoreFile Name %s) to allocate Unicode String Failed with status 0x%08x \n", 
							LG_Info->vdevname, status));

				try_return(status);
			}
    
			// Memory Manager init per LG if required.....
			OS_SetFlag( pSftk_LG->flags, SFTK_LG_FLAG_CACHE_INITIALIZED);

			// -------- TODO : Socket Manager Calls, Veera
			OS_SetFlag( pSftk_LG->flags, SFTK_LG_FLAG_SOCKET_INITIALIZED);
		
			// Now set the state of LG Device.
			// pSftk_LG->state			= SFTK_MODE_PASSTHRU;
			// create time LG, State mode default  get sets to Tracking mode.
#if TARGET_SIDE
			pSftk_LG->Role.CreationRole = LG_Info->lgCreationRole;

			// Create and store unicode Journal Path name in SFTK_LG->Role.
			RtlCopyMemory( pSftk_LG->Role.JPath, LG_Info->JournalPath, sizeof(LG_Info->JournalPath));
			RtlInitAnsiString( &ntString, LG_Info->JournalPath);

			status = OS_AnsiStringToUnicodeString( &pSftk_LG->Role.JPathUnicode, &ntString, TRUE);
			if (!NT_SUCCESS(status)) 
			{
				DebugPrint((DBG_ERROR, "sftk_Create_InitializeLGDevice: OS_AnsiStringToUnicodeString(JPathName %s) to allocate Unicode String Failed with status 0x%08x \n", 
							LG_Info->JournalPath, status));
				try_return(status);
			}
    
			pSftk_LG->Role.CurrentRole	 = pSftk_LG->Role.PreviouseRole = pSftk_LG->Role.CreationRole;
			pSftk_LG->Role.FailOver      = FALSE;
			pSftk_LG->Role.JEnable		 = FALSE;
			pSftk_LG->Role.JApplyRunning = FALSE;	

			KeInitializeEvent( &pSftk_LG->Role.JApplyWorkDoneEvent, SynchronizationEvent, FALSE);
#endif
			pSftk_LG->state						= SFTK_MODE_TRACKING;

			pSftk_LG->UserChangedToTrackingMode = TRUE;
			// The Pause Value is set to FALSE during FULL REFRESH, User can change this value to TRUE to
			// Pause the FULL REFRESH
			pSftk_LG->bPauseFullRefresh = FALSE;
			// At new LG create, this becomes TRUE, it remains FALSE till first time Smart Refresh gets started.
			pSftk_LG->bInconsistantData = TRUE;
			pSftk_LG->sync_depth	= SFTK_LG_SYNCH_DEPTH_DEFAULT;
			pSftk_LG->sync_timeout	= 0;
			pSftk_LG->iodelay		= 0;
			pSftk_LG->ndevs			= 0;
			pSftk_LG->dirtymaplag	= 0;
			
			pSftk_LG->throtal_refresh_send_pkts		= DEFAULT_ALLOWED_NUM_OF_PKTS_PENDING_FOR_REFRESH;
			pSftk_LG->throtal_refresh_send_totalsize= DEFAULT_ALLOWED_MAX_SIZE_PENDING_FOR_REFRESH;
			pSftk_LG->throtal_commit_send_pkts		= DEFAULT_ALLOWED_NUM_OF_PKTS_PENDING_FOR_COMMIT;
			pSftk_LG->throtal_commit_send_totalsize = DEFAULT_ALLOWED_MAX_SIZE_PENDING_FOR_COMMIT;
			pSftk_LG->NumOfPktsSendAtaTime			= DEFAULT_NUM_OF_PKTS_SEND_AT_A_TIME;
			pSftk_LG->NumOfPktsRecvAtaTime			= DEFAULT_NUM_OF_PKTS_RECV_AT_A_TIME;
			pSftk_LG->NumOfSendBuffers				= DEFAULT_MAX_SEND_BUFFERS;
			pSftk_LG->MaxOutBandPktWaitTimeout.QuadPart= DEFAULT_TIMEOUT_FOR_MAX_WAIT_FOR_OUTBAND_PKT;

			// Add this created LG device into Global Anchor Group List
			OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
#if TARGET_SIDE
			if (pSftk_LG->Role.CreationRole == PRIMARY)
			{
				OS_ASSERT(pSftk_LG->Role.CreationRole == PRIMARY);
				InsertTailList( &GSftk_Config.Lg_GroupList.ListEntry, &pSftk_LG->Lg_GroupLink );
				GSftk_Config.Lg_GroupList.NumOfNodes ++;
			}
			else
			{
				OS_ASSERT(pSftk_LG->Role.CreationRole == SECONDARY);
				InsertTailList( &GSftk_Config.TLg_GroupList.ListEntry, &pSftk_LG->Lg_GroupLink );
				GSftk_Config.TLg_GroupList.NumOfNodes ++;
			}
#else
			InsertTailList( &GSftk_Config.Lg_GroupList.ListEntry, &pSftk_LG->Lg_GroupLink );
			GSftk_Config.Lg_GroupList.NumOfNodes ++;
#endif
			OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
			OS_SetFlag( pSftk_LG->flags, SFTK_LG_FLAG_ADDED_TO_LGDEVICE_ANCHOR);
		
			if (CreatePstoreFile == TRUE)
			{ // Create and Format PSTORE File for current SFTK_LG.
				// we create Specify Pstore File for current LG, 
				// While creating we must Format and write Pstore file too
				// As new device gets added we also update this pstore file.
				status = sftk_format_pstorefile( pSftk_LG);
				if (!NT_SUCCESS(status)) 
				{
					DebugPrint((DBG_ERROR, "sftk_Create_InitializeLGDevice: sftk_format_pstorefile( LG %d PstoreFile Name %S) Failed with status 0x%08x \n", 
								pSftk_LG->LGroupNumber, pSftk_LG->PStoreFileName.Buffer, status));
					// Not Needed to do Log Messaged since we are failing operation right away...
					try_return(status);
				}
				OS_SetFlag( pSftk_LG->flags, SFTK_LG_FLAG_PSTORE_FILE_CREATED);
			}

			if (CreateRegistry == TRUE)
			{ // Create Registry key for new LG
				status = sftk_lg_Create_RegKey( pSftk_LG );
				if (!NT_SUCCESS(status)) 
				{
					DebugPrint((DBG_ERROR, "sftk_Create_InitializeLGDevice: sftk_lg_Create_RegKey( LG %d PstoreFile Name %S) Failed with status 0x%08x \n", 
								pSftk_LG->LGroupNumber, pSftk_LG->PStoreFileName.Buffer, status));
					// Not Needed to do Log Messaged since we are failing operation right away...
					try_return(status);
				}
				OS_SetFlag( pSftk_LG->flags, SFTK_LG_FLAG_REG_CREATED);
			}
			
			// Starts Required thread Group Based......
			KeInitializeEvent( &pSftk_LG->EventRefreshWorkStop, SynchronizationEvent, FALSE);
			KeInitializeEvent( &pSftk_LG->EventAckFinishBitmapPrep, SynchronizationEvent, FALSE);
			KeInitializeEvent( &pSftk_LG->Event_LGFreeAllMemOfMM, SynchronizationEvent, FALSE);
			
			status = sftk_Create_LGThread( pSftk_LG, LG_Info );
			if (!NT_SUCCESS(status)) 
			{
				DebugPrint((DBG_ERROR, "sftk_Create_InitializeLGDevice: sftk_Create_LGThread(LG DeviceID %d) Failed with status 0x%08x \n", 
							pSftk_LG->LGroupNumber, status));

				try_return(status);
			}
			OS_SetFlag( pSftk_LG->flags, SFTK_LG_FLAG_STARTED_LG_THREADS);

			// initialize session manager for new LG
			RtlZeroMemory( &sm_Params, sizeof(sm_Params));

			// set all values as default....TODO: Service must have passed this default values during LG create !!
			sm_Params.lgnum						= pSftk_LG->LGroupNumber;
			sm_Params.nSendWindowSize			= pSftk_LG->MaxTransferUnit;	// DEFAULT_MAX_TRANSFER_UNIT_SIZE = 256 K
			sm_Params.nMaxNumberOfSendBuffers	= DEFAULT_MAX_SEND_BUFFERS;	// 5 defined in ftdio.h
			sm_Params.nReceiveWindowSize		= pSftk_LG->MaxTransferUnit;	
			sm_Params.nMaxNumberOfReceiveBuffers= DEFAULT_MAX_RECEIVE_BUFFERS;	// 5 defined in ftdio.h
			sm_Params.nChunkSize				= 0;	 
			sm_Params.nChunkDelay				= 0;	 

			status = COM_InitializeSessionManager( &pSftk_LG->SessionMgr, &sm_Params);
			if (!NT_SUCCESS(status))
			{ // socket connection failed....bumer...called mike to fix this batch process error handling....
				DebugPrint((DBG_ERROR, "sftk_Create_InitializeLGDevice: COM_InitializeSessionManager(LG DeviceID %d) Failed with status 0x%08x \n", 
							pSftk_LG->LGroupNumber, status));

				try_return(status);
			} // socket connection failed....bumer...called mike to fix this batch process error handling....

			OS_ASSERT(pSftk_LG->SessionMgr.pLogicalGroupPtr == pSftk_LG);
			
            // We do this whenever device objects are create on-the-fly (i.e. not as
            // part of driver initialization).
            deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
			
			QM_Init(pSftk_LG);

			pSftk_LG->ConfigStart = TRUE;	// Created new one
			status = STATUS_SUCCESS;	// we are done.

        }  // try - except
        except ( sftk_ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
        {
			// Log event message here
			sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
									0, __FILE__, __LINE__, GetExceptionCode());

            // We encountered an exception somewhere, eat it up.
            // FTD_ERR(FTD_WRNLVL, "ftd_nt_add_lg() : EXCEPTION_EXECUTE_HANDLER, status = %x.", GetExceptionCode());
			
			// Log event message here
			sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
									0, __FILE__, __LINE__, GetExceptionCode());

			DebugPrint((DBG_ERROR, "sftk_Create_InitializeLGDevice(): EXCEPTION_EXECUTE_HANDLER, status = %x \n", 
							GetExceptionCode()));
        }

        try_exit:   NOTHING;
    }  // try 
    finally 
    {
		if( numberString.Buffer )
			OS_FreeMemory ( numberString.Buffer );

		if( driverDeviceName.Buffer )
			OS_FreeMemory ( driverDeviceName.Buffer );

        // Start unwinding if we were unsuccessful.
        if (!NT_SUCCESS(status)) 
        {
			NTSTATUS	tmpStatus;

            if (pSftk_LG)
            {
				if ( OS_IsFlagSet( pSftk_LG->flags, SFTK_LG_FLAG_ADDED_TO_LGDEVICE_ANCHOR) ) 
				{ // Remove from LG Device List Anchor
					OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
					RemoveEntryList( &pSftk_LG->Lg_GroupLink );
#if TARGET_SIDE
					if (pSftk_LG->Role.CreationRole == PRIMARY)
					{
						GSftk_Config.Lg_GroupList.NumOfNodes --;
					}
					else
					{
						OS_ASSERT(pSftk_LG->Role.CreationRole == SECONDARY);
						GSftk_Config.TLg_GroupList.NumOfNodes --;
					}
#else
					GSftk_Config.Lg_GroupList.NumOfNodes --;
#endif
					OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
					InitializeListHead( &pSftk_LG->Lg_GroupLink );
				}

				if ( OS_IsFlagSet( pSftk_LG->flags, SFTK_LG_FLAG_STARTED_LG_THREADS) ) 
				{ // Terminate Thread for LG
					sftk_Terminate_LGThread( pSftk_LG );
				}

				if(pSftk_LG->statbuf)
					OS_FreeMemory( pSftk_LG->statbuf );

				if ( OS_IsFlagSet( pSftk_LG->flags, SFTK_LG_FLAG_SYMLINK_INITIALIZED) ) 
                    IoDeleteSymbolicLink(&pSftk_LG->UserVisibleName);

                if (pSftk_LG->UserVisibleName.Buffer != NULL) 
					OS_FreeMemory( pSftk_LG->UserVisibleName.Buffer );

				if ( OS_IsFlagSet( pSftk_LG->flags, SFTK_LG_FLAG_PSTORE_FILE_CREATED) ) 
				{
					tmpStatus = sftk_delete_pstorefile( pSftk_LG);
					if (!NT_SUCCESS(tmpStatus)) 
					{
						DebugPrint((DBG_ERROR, "sftk_Create_InitializeLGDevice: sftk_delete_pstorefile( LG %d PstoreFile Name %S) Failed with tmpStatus 0x%08x \n", 
									pSftk_LG->LGroupNumber, pSftk_LG->PStoreFileName.Buffer, tmpStatus));
					}
				}

				if ( OS_IsFlagSet( pSftk_LG->flags, SFTK_LG_FLAG_REG_CREATED) ) 
				{
					tmpStatus = sftk_lg_Delete_RegKey( pSftk_LG);
					if (!NT_SUCCESS(tmpStatus)) 
					{
						DebugPrint((DBG_ERROR, "sftk_Create_InitializeLGDevice: sftk_lg_Delete_RegKey( LG %d PstoreFile Name %S) Failed with tmpStatus 0x%08x \n", 
									pSftk_LG->LGroupNumber, pSftk_LG->PStoreFileName.Buffer, tmpStatus));
					}
				}

				if(pSftk_LG->PStoreFileName.Buffer)
					RtlFreeUnicodeString( &pSftk_LG->PStoreFileName );

#if TARGET_SIDE
				if(pSftk_LG->Role.JPathUnicode.Buffer)
					RtlFreeUnicodeString( &pSftk_LG->Role.JPathUnicode );
#endif
				if ( OS_IsFlagSet( pSftk_LG->flags, SFTK_LG_FLAG_CACHE_INITIALIZED) ) 
				{
					// Memory Manager cleanup if needed...
                    DebugPrint((DBG_ERROR, "FIXME FIXME : sftk_Create_InitializeLGDevice: Call Reverse of RplCCLgInit() to DeInit Logical Group From Cache Manaher.... \n"));
				}

				if ( OS_IsFlagSet( pSftk_LG->flags, SFTK_LG_FLAG_SOCKET_INITIALIZED) ) 
				{
					// TODO - Socket Manager, Veera
                    DebugPrint((DBG_ERROR, "FIXME FIXME : sftk_Create_InitializeLGDevice: Call DeInit Socket per Group based, if itrs needed...while deltion of Logical Group ... \n"));
				}

                // Now, delete any device objects, etc. we may have created
                if (pSftk_LG->DeviceObject) 
                    IoDeleteDevice(pSftk_LG->DeviceObject);

            }
			pSftk_LG = NULL;
        } // if (!NT_SUCCESS(status)) 

    } // finally 

	*Sftk_Lg = pSftk_LG;
	// initialize returned pointer

    return(status);
} // sftk_Create_InitializeLGDevice()

#if TARGET_SIDE
PSFTK_LG
sftk_lookup_lg_by_lgnum(IN ULONG LgNumber, ROLE_TYPE RoleType)
#else
PSFTK_LG
sftk_lookup_lg_by_lgnum(IN ULONG LgNumber)
#endif
{
	PSFTK_LG		pLg			= NULL;
	PLIST_ENTRY		plistEntry	= NULL;
#if TARGET_SIDE
	PANCHOR_LINKLIST	pLGList  = NULL;
#endif

	// typedef dev_t           minor_t;
    // minor_t        minor_num;
	// minor_t        minor_num = getminor(sb.lg_num); 
	OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_SHARED, TRUE, NULL);

#if TARGET_SIDE
	if (RoleType == PRIMARY)
		pLGList = &GSftk_Config.Lg_GroupList;
	else
		pLGList = &GSftk_Config.TLg_GroupList;

	for( plistEntry = pLGList->ListEntry.Flink;
		 plistEntry != &pLGList->ListEntry;
		 plistEntry = plistEntry->Flink )
#else
	for( plistEntry = GSftk_Config.Lg_GroupList.ListEntry.Flink;
		 plistEntry != &GSftk_Config.Lg_GroupList.ListEntry;
		 plistEntry = plistEntry->Flink )
#endif
	{ // for :scan thru each and every logical group list 
		pLg = CONTAINING_RECORD( plistEntry, SFTK_LG, Lg_GroupLink);

		OS_ASSERT(pLg->NodeId.NodeType == NODE_TYPE_SFTK_LG);
#if TARGET_SIDE
		if (RoleType == PRIMARY)
		{
			OS_ASSERT(pLg->Role.CreationRole == PRIMARY);
		}
		else
		{
			OS_ASSERT(pLg->Role.CreationRole == SECONDARY);
		}
#endif
		if (pLg->LGroupNumber == LgNumber)
		{ // Found it
			OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
			return pLg;
		}
	} // for : scan thru each and every logical group list 

	OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
	return NULL;
} // sftk_lookup_lg_by_lgnum

/*
 * Open the logical group control device.  We allow only one open at a time
 * as a way of enforcing in the kernel that there is only one pmd per 
 * logical group at any given time.
 */
NTSTATUS
sftk_lg_open( PSFTK_LG Sftk_LG )
{
#if DBG
	// Double Check that this LG Group device is in Global Link List or not...
#if TARGET_SIDE
	if ( Sftk_LG == sftk_lookup_lg_by_lgnum( Sftk_LG->LGroupNumber, Sftk_LG->Role.CreationRole) ) 
#else
    if ( Sftk_LG == sftk_lookup_lg_by_lgnum( Sftk_LG->LGroupNumber) ) 
#endif
    {
		DebugPrint((DBG_ERROR, "FIXME FIXME : sftk_lg_open: Mismatch of LGNumber exist in Global SFTK_CONFIG->LGDeviceList... \n"));
        // return (ENXIO);         /* invalid minor number */
    }
#endif

    OS_ACQUIRE_LOCK( &Sftk_LG->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	if ( !(OS_IsFlagSet( Sftk_LG->flags, SFTK_LG_FLAG_OPENED_EXCLUSIVE)) ) 
    {
		OS_SetFlag( Sftk_LG->flags, SFTK_LG_FLAG_OPENED_EXCLUSIVE);
    }
	else
	{
		DebugPrint((DBG_ERROR, "sftk_lg_open: Current LG Device %d is already opened by Service, Previously Exclusive Opened only allowed, we allowed now multiple open, so returning success!!! \n"));
		// just ignore this  why do we have rules like this ??? not needed
        // return STATUS_SHARING_VIOLATION; 
	}

	Sftk_LG->Lg_OpenCount ++;	// increment open count for specified LG
    OS_RELEASE_LOCK( &Sftk_LG->Lock, NULL);

	// Increments Global LG_Open count, since we are LG opening device.
	OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
    GSftk_Config.Lg_OpenCount ++;
    OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);

	if (Sftk_LG->Event == NULL)
		sftk_lg_create_named_event( Sftk_LG );	// create named event here.

    return STATUS_SUCCESS;
} // sftk_lg_open()

/*
 * Close logical group.  This usually means that the pmd has
 * gone away.
 */
NTSTATUS
sftk_lg_close( PSFTK_LG Sftk_LG )
{
#if DBG
	// Double Check that this LG Group device is in Global Link List or not...
#if TARGET_SIDE
	if ( Sftk_LG == sftk_lookup_lg_by_lgnum( Sftk_LG->LGroupNumber, Sftk_LG->Role.CreationRole) ) 
#else
    if ( Sftk_LG == sftk_lookup_lg_by_lgnum( Sftk_LG->LGroupNumber ) ) 
#endif
    {
		DebugPrint((DBG_ERROR, "FIXME FIXME : sftk_lg_open: Mismatch of LGNumber exist in Global SFTK_CONFIG->LGDeviceList... \n"));
    }
#endif

    OS_ACQUIRE_LOCK( &Sftk_LG->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	Sftk_LG->Lg_OpenCount --;	// decrement open count for specified LG
	if (Sftk_LG->Lg_OpenCount == 0)
		OS_ClearFlag( Sftk_LG->flags, SFTK_LG_FLAG_OPENED_EXCLUSIVE);	// clear flag if count gets zero
    OS_RELEASE_LOCK( &Sftk_LG->Lock, NULL);

	// Increments Global LG_Open count, since we are LG opening device.
	OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
    GSftk_Config.Lg_OpenCount --;
    OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);

    // sftk_lg_close_named_event( Sftk_LG );

    return STATUS_SUCCESS;
} // sftk_lg_open()

NTSTATUS
sftk_lg_create_named_event( PSFTK_LG Sftk_LG )
{
    NTSTATUS            status = STATUS_SUCCESS;
    UNICODE_STRING      lgNumberString;
    UNICODE_STRING      eventName;

	try 
    {
        try 
        {
			RtlInitUnicodeString(&lgNumberString, NULL);
			RtlInitUnicodeString(&eventName, NULL);

            // initialize the unicode string for the device number
            lgNumberString.MaximumLength = sizeof(WCHAR)*4;
			
            lgNumberString.Buffer = OS_AllocMemory(PagedPool,(sizeof(WCHAR)*3 + sizeof(UNICODE_NULL)) );
            if (!lgNumberString.Buffer) 
            {
                DebugPrint((DBG_ERROR, "sftk_lg_create_named_event: OS_AllocMemory( NumString: size %d ) Failed for LG Device %d, returning 0x%08x !!! \n",
										(sizeof(WCHAR)*4), Sftk_LG->LGroupNumber, STATUS_INSUFFICIENT_RESOURCES));

                try_return(status = STATUS_INSUFFICIENT_RESOURCES); 
            }

            RtlIntegerToUnicodeString( Sftk_LG->LGroupNumber, 10, &lgNumberString);

            
            eventName.MaximumLength = sizeof(FTD_LG_NUM_EVENT_NAME) + lgNumberString.Length + sizeof(FTD_LG_EVENT_NAME);
            eventName.Buffer = OS_AllocMemory( PagedPool,(eventName.MaximumLength + sizeof(UNICODE_NULL)) );
            if (!eventName.Buffer) 
            {
                DebugPrint((DBG_ERROR, "sftk_lg_create_named_event: OS_AllocMemory( size %d ) Failed for LG Device %d, returning 0x%08x !!! \n",
										(eventName.MaximumLength + sizeof(UNICODE_NULL)), 
										Sftk_LG->LGroupNumber, 
										STATUS_INSUFFICIENT_RESOURCES));

                try_return(status = STATUS_INSUFFICIENT_RESOURCES); 
            }

            RtlZeroMemory( eventName.Buffer, (eventName.MaximumLength + sizeof(UNICODE_NULL)) );

            RtlAppendUnicodeToString(&eventName,		FTD_LG_NUM_EVENT_NAME);	// L"\\BaseNamedObjects\\DTClg"
            RtlAppendUnicodeStringToString(&eventName,	&lgNumberString);		// LGNumber
            RtlAppendUnicodeToString(&eventName,		FTD_LG_EVENT_NAME);		// L"bab"

            Sftk_LG->Event = IoCreateSynchronizationEvent( &eventName, &Sftk_LG->hEvent ) ;
            if (Sftk_LG->Event == NULL) 
            {
                DebugPrint((DBG_ERROR, "sftk_lg_create_named_event: IoCreateSynchronizationEvent() Failed for LG Device %d, returning 0x%08x !!! \n",
										Sftk_LG->LGroupNumber,	STATUS_INSUFFICIENT_RESOURCES));

                try_return(status = STATUS_INSUFFICIENT_RESOURCES); 
            }

			// Do we need lock ? !! its ok we do not need it here causes its safe either way.
			OS_ACQUIRE_LOCK( &Sftk_LG->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
			OS_SetFlag( Sftk_LG->flags, SFTK_LG_FLAG_NAMED_EVENT_CREATED);
			OS_RELEASE_LOCK( &Sftk_LG->Lock, NULL);
        } 
        except (sftk_ExceptionFilter(GetExceptionInformation(), GetExceptionCode()) ) 
        {
			// Log event message here
			sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
									0, __FILE__, __LINE__, GetExceptionCode());
			// We encountered an exception somewhere, eat it up.
			// Log event message here
			sftk_LogEventChar1Num2(GSftk_Config.DriverObject, MSG_REPL_DRIVER_EXCEPTION_OCCURRED, GetExceptionCode(), 
									0, __FILE__, __LINE__, GetExceptionCode());

			DebugPrint((DBG_ERROR, "sftk_lg_create_named_event(): EXCEPTION_EXECUTE_HANDLER, status = %x \n", 
							GetExceptionCode()));
        }
		try_exit:   NOTHING;
    } 
    finally 
    {
        // Start unwinding if we were unsuccessful.
		if (eventName.Buffer != NULL) 
            OS_FreeMemory( eventName.Buffer );

        if (lgNumberString.Buffer != NULL) 
            OS_FreeMemory( lgNumberString.Buffer );

        if (!NT_SUCCESS(status)) 
            sftk_lg_close_named_event(Sftk_LG, TRUE);

    }

    return(status);
} // sftk_lg_create_named_event()

NTSTATUS
sftk_lg_close_named_event( PSFTK_LG Sftk_LG, BOOLEAN bGrabLgLock)
{
	if (bGrabLgLock == TRUE)
		OS_ACQUIRE_LOCK( &Sftk_LG->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

    if OS_IsFlagSet( Sftk_LG->flags, SFTK_LG_FLAG_NAMED_EVENT_CREATED) 
        ZwClose( Sftk_LG->hEvent );

	Sftk_LG->hEvent = NULL;
	Sftk_LG->Event	= NULL;
            
    OS_ClearFlag( Sftk_LG->flags, SFTK_LG_FLAG_NAMED_EVENT_CREATED);

	if (bGrabLgLock == TRUE)
		OS_RELEASE_LOCK( &Sftk_LG->Lock, NULL);
        
    return STATUS_SUCCESS;
} // sftk_lg_close_named_event()


// NOTE: if UserRequested == TRUE means Calls is from IOCTL from service
// If its FALSE and newState == TRACKING_MODE than Lock is already acquired due to BAB OverFlow case.
NTSTATUS
sftk_lg_change_State( PSFTK_LG	Sftk_Lg, LONG  PrevState, LONG  NewState, BOOLEAN	UserRequested)
{
	NTSTATUS		status;
	PLIST_ENTRY		plistEntry;
	PSFTK_DEV		pSftkDev;
	ULONG			totalQMPkts;
	WCHAR			wStr1[64], wStr2[64], wStr3[64], wStr4[128];
	HANDLE			fileHandle		= NULL;
	BOOLEAN			bHandleValid	= FALSE;
	OS_PERF;

#if TARGET_SIDE
	if (LG_IS_SECONDARY_MODE(Sftk_Lg))
	{
		DebugPrint((DBG_ERROR, "sftk_lg_change_State : BUG FIXME FIXME : LG %d PrevState 0x%08x NewState 0x%08x, is in Secondary MODE, Caller must not call this API !! FIXME FIXME \n", 
								Sftk_Lg->LGroupNumber,  PrevState, NewState)); 
		OS_ASSERT(FALSE);
	}
#endif

	if (PrevState == NewState)
	{
		if (PrevState == SFTK_MODE_TRACKING)
		{
			DebugPrint((DBG_ERROR, "sftk_lg_change_State : LG %d PrevState 0x%08x NewState 0x%08x\n", 
								Sftk_Lg->LGroupNumber,  PrevState, NewState)); 

			OS_ASSERT( Sftk_Lg->QueueMgr.CommitList.NumOfNodes == 0);
			OS_ASSERT( IsListEmpty(&Sftk_Lg->QueueMgr.CommitList.ListEntry) == TRUE);

			OS_ASSERT( Sftk_Lg->QueueMgr.PendingList.NumOfNodes == 0);
			OS_ASSERT( IsListEmpty(&Sftk_Lg->QueueMgr.PendingList.ListEntry) == TRUE);

			OS_ASSERT( Sftk_Lg->QueueMgr.RefreshList.NumOfNodes == 0);
			OS_ASSERT( IsListEmpty(&Sftk_Lg->QueueMgr.RefreshList.ListEntry) == TRUE);

			OS_ASSERT( Sftk_Lg->QueueMgr.MigrateList.NumOfNodes == 0);
			OS_ASSERT( IsListEmpty(&Sftk_Lg->QueueMgr.MigrateList.ListEntry) == TRUE);
		}
		return STATUS_SUCCESS;
	}

	if (UserRequested == FALSE)
	{
		if ( (PrevState == SFTK_MODE_PASSTHRU) && (NewState == SFTK_MODE_TRACKING))
		{
			status = STATUS_INVALID_DEVICE_REQUEST;
			DebugPrint((DBG_ERROR, "sftk_lg_change_State : NonUser call PASSTHRU to TRACKING is not allowed, Failing status 0x%08x \n", 
								status)); 
			OS_ASSERT(FALSE);
			return status;
		}
	}

	// we can not change the state till MM is started if new state requires QM
	if ( (GSftk_Config.Mmgr.MM_Initialized == FALSE) &&
		 (	(NewState == SFTK_MODE_NORMAL) || 
			(NewState == SFTK_MODE_FULL_REFRESH) ||
			(NewState == SFTK_MODE_SMART_REFRESH) ) )
	{ // return invalid request can't change state
		status = STATUS_INVALID_DEVICE_REQUEST;

		// LG State change error log event message
		swprintf( wStr1, L"%d", Sftk_Lg->LGroupNumber);
		sftk_get_lg_state_string(PrevState, wStr2);
		sftk_get_lg_state_string(NewState, wStr3);
		swprintf( wStr4, L"MM Not Initialized");
		sftk_LogEventWchar4(	GSftk_Config.DriverObject, MSG_REPL_STATE_CHANGE_ERROR, status, 0, 
								wStr1, wStr2, wStr3, wStr4);
					
		DebugPrint((DBG_ERROR, "sftk_lg_change_State:: PrevState 0x%08x NewState 0x%08x, LG Num 0x%08x Failed with status 0x%08x due to MM is not initialized!\n", 
								PrevState, NewState, Sftk_Lg->LGroupNumber, status ));
		return status;
	}

	// we can not change the state till MM is started if new state requires QM
	if (sftk_lg_is_socket_alive( Sftk_Lg ) == FALSE)
	{ // connection is not active.....
		if ((NewState == SFTK_MODE_NORMAL)			|| 
			(NewState == SFTK_MODE_FULL_REFRESH)	|| 
			(NewState == SFTK_MODE_SMART_REFRESH)	|| 
			(NewState == SFTK_MODE_BACKFRESH) ) 
		{ // return invalid request can't change state, if can't established connections
			SM_INIT_PARAMS	sm_Params;

			RtlZeroMemory( &sm_Params, sizeof(sm_Params));

			// set all values as default....TODO: Service must have passed this default values during LG create !!
			sm_Params.lgnum						= Sftk_Lg->LGroupNumber;
			sm_Params.nSendWindowSize			= CONFIGURABLE_MAX_SEND_BUFFER_SIZE( Sftk_Lg->MaxTransferUnit, Sftk_Lg->NumOfPktsSendAtaTime);	
			sm_Params.nMaxNumberOfSendBuffers	= DEFAULT_MAX_SEND_BUFFERS;	// 5 defined in ftdio.h
			sm_Params.nReceiveWindowSize		= CONFIGURABLE_MAX_RECIEVE_BUFFER_SIZE( Sftk_Lg->MaxTransferUnit,Sftk_Lg->NumOfPktsRecvAtaTime);
			sm_Params.nMaxNumberOfReceiveBuffers= DEFAULT_MAX_RECEIVE_BUFFERS;	// 5 defined in ftdio.h
			sm_Params.nChunkSize				= 0;	 
			sm_Params.nChunkDelay				= 0;	 

			status = COM_StartPMD( &Sftk_Lg->SessionMgr, &sm_Params);
			if (!NT_SUCCESS(status))
			{ // socket connection failed....bumer...called mike to fix this batch process error handling....
				DebugPrint((DBG_ERROR, "sftk_lg_change_State : SFTK_IOCTL_START_PMD: COM_StartPMD(LgNum %d) Failed with status 0x%08x \n", 
								sm_Params.lgnum, status)); 
				
				// Log event message here for connection failure
				swprintf( wStr1, L"%d", Sftk_Lg->LGroupNumber);
				swprintf( wStr2, L"0x%08X", status );
				sftk_LogEventString2(	GSftk_Config.DriverObject, MSG_LG_START_SOCKET_CONNECTION_ERROR, status, 0, 
										wStr1, wStr2);

				// LG State change error log event message
				swprintf( wStr1, L"%d", Sftk_Lg->LGroupNumber);
				sftk_get_lg_state_string(PrevState, wStr2);
				sftk_get_lg_state_string(NewState, wStr3);
				swprintf( wStr4, L"Connection");
				sftk_LogEventWchar4(	GSftk_Config.DriverObject, MSG_REPL_STATE_CHANGE_ERROR, status, 0, 
										wStr1, wStr2, wStr3, wStr4);

				// status = STATUS_INVALID_DEVICE_REQUEST;
				// status = STATUS_SUCCESS;	// ignore this error.....FIX ME, Can't do anything here.....
				DebugPrint((DBG_ERROR, "sftk_lg_change_State:: PrevState 0x%08x NewState 0x%08x, LG Num 0x%08x Failed with status 0x%08x due to Socket Conection failed!\n", 
								PrevState, NewState, Sftk_Lg->LGroupNumber, status ));
				return status;
				
			} // socket connection failed....bumer...called mike to fix this batch process error handling....
		} // return invalid request can't change state, if can't established connections
	} // connection is not active.....

	if (OS_IsFlagSet( Sftk_Lg->flags, SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE) == FALSE)
	{
		// Open pstore file so we can flush new state and other required info
		status = sftk_open_pstore( &Sftk_Lg->PStoreFileName , &fileHandle, FALSE);
		if ( !NT_SUCCESS(status) ) 
		{ // failed to open or create pstore file...
			DebugPrint((DBG_ERROR, "sftk_lg_change_State:: sftk_open_pstore(%S, LG Num 0x%08x ) Failed with status 0x%08x !\n", 
									Sftk_Lg->PStoreFileName.Buffer, Sftk_Lg->LGroupNumber, status ));
			DebugPrint((DBG_ERROR, "TODO FIXME FIXME :: Fix Error Handling, for time being we just continue the operations...\n"));

			// Log Event, and error handling, we should terminate here ??? or just continue ... ???
			swprintf(  wStr4, L"Change Of LG %d State Update",	Sftk_Lg->LGroupNumber);
			sftk_LogEventString2( GSftk_Config.DriverObject, MSG_REPL_PSTORE_OPEN_ERROR, status, 
								  0, Sftk_Lg->PStoreFileName.Buffer, wStr4);
			// ignore error
			bHandleValid = FALSE;
			fileHandle = NULL;
		}
		else
		{
			bHandleValid = TRUE;
		}
	}

	if (UserRequested == TRUE)
	{
		if (Sftk_Lg->UserChangedToTrackingMode == TRUE) 
		{
			if ((NewState == SFTK_MODE_SMART_REFRESH) && (Sftk_Lg->bInconsistantData == TRUE))
			{ // At new LG create, this becomes TRUE, it remains FALSE till first time Smart Refresh gets started.
				Sftk_Lg->bInconsistantData = FALSE;
			}
		}

		if (NewState == SFTK_MODE_TRACKING)
			Sftk_Lg->UserChangedToTrackingMode = TRUE;	// First mark request is from User 
		else 
			Sftk_Lg->UserChangedToTrackingMode = FALSE;
		
		Sftk_Lg->SRefreshWasNotDone		   = FALSE;
	}
	else
	{
		Sftk_Lg->UserChangedToTrackingMode = FALSE;

		if ((NewState == SFTK_MODE_SMART_REFRESH) && (Sftk_Lg->bInconsistantData == TRUE))
		{ // At new LG create, this becomes TRUE, it remains FALSE till first time Smart Refresh gets started.
			Sftk_Lg->bInconsistantData = FALSE;
		}
	}

#if DBG
	sftk_get_stateString(PrevState, wStr2); sftk_get_stateString(NewState, wStr3);
	DebugPrint((DBG_ERROR, "sftk_lg_change_state: Changing state: PrevState %x %S, NewState: %x %S \n",PrevState, wStr2, NewState, wStr3));
#endif
/*
#if 0		
	if ( (NewState == SFTK_MODE_NORMAL) || 
		 (NewState == SFTK_MODE_FULL_REFRESH) || 
		 (NewState == SFTK_MODE_SMART_REFRESH) || 
		 (NewState == SFTK_MODE_BACKFRESH) )
	{ // Connection must be ON, before we do change to these changes.....
		if (sftk_lg_is_socket_alive(Sftk_Lg) == FALSE)
		{
			DebugPrint((DBG_ERROR, "FIXME FIXME *** sftk_lg_change_State:: Changing to QM Used State, TDI Connection is Not ON, We must failed change state here *** FIXME FIXME !!!!\n", 
								Sftk_Lg->PStoreFileName.Buffer, Sftk_Lg->LGroupNumber, status ));
			// return error;
		}
	}
#endif
*/
	// Reset the Pause Flag whenever there is a state change
	Sftk_Lg->bPauseFullRefresh = FALSE;

	switch (NewState)
	{
		case SFTK_MODE_TRACKING: 
				{
					if (PrevState == SFTK_MODE_FULL_REFRESH) 
					{
						DebugPrint((DBG_ERROR, "BUG FIXME FIXME sftk_lg_change_state:: Must not change the state like this, not allowed, PrevState %x SFTK_MODE_FULL_REFRESH, Newstate %x SFTK_MODE_TRACKING ...BUG FIXME FIXME \n",
												PrevState, NewState));
						OS_ASSERT(FALSE);
					}

					if ( (PrevState == SFTK_MODE_SMART_REFRESH) || (PrevState == SFTK_MODE_NORMAL) )
					{ // BAB is used in previouse state so we need to rebuild HRDB for racking mode changes
						OS_PERF_STARTTIME;
						QM_GetTotalPkts( Sftk_Lg, totalQMPkts);
	
						if (UserRequested == TRUE)
						{ // if UserRequested Grab lock if its FALSE means caller already has lock acquired for this new state transition
							if (Sftk_Lg->LgDev_List.NumOfNodes > 1)
								OS_ACQUIRE_LOCK( &Sftk_Lg->AckLock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
						}

						Sftk_Lg->CacheOverFlow = TRUE;	// Mark this TRUE so Ack thread will clean this up.
						KeClearEvent( &Sftk_Lg->Event_LGFreeAllMemOfMM );
#if DBG
						if (PrevState == SFTK_MODE_NORMAL)
						{ // Double Check LastBitindex was properly set in this mode....
							if (sftk_match_lg_dev_lastBitIndex(Sftk_Lg, DEV_LASTBIT_CLEAN_ALL) == FALSE) 
							{
								DebugPrint((DBG_ERROR, "sftk_dev_master_thread:: BUG FIXME FIXME : LG %d,NORMAL MODE Cache Overflow found, but sftk_match_lg_dev_lastBitIndex() returns FALSE !!!!\n", 
											Sftk_Lg->LGroupNumber ));
								OS_ASSERT(FALSE);	
							}
						}
#endif

						if (PrevState == SFTK_MODE_SMART_REFRESH)
						{ // for safe side Reset events for Smart Refresh
							KeClearEvent( &Sftk_Lg->EventRefreshWorkStop);
						}

						// Now change the state mode.
						Sftk_Lg->PrevState = Sftk_Lg->state;	// Save Prev State, need it for protocol
						Sftk_Lg->state = NewState;	// Its Tracking mode state

						if (PrevState != NewState)
						{ // Signal change of state 
							// signal Smart Refresh about change in state, if smart refresh is 
							// waiting for next buffer it uses this wait state too
							KeReleaseSemaphore( &Sftk_Lg->RefreshStateChangeSemaphore, 0, 1, FALSE);
						}
						Sftk_Lg->TrackingIoCount = 0;	// reinistalize count

						if (PrevState == SFTK_MODE_SMART_REFRESH)
						{
							// wait for Smart Refresh terminates
							KeWaitForSingleObject(	&Sftk_Lg->EventRefreshWorkStop,Executive,
													KernelMode,FALSE,NULL );
						}

						if (UserRequested == TRUE)
						{ // if UserRequested Grab lock if its FALSE means caller already has lock acquired for this new state transition
							if (Sftk_Lg->LgDev_List.NumOfNodes > 1)
								OS_RELEASE_LOCK( &Sftk_Lg->AckLock, NULL);	// Release lock now 
						}

						// signal Ack thread
						KeReleaseSemaphore( &Sftk_Lg->AckStateChangeSemaphore, 0, 1, FALSE);

						// wait till Ack thread finish its Bitmap prepration
						KeWaitForSingleObject(	&Sftk_Lg->EventAckFinishBitmapPrep, Executive,
													KernelMode,FALSE,NULL );

						OS_PERF_ENDTIME(BAB_OVERFLOW_STATE_TRANISITION,totalQMPkts);	// TODO get WorkLoad values = total Cache has BAB entries

					} // if : BAB is used in previouse state so we need to rebuild HRDB for racking mode changes
					else
					{ // all other mode just change the state

						Sftk_Lg->PrevState = Sftk_Lg->state;	// Save Prev State, need it for protocol
						Sftk_Lg->state = NewState;
						if (PrevState != NewState)
							KeReleaseSemaphore( &Sftk_Lg->RefreshStateChangeSemaphore, 0, 1, FALSE);	// Signal change of state
					}
				} // case NewState = SFTK_MODE_TRACKING: 
				break;

		case SFTK_MODE_FULL_REFRESH: 
		case SFTK_MODE_PASSTHRU:
		case SFTK_MODE_BACKFRESH:
				// Reset All Bitmaps
				// OS_ACQUIRE_LOCK( &Sftk_Lg->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);	// Do We need to grab lock for this !!!! ???
				for(	plistEntry = Sftk_Lg->LgDev_List.ListEntry.Flink;
						plistEntry != &Sftk_Lg->LgDev_List.ListEntry;
						plistEntry = plistEntry->Flink )
				{ // for :scan thru each and every Devices under logical group 
					pSftkDev = CONTAINING_RECORD( plistEntry, SFTK_DEV, LgDev_Link);

					OS_ASSERT(pSftkDev->SftkLg == Sftk_Lg);
					OS_ASSERT(pSftkDev->LGroupNumber == Sftk_Lg->LGroupNumber);

					if ( (OS_IsFlagSet( Sftk_Lg->flags, SFTK_LG_FLAG_AFTER_BOOT_RESUME_FULL_REFRESH) == TRUE) &&
						 (NewState == SFTK_MODE_FULL_REFRESH) )
					{ // After Reboot conditions, clean all bitmaps after Valid RefreshLastBitIndex used for Full Refresh !!....
						if ( (pSftkDev->RefreshLastBitIndex != DEV_LASTBIT_NO_CLEAN) &&
							 (pSftkDev->RefreshLastBitIndex != DEV_LASTBIT_CLEAN_ALL) )
						{ // update Lrdb and HRdb for Smart refresh from prev state Full resfresh before boot
							ULONG			numOfBytesPerBit,  startBit, endBit, lrdb_refreshLastBitIndex;
							LARGE_INTEGER	byteOffset;

							// first do for lrdb
							numOfBytesPerBit 	= pSftkDev->Hrdb.Sectors_per_bit * SECTOR_SIZE;  
							byteOffset.QuadPart = (LONGLONG) ( (INT64) pSftkDev->RefreshLastBitIndex * (INT64) numOfBytesPerBit ); 

							startBit = endBit = 0;
							sftk_bit_range_from_offset( &pSftkDev->Lrdb, 
														byteOffset.QuadPart, 
														(ULONG) numOfBytesPerBit, 
														&startBit, 
														&endBit );

							lrdb_refreshLastBitIndex = FULL_REFRESH_LAST_INDEX(endBit); // 2 bit bump up, for safety

							if (lrdb_refreshLastBitIndex <= pSftkDev->Lrdb.TotalNumOfBits)
							{
								// TODO : Why we need to increment its safe to not to do that worst to worst it will skip one bit 
								RtlClearBits( pSftkDev->Lrdb.pBitmapHdr, lrdb_refreshLastBitIndex, 
											  (pSftkDev->Lrdb.TotalNumOfBits-1) );	// since endBit is zero based we should increment it to 1
							}

							// Now Do for Hrdb
							lrdb_refreshLastBitIndex = FULL_REFRESH_LAST_INDEX(pSftkDev->RefreshLastBitIndex); // 2 bit bump up, for safety

							if (lrdb_refreshLastBitIndex <= pSftkDev->Hrdb.TotalNumOfBits)
							{
								RtlClearBits( pSftkDev->Hrdb.pBitmapHdr, lrdb_refreshLastBitIndex, 
												(pSftkDev->Hrdb.TotalNumOfBits-1) );	// since endBit is zero based we should increment it to 1
							}
						} // update Lrdb and HRdb for Smart refresh from prev state Full resfresh before boot
					}  // After Reboot conditions, clean all bitmaps after Valid RefreshLastBitIndex used for Full Refresh !!....
					else
					{ // non-reboot conditions, so clean all bitmaps....
						RtlClearAllBits( pSftkDev->Lrdb.pBitmapHdr );
						RtlClearAllBits( pSftkDev->Hrdb.pBitmapHdr );
						RtlClearAllBits( pSftkDev->ALrdb.pBitmapHdr );

						pSftkDev->RefreshLastBitIndex = DEV_LASTBIT_NO_CLEAN;	// reset this
					}

					if (OS_IsFlagSet( Sftk_Lg->flags, SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE) == FALSE)
					{
						pSftkDev->PsDev->RefreshLastBitIndex = pSftkDev->RefreshLastBitIndex;

						// Write last bit info changed to SFTK_PS_DEV and flush it to Pstore file
						// Also flush all new bitmap to pstore file
						status = sftk_flush_all_bitmaps_to_pstore(pSftkDev, bHandleValid, fileHandle, TRUE, TRUE);
						if ( !NT_SUCCESS(status) ) 
						{ // failed to open or create pstore file...
							DebugPrint((DBG_ERROR, "sftk_lg_change_State:: sftk_flush_all_bitmaps_to_pstore(%S, LG 0x%08x Sftk_Dev 0x%08x for Device %s ) Failed with status 0x%08x Ignoring Error !!!\n", 
										Sftk_Lg->PStoreFileName.Buffer, Sftk_Lg->LGroupNumber, 
										pSftkDev, pSftkDev->Vdevname, status ));

							swprintf( wStr4, L"Flushing All Dev %d Bitmaps", pSftkDev->cdev);
							sftk_LogEventNum2Wchar2(GSftk_Config.DriverObject, MSG_REPL_DEV_PSTORE_WRITE_ERROR, status, 
													0, pSftkDev->cdev, pSftkDev->SftkLg->LGroupNumber, 
													pSftkDev->SftkLg->PStoreFileName.Buffer, wStr4);
							// ignoring error
						}
					}
				} // for :scan thru each and every Devices under logical group 

				Sftk_Lg->PrevState = Sftk_Lg->state;	// Save Prev State, need it for protocol
				Sftk_Lg->state = NewState;
				// if (PrevState != NewState)
				KeReleaseSemaphore( &Sftk_Lg->RefreshStateChangeSemaphore, 0, 1, FALSE);	// Signal change of state
					
				// OS_RELEASE_LOCK( &Sftk_Lg->Lock, NULL);
				if ( (PrevState == SFTK_MODE_SMART_REFRESH) || (PrevState == SFTK_MODE_NORMAL) 
						|| (PrevState == SFTK_MODE_FULL_REFRESH) )
				{ // BAB is used in previouse state so we need to free all BAB and QM pkts without updating Bitmaps
					KeClearEvent( &Sftk_Lg->Event_LGFreeAllMemOfMM );

					// if in Prev States BAB Was Used than clear all QM pkts
					status = QM_ScanAllQList( Sftk_Lg, TRUE, FALSE);
					if (!NT_SUCCESS(status))
					{
						DebugPrint((DBG_THREAD, "sftk_lg_change_state:: QM_ScanAllQList( LG %d, FreeAllEntries : TRUE, UpdateBitmap: FALSE) Failed with status 0x%08x !!\n", 
												Sftk_Lg->LGroupNumber, status ));
					}
					// Signal the event here to tell MM has free all its memory for current LG
					KeSetEvent( &Sftk_Lg->Event_LGFreeAllMemOfMM, 0, FALSE);

				} // Clear BAB
				break;

		case SFTK_MODE_SMART_REFRESH:	// Reset last bitindex counter for all devices, if needed
#if TARGET_SIDE
			if (Sftk_Lg->SRefreshWasNotDone	== FALSE)
			{ // this is first time smart refresh is starting, so prepare All Device's SRDB from HRDB
				for(	plistEntry = Sftk_Lg->LgDev_List.ListEntry.Flink;
						plistEntry != &Sftk_Lg->LgDev_List.ListEntry;
						plistEntry = plistEntry->Flink )
				{ // for :scan thru each and every Devices under logical group 
				pSftkDev = CONTAINING_RECORD( plistEntry, SFTK_DEV, LgDev_Link);

				OS_ASSERT(pSftkDev->SftkLg == Sftk_Lg);
				OS_ASSERT(pSftkDev->LGroupNumber == Sftk_Lg->LGroupNumber);

				RtlCopyMemory(pSftkDev->Srdb.pBits, pSftkDev->Hrdb.pBits, pSftkDev->Hrdb.BitmapSize);
				}
				Sftk_Lg->SRefreshWasNotDone = TRUE;	// TRUE means SR started but not completed to NORMAL
				Sftk_Lg->UseSRDB = TRUE; // Now on start updating SRDB also
			}
			Sftk_Lg->PrevState = Sftk_Lg->state;	// Save Prev State, need it for protocol
			Sftk_Lg->state = NewState;
			if (PrevState != NewState)
				KeReleaseSemaphore( &Sftk_Lg->RefreshStateChangeSemaphore, 0, 1, FALSE);	// Signal change of state
			break;
#endif
		case SFTK_MODE_NORMAL:			// Check if connection is on
#if TARGET_SIDE
			if (PrevState == SFTK_MODE_SMART_REFRESH)
			{ // Since from smart refresh to Normal, Clear all bits in all Device's SRDB to zeros				Sftk_Lg->UseSRDB = FALSE;
				for(	plistEntry = Sftk_Lg->LgDev_List.ListEntry.Flink;
						plistEntry != &Sftk_Lg->LgDev_List.ListEntry;
						plistEntry = plistEntry->Flink )
				{ // for :scan thru each and every Devices under logical group 
				pSftkDev = CONTAINING_RECORD( plistEntry, SFTK_DEV, LgDev_Link);

				OS_ASSERT(pSftkDev->SftkLg == Sftk_Lg);
				OS_ASSERT(pSftkDev->LGroupNumber == Sftk_Lg->LGroupNumber);

				RtlClearAllBits( pSftkDev->Srdb.pBitmapHdr); 
				}
				Sftk_Lg->SRefreshWasNotDone = FALSE;	// FALSE means SR completed successfully to NORMAL
				Sftk_Lg->UseSRDB = FALSE; // Now on stop updating SRDB
			}
#endif
		default:	// all other mode just change the state
				if (NewState == SFTK_MODE_NORMAL)
				{
					sftk_update_lg_dev_lastBitIndex(Sftk_Lg, DEV_LASTBIT_CLEAN_ALL);
				}

				Sftk_Lg->PrevState = Sftk_Lg->state;	// Save Prev State, need it for protocol
				Sftk_Lg->state = NewState;
				if (PrevState != NewState)
					KeReleaseSemaphore( &Sftk_Lg->RefreshStateChangeSemaphore, 0, 1, FALSE);	// Signal change of state
				break;
	} // switch (NewState)

	if (OS_IsFlagSet( Sftk_Lg->flags, SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE) == FALSE)
	{
		// Update Pstore with new LG states, Do we need this to update all the time !! ?? FIXME FIXME
		/*
		Sftk_Lg->PsHeader->LgInfo.state = Sftk_Lg->state;
		Sftk_Lg->PsHeader->LgInfo.PrevState = PrevState;
		Sftk_Lg->PsHeader->LgInfo.bInconsistantData = Sftk_Lg->bInconsistantData;
		Sftk_Lg->PsHeader->LgInfo.UserChangedToTrackingMode = Sftk_Lg->UserChangedToTrackingMode;
		// This API sftk_flush_psHdr_to_pstore() dies this work
		*/
		// Write state changed to SFTK_PS_LG and flush it to Pstore file
		status = sftk_flush_psHdr_to_pstore(Sftk_Lg, bHandleValid, fileHandle, TRUE);
		if ( !NT_SUCCESS(status) ) 
			{ // failed to open or create pstore file...
				DebugPrint((DBG_ERROR, "sftk_lg_change_State:: sftk_flush_psHdr_to_pstore(%S, LG 0x%08x)  Failed with status 0x%08x Ignoring Error !!!\n", 
										Sftk_Lg->PStoreFileName.Buffer, Sftk_Lg->LGroupNumber, status ));
				// ignoring error
				swprintf( wStr4, L"Flushing LG %d Hdr", Sftk_Lg->LGroupNumber);
				sftk_LogEventNum2Wchar2(	GSftk_Config.DriverObject, MSG_REPL_DEV_PSTORE_WRITE_ERROR, status, 
											0, Sftk_Lg->LGroupNumber, 
											Sftk_Lg->LGroupNumber, Sftk_Lg->PStoreFileName.Buffer, wStr4);
			}
	}
#if  DBG
	sftk_lg_Verify_State_change_conditions(Sftk_Lg, PrevState, NewState);
#endif

	if ((bHandleValid == TRUE) && (fileHandle))
		ZwClose(fileHandle);

	// Log Event here
	OS_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
	swprintf(wStr1, L"0x%08x",Sftk_Lg->LGroupNumber);
	sftk_get_stateString(NewState, wStr2);
	sftk_get_stateString(PrevState, wStr3);
	sftk_LogEventString3(	GSftk_Config.DriverObject, MSG_REPL_STATE_CHANGE, 
							STATUS_SUCCESS, 0, 
							wStr1, wStr2, wStr3);

	return STATUS_SUCCESS;
} // sftk_lg_change_State()

VOID
sftk_get_lg_state_string(LONG  LgState, PWCHAR Wstring)
{
	switch(LgState)
	{
		case SFTK_MODE_PASSTHRU:		swprintf(Wstring, L"PASSTHRU"); break;
		case SFTK_MODE_TRACKING:		swprintf(Wstring, L"TRACKING"); break;
		case SFTK_MODE_NORMAL:			swprintf(Wstring, L"NORMAL"); break;
		case SFTK_MODE_FULL_REFRESH:	swprintf(Wstring, L"FULL_REFRESH"); break;
		case SFTK_MODE_SMART_REFRESH:	swprintf(Wstring, L"SMART_REFRESH"); break;
		case SFTK_MODE_BACKFRESH:		swprintf(Wstring, L"BACKFRESH"); break;
		default:						swprintf(Wstring, L"Unknown"); break;
	} // switch(LgState)
	return;
} // sftk_get_lg_state_string()

// sftk_lg_Verify_State_change_conditions() used only for debugging purpose
NTSTATUS
sftk_lg_Verify_State_change_conditions( PSFTK_LG	Sftk_Lg, LONG  PrevState, LONG  NewState)	
{
	BOOLEAN		bret = FALSE;

	DebugPrint((DBG_STATE, "sftk_lg_Verify_State_change_conditions:: LG %d, PrevState Mode 0x%08x changing to NewState Mode 0x%08x !!\n", 
											Sftk_Lg->LGroupNumber, PrevState, NewState));
	// signals appropriate events on change of state
	switch(NewState)
		{
		case SFTK_MODE_PASSTHRU:	// OS_ASSERT( (NewState == SFTK_MODE_FULL_REFRESH) || (NewState == SFTK_MODE_TRACKING) );
		case SFTK_MODE_BACKFRESH:	//OS_ASSERT(NewState == SFTK_MODE_FULL_REFRESH);
		case SFTK_MODE_FULL_REFRESH:
									// OS_ASSERT(NewState == SFTK_MODE_SMART_REFRESH);
									bret = sftk_match_lg_dev_lastBitIndex( Sftk_Lg, DEV_LASTBIT_NO_CLEAN);
									if (bret == FALSE)
									{
									DebugPrint((DBG_ERROR, "sftk_lg_Verify_State_change_conditions:: BUG FIXME FIXME : LG %d, sftk_match_lg_dev_lastBitIndex() Return FALSE !! PrevState Mode SMART_REFRESH to Newstate NORMAL !!\n", 
													Sftk_Lg->LGroupNumber));
									OS_ASSERT(FALSE);	
									}
									break;

		case SFTK_MODE_SMART_REFRESH:
									// if bab overflow occurs than we goto Tracking mode else always we goto Normal mode
									// OS_ASSERT( (NewState == SFTK_MODE_NORMAL) || (NewState == SFTK_MODE_TRACKING));

									// Verify that each and every devices has RefreshLastBitIndex = DEV_LASTBIT_CLEAN_ALL
									/*
									if (NewState == SFTK_MODE_NORMAL)
									{
										bret = sftk_match_lg_dev_lastBitIndex( Sftk_Lg, DEV_LASTBIT_CLEAN_ALL);
										if (bret == FALSE)
										{
										DebugPrint((DBG_ERROR, "sftk_lg_Verify_State_change_conditions:: BUG FIXME FIXME : LG %d, sftk_match_lg_dev_lastBitIndex() Return FALSE !! PrevState Mode SMART_REFRESH to Newstate NORMAL !!\n", 
														Sftk_Lg->LGroupNumber));
										OS_ASSERT(FALSE);	
										}
									}
									*/
									break;

		case SFTK_MODE_NORMAL:		
									// OS_ASSERT( NewState == SFTK_MODE_TRACKING );
									// OS_ASSERT( Sftk_Lg->RefreshFinishedParseI == FALSE);
									// Verify that each and every devices has RefreshLastBitIndex = DEV_LASTBIT_CLEAN_ALL
									bret = sftk_match_lg_dev_lastBitIndex( Sftk_Lg, DEV_LASTBIT_CLEAN_ALL);
									if (bret == FALSE)
									{
										DebugPrint((DBG_ERROR, "sftk_lg_Verify_State_change_conditions:: BUG FIXME FIXME : LG %d, sftk_match_lg_dev_lastBitIndex() Return FALSE !! NewState Mode NORMAL, PrevState  0x%08x !!\n", 
														Sftk_Lg->LGroupNumber, PrevState));
										OS_ASSERT(FALSE);	
									}
									break;

		case SFTK_MODE_TRACKING:	// Sftk_Lg->RefreshFinishedParseI = FALSE;
									/*
									if (Sftk_Lg->UserChangedToTrackingMode == FALSE)
									{ 
										OS_ASSERT( (NewState == SFTK_MODE_SMART_REFRESH) || (NewState == SFTK_MODE_FULL_REFRESH) || (NewState == SFTK_MODE_NORMAL));
									}
									*/
									break;
			
		default:	
					DebugPrint((DBG_ERROR, "sftk_lg_Verify_State_change_conditions:: BUG FIXME FIXME : LG %d, NewState Mode is unknown 0x%08x, PrevState 0x%08x  !!\n", 
													Sftk_Lg->LGroupNumber, NewState, PrevState));
					// OS_ASSERT(FALSE);	
					break;
		
		} // switch(NewState)
	return STATUS_SUCCESS;
} // sftk_lg_Verify_State_change_conditions()

// Returns TRUE if specified RefreshLastBitIndex values meatches to all devices under LG
// else FALSE
BOOLEAN
sftk_match_lg_dev_lastBitIndex( PSFTK_LG	Sftk_Lg, ULONG RefreshLastBitIndex	)
{
	PSFTK_DEV		pSftkDev	= NULL;
	PLIST_ENTRY		plistEntry	= NULL;

	// OS_ACQUIRE_LOCK( &Sftk_Lg->Lock, OS_ACQUIRE_SHARED, TRUE, NULL);

	for( plistEntry = Sftk_Lg->LgDev_List.ListEntry.Flink;
		 plistEntry != &Sftk_Lg->LgDev_List.ListEntry;
		 plistEntry = plistEntry->Flink )
	{ // for :scan thru each and every logical group list 
		pSftkDev = CONTAINING_RECORD( plistEntry, SFTK_DEV, LgDev_Link);

		if (pSftkDev->RefreshLastBitIndex != RefreshLastBitIndex)
			return FALSE;	// Not match
	}
	// OS_RELEASE_LOCK( &Sftk_Lg->Lock, NULL);

	return TRUE; // all match

} // sftk_match_lg_dev_lastBitIndex()

NTSTATUS
sftk_update_lg_dev_lastBitIndex( PSFTK_LG	Sftk_Lg, ULONG RefreshLastBitIndex	)
{
	PSFTK_DEV		pSftkDev	= NULL;
	PLIST_ENTRY		plistEntry	= NULL;

	// OS_ACQUIRE_LOCK( &Sftk_Lg->Lock, OS_ACQUIRE_SHARED, TRUE, NULL);

	for( plistEntry = Sftk_Lg->LgDev_List.ListEntry.Flink;
		 plistEntry != &Sftk_Lg->LgDev_List.ListEntry;
		 plistEntry = plistEntry->Flink )
	{ // for :scan thru each and every logical group list 
		pSftkDev = CONTAINING_RECORD( plistEntry, SFTK_DEV, LgDev_Link);

		pSftkDev->RefreshLastBitIndex = RefreshLastBitIndex;
	}
	// OS_RELEASE_LOCK( &Sftk_Lg->Lock, NULL);

	return STATUS_SUCCESS; // all match

} // sftk_update_lg_dev_lastBitIndex()


// TRUE means state is not Refresh state (not Full or /Smart Refresh)
// FALSE means state is Refresh state (Full or /Smart Refresh)
BOOLEAN
sftk_lg_State_is_not_refresh( PSFTK_LG	Sftk_Lg)
{
	return ( (Sftk_Lg->state != SFTK_MODE_FULL_REFRESH) && (Sftk_Lg->state != SFTK_MODE_SMART_REFRESH) );
}

LONG
sftk_lg_get_state( PSFTK_LG	Sftk_Lg)
{
	return (Sftk_Lg->state);
}

// Return True if Normal (Resume) else if Paused return FALSE.
BOOLEAN
sftk_lg_get_refresh_thread_state( PSFTK_LG	Sftk_Lg)
{
	return (Sftk_Lg->bPauseFullRefresh == FALSE ? TRUE : FALSE);
}


NTSTATUS
sftk_ctl_lg_Get_All_StatsInfo( PIRP Irp)
{
	NTSTATUS			status				= STATUS_SUCCESS;
	PIO_STACK_LOCATION  pCurStackLocation	= IoGetCurrentIrpStackLocation(Irp);
	PALL_LG_STATISTICS	outBuffer			= Irp->AssociatedIrp.SystemBuffer;
	ULONG				sizeOfBuffer		= pCurStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	ULONG				sizeExpected;
	PLIST_ENTRY			plistEntry, pListEntry;
	PSFTK_LG			pSftkLg;
	PSFTK_DEV			pSftkDev;
	PLG_STATISTICS		pLgStats;
	PDEV_STATISTICS		pDevStats;

	sizeExpected = MAX_SIZE_ALL_LG_STATISTICS(	outBuffer->NumOfLgEntries, 
												outBuffer->NumOfDevEntries);
	if (sizeOfBuffer < sizeExpected)
	{
		status = STATUS_BUFFER_OVERFLOW;
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_Get_All_StatsInfo: sizeOfBuffer %d < %d MAX_SIZE_ALL_LG_STATISTICS(NumOfLgEntries %d, NumOfDevEntries %d) Failed with status 0x%08x !!! \n",
										sizeOfBuffer, sizeExpected, outBuffer->NumOfLgEntries, outBuffer->NumOfDevEntries, status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	sizeExpected = MAX_SIZE_ALL_LG_STATISTICS(	GSftk_Config.Lg_GroupList.NumOfNodes, 
												GSftk_Config.SftkDev_List.NumOfNodes);
	if (sizeOfBuffer < sizeExpected)
	{
		status = STATUS_BUFFER_OVERFLOW;
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_Get_All_StatsInfo: Actial sizeOfBuffer %d < %d MAX_SIZE_ALL_LG_STATISTICS(Lg_GroupList NumOfLgEntries %d, NumOfDevEntries %d) Failed with status 0x%08x !!! \n",
										sizeOfBuffer, sizeExpected, GSftk_Config.Lg_GroupList.NumOfNodes, 
										GSftk_Config.SftkDev_List.NumOfNodes, status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	outBuffer->NumOfLgEntries	= 0;
	outBuffer->NumOfDevEntries	= 0;

	pLgStats = &outBuffer->LgStats[0];

	for( plistEntry = GSftk_Config.Lg_GroupList.ListEntry.Flink;
		 plistEntry != &GSftk_Config.Lg_GroupList.ListEntry;
		 plistEntry = plistEntry->Flink )
	{ // for :scan thru each and every LG
		pSftkLg = CONTAINING_RECORD( plistEntry, SFTK_LG, Lg_GroupLink);

		OS_ZeroMemory( pLgStats, sizeof(LG_STATISTICS) );
		pLgStats->LgNum		= pSftkLg->LGroupNumber;
		pLgStats->Flags		= 0;
		pLgStats->cdev		= 0;
		pLgStats->LgState	= (ULONG) pSftkLg->state;

		OS_RtlCopyMemory( &pLgStats->LgStats, &pSftkLg->Statistics, sizeof(pLgStats->LgStats) );
		
		pLgStats->LgStats.MM_TotalMemAllocated	= GSftk_Config.Mmgr.MmSlab[MM_TYPE_4K_NPAGED_MEM].TotalMemSize;
		pLgStats->LgStats.MM_TotalMemIsUsed		= (UINT64)
				(GSftk_Config.Mmgr.MmSlab[MM_TYPE_4K_NPAGED_MEM].TotalNumberOfPagesInUse * GSftk_Config.Mmgr.PageSize);

		pLgStats->LgStats.MM_TotalOSMemUsed					= GSftk_Config.Mmgr.MM_TotalOSMemUsed;
		pLgStats->LgStats.MM_OSMemUsed						= GSftk_Config.Mmgr.MM_OSMemUsed;
		pLgStats->LgStats.MM_TotalNumOfMdlLocked			= GSftk_Config.Mmgr.MM_TotalNumOfMdlLocked;
		pLgStats->LgStats.MM_TotalSizeOfMdlLocked			= GSftk_Config.Mmgr.MM_TotalSizeOfMdlLocked;
		pLgStats->LgStats.MM_TotalNumOfMdlLockedAtPresent	= GSftk_Config.Mmgr.MM_TotalNumOfMdlLockedAtPresent;
		pLgStats->LgStats.MM_TotalSizeOfMdlLockedAtPresent	= GSftk_Config.Mmgr.MM_TotalSizeOfMdlLockedAtPresent;

		// pLgStats->NumOfDevStats		= pSftkLg->LgDev_List.NumOfNodes;
		pLgStats->NumOfDevStats = 0;
		pDevStats = &pLgStats->DevStats[0];

		for(pListEntry = pSftkLg->LgDev_List.ListEntry.Flink;
			pListEntry != &pSftkLg->LgDev_List.ListEntry;
			pListEntry = pListEntry->Flink )
		{ // for :scan thru each and every DEV
			pSftkDev = CONTAINING_RECORD( pListEntry, SFTK_DEV, LgDev_Link);

			OS_ZeroMemory( pDevStats, sizeof(DEV_STATISTICS) );

			pDevStats->LgNum		= pSftkDev->LGroupNumber;
			pDevStats->cdev			= pSftkDev->cdev;
			pDevStats->DevState		= 0; // pSftkDev->LGroupNumber;
			// OS_RtlCopyMemory( pDevStats->Devname, pSftkDev->Devname, sizeof(pDevStats->Devname));
			pDevStats->Disksize		= pSftkDev->Disksize;

			OS_RtlCopyMemory( &pDevStats->DevStats, &pSftkDev->Statistics, sizeof(pDevStats->DevStats));

			pDevStats->DevStats.MM_TotalMemAllocated = GSftk_Config.Mmgr.MmSlab[MM_TYPE_4K_NPAGED_MEM].TotalMemSize;
			pDevStats->DevStats.MM_TotalMemIsUsed	 = (UINT64)
				(GSftk_Config.Mmgr.MmSlab[MM_TYPE_4K_NPAGED_MEM].TotalNumberOfPagesInUse * GSftk_Config.Mmgr.PageSize);

			// pDevStats ++;  or
			pDevStats = (PDEV_STATISTICS) ((ULONG) pDevStats + sizeof(DEV_STATISTICS));
			pLgStats->NumOfDevStats ++;
		} // for :scan thru each and every DEV

		if (pLgStats->NumOfDevStats == 0)
			pDevStats = (PDEV_STATISTICS) ((ULONG) pDevStats + sizeof(DEV_STATISTICS));

		outBuffer->NumOfDevEntries	+= pLgStats->NumOfDevStats;
		outBuffer->NumOfLgEntries	++;

		pLgStats = (PLG_STATISTICS) pDevStats;
	} // for :scan thru each and every LG

	return status;
} // sftk_ctl_lg_Get_All_StatsInfo()

NTSTATUS
sftk_ctl_lg_Get_StatsInfo( PIRP Irp)
{
	NTSTATUS			status				= STATUS_SUCCESS;
	PIO_STACK_LOCATION  pCurStackLocation	= IoGetCurrentIrpStackLocation(Irp);
	PLG_STATISTICS		pLgStats			= Irp->AssociatedIrp.SystemBuffer;
	ULONG				sizeOfBuffer		= pCurStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	ULONG				sizeExpected;
	PLIST_ENTRY			plistEntry, pListEntry;
	PSFTK_LG			pSftkLg;
	PSFTK_DEV			pSftkDev;
	PDEV_STATISTICS		pDevStats;

	sizeExpected = MAX_SIZE_LG_STATISTICS(	pLgStats->NumOfDevStats );
	if (sizeOfBuffer < sizeExpected)
	{
		status = STATUS_BUFFER_OVERFLOW;
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_Get_StatsInfo: sizeOfBuffer %d < %d MAX_SIZE_LG_STATISTICS(NumOfDevEntries %d) Failed with status 0x%08x !!! \n",
										sizeOfBuffer, sizeExpected, pLgStats->NumOfDevStats, status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

#if TARGET_SIDE
	pSftkLg = sftk_lookup_lg_by_lgnum( pLgStats->LgNum, pLgStats->lgCreationRole );	
#else
	pSftkLg = sftk_lookup_lg_by_lgnum( pLgStats->LgNum );	// minor_num = getminor(dev) & ~FTD_LGFLAG; 
#endif
	if (pSftkLg == NULL) 
	{
		status = STATUS_INVALID_PARAMETER;
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_Get_StatsInfo: sftk_lookup_lg_by_lgnum(specified Lgdev number = %d) NOT exist !!! returning error 0x%08x \n",
									pLgStats->LgNum, status));
		return status;	
	}

	if( pLgStats->Flags == GET_LG_AND_ITS_ALL_DEV_INFO)
	{
		sizeExpected = MAX_SIZE_LG_STATISTICS(	pSftkLg->LgDev_List.NumOfNodes );
		if (sizeOfBuffer < sizeExpected)
		{
			status = STATUS_BUFFER_OVERFLOW;
			DebugPrint((DBG_ERROR, "sftk_ctl_lg_Get_StatsInfo: Actial sizeOfBuffer %d < %d MAX_SIZE_ALL_LG_STATISTICS(pSftkLg->LgDev_List.NumOfNodes %d) Failed with status 0x%08x !!! \n",
											sizeOfBuffer, sizeExpected, pSftkLg->LgDev_List.NumOfNodes, status));
			return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
		}
	}

	OS_ZeroMemory( pLgStats, sizeof(LG_STATISTICS) );
	pLgStats->LgNum		= pSftkLg->LGroupNumber;
	pLgStats->LgState	= (ULONG) pSftkLg->state;

	OS_RtlCopyMemory( &pLgStats->LgStats, &pSftkLg->Statistics, sizeof(pLgStats->LgStats) );
	
	pLgStats->LgStats.MM_TotalMemAllocated	= GSftk_Config.Mmgr.MmSlab[MM_TYPE_4K_NPAGED_MEM].TotalMemSize;
	pLgStats->LgStats.MM_TotalMemIsUsed		= (UINT64)
			(GSftk_Config.Mmgr.MmSlab[MM_TYPE_4K_NPAGED_MEM].TotalNumberOfPagesInUse * GSftk_Config.Mmgr.PageSize);

	pLgStats->LgStats.MM_TotalOSMemUsed					= GSftk_Config.Mmgr.MM_TotalOSMemUsed;
	pLgStats->LgStats.MM_OSMemUsed						= GSftk_Config.Mmgr.MM_OSMemUsed;
	pLgStats->LgStats.MM_TotalNumOfMdlLocked			= GSftk_Config.Mmgr.MM_TotalNumOfMdlLocked;
	pLgStats->LgStats.MM_TotalSizeOfMdlLocked			= GSftk_Config.Mmgr.MM_TotalSizeOfMdlLocked;
	pLgStats->LgStats.MM_TotalNumOfMdlLockedAtPresent	= GSftk_Config.Mmgr.MM_TotalNumOfMdlLockedAtPresent;
	pLgStats->LgStats.MM_TotalSizeOfMdlLockedAtPresent	= GSftk_Config.Mmgr.MM_TotalSizeOfMdlLockedAtPresent;

	// pLgStats->NumOfDevStats		= pSftkLg->LgDev_List.NumOfNodes;
	pLgStats->NumOfDevStats = 0;
	pDevStats = &pLgStats->DevStats[0];

	OS_ZeroMemory( pDevStats, sizeof(DEV_STATISTICS) );

	if( pLgStats->Flags != GET_LG_AND_ITS_ALL_DEV_INFO)
	{ // returns specified asking info only
		if (pLgStats->Flags == GET_LG_AND_SPECIFIED_ONE_DEV_INFO)
		{ // returned specified dev info
			pSftkDev = sftk_lookup_dev_by_cdev_in_SftkLG( pSftkLg, pLgStats->cdev );	// minor_num = getminor(dev) & ~FTD_LGFLAG; 
			if (pSftkDev == NULL) 
			{
				status = STATUS_INVALID_PARAMETER;
				DebugPrint((DBG_ERROR, "sftk_ctl_lg_Get_StatsInfo: sftk_lookup_dev_by_cdev_in_SftkLG(Lgdev number = %d, cdev %d) NOT exist !!! returning error 0x%08x \n",
											pLgStats->LgNum, pLgStats->cdev, status));
				return status;	
			}
			pDevStats->LgNum		= pSftkDev->LGroupNumber;
			pDevStats->cdev			= pSftkDev->cdev;
			pDevStats->DevState		= 0; // pSftkDev->LGroupNumber;
			// OS_RtlCopyMemory( pDevStats->Devname, pSftkDev->Devname, sizeof(pDevStats->Devname));
			pDevStats->Disksize		= pSftkDev->Disksize;

			OS_RtlCopyMemory( &pDevStats->DevStats, &pSftkDev->Statistics, sizeof(pDevStats->DevStats));

			pDevStats->DevStats.MM_TotalMemAllocated = GSftk_Config.Mmgr.MmSlab[MM_TYPE_4K_NPAGED_MEM].TotalMemSize;
			pDevStats->DevStats.MM_TotalMemIsUsed	 = (UINT64)
				(GSftk_Config.Mmgr.MmSlab[MM_TYPE_4K_NPAGED_MEM].TotalNumberOfPagesInUse * GSftk_Config.Mmgr.PageSize);

			pLgStats->NumOfDevStats ++;
		} // returned specified dev info
		return STATUS_SUCCESS;
	} // returns specified asking info only

	// return all dev info 
	pLgStats->NumOfDevStats	= 0;

	for(pListEntry = pSftkLg->LgDev_List.ListEntry.Flink;
		pListEntry != &pSftkLg->LgDev_List.ListEntry;
		pListEntry = pListEntry->Flink )
	{ // for :scan thru each and every DEV
		pSftkDev = CONTAINING_RECORD( pListEntry, SFTK_DEV, LgDev_Link);

		OS_ZeroMemory( pDevStats, sizeof(DEV_STATISTICS) );

		pDevStats->LgNum		= pSftkDev->LGroupNumber;
		pDevStats->cdev			= pSftkDev->cdev;
		pDevStats->DevState		= 0; // pSftkDev->LGroupNumber;
		// OS_RtlCopyMemory( pDevStats->Devname, pSftkDev->Devname, sizeof(pDevStats->Devname));
		pDevStats->Disksize		= pSftkDev->Disksize;

		OS_RtlCopyMemory( &pDevStats->DevStats, &pSftkDev->Statistics, sizeof(pDevStats->DevStats));

		pDevStats->DevStats.MM_TotalMemAllocated = GSftk_Config.Mmgr.MmSlab[MM_TYPE_4K_NPAGED_MEM].TotalMemSize;
		pDevStats->DevStats.MM_TotalMemIsUsed	 = 
			(GSftk_Config.Mmgr.MmSlab[MM_TYPE_4K_NPAGED_MEM].TotalNumberOfPagesInUse * GSftk_Config.Mmgr.PageSize);

		// pDevStats ++;  or
		pDevStats = (PDEV_STATISTICS) ((ULONG) pDevStats + sizeof(DEV_STATISTICS));
		pLgStats->NumOfDevStats ++;
	} // for :scan thru each and every DEV

	return STATUS_SUCCESS;
} // sftk_ctl_lg_Get_StatsInfo()

NTSTATUS
sftk_ctl_lg_Get_Lg_TuningParam( PIRP Irp)
{
	NTSTATUS			status				= STATUS_SUCCESS;
	PIO_STACK_LOCATION  pCurStackLocation	= IoGetCurrentIrpStackLocation(Irp);
	PLG_PARAM			pLgParam			= Irp->AssociatedIrp.SystemBuffer;
	ULONG				sizeOfBuffer		= pCurStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	PSFTK_LG			pSftkLg;
	
	if (sizeOfBuffer < sizeof(LG_PARAM))
	{
		status = STATUS_BUFFER_OVERFLOW;
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_Get_Lg_TuningParam: sizeOfBuffer %d < sizeof(LG_PARAM) %d Failed with status 0x%08x !!! \n",
										sizeOfBuffer, sizeof(LG_PARAM), status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

#if TARGET_SIDE
	pSftkLg = sftk_lookup_lg_by_lgnum( pLgParam->LgNum, pLgParam->lgCreationRole );	
#else
	pSftkLg = sftk_lookup_lg_by_lgnum( pLgParam->LgNum );
#endif
	if (pSftkLg == NULL) 
	{
		status = STATUS_INVALID_PARAMETER;
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_Get_Lg_TuningParam: sftk_lookup_lg_by_lgnum(specified Lgdev number = %d) NOT exist !!! returning error 0x%08x \n",
									pLgParam->LgNum, status));
		return status;	
	}

	pLgParam->ParamFlags				= 0;			
	pLgParam->Param.TrackingIoCount		= pSftkLg->TrackingIoCount;
	pLgParam->Param.MaxTransferUnit		= pSftkLg->MaxTransferUnit;
	pLgParam->Param.NumOfAsyncRefreshIO	= pSftkLg->NumOfAsyncRefreshIO;
	pLgParam->Param.AckWakeupTimeout	= pSftkLg->AckWakeupTimeout.QuadPart;
	pLgParam->Param.RefreshThreadWakeupTimeout	= pSftkLg->RefreshThreadWakeupTimeout.QuadPart;
	pLgParam->Param.sync_depth			= pSftkLg->sync_depth;
	pLgParam->Param.sync_timeout		= pSftkLg->sync_timeout;
	pLgParam->Param.iodelay				= pSftkLg->iodelay;

	pLgParam->Param.throtal_refresh_send_pkts		= pSftkLg->throtal_refresh_send_pkts;
	pLgParam->Param.throtal_refresh_send_totalsize	= pSftkLg->throtal_refresh_send_totalsize;
	pLgParam->Param.throtal_commit_send_pkts		= pSftkLg->throtal_commit_send_pkts;
	pLgParam->Param.throtal_commit_send_totalsize	= pSftkLg->throtal_commit_send_totalsize;
	pLgParam->Param.NumOfPktsSendAtaTime			= pSftkLg->NumOfPktsSendAtaTime;
	pLgParam->Param.NumOfPktsRecvAtaTime			= pSftkLg->NumOfPktsRecvAtaTime;
	pLgParam->Param.NumOfSendBuffers				= pSftkLg->NumOfSendBuffers;

#if DBG
	pLgParam->Param.DebugLevel	= SwrDebug;
#endif
	return status;
} // sftk_ctl_lg_Get_Lg_TuningParam()

NTSTATUS
sftk_ctl_lg_Set_Lg_TuningParam( PIRP Irp)
{
	NTSTATUS			status				= STATUS_SUCCESS;
	PIO_STACK_LOCATION  pCurStackLocation	= IoGetCurrentIrpStackLocation(Irp);
	PLG_PARAM			pLgParam			= Irp->AssociatedIrp.SystemBuffer;
	ULONG				sizeOfBuffer		= pCurStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	PSFTK_LG			pSftkLg;
	
	if (sizeOfBuffer < sizeof(LG_PARAM))
	{
		status = STATUS_BUFFER_OVERFLOW;
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_Set_Lg_TuningParam: sizeOfBuffer %d < sizeof(LG_PARAM) %d Failed with status 0x%08x !!! \n",
										sizeOfBuffer, sizeof(LG_PARAM), status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

#if TARGET_SIDE
	pSftkLg = sftk_lookup_lg_by_lgnum( pLgParam->LgNum, pLgParam->lgCreationRole );	
#else
	pSftkLg = sftk_lookup_lg_by_lgnum( pLgParam->LgNum );
#endif

	if (pSftkLg == NULL) 
	{
#if DBG
		if (pLgParam->ParamFlags & LG_PARAM_FLAG_DebugLevel)
		{
			SwrDebug = pLgParam->Param.DebugLevel;
		}
		else
#endif
		{
			status = STATUS_INVALID_PARAMETER;
			DebugPrint((DBG_ERROR, "sftk_ctl_lg_Set_Lg_TuningParam: sftk_lookup_lg_by_lgnum(specified Lgdev number = %d) NOT exist !!! returning error 0x%08x \n",
									pLgParam->LgNum, status));
		}
		return status;	
	}

	if (pLgParam->ParamFlags & LG_PARAM_FLAG_USE_TrackingIoCount)
	{
		if ( pLgParam->Param.TrackingIoCount <= 0 )
		{
			status = STATUS_INVALID_PARAMETER;
			DebugPrint((DBG_ERROR, "sftk_ctl_lg_Set_Lg_TuningParam: Lgnum %d, pLgParam->Param.TrackingIoCount %d <= 0 !!! returning error 0x%08x \n",
									pLgParam->LgNum, pLgParam->Param.TrackingIoCount, status));
			return status;	
		}
		pSftkLg->TrackingIoCount = pLgParam->Param.TrackingIoCount;
	}

	if (pLgParam->ParamFlags & LG_PARAM_FLAG_USE_MaxTransferUnit)
	{
		if ( pLgParam->Param.MaxTransferUnit <= 0 )
		{
			status = STATUS_INVALID_PARAMETER;
			DebugPrint((DBG_ERROR, "sftk_ctl_lg_Set_Lg_TuningParam: Lgnum %d, pLgParam->Param.MaxTransferUnit %d <= 0 !!! returning error 0x%08x \n",
									pLgParam->LgNum, pLgParam->Param.MaxTransferUnit, status));
			return status;	
		}
		pSftkLg->MaxTransferUnit = pLgParam->Param.MaxTransferUnit;
	}

	if (pLgParam->ParamFlags & LG_PARAM_FLAG_USE_NumOfAsyncRefreshIO)
	{
		if ( pLgParam->Param.NumOfAsyncRefreshIO <= 0 )
		{
			status = STATUS_INVALID_PARAMETER;
			DebugPrint((DBG_ERROR, "sftk_ctl_lg_Set_Lg_TuningParam: Lgnum %d, pLgParam->Param.NumOfAsyncRefreshIO %d <= 0 !!! returning error 0x%08x \n",
									pLgParam->LgNum, pLgParam->Param.NumOfAsyncRefreshIO, status));
			return status;	
		}
		pSftkLg->NumOfAsyncRefreshIO = pLgParam->Param.NumOfAsyncRefreshIO;
	}
	if (pLgParam->ParamFlags & LG_PARAM_FLAG_USE_AckWakeupTimeout)
	{
		if ( pLgParam->Param.AckWakeupTimeout == 0 )
		{
			status = STATUS_INVALID_PARAMETER;
			DebugPrint((DBG_ERROR, "sftk_ctl_lg_Set_Lg_TuningParam: Lgnum %d, pLgParam->Param.AckWakeupTimeout %I64x == 0 !!! returning error 0x%08x \n",
									pLgParam->LgNum, pLgParam->Param.AckWakeupTimeout, status));
			return status;	
		}
		pSftkLg->AckWakeupTimeout.QuadPart = pLgParam->Param.AckWakeupTimeout;
	}
	if (pLgParam->ParamFlags & LG_PARAM_FLAG_USE_RefreshThreadWakeupTimeout)
	{
		if ( pLgParam->Param.RefreshThreadWakeupTimeout == 0 )
		{
			status = STATUS_INVALID_PARAMETER;
			DebugPrint((DBG_ERROR, "sftk_ctl_lg_Set_Lg_TuningParam: Lgnum %d, pLgParam->Param.RefreshThreadWakeupTimeout %I64x == 0 !!! returning error 0x%08x \n",
									pLgParam->LgNum, pLgParam->Param.RefreshThreadWakeupTimeout, status));
			return status;	
		}
		pSftkLg->RefreshThreadWakeupTimeout.QuadPart = pLgParam->Param.RefreshThreadWakeupTimeout;
	}

	if (pLgParam->ParamFlags & LG_PARAM_FLAG_USE_sync_depth)
	{
		pSftkLg->sync_depth		= pLgParam->Param.sync_depth;
	}
	if (pLgParam->ParamFlags & LG_PARAM_FLAG_USE_sync_timeout)
	{
		pSftkLg->sync_timeout	= pLgParam->Param.sync_timeout;
	}
	if (pLgParam->ParamFlags & LG_PARAM_FLAG_USE_iodelay)
	{
		pSftkLg->iodelay		= pLgParam->Param.iodelay;
	}

	if (pLgParam->ParamFlags & LG_PARAM_FLAG_THROTAL_REFRESH_SEND_PKTS)
	{
		if ( pLgParam->Param.throtal_refresh_send_pkts <= 0 )
		{
			status = STATUS_INVALID_PARAMETER;
			DebugPrint((DBG_ERROR, "sftk_ctl_lg_Set_Lg_TuningParam: Lgnum %d, pLgParam->Param.throtal_refresh_send_pkts %d == 0 !!! returning error 0x%08x \n",
									pLgParam->LgNum, pLgParam->Param.throtal_refresh_send_pkts, status));
			return status;	
		}
		pSftkLg->throtal_refresh_send_pkts= pLgParam->Param.throtal_refresh_send_pkts;
	}

	if (pLgParam->ParamFlags & LG_PARAM_FLAG_THROTAL_REFRESH_SEND_TOTALSIZE)
	{
		pSftkLg->throtal_refresh_send_totalsize= pLgParam->Param.throtal_refresh_send_totalsize;
	}
	if (pLgParam->ParamFlags & LG_PARAM_FLAG_THROTAL_COMMIT_SEND_PKTS)
	{
		if ( pLgParam->Param.throtal_commit_send_pkts <= 0 )
		{
			status = STATUS_INVALID_PARAMETER;
			DebugPrint((DBG_ERROR, "sftk_ctl_lg_Set_Lg_TuningParam: Lgnum %d, pLgParam->Param.throtal_commit_send_pkts %d == 0 !!! returning error 0x%08x \n",
									pLgParam->LgNum, pLgParam->Param.throtal_commit_send_pkts, status));
			return status;	
		}
		pSftkLg->throtal_commit_send_pkts		= pLgParam->Param.throtal_commit_send_pkts;
	}
	if (pLgParam->ParamFlags & LG_PARAM_FLAG_THROTAL_COMMIT_SEND_TOTALSIZE)
	{
		pSftkLg->throtal_commit_send_totalsize = pLgParam->Param.throtal_commit_send_totalsize;
	}

	if (pLgParam->ParamFlags & LG_PARAM_FLAG_NumOfPktsSendAtaTime)
	{
		if ( pLgParam->Param.NumOfPktsSendAtaTime <= 0 )
		{
			status = STATUS_INVALID_PARAMETER;
			DebugPrint((DBG_ERROR, "sftk_ctl_lg_Set_Lg_TuningParam: Lgnum %d, pLgParam->Param.NumOfPktsSendAtaTime %d == 0 !!! returning error 0x%08x \n",
									pLgParam->LgNum, pLgParam->Param.NumOfPktsSendAtaTime, status));
			return status;	
		}
		pSftkLg->NumOfPktsSendAtaTime = pLgParam->Param.NumOfPktsSendAtaTime;
	}

	if (pLgParam->ParamFlags & LG_PARAM_FLAG_NumOfPktsRecvAtaTime)
	{
		if ( pLgParam->Param.NumOfPktsRecvAtaTime <= 0 )
		{
			status = STATUS_INVALID_PARAMETER;
			DebugPrint((DBG_ERROR, "sftk_ctl_lg_Set_Lg_TuningParam: Lgnum %d, pLgParam->Param.NumOfPktsRecvAtaTime %d == 0 !!! returning error 0x%08x \n",
									pLgParam->LgNum, pLgParam->Param.NumOfPktsRecvAtaTime, status));
			return status;	
		}
		pSftkLg->NumOfPktsRecvAtaTime = pLgParam->Param.NumOfPktsRecvAtaTime;
	}

	if (pLgParam->ParamFlags & LG_PARAM_FLAG_NumOfSendBuffers)
	{
		if ( pLgParam->Param.NumOfSendBuffers <= 0 )
		{
			status = STATUS_INVALID_PARAMETER;
			DebugPrint((DBG_ERROR, "sftk_ctl_lg_Set_Lg_TuningParam: Lgnum %d, pLgParam->Param.NumOfSendBuffers %d == 0 !!! returning error 0x%08x \n",
									pLgParam->LgNum, pLgParam->Param.NumOfSendBuffers, status));
			return status;	
		}
		pSftkLg->NumOfSendBuffers = pLgParam->Param.NumOfSendBuffers;
	}

#if DBG
	if (pLgParam->ParamFlags & LG_PARAM_FLAG_DebugLevel)
	{
		SwrDebug = pLgParam->Param.DebugLevel;
	}
#endif
	return status;
} // sftk_ctl_lg_Set_Lg_TuningParam()


NTSTATUS
sftk_ctl_get_lg_state_buffer(dev_t dev, int cmd, int arg, int flag)
{
	PSFTK_LG		pSftk_Lg	= NULL;
    stat_buffer_t	*sbptr		= (stat_buffer_t *)arg;

	// Retrieve LG Device
#if TARGET_SIDE
	pSftk_Lg = sftk_lookup_lg_by_lgnum( sbptr->lg_num, sbptr->lgCreationRole );
#else
    pSftk_Lg = sftk_lookup_lg_by_lgnum( sbptr->lg_num );
#endif
    if (pSftk_Lg == NULL) 
    {
		DebugPrint((DBG_ERROR, "sftk_ctl_get_dev_state_buffer: sftk_lookup_lg_by_lgnum() specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
							sbptr->lg_num, STATUS_DEVICE_DOES_NOT_EXIST));
        return STATUS_DEVICE_DOES_NOT_EXIST;	
    }
    // minor = getminor(sbptr->lg_num) & ~FTD_LGFLAG;	// get the logical group state 
    if (sbptr->len != (LONG) pSftk_Lg->statsize)
    {
		DebugPrint((DBG_ERROR, "sftk_ctl_get_dev_state_buffer: dev_num = %d has sbptr->len %d != pSftk_Lg->statsize %d !!! returning error 0x%08x \n",
							sbptr->dev_num, sbptr->len, pSftk_Lg->statsize, STATUS_INVALID_PARAMETER));
        return STATUS_INVALID_PARAMETER;
    }

	RtlCopyMemory( sbptr->addr, pSftk_Lg->statbuf, sbptr->len); 

    return STATUS_SUCCESS;;
} // sftk_ctl_get_lg_state_buffer()

NTSTATUS
sftk_ctl_set_lg_state_buffer( dev_t dev, int cmd, int arg, int flag )
{
	PSFTK_LG		pSftk_Lg	= NULL;
    stat_buffer_t	*sbptr		= (stat_buffer_t *)arg;

	// Retrieve LG Device
#if TARGET_SIDE
	pSftk_Lg = sftk_lookup_lg_by_lgnum( sbptr->lg_num, sbptr->lgCreationRole );
#else
    pSftk_Lg = sftk_lookup_lg_by_lgnum( sbptr->lg_num );
#endif
    if (pSftk_Lg == NULL) 
    {
		DebugPrint((DBG_ERROR, "sftk_ctl_set_dev_state_buffer: sftk_lookup_lg_by_lgnum() specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
							sbptr->lg_num, STATUS_DEVICE_DOES_NOT_EXIST));
        return STATUS_DEVICE_DOES_NOT_EXIST;	
    }
    // minor = getminor(sbptr->lg_num) & ~FTD_LGFLAG;	// get the logical group state 
    if (sbptr->len != (LONG) pSftk_Lg->statsize)
    {
		DebugPrint((DBG_ERROR, "sftk_ctl_set_dev_state_buffer: dev_num = %d has sbptr->len %d != pSftk_Lg->statsize %d !!! returning error 0x%08x \n",
							sbptr->dev_num, sbptr->len, pSftk_Lg->statsize, STATUS_INVALID_PARAMETER));
        return STATUS_INVALID_PARAMETER;
    }

	RtlCopyMemory( pSftk_Lg->statbuf, sbptr->addr, sbptr->len); 

    return STATUS_SUCCESS;;
} // sftk_ctl_set_lg_state_buffer()

NTSTATUS
sftk_ctl_lg_get_num_devices(dev_t dev, int cmd, int arg, int flag)
{
	
	PSFTK_LG		pSftk_Lg= NULL;
#if TARGET_SIDE
	// Fixed this dev is invalid use the arg parameter.
	ftd_state_t		*pFtd_State_state_s = (ftd_state_t *) arg;
	LONG			numDev	= pFtd_State_state_s->lg_num;
	pSftk_Lg = sftk_lookup_lg_by_lgnum( pFtd_State_state_s->lg_num, pFtd_State_state_s->lgCreationRole);
#else
	LONG			numDev	= *(int *)arg;
	pSftk_Lg = sftk_lookup_lg_by_lgnum( numDev );	// minor_num = getminor(num); 
#endif

    if (pSftk_Lg == NULL) 
    {
#if TARGET_SIDE
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_get_num_devices: sftk_lookup_lg_by_lgnum() specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
							pFtd_State_state_s->lg_num, STATUS_DEVICE_DOES_NOT_EXIST));
#else
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_get_num_devices: sftk_lookup_lg_by_lgnum() specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
							numDev, STATUS_DEVICE_DOES_NOT_EXIST));

#endif
        return STATUS_DEVICE_DOES_NOT_EXIST;	
    }

	OS_ACQUIRE_LOCK( &pSftk_Lg->Lock, OS_ACQUIRE_SHARED, TRUE, NULL);
	numDev = pSftk_Lg->LgDev_List.NumOfNodes;
	OS_RELEASE_LOCK( &pSftk_Lg->Lock, NULL);

	*((int *)arg) = numDev;

	return STATUS_SUCCESS;
} // sftk_ctl_lg_get_num_devices()

NTSTATUS
sftk_ctl_lg_start(dev_t dev, int cmd, int arg, int flag)
{
	PSFTK_LG	pSftk_Lg= NULL;
#if TARGET_SIDE
	ftd_state_t		*pFtd_State_state_s = (ftd_state_t *) dev;
	ULONG			lgNum	= pFtd_State_state_s->lg_num;
	pSftk_Lg = sftk_lookup_lg_by_lgnum( pFtd_State_state_s->lg_num, pFtd_State_state_s->lgCreationRole);
#else
	ULONG		lgNum	= *(ULONG *)arg;
	pSftk_Lg = sftk_lookup_lg_by_lgnum( lgNum );	// minor_num = getminor(num); 
#endif

    if (pSftk_Lg == NULL) 
    {
#if TARGET_SIDE
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_start: sftk_lookup_lg_by_lgnum() specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
							pFtd_State_state_s->lg_num, STATUS_DEVICE_DOES_NOT_EXIST));
#else
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_start: sftk_lookup_lg_by_lgnum() specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
							lgNum, STATUS_DEVICE_DOES_NOT_EXIST));
#endif
        return STATUS_DEVICE_DOES_NOT_EXIST;	
    }
	// FIXME: to make this work correctly, the init code should pass down 
    //	dirtybits and other state crap from the persistent store and then 
    //  we can decide what state to go into. Obviously, all of the state
    //  handling in this driver is terribly incomplete.

	// TODO: Its better we Read bitmap from pstore and then directly jump to
	//		 smart refresh on this call. this IOCTL may get use after reboot 
	//		 Anyhow after reboot, if we use binary config file than this operation
	//		should be taken care during boot time only.
	// During boot time, we must read binary config file and make first lg state to
	// tracking mode, once socket connection gets established we should switch to 
	// smart refresh mode later.

	DebugPrint((DBG_ERROR, "sftk_ctl_lg_start: FIXME FIXME TODO : Lgdev number = %d What to do Here ? Start Full Refresh Hre !! returning Success \n",
							lgNum ));

    switch (pSftk_Lg->state) 
    {
    case FTD_MODE_PASSTHRU:
    case FTD_MODE_TRACKING:
    case FTD_MODE_REFRESH:
    case FTD_MODE_NORMAL:	// do nothing 
        break;
    default:
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    // Need to somehow flush the state to disk here, no? XXX FIXME XXX //

	return STATUS_SUCCESS;
} // sftk_ctl_lg_start()

NTSTATUS
sftk_ctl_lg_get_num_groups(dev_t dev, int cmd, int arg, int flag)
{
    LONG			numLg	= 0;
#if TARGET_SIDE
	OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_SHARED, TRUE, NULL);
	
	if ( (ROLE_TYPE) (*((int*)arg ))== PRIMARY)
		numLg = GSftk_Config.Lg_GroupList.NumOfNodes;
	else
		numLg = GSftk_Config.TLg_GroupList.NumOfNodes;

	OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
#else
	OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_SHARED, TRUE, NULL);
	numLg = GSftk_Config.Lg_GroupList.NumOfNodes;
	OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
#endif

	*((int *)arg) = numLg;

	return STATUS_SUCCESS;
} // sftk_ctl_lg_get_num_groups()

NTSTATUS
sftk_ctl_lg_get_devices_info(dev_t dev, int cmd, int arg, int flag)
{
	PSFTK_LG		pSftk_Lg	= NULL;
	PSFTK_DEV		pSftkDev	= NULL;
	PLIST_ENTRY		plistEntry	= NULL;
	stat_buffer_t	sb;
	ftd_dev_info_t	*info, ditemp;
	LONG			totalSize, devRetCount, i;

	sb = *(stat_buffer_t *)arg;
	
#if TARGET_SIDE
	pSftk_Lg = sftk_lookup_lg_by_lgnum( sb.lg_num, sb.lgCreationRole );	// minor_num = getminor(num); 
#else
	pSftk_Lg = sftk_lookup_lg_by_lgnum( sb.lg_num );	// minor_num = getminor(num); 
#endif

    if (pSftk_Lg == NULL) 
    {
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_get_devices_info: sftk_ctl_lg_get_devices_info() specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
							sb.lg_num, STATUS_DEVICE_DOES_NOT_EXIST));
        return STATUS_DEVICE_DOES_NOT_EXIST;	
    }

	// Go thru each and every devices under this LG and get information
	OS_ACQUIRE_LOCK( &pSftk_Lg->Lock, OS_ACQUIRE_SHARED, TRUE, NULL);

	totalSize		= sizeof(ftd_dev_info_t) * pSftk_Lg->LgDev_List.NumOfNodes;
	devRetCount		= pSftk_Lg->LgDev_List.NumOfNodes;
	if (sb.len < totalSize)
	{
		devRetCount = sb.len / sizeof(ftd_dev_info_t);

		DebugPrint((DBG_ERROR, "sftk_ctl_lg_get_devices_info::Lgnum %d has (sb.len %d < totalSize %d (Total Device %d) ) !!! returning dev %d info only \n",
							sb.lg_num, sb.len, 
							totalSize, pSftk_Lg->LgDev_List.NumOfNodes, devRetCount));
	}
	
	info = (ftd_dev_info_t *)sb.addr;
	i = 0;
	for( plistEntry = pSftk_Lg->LgDev_List.ListEntry.Flink;
		 plistEntry != &pSftk_Lg->LgDev_List.ListEntry;
		 plistEntry = plistEntry->Flink)
	{ // for :scan thru each and every Devices under logical group 
		pSftkDev = CONTAINING_RECORD( plistEntry, SFTK_DEV, LgDev_Link);

		if (i >= devRetCount)
			break; // We are done with specified space.

		OS_ASSERT(pSftkDev->SftkLg == pSftk_Lg);
		OS_ASSERT(pSftkDev->LGroupNumber == pSftk_Lg->LGroupNumber);
	
		ditemp.lgnum		= pSftkDev->SftkLg->LGroupNumber;  // devp->lgp->dev; 
        ditemp.localcdev	= pSftkDev->localcdisk;
        ditemp.cdev			= pSftkDev->cdev;
        ditemp.bdev			= pSftkDev->bdev;
        ditemp.disksize		= pSftkDev->Disksize;
        ditemp.lrdbsize32	= pSftkDev->Lrdb.len32;
        ditemp.hrdbsize32	= pSftkDev->Hrdb.len32;
        ditemp.lrdb_res		= pSftkDev->Lrdb.bitsize;	
        ditemp.hrdb_res		= pSftkDev->Hrdb.bitsize;
        ditemp.lrdb_numbits = pSftkDev->Lrdb.numbits;
        ditemp.hrdb_numbits = pSftkDev->Hrdb.numbits;
        ditemp.statsize		= pSftkDev->statsize;

#if 1 // PARAG_ADDED
		ditemp.bUniqueVolumeIdValid = pSftkDev->bUniqueVolumeIdValid;
        ditemp.UniqueIdLength = pSftkDev->UniqueIdLength;
        RtlCopyMemory(ditemp.UniqueId, pSftkDev->UniqueId, sizeof(ditemp.UniqueId));

		ditemp.bSignatureUniqueVolumeIdValid = pSftkDev->bSignatureUniqueVolumeIdValid;
        ditemp.SignatureUniqueIdLength = pSftkDev->SignatureUniqueIdLength;
        RtlCopyMemory(ditemp.SignatureUniqueId, pSftkDev->SignatureUniqueId, sizeof(ditemp.SignatureUniqueId));

		ditemp.bSuggestedDriveLetterLinkValid = pSftkDev->bSuggestedDriveLetterLinkValid;
        ditemp.SuggestedDriveLetterLinkLength = pSftkDev->SuggestedDriveLetterLinkLength;
        RtlCopyMemory(ditemp.SuggestedDriveLetterLink, pSftkDev->SuggestedDriveLetterLink, sizeof(ditemp.SuggestedDriveLetterLink));
#endif

		OS_RtlCopyMemory( info, &ditemp, sizeof(ditemp) );

        info++;
		i++;
	
	} // for :scan thru each and every Devices under logical group 
	OS_RELEASE_LOCK( &pSftk_Lg->Lock, NULL);

    return STATUS_SUCCESS;
} // sftk_ctl_lg_get_devices_info()

NTSTATUS
sftk_ctl_lg_get_groups_info(dev_t dev, int cmd, int arg, int flag)
{
	PSFTK_LG		pSftk_Lg	= NULL;
	PLIST_ENTRY		plistEntry	= NULL;
	LONG			totalSize, devRetCount, i;
    ftd_lg_info_t	*info, lgtemp;
    stat_buffer_t	sb;
	

    sb = *(stat_buffer_t *)arg;

	info = (ftd_lg_info_t *)sb.addr;
	if (sb.lg_num != FTD_CTL) 
	{ // pass specified LG number only
#if TARGET_SIDE
		pSftk_Lg = sftk_lookup_lg_by_lgnum( sb.lg_num, sb.lgCreationRole );	// minor_num = getminor(num); 
#else
		pSftk_Lg = sftk_lookup_lg_by_lgnum( sb.lg_num );	// minor_num = getminor(num); 
#endif
		if (pSftk_Lg == NULL) 
		{
			DebugPrint((DBG_ERROR, "sftk_ctl_lg_get_groups_info: sftk_ctl_lg_get_devices_info() specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
								sb.lg_num, STATUS_DEVICE_DOES_NOT_EXIST));
			return STATUS_DEVICE_DOES_NOT_EXIST;	
		}

		if (sb.len < sizeof(lgtemp))
		{
			DebugPrint((DBG_ERROR, "sftk_ctl_lg_get_devices_info::Lgdev %d has (sb.len %d < sizeof(ftd_lg_info_t) %d !!! returning error 0x%08x\n",
							sb.lg_num, sb.len, sizeof(lgtemp), STATUS_BUFFER_TOO_SMALL));
			return STATUS_BUFFER_TOO_SMALL;
		}

		lgtemp.lgdev	= pSftk_Lg->LGroupNumber; // FIXME: minor_num? 
        lgtemp.statsize = pSftk_Lg->statsize;

		OS_RtlCopyMemory( info, &lgtemp, sizeof(lgtemp));

        return STATUS_SUCCESS;
	} // if (sb.lg_num != FTD_CTL) 

	OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_SHARED, TRUE, NULL);
	totalSize		= sizeof(ftd_lg_info_t) * GSftk_Config.Lg_GroupList.NumOfNodes;
	devRetCount		= GSftk_Config.Lg_GroupList.NumOfNodes;

	if (sb.len < totalSize)
	{
		devRetCount = sb.len / sizeof(ftd_lg_info_t);

		DebugPrint((DBG_ERROR, "sftk_ctl_lg_get_groups_info::(sb.len %d < totalSize %d (Total Device %d) ) !!! returning Total Lg %d info only \n",
							sb.len, totalSize, GSftk_Config.Lg_GroupList.NumOfNodes, devRetCount));
	}
	
	i = 0;
	for( plistEntry = GSftk_Config.Lg_GroupList.ListEntry.Flink;
		 plistEntry != &GSftk_Config.Lg_GroupList.ListEntry;
		 plistEntry = plistEntry->Flink)
	{ // for :scan thru each and every logical group 
		pSftk_Lg = CONTAINING_RECORD( plistEntry, SFTK_LG, Lg_GroupLink);

		if (i >= devRetCount)
			break; // We are done with specified space.

		lgtemp.lgdev	= pSftk_Lg->LGroupNumber; // FIXME: minor_num? 
        lgtemp.statsize = pSftk_Lg->statsize;

		OS_RtlCopyMemory( info, &lgtemp, sizeof(lgtemp));

		info ++;
		i ++;
	} // for :scan thru each and every logical group 

	OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);

    return STATUS_SUCCESS;
} // sftk_ctl_lg_get_groups_info()

NTSTATUS
sftk_ctl_lg_get_device_stats(dev_t dev, int cmd, int arg, int flag)
{
	PSFTK_LG		pSftk_Lg	= NULL;
	PSFTK_DEV		pSftk_Dev	= NULL;
	PLIST_ENTRY		plistEntry	= NULL;
    disk_stats_t    temp, *stats;
    stat_buffer_t   sb;
    ULONG			minor_num;
    LONG			totalSize, devRetCount, i;

    sb = *(stat_buffer_t *)arg;

    // minor_num = getminor(sb.lg_num);
#if TARGET_SIDE
	pSftk_Lg = sftk_lookup_lg_by_lgnum( sb.lg_num, sb.lgCreationRole );	// minor_num = getminor(num); 
#else
    pSftk_Lg = sftk_lookup_lg_by_lgnum( sb.lg_num );	// minor_num = getminor(num); 
#endif
	if (pSftk_Lg == NULL) 
	{
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_get_device_stats: sftk_ctl_lg_get_devices_info() specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
								sb.lg_num, STATUS_INVALID_PARAMETER));
		return STATUS_INVALID_PARAMETER;	
	}

    // get the device state 
	stats = (disk_stats_t *)sb.addr;
    minor_num = getminor(sb.dev_num);
    if (minor_num != L_MAXMIN) 
    { // Specified Device only
		// Retrieve Sftk Device
		pSftk_Dev	= sftk_lookup_dev_by_cdev( sb.dev_num );
		if (pSftk_Dev == NULL) 
		{
			DebugPrint((DBG_ERROR, "sftk_ctl_lg_get_device_stats: sftk_lookup_dev_by_cdev() specified dev_num = %d NOT exist !!! returning error 0x%08x \n",
								sb.dev_num, STATUS_INVALID_PARAMETER));
			return STATUS_INVALID_PARAMETER;	
		}

        strcpy(temp.devname, pSftk_Dev->Devname);
        temp.localbdisk		= pSftk_Dev->localbdisk;
        temp.localdisksize	= pSftk_Dev->Disksize;

        temp.readiocnt		= pSftk_Dev->readiocnt;
        temp.writeiocnt		= pSftk_Dev->writeiocnt;
        temp.sectorsread	= pSftk_Dev->sectorsread;
        temp.sectorswritten = pSftk_Dev->sectorswritten;
        temp.wlentries		= pSftk_Dev->wlentries;
        temp.wlsectors		= pSftk_Dev->wlsectors;

		OS_RtlCopyMemory( stats, &temp, sizeof(temp) );
		return STATUS_SUCCESS;
    }  // Specified Device only
    
	// Return all devices under LG 
    // Go thru each and every devices under this LG and get information
	OS_ACQUIRE_LOCK( &pSftk_Lg->Lock, OS_ACQUIRE_SHARED, TRUE, NULL);

	totalSize		= sizeof(disk_stats_t) * pSftk_Lg->LgDev_List.NumOfNodes;
	devRetCount		= pSftk_Lg->LgDev_List.NumOfNodes;
	if (sb.len < totalSize)
	{
		devRetCount = sb.len / sizeof(disk_stats_t);

		DebugPrint((DBG_ERROR, "sftk_ctl_lg_get_device_stats::Lgnum %d has (sb.len %d < totalSize %d (Total Device %d) ) !!! returning dev %d info only \n",
							sb.lg_num, sb.len, 
							totalSize, pSftk_Lg->LgDev_List.NumOfNodes, devRetCount));
	}

	i = 0;
	for( plistEntry = pSftk_Lg->LgDev_List.ListEntry.Flink;
		 plistEntry != &pSftk_Lg->LgDev_List.ListEntry;
		 plistEntry = plistEntry->Flink)
	{ // for :scan thru each and every Devices under logical group 
		pSftk_Dev = CONTAINING_RECORD( plistEntry, SFTK_DEV, LgDev_Link);

		if (i >= devRetCount)
			break; // We are done with specified space.

		OS_ASSERT(pSftk_Dev->SftkLg == pSftk_Lg);
		OS_ASSERT(pSftk_Dev->LGroupNumber == pSftk_Lg->LGroupNumber);
	
		strcpy(temp.devname, pSftk_Dev->Devname);
        temp.localbdisk		= pSftk_Dev->localbdisk;
        temp.localdisksize	= pSftk_Dev->Disksize;

        temp.readiocnt		= pSftk_Dev->readiocnt;
        temp.writeiocnt		= pSftk_Dev->writeiocnt;
        temp.sectorsread	= pSftk_Dev->sectorsread;
        temp.sectorswritten = pSftk_Dev->sectorswritten;
        temp.wlentries		= pSftk_Dev->wlentries;
        temp.wlsectors		= pSftk_Dev->wlsectors;

        OS_RtlCopyMemory( stats, &temp, sizeof(temp) );

        stats ++;
		i ++;
	} // for :scan thru each and every Devices under logical group 

	OS_RELEASE_LOCK( &pSftk_Lg->Lock, NULL);

    return STATUS_SUCCESS;
} // sftk_ctl_lg_get_device_stats()

NTSTATUS
sftk_ctl_lg_get_group_stats(dev_t dev, int cmd, int arg, int flag)
{
	PSFTK_LG		pSftk_Lg	= NULL;
	PLIST_ENTRY		plistEntry	= NULL;
    ftd_stat_t		temp, *stats;
    stat_buffer_t   sb;
    ULONG			minor_num;
    LONG			totalSize, devRetCount, i;

    sb = *(stat_buffer_t *)arg;

    // minor_num = getminor(sb.lg_num);
    
    // get the device state 
	stats = (ftd_stat_t *)sb.addr;
    minor_num = getminor(sb.lg_num);
    if (minor_num != L_MAXMIN) 
    { // Specified Device only
		// Retrieve Sftk Device
#if TARGET_SIDE
		pSftk_Lg = sftk_lookup_lg_by_lgnum( sb.lg_num, sb.lgCreationRole );	// minor_num = getminor(num); 
#else
		pSftk_Lg = sftk_lookup_lg_by_lgnum( sb.lg_num );	// minor_num = getminor(num); 
#endif
		if (pSftk_Lg == NULL) 
		{
			DebugPrint((DBG_ERROR, "sftk_ctl_lg_get_group_stats: sftk_ctl_lg_get_devices_info() specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
									sb.lg_num, STATUS_INVALID_PARAMETER));
			return STATUS_INVALID_PARAMETER;	
		}

        temp.lgnum			= pSftk_Lg->LGroupNumber & ~FTD_LGFLAG;	// lgp->dev & ~FTD_LGFLAG;
        temp.wlentries		= pSftk_Lg->wlentries;
        temp.wlsectors		= pSftk_Lg->wlsectors;
		// TODO : Call MM to get Group based Free and Used memory information.....
		temp.bab_free		= 0;	// TODO
		temp.bab_used		= 0;	// TODO
        // temp.bab_free		= ftd_bab_get_free_length(pSftk_Lg->mgr) * sizeof(ftd_uint64_t);
        // temp.bab_used		= ftd_bab_get_used_length(pSftk_Lg->mgr) * sizeof(ftd_uint64_t);
        temp.state			= pSftk_Lg->state;
        temp.ndevs			= pSftk_Lg->LgDev_List.NumOfNodes;	// This is total number of devices under LG
        temp.sync_depth		= pSftk_Lg->sync_depth;
        temp.sync_timeout	= pSftk_Lg->sync_timeout;
        temp.iodelay		= pSftk_Lg->iodelay;

		OS_RtlCopyMemory( stats, &temp, sizeof(temp) );

		return STATUS_SUCCESS;
    }  // Specified Device only
    
	// Return all LG information
    // Go thru each and every devices under this LG and get information
	OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_SHARED, TRUE, NULL);
	totalSize		= sizeof(ftd_stat_t) * GSftk_Config.Lg_GroupList.NumOfNodes;
	devRetCount		= GSftk_Config.Lg_GroupList.NumOfNodes;

	if (sb.len < totalSize)
	{
		devRetCount = sb.len / sizeof(ftd_stat_t);

		DebugPrint((DBG_ERROR, "sftk_ctl_lg_get_group_stats::(sb.len %d < totalSize %d (Total Device %d) ) !!! returning Total Lg %d info only \n",
							sb.len, totalSize, GSftk_Config.Lg_GroupList.NumOfNodes, devRetCount));
	}
	
	i = 0;
	for( plistEntry = GSftk_Config.Lg_GroupList.ListEntry.Flink;
		 plistEntry != &GSftk_Config.Lg_GroupList.ListEntry;
		 plistEntry = plistEntry->Flink)
	{ // for :scan thru each and every logical group 
		pSftk_Lg = CONTAINING_RECORD( plistEntry, SFTK_LG, Lg_GroupLink);

		if (i >= devRetCount)
			break; // We are done with specified space.

		temp.lgnum			= pSftk_Lg->LGroupNumber & ~FTD_LGFLAG;	// lgp->dev & ~FTD_LGFLAG;
        temp.wlentries		= pSftk_Lg->wlentries;
        temp.wlsectors		= pSftk_Lg->wlsectors;
		// TODO : Call MM to get Group based Free and Used memory information.....
		temp.bab_free		= 0;	// TODO: call Jerome for this value
		temp.bab_used		= 0;	// TODO: call Jerome for this value
        // temp.bab_free		= ftd_bab_get_free_length(pSftk_Lg->mgr) * sizeof(ftd_uint64_t);

		// bab_free - is nothing but total number of free space in BAB (Cache manager) including 
		// free space thorws just from Specified LG.
		// = Total overall Free space of cache manager + Total specified LG free space 

        // temp.bab_used		= ftd_bab_get_used_length(pSftk_Lg->mgr) * sizeof(ftd_uint64_t);
		// bab_used - is nothing but total number of used space from BAB (Cache manager) for Specified LG.
		// = Total specified LG's used allocated space.

        temp.state			= pSftk_Lg->state;
        temp.ndevs			= pSftk_Lg->LgDev_List.NumOfNodes;	// This is total number of devices under LG
        temp.sync_depth		= pSftk_Lg->sync_depth;
        temp.sync_timeout	= pSftk_Lg->sync_timeout;
        temp.iodelay		= pSftk_Lg->iodelay;

		OS_RtlCopyMemory( stats, &temp, sizeof(temp) );

        stats ++;
		i ++;
	} // for :scan thru each and every Devices under logical group 

	OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
    return STATUS_SUCCESS;
} // sftk_ctl_lg_get_group_stats()

// Clear complete specified Bitmaps
NTSTATUS
sftk_ctl_lg_clear_dirtybits(int arg, int type)
{
	PSFTK_LG		pSftk_Lg	= NULL;
	PLIST_ENTRY		plistEntry  = NULL;
	PSFTK_DEV		pSftk_Dev	= NULL;
    dev_t			lgNum;

#if TARGET_SIDE
	ftd_state_t		*pFtd_State_state_s = (ftd_state_t *) arg;
	lgNum = pFtd_State_state_s->lg_num;

	pSftk_Lg = sftk_lookup_lg_by_lgnum( pFtd_State_state_s->lg_num, pFtd_State_state_s->lgCreationRole );	
#else
	lgNum = *(dev_t *)arg;
	pSftk_Lg = sftk_lookup_lg_by_lgnum( lgNum );	// minor_num = getminor(dev) & ~FTD_LGFLAG; 
#endif
	if (pSftk_Lg == NULL) 
	{
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_clear_dirtybits: sftk_ctl_lg_get_devices_info() specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
									lgNum, STATUS_DEVICE_DOES_NOT_EXIST));
		return STATUS_DEVICE_DOES_NOT_EXIST;	
	}

	OS_ACQUIRE_LOCK( &pSftk_Lg->Lock, OS_ACQUIRE_SHARED, TRUE, NULL);
	
	for( plistEntry = pSftk_Lg->LgDev_List.ListEntry.Flink;
		 plistEntry != &pSftk_Lg->LgDev_List.ListEntry;
		 plistEntry = plistEntry->Flink)
	{ // for :scan thru each and every Devices under logical group 
		pSftk_Dev = CONTAINING_RECORD( plistEntry, SFTK_DEV, LgDev_Link);

		OS_ASSERT(pSftk_Dev->SftkLg == pSftk_Lg);
		OS_ASSERT(pSftk_Dev->LGroupNumber == pSftk_Lg->LGroupNumber);

		if (type == FTD_LOW_RES_DIRTYBITS) 
		{	// Clears all bitmaps of LRDB
			OS_ACQUIRE_LOCK( &pSftk_Dev->Lrdb.Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
			RtlClearAllBits( pSftk_Dev->Lrdb.pBitmapHdr ); 
			// flush it to pstore file
			OS_RELEASE_LOCK( &pSftk_Dev->Lrdb.Lock, NULL);
		}
		
		// TODO : We must update LRDB and HRDB both simultenously all the time.... FIXME FIXME 
		// This is from old code !!! Do we need this any more !!! FIXME FIXME 
		if (type == FTD_HIGH_RES_DIRTYBITS) 
		{	// Clears all bitmaps of HRDB
			OS_ACQUIRE_LOCK( &pSftk_Dev->Hrdb.Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
			RtlClearAllBits( pSftk_Dev->Hrdb.pBitmapHdr ); 
			OS_RELEASE_LOCK( &pSftk_Dev->Hrdb.Lock, NULL);
		}

		sftk_flush_all_bitmaps_to_pstore(pSftk_Dev, FALSE, NULL, FALSE, FALSE);
	} // for :scan thru each and every Devices under logical group 
	
	OS_RELEASE_LOCK( &pSftk_Lg->Lock, NULL);
    
    return STATUS_SUCCESS;;
} // sftk_ctl_lg_clear_dirtybits()

// Clear complete specified Bitmaps
NTSTATUS
sftk_ctl_lg_group_state(dev_t dev, int cmd, int arg, int flag)
{
	PSFTK_LG		pSftk_Lg	= NULL;
    dev_t			lg_num;

	lg_num = ((ftd_state_t *)arg)->lg_num;

#if TARGET_SIDE
	pSftk_Lg = sftk_lookup_lg_by_lgnum( lg_num, ((ftd_state_t *)arg)->lgCreationRole );	
#else
    pSftk_Lg = sftk_lookup_lg_by_lgnum( lg_num );	// minor_num = getminor(dev) & ~FTD_LGFLAG; 
#endif
	if (pSftk_Lg == NULL) 
	{
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_group_state: sftk_ctl_lg_get_devices_info() specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
									lg_num, STATUS_DEVICE_DOES_NOT_EXIST));
		return STATUS_DEVICE_DOES_NOT_EXIST;	
	}

	OS_ACQUIRE_LOCK( &pSftk_Lg->Lock, OS_ACQUIRE_SHARED, TRUE, NULL);
	((ftd_state_t *)arg)->state = pSftk_Lg->state;
	((ftd_state_t *)arg)->bInconsistantData = pSftk_Lg->bInconsistantData;
	OS_RELEASE_LOCK( &pSftk_Lg->Lock, NULL);

	return STATUS_SUCCESS;
} // sftk_ctl_lg_group_state()

NTSTATUS
sftk_ctl_lg_set_iodelay(dev_t dev, int cmd, int arg, int flag)
{
	PSFTK_LG		pSftk_Lg	= NULL;
	ftd_param_t     vb;

    vb = *(ftd_param_t *)arg;

#if TARGET_SIDE
	pSftk_Lg = sftk_lookup_lg_by_lgnum( vb.lgnum, vb.lgCreationRole );	
#else
	pSftk_Lg = sftk_lookup_lg_by_lgnum( vb.lgnum );	// minor_num = getminor(vb.lgnum) & ~FTD_LGFLAG;
#endif
	if (pSftk_Lg == NULL) 
	{
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_set_iodelay: sftk_ctl_lg_get_devices_info() specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
									vb.lgnum, STATUS_DEVICE_DOES_NOT_EXIST));
		return STATUS_DEVICE_DOES_NOT_EXIST;	
	}

	OS_ACQUIRE_LOCK( &pSftk_Lg->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	pSftk_Lg->iodelay = vb.value;
	OS_RELEASE_LOCK( &pSftk_Lg->Lock, NULL);

    return STATUS_SUCCESS;;
} // sftk_ctl_lg_set_iodelay()


NTSTATUS
sftk_ctl_lg_set_sync_depth(dev_t dev, int cmd, int arg, int flag)
{
	PSFTK_LG		pSftk_Lg	= NULL;
	ftd_param_t     vb;

    vb = *(ftd_param_t *)arg;

#if TARGET_SIDE
	pSftk_Lg = sftk_lookup_lg_by_lgnum( vb.lgnum, vb.lgCreationRole );	
#else
	pSftk_Lg = sftk_lookup_lg_by_lgnum( vb.lgnum );	// minor_num = getminor(vb.lgnum) & ~FTD_LGFLAG;
#endif
	if (pSftk_Lg == NULL) 
	{
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_set_sync_depth: sftk_ctl_lg_get_devices_info() specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
									vb.lgnum, STATUS_DEVICE_DOES_NOT_EXIST));
		return STATUS_DEVICE_DOES_NOT_EXIST;	
	}

	if (vb.value == 0)
	{
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_set_sync_depth: sftk_ctl_lg_get_devices_info() specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
									vb.lgnum, STATUS_INVALID_PARAMETER));
		return STATUS_INVALID_PARAMETER;	
	}

	OS_ACQUIRE_LOCK( &pSftk_Lg->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	// ftd_do_sync_done(lgp);
	pSftk_Lg->sync_depth = vb.value;
	OS_RELEASE_LOCK( &pSftk_Lg->Lock, NULL);

    return STATUS_SUCCESS;;
} // sftk_ctl_lg_set_sync_depth()

NTSTATUS
sftk_ctl_lg_set_sync_timeout(dev_t dev, int cmd, int arg, int flag)
{
	PSFTK_LG		pSftk_Lg	= NULL;
	ftd_param_t     vb;

    vb = *(ftd_param_t *)arg;

#if TARGET_SIDE
	pSftk_Lg = sftk_lookup_lg_by_lgnum( vb.lgnum, vb.lgCreationRole );	
#else
	pSftk_Lg = sftk_lookup_lg_by_lgnum( vb.lgnum );	// minor_num = getminor(vb.lgnum) & ~FTD_LGFLAG;
#endif
	if (pSftk_Lg == NULL) 
	{
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_set_sync_timeout: sftk_ctl_lg_get_devices_info() specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
									vb.lgnum, STATUS_DEVICE_DOES_NOT_EXIST));
		return STATUS_DEVICE_DOES_NOT_EXIST;	
	}

	if (vb.value == 0)
	{
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_set_sync_timeout: sftk_ctl_lg_get_devices_info() specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
									vb.lgnum, STATUS_INVALID_PARAMETER));
		return STATUS_INVALID_PARAMETER;	
	}

	OS_ACQUIRE_LOCK( &pSftk_Lg->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	pSftk_Lg->sync_timeout = vb.value;
	OS_RELEASE_LOCK( &pSftk_Lg->Lock, NULL);

    return STATUS_SUCCESS;;
} // sftk_ctl_lg_set_sync_timeout()

//clear the bab for this group
NTSTATUS
sftk_ctl_lg_clear_bab(dev_t dev, int cmd, int arg, int flag)
{
	PSFTK_LG		pSftk_Lg	= NULL;
    dev_t			lgNum;
#if TARGET_SIDE
	ftd_param_t     vb;
    vb = *(ftd_param_t *)arg;
	pSftk_Lg = sftk_lookup_lg_by_lgnum( vb.lgnum, vb.lgCreationRole );	
	lgNum = vb.lgnum;
#else
	lgNum = *(dev_t *)arg;
	pSftk_Lg = sftk_lookup_lg_by_lgnum( lgNum );	// minor_num = getminor(num); 
#endif
	if (pSftk_Lg == NULL) 
	{
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_clear_bab: sftk_ctl_lg_get_devices_info() specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
									lgNum, STATUS_DEVICE_DOES_NOT_EXIST));
		return STATUS_DEVICE_DOES_NOT_EXIST;	
	}

	// LOCK 
	DebugPrint((DBG_ERROR, "sftk_ctl_lg_clear_bab: TODO TODO FIXME : Call Cache manager to clear LG data, Not Implemented...Returning success !!\n",
									lgNum));
	// UNLOCK 
    
    return STATUS_SUCCESS;;
} // sftk_ctl_lg_clear_bab()

//Get total Bab Cache Manager size
NTSTATUS
sftk_ctl_get_bab_size(dev_t dev, int cmd, int arg, int flag)
{

	// LOCK 
	// *((int *)arg) = ctlp->bab_size; // Return total number of cache like this
	DebugPrint((DBG_ERROR, "sftk_ctl_lg_get_bab_size: TODO TODO FIXME : Call Cache manager to get Total Cache Manage (Bab) Size, Not Implemented...Returning success !!\n"));
	// UNLOCK 
    
    return STATUS_SUCCESS;;
} // sftk_ctl_get_bab_size()


NTSTATUS
sftk_ctl_lg_set_group_state(dev_t dev, int cmd, int arg, int flag)
{
	PSFTK_LG		pSftk_Lg	= NULL;
    ftd_state_t		sb;
	NTSTATUS		status;
    
    LONG			totalSize, devRetCount, i;
	
	sb = *(ftd_state_t *)arg;

    // Retrieve Sftk Device
#if TARGET_SIDE
	pSftk_Lg = sftk_lookup_lg_by_lgnum( sb.lg_num, sb.lgCreationRole );	
#else
	pSftk_Lg = sftk_lookup_lg_by_lgnum( sb.lg_num );	// minor_num = getminor(sb.lg_num) & ~FTD_LGFLAG; 
#endif
	if (pSftk_Lg == NULL) 
	{
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_set_group_state: sftk_lookup_lg_by_lgnum() specified Lgdev number = %d NOT exist !!! returning error 0x%08x \n",
									sb.lg_num, STATUS_DEVICE_DOES_NOT_EXIST));
		return STATUS_DEVICE_DOES_NOT_EXIST;	
	}

	if (pSftk_Lg->state == sb.state)
	{
		DebugPrint((DBG_IOCTL, "sftk_ctl_lg_set_group_state: LG 0x%08x, New state is same as old 0x%08x !!! returning success 0x%08x  \n",
									sb.lg_num, sb.state, STATUS_SUCCESS));
		return STATUS_SUCCESS;
	}

	status = sftk_lg_change_State(pSftk_Lg, pSftk_Lg->state, sb.state, TRUE);
	if (status != STATUS_SUCCESS) 
	{
		DebugPrint((DBG_ERROR, "sftk_ctl_lg_set_group_state: sftk_lg_change_State( LG 0x%08x, prevState 0x%08x,NewState 0x%08x) Failed with error 0x%08x \n",
									sb.lg_num, pSftk_Lg->state, sb.state, status));
		return status;	
	}

	return STATUS_SUCCESS;
} // sftk_ctl_lg_set_group_state()

/*
ULONG
sftk_GetLGState( PSFTK_LG	Sftk_LG)
{
	CC_lgstate_t retState;
	switch(Sftk_LG->state)
	{
		case	SFTK_MODE_PASSTHRU:			retState = PASSTHRU;	break;

		case	SFTK_MODE_FULL_REFRESH:		
		case	SFTK_MODE_SMART_REFRESH:	retState = REFRESH ;	break;

		case	SFTK_MODE_TRACKING:			retState = TRACKING;	break;

		case	SFTK_MODE_NORMAL:			retState = NORMAL;		break;
		
		case	SFTK_MODE_BACKFRESH:
		default:							retState = PASSTHRU;	break;
	}

	return retState;

} // sftk_GetLGState;
*/

#if TARGET_SIDE

// Pause or Resume a Full Refresh. 
// 1 - PAUSE , 0 - RESUME
NTSTATUS
sftk_lg_ctl_fullrefresh_pause_resume( PIRP Irp)
{
	NTSTATUS			status				= STATUS_SUCCESS;
	PIO_STACK_LOCATION  pCurStackLocation	= IoGetCurrentIrpStackLocation(Irp);
	ftd_param_t			*pLgParam			= Irp->AssociatedIrp.SystemBuffer;
	ULONG				sizeOfBuffer		= pCurStackLocation->Parameters.DeviceIoControl.InputBufferLength;
    PSFTK_LG			pSftk_Lg			= NULL;
	
	if (sizeOfBuffer < sizeof(ftd_param_t))
	{
		status = STATUS_BUFFER_OVERFLOW;
		DebugPrint((DBG_ERROR, "sftk_lg_ctl_fullrefresh_pause_resume: LG %d, sizeOfBuffer %d < sizeof(ftd_param_t) %d, Failed with status 0x%08x !!! \n",
								pLgParam->lgnum, sizeOfBuffer, sizeof(ftd_param_t), status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	try
	{
		pSftk_Lg = sftk_lookup_lg_by_lgnum( pLgParam->lgnum, pLgParam->lgCreationRole );
		if (pSftk_Lg == NULL) 
		{ // LG Already exist
			status = STATUS_DEVICE_DOES_NOT_EXIST;
			DebugPrint((DBG_ERROR, "sftk_lg_ctl_fullrefresh_pause_resume: specified Lgdev number = %d does NOT exist !!! returning error 0x%08x \n",
						pLgParam->lgnum, status));
			leave;
		}

		// issue the command to Resume OR Pause the Full Refresh
		// If Pause
		// Set the Falg bPauseFullRefresh falg to TRUE
		// Stop all the Sessions of the SESSION_MANAGER
		// Stop the Connect thread
		// If Resume
		// Set the Falg bPauseFullRefresh falg to FALSE
		// Start all the Sessions of the SESSION_MANAGER
		// Start the Connect Thread
		status = sftk_lg_fullrefresh_pause_resume(pSftk_Lg, pLgParam->value);
	} // try
	finally
	{

	}
	return status;
} // sftk_lg_ctl_fullrefresh_pause_resume()

// Pause or Resume a Full Refresh. 
// 1 - PAUSE , 0 - RESUME
NTSTATUS
sftk_lg_fullrefresh_pause_resume( PSFTK_LG	Sftk_Lg , EXECUTION_STATE ExecState)
{
	NTSTATUS			status				= STATUS_SUCCESS;

	try
	{
		if (LG_IS_SECONDARY_MODE(Sftk_Lg))
		{ // LG current role is Secondary
			// If the current Role is Secondary then we cannot do anything in a full refresh mode so
			// just returning an Error of unsupported command
			status = STATUS_INVALID_PARAMETER;
			DebugPrint((DBG_ERROR, "sftk_lg_fullrefresh_pause_resume: specified Lgdev number = %d is in Secondary Mode Command not supported returning error 0x%08x \n",
						Sftk_Lg->LGroupNumber, status));
			leave;

		} // if (LG_IS_SECONDARY_MODE(Sftk_Lg))


		if (LG_IS_PRIMARY_MODE(Sftk_Lg))
		{ // LG current role is primary 

			if(ExecState == PAUSE)
			{ // PAUSE
				if (sftk_lg_get_state(Sftk_Lg) == SFTK_MODE_FULL_REFRESH)
				{ // We are in Full Refresh so it is OK to Pause
					Sftk_Lg->bPauseFullRefresh = TRUE;

					// Stop all the sessions and then
					// Stop the CONNECT Thread so that we will not even try to connect to the Remote System
					COM_StopPMD(&Sftk_Lg->SessionMgr);
				}
				else
				{ // Full Refresh is already done OR we are not in a Full Refresh mode so cannot pause
					status = STATUS_INVALID_PARAMETER;
					DebugPrint((DBG_ERROR, "sftk_lg_fullrefresh_pause_resume: specified Lgdev number = %d is not in Full Refersh so cannot pause returning error 0x%08x \n",
								Sftk_Lg->LGroupNumber, status));
				}
			} // if(pLgParam->value == PAUSE)
			else if(ExecState == RESUME)
			{ // RESUME
				if (sftk_lg_get_state(Sftk_Lg) == SFTK_MODE_FULL_REFRESH)
				{ // We are in Full Refresh Mode Just Reset the Falg bPauseFullRefresh to False
					Sftk_Lg->bPauseFullRefresh = FALSE;

					if (sftk_lg_is_socket_alive(Sftk_Lg ) == FALSE)
					{ // connection does not exist, try to establish connection....
						SM_INIT_PARAMS	sm_Params;

						RtlZeroMemory( &sm_Params, sizeof(sm_Params));

						// set all values as default....TODO: Service must have passed this default values during LG create !!
						sm_Params.lgnum						= Sftk_Lg->LGroupNumber;
						sm_Params.nSendWindowSize			= CONFIGURABLE_MAX_SEND_BUFFER_SIZE( Sftk_Lg->MaxTransferUnit, Sftk_Lg->NumOfPktsSendAtaTime);	
						sm_Params.nMaxNumberOfSendBuffers	= DEFAULT_MAX_SEND_BUFFERS;	// 5 defined in ftdio.h
						sm_Params.nReceiveWindowSize		= CONFIGURABLE_MAX_RECIEVE_BUFFER_SIZE( Sftk_Lg->MaxTransferUnit,Sftk_Lg->NumOfPktsRecvAtaTime);
						sm_Params.nMaxNumberOfReceiveBuffers= DEFAULT_MAX_RECEIVE_BUFFERS;	// 5 defined in ftdio.h
						sm_Params.nChunkSize				= 0;	 
						sm_Params.nChunkDelay				= 0;	 

						status = COM_StartPMD( &Sftk_Lg->SessionMgr, &sm_Params);
						if (!NT_SUCCESS(status))
						{ // socket connection failed....bumer...called mike to fix this batch process error handling....
							DebugPrint((DBG_ERROR, "sftk_lg_fullrefresh_pause_resume: Primary : SFTK_IOCTL_START_PMD: COM_StartPMD(LgNum %d) Failed with status 0x%08x \n", 
											Sftk_Lg->LGroupNumber, status));
							
						} // socket connection failed....bumer...called mike to fix this batch process error handling....
					} // if (sftk_lg_is_socket_alive( Sftk_Lg ) == FALSE)
				} // if (sftk_lg_get_state(Sftk_Lg) == SFTK_MODE_FULL_REFRESH)
				else
				{
					// The Current Mode is not Full Refresh so this is not supported error
					// because it is only possible to resume the Full Refresh if we are in Full Refresh Mode
					status = STATUS_INVALID_PARAMETER;
					DebugPrint((DBG_ERROR, "sftk_lg_fullrefresh_pause_resume: specified Lgdev number = %d is not in Full Refersh so cannot Resume returning error 0x%08x \n",
								Sftk_Lg->LGroupNumber, status));
				}
			} // else if(pLgParam->value == RESUME)
			else
			{
				status = STATUS_INVALID_PARAMETER;
				DebugPrint((DBG_ERROR, "sftk_lg_fullrefresh_pause_resume: specified Lgdev number = %d Value %d not supported returning error 0x%08x \n",
							Sftk_Lg->LGroupNumber, ExecState, status));
			}
		} // if (LG_IS_PRIMARY_MODE(Sftk_Lg))
	} // try
	finally
	{

	}
	return status;
} // sftk_lg_fullrefresh_pause_resume()


NTSTATUS
sftk_lg_failover( PIRP Irp)
{
	NTSTATUS			status				= STATUS_SUCCESS;
	PIO_STACK_LOCATION  pCurStackLocation	= IoGetCurrentIrpStackLocation(Irp);
	PFAIL_OVER			pFailOver			= Irp->AssociatedIrp.SystemBuffer;
	ULONG				sizeOfBuffer		= pCurStackLocation->Parameters.DeviceIoControl.InputBufferLength;
    PSFTK_LG			pSftk_Lg			= NULL;
	
	if (sizeOfBuffer < sizeof(FAIL_OVER))
	{
		status = STATUS_BUFFER_OVERFLOW;
		DebugPrint((DBG_ERROR, "sftk_lg_failover: LG %d, sizeOfBuffer %d < sizeof(FAIL_OVER) %d, Failed with status 0x%08x !!! \n",
										pFailOver->LgNum, sizeOfBuffer, sizeof(FAIL_OVER), status));
		return status;	// The returned data was too large to fit into the specified input buffer, try with bigger size.
	}

	pSftk_Lg = sftk_lookup_lg_by_lgnum( pFailOver->LgNum, pFailOver->lgCreationRole );
	if (pSftk_Lg == NULL) 
    { // LG Already exist
		status = STATUS_DEVICE_DOES_NOT_EXIST;
		DebugPrint((DBG_ERROR, "sftk_lg_failover: specified Lgdev number = %d does NOT exist !!! returning error 0x%08x \n",
					pFailOver->LgNum, status));
		goto done;
	}

	if (LG_IS_PRIMARY_MODE(pSftk_Lg))
	{ // LG current role is primary 
		// - TODO Veera: Check if connection exist, if not then try to establish connection if connection can not get 
		// established with timeout fail this command
		if (sftk_lg_is_socket_alive( pSftk_Lg ) == FALSE)
		{ // connection does not exist, try to establish connection....
			SM_INIT_PARAMS	sm_Params;

			RtlZeroMemory( &sm_Params, sizeof(sm_Params));

			// set all values as default....TODO: Service must have passed this default values during LG create !!
			sm_Params.lgnum						= pSftk_Lg->LGroupNumber;
			sm_Params.nSendWindowSize			= CONFIGURABLE_MAX_SEND_BUFFER_SIZE( pSftk_Lg->MaxTransferUnit, pSftk_Lg->NumOfPktsSendAtaTime);	
			sm_Params.nMaxNumberOfSendBuffers	= DEFAULT_MAX_SEND_BUFFERS;	// 5 defined in ftdio.h
			sm_Params.nReceiveWindowSize		= CONFIGURABLE_MAX_RECIEVE_BUFFER_SIZE( pSftk_Lg->MaxTransferUnit,pSftk_Lg->NumOfPktsRecvAtaTime);
			sm_Params.nMaxNumberOfReceiveBuffers= DEFAULT_MAX_RECEIVE_BUFFERS;	// 5 defined in ftdio.h
			sm_Params.nChunkSize				= 0;	 
			sm_Params.nChunkDelay				= 0;	 

			status = COM_StartPMD( &pSftk_Lg->SessionMgr, &sm_Params);
			if (!NT_SUCCESS(status))
			{ // socket connection failed....bumer...called mike to fix this batch process error handling....
				DebugPrint((DBG_ERROR, "sftk_lg_failover: Primary : SFTK_IOCTL_START_PMD: COM_StartPMD(LgNum %d) Failed with status 0x%08x \n", 
								pSftk_Lg->LGroupNumber, status));
				
				// Log event message here for connection failure
				#if 0
				/*
				swprintf( wStr1, L"%d", Sftk_Lg->LGroupNumber);
				swprintf( wStr2, L"0x%08X", status );
				sftk_LogEventString2(	GSftk_Config.DriverObject, MSG_LG_START_SOCKET_CONNECTION_ERROR, status, 0, 
										wStr1, wStr2);

				// LG State change error log event message
				swprintf( wStr1, L"%d", Sftk_Lg->LGroupNumber);
				sftk_get_lg_state_string(PrevState, wStr2);
				sftk_get_lg_state_string(NewState, wStr3);
				swprintf( wStr4, L"Connection");
				sftk_LogEventWchar4(	GSftk_Config.DriverObject, MSG_REPL_STATE_CHANGE_ERROR, status, 0, 
										wStr1, wStr2, wStr3, wStr4);
				*/
				#endif
				// status = STATUS_INVALID_DEVICE_REQUEST;
				DebugPrint((DBG_ERROR, "sftk_lg_failover: Primary : Lgdev = %d failed StartPMD() for FAIL_OVER!!! returning error 0x%08x \n",
						pSftk_Lg->LGroupNumber, status));
				goto done;
			} // socket connection failed....bumer...called mike to fix this batch process error handling....
		}

		// - TODO Veera: Send Internal Failover Protocol command to Remote system, if it fails return that error status
		// structure initialize for failover with Journal or not, pass it to secondary in proto command
		// status = QM_SendOutBandPkt( pSftk_Lg, TRUE, TRUE, FAIL_OVER_PROTO_CMD);
		if (!NT_SUCCESS(status)) 
		{ // Out band Proto Failover failed
			// status = STATUS_DEVICE_DOES_NOT_EXIST;
			DebugPrint((DBG_ERROR, "sftk_lg_failover: Primary : Lgdev = %d failed outband Proto Command for FAIL_OVER!!! returning error 0x%08x \n",
						pSftk_Lg->LGroupNumber, status));
			goto done;
		}
		
		// -If Failover with journal mode, Keep doing primary work as it is....and return status message back to IOCTL caller.
		if (pFailOver->UseJournal == TRUE)
		{
			status = STATUS_SUCCESS;
			DebugPrint((DBG_ERROR, "sftk_lg_failover: Primary Lgdev = %d failover with Journal !!! returning status 0x%08x \n",
					pFailOver->LgNum, status));
			goto done;
		}

		// -If Failover with NO Journal mode, switch primary state to user mode defined Tracking.
		if (pFailOver->UseJournal == FALSE)
		{ // Failover No Journal 
			NTSTATUS tmpstatus;

			tmpstatus = sftk_lg_change_State( pSftk_Lg, pSftk_Lg->PrevState, SFTK_MODE_TRACKING, TRUE);
			if (!NT_SUCCESS(tmpstatus))
			{
				DebugPrint((DBG_ERROR, "sftk_lg_failover: Primary sftk_lg_change_State(lg %d) Failed %08x to go in tracking, ignoring, causes remote proto command will failover and put it into usertracking automatically.!!! \n",
								pFailOver->LgNum, tmpstatus));
			}
		} // Failover No Journal 

		goto done;
	} // if (LG_IS_PRIMARY_MODE(pSftk_Lg))

	if (LG_IS_SECONDARY_MODE(pSftk_Lg))
	{ // LG current role is primary 
		if (LG_IS_SECONDARY_WITH_TRACKING(pSftk_Lg))
		{ // we already running in failover mode.. so just return success
			status = STATUS_SUCCESS;
			DebugPrint((DBG_ERROR, "sftk_lg_failover: Secondary lg %d, already running in failover mode. returning success!!! \n",
								pFailOver->LgNum, status));
			goto done;
		}

		// If Journal Apply is ON then make sure it finishes before we goto failover.
		// TODO : Should we inform Journal Apply process to finish it fast ? means if any .NC file exist and its still 
		// getting used with incoming normal Writes, Should we do not apply this Journal .Nc File, and go for failover once
		// we apply rest of previouse Journal files ? if not then Journal Apply process will continue to run causes
		// New Normal data Writes will continue to arrived .... !!!! ?

		if (!LG_IS_APPLY_JOURNAL_ON(pSftk_Lg))
		{ // if Journal Apply is not running 
			if (LG_IS_JOURNAL_ON(pSftk_Lg))
			{ // if Journal is ON then only, start Journal Apply process
				pSftk_Lg->Secondary.DoFastMinimalJApply = TRUE; 
				KeSetEvent( &pSftk_Lg->Secondary.JApplyWorkEvent, 0, FALSE);
				// Journal Thread won't do any work if there is no .SRc or SNc file exist. it will trigger event and comeout
			}
		}

		// Implementation : Currently I think best things to do is to inform JApply process to start Failover as soon as 
		// possible with consistent data, means Do not apply last .NC file, when reaches to last .NC file then 
		// quite Journal Apply process, and inform here back that its done.
		
		if (LG_IS_APPLY_JOURNAL_ON(pSftk_Lg) )
		{ // JApply is Running
			// TODO Veera: use this field in JApply process, if this set do not apply last .NC file
			// then signal JApplyWorkDoneEvent event with quitting JApply process

			// TODO : Do we need timeout ? its better to have it, let's Caller pass this timeout
			pSftk_Lg->Secondary.DoFastMinimalJApply = TRUE; 
			status = KeWaitForSingleObject(	(PVOID) &pSftk_Lg->Role.JApplyWorkDoneEvent,
													Executive,
													KernelMode,
													FALSE,
													&pFailOver->TimeOutForJApplyprocess );
			if (status != STATUS_SUCCESS) // not-signalled
			{
				DebugPrint((DBG_ERROR, "sftk_lg_failover: Secondary lg %d, JApply timeout returning Error %08x.!!! \n",
								pFailOver->LgNum, status));
				goto done;
			}
			pSftk_Lg->Secondary.DoFastMinimalJApply = FALSE;	// reset after use this flag here only
		} // JApply is Running

		OS_ASSERT( !LG_IS_APPLY_JOURNAL_ON(pSftk_Lg));	// Journal Apply must stop by this time.

		// Now since Data on Disk is Consistent, we can go for Failover mode.
		// TODO : Do We need to create new Journal File, I do not think so, causes Existing journal file is fine.
		if (pFailOver->UseJournal == TRUE)
		{ // Use Journal is TRUE, 
			if (!LG_IS_JOURNAL_ON(pSftk_Lg))
			{	// if no journal mode, then make it Secondary in Journal mode, which will create new .NC file automatically
				// In apply process depends on primary send Proto command (start SR, creates .SR, else .Nc)
				pSftk_Lg->Role.JEnable = TRUE;
			}
			// else : it already set Journal ON
		}
		else
		{ // else : make is Journal OFf, Failover with No Journal

			// TODO Veera: TWrite thread will check Failover is ON and no Journal, So it will reject any data pkts 
			// from primary with Failover with No Journal error, This will put Primary in ProtoDefinedTracking Mode
			// till it get resets by commit or roll back IOCTL either side.
			pSftk_Lg->Role.JEnable = FALSE;
			// This will also keep any existing .i file, this .i file won't change to .c file till later SR 
			// completes in .Nc file once failover changed to Roll back or commit on either side.
		}

		// Now we are done with this, so just set failover fields.
		pSftk_Lg->Role.FailOver = TRUE;
	} // if (LG_IS_SECONDARY_MODE(pSftk_Lg))
done:
	return status;
} // sftk_lg_failover()

#endif