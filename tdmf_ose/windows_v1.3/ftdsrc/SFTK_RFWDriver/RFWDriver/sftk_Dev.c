/**************************************************************************************

Module Name: sftk_Dev.C   
Author Name: Parag sanghvi 
Description: All APIS related with SFTK_DEV struct
	
This software  is the licensed software of Softek Software Technology Corporation

Copyright (c) 2002 by Softek Software Technology Corporation

THIS SOFTWARE IS THE CONFIDENTIAL OR TRADE SECRET PROPERTY OF SOFTEK 
SOFTWARE TECHNOLOGY CORPORATION AND MAY ONLY BE USED UNDER LICENSE FROM 
SOFTEK SOFTWARE TECHNOLOGY CORPORATION

****************************************************************************************/
#include <sftk_main.h>

NTSTATUS
sftk_ctl_new_device(dev_t dev, int cmd, int arg, int flag)
{
    ftd_dev_info_t      *in_dev_info	= (ftd_dev_info_t *) arg;
	PSFTK_DEV           pSftkDev		= NULL;
    PSFTK_LG			pSftkLg			= NULL;	
	NTSTATUS			status			= STATUS_SUCCESS;

    // the caller needs to give us: info about the device and the group device number 
	pSftkDev = sftk_lookup_dev_by_devid( in_dev_info->devname );
    // pSftkDev = sftk_lookup_dev_by_cdev( in_dev_info->cdev );
	if (pSftkDev != NULL) 
    { // Dev is already existed
		if (pSftkDev->ConfigStart == FALSE)
		{ // Config start time Phase, Do Validation of existing LG 

			// Do Validation of input Dev paramteres with already existed SFTK_Dev !
			// Do according action if validation fails !!! 
			status = sftk_dev_validation_with_configFile( pSftkDev, in_dev_info);
			if (!NT_SUCCESS(status)) 
			{ // failed
				DebugPrint((DBG_ERROR, "sftk_ctl_new_device: sftk_dev_validation_with_configFile(Device - %s, LG Num %d) Failed with status 0x%08x !!! \n",
									in_dev_info->vdevname, pSftkDev->LGroupNumber, status));

				// we will be deleting this device.....
				return status;
			}
			// Now Check if this boot time, first call to create Dev during service start...if yes than do work accordingly
			if (pSftkDev->SftkLg == NULL) 
			{ // ops serious problem.....
				status = STATUS_INVALID_PARAMETER;
				DebugPrint((DBG_ERROR, "FIXME FIXME BUG ***** sftk_ctl_new_device: SftkDev Device - %s, LG Num %d already exist but pSftkDev->SftkLg 0x%08x == NULL !!! FIXME FIXME BUG **** Failing with status 0x%08x !!! \n",
									in_dev_info->vdevname, pSftkDev->LGroupNumber, pSftkDev, status));

				OS_ASSERT(FALSE);

				// we will be deleting this device.....
				return status;
			}

			if (OS_IsFlagSet( pSftkDev->SftkLg->flags, SFTK_LG_FLAG_DO_NOT_USED_PSTORE_FILE) == TRUE) 
			{ // open pstore now, and merge Bitmap from Pstore Bitmap 
				// open pstore file and merger bitmap here.....
				status = sftk_Update_Dev_from_pstore( pSftkDev->SftkLg, pSftkDev );
				if ( !NT_SUCCESS(status) ) 
				{ // failed to open or create pstore file...
					DebugPrint((DBG_ERROR, "sftk_ctl_new_dev:: sftk_Update_Dev_from_pstore( LGNum %d, VDevName %s) Failed with status 0x%08x !\n", 
											pSftkDev->LGroupNumber, in_dev_info->vdevname, status ));

					// Log Event 
					sftk_LogEventNum2Wchar1(GSftk_Config.DriverObject, MSG_REPL_DELETEING_DEV_DUE_TO_PSTORE, status, 
												0, pSftkDev->cdev, pSftkDev->SftkLg->LGroupNumber, pSftkDev->SftkLg->PStoreFileName.Buffer);

					DebugPrint((DBG_ERROR, "sftk_ctl_new_dev: Mark to Delete this Dev %s under LG %d due to PstoreFile Name can't create or opened !!\n", 
								in_dev_info->vdevname, pSftkDev->LGroupNumber ));

					// this Dev will get deleted later in Config_end....
					return status;
				} // failed, seriouse error !!!
			} // open pstore now, and merge Bitmap from Pstore Bitmap 

			pSftkDev->ConfigStart = TRUE;	// Already exist

			return STATUS_SUCCESS;
		} // Config start time Phase, Do Validation of existing LG 

		DebugPrint((DBG_ERROR, "sftk_ctl_new_device: specified cDev number = %d already exist !!! returning error 0x%08x \n",
							in_dev_info->cdev, STATUS_OBJECTID_EXISTS));
        return STATUS_OBJECTID_EXISTS;	
    } // Dev is already existed

	status = sftk_CreateInitialize_SftekDev( in_dev_info, &pSftkDev, TRUE, TRUE, FALSE);
	
    if (!NT_SUCCESS(status)) 
    { // failed
		DebugPrint((DBG_ERROR, "sftk_ctl_new_device: sftk_CreateInitialize_SftekDev(Device - %s) Failed with status 0x%08x !!! \n",
							in_dev_info->vdevname, status));
		return status;
    }
	else
	{ // success, added new device
		if(pSftkDev)
		{
			pSftkDev->ConfigStart = TRUE;	// Created new one

			if ( (pSftkDev->SftkLg) && 
				 (pSftkDev->SftkLg->ResetConnections == FALSE) && 
				 (sftk_lg_is_socket_alive( pSftkDev->SftkLg ) == TRUE) )
			{
				pSftkDev->SftkLg->ResetConnections = TRUE;
			}
		}
	}

    return status;
} // sftk_ctl_new_device()

NTSTATUS
sftk_dev_validation_with_configFile( PSFTK_DEV Sftk_Dev, ftd_dev_info_t *Dev_Info)
{
	NTSTATUS status = STATUS_IMAGE_CHECKSUM_MISMATCH;
	
	if ( Dev_Info->lgnum != (int) Sftk_Dev->LGroupNumber )
	{ // Failed::: TODO : What to do ? returned error ?? !!
		DebugPrint((DBG_ERROR, "sftk_dev_validation_with_configFile: FIXME FIXME (Dev_Info->lgnum 0x%08x != Sftk_Dev->LGroupNumber 0x%08x) Failed !! FIXME FIXME \n", 
						Dev_Info->lgnum, Sftk_Dev->LGroupNumber));
		// goto done;	// seriouse Error !!!
	}
	// Check Dev_Info->localcdev	= Sftk_Dev->localcdisk;
	if (Dev_Info->cdev != Sftk_Dev->cdev )
	{ // Failed::: TODO : What to do ? returned error ?? !!
		DebugPrint((DBG_ERROR, "sftk_dev_validation_with_configFile: FIXME FIXME (Dev_Info->cdev 0x%08x != Sftk_Dev->cdev 0x%08x) Failed !! FIXME FIXME \n", 
						Dev_Info->cdev, Sftk_Dev->cdev));
	}
	if (Dev_Info->bdev != Sftk_Dev->bdev )
	{ // Failed::: TODO : What to do ? returned error ?? !!
		DebugPrint((DBG_ERROR, "sftk_dev_validation_with_configFile: FIXME FIXME (Dev_Info->bdev 0x%08x != Sftk_Dev->bdev 0x%08x) Failed !! FIXME FIXME \n", 
						Dev_Info->bdev, Sftk_Dev->bdev));
	}
	if (Dev_Info->disksize != Sftk_Dev->Disksize )
	{ // Failed::: TODO : What to do ? returned error ?? !!
		DebugPrint((DBG_ERROR, "sftk_dev_validation_with_configFile: FIXME FIXME (Dev_Info->Disksize 0x%08x != Sftk_Dev->Disksize 0x%08x) Failed !! FIXME FIXME \n", 
						Dev_Info->disksize, Sftk_Dev->Disksize));
	}
	if (Dev_Info->lrdbsize32 != (int) Sftk_Dev->Lrdb.len32)
	{ // Failed::: TODO : What to do ? returned error ?? !!
		DebugPrint((DBG_ERROR, "sftk_dev_validation_with_configFile: FIXME FIXME (Dev_Info->lrdbsize32 0x%08x != Sftk_Dev->Lrdb.len32 0x%08x) Failed !! FIXME FIXME \n", 
						Dev_Info->lrdbsize32, Sftk_Dev->Lrdb.len32));
	}
	if (Dev_Info->hrdbsize32 != (int) Sftk_Dev->Hrdb.len32)
	{ // Failed::: TODO : What to do ? returned error ?? !!
		DebugPrint((DBG_ERROR, "sftk_dev_validation_with_configFile: FIXME FIXME (Dev_Info->hrdbsize32 0x%08x != Sftk_Dev->Hrdb.len32 0x%08x) Failed !! FIXME FIXME \n", 
						Dev_Info->hrdbsize32, Sftk_Dev->Hrdb.len32));
	}
	// Check Dev_Info->lrdb_res	= Sftk_Dev->Lrdb.bitsize;	
	// Check Dev_Info->hrdb_res	= Sftk_Dev->Hrdb.bitsize;
	// Check ditemp.lrdb_numbits		= Sftk_Dev->Lrdb.numbits;
	// Check ditemp.hrdb_numbits		= Sftk_Dev->Hrdb.numbits;
	if (Dev_Info->statsize != (int) Sftk_Dev->statsize )
	{ // Failed::: TODO : What to do ? returned error ?? !!
		DebugPrint((DBG_ERROR, "sftk_dev_validation_with_configFile: FIXME FIXME (Dev_Info->statsize 0x%08x != Sftk_Dev->statsize 0x%08x) Failed !! FIXME FIXME \n", 
						Dev_Info->statsize, Sftk_Dev->statsize));
	}
	// Check Dev_Info->lrdb_offset = Sftk_Dev->lrdb_offset 
	if (strcmp(Dev_Info->devname,Sftk_Dev->Devname) != 0 ) 
	{ // Failed::: TODO : What to do ? returned error ?? !!
		DebugPrint((DBG_ERROR, "sftk_dev_validation_with_configFile: FIXME FIXME (Dev_Info->devname %s != Sftk_Dev->Devname %s) Failed !! FIXME FIXME \n", 
						Dev_Info->devname, Sftk_Dev->Devname));
	}
	if (strcmp(Dev_Info->vdevname,Sftk_Dev->Vdevname) != 0 ) 
	{ // Failed::: TODO : What to do ? returned error ?? !!
		DebugPrint((DBG_ERROR, "sftk_dev_validation_with_configFile: FIXME FIXME (Dev_Info->vdevname %s != Sftk_Dev->Vdevname %s) Failed !! FIXME FIXME \n", 
						Dev_Info->vdevname, Sftk_Dev->Vdevname));
	}
	if (Dev_Info->bUniqueVolumeIdValid == TRUE) 
	{ // Compare UniqueVolumeId
		if (Sftk_Dev->bUniqueVolumeIdValid == FALSE) 
		{
			DebugPrint((DBG_ERROR, "sftk_dev_validation_with_configFile: FIXME FIXME (Dev_Info->bUniqueVolumeIdValid TRUE But Sftk_Dev->bUniqueVolumeIdValid == FALSE) Failed !! FIXME FIXME \n"));
		}
		else
		{
			if (Dev_Info->UniqueIdLength != Sftk_Dev->UniqueIdLength )
			{ // Failed::: TODO : What to do ? returned error ?? !!
				DebugPrint((DBG_ERROR, "sftk_dev_validation_with_configFile: FIXME FIXME (Dev_Info->UniqueIdLength 0x%08x != Sftk_Dev->UniqueIdLength 0x%08x) Failed !! FIXME FIXME \n", 
								Dev_Info->UniqueIdLength, Sftk_Dev->UniqueIdLength));
			}
			else
			{
				if (RtlCompareMemory(Dev_Info->UniqueId, Sftk_Dev->UniqueId, Sftk_Dev->UniqueIdLength) != Sftk_Dev->UniqueIdLength)
				{ // Failed::: TODO : What to do ? returned error ?? !!
					DebugPrint((DBG_ERROR, "sftk_dev_validation_with_configFile: FIXME FIXME (Dev_Info->UniqueId %s != Sftk_Dev->UniqueId %s) Failed !! FIXME FIXME \n", 
											Dev_Info->UniqueId, Sftk_Dev->UniqueId));
				}
			}
		}
	} // Compare UniqueVolumeId
	if (Dev_Info->bSignatureUniqueVolumeIdValid == TRUE) 
	{ // Compare UniqueVolumeId
		if (Sftk_Dev->bSignatureUniqueVolumeIdValid == FALSE) 
		{
			DebugPrint((DBG_ERROR, "sftk_dev_validation_with_configFile: FIXME FIXME (Dev_Info->bSignatureUniqueVolumeIdValid TRUE But Sftk_Dev->bSignatureUniqueVolumeIdValid == FALSE) Failed !! FIXME FIXME \n"));
		}
		else
		{
			if (Dev_Info->SignatureUniqueIdLength != Sftk_Dev->SignatureUniqueIdLength )
			{ // Failed::: TODO : What to do ? returned error ?? !!
				DebugPrint((DBG_ERROR, "sftk_dev_validation_with_configFile: FIXME FIXME (Dev_Info->SignatureUniqueIdLength 0x%08x != Sftk_Dev->SignatureUniqueIdLength 0x%08x) Failed !! FIXME FIXME \n", 
								Dev_Info->SignatureUniqueIdLength, Sftk_Dev->SignatureUniqueIdLength));
			}
			else
			{
				if (RtlCompareMemory(Dev_Info->SignatureUniqueId, Sftk_Dev->SignatureUniqueId, Sftk_Dev->SignatureUniqueIdLength) != Sftk_Dev->SignatureUniqueIdLength)
				{ // Failed::: TODO : What to do ? returned error ?? !!
					DebugPrint((DBG_ERROR, "sftk_dev_validation_with_configFile: FIXME FIXME (Dev_Info->SignatureUniqueId %s != Sftk_Dev->SignatureUniqueId %s) Failed !! FIXME FIXME \n", 
											Dev_Info->SignatureUniqueId, Sftk_Dev->SignatureUniqueId));
				}
			}
		}
	} // Compare UniqueVolumeId

	if (Dev_Info->bSuggestedDriveLetterLinkValid == TRUE) 
	{ // Compare UniqueVolumeId
		if (Sftk_Dev->bSuggestedDriveLetterLinkValid == FALSE) 
		{
			DebugPrint((DBG_ERROR, "sftk_dev_validation_with_configFile: FIXME FIXME (Dev_Info->bSuggestedDriveLetterLinkValid TRUE But Sftk_Dev->bSuggestedDriveLetterLinkValid == FALSE) Failed !! FIXME FIXME \n"));
		}
		else
		{
			if (Dev_Info->SuggestedDriveLetterLinkLength != Sftk_Dev->SuggestedDriveLetterLinkLength )
			{ // Failed::: TODO : What to do ? returned error ?? !!
				DebugPrint((DBG_ERROR, "sftk_dev_validation_with_configFile: FIXME FIXME (Dev_Info->SuggestedDriveLetterLinkLength 0x%08x != Sftk_Dev->SuggestedDriveLetterLinkLength 0x%08x) Failed !! FIXME FIXME \n", 
								Dev_Info->SuggestedDriveLetterLinkLength, Sftk_Dev->SuggestedDriveLetterLinkLength));
			}
			else
			{
				if (RtlCompareMemory(Dev_Info->SuggestedDriveLetterLink, Sftk_Dev->SuggestedDriveLetterLink, Sftk_Dev->SuggestedDriveLetterLinkLength) != Sftk_Dev->SuggestedDriveLetterLinkLength)
				{ // Failed::: TODO : What to do ? returned error ?? !!
					DebugPrint((DBG_ERROR, "sftk_dev_validation_with_configFile: FIXME FIXME (Dev_Info->SuggestedDriveLetterLink %s != Sftk_Dev->SuggestedDriveLetterLink %s) Failed !! FIXME FIXME \n", 
											Dev_Info->SuggestedDriveLetterLink, Sftk_Dev->SuggestedDriveLetterLink));
				}
			}
		}
	} // Compare UniqueVolumeId

	status = STATUS_SUCCESS; // For time being ignoring all kind of validation error!!! Fixme fixme later if needed
//done:	
	return status;
} // sftk_dev_validation_with_configFile()

NTSTATUS
sftk_ctl_del_device(dev_t dev, int cmd, int arg, int flag)
{
    // dev_t				cdev			= *(dev_t *)arg;	// old code same as old driver
	stat_buffer_t		*sbptr			= (stat_buffer_t *)arg;
	PSFTK_DEV           pSftkDev		= NULL;
	NTSTATUS			status			= STATUS_SUCCESS;

    // the caller needs to give us: info about the device and the group device number 
	pSftkDev = sftk_lookup_dev_by_devid( sbptr->DevId );
    // pSftkDev = sftk_lookup_dev_by_cdev( cdev );
	if (pSftkDev == NULL) 
    {
		DebugPrint((DBG_ERROR, "sftk_ctl_del_device: specified cDev number = %d and DevID %s NOT exist !!! returning error 0x%08x \n",
								sbptr->dev_num, sbptr->DevId, STATUS_DEVICE_DOES_NOT_EXIST));
        return STATUS_DEVICE_DOES_NOT_EXIST;	
    }

	status = sftk_delete_SftekDev( pSftkDev, TRUE, TRUE);

    if (!NT_SUCCESS(status)) 
    { // failed
		DebugPrint((DBG_ERROR, "sftk_ctl_del_device: sftk_delte_SftekDev(Device - %s) Failed with status 0x%08x !!! \n",
							pSftkDev->Vdevname, status));
		return status;	
    }

    return status;
} // sftk_ctl_del_device()

NTSTATUS
sftk_delete_SftekDev( IN OUT 	PSFTK_DEV	SftkDev, BOOLEAN bGrabLGLock, BOOLEAN bGrabGlobalLock )
{
	NTSTATUS	status = STATUS_SUCCESS;

	OS_ASSERT( SftkDev != NULL );
	OS_ASSERT( SftkDev->SftkLg != NULL );

	DebugPrint((DBG_IOCTL, "sftk_delete_SftekDev: (Device - %s, Dev Num 08%0x) LG Number %d!!! \n",
							SftkDev->Vdevname, SftkDev->cdev, SftkDev->LGroupNumber));

	// Detach Device from Disk 
	if (SftkDev->DevExtension)
	{ // Disk Device is attached so just remove info from Deisk Dev Ext
		SftkDev->DevExtension->Sftk_dev = NULL;
	}
	SftkDev->DevExtension = NULL;

	// Terminate Threads
	if ( OS_IsFlagSet( SftkDev->Flags, SFTK_DEV_FLAG_MASTER_THREAD_STARTED) )
		sftk_Terminate_DevThread( SftkDev);

	// Remove SftkDev from SFTK_CONFIG Device list and its Logical Group list. 

	// Remove this created device from its resepctive LG list 
	if (bGrabLGLock == TRUE)
		OS_ACQUIRE_LOCK( &SftkDev->SftkLg->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	RemoveEntryList( &SftkDev->LgDev_Link );
	SftkDev->SftkLg->LgDev_List.NumOfNodes --;
	if (bGrabLGLock == TRUE)
		OS_RELEASE_LOCK( &SftkDev->SftkLg->Lock, NULL);

	// Remove this created device from its resepctive SFTK_CONFIG list 
	if (bGrabGlobalLock == TRUE)
		OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
	RemoveEntryList( &SftkDev->SftkDev_Link );
	GSftk_Config.SftkDev_List.NumOfNodes --;
	if (bGrabGlobalLock == TRUE)
		OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);

	// Free other memory
	if (SftkDev->statbuf)
		OS_FreeMemory(SftkDev->statbuf);

	// Delete Bitmaps
	sftk_Delete_DevBitmaps(SftkDev);

	if ( OS_IsFlagSet( SftkDev->Flags, SFTK_DEV_FLAG_REG_CREATED) )
	{
		status = sftk_dev_Delete_RegKey( SftkDev);
		if (!NT_SUCCESS(status)) 
		{
			DebugPrint((DBG_ERROR, "sftk_delete_SftekDev: sftk_dev_Delete_RegKey( LG %d Dev %s ) Failed with status 0x%08x \n", 
						SftkDev->LGroupNumber, SftkDev->Vdevname, status));
		}
	}

	if ( OS_IsFlagSet( SftkDev->Flags, SFTK_DEV_FLAG_PSTORE_FILE_ADDED) )
	{
		status = sftk_Delete_dev_in_pstorefile( SftkDev );
		if (!NT_SUCCESS(status)) 
		{
			DebugPrint((DBG_ERROR, "sftk_delete_SftekDev: sftk_Delete_dev_in_pstorefile( LG %d Dev %s PstoreFile Name %S) Failed with status 0x%08x \n", 
						SftkDev->LGroupNumber, SftkDev->Vdevname,SftkDev->SftkLg->PStoreFileName.Buffer, status));
		}
	}

	if (SftkDev->PsDev)
		OS_FreeMemory(SftkDev->PsDev);
	
	OS_FreeMemory(SftkDev);

	return status;
} // sftk_delete_SftekDev()

PSFTK_DEV
sftk_lookup_dev_by_cdev(IN ULONG Cdev)
{
	PSFTK_DEV		pSftkDev	= NULL;
	PLIST_ENTRY		plistEntry	= NULL;

	OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_SHARED, TRUE, NULL);

	for( plistEntry = GSftk_Config.SftkDev_List.ListEntry.Flink;
		 plistEntry != &GSftk_Config.SftkDev_List.ListEntry;
		 plistEntry = plistEntry->Flink )
	{ // for :scan thru each and every logical group list 
		pSftkDev = CONTAINING_RECORD( plistEntry, SFTK_DEV, SftkDev_Link);

		if (pSftkDev->cdev == Cdev)
		{ // Found it
			OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
			return pSftkDev;
		}
	} // for : scan thru each and every logical group list 

	OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
	return NULL;
} // sftk_lookup_dev_by_cdev

PSFTK_DEV
sftk_lookup_dev_by_cdev_in_SftkLG(IN PSFTK_LG Sftk_LG, IN ULONG Cdev)
{
	PSFTK_DEV		pSftkDev	= NULL;
	PLIST_ENTRY		plistEntry	= NULL;

	OS_ACQUIRE_LOCK( &Sftk_LG->Lock, OS_ACQUIRE_SHARED, TRUE, NULL);

	for( plistEntry = Sftk_LG->LgDev_List.ListEntry.Flink;
		 plistEntry != &Sftk_LG->LgDev_List.ListEntry;
		 plistEntry = plistEntry->Flink )
	{ // for :scan thru each and every logical group list 
		pSftkDev = CONTAINING_RECORD( plistEntry, SFTK_DEV, LgDev_Link);

		if (pSftkDev->cdev == Cdev)
		{ // Found it
			OS_RELEASE_LOCK( &Sftk_LG->Lock, NULL);
			return pSftkDev;
		}
	} // for : scan thru each and every logical group list 

	OS_RELEASE_LOCK( &Sftk_LG->Lock, NULL);
	return NULL;
} // sftk_lookup_dev_by_cdev_in_SftkLG

PSFTK_DEV
sftk_lookup_dev_by_devid(IN PCHAR DevIdString)
{
	PSFTK_DEV		pSftkDev	= NULL;
	PLIST_ENTRY		plistEntry	= NULL;
	STRING          inDevString, kernelDevString;

	RtlInitAnsiString(&inDevString, DevIdString);

	OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_SHARED, TRUE, NULL);

	for( plistEntry = GSftk_Config.SftkDev_List.ListEntry.Flink;
		 plistEntry != &GSftk_Config.SftkDev_List.ListEntry;
		 plistEntry = plistEntry->Flink )
	{ // for :scan thru each and every logical group list 
		pSftkDev = CONTAINING_RECORD( plistEntry, SFTK_DEV, SftkDev_Link);

		RtlInitAnsiString(&kernelDevString, pSftkDev->Devname);
		// if (stricmp( pSftkDev->Devname, DevIdString) == 0)
		if (RtlEqualString( &kernelDevString, &inDevString, TRUE) == TRUE)
		{ // Found it
			OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
			return pSftkDev;
		}
	} // for : scan thru each and every logical group list 

	OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
	return NULL;
} // sftk_lookup_dev_by_devid


// IgnoreDevicePresence - TRUE means create SFTK_DEV even though device 
// is not presence - Boot time scenereio
NTSTATUS
sftk_CreateInitialize_SftekDev( IN		ftd_dev_info_t  *In_dev_info, 
								IN OUT 	PSFTK_DEV		*Sftk_Dev,
								IN		BOOLEAN			UpdatePstoreFile,
								IN		BOOLEAN			UpdateRegKey,
								IN		BOOLEAN			IgnoreDevicePresence)	
{
	NTSTATUS                status					= STATUS_SUCCESS;
	PDEVICE_EXTENSION		pAttachedDevExt			= NULL;
    PSFTK_DEV               pSftkDev				= NULL;	
	PSFTK_LG				pSftkLg					= NULL;	
	BOOLEAN                 InitializedDeviceObject = FALSE;

	*Sftk_Dev = NULL;

	// We need to check if it exists already, if so say return error
    pSftkDev = sftk_lookup_dev_by_devid( In_dev_info->devname );
    if (pSftkDev != NULL) 
    { // LG Already exist
		DebugPrint((DBG_ERROR, "sftk_CreateInitialize_SftekDev: Devname %s Already Exist !! Failed with status 0x%08x !!! \n",
									In_dev_info->devname, STATUS_OBJECTID_EXISTS));
		return STATUS_OBJECTID_EXISTS;	// LG exist return error
	}
	pSftkDev = NULL;

    try 
    {
		// Find Specified Logical Group in kernel
#if TARGET_SIDE
		pSftkLg = sftk_lookup_lg_by_lgnum( In_dev_info->lgnum, In_dev_info->lgCreationRole );
#else
		pSftkLg = sftk_lookup_lg_by_lgnum( In_dev_info->lgnum );
#endif
		if (pSftkLg == NULL)
		{
			status = STATUS_DEVICE_DOES_NOT_EXIST;	// specific error, same as previous version of driver...
			DebugPrint((DBG_ERROR, "sftk_CreateInitialize_SftekDev:: sftk_lookup_lg_by_lgnum(LGNum %d, device- %s) Unable to find LG, returning 0x%08x !", 
								In_dev_info->lgnum, In_dev_info->devname, status ));
			try_return(status);
		}

		// Find Disk Filter Attached Device for specified input device.
		pAttachedDevExt = sftk_find_attached_deviceExt( In_dev_info, TRUE );

        if (pAttachedDevExt == NULL) 
        { // failed to create a device object; cannot do much now.
			if (IgnoreDevicePresence == FALSE)
			{
				status = STATUS_UNSUCCESSFUL;
				DebugPrint((DBG_ERROR, "sftk_CreateInitialize_SftekDev:: sftk_find_attached_deviceExt(device - %s) Unable to find the device, returning 0x%08x !", 
									In_dev_info->devname, status ));
				try_return(status);
			}
        }

		/*
		// Check this condition if requires, I Don't think so, cause's we already 
		// attached device at Disk Level so its irrelevent to check this...
        if ((!diskDeviceObject->Vpb) || (diskDeviceObject->Vpb->Flags & VPB_MOUNTED)) 
        { // Check if this device is already mounted, Device is already mounted...
            DebugPrint((DBG_ERROR, "The device %s is already mounted!", In_dev_info->devname));
            try_return(status = STATUS_UNSUCCESSFUL);	// Assume this device has already been attached.
        }
		*/

		// Allocate pSftkDev Memory 
		pSftkDev = OS_AllocMemory( NonPagedPool, sizeof(SFTK_DEV) );
		if (pSftkDev == NULL)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			DebugPrint((DBG_ERROR, "sftk_CreateInitialize_SftekDev:: OS_AllocMemory(size - %d) Failed, returning 0x%08x !", 
								sizeof(SFTK_DEV), status ));
            try_return(status);
		}
		OS_ZeroMemory( pSftkDev, sizeof(SFTK_DEV) );

		// Initialize SFTK_DEV structures 
		pSftkDev->NodeId.NodeType = NODE_TYPE_SFTK_DEV;
		pSftkDev->NodeId.NodeSize = sizeof(SFTK_DEV);

		pSftkDev->DevExtension	  = pAttachedDevExt;

		KeInitializeEvent( &pSftkDev->PnpEventDiskArrived, SynchronizationEvent, FALSE);

		OS_INITIALIZE_LOCK( &pSftkDev->Lock, OS_ERESOURCE_LOCK, NULL);

		InitializeListHead( &pSftkDev->LgDev_Link );
		InitializeListHead( &pSftkDev->SftkDev_Link );

		// Copy Create time Caller's Device Info into Driver's Sftk_DEv infor, this is very useful for PNP Removal/Arrival 
		// Auto Reattached Process.
		RtlCopyMemory( pSftkDev->Devname, In_dev_info->devname, strlen(In_dev_info->devname) );
		RtlCopyMemory( pSftkDev->Vdevname, In_dev_info->vdevname, strlen(In_dev_info->vdevname) );

		// Added the Remote Device Name this will be sent at handshake time
		RtlCopyMemory( pSftkDev->strRemoteDeviceName, In_dev_info->strRemoteDeviceName, strlen(In_dev_info->strRemoteDeviceName) );

		pSftkDev->bUniqueVolumeIdValid	= In_dev_info->bUniqueVolumeIdValid;
		pSftkDev->UniqueIdLength		= In_dev_info->UniqueIdLength;
		RtlCopyMemory( pSftkDev->UniqueId, In_dev_info->UniqueId, sizeof(In_dev_info->UniqueId) );

		pSftkDev->bSignatureUniqueVolumeIdValid	= In_dev_info->bSignatureUniqueVolumeIdValid;
		pSftkDev->SignatureUniqueIdLength		= In_dev_info->SignatureUniqueIdLength;
		RtlCopyMemory( pSftkDev->SignatureUniqueId, In_dev_info->SignatureUniqueId, sizeof(In_dev_info->SignatureUniqueId) );

		pSftkDev->bSuggestedDriveLetterLinkValid = In_dev_info->bSuggestedDriveLetterLinkValid;
		pSftkDev->SuggestedDriveLetterLinkLength = In_dev_info->SuggestedDriveLetterLinkLength;
		RtlCopyMemory(	pSftkDev->SuggestedDriveLetterLink, In_dev_info->SuggestedDriveLetterLink, 
						sizeof(In_dev_info->SuggestedDriveLetterLink) );

		pSftkDev->LGroupNumber	= pSftkLg->LGroupNumber;

		pSftkDev->SftkLg	= pSftkLg;
		pSftkDev->Flags		= 0;
		pSftkDev->Disksize	= In_dev_info->disksize;

		pSftkDev->localcdisk = pSftkDev->cdev = In_dev_info->cdev;
	    pSftkDev->localbdisk = pSftkDev->bdev = In_dev_info->bdev;

		pSftkDev->lrdb_offset= In_dev_info->lrdb_offset;	

		pSftkDev->statsize	= In_dev_info->statsize; 
		pSftkDev->statbuf	= OS_AllocMemory( NonPagedPool, pSftkDev->statsize);
		if (pSftkDev->statbuf == NULL)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			DebugPrint((DBG_ERROR, "sftk_CreateInitialize_SftekDev:: OS_AllocMemory(for SFTK_DEV->statbuf, statesize - %d) Failed, returning 0x%08x !", 
								pSftkDev->statsize, status ));
            try_return(status);
		}
    
		OS_SetFlag( pSftkDev->Flags, SFTK_DEV_FLAG_SRC_DEVICE_ONLINE);	// since devext is valid value...
		
		pSftkDev->PendingIOs			= 0;
		pSftkDev->PendingIOs_highwater	= 0;
		pSftkDev->PendingIOs_Lowwater	= 0;
		
		status = sftk_Create_DevBitmaps( pSftkDev, In_dev_info);
		if (!NT_SUCCESS(status))
		{ // Failed
			DebugPrint((DBG_ERROR, "sftk_CreateInitialize_SftekDev:: sftk_Initialize_DevBitmaps(Device %s) Failed, returning 0x%08x !", 
								In_dev_info->vdevname, status ));
            try_return(status);
		}

		if (UpdateRegKey == TRUE)
		{ // Add current SFTK_DEV into Registry
			// we create Specify Pstore File for current LG, 
			// While creating we must Format and write Pstore file too
			// As new device gets added we also update this pstore file.
			status = sftk_dev_Create_RegKey( pSftkDev);
			if (!NT_SUCCESS(status)) 
			{
				DebugPrint((DBG_ERROR, "sftk_CreateInitialize_SftekDev: sftk_dev_Create_RegKey( LG %d Dev %s PstoreFile Name %S) Failed with status 0x%08x \n", 
							pSftkDev->SftkLg->LGroupNumber, In_dev_info->vdevname,
							pSftkDev->SftkLg->PStoreFileName.Buffer, 
							status));
				// Returning error, no need to update Log Event !!
				try_return(status);
			}
			OS_SetFlag( pSftkDev->Flags, SFTK_DEV_FLAG_REG_CREATED);
		}

		if (UpdatePstoreFile == TRUE)
		{ // Add current SFTK_DEV into Pstore File
			// we create Specify Pstore File for current LG, 
			// While creating we must Format and write Pstore file too
			// As new device gets added we also update this pstore file.
			status = sftk_create_dev_in_pstorefile( pSftkDev->SftkLg, pSftkDev);
			if (!NT_SUCCESS(status)) 
			{
				DebugPrint((DBG_ERROR, "sftk_CreateInitialize_SftekDev: sftk_create_dev_in_pstorefile( LG %d Dev %s PstoreFile Name %S) Failed with status 0x%08x \n", 
							pSftkDev->SftkLg->LGroupNumber, In_dev_info->vdevname,
							pSftkDev->SftkLg->PStoreFileName.Buffer, 
							status));
				// Returning error, no need to update Log Event !!
				try_return(status);
			}
			OS_SetFlag( pSftkDev->Flags, SFTK_DEV_FLAG_PSTORE_FILE_ADDED);
		}
		
		// PSTORE thread and its relative informations
		status = sftk_Create_DevThread( pSftkDev, In_dev_info);
		if (!NT_SUCCESS(status))
		{ // Failed
			DebugPrint((DBG_ERROR, "sftk_CreateInitialize_SftekDev:: sftk_Initialize_DevBitmaps(Device %s) Failed, returning 0x%08x !", 
								In_dev_info->vdevname, status ));
            try_return(status);
		}
		OS_SetFlag( pSftkDev->Flags, SFTK_DEV_FLAG_MASTER_THREAD_STARTED);

		// insert Device into SFTK_CONFIG Device list and its Logical Group list. 
		// Add this created device into Global Anchor SFTK_CONFIG Device List
		OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
		InsertTailList( &GSftk_Config.SftkDev_List.ListEntry, &pSftkDev->SftkDev_Link );
		GSftk_Config.SftkDev_List.NumOfNodes ++;
		OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);

		// Add this created device into its resepctive LG list 
		OS_ACQUIRE_LOCK( &pSftkLg->Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);
		InsertTailList( &pSftkLg->LgDev_List.ListEntry, &pSftkDev->LgDev_Link );
		pSftkLg->LgDev_List.NumOfNodes ++;
		OS_RELEASE_LOCK( &pSftkLg->Lock, NULL);
		
        // We are there now. All I/O requests will start being redirected to
        // us until we detach ourselves.
		if (pAttachedDevExt != NULL)
		{
			pAttachedDevExt->Sftk_dev = pSftkDev;
		}
		else
		{ // since attached device is OFFLINE, put this device into removed devlist so PNP add can reattach device back later
			OS_ClearFlag( pSftkDev->Flags, SFTK_DEV_FLAG_SRC_DEVICE_ONLINE);	// since devext is valid value... 
			pSftkDev->DevExtension = NULL;
			KeClearEvent( &pSftkDev->PnpEventDiskArrived );

			// Add this in Global Removed device list
			OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_EXCLUSIVE, TRUE, NULL);

			InsertTailList( &GSftk_Config.SftkDev_PnpRemovedList.ListEntry, &pSftkDev->SftkDev_PnpRemovedLink );
			GSftk_Config.SftkDev_PnpRemovedList.NumOfNodes ++;

			OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);
		}

		pSftkDev->ConfigStart = TRUE;	// Created new one
		status = STATUS_SUCCESS;	// successfully created

		try_exit:   NOTHING;
    } 
    finally 
    {
        if (!NT_SUCCESS(status)) 
        { // Failed
			NTSTATUS	tmpStatus;
			if (pSftkDev)
			{
				if ( OS_IsFlagSet( pSftkDev->Flags, SFTK_DEV_FLAG_MASTER_THREAD_STARTED) )
					sftk_Terminate_DevThread( pSftkDev);

				if ( OS_IsFlagSet( pSftkDev->Flags, SFTK_DEV_FLAG_REG_CREATED) )
				{
					tmpStatus = sftk_dev_Delete_RegKey( pSftkDev);
					if (!NT_SUCCESS(tmpStatus)) 
					{
						DebugPrint((DBG_ERROR, "sftk_CreateInitialize_SftekDev: sftk_dev_Delete_RegKey( LG %d Dev %s PstoreFile Name %S) Failed with tmpStatus 0x%08x \n", 
									pSftkDev->SftkLg->LGroupNumber, In_dev_info->vdevname,
									pSftkDev->SftkLg->PStoreFileName.Buffer, 
									tmpStatus));
					}
				}

				if ( OS_IsFlagSet( pSftkDev->Flags, SFTK_DEV_FLAG_PSTORE_FILE_ADDED) )
				{
					tmpStatus = sftk_Delete_dev_in_pstorefile( pSftkDev);
					if (!NT_SUCCESS(tmpStatus)) 
					{
						DebugPrint((DBG_ERROR, "sftk_CreateInitialize_SftekDev: sftk_Delete_dev_in_pstorefile( LG %d Dev %s PstoreFile Name %S) Failed with tmpStatus 0x%08x \n", 
									pSftkDev->SftkLg->LGroupNumber, In_dev_info->vdevname,
									pSftkDev->SftkLg->PStoreFileName.Buffer, 
									tmpStatus));
					}
				}

				if (pSftkDev->statbuf)
					OS_FreeMemory(pSftkDev->statbuf);

				sftk_Delete_DevBitmaps(pSftkDev);
				OS_FreeMemory(pSftkDev);
				pSftkDev = NULL;	// return NULL on Failure
			}
        } // Failed 
    }

	*Sftk_Dev = pSftkDev;

    return status;
} // sftk_CreateInitialize_SftekDev()

NTSTATUS
sftk_ctl_get_dev_state_buffer(dev_t dev, int cmd, int arg, int flag)
{
	PSFTK_LG		pSftk_Lg	= NULL;
	PSFTK_DEV		pSftk_Dev	= NULL;
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

	// Retrieve Sftk Device
    // pSftk_Dev	= sftk_lookup_dev_by_cdev( sbptr->dev_num );
	pSftk_Dev	= sftk_lookup_dev_by_devid( sbptr->DevId );
    if (pSftk_Dev == NULL) 
    {
		DebugPrint((DBG_ERROR, "sftk_ctl_get_dev_state_buffer: sftk_lookup_dev_by_devid(%s) specified dev_num = %d NOT exist !!! returning error 0x%08x \n",
							sbptr->DevId, sbptr->dev_num, STATUS_DEVICE_DOES_NOT_EXIST));
        return STATUS_DEVICE_DOES_NOT_EXIST;	
    }
    // minor = getminor(sbptr->dev_num);	// get the device state 

    if (sbptr->len != (LONG) pSftk_Dev->statsize)
    {
		DebugPrint((DBG_ERROR, "sftk_ctl_get_dev_state_buffer: dev_num = %d has sbptr->len %d != pSftk_Dev->statsize %d !!! returning error 0x%08x \n",
							sbptr->dev_num, sbptr->len, pSftk_Dev->statsize, STATUS_INVALID_PARAMETER));
        return STATUS_INVALID_PARAMETER;
    }

	RtlCopyMemory( sbptr->addr, pSftk_Dev->statbuf, sbptr->len); 

    return STATUS_SUCCESS;;
} // sftk_ctl_get_dev_state_buffer()

NTSTATUS
sftk_ctl_set_dev_state_buffer(dev_t dev, int cmd, int arg, int flag)
{
	PSFTK_LG		pSftk_Lg	= NULL;
	PSFTK_DEV		pSftk_Dev	= NULL;
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

	// Retrieve Sftk Device
	pSftk_Dev	= sftk_lookup_dev_by_devid( sbptr->DevId );
    // pSftk_Dev	= sftk_lookup_dev_by_cdev( sbptr->dev_num );
    if (pSftk_Dev == NULL) 
    {
		DebugPrint((DBG_ERROR, "sftk_ctl_get_dev_state_buffer: sftk_lookup_dev_by_devid(%s) specified dev_num = %d NOT exist !!! returning error 0x%08x \n",
							sbptr->DevId, sbptr->dev_num, STATUS_DEVICE_DOES_NOT_EXIST));
        return STATUS_DEVICE_DOES_NOT_EXIST;	
    }
    // minor = getminor(sbptr->dev_num);	// get the device state 

    if (sbptr->len != (LONG) pSftk_Dev->statsize)
    {
		DebugPrint((DBG_ERROR, "sftk_ctl_get_dev_state_buffer: dev_num = %d has sbptr->len %d != pSftk_Dev->statsize %d !!! returning error 0x%08x \n",
							sbptr->dev_num, sbptr->len, pSftk_Dev->statsize, STATUS_INVALID_PARAMETER));
        return STATUS_INVALID_PARAMETER;
    }

	RtlCopyMemory( pSftk_Dev->statbuf, sbptr->addr, sbptr->len); 

    return STATUS_SUCCESS;;
} // sftk_ctl_set_dev_state_buffer()

NTSTATUS
sftk_ctl_get_device_nums(dev_t dev, int cmd, int arg, int flag)
{
    ftd_devnum_t	*devnum = (ftd_devnum_t *)arg;
	LONG			index;

	OS_ACQUIRE_LOCK( &GSftk_Config.Lock, OS_ACQUIRE_SHARED, TRUE, NULL);
	index = GSftk_Config.SftkDev_List.NumOfNodes;
	OS_RELEASE_LOCK( &GSftk_Config.Lock, NULL);

	devnum->c_major = index;
    devnum->b_major = index;

	return STATUS_SUCCESS;
} // sftk_ctl_get_device_nums()
